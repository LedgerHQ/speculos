#define _SDK_2_0_
#include <err.h>
#include <errno.h>
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "bolos/cxlib.h"

//-----------------------------------------------------------------------------

bool sys_cx_bn_is_locked(void)
{
  // When a context is locked, this variable is not NULL.
  return cx_get_bn_ctx() != NULL;
}

bool sys_cx_bn_is_unlocked(void)
{
  // When a context is not locked, this variable is NULL.
  return cx_get_bn_ctx() == NULL;
}

cx_err_t cx_bn_locked(void)
{
  return sys_cx_bn_is_locked() ? CX_OK : CX_NOT_LOCKED;
}

cx_err_t cx_bn_unlocked(void)
{
  return sys_cx_bn_is_unlocked() ? CX_OK : CX_NOT_UNLOCKED;
}

cx_err_t sys_cx_bn_lock(size_t word_size, uint32_t flags)
{
  cx_err_t error;

  CX_CHECK(cx_bn_unlocked());
  CX_CHECK(cx_mpi_lock(word_size, flags));
end:
  return error;
}

uint32_t sys_cx_bn_unlock(void)
{
  cx_err_t error;

  CX_CHECK(cx_bn_locked());
  CX_CHECK(cx_mpi_unlock());
end:
  return error;
}

cx_err_t sys_cx_bn_alloc(cx_bn_t *bn_x, size_t size)
{
  cx_err_t error;

  CX_CHECK(cx_bn_locked());

  if (cx_mpi_alloc(bn_x, size) == NULL) {
    error = CX_MEMORY_FULL;
  } else {
    error = CX_OK;
  }
end:
  return error;
}

cx_err_t sys_cx_bn_destroy(cx_bn_t *bn_x)
{
  return cx_mpi_destroy(bn_x);
}

cx_err_t sys_cx_bn_init(cx_bn_t bn_x, const uint8_t *bytes, size_t nbytes)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_init(x, bytes, nbytes));
end:
  return error;
}

cx_err_t sys_cx_bn_alloc_init(cx_bn_t *bn_x, size_t size, const uint8_t *bytes,
                              size_t nbytes)
{
  cx_err_t error;

  CX_CHECK(sys_cx_bn_alloc(bn_x, size));
  CX_CHECK(sys_cx_bn_init(*bn_x, bytes, nbytes));
end:
  return error;
}

cx_err_t sys_cx_bn_export(const cx_bn_t bn_x, uint8_t *bytes, size_t nbytes)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_export(x, bytes, nbytes));
end:
  return error;
}

cx_err_t sys_cx_bn_copy(cx_bn_t bn_a, const cx_bn_t bn_b)
{
  cx_err_t error;
  cx_mpi_t *a, *b;

  if (bn_a == bn_b) {
    return CX_OK;
  }
  CX_CHECK(cx_bn_ab_to_mpi(bn_a, &a, bn_b, &b));
  // Copy b into a:
  CX_CHECK(cx_mpi_copy(a, b));
end:
  return error;
}

cx_err_t sys_cx_bn_nbytes(const cx_bn_t bn_x, size_t *nbytes)
{
  cx_err_t error;

  CX_CHECK(cx_bn_locked());
  CX_CHECK(cx_mpi_bytes(bn_x, nbytes));
end:
  return error;
}

cx_err_t sys_cx_bn_set_u32(cx_bn_t bn_x, uint32_t n)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  cx_mpi_set_u32(x, n);
end:
  return error;
}

cx_err_t sys_cx_bn_get_u32(const cx_bn_t bn_x, uint32_t *n)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  *n = cx_mpi_get_u32(x);
end:
  return error;
}

cx_err_t sys_cx_bn_cmp_u32(const cx_bn_t bn_a, uint32_t b, int *diff)
{
  cx_err_t error;
  cx_mpi_t *a;

  CX_CHECK(cx_bn_to_mpi(bn_a, &a));
  *diff = cx_mpi_cmp_u32(a, b);
end:
  return error;
}

cx_err_t sys_cx_bn_cmp(const cx_bn_t bn_a, const cx_bn_t bn_b, int *diff)
{
  cx_err_t error;
  cx_mpi_t *a, *b;

  CX_CHECK(cx_bn_ab_to_mpi(bn_a, &a, bn_b, &b));
  *diff = cx_mpi_cmp(a, b);
end:
  return error;
}

cx_err_t sys_cx_bn_rand(cx_bn_t bn_x)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_rand(x));
end:
  return error;
}

cx_err_t sys_cx_bn_rng(cx_bn_t bn_r, const cx_bn_t bn_n)
{
  cx_err_t error;
  cx_mpi_t *r, *n;

  CX_CHECK(cx_bn_ab_to_mpi(bn_r, &r, bn_n, &n));
  CX_CHECK(cx_mpi_rng(r, n));
end:
  return error;
}

cx_err_t sys_cx_bn_tst_bit(const cx_bn_t bn_x, uint32_t pos, bool *set)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_tst_bit(x, pos, set));
end:
  return error;
}

cx_err_t sys_cx_bn_set_bit(cx_bn_t bn_x, uint32_t pos)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_set_bit(x, pos));
end:
  return error;
}

cx_err_t sys_cx_bn_clr_bit(cx_bn_t bn_x, uint32_t pos)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_clr_bit(x, pos));
end:
  return error;
}

cx_err_t sys_cx_bn_cnt_bits(cx_bn_t bn_x, uint32_t *nbits)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  *nbits = cx_mpi_cnt_bits(x);
end:
  return error;
}

cx_err_t sys_cx_bn_shr(cx_bn_t bn_r, uint32_t n)
{
  cx_err_t error;
  cx_mpi_t *r;

  CX_CHECK(cx_bn_to_mpi(bn_r, &r));
  CX_CHECK(cx_mpi_shr(r, n));
end:
  return error;
}

cx_err_t sys_cx_bn_shl(cx_bn_t bn_r, uint32_t n)
{
  cx_err_t error;
  cx_mpi_t *r;

  CX_CHECK(cx_bn_to_mpi(bn_r, &r));
  CX_CHECK(cx_mpi_shl(r, n));
end:
  return error;
}

cx_err_t sys_cx_bn_xor(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b));
  CX_CHECK(cx_mpi_xor(r, a, b));
end:
  return error;
}

cx_err_t sys_cx_bn_or(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b));
  CX_CHECK(cx_mpi_or(r, a, b));
end:
  return error;
}

cx_err_t sys_cx_bn_and(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b));
  CX_CHECK(cx_mpi_and(r, a, b));
end:
  return error;
}

cx_err_t sys_cx_bn_add(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b));
  CX_CHECK(cx_mpi_add(r, a, b));
end:
  return error;
}

cx_err_t sys_cx_bn_sub(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b));
  CX_CHECK(cx_mpi_sub(r, a, b));
end:
  return error;
}

cx_err_t sys_cx_bn_mul(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b));
  CX_CHECK(cx_mpi_mul(r, a, b));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_add(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b,
                           const cx_bn_t bn_m)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b, *m;

  CX_CHECK(cx_bn_rabm_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b, bn_m, &m));
  CX_CHECK(cx_mpi_mod_add(r, a, b, m));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_sub(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b,
                           const cx_bn_t bn_m)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b, *m;

  CX_CHECK(cx_bn_rabm_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b, bn_m, &m));
  CX_CHECK(cx_mpi_mod_sub(r, a, b, m));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_mul(cx_bn_t bn_r, const cx_bn_t bn_a, const cx_bn_t bn_b,
                           const cx_bn_t bn_m)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *b, *m;

  CX_CHECK(cx_bn_rabm_to_mpi(bn_r, &r, bn_a, &a, bn_b, &b, bn_m, &m));
  CX_CHECK(cx_mpi_mod_mul(r, a, b, m));
end:
  return error;
}

cx_err_t sys_cx_bn_reduce(cx_bn_t bn_r, const cx_bn_t bn_d, const cx_bn_t bn_n)
{
  cx_err_t error;
  cx_mpi_t *r, *d, *n;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_d, &d, bn_n, &n));
  CX_CHECK(cx_mpi_rem(r, d, n));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_sqrt(cx_bn_t bn_r, const cx_bn_t bn_a,
                            const cx_bn_t bn_n, uint32_t sign)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *n;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_n, &n));
  CX_CHECK(cx_mpi_mod_sqrt(r, a, n, sign));
end:
  return error;
}

cx_err_t sys_cx_bn_is_odd(const cx_bn_t bn_a, bool *odd)
{
  cx_err_t error;
  cx_mpi_t *a;

  CX_CHECK(cx_bn_to_mpi(bn_a, &a));
  *odd = cx_mpi_is_odd(a);
end:
  return error;
}

cx_err_t sys_cx_bn_mod_invert_nprime(cx_bn_t bn_r, const cx_bn_t bn_a,
                                     const cx_bn_t bn_n)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *n;

  CX_CHECK(cx_bn_rab_to_mpi(bn_r, &r, bn_a, &a, bn_n, &n));
  CX_CHECK(cx_mpi_mod_invert_nprime(r, a, n));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_u32_invert(cx_bn_t bn_r, const uint32_t e,
                                  const cx_bn_t bn_n)
{
  cx_err_t error;
  cx_mpi_t *r, *n;

  CX_CHECK(cx_bn_ab_to_mpi(bn_r, &r, bn_n, &n));
  CX_CHECK(cx_mpi_mod_u32_invert(r, e, n));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_pow(cx_bn_t bn_r, const cx_bn_t bn_a,
                           const uint8_t *bytes_e, uint32_t len_e,
                           const cx_bn_t bn_n)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *e, *n;
  cx_bn_t bn_e;

  CX_CHECK(sys_cx_bn_alloc_init(&bn_e, cx_get_bn_size(), bytes_e, len_e));
  CX_CHECK(cx_bn_rabm_to_mpi(bn_r, &r, bn_a, &a, bn_e, &e, bn_n, &n));
  CX_CHECK(cx_mpi_mod_pow(r, a, e, n));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_pow_bn(cx_bn_t bn_r, const cx_bn_t bn_a,
                              const cx_bn_t bn_e, const cx_bn_t bn_n)
{
  cx_err_t error;
  cx_mpi_t *r, *a, *e, *n;

  CX_CHECK(cx_bn_rabm_to_mpi(bn_r, &r, bn_a, &a, bn_e, &e, bn_n, &n));
  CX_CHECK(cx_mpi_mod_pow(r, a, e, n));
end:
  return error;
}

cx_err_t sys_cx_bn_mod_pow2(cx_bn_t bn_r, const cx_bn_t bn_a,
                            const uint8_t *bytes_e, uint32_t len_e,
                            const cx_bn_t bn_n)
{
  return sys_cx_bn_mod_pow(bn_r, bn_a, bytes_e, len_e, bn_n);
}

cx_err_t sys_cx_bn_is_prime(const cx_bn_t bn_x, bool *prime)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_is_prime(x, prime));
end:
  return error;
}

cx_err_t sys_cx_bn_next_prime(const cx_bn_t bn_x)
{
  cx_err_t error;
  cx_mpi_t *x;

  CX_CHECK(cx_bn_to_mpi(bn_x, &x));
  CX_CHECK(cx_mpi_next_prime(x));
end:
  return error;
}
