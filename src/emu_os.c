#include <stdio.h>
#include <unistd.h>

#include "emulate.h"

#define BOLOS_TAG_APPNAME			0x01
#define BOLOS_TAG_APPVERSION	0x02

#define PATH_MAX  1024

#define MAX_LIBCALL	3

struct libcall_s {
    struct app_s *app;
    struct sigcontext sigcontext;
};

static try_context_t *try_context;
static struct libcall_s libcalls[MAX_LIBCALL];
static unsigned int libcall_index;

unsigned long sys_os_flags(void)
{
  /* not in recovery and mcu unsigned */
  return 0;
}

unsigned long sys_os_registry_get_current_app_tag(unsigned int tag, uint8_t *buffer, size_t length)
{
  if (length < 1) {
    return 0;
  }

  /* TODO */
  switch (tag) {
  case BOLOS_TAG_APPNAME:
    strncpy((char *)buffer, "speculos", length);
    break;
  case BOLOS_TAG_APPVERSION:
    strncpy((char *)buffer, "1.33.7", length);
    break;
  default:
    length = 0;
    break;
  }

  buffer[length] = '\x00';

  return length;
}

unsigned long sys_os_lib_call(unsigned long *call_parameters)
{
  char libname[PATH_MAX];

  if (libcall_index >= MAX_LIBCALL) {
    fprintf(stderr, "too many os_lib_call calls\n");
    _exit(1);
  }

  /* save current app to restore it later */
  save_current_context(&libcalls[libcall_index].sigcontext);
  libcalls[libcall_index].app = get_current_app();
  libcall_index += 1;

  /* libname must be on the stack */
  strncpy(libname, (char *)call_parameters[0], sizeof(libname));

  /* unmap current app */
  unload_running_app(false);

  /* map lib code and jump to main */
  run_lib(libname, &call_parameters[1]);

  /* never reached */
  return 0xdeadbeef;
}


unsigned long sys_os_version(uint8_t *buffer, unsigned int len)
{
  const char *kFirmVersion = "Speculos";
  const uint8_t kLen = strlen(kFirmVersion);

  if (len > 0) {
    strncpy((char *)buffer, kFirmVersion, len);
    if (len < kLen)
      return len;
    else return kLen; // This mimick the real behaviour that does return the data without the '\0'
  }

  return 0;
}


unsigned long sys_os_lib_end(void)
{
  if (libcall_index < 1) {
    fprintf(stderr, "too many os_lib_end calls\n");
    _exit(1);
  }

  libcall_index--;

  if (replace_current_code(libcalls[libcall_index].app) != 0) {
    fprintf(stderr, "os_lib_end failed\n");
    _exit(1);
  }

  replace_current_context(&libcalls[libcall_index].sigcontext);

  return 0;
}

unsigned long sys_try_context_set(try_context_t *context)
{
  try_context_t *previous_context;

  previous_context = try_context;
  try_context = context;

  return (unsigned long)previous_context;
}

unsigned long sys_try_context_get(void)
{
  return (unsigned long)try_context;
}

unsigned long sys_check_api_level(void)
{
  return 0;
}

unsigned long sys_os_sched_exit(void)
{
  fprintf(stderr, "[*] exit syscall called (skipped)\n");
  return 0;
}

unsigned long sys_reset(void)
{
  fprintf(stderr, "[*] reset syscall called (skipped)\n");
  return 0;
}

unsigned long sys_os_lib_throw(unsigned int exception)
{
  fprintf(stderr, "[*] os_lib_throw(0x%x) unhandled\n", exception);
  _exit(1);
  return 0;
}
