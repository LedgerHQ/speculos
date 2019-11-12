#ifndef _EMULATE_H
#define _EMULATE_H

/* for ucontext_t */
#include <signal.h>

/* for bool */
#include <stdbool.h>

/* for uint8_t, uint16_t, etc. */
#include <stdint.h>

/* for cx_curve_t */
#include "cx_ec.h"

/* for cx_hash_t */
#include "cx_hash.h"

typedef enum {
  SDK_1_5,
  SDK_1_6,
  SDK_BLUE_2_2_5,
  SDK_LAST
} sdk_version_t;

// Max number of characters allowed in version number
#define MAX_SDK_VER_LEN  6

/* TODO */
typedef void bolos_ux_params_t;
typedef struct try_context_s try_context_t;

typedef struct cx_ecfp_256_public_key_s cx_ecfp_public_key_t;
typedef struct cx_ecfp_256_private_key_s cx_ecfp_private_key_t;

struct app_s;

#define SEPH_FILENO	0 /* 0 is stdin fileno */

#ifndef UNUSED
# ifdef __GNUC__
#   define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
# else
#   define UNUSED(x) UNUSED_ ## x
# endif
#endif

int emulate(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose, unsigned int sdk_version);
int emulate_common(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose);

int emulate_1_5(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose);
int emulate_1_6(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose);
int emulate_blue_2_2_5(unsigned long syscall, unsigned long *parameters, unsigned long *ret, bool verbose);

unsigned long sys_os_version(uint8_t *buffer, unsigned int len);
unsigned long sys_os_lib_call(unsigned long *parameters);
unsigned long sys_os_lib_end(void);
unsigned long sys_os_global_pin_is_validated(void);
unsigned long sys_os_global_pin_invalidate(void);
unsigned long sys_os_perso_isonboarded(void);
unsigned long sys_os_flags(void);
int sys_nvm_write(void *dst_addr, void* src_addr, size_t src_len);
unsigned long sys_os_perso_derive_node_bip32(cx_curve_t curve, const uint32_t *path, size_t length, uint8_t *private_key, uint8_t* chain);
unsigned long sys_os_registry_get_current_app_tag(unsigned int tag, uint8_t *buffer, size_t length);
unsigned long sys_os_ux(bolos_ux_params_t *params);

int sys_cx_ecdsa_sign(const cx_ecfp_private_key_t *key, int mode, cx_md_t hashID, const uint8_t *hash, unsigned int hash_len, uint8_t *sig, unsigned int sig_len, unsigned int *info);
int sys_cx_ecdsa_verify(const cx_ecfp_public_key_t *key, int mode, cx_md_t hashID, const uint8_t *hash, unsigned int hash_len, const uint8_t *sig,  unsigned int sig_len);
int sys_cx_ecfp_generate_pair(cx_curve_t curve, cx_ecfp_public_key_t *public_key, cx_ecfp_private_key_t *private_key, int keep_private);
int sys_cx_ecfp_init_private_key(cx_curve_t curve, const uint8_t *raw_key, unsigned int key_len, cx_ecfp_private_key_t *key);
unsigned long sys_cx_hash(cx_hash_t *hash, int mode, const uint8_t *in, size_t len, uint8_t *out, size_t out_len);
unsigned long sys_cx_rng(uint8_t *buffer, unsigned int length);
unsigned long sys_cx_rng_u8(void);

unsigned long sys_io_seproxyhal_spi_is_status_sent(void);
unsigned long sys_io_seproxyhal_spi_send(const uint8_t *buffer, uint16_t length);
unsigned long sys_io_seproxyhal_spi_recv(uint8_t *buffer, uint16_t maxlength, unsigned int flags);

unsigned long sys_io_seph_is_status_sent(void);
unsigned long sys_io_seph_send(const uint8_t *buffer, uint16_t length);
unsigned long sys_io_seph_recv(uint8_t *buffer, uint16_t maxlength, unsigned int flags);

void unload_running_app(bool unload_data);
struct app_s *get_current_app(void);
void save_current_context(struct sigcontext *sigcontext);
void replace_current_context(struct sigcontext *sigcontext);
int replace_current_code(struct app_s *app);
int run_lib(char *name, unsigned long *parameters);

unsigned long sys_try_context_set(try_context_t *context);
unsigned long sys_try_context_get(void);

unsigned long sys_os_sched_last_status_1_5(void);
unsigned long sys_os_sched_last_status_1_6(unsigned int task_idx);

unsigned long sys_check_api_level(void);

unsigned long sys_os_sched_exit(void);
unsigned long sys_reset(void);

unsigned long sys_os_lib_throw(unsigned int exception);

#define print_syscall(fmt, ...)                           \
  do {                                                    \
    if (verbose) {                                        \
      fprintf(stderr, "[*] syscall: " fmt, __VA_ARGS__);  \
    }                                                     \
  } while (0)

#define print_ret(ret)                                    \
  do {                                                    \
    if (verbose) {                                        \
      fprintf(stderr, " = %ld\n", ret);                   \
    }                                                     \
  } while (0)

#define SYSCALL0(_name)                              \
  case SYSCALL_ ## _name ## _ID_IN: {                \
    *ret = sys_ ## _name();                          \
    print_syscall(# _name "(%s)", "");               \
    retid = SYSCALL_ ## _name ## _ID_OUT;            \
    print_ret(*ret);                                 \
    break;                                           \
  }

#define SYSCALL0i(_name, _funcname)                  \
  case SYSCALL_ ## _name ## _ID_IN: {                \
    *ret = sys_ ## _funcname();                      \
    print_syscall(# _name "(%s)", "");               \
    retid = SYSCALL_ ## _name ## _ID_OUT;            \
    print_ret(*ret);                                 \
    break;                                           \
  }

#define SYSCALL1(_name, _fmt, _type0, _arg0)          \
  case SYSCALL_ ## _name ## _ID_IN: {                 \
    _type0 _arg0 = (_type0)parameters[0];             \
    print_syscall(# _name "" _fmt, (_type0)_arg0);    \
    *ret = sys_ ## _name(_arg0);                      \
    print_ret(*ret);                                  \
    retid = SYSCALL_ ## _name ## _ID_OUT;             \
    break;                                            \
  }

#define SYSCALL1i(_name, _fmt, _type0, _arg0, _funcname) \
  case SYSCALL_ ## _name ## _ID_IN: {                 \
    _type0 _arg0 = (_type0)parameters[0];             \
    print_syscall(# _name "" _fmt, (_type0)_arg0);    \
    *ret = sys_ ## _funcname(_arg0);                  \
    print_ret(*ret);                                  \
    retid = SYSCALL_ ## _name ## _ID_OUT;             \
    break;                                            \
  }


#define SYSCALL2(_name, _fmt, _type0, _arg0, _type1, _arg1)             \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1);       \
    *ret = sys_ ## _name(_arg0, _arg1);                                 \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL3(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2);                          \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL4(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3);                   \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL5(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4);            \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL6(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4, (_type5)_arg5); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5);     \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL7(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    _type6 _arg6 = (_type6)parameters[6];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6); \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#define SYSCALL8(_name, _fmt, _type0, _arg0, _type1, _arg1, _type2, _arg2, _type3, _arg3, _type4, _arg4, _type5, _arg5, _type6, _arg6, _type7, _arg7) \
  case SYSCALL_ ## _name ## _ID_IN: {                                   \
    _type0 _arg0 = (_type0)parameters[0];                               \
    _type1 _arg1 = (_type1)parameters[1];                               \
    _type2 _arg2 = (_type2)parameters[2];                               \
    _type3 _arg3 = (_type3)parameters[3];                               \
    _type4 _arg4 = (_type4)parameters[4];                               \
    _type5 _arg5 = (_type5)parameters[5];                               \
    _type6 _arg6 = (_type6)parameters[6];                               \
    _type7 _arg7 = (_type7)parameters[7];                               \
    print_syscall(# _name "" _fmt, (_type0)_arg0, (_type1)_arg1, (_type2)_arg2, (_type3)_arg3, (_type4)_arg4, (_type5)_arg5, (_type6)_arg6, (_type7)_arg7); \
    *ret = sys_ ## _name(_arg0, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7); \
    print_ret(*ret);                                                    \
    retid = SYSCALL_ ## _name ## _ID_OUT;                               \
    break;                                                              \
  }

#endif
