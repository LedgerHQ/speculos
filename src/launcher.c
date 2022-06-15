/* fix for the following error on Windows (WSL):
 * fstat: Value too large for defined data type */
#define _FILE_OFFSET_BITS 64
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "emulate.h"
#include "svc.h"

#define LOAD_ADDR     ((void *)0x40000000)
#define MAX_APP       16
#define MAIN_APP_NAME "main"

#ifndef GIT_REVISION
#warning GIT_REVISION is not defined
#define GIT_REVISION "00000000"
#endif

#define CX_ADDR_NANOS  ((void *)0x00120000)
#define CX_ADDR_NANOX  ((void *)0x00210000)
#define CX_ADDR_NANOSP ((void *)0x00808000)

#define CX_SIZE          0x8000
#define CX_OFFSET        0x10000
#define CX_OFFSET_NANOSP 0x8000 // offset of text section within cx.elf

#define CXRAM_ADDR ((void *)0x00603000)
#define CXRAM_SIZE 0x800

#define CXRAM_NANOSP_ADDR ((void *)0x20003000)
#define CXRAM_NANOSP_SIZE 0x800

typedef enum {
  MODEL_NANO_S,
  MODEL_NANO_SP,
  MODEL_NANO_X,
  MODEL_BLUE,
  MODEL_COUNT
} hw_model_t;

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

typedef struct model_sdk_s {
  hw_model_t model;
  char *sdk;
} MODEL_SDK;

static MODEL_SDK sdkmap[SDK_COUNT] = {
  { MODEL_NANO_X, "1.2" },      { MODEL_NANO_X, "2.0" },
  { MODEL_NANO_X, "2.0.2" },    { MODEL_NANO_S, "1.5" },
  { MODEL_NANO_S, "1.6" },      { MODEL_NANO_S, "2.0" },
  { MODEL_NANO_S, "2.1" },      { MODEL_BLUE, "1.5" },
  { MODEL_BLUE, "blue-2.2.5" }, { MODEL_NANO_SP, "1.0" },
  { MODEL_NANO_SP, "1.0.3" }
};

static char *model_name[MODEL_COUNT] = { "nanos", "nanosp", "nanox", "blue" };

static struct memory_s memory;
static struct app_s apps[MAX_APP];
static unsigned int napp;
static void *extra_rampage_addr;
static size_t extra_rampage_size;
sdk_version_t sdk_version;

static struct app_s *current_app;

static void *get_lower_page_aligned_addr(uintptr_t vaddr)
{
  return (void *)((uintptr_t)vaddr & ~((uintptr_t)sysconf(_SC_PAGESIZE) - 1u));
}

static size_t get_upper_page_aligned_size(size_t vsize)
{
  size_t page_size = sysconf(_SC_PAGESIZE);
  return ((vsize + page_size - 1u) & ~(page_size - 1u));
}

static struct app_s *search_app_by_name(char *name)
{
  unsigned int i;

  for (i = 0; i < napp; i++) {
    if (strcmp(apps[i].name, name) == 0) {
      return &apps[i];
    }
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

int replace_current_code(struct app_s *app)
{
  int flags, prot;
  size_t page_size = sysconf(_SC_PAGESIZE);

  if (memory.code != MAP_FAILED) {
    if (munmap(memory.code, memory.code_size) != 0) {
      warn("munmap failed");
      return -1;
    }
  }

  flags = MAP_PRIVATE | MAP_FIXED;
  prot = PROT_READ | PROT_EXEC;
  /* map an extra page in case the _install_params are mapped in the beginning
   * of a new page so that they can still be accessed */
  memory.code = mmap(LOAD_ADDR, app->elf.load_size + page_size, prot, flags,
                     app->fd, app->elf.load_offset);

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
  if (munmap(memory.code, memory.code_size) != 0) {
    warn("munmap code");
  }

  if (unload_data) {
    if (munmap(memory.data, memory.data_size) != 0) {
      warn("munmap data");
    }
  }

  reset_memory(unload_data);
}

/* map the app to memory */
static void *load_app(char *name)
{
  void *code, *data;
  struct app_s *app;
  struct stat st;
  size_t size;
  void *data_addr, *extra_addr = NULL;
  size_t data_size, extra_size = 0;
  size_t page_size = sysconf(_SC_PAGESIZE);

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
    warnx("app load offset is larger than file size (%lu > %lld)\n",
          app->elf.load_offset, st.st_size);
    goto error;
  }

  size = app->elf.load_size;
  if (size > st.st_size - app->elf.load_offset) {
    warnx("app load size is larger than file size (%lu > %lld)\n",
          app->elf.load_size, st.st_size);
    goto error;
  }

  data_addr = get_lower_page_aligned_addr(app->elf.stack_addr);
  data_size = get_upper_page_aligned_size(
      app->elf.stack_size + app->elf.stack_addr - (unsigned long)data_addr);

  /* load code
   * map an extra page in case the _install_params are mapped in the beginning
   * of a new page so that they can still be accessed */
  code = mmap(LOAD_ADDR, size + page_size, PROT_READ | PROT_EXEC,
              MAP_PRIVATE | MAP_FIXED, app->fd, app->elf.load_offset);
  if (code == MAP_FAILED) {
    warn("mmap code");
    goto error;
  }

  /* setup data */
  if (memory.data == MAP_FAILED) {
    if (mmap(data_addr, data_size, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
      warn("mmap data");
      goto error;
    }
    /* initialize .bss (and the stack) to 0xa5 to mimic BOLOS behavior in older
     * FW versions, even if it violates section 3.5.7 of the C89 standard */
    switch (sdk_version) {
    case SDK_BLUE_1_5:
    case SDK_BLUE_2_2_5:
    case SDK_NANO_S_1_5:
    case SDK_NANO_S_1_6:
    case SDK_NANO_S_2_0:
    case SDK_NANO_X_1_2:
      memset(data_addr, 0xa5, data_size);
      break;
    /* In newer FW versions the app RAM (.bss and stack) is zeroized.
     * This is done by mmap thanks to the MAP_ANONYMOUS flag. */
    default:
      break;
    }
  }

  /* setup extra page as additional RAM available to the app */
  if (extra_rampage_addr != NULL && extra_rampage_size != 0) {
    extra_addr = get_lower_page_aligned_addr((uintptr_t)extra_rampage_addr);
    extra_size = get_upper_page_aligned_size(extra_rampage_size);

    if (extra_addr != NULL && extra_size != 0) {
      if (mmap(extra_addr, extra_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
        warn("mmap extra RAM page");
        goto error;
      }
      fprintf(stderr, "[*] using extra RAM page [@=%p, size=%u bytes]",
              extra_addr, extra_size);
      if (extra_addr != extra_rampage_addr ||
          extra_size != extra_rampage_size) {
        fprintf(stderr, ", realigned from [@=%p, size=%u bytes]\n",
                extra_rampage_addr, extra_rampage_size);
      } else {
        fprintf(stderr, "\n");
      }
    }
  }

  if (patch_svc(code, size) != 0) {
    goto error;
  }

  memory.code = code;
  memory.code_size = size;

  if (memory.data == MAP_FAILED) {
    memory.data = data_addr;
    memory.data_size = data_size;
  }

  current_app = app;

  return code;

error:
  if (code != MAP_FAILED && munmap(code, size) != 0) {
    warn("munmap");
  }

  if (data != MAP_FAILED && munmap(data_addr, data_size) != 0) {
    warn("munmap");
  }

  return NULL;
}

static int load_cxlib(hw_model_t model, char *cxlib_path)
{
  // First, try to open the cx.elf file specified (could be the one by default)
  int fd = open(cxlib_path, O_RDONLY);
  if (fd == -1) {
    // Try to use environment variable CXLIB_PATH
    char *path = getenv("CXLIB_PATH");
    if (path == NULL) {
      warnx("failed to open \"%s\" and no CXLIB_PATH environment found!",
            cxlib_path);
      return -1;
    }
    fprintf(stderr, "[*] failed to open \"%s\", trying CXLIB_PATH...\n",
            cxlib_path);
    fd = open(path, O_RDONLY);
    if (fd == -1) {
      warn("failed to open \"%s\"!", path);
      return -1;
    }
    cxlib_path = path;
  }
  fprintf(stderr, "[*] loading CXLIB from \"%s\"\n", cxlib_path);

  int flags = MAP_PRIVATE | MAP_FIXED;
  int prot = PROT_READ | PROT_EXEC;

  void *cx_addr = CX_ADDR_NANOS;
  int offset = CX_OFFSET;

  switch (model) {
  case MODEL_NANO_X:
    cx_addr = CX_ADDR_NANOX;
    if (mmap(CXRAM_ADDR, CXRAM_SIZE, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
      warn("mmap cxram");
      return -1;
    }
    break;
  case MODEL_NANO_SP:
    cx_addr = CX_ADDR_NANOSP;
    offset = CX_OFFSET_NANOSP;
    if (mmap(CXRAM_NANOSP_ADDR, CXRAM_NANOSP_SIZE, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
      warn("mmap cxram");
      return -1;
    }
    break;
  default:
    break;
  }

  void *p = mmap(cx_addr, CX_SIZE, prot, flags, fd, offset);
  if (p == MAP_FAILED) {
    warn("mmap cxlib");
    close(fd);
    return -1;
  }

  if (patch_svc(cx_addr, CX_SIZE) != 0) {
    if (munmap(p, CX_SIZE) != 0) {
      warn("munmap");
    }
    close(fd);
    return -1;
  }

  return 0;
}

static int run_app(char *name, unsigned long *parameters)
{
  unsigned long stack_end, stack_start;
  void (*f)(unsigned long *);
  struct app_s *app;
  void *p;

  p = load_app(name);
  if (p == NULL) {
    return -1;
  }

  app = get_current_app();

  /* thumb mode */
  f = (void *)((unsigned long)p | 1);
  stack_end = app->elf.stack_addr;
  stack_start = app->elf.stack_addr + app->elf.stack_size;

  asm volatile("mov r0, %2\n"
               "mov r9, %1\n"
               "mov sp, %0\n"
               "bx  %3\n"
               "bkpt\n" /* the call should neved return */
               :
               : "r"(stack_start), "r"(stack_end), "r"(parameters), "r"(f)
               : "r0", "r9");

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

  p = load_app(name);
  if (p == NULL) {
    return -1;
  }

  /* no thumb mode set when returning from signal handler */
  f = (unsigned long)p & 0xfffffffe;
  setup_context((unsigned long)parameters, f);

  return 0;
}

/*
 * Libraries are given with the following format:
 * name:path:load_offset:load_size. eg.
 * Bitcoin:apps/btc.elf:0x1000:0x9fc0:0x20001800:0x1800
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

  ret = sscanf(arg, "%[^:]:%[^:]:0x%lx:0x%lx:0x%lx:0x%lx", libname, *filename,
               &elf->load_offset, &elf->load_size, &elf->stack_addr,
               &elf->stack_size);
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
    if (libname == NULL) {
      return -1;
    }

    if (open_app(libname, filename, &elf) != 0) {
      return -1;
    }
  }

  return 0;
}

static sdk_version_t str2sdkver(hw_model_t model, char *arg)
{
  sdk_version_t version;

  for (version = 0; version < SDK_COUNT; version++) {
    if (sdkmap[version].model == model &&
        strcmp(sdkmap[version].sdk, arg) == 0) {
      fprintf(stderr, "[*] using SDK version %s on %s\n", sdkmap[version].sdk,
              model_name[model]);
      break;
    }
  }

  return version;
}

static void usage(char *argv0)
{
  fprintf(stderr,
          "Usage: %s [-t] [-r <rampage:ramsize> -k <sdk_version>] <app.elf> "
          "[libname:lib.elf:0x1000:0x9fc0:0x20001800:0x1800 ...]\n",
          argv0);
  fprintf(stderr, "\n\
  -r <rampage:ramsize>: Address and size of extra ram (both in hex) to map app.elf memory.\n\
  -m <model>:           Optional string representing the device model being emula-\n\
                        ted. Currently supports \"nanos\", \"nanosp\", \"nanox\" and \"blue\".\n\
  -k <sdk_version>:     A string representing the SDK version to be used, like \"1.6\".\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  char *cxlib_path = NULL;
  hw_model_t model;
  int opt;

  trace_syscalls = false;
  // Nano S with SDK 2.0 by default:
  sdk_version = SDK_NANO_S_2_0;
  model = MODEL_NANO_S;
  char *sdk = sdkmap[SDK_NANO_S_2_0].sdk;

  extra_rampage_addr = NULL;
  extra_rampage_size = 0;

  fprintf(stderr, "[*] speculos launcher revision: " GIT_REVISION "\n");

  while ((opt = getopt(argc, argv, "c:tr:s:m:k:")) != -1) {
    switch (opt) {
    case 'c':
      cxlib_path = optarg;
      break;
    case 'k':
      sdk = optarg;
      break;
    case 't':
      trace_syscalls = true;
      break;
    case 'r':
      if (sscanf(optarg, "%p:%x", &extra_rampage_addr, &extra_rampage_size) !=
          2) {
        errx(1, "invalid extram ram page/size\n");
      }
      break;
    case 'm':
      if (strcmp(optarg, "nanos") == 0) {
        model = MODEL_NANO_S;
      } else if (strcmp(optarg, "nanox") == 0) {
        model = MODEL_NANO_X;
      } else if (strcmp(optarg, "blue") == 0) {
        model = MODEL_BLUE;
      } else if (strcmp(optarg, "nanosp") == 0) {
        model = MODEL_NANO_SP;
      } else {
        errx(1, "invalid model \"%s\"", optarg);
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

  sdk_version = str2sdkver(model, sdk);

  if (sdk_version == SDK_COUNT) {
    errx(1, "invalid SDK version");
  }

  switch (model) {
  case MODEL_NANO_S:
    if (sdk_version != SDK_NANO_S_1_5 && sdk_version != SDK_NANO_S_1_6 &&
        sdk_version != SDK_NANO_S_2_0 && sdk_version != SDK_NANO_S_2_1) {
      errx(1, "invalid SDK version for the Ledger Nano S");
    }
    break;
  case MODEL_NANO_X:
    if (sdk_version != SDK_NANO_X_1_2 && sdk_version != SDK_NANO_X_2_0 &&
        sdk_version != SDK_NANO_X_2_0_2) {
      errx(1, "invalid SDK version for the Ledger Nano X");
    }
    break;
  case MODEL_BLUE:
    if (sdk_version != SDK_BLUE_1_5 && sdk_version != SDK_BLUE_2_2_5) {
      errx(1, "invalid SDK version for the Ledger Blue");
    }
    break;
  case MODEL_NANO_SP:
    if (sdk_version != SDK_NANO_SP_1_0 && sdk_version != SDK_NANO_SP_1_0_3) {
      errx(1, "invalid SDK version for the Ledger NanoSP");
    }
    break;
  default:
    usage(argv[0]);
    break;
  }

  make_openssl_random_deterministic();
  reset_memory(true);

  if (load_apps(argc - optind, &argv[optind]) != 0) {
    return 1;
  }

  if (sdk_version == SDK_NANO_S_2_0 || sdk_version == SDK_NANO_S_2_1 ||
      sdk_version == SDK_NANO_X_2_0 || sdk_version == SDK_NANO_X_2_0_2 ||
      sdk_version == SDK_NANO_SP_1_0 || sdk_version == SDK_NANO_SP_1_0_3) {
    if (load_cxlib(model, cxlib_path) != 0) {
      return 1;
    }
  }

  if (setup_signals() != 0) {
    return 1;
  }

  run_app(MAIN_APP_NAME, NULL);

  return 0;
}
