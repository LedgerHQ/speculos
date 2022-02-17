#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "emulate.h"
#include "svc.h"

#define OS_SETTING_PLANEMODE_OLD 5
#define OS_SETTING_PLANEMODE_NEW 6

#define BOLOS_TAG_APPNAME    0x01
#define BOLOS_TAG_APPVERSION 0x02

#undef PATH_MAX
#define PATH_MAX 1024

#define MAX_LIBCALL 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct libcall_s {
  struct app_s *app;
  struct sigcontext sigcontext;
  try_context_t *trycontext;
};

static try_context_t *try_context;
static struct libcall_s libcalls[MAX_LIBCALL];
static unsigned int libcall_index;

unsigned long sys_os_flags(void)
{
  /* not in recovery and mcu unsigned */
  return 0;
}

unsigned long sys_os_perso_isonboarded(void)
{
  return true;
}

unsigned long sys_os_setting_get(unsigned int setting_id,
                                 uint8_t *UNUSED(value), size_t UNUSED(maxlen))
{
  // Since Nano X SDK 2.0 & Nano S SDK 2.1, OS_SETTING_PLANEMODE is 6!
  if (sdk_version == SDK_NANO_X_2_0 || sdk_version == SDK_NANO_X_2_0_2 ||
      sdk_version == SDK_NANO_S_2_1) {
    if (setting_id == OS_SETTING_PLANEMODE_NEW) {
      return 1;
    }
  } else if (setting_id == OS_SETTING_PLANEMODE_OLD) {
    return 1;
  }
  fprintf(stderr, "os_setting_get not implemented for 0x%x\n", setting_id);

  return 0;
}

unsigned long sys_os_registry_get_current_app_tag(unsigned int tag,
                                                  uint8_t *buffer,
                                                  size_t length)
{
  char *name, *p, *str, *version;

  if (length < 1) {
    return 0;
  }

  name = "app";
  version = "1.33.7";

  str = getenv("SPECULOS_APPNAME");
  if (str != NULL && (str = strdup(str)) != NULL) {
    p = strstr(str, ":");
    if (p != NULL) {
      *p = '\x00';
      name = str;
      version = p + 1;
    }
  }

  switch (tag) {
  case BOLOS_TAG_APPNAME:
    strncpy((char *)buffer, name, length);
    length = MIN(length, strlen(name));
    break;
  case BOLOS_TAG_APPVERSION:
    strncpy((char *)buffer, version, length);
    length = MIN(length, strlen(version));
    break;
  default:
    length = 0;
    break;
  }

  buffer[length] = '\x00';
  free(str);

  return length;
}

unsigned long sys_os_lib_call(unsigned long *call_parameters)
{
  char libname[PATH_MAX + 1];

  if (libcall_index >= MAX_LIBCALL) {
    fprintf(stderr, "too many os_lib_call calls\n");
    _exit(1);
  }

  /* save current try_context of the caller */
  libcalls[libcall_index].trycontext = try_context;

  /* save current app to restore it later */
  save_current_context(&libcalls[libcall_index].sigcontext);
  libcalls[libcall_index].app = get_current_app();
  libcall_index += 1;

  /* libname must be on the stack */
  strncpy(libname, (char *)call_parameters[0], sizeof(libname) - 1);

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
    else
      return kLen; // This mimics the real behaviour that does return the data
                   // without the '\0'
  }

  return 0;
}

unsigned long sys_os_seph_version(uint8_t *buffer, size_t len)
{
  const char *kMcuVersion = "SpeculosMCU";
  const size_t kLen = strlen(kMcuVersion);

  if (len > 0) {
    strncpy((char *)buffer, kMcuVersion, len);
    if (len < kLen) {
      return len;
    } else {
      return kLen;
    }
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

  /* restore try_context of the caller */
  try_context = libcalls[libcall_index].trycontext;

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

unsigned long sys_os_sched_exit(unsigned int code)
{
  fprintf(stderr, "[*] exit called (%u)\n", code);
  _exit(0);
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
