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

struct elf_info_s {
  unsigned long load_offset;
  unsigned long load_size;
  unsigned long stack_addr;
  unsigned long stack_size;
  unsigned long svc_call_addr;
  unsigned long svc_cx_call_addr;
  unsigned long text_load_addr;
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

static struct memory_s memory;
static struct app_s apps[MAX_APP];
static unsigned int napp;
static void *extra_rampage_addr;
static size_t extra_rampage_size;

sdk_version_t sdk_version = SDK_COUNT;
hw_model_t hw_model = MODEL_COUNT;

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
  apps[napp].elf.svc_call_addr = elf->svc_call_addr;
  apps[napp].elf.svc_cx_call_addr = elf->svc_cx_call_addr;
  apps[napp].elf.text_load_addr = elf->text_load_addr;

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

// On old versions of the SDK, functions SVC_Call and SVC_cx_call were inlined
// In this case we can't look for the symbols in the elf to only patch the SVC_1
// inside of these functions
static bool is_svc_inlined(void)
{
  switch (sdk_version) {
  case SDK_BLUE_1_5:
  case SDK_BLUE_2_2_5:
  case SDK_NANO_S_1_5:
  case SDK_NANO_S_1_6:
  case SDK_NANO_X_1_2:
    return true;
    break;
  default:
    return false;
    break;
  }
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

  if (mprotect(memory.code, app->elf.load_size, PROT_READ | PROT_WRITE) != 0) {
    warn("could not update mprotect in rw mode for app");
    _exit(1);
  }

  // If the syscall functions are not inlined and their symbols have been found
  // in the elf file, patch the elf at this address to remove the SVC 1 call
  if (!is_svc_inlined() &&
      (app->elf.svc_call_addr != 0 || app->elf.svc_cx_call_addr != 0)) {
    if (app->elf.svc_call_addr != 0) {
      uint32_t start = app->elf.svc_call_addr - app->elf.text_load_addr;

      if (patch_svc(memory.code + start, 2) != 0) {
        /* this should never happen, because the svc were already patched
         * without error during the first load */
        _exit(1);
      }
    }

    if (app->elf.svc_cx_call_addr != 0) {
      uint32_t start = app->elf.svc_cx_call_addr - app->elf.text_load_addr;

      if (patch_svc(memory.code + start, 2) != 0) {
        /* this should never happen, because the svc were already patched
         * without error during the first load */
        _exit(1);
      }
    }
  } else {
    if (patch_svc(memory.code, app->elf.load_size) != 0) {
      /* this should never happen, because the svc were already patched
       * without error during the first load */
      _exit(1);
    }
  }

  if (mprotect(memory.code, app->elf.load_size, PROT_READ | PROT_EXEC) != 0) {
    warn("could not update mprotect in rx mode for app");
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

  if (mprotect(code, size, PROT_READ | PROT_WRITE) != 0) {
    warn("could not update mprotect in rw mode for app");
    goto error;
  }

  // If the syscall functions are not inlined and their symbols have been found
  // in the elf file, patch the elf at this address to remove the SVC 1 call
  if (!is_svc_inlined() &&
      (app->elf.svc_call_addr != 0 || app->elf.svc_cx_call_addr != 0)) {
    if (app->elf.svc_call_addr != 0) {
      uint32_t start = app->elf.svc_call_addr - app->elf.text_load_addr;

      if (patch_svc(code + start, 2) != 0) {
        goto error;
      }
    }

    if (app->elf.svc_cx_call_addr != 0) {
      uint32_t start = app->elf.svc_cx_call_addr - app->elf.text_load_addr;

      if (patch_svc(code + start, 2) != 0) {
        goto error;
      }
    }
  } else {
    if (patch_svc(code, app->elf.load_size) != 0) {
      goto error;
    }
  }

  if (mprotect(code, size, PROT_READ | PROT_EXEC) != 0) {
    warn("could not update mprotect in rx mode for app");
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

static int load_fonts(char *fonts_path)
{
  int fd = open(fonts_path, O_RDONLY);
  if (fd == -1) {
    warnx("failed to open \"%s\"", fonts_path);
    return -1;
  }
  fprintf(stderr, "[*] loading fonts from \"%s\"\n", fonts_path);

  int flags = MAP_PRIVATE | MAP_FIXED;
  int prot = PROT_READ;
  int load_addr;
  int load_size;

  if (sdk_version == SDK_API_LEVEL_1 || sdk_version == SDK_API_LEVEL_3 ||
      sdk_version == SDK_API_LEVEL_5) {
    load_addr = 0x00805000;
    load_size = 20480;
  } else if (sdk_version == SDK_API_LEVEL_7) {
    load_addr = 0x00805000;
    load_size = 45056;
  } else if ((sdk_version == SDK_API_LEVEL_8 ||
              sdk_version == SDK_API_LEVEL_9 ||
              sdk_version == SDK_API_LEVEL_10 ||
              sdk_version == SDK_API_LEVEL_11 ||
              sdk_version == SDK_API_LEVEL_12)) {
    load_addr = 0x00805000;
    load_size = 40960;
  } else {
    warn("Invalid sdk version for fonts");
    close(fd);
    return -1;
  }
  void *p = mmap((void *)load_addr, load_size, prot, flags, fd, 0);
  fprintf(stderr, "[*] loaded fonts at %p\n", p);

  if (p == MAP_FAILED) {
    warn("mmap fonts");
    close(fd);
    return -1;
  }
  return 0;
}

static int load_cxlib(char *cxlib_args)
{
  char *cxlib_path = strdup(cxlib_args);
  uint32_t sh_offset, sh_size, sh_load, cx_ram_size, cx_ram_load;

  int ret = sscanf(cxlib_args, "%[^:]:0x%x:0x%x:0x%x:0x%x:0x%x", cxlib_path,
                   &sh_offset, &sh_size, &sh_load, &cx_ram_size, &cx_ram_load);

  if (ret != 6) {
    fprintf(stderr, "sscanf failed: %d", ret);
  }
  // First, try to open the cx.elf file specified (could be the one by default)
  int fd = open(cxlib_path, O_RDONLY);
  if (fd == -1) {
    warnx("failed to open \"%s\"", cxlib_path);
    return -1;
  }
  fprintf(stderr, "[*] loading CXLIB from \"%s\"\n", cxlib_path);

  int flags = MAP_PRIVATE | MAP_FIXED;
  int prot = PROT_READ | PROT_EXEC;
  void *p = mmap((void *)sh_load, sh_size, prot, flags, fd, sh_offset);
  if (p == MAP_FAILED) {
    warn("mmap cxlib");
    close(fd);
    return -1;
  }

  // Map CXRAM on non NanoS devices
  if (hw_model != MODEL_NANO_S) {
    // Make sure cr_ram is aligned
    if (cx_ram_load % sysconf(_SC_PAGESIZE)) {
      cx_ram_size += cx_ram_load % sysconf(_SC_PAGESIZE);
      cx_ram_load -= (cx_ram_load % sysconf(_SC_PAGESIZE));
    }

    if (mmap((void *)cx_ram_load, cx_ram_size, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_FIXED | MAP_ANONYMOUS, -1, 0) == MAP_FAILED) {
      warn("mmap cxram %x, %x", cx_ram_load, cx_ram_size);
      return -1;
    }
  }

  if (mprotect(p, sh_size, PROT_READ | PROT_WRITE) != 0) {
    warn("could not update mprotect in rw mode for cxlib");
    return -1;
  }

  if (patch_svc(p, sh_size) != 0) {
    if (munmap(p, sh_size) != 0) {
      warn("munmap");
    }
    close(fd);
    return -1;
  }

  if (mprotect(p, sh_size, PROT_READ | PROT_EXEC) != 0) {
    warn("could not update mprotect in rx mode for cxlib");
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

  ret = sscanf(arg, "%[^:]:%[^:]:0x%lx:0x%lx:0x%lx:0x%lx:0x%lx:0x%lx:0x%lx",
               libname, *filename, &elf->load_offset, &elf->load_size,
               &elf->stack_addr, &elf->stack_size, &elf->svc_call_addr,
               &elf->svc_cx_call_addr, &elf->text_load_addr);
  if (ret != 9) {
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

static sdk_version_t apilevelstr2sdkver(const char *api_level_arg)
{
  if (strcmp("1", api_level_arg) == 0) {
    return SDK_API_LEVEL_1;
  } else if (strcmp("3", api_level_arg) == 0) {
    return SDK_API_LEVEL_3;
  } else if (strcmp("5", api_level_arg) == 0) {
    return SDK_API_LEVEL_5;
  } else if (strcmp("7", api_level_arg) == 0) {
    return SDK_API_LEVEL_7;
  } else if (strcmp("8", api_level_arg) == 0) {
    return SDK_API_LEVEL_8;
  } else if (strcmp("9", api_level_arg) == 0) {
    return SDK_API_LEVEL_9;
  } else if (strcmp("10", api_level_arg) == 0) {
    return SDK_API_LEVEL_10;
  } else if (strcmp("11", api_level_arg) == 0) {
    return SDK_API_LEVEL_11;
  } else if (strcmp("12", api_level_arg) == 0) {
    return SDK_API_LEVEL_12;
  } else {
    return SDK_COUNT;
  }
}

static sdk_version_t sdkstr2sdkver(const char *sdk_arg)
{
  sdk_version_t version;

  for (version = 0; version < SDK_COUNT; version++) {
    if (sdkmap[version].model == hw_model &&
        strcmp(sdkmap[version].sdk, sdk_arg) == 0) {
      break;
    }
  }

  return version;
}

static void usage(char *argv0)
{
  fprintf(stderr,
          "Usage: %s [-t] [-r <rampage:ramsize> -k <sdk_version> -a "
          "<api_level>] <app.elf> "
          "[libname:lib.elf:0x1000:0x9fc0:0x20001800:0x1800 ...]\n",
          argv0);
  fprintf(stderr, "\n\
  -r <rampage:ramsize>: Address and size of extra ram (both in hex) to map app.elf memory.\n\
  -m <model>:           Optional string representing the device model being emula-\n\
                        ted. Currently supports \"nanos\", \"nanosp\", \"nanox\", \"stax\" and \"blue\".\n\
  -k <sdk_version>:     A string representing the SDK version to be used, like \"1.6\".\n\
  -a <api_level>:       A string representing the SDK api level to be used, like \"1\".\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  char *cxlib_path = NULL;
  char *fonts_path = NULL;

  int opt;

  trace_syscalls = false;
  char *model_str = NULL;
  char *sdk = NULL;
  char *api_level = NULL;

  extra_rampage_addr = NULL;
  extra_rampage_size = 0;

  fprintf(stderr, "[*] speculos launcher revision: " GIT_REVISION "\n");

  while ((opt = getopt(argc, argv, "c:tr:s:m:k:a:f:")) != -1) {
    switch (opt) {
    case 'f':
      fonts_path = optarg;
      break;
    case 'c':
      cxlib_path = optarg;
      break;
    case 'k':
      sdk = optarg;
      break;
    case 'a':
      api_level = optarg;
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
      model_str = optarg;
      if (strcmp(optarg, "nanos") == 0) {
        hw_model = MODEL_NANO_S;
      } else if (strcmp(optarg, "nanox") == 0) {
        hw_model = MODEL_NANO_X;
      } else if (strcmp(optarg, "blue") == 0) {
        hw_model = MODEL_BLUE;
      } else if (strcmp(optarg, "nanosp") == 0) {
        hw_model = MODEL_NANO_SP;
      } else if (strcmp(optarg, "stax") == 0) {
        hw_model = MODEL_STAX;
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

  if (hw_model == MODEL_COUNT) {
    errx(1, "invalid model");
  }

  if (api_level != NULL) {
    sdk_version = apilevelstr2sdkver(api_level);
    if (sdk_version == SDK_COUNT) {
      errx(1, "invalid SDK api_level: %s", api_level);
    }
  } else {
    sdk_version = sdkstr2sdkver(sdk);
    if (sdk_version == SDK_COUNT) {
      errx(1, "invalid SDK version: %s", sdk);
    }
  }

  fprintf(stderr, "[*] using SDK version %u on %s\n", sdk_version, model_str);

  switch (hw_model) {
  case MODEL_NANO_S:
    if (sdk_version != SDK_NANO_S_1_5 && sdk_version != SDK_NANO_S_1_6 &&
        sdk_version != SDK_NANO_S_2_0 && sdk_version != SDK_NANO_S_2_1) {
      errx(1, "invalid SDK version for the Ledger Nano S");
    }
    break;
  case MODEL_NANO_X:
    if (sdk_version != SDK_NANO_X_1_2 && sdk_version != SDK_NANO_X_2_0 &&
        sdk_version != SDK_NANO_X_2_0_2 && sdk_version != SDK_API_LEVEL_1 &&
        sdk_version != SDK_API_LEVEL_5 && sdk_version != SDK_API_LEVEL_12) {
      errx(1, "invalid SDK version for the Ledger Nano X");
    }
    break;
  case MODEL_BLUE:
    if (sdk_version != SDK_BLUE_1_5 && sdk_version != SDK_BLUE_2_2_5) {
      errx(1, "invalid SDK version for the Ledger Blue");
    }
    break;
  case MODEL_NANO_SP:
    if (sdk_version != SDK_NANO_SP_1_0 && sdk_version != SDK_NANO_SP_1_0_3 &&
        sdk_version != SDK_API_LEVEL_1 && sdk_version != SDK_API_LEVEL_5 &&
        sdk_version != SDK_API_LEVEL_12) {
      errx(1, "invalid SDK version for the Ledger NanoSP");
    }
    break;
  case MODEL_STAX:
    if (sdk_version != SDK_API_LEVEL_1 && sdk_version != SDK_API_LEVEL_3 &&
        sdk_version != SDK_API_LEVEL_5 && sdk_version != SDK_API_LEVEL_7 &&
        sdk_version != SDK_API_LEVEL_8 && sdk_version != SDK_API_LEVEL_9 &&
        sdk_version != SDK_API_LEVEL_10 && sdk_version != SDK_API_LEVEL_11 &&
        sdk_version != SDK_API_LEVEL_12) {
      errx(1, "invalid SDK version for the Ledger Stax");
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
      sdk_version == SDK_NANO_SP_1_0 || sdk_version == SDK_NANO_SP_1_0_3 ||
      sdk_version == SDK_API_LEVEL_1 || sdk_version == SDK_API_LEVEL_3 ||
      sdk_version == SDK_API_LEVEL_5 || sdk_version == SDK_API_LEVEL_7 ||
      sdk_version == SDK_API_LEVEL_8 || sdk_version == SDK_API_LEVEL_9 ||
      sdk_version == SDK_API_LEVEL_10 || sdk_version == SDK_API_LEVEL_11 ||
      sdk_version == SDK_API_LEVEL_12) {
    if (load_cxlib(cxlib_path) != 0) {
      return 1;
    }
  }

  if (hw_model == MODEL_STAX && fonts_path) {
    if (load_fonts(fonts_path) != 0) {
      return 1;
    }
  }

  if (setup_signals() != 0) {
    return 1;
  }

  run_app(MAIN_APP_NAME, NULL);

  return 0;
}
