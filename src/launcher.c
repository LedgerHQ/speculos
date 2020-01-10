#define _GNU_SOURCE
/* fix for the following error on Windows (WSL):
 * fstat: Value too large for defined data type */
#define _FILE_OFFSET_BITS 64
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <execinfo.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/types.h>

#include "emulate.h"

#define HANDLER_STACK_SIZE     (SIGSTKSZ*4)

// Idx Name          Size      VMA       LMA       File off  Algn
//  0 .text         00011f00  c0d00000  c0d00000  00010000  2**2
//                  CONTENTS, ALLOC, LOAD, READONLY, CODE
//  1 .data         00000000  d0000000  d0000000  00021f00  2**0
//                  CONTENTS, ALLOC, LOAD, DATA
//  2 .bss          0000132c  20001800  20001800  00001800  2**2

#define CODE_SIZE       0x10000
#define LOAD_ADDR		((void *)0x40000000)

#define MAX_APP       16
#define MAIN_APP_NAME "main"

typedef enum
{
  ALL = 0,
  NANOS,
  BLUE,
  NANOX
} hw_platform_t;

#define platform_string(platform)  (platform) == BLUE ? "Blue" : (platform) == NANOX ? "Nano X" : "Nano S"

struct elf_info_s {
  unsigned long load_offset;
  unsigned long load_size;
  unsigned long stack_addr;
  unsigned long stack_size;
};

struct app_s {
  char *name;
  int fd;
  struct elf_info_s elf;
};

struct memory_s {
  void *code;
  size_t code_size;
  void *data;
  size_t data_size;
};

static char *sdkmap[SDK_LAST] = {
    "1.5",
    "1.6",
    "blue-2.2.5",
};

static struct memory_s memory;
static struct app_s apps[MAX_APP];
static unsigned int napp;
static bool trace_syscalls;
static void* extra_rampage_addr;
static size_t extra_rampage_size;
static sdk_version_t sdk_version;

static struct app_s *current_app;
static ucontext_t *context;
static unsigned long *svc_addr;
static unsigned int n_svc_call;

static void* get_lower_page_aligned_addr(uintptr_t vaddr){
  return (void*)((uintptr_t)vaddr & ~((uintptr_t)getpagesize() - 1u));
}

static size_t get_upper_page_aligned_size(size_t vsize){
  size_t page_size = getpagesize();
  return ((vsize + page_size - 1u) & ~(page_size - 1u));
}

static void crash_handler(int sig_no)
{
  void *array[10];
  size_t size;

  size = backtrace(array, 10);

  fprintf(stderr, "[-] The app crashed with signal %d\n", sig_no);
  backtrace_symbols_fd(array, size, STDERR_FILENO);

  _exit(1);
}

static bool is_syscall_instruction(unsigned long addr)
{
  unsigned int i;

  for (i = 0; i < n_svc_call; i++) {
    if (svc_addr[i] == addr)
      return true;
  }

  return false;
}

static void sigill_handler(int sig_no, siginfo_t *UNUSED(info), void *vcontext)
{
  unsigned long pc, syscall, ret;
  unsigned long *parameters;
  int retid;

  context = (ucontext_t *)vcontext;
  syscall = context->uc_mcontext.arm_r0;
  parameters = (unsigned long *)context->uc_mcontext.arm_r1;
  pc = context->uc_mcontext.arm_pc;

  if (!is_syscall_instruction(context->uc_mcontext.arm_pc)) {
    fprintf(stderr, "[*] unhandled instruction at pc 0x%08lx\n", pc);
    fprintf(stderr, "    it would have triggered a crash on a real device\n");
    crash_handler(sig_no);
    _exit(1);
  }

  //fprintf(stderr, "[*] syscall: 0x%08lx (pc: 0x%08lx)\n", syscall, pc);

  ret = 0;
  retid = emulate(syscall, parameters, &ret, trace_syscalls, sdk_version);

  /* handle the os_lib_call syscall specially since it modifies the context
   * directly */
  if (sdk_version == SDK_1_5) {
    if (syscall == 0x6000650b) {        /* SYSCALL_os_lib_call_ID_IN */
      return;
    }
  }
  else if (sdk_version == SDK_1_6) {
    if (syscall == 0x6000670d) {        /* SYSCALL_os_lib_call_ID_IN */
      return;
    }
  }
  else if (sdk_version == SDK_BLUE_2_2_5) {
    if (syscall == 0x6000670d) {        /* SYSCALL_os_lib_call_ID_IN */
      return;
    }
  }

  /* In some versions of the SDK, a few syscalls don't use SVC_Call to issue
   * syscalls but call the svc instruction directly. I don't remember why it
   * fixes the issue however... */
  if (n_svc_call > 1) {
    parameters[0] = retid;
    parameters[1] = ret;
  } else {
    /* Default SVC_Call behavior */
    context->uc_mcontext.arm_r0 = retid;
    context->uc_mcontext.arm_r1 = ret;
  }

  /* skip undefined (originally svc) instruction */
  context->uc_mcontext.arm_pc += 2;
}

static int setup_alternate_stack(void)
{
  stack_t ss = {};
  /* HANDLER_STACK_SIZE + 2 guard pages before/after */
  char* mem = mmap (NULL,
                HANDLER_STACK_SIZE + 2 * getpagesize(),
                PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
                -1, 0);
  mprotect(mem, getpagesize(), PROT_NONE);
  mprotect(mem + getpagesize() + HANDLER_STACK_SIZE, getpagesize(), PROT_NONE);
  ss.ss_sp = mem + getpagesize();
  ss.ss_size = HANDLER_STACK_SIZE;
  ss.ss_flags = 0;

  if (sigaltstack(&ss, NULL) != 0) {
    return -1;
  }

  return 0;
}

static int setup_signals(void)
{
  struct sigaction sig_action;

  memset(&sig_action, 0, sizeof(sig_action));

  sig_action.sa_sigaction = sigill_handler;
  sig_action.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&sig_action.sa_mask);

  if (setup_alternate_stack() != 0) {
    warn("setup_alternate_stack()");
    return -1;
  }

  if (sigaction(SIGILL, &sig_action, 0) != 0) {
    warn("sigaction(SIGILL)");
    return -1;
  }

  if (signal(SIGSEGV, &crash_handler) == SIG_ERR) {
    warn("signal(SIGSEGV");
    return -1;
  }

  return 0;
}

/*
 * Replace the SVC instruction with an undefined instruction.
 *
 * It generates a SIGILL upon execution, which is catched to handle that
 * syscall.
 */
static int patch_svc(void *p, size_t size)
{
  unsigned char *addr, *end, *next;
  int ret;

  if (mprotect(p, size, PROT_READ | PROT_WRITE) != 0) {
    warn("mprotect(PROT_READ | PROT_WRITE)");
    return -1;
  }

  n_svc_call = 0;
  svc_addr = NULL;
  addr = p;
  end = addr + size;
  ret = 0;

  while (addr < end - 2) {
    next = memmem(addr, end - addr, "\x01\xdf", 2);
    if (next == NULL)
      break;

    /* instructions are aligned on 2 bytes */
    if ((unsigned long)next & 1) {
      addr = (unsigned char *)next + 1;
      continue;
    }

    svc_addr = realloc(svc_addr, (n_svc_call + 1) * sizeof(unsigned long));
    if (svc_addr == NULL)
      err(1, "realloc");
    svc_addr[n_svc_call] = (unsigned long)next;

    /* undefined instruction */
    memcpy(next, "\xff\xde", 2);

    fprintf(stderr, "[*] patching svc instruction at %p\n", next);

    addr = (unsigned char *)next + 2;
    n_svc_call++;
  }

  if (mprotect(p, size, PROT_READ | PROT_EXEC) != 0) {
    warn("mprotect(PROT_READ | PROT_EXEC)");
    return -1;
  }

  if (n_svc_call == 0) {
    warnx("failed to find SVC_call");
    return -1;
  }

  return ret;
}

static struct app_s *search_app_by_name(char *name)
{
  unsigned int i;

  for (i = 0; i < napp; i++) {
    if (strcmp(apps[i].name, name) == 0)
      return &apps[i];
  }

  return NULL;
}

/* open the app file */
static int open_app(char *name, char *filename, struct elf_info_s *elf)
{
  int fd;

  if (napp >= MAX_APP) {
    warnx("too many apps opened");
    return -1;
  }

  if (search_app_by_name(name) != NULL) {
    warnx("can't load 2 apps with the same name (\"%s\")", name);
    return -1;
  }

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    warn("open(\"%s\")", filename);
    return -1;
  }

  apps[napp].name = name;
  apps[napp].fd = fd;
  apps[napp].elf.load_offset = elf->load_offset;
  apps[napp].elf.load_size = elf->load_size;
  apps[napp].elf.stack_addr = elf->stack_addr;
  apps[napp].elf.stack_size = elf->stack_size;

  napp++;

  return 0;
}

static void reset_memory(bool unload_data)
{
  memory.code = MAP_FAILED;
  memory.code_size = 0;

  if (unload_data) {
    memory.data = MAP_FAILED;
    memory.data_size = 0;
  }
}

struct app_s *get_current_app(void)
{
  return current_app;
}

void save_current_context(struct sigcontext *sigcontext)
{
  memcpy(sigcontext, &context->uc_mcontext, sizeof(*sigcontext));
}

void replace_current_context(struct sigcontext *sigcontext)
{
  memcpy(&context->uc_mcontext, sigcontext, sizeof(*sigcontext));
}

int replace_current_code(struct app_s *app)
{
  int flags, prot;

  if (memory.code != MAP_FAILED) {
    if (munmap(memory.code, memory.code_size) != 0) {
      warn("munmap failed");
      return -1;
    }
  }

  flags = MAP_PRIVATE | MAP_FIXED;
  prot = PROT_READ | PROT_EXEC;
  memory.code = mmap(LOAD_ADDR, app->elf.load_size, prot, flags, app->fd, app->elf.load_offset);
  if (memory.code == MAP_FAILED) {
    warn("mmap code");
    return -1;
  }

  if (patch_svc(memory.code, app->elf.load_size) != 0) {
    /* this should never happen, because the svc were already patched without
     * error during the first load */
    _exit(1);
  }

  memory.code_size = app->elf.load_size;
  current_app = app;

  return 0;
}

void unload_running_app(bool unload_data)
{
  if (munmap(memory.code, memory.code_size) != 0)
    warn("munmap code");

  if (unload_data) {
    if (munmap(memory.data, memory.data_size) != 0)
      warn("munmap data");
  }

  reset_memory(unload_data);
}

/* map the app to memory */
static void *load_app(char *name, hw_platform_t plat)
{
  void *code, *data;
  struct app_s *app;
  struct stat st;
  size_t size;
  void* data_addr, *extra_addr = NULL;
  size_t data_size, extra_size = 0;

  code = MAP_FAILED;
  data = MAP_FAILED;

  app = search_app_by_name(name);
  if (app == NULL) {
    warnx("failed to find app \"%s\"", name);
    goto error;
  }

  if (fstat(app->fd, &st) != 0) {
    warn("fstat");
    goto error;
  }

  if (app->elf.load_offset > st.st_size) {
      warnx("app load offset is larger than file size (%ld > %lld)\n", app->elf.load_offset, st.st_size);
      goto error;
  }

  size = app->elf.load_size;
  if (size > st.st_size - app->elf.load_offset) {
      warnx("app load size is larger than file size (%ld > %lld)\n", app->elf.load_size, st.st_size);
      goto error;
  }

  data_addr = get_lower_page_aligned_addr(app->elf.stack_addr);
  data_size = get_upper_page_aligned_size(app->elf.stack_size + app->elf.stack_addr - (unsigned long)data_addr);

  /* load code */
  code = mmap(LOAD_ADDR, size, PROT_READ | PROT_EXEC, MAP_PRIVATE | MAP_FIXED, app->fd, app->elf.load_offset);
  if (code == MAP_FAILED) {
    warn("mmap code");
    goto error;
  }

  /* setup data */
  if (memory.data == MAP_FAILED) {
    if (mmap(data_addr, data_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
      warn("mmap data");
      goto error;
    }
  }

  /* setup extra page as additional RAM available to the app */
  if (extra_rampage_addr != NULL && extra_rampage_size != 0) {
    extra_addr = get_lower_page_aligned_addr((uintptr_t)extra_rampage_addr);
    extra_size = get_upper_page_aligned_size(extra_rampage_size);

    if (extra_addr != NULL && extra_size != 0) {
      if (mmap(extra_addr, extra_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
        warn("mmap extra RAM page");
        goto error;
      }
      fprintf(stderr, "[*] platform = %s, using extra RAM page [@=%p, size=%u bytes]", platform_string(plat), extra_addr, extra_size);
      if (extra_addr != extra_rampage_addr || extra_size != extra_rampage_size) {
        fprintf(stderr, ", realigned from [@=%p, size=%u bytes]\n", extra_rampage_addr, extra_rampage_size);
      }
      else {
        fprintf(stderr, "\n");
      }
    }
    else  {
      fprintf(stderr, "[*] platform = %s, not using extra RAM\n", platform_string(plat));
    }
  }

  if (patch_svc(code, size) != 0)
    goto error;

  memory.code = code;
  memory.code_size = size;

  if (memory.data == MAP_FAILED) {
    memory.data = data_addr;
    memory.data_size = data_size;
  }

  current_app = app;

  return code;

 error:
  if (code != MAP_FAILED && munmap(code, size) != 0)
    warn("munmap");

  if (data != MAP_FAILED && munmap(data_addr, data_size) != 0)
    warn("munmap");

  return NULL;
}

static int run_app(char *name, unsigned long *parameters, hw_platform_t plat)
{
  unsigned long stack_end, stack_start;
  void (*f)(unsigned long *);
  struct app_s *app;
  void *p;

  p = load_app(name, plat);
  if (p == NULL)
    return -1;

  app = get_current_app();

  /* thumb mode */
  f = (void *)((unsigned long)p | 1);
  stack_end = app->elf.stack_addr;
  stack_start = app->elf.stack_addr + app->elf.stack_size;

  asm volatile (
    "mov r0, %2\n"
    "mov r9, %1\n"
    "mov sp, %0\n"
    "bx  %3\n"
    "bkpt\n" /* the call should neved return */
    :
    : "r"(stack_start), "r"(stack_end), "r"(parameters), "r"(f)
    : "r0", "r9", "sp");

  /* never reached */
  errx(1, "the app returned, exiting...");

  return 0;
}

/* This is a bit different than run_app since this function is called within the
 * SIGILL handler. Just setup a fake context to allow the signal handler to
 * return to the lib code. */
int run_lib(char *name, unsigned long *parameters)
{
  unsigned long f;
  void *p;

  p = load_app(name, ALL);
  if (p == NULL)
    return -1;

  /* no thumb mode set when returning from signal handler */
  f = (unsigned long)p & 0xfffffffe;

  context->uc_mcontext.arm_r0 = (unsigned long)parameters;
  context->uc_mcontext.arm_pc = f;

  return 0;
}

/*
 * Libraries are given with the following format:
 * name:path:load_offset:load_size. eg. Bitcoin:apps/btc.elf:0x1000:0x9fc0:0x20001800:0x1800
 */
static char *parse_app_infos(char *arg, char **filename, struct elf_info_s *elf)
{
  char *libname;
  int ret;

  libname = strdup(arg);
  *filename = strdup(arg);
  if (libname == NULL || *filename == NULL) {
    err(1, "strdup");
  }

  ret = sscanf(arg, "%[^:]:%[^:]:0x%lx:0x%lx:0x%lx:0x%lx",
               libname, *filename, &elf->load_offset, &elf->load_size, &elf->stack_addr, &elf->stack_size);
  if (ret != 6) {
    warnx("failed to parse app infos (\"%s\", %d)", arg, ret);
    free(libname);
    free(*filename);
    return NULL;
  }

  return libname;
}

static int load_apps(int argc, char *argv[])
{
  char *filename, *libname;
  struct elf_info_s elf;
  int i;

  for (i = 0; i < argc; i++) {
    libname = parse_app_infos(argv[i], &filename, &elf);
    if (libname == NULL)
      return -1;

    if (open_app(libname, filename, &elf) != 0)
      return -1;
  }

  return 0;
}

static sdk_version_t str2sdkver(char *arg)
{
  sdk_version_t version;

  for (version = SDK_1_5; version < SDK_LAST; version++) {
    if (strcmp(sdkmap[version], arg) == 0) {
      fprintf(stderr, "[*] using SDK version %s\n", sdkmap[version]);
      break;
    }
  }

  return version;
}

static void usage(char *argv0)
{
  fprintf(stderr, "Usage: %s [-t] [-r <rampage_address> -s <rampage_size> -k <sdk_version>] <app.elf> [libname:lib.elf:0x1000:0x9fc0:0x20001800:0x1800 ...]\n", argv0);
  fprintf(stderr, "\n\
  -r <rampage_address>: Hex address (prefixed with '0x') of extra ram page to map\n\
                        app.elf memory. Requires '-s' parameter.\n\
  -s <rampage_size>:    Size in bytes of the extra RAM page provided by '-r'.\n\
                        Value must be provided in hex, prefixed with '0x'.\n\
  -m <model>:           Optional string representing the device model being emula-\n\
                        ted. Currently supports \"nanos\" and \"blue\".\n\
  -k <sdk_version>:     A string representing the SDK version to be used, like \"1.6\".\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  int opt;
  hw_platform_t plat = ALL;

  trace_syscalls = false;
  sdk_version = SDK_1_5;

  extra_rampage_addr = NULL;
  extra_rampage_size = 0;

  while((opt = getopt(argc, argv, "tr:s:m:k:")) != -1) {
    switch(opt) {
    case 'k':
      sdk_version = str2sdkver(optarg);
      break;
    case 't':
      trace_syscalls = true;
      break;
    case 'r':
      sscanf(optarg, "%p", &extra_rampage_addr);
      break;
    case 's':
      sscanf(optarg, "%x", &extra_rampage_size);
      break;
    case 'm':
      if (!strcmp(optarg, "nanos")) {
        plat = NANOS;
      } else if (!strcmp(optarg, "blue")) {
        plat = BLUE;
      } else if (!strcmp(optarg, "nanox")) {
        plat= NANOX;
      }
      break;
    default:
      usage(argv[0]);
      break;
    }
  }

  if (argc - optind <= 0) {
    usage(argv[0]);
  }

  if (sdk_version == SDK_LAST) {
    errx(1, "invalid SDK version");
  }

  reset_memory(true);

  if (load_apps(argc - optind, &argv[optind]) != 0)
    return 1;

  if (setup_signals() != 0)
    return 1;

  run_app(MAIN_APP_NAME, NULL, plat);

  return 0;
}
