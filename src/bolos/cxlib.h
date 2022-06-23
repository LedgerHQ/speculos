#pragma once
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <stdbool.h>

#define _SDK_2_0_

#include "bolos/cx_aes.h"
#include "bolos/cx_ec.h"
#include "bolos/cx_ed25519.h"

//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
// Those defines can be found in the SDK, in cx_errors.h file:
#define CX_CHECK(call)                                                         \
  do {                                                                         \
    error = call;                                                              \
    if (error) {                                                               \
      goto end;                                                                \
    }                                                                          \
  } while (0)

#define CX_CHECK_IGNORE_CARRY(call)                                            \
  do {                                                                         \
    error = call;                                                              \
    if (error && error != CX_CARRY) {                                          \
      goto end;                                                                \
    }                                                                          \
  } while (0)

#define CX_OK 0x00000000

#define CX_CARRY 0xFFFFFF21

#define CX_LOCKED                  0xFFFFFF81
#define CX_UNLOCKED                0xFFFFFF82
#define CX_NOT_LOCKED              0xFFFFFF83
#define CX_NOT_UNLOCKED            0xFFFFFF84
#define CX_INTERNAL_ERROR          0xFFFFFF85
#define CX_INVALID_PARAMETER_SIZE  0xFFFFFF86
#define CX_INVALID_PARAMETER_VALUE 0xFFFFFF87
#define CX_INVALID_PARAMETER       0xFFFFFF88
#define CX_NOT_INVERTIBLE          0xFFFFFF89
#define CX_OVERFLOW                0xFFFFFF8A
#define CX_MEMORY_FULL             0xFFFFFF8B
#define CX_NO_RESIDUE              0xFFFFFF8C

#define CX_EC_INFINITE_POINT 0xFFFFFF41
#define CX_EC_INVALID_POINT  0xFFFFFFA2
#define CX_EC_INVALID_CURVE  0xFFFFFFA3

typedef uint32_t cx_err_t;

#define CX_APILEVEL 12

//-----------------------------------------------------------------------------
// Typedef & structs
//-----------------------------------------------------------------------------

typedef BIGNUM cx_mpi_t;

typedef struct cx_mpi_ecpoint_s {
  cx_mpi_t *x;
  cx_mpi_t *y;
  cx_mpi_t *z;
  cx_curve_t curve;

} cx_mpi_ecpoint_t;

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
// cx_mpi.c
void display_bytes(uint8_t *ptr, int len);
void display_mpi(cx_mpi_t *a);
void display_bn(uint32_t bn_r);
void display_BN_error(void);

cx_err_t cx_bn_to_mpi(const cx_bn_t bn_x, cx_mpi_t **x);
cx_err_t cx_bn_ab_to_mpi(const cx_bn_t bn_a, cx_mpi_t **a, const cx_bn_t bn_b,
                         cx_mpi_t **b);
cx_err_t cx_bn_rab_to_mpi(const cx_bn_t bn_r, cx_mpi_t **r, const cx_bn_t bn_a,
                          cx_mpi_t **a, const cx_bn_t bn_b, cx_mpi_t **b);
cx_err_t cx_bn_rabm_to_mpi(const cx_bn_t bn_r, cx_mpi_t **r, const cx_bn_t bn_a,
                           cx_mpi_t **a, const cx_bn_t bn_b, cx_mpi_t **b,
                           const cx_bn_t bn_m, cx_mpi_t **m);
uint32_t size_to_mpi_word_bytes(uint32_t size);
uint32_t cx_mpi_nbytes(const cx_mpi_t *x);
cx_err_t cx_mpi_bytes(const cx_bn_t bn_x, size_t *nbytes);
BN_CTX *cx_get_bn_ctx(void);
uint32_t cx_get_bn_size(void);
cx_err_t cx_mpi_destroy(cx_bn_t *bn_x);
cx_err_t cx_mpi_lock(size_t word_size, uint32_t flags __attribute__((unused)));
cx_err_t cx_mpi_unlock(void);
cx_mpi_t *cx_mpi_alloc(cx_bn_t *bn_x, size_t size);
cx_err_t cx_mpi_init(cx_mpi_t *x, const uint8_t *bytes, size_t nbytes);
cx_mpi_t *cx_mpi_alloc_init(cx_bn_t *bn_x, size_t size, const uint8_t *bytes,
                            size_t nbytes);
cx_err_t cx_mpi_export(const cx_mpi_t *x, uint8_t *bytes, size_t nbytes);
cx_err_t cx_mpi_copy(cx_mpi_t *dst, cx_mpi_t *src);
void cx_mpi_set_u32(cx_mpi_t *x, uint32_t n);
uint32_t cx_mpi_get_u32(const cx_mpi_t *x);
int32_t cx_mpi_cmp_u32(const cx_mpi_t *a, uint32_t b);
int cx_mpi_cmp(cx_mpi_t *a, cx_mpi_t *b);
int cx_mpi_is_odd(const cx_mpi_t *a);
int cx_mpi_is_zero(const cx_mpi_t *a);
int cx_mpi_is_one(const cx_mpi_t *a);
cx_err_t cx_mpi_tst_bit(const cx_mpi_t *x, const uint32_t pos, bool *set);
cx_err_t cx_mpi_set_bit(cx_mpi_t *x, const uint32_t pos);
cx_err_t cx_mpi_clr_bit(cx_mpi_t *x, const uint32_t pos);
uint32_t cx_mpi_cnt_bits(const cx_mpi_t *x);
cx_err_t cx_mpi_shr(cx_mpi_t *x, const uint32_t n);
cx_err_t cx_mpi_shl(cx_mpi_t *x, const uint32_t n);
cx_err_t cx_mpi_xor(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b);
cx_err_t cx_mpi_or(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b);
cx_err_t cx_mpi_and(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b);
cx_err_t cx_mpi_rand(cx_mpi_t *x);
cx_err_t cx_mpi_rng(cx_mpi_t *r, const cx_mpi_t *n);
cx_err_t cx_mpi_add(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b);
cx_err_t cx_mpi_sub(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b);
cx_err_t cx_mpi_mul(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *b);
cx_err_t cx_mpi_div(cx_mpi_t *r, cx_mpi_t *d, const cx_mpi_t *n);
cx_err_t cx_mpi_rem(cx_mpi_t *r, cx_mpi_t *d, const cx_mpi_t *n);
cx_err_t cx_mpi_mod_add(cx_mpi_t *r, cx_mpi_t *a, cx_mpi_t *b,
                        const cx_mpi_t *n);
cx_err_t cx_mpi_mod_sub(cx_mpi_t *r, cx_mpi_t *a, cx_mpi_t *b,
                        const cx_mpi_t *n);
cx_err_t cx_mpi_mod_mul(cx_mpi_t *r, cx_mpi_t *a, cx_mpi_t *b,
                        const cx_mpi_t *n);
cx_err_t cx_mpi_mod_sqrt(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *n,
                         uint32_t sign);
cx_err_t cx_mpi_mod_invert_nprime(cx_mpi_t *r, cx_mpi_t *a, const cx_mpi_t *n);
cx_err_t cx_mpi_mod_u32_invert(cx_mpi_t *r, uint32_t e, const cx_mpi_t *n);
cx_err_t cx_mpi_mod_pow(cx_mpi_t *r, const cx_mpi_t *a, const cx_mpi_t *e,
                        const cx_mpi_t *n);
cx_err_t cx_mpi_is_prime(cx_mpi_t *x, bool *prime);
cx_err_t cx_mpi_next_prime(cx_mpi_t *x);

// cx_bn.c
bool sys_cx_bn_is_locked(void);
bool sys_cx_bn_is_unlocked(void);
cx_err_t cx_bn_locked(void);
cx_err_t cx_bn_unlocked(void);
cx_err_t sys_cx_bn_lock(size_t word_nbytes, uint32_t flags);
uint32_t sys_cx_bn_unlock(void);
cx_err_t sys_cx_bn_alloc(cx_bn_t *bn_x, size_t size);
cx_err_t sys_cx_bn_destroy(cx_bn_t *bn_x);
cx_err_t sys_cx_bn_init(cx_bn_t bn_x, const uint8_t *bytes, size_t nbytes);
cx_err_t sys_cx_bn_alloc_init(cx_bn_t *bn_x, size_t size, const uint8_t *bytes,
                              size_t nbytes);
cx_err_t sys_cx_bn_nbytes(const cx_bn_t bn_x, size_t *nbytes);
cx_err_t sys_cx_bn_export(const cx_bn_t bn_x, uint8_t *bytes, size_t nbytes);
cx_err_t sys_cx_bn_copy(cx_bn_t bn_a, const cx_bn_t bn_b);
cx_err_t sys_cx_bn_set_u32(cx_bn_t bn_x, uint32_t n);
cx_err_t sys_cx_bn_get_u32(const cx_bn_t bn_x, uint32_t *n);
cx_err_t sys_cx_bn_cmp_u32(const cx_bn_t bn_a, uint32_t b, int *diff);
cx_err_t sys_cx_bn_cmp(const cx_bn_t bn_a, const cx_bn_t bn_b, int *diff);
cx_err_t sys_cx_bn_rand(cx_bn_t x);
cx_err_t sys_cx_bn_rng(cx_bn_t r, const cx_bn_t n);
cx_err_t sys_cx_bn_tst_bit(const cx_bn_t bn_x, uint32_t pos, bool *set);
cx_err_t sys_cx_bn_set_bit(cx_bn_t bn_x, uint32_t pos);
cx_err_t sys_cx_bn_clr_bit(cx_bn_t bn_x, uint32_t pos);
cx_err_t sys_cx_bn_shr(cx_bn_t r, uint32_t n);
cx_err_t sys_cx_bn_shl(cx_bn_t r, uint32_t n);
cx_err_t sys_cx_bn_cnt_bits(cx_bn_t bn_x, uint32_t *nbits);
cx_err_t sys_cx_bn_xor(cx_bn_t r, const cx_bn_t a, const cx_bn_t b);
cx_err_t sys_cx_bn_or(cx_bn_t r, const cx_bn_t a, const cx_bn_t b);
cx_err_t cx_mpi_not(cx_mpi_t *a);
cx_err_t cx_mpi_neg(cx_mpi_t *a);
cx_err_t sys_cx_bn_and(cx_bn_t r, const cx_bn_t a, const cx_bn_t b);
cx_err_t sys_cx_bn_add(cx_bn_t r, const cx_bn_t a, const cx_bn_t b);
cx_err_t sys_cx_bn_sub(cx_bn_t r, const cx_bn_t a, const cx_bn_t b);
cx_err_t cx_mpi_inc(cx_mpi_t *a);
cx_err_t cx_mpi_dec(cx_mpi_t *a);
cx_err_t sys_cx_bn_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b);
cx_err_t sys_cx_bn_mod_add(cx_bn_t r, const cx_bn_t a, const cx_bn_t b,
                           const cx_bn_t m);
cx_err_t sys_cx_bn_mod_sub(cx_bn_t r, const cx_bn_t a, const cx_bn_t b,
                           const cx_bn_t m);
cx_err_t sys_cx_bn_mod_mul(cx_bn_t r, const cx_bn_t a, const cx_bn_t b,
                           const cx_bn_t m);
cx_err_t sys_cx_bn_reduce(cx_bn_t r, const cx_bn_t d, const cx_bn_t n);
cx_err_t sys_cx_bn_mod_sqrt(cx_bn_t bn_r, const cx_bn_t bn_a,
                            const cx_bn_t bn_n, uint32_t sign);
cx_err_t sys_cx_bn_is_odd(const cx_bn_t bn_a, bool *odd);
cx_err_t sys_cx_bn_mod_invert_nprime(cx_bn_t bn_r, const cx_bn_t bn_a,
                                     const cx_bn_t bn_n);
cx_err_t sys_cx_bn_mod_u32_invert(cx_bn_t bn_r, const uint32_t e,
                                  const cx_bn_t bn_n);
cx_err_t sys_cx_bn_mod_pow(cx_bn_t bn_r, const cx_bn_t bn_a, const uint8_t *e,
                           uint32_t len_e, const cx_bn_t bn_n);
cx_err_t sys_cx_bn_mod_pow_bn(cx_bn_t bn_r, const cx_bn_t bn_a,
                              const cx_bn_t bn_e, const cx_bn_t bn_n);
cx_err_t sys_cx_bn_mod_pow2(cx_bn_t bn_r, const cx_bn_t bn_a, const uint8_t *e,
                            uint32_t len_e, const cx_bn_t bn_n);
cx_err_t sys_cx_bn_is_prime(const cx_bn_t bn_x, bool *prime);
cx_err_t sys_cx_bn_next_prime(const cx_bn_t bn_x);

// cx_ecdomain.c
int cx_nid_from_curve(cx_curve_t curve);
EC_GROUP *cx_group_from_nid_and_curve(int nid, cx_curve_t cid);
EC_GROUP *cx_create_generic_curve(cx_curve_t cid);
const cx_curve_domain_t *cx_ecdomain(cx_curve_t curve);
cx_err_t sys_cx_ecdomain_parameters_length(cx_curve_t curve, size_t *length);
cx_err_t sys_cx_ecdomain_size(cx_curve_t curve, size_t *length);
cx_err_t sys_cx_ecdomain_parameter(cx_curve_t curve, cx_curve_dom_param_t id,
                                   uint8_t *p, uint32_t p_len);
cx_err_t sys_cx_ecdomain_parameter_bn(cx_curve_t curve, cx_curve_dom_param_t id,
                                      cx_bn_t p);
cx_err_t sys_cx_ecdomain_generator(cx_curve_t curve, uint8_t *Gx, uint8_t *Gy,
                                   size_t len);
cx_err_t sys_cx_ecdomain_generator_bn(cx_curve_t cv, cx_ecpoint_t *P);

// cxlib.c
unsigned int sys_get_api_level(void);
cx_err_t sys_cx_get_random_bytes(void *buffer, size_t len);
cx_err_t sys_cx_trng_get_random_data(void *buffer, size_t len);
uint32_t sys_cx_crc32_hw(const void *_buf, size_t len);

// cx_aes_sdk2.c
cx_err_t sys_cx_aes_set_key_hw(const cx_aes_key_t *key, uint32_t mode);
cx_err_t sys_cx_aes_block_hw(const unsigned char *inblock,
                             unsigned char *outblock);
void sys_cx_aes_reset_hw(void);

// cx_twisted_edwards.c
cx_err_t cx_twisted_edwards_recover_x(cx_mpi_ecpoint_t *P, uint32_t sign);

// cx_weierstrass.c
cx_err_t cx_weierstrass_recover_y(cx_mpi_ecpoint_t *P, uint32_t sign);
// cx_montgomery.c
cx_err_t cx_montgomery_recover_y(cx_mpi_ecpoint_t *P, uint32_t sign);

// cx_ecpoint.c
cx_err_t cx_mpi_ecpoint_from_ecpoint(cx_mpi_ecpoint_t *P,
                                     const cx_ecpoint_t *Q);
cx_err_t sys_cx_ecpoint_alloc(cx_ecpoint_t *P, cx_curve_t cv);
cx_err_t sys_cx_ecpoint_destroy(cx_ecpoint_t *P);
cx_err_t sys_cx_ecpoint_init(cx_ecpoint_t *ec_P, const uint8_t *x, size_t x_len,
                             const uint8_t *y, size_t y_len);
cx_err_t sys_cx_ecpoint_init_bn(cx_ecpoint_t *ec_P, const cx_bn_t bn_x,
                                const cx_bn_t bn_y);
cx_err_t sys_cx_ecpoint_export_bn(const cx_ecpoint_t *ec_P, cx_bn_t *bn_x,
                                  cx_bn_t *bn_y);
cx_err_t sys_cx_ecpoint_export(const cx_ecpoint_t *P, uint8_t *x, size_t x_len,
                               uint8_t *y, size_t y_len);
cx_err_t sys_cx_ecpoint_compress(const cx_ecpoint_t *P, uint8_t *xy_compressed,
                                 size_t xy_compressed_len, uint32_t *sign);
cx_err_t sys_cx_ecpoint_decompress(cx_ecpoint_t *ec_P,
                                   const uint8_t *xy_compressed,
                                   size_t xy_compressed_len, uint32_t sign);
cx_err_t sys_cx_ecpoint_add(cx_ecpoint_t *ec_R, const cx_ecpoint_t *ec_P,
                            const cx_ecpoint_t *ec_Q);
cx_err_t sys_cx_ecpoint_neg(cx_ecpoint_t *ec_P);
cx_err_t sys_cx_ecpoint_cmp(const cx_ecpoint_t *ec_P, const cx_ecpoint_t *ec_Q,
                            bool *is_equal);
cx_err_t sys_cx_ecpoint_scalarmul(cx_ecpoint_t *ec_P, const uint8_t *k,
                                  size_t k_len);
cx_err_t sys_cx_ecpoint_scalarmul_bn(cx_ecpoint_t *ec_P, const cx_bn_t bn_k);
cx_err_t sys_cx_ecpoint_rnd_scalarmul(cx_ecpoint_t *ec_P, const uint8_t *k,
                                      size_t k_len);
cx_err_t sys_cx_ecpoint_rnd_scalarmul_bn(cx_ecpoint_t *ec_P,
                                         const cx_bn_t bn_k);
cx_err_t sys_cx_ecpoint_rnd_fixed_scalarmul(cx_ecpoint_t *ec_P,
                                            const uint8_t *k, size_t k_len);
cx_err_t sys_cx_ecpoint_double_scalarmul(cx_ecpoint_t *ec_R, cx_ecpoint_t *ec_P,
                                         cx_ecpoint_t *ec_Q, const uint8_t *k,
                                         size_t k_len, const uint8_t *r,
                                         size_t r_len);
cx_err_t sys_cx_ecpoint_double_scalarmul_bn(cx_ecpoint_t *ec_R,
                                            cx_ecpoint_t *ec_P,
                                            cx_ecpoint_t *ec_Q,
                                            const cx_bn_t bn_k,
                                            const cx_bn_t bn_r);
cx_err_t sys_cx_ecpoint_is_on_curve(const cx_ecpoint_t *ec_P,
                                    bool *is_on_curve);
cx_err_t sys_cx_ecpoint_is_at_infinity(const cx_ecpoint_t *ec_P,
                                       bool *is_infinite);
