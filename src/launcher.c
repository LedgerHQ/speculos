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
#include "environment.h"
#include "fonts.h"
#include "launcher.h"
#include "svc.h"

#define LOAD_ADDR     ((void *)0x40000000)
#define LINK_RAM_ADDR (0xda7a0000)
#define LOAD_RAM_ADDR (0x50000000)
#define MAX_APP       64
#define MAIN_APP_NAME "main"

#ifndef GIT_REVISION
#warning GIT_REVISION is not defined
#define GIT_REVISION "00000000"
#endif

#define STORAGE_APP_NAME_LEN 30
const char STORAGE_BACKUP[] = ".backup";

#define DERIVATION_PATH_MAX_LEN 256

struct elf_info_s {
  unsigned long load_offset;
  unsigned long load_size;
  unsigned long stack_addr;
  unsigned long stack_size;
  unsigned long svc_call_addr;
  unsigned long svc_cx_call_addr;
  unsigned long text_load_addr;
  unsigned long fonts_addr;
  unsigned long fonts_size;
  unsigned long app_nvram_addr;
  unsigned long app_nvram_size;
  int use_nbgl;
  uint8_t derivation_path[DERIVATION_PATH_MAX_LEN];
  int derivation_path_len;
};

struct app_s {
  char *name;
  char nvram_file_name[STORAGE_APP_NAME_LEN];
  bool load_nvram;
  bool save_nvram;
  int fd;
  bool use_nbgl;
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

typedef void (*pic_init_t)(void *pic_flash_start, void *pic_ram_start);

static struct memory_s memory;
static struct app_s apps[MAX_APP];
static unsigned int napp;
static unsigned long pic_init_addr;       // pic_init() addr in Shared lib
static unsigned long sh_svc_call_addr;    // SVC_Call addr in Shared lib
static unsigned long sh_svc_cx_call_addr; // SVC_cx_call addr in Shared lib

int g_api_level = 0;
hw_model_t hw_model = MODEL_COUNT;
extern bool pki_prod;

// app_flags extracted from app
uint64_t app_flags = 0;

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
static int open_app(char *name, char *filename, struct elf_info_s *elf,
                    bool load_nvram, bool save_nvram)
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

  size_t len = STORAGE_APP_NAME_LEN - 1;
  strncat(apps[napp].nvram_file_name, name, len);
  len -= strlen(name);
  strncat(apps[napp].nvram_file_name, "_nvram.bin", len);
  apps[napp].load_nvram = load_nvram;
  apps[napp].save_nvram = save_nvram;

  apps[napp].fd = fd;
  apps[napp].use_nbgl = (elf->use_nbgl != 0);
  apps[napp].elf.load_offset = elf->load_offset;
  apps[napp].elf.load_size = elf->load_size;
  apps[napp].elf.stack_addr = elf->stack_addr;
  apps[napp].elf.stack_size = elf->stack_size;
  apps[napp].elf.svc_call_addr = elf->svc_call_addr;
  apps[napp].elf.svc_cx_call_addr = elf->svc_cx_call_addr;
  apps[napp].elf.text_load_addr = elf->text_load_addr;
  apps[napp].elf.fonts_addr = elf->fonts_addr;
  apps[napp].elf.fonts_size = elf->fonts_size;
  apps[napp].elf.app_nvram_addr = elf->app_nvram_addr;
  apps[napp].elf.app_nvram_size = elf->app_nvram_size;
  apps[napp].elf.derivation_path_len = elf->derivation_path_len;
  memcpy(apps[napp].elf.derivation_path, elf->derivation_path,
         elf->derivation_path_len);
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

void *get_memory_code_address(void)
{
  return memory.code;
}

struct app_s *get_current_app(void)
{
  return current_app;
}

char *get_app_nvram_file_name(void)
{
  return current_app->nvram_file_name;
}

bool get_app_save_nvram(void)
{
  return current_app->save_nvram;
}

unsigned long get_app_nvram_address(void)
{
  return current_app->elf.app_nvram_addr;
}

unsigned long get_app_nvram_size(void)
{
  return current_app->elf.app_nvram_size;
}

unsigned long get_app_text_load_addr(void)
{
  return current_app->elf.text_load_addr;
}

unsigned long get_app_derivation_path(uint8_t **derivationPath)
{
  // use the derivation of the app, not libs
  *derivationPath = apps[0].elf.derivation_path;
  return apps[0].elf.derivation_path_len;
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
  if (app->elf.svc_call_addr != 0 || app->elf.svc_cx_call_addr != 0) {
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

  // Parse fonts and build bitmap -> character table
  parse_fonts(memory.code, app->elf.text_load_addr, app->elf.fonts_addr,
              app->elf.fonts_size, app->use_nbgl);

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
  void *data_addr;
  size_t data_size;
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
  if (app->elf.stack_addr == LINK_RAM_ADDR) {
    // Emulate RAM relocation
    data_addr = (void *)LOAD_RAM_ADDR;
    data_size = get_upper_page_aligned_size(app->elf.stack_size);
  }

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
  }

  if (mprotect(code, size, PROT_READ | PROT_WRITE) != 0) {
    warn("could not update mprotect in rw mode for app");
    goto error;
  }

  // If the syscall functions are not inlined and their symbols have been found
  // in the elf file, patch the elf at this address to remove the SVC 1 call
  if (app->elf.svc_call_addr != 0 || app->elf.svc_cx_call_addr != 0) {
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

  // App NVRAM data update
  if (app->elf.app_nvram_addr != 0) {
    unsigned long app_nvram_offset =
        app->elf.app_nvram_addr - app->elf.text_load_addr;
    memset(code + app_nvram_offset, 0, app->elf.app_nvram_size);
    size_t preload_file_size = 0;

    if (app->load_nvram) {
      FILE *fptr = fopen(app->nvram_file_name, "rb");
      if (fptr == NULL) {
        warnx("App NVRAM file %s is absent\n", app->nvram_file_name);
        goto error;
      }
      fseek(fptr, 0, SEEK_END);
      long lSize = ftell(fptr);
      rewind(fptr);

      uint8_t *buffer = (uint8_t *)malloc(sizeof(uint8_t) * lSize);
      if (buffer == NULL) {
        warnx("Error to allocate memory for app nvram read\n");
        fclose(fptr);
        goto error;
      }
      preload_file_size = fread(buffer, 1, lSize, fptr);
      if (preload_file_size != (size_t)lSize) {
        warnx("App nvram file size mismatch\n");
        free(buffer);
        fclose(fptr);
        goto error;
      }

      // The patch
      memcpy(code + app_nvram_offset, buffer, preload_file_size);

      free(buffer);
      fclose(fptr);
    }

    if (app->save_nvram) {
      /* Let's rename and thus backup the current file if present */
      char backup_file_name[STORAGE_APP_NAME_LEN + sizeof(STORAGE_BACKUP)] = {
        0
      };

      strcpy(backup_file_name, app->nvram_file_name);
      strcat(backup_file_name, STORAGE_BACKUP);
      rename(app->nvram_file_name, backup_file_name);

      if (preload_file_size > 0) {
        /* Let's save the initial content to the new file */
        FILE *fptr = fopen(app->nvram_file_name, "w");
        if (fptr == NULL) {
          err(1, "Failed to open the app NVRAM file %s\n",
              app->nvram_file_name);
        }
        if (fwrite(code + app_nvram_offset, 1, preload_file_size, fptr) !=
            preload_file_size) {
          errx(1, "App NVRAM write attempt failed\n");
        }
        fclose(fptr);
      }
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

  // Parse fonts and build bitmap -> character table
  parse_fonts(memory.code, app->elf.text_load_addr, app->elf.fonts_addr,
              app->elf.fonts_size, app->use_nbgl);

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

  int load_size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0L, SEEK_SET);

  fprintf(stderr, "[*] loading fonts from \"%s\" (%d)\n", fonts_path,
          load_size);

  int flags = MAP_PRIVATE | MAP_FIXED;
  int prot = PROT_READ;
  int load_addr = 0;

  switch (hw_model) {
  case MODEL_STAX:
    load_addr = STAX_FONTS_ARRAY_ADDR;
    break;
  case MODEL_FLEX:
    load_addr = FLEX_FONTS_ARRAY_ADDR;
    break;
  case MODEL_NANO_SP:
    load_addr = NANOSP_FONTS_ARRAY_ADDR;
    break;
  case MODEL_NANO_X:
    load_addr = NANOX_FONTS_ARRAY_ADDR;
    break;
  case MODEL_APEX_P:
    load_addr = APEX_P_FONTS_ARRAY_ADDR;
    break;
  default:
    warnx("hw_model %u not supported", hw_model);
    return -1;
  }

  void *p = mmap((void *)load_addr, load_size, prot, flags, fd, 0);

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

  int ret = sscanf(
      cxlib_args, "%[^:]:0x%x:0x%x:0x%x:0x%x:0x%x:0x%lx:0x%lx:0x%lx",
      cxlib_path, &sh_offset, &sh_size, &sh_load, &cx_ram_size, &cx_ram_load,
      &pic_init_addr, &sh_svc_call_addr, &sh_svc_cx_call_addr);

  if (ret != 9) {
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

  // Map CXRAM on devices
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

  if (mprotect(p, sh_size, PROT_READ | PROT_WRITE) != 0) {
    warn("could not update mprotect in rw mode for cxlib");
    return -1;
  }

  if (sh_svc_call_addr) {
    if (patch_svc_instr((unsigned char *)sh_svc_call_addr) != 0) {
      close(fd);
      return -1;
    }
    if (patch_svc_instr((unsigned char *)sh_svc_cx_call_addr) != 0) {
      close(fd);
      return -1;
    }
  } else if (patch_svc(p, sh_size) != 0) {
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
  pic_init_t pic_init;

  p = load_app(name);
  if (p == NULL) {
    return -1;
  }

  app = get_current_app();

  /* thumb mode */
  f = (void *)((unsigned long)p | 1);
  stack_end = app->elf.stack_addr;
  if (app->elf.stack_addr == LINK_RAM_ADDR) {
    // Emulate RAM relocation
    stack_end = LOAD_RAM_ADDR;
  }
  stack_start = stack_end + app->elf.stack_size;
  if (g_api_level >= 23) {
    // initialize shared library PIC
    pic_init = (pic_init_t)pic_init_addr;
    pic_init(LOAD_ADDR, (void *)LOAD_RAM_ADDR);
  }

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
 * Bitcoin:apps/btc.elf:0x1000:0x9fc0:0x20001800:0x1800 ...
 */
static char *parse_app_infos(char *arg, char **filename, struct elf_info_s *elf,
                             bool *load_nvram, bool *save_nvram)
{
  char *libname;
  char *derivation_path;
  int ret;
  int load_nvram_i = 0;
  int save_nvram_i = 0;

  libname = strdup(arg);
  *filename = strdup(arg);
  derivation_path = strdup(arg);
  if (libname == NULL || *filename == NULL || derivation_path == NULL) {
    err(1, "strdup");
  }

  ret = sscanf(arg,
               "%[^:]:%[^:]:0x%lx:0x%lx:0x%lx:0x%lx:0x%lx:0x%lx:0x%lx:0x%lx:0x%"
               "lx:0x%lx:0x%lx:%d:%d:%d:%[^:]",
               libname, *filename, &elf->load_offset, &elf->load_size,
               &elf->stack_addr, &elf->stack_size, &elf->svc_call_addr,
               &elf->svc_cx_call_addr, &elf->text_load_addr, &elf->fonts_addr,
               &elf->fonts_size, &elf->app_nvram_addr, &elf->app_nvram_size,
               &load_nvram_i, &save_nvram_i, &elf->use_nbgl, derivation_path);
  if (ret != 17) {
    warnx("failed to parse app infos (\"%s\", %d)", arg, ret);
    free(libname);
    free(*filename);
    return NULL;
  }
  *load_nvram = ((load_nvram_i == 1) ? true : false);
  *save_nvram = ((save_nvram_i == 1) ? true : false);

  // convert derivation path to byte array
  size_t len = strlen(derivation_path);
  if (len > 1) {
    for (size_t i = 0; i < len; i += 2) {
      sscanf(derivation_path + i, "%2hhx", &elf->derivation_path[i / 2]);
    }
    elf->derivation_path_len = len / 2;
  } else {
    elf->derivation_path_len = 0;
  }

  return libname;
}

static int load_apps(int argc, char *argv[])
{
  char *filename, *libname;
  struct elf_info_s elf;
  bool load_nvram = false;
  bool save_nvram = false;
  int i;

  for (i = 0; i < argc; i++) {
    libname =
        parse_app_infos(argv[i], &filename, &elf, &load_nvram, &save_nvram);
    if (libname == NULL) {
      return -1;
    }

    if (open_app(libname, filename, &elf, load_nvram, save_nvram) != 0) {
      return -1;
    }
  }

  return 0;
}

static void usage(char *argv0)
{
  fprintf(stderr,
          "Usage: %s -m <model> [-t] [-a <api_level>] <app.elf> "
          "[libname:lib.elf:0x1000:0x9fc0:0x20001800:0x1800 ...]\n",
          argv0);
  fprintf(stderr, "\n\
  -m <model>:           Optional string representing the device model being emula-\n\
                        ted. Currently supports \"nanosp\", \"nanox\", \"stax\", \"flex\" and \"apex_p\".\n\
  -a <api_level>:       A string representing the SDK api level to be used, like \"22\".\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
  char *cxlib_path = NULL;
  char *fonts_path = NULL;

  int opt;

  trace_syscalls = false;
  char *model_str = NULL;
  char *api_level_str = NULL;

  fprintf(stderr, "[*] speculos launcher revision: " GIT_REVISION "\n");

  while ((opt = getopt(argc, argv, "c:tr:s:m:k:a:f:p:l:")) != -1) {
    switch (opt) {
    case 'f':
      fonts_path = optarg;
      break;
    case 'c':
      cxlib_path = optarg;
      break;
    case 'a':
      api_level_str = optarg;
      break;
    case 't':
      trace_syscalls = true;
      break;
    case 'm':
      model_str = optarg;
      if (strcmp(optarg, "nanox") == 0) {
        hw_model = MODEL_NANO_X;
      } else if (strcmp(optarg, "nanosp") == 0) {
        hw_model = MODEL_NANO_SP;
      } else if (strcmp(optarg, "stax") == 0) {
        hw_model = MODEL_STAX;
      } else if (strcmp(optarg, "flex") == 0) {
        hw_model = MODEL_FLEX;
      } else if (strcmp(optarg, "apex_p") == 0) {
        hw_model = MODEL_APEX_P;
      } else {
        errx(1, "invalid model \"%s\"", optarg);
      }
      break;
    case 'p':
      pki_prod = true;
      break;
    case 'l':
      app_flags = atol(optarg);
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

  if (api_level_str != NULL) {
    g_api_level = atoi(api_level_str);
    if ((g_api_level < FIRST_SUPPORTED_API_LEVEL) ||
        (g_api_level > LAST_SUPPORTED_API_LEVEL)) {
      errx(1, "invalid SDK api_level: %d", g_api_level);
    }
    fprintf(stderr, "[*] using API_LEVEL version %d on %s\n", g_api_level,
            model_str);
  } else {
    errx(1, "missing SDK api_level argument");
  }

  make_openssl_random_deterministic();
  reset_memory(true);

  if (load_apps(argc - optind, &argv[optind]) != 0) {
    return 1;
  }

  if (load_cxlib(cxlib_path) != 0) {
    return 1;
  }

  init_environment();

  if (fonts_path) {
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
