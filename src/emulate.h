#pragma once

/* for ucontext_t */
#include <signal.h>

/* for bool */
#include <stdbool.h>

/* for uint8_t, uint16_t, etc. */
#include <stdint.h>

/* for BIGNUM */
#include <openssl/bn.h>

#ifndef _SDK_2_0_
/* for cx_curve_t */
#include "bolos/cx_ec.h"
#endif //_SDK_2_0_

/* for cx_hash_t */
#include "bolos/cx_hash.h"

#include "bolos/cx_crc.h"

#include "sdk.h"

/* TODO */
typedef void bolos_ux_params_t;
typedef struct try_context_s try_context_t;

typedef struct cx_ecfp_256_public_key_s cx_ecfp_public_key_t;
typedef struct cx_ecfp_256_private_key_s cx_ecfp_private_key_t;

struct app_s;

#define SEPH_FILENO 0 /* 0 is stdin fileno */

#ifndef UNUSED
#ifdef __GNUC__
#define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#define UNUSED(x) UNUSED_##x
#endif
#endif

int emulate(unsigned long syscall, unsigned long *parameters,
            unsigned long *ret, bool verbose, unsigned int sdk_version);
int emulate_common(unsigned long syscall, unsigned long *parameters,
                   unsigned long *ret, bool verbose);

int emulate_1_2(unsigned long syscall, unsigned long *parameters,
                unsigned long *ret, bool verbose);
int emulate_1_5(unsigned long syscall, unsigned long *parameters,
                unsigned long *ret, bool verbose);
int emulate_1_6(unsigned long syscall, unsigned long *parameters,
                unsigned long *ret, bool verbose);
int emulate_2_0(unsigned long syscall, unsigned long *parameters,
                unsigned long *ret, bool verbose);
int emulate_blue_2_2_5(unsigned long syscall, unsigned long *parameters,
                       unsigned long *ret, bool verbose);
int emulate_nanosp_1_0(unsigned long syscall, unsigned long *parameters,
                       unsigned long *ret, bool verbose);

unsigned long sys_os_version(uint8_t *buffer, unsigned int len);
unsigned int sys_os_serial(unsigned char *serial, unsigned int maxlength);
unsigned long sys_os_seph_version(uint8_t *buffer, size_t len);
unsigned long sys_os_lib_call(unsigned long *parameters);
unsigned long sys_os_lib_end(void);
unsigned long sys_os_global_pin_is_validated_1_2(void);
unsigned long sys_os_global_pin_is_validated_1_5(void);
unsigned long sys_os_global_pin_is_validated_1_6(void);
unsigned long sys_os_global_pin_is_validated_2_0(void);
unsigned long sys_os_global_pin_invalidate(void);
unsigned long sys_os_perso_isonboarded_1_2(void);
unsigned long sys_os_perso_isonboarded_1_5(void);
unsigned long sys_os_perso_isonboarded_1_6(void);
unsigned long sys_os_perso_isonboarded_2_0(void);
unsigned long sys_os_flags(void);
int sys_nvm_write(void *dst_addr, void *src_addr, size_t src_len);
int sys_nvm_erase(void *dst_addr, size_t src_len);
int sys_nvm_erase_page(unsigned int page_adr);

unsigned long sys_os_perso_derive_node_bip32(cx_curve_t curve,
                                             const uint32_t *path,
                                             size_t length,
                                             uint8_t *private_key,
                                             uint8_t *chain);
unsigned long sys_os_perso_derive_node_with_seed_key(
    unsigned int mode, cx_curve_t curve, const unsigned int *path,
    unsigned int pathLength, unsigned char *privateKey, unsigned char *chain,
    unsigned char *seed_key, unsigned int seed_key_length);
unsigned long sys_os_perso_derive_node_bip32_seed_key(
    unsigned int mode, cx_curve_t curve, const unsigned int *path,
    unsigned int pathLength, unsigned char *privateKey, unsigned char *chain,
    unsigned char *seed_key, unsigned int seed_key_length);
unsigned int sys_os_perso_seed_cookie(unsigned char *seed_cookie,
                                      unsigned int seed_cookie_length);
unsigned long sys_os_perso_derive_eip2333(cx_curve_t curve,
                                          const unsigned int *path,
                                          unsigned int pathLength,
                                          unsigned char *privateKey);
unsigned long sys_os_perso_isonboarded(void);
unsigned long sys_os_setting_get(unsigned int setting_id, uint8_t *value,
                                 size_t maxlen);
unsigned long sys_os_registry_get_current_app_tag(unsigned int tag,
                                                  uint8_t *buffer,
                                                  size_t length);
unsigned long sys_os_ux_1_2(bolos_ux_params_t *params);
unsigned long sys_os_ux_1_5(bolos_ux_params_t *params);
unsigned long sys_os_ux_1_6(bolos_ux_params_t *params);
unsigned long sys_os_ux_2_0(bolos_ux_params_t *params);

unsigned long sys_cx_hash(cx_hash_t *hash, int mode, const uint8_t *in,
                          size_t len, uint8_t *out, size_t out_len);
unsigned long sys_cx_rng(uint8_t *buffer, unsigned int length);
unsigned long sys_cx_rng_u8(void);
unsigned long sys_cx_rng_u32(void);
void make_openssl_random_deterministic(void);

unsigned long sys_io_seproxyhal_spi_is_status_sent(void);
unsigned long sys_io_seproxyhal_spi_send(const uint8_t *buffer,
                                         uint16_t length);
unsigned long sys_io_seproxyhal_spi_recv(uint8_t *buffer, uint16_t maxlength,
                                         unsigned int flags);

unsigned long sys_io_seph_is_status_sent(void);
unsigned long sys_io_seph_send(const uint8_t *buffer, uint16_t length);
unsigned long sys_io_seph_recv(uint8_t *buffer, uint16_t maxlength,
                               unsigned int flags);

void unload_running_app(bool unload_data);
struct app_s *get_current_app(void);
void save_current_context(struct sigcontext *sigcontext);
void replace_current_context(struct sigcontext *sigcontext);
int replace_current_code(struct app_s *app);
int run_lib(char *name, unsigned long *parameters);

unsigned long sys_try_context_set(try_context_t *context);
unsigned long sys_try_context_get(void);

unsigned long sys_os_sched_last_status_1_2(unsigned int task_idx);
unsigned long sys_os_sched_last_status_1_5(void);
unsigned long sys_os_sched_last_status_1_6(unsigned int task_idx);
unsigned long sys_os_sched_last_status_2_0(unsigned int task_idx);

unsigned long sys_check_api_level(void);

unsigned long sys_os_sched_exit(unsigned int code);
unsigned long sys_reset(void);

unsigned long sys_os_lib_throw(unsigned int exception);

#define print_syscall(fmt, ...)                                                \
  do {                                                                         \
    if (verbose) {                                                             \
      fprintf(stderr, "[*] syscall: " fmt, __VA_ARGS__);                       \
    }                                                                          \
  } while (0)

#define print_ret(ret)                                                         \
  do {                                                                         \
    if (verbose) {                                                             \
      fprintf(stderr, " = %lu (0x%lx)\n", ret, ret);                           \
    }                                                                          \
  } while (0)

#ifdef _SDK_2_0_
#define GET_RETID(name)
#else //_SDK_2_0__
#define GET_RETID(name)                                                        \
  do {                                                                         \
    retid = name;                                                              \
  } while (0)
#endif //_SDK_2_0_

#define SYSCALL0(_name)                                                        \
  case SYSCALL_##_name##_ID_IN: {                                              \
    *ret = sys_##_name();                                                      \
    print_syscall(#_name "(%s)", "");                                          \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    print_ret(*ret);                                                           \
    break;                                                                     \
  }

#define SYSCALL0v(_name)                                                       \
  case SYSCALL_##_name##_ID_IN: {                                              \
    sys_##_name();                                                             \
    print_syscall(#_name "(%s)", "");                                          \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL0i(_name, _funcname)                                            \
  case SYSCALL_##_name##_ID_IN: {                                              \
    *ret = sys_##_funcname();                                                  \
    print_syscall(#_name "(%s)", "");                                          \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    print_ret(*ret);                                                           \
    break;                                                                     \
  }

#define SYSCALL1(_name, _fmt, _type0, _arg0)                                   \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0);                              \
    *ret = sys_##_name(_arg0);                                                 \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL1i(_name, _fmt, _type0, _arg0, _funcname)                       \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0);                              \
    *ret = sys_##_funcname(_arg0);                                             \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL2(_name, _fmt, _type0, _arg0, _type1, _arg1)                    \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1);               \
    *ret = sys_##_name(_arg0, _arg1);                                          \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL3(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2)     \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1,                \
                  (_type2)_arg2);                                              \
    *ret = sys_##_name(_arg0, _arg1, _arg2);                                   \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL4(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2,     \
                 _type3, _arg3)                                                \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    _type3 _arg3 = (_type3)parameters[3];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, \
                  (_type3)_arg3);                                              \
    *ret = sys_##_name(_arg0, _arg1, _arg2, _arg3);                            \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL5(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2,     \
                 _type3, _arg3, _type4, _arg4)                                 \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    _type3 _arg3 = (_type3)parameters[3];                                      \
    _type4 _arg4 = (_type4)parameters[4];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, \
                  (_type3)_arg3, (_type4)_arg4);                               \
    *ret = sys_##_name(_arg0, _arg1, _arg2, _arg3, _arg4);                     \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL6(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2,     \
                 _type3, _arg3, _type4, _arg4, _type5, _arg5)                  \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    _type3 _arg3 = (_type3)parameters[3];                                      \
    _type4 _arg4 = (_type4)parameters[4];                                      \
    _type5 _arg5 = (_type5)parameters[5];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, \
                  (_type3)_arg3, (_type4)_arg4, (_type5)_arg5);                \
    *ret = sys_##_name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5);              \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL7(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2,     \
                 _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6)   \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    _type3 _arg3 = (_type3)parameters[3];                                      \
    _type4 _arg4 = (_type4)parameters[4];                                      \
    _type5 _arg5 = (_type5)parameters[5];                                      \
    _type6 _arg6 = (_type6)parameters[6];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, \
                  (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6); \
    *ret = sys_##_name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6);       \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL8(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2,     \
                 _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6,   \
                 _type7, _arg7)                                                \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    _type3 _arg3 = (_type3)parameters[3];                                      \
    _type4 _arg4 = (_type4)parameters[4];                                      \
    _type5 _arg5 = (_type5)parameters[5];                                      \
    _type6 _arg6 = (_type6)parameters[6];                                      \
    _type7 _arg7 = (_type7)parameters[7];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, \
                  (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6,  \
                  (_type7)_arg7);                                              \
    *ret =                                                                     \
        sys_##_name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7);   \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL9(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2,     \
                 _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6,   \
                 _type7, _arg7, _type8, _arg8)                                 \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    _type3 _arg3 = (_type3)parameters[3];                                      \
    _type4 _arg4 = (_type4)parameters[4];                                      \
    _type5 _arg5 = (_type5)parameters[5];                                      \
    _type6 _arg6 = (_type6)parameters[6];                                      \
    _type7 _arg7 = (_type7)parameters[7];                                      \
    _type8 _arg8 = (_type8)parameters[8];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, \
                  (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6,  \
                  (_type7)_arg7, (_type8)_arg8);                               \
    *ret = sys_##_name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, \
                       _arg8);                                                 \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }

#define SYSCALL10(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2,    \
                  _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6,  \
                  _type7, _arg7, _type8, _arg8, _type9, _arg9)                 \
  case SYSCALL_##_name##_ID_IN: {                                              \
    _type0 _arg0 = (_type0)parameters[0];                                      \
    _type1 _arg1 = (_type1)parameters[1];                                      \
    _type2 _arg2 = (_type2)parameters[2];                                      \
    _type3 _arg3 = (_type3)parameters[3];                                      \
    _type4 _arg4 = (_type4)parameters[4];                                      \
    _type5 _arg5 = (_type5)parameters[5];                                      \
    _type6 _arg6 = (_type6)parameters[6];                                      \
    _type7 _arg7 = (_type7)parameters[7];                                      \
    _type8 _arg8 = (_type8)parameters[8];                                      \
    _type9 _arg9 = (_type9)parameters[9];                                      \
    print_syscall(#_name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, \
                  (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6,  \
                  (_type7)_arg7, (_type8)_arg8, (_type9)_arg9);                \
    *ret = sys_##_name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, \
                       _arg8, _arg9);                                          \
    print_ret(*ret);                                                           \
    GET_RETID(SYSCALL_##_name##_ID_OUT);                                       \
    break;                                                                     \
  }
