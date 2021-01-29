#include <err.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <openssl/bn.h>

/* Arbitrary maximum number size in bytes. Operations on numbers with size
 * larger than this limit would throw an exception on a real device. */
#define MAX_SIZE 4096

/*
 * Assumes tolen is less than or equal to MAX_SIZE.
 * Return true if the result is truncated, false otherwise.
 */
static bool BN_bn2binpad_truncate(const BIGNUM *a, uint8_t *to, int tolen)
{
  uint8_t buf[MAX_SIZE + 1];

  if (BN_bn2binpad(a, to, tolen) != -1) {
    return false;
  }

  /* this can't happen if tolen <= MAX_SIZE */
  if (BN_bn2binpad(a, buf, sizeof(buf)) == -1) {
    errx(1, "BN_bn2binpad failed in BN_bn2binpad_truncate");
  }

  memcpy(to, buf + sizeof(buf) - tolen, tolen);

  return true;
}

int sys_cx_math_cmp(const uint8_t *a, const uint8_t *b, unsigned int len)
{
  unsigned int i;

  for (i = 0; i < len; i++) {
    if (a[i] != b[i]) {
      return a[i] < b[i] ? -1 : 1;
    }
  }
  return 0;
}

int sys_cx_math_invintm(uint8_t *r, uint32_t a, const uint8_t *m, size_t len)
{
  BIGNUM *aa, *rr, *mm;
  BN_CTX *ctx;

  ctx = BN_CTX_new();
  aa = BN_new();
  mm = BN_new();
  rr = BN_new();

  BN_lebin2bn((uint8_t *)&a, sizeof(a), aa);
  BN_bin2bn(m, len, mm);

  BN_mod_inverse(rr, aa, mm, ctx);
  BN_bn2binpad(rr, r, len);

  BN_free(rr);
  BN_free(mm);
  BN_free(aa);
  BN_CTX_free(ctx);

  return 0;
}

int sys_cx_math_invprimem(uint8_t *r, const uint8_t *a, const uint8_t *m,
                          unsigned int len)
{
  BIGNUM *aa, *rr, *mm;
  BN_CTX *ctx;

  ctx = BN_CTX_new();
  aa = BN_new();
  mm = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(m, len, mm);

  BN_mod_inverse(rr, aa, mm, ctx);
  BN_bn2binpad(rr, r, len);

  BN_free(rr);
  BN_free(mm);
  BN_free(aa);
  BN_CTX_free(ctx);

  return 0;
}

int sys_cx_math_is_zero(const uint8_t *a, unsigned int len)
{
  unsigned int i;

  for (i = 0; i < len; i++) {
    if (a[i] != 0) {
      return 0;
    }
  }
  return 1;
}

int sys_cx_math_add(uint8_t *r, const uint8_t *a, const uint8_t *b,
                    unsigned int len)
{
  BIGNUM *aa, *bb, *rr;
  int carry;

  if (len > MAX_SIZE) {
    errx(1, "sys_cx_math_add: size too large");
  }

  aa = BN_new();
  bb = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(b, len, bb);

  BN_add(rr, aa, bb);
  carry = BN_bn2binpad_truncate(rr, r, len) ? 1 : 0;

  BN_free(rr);
  BN_free(bb);
  BN_free(aa);

  return carry;
}

int sys_cx_math_addm(uint8_t *r, const uint8_t *a, const uint8_t *b,
                     const uint8_t *m, unsigned int len)
{
  BIGNUM *aa, *bb, *rr, *mm;
  BN_CTX *ctx = BN_CTX_new();
  aa = BN_new();
  bb = BN_new();
  mm = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(b, len, bb);
  BN_bin2bn(m, len, mm);

  BN_mod_add(rr, aa, bb, mm, ctx);
  BN_bn2binpad(rr, r, len);

  BN_free(rr);
  BN_free(mm);
  BN_free(bb);
  BN_free(aa);
  BN_CTX_free(ctx);

  /* XXX: this function should return void */
  return 0xdeadbeef;
}

int sys_cx_math_powm(uint8_t *r, const uint8_t *a, const uint8_t *e,
                     size_t len_e, const uint8_t *m, size_t len)
{
  BIGNUM *aa, *ee, *rr, *mm;
  BN_CTX *ctx;

  ctx = BN_CTX_new();
  aa = BN_new();
  ee = BN_new();
  mm = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(e, len_e, ee);
  BN_bin2bn(m, len, mm);

  BN_mod_exp(rr, aa, ee, mm, ctx);
  BN_bn2binpad(rr, r, len);

  BN_free(rr);
  BN_free(mm);
  BN_free(ee);
  BN_free(aa);
  BN_CTX_free(ctx);

  return 0;
}

int sys_cx_math_sub(uint8_t *r, const uint8_t *a, const uint8_t *b,
                    unsigned int len)
{
  BIGNUM *aa, *bb, *rr;

  aa = BN_new();
  bb = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(b, len, bb);

  BN_sub(rr, aa, bb);
  BN_bn2binpad(rr, r, len);

  BN_free(rr);
  BN_free(bb);
  BN_free(aa);

  return 0;
}

int sys_cx_math_subm(uint8_t *r, const uint8_t *a, const uint8_t *b,
                     const uint8_t *m, unsigned int len)
{
  BIGNUM *aa, *bb, *rr, *mm;
  BN_CTX *ctx;

  ctx = BN_CTX_new();
  aa = BN_new();
  bb = BN_new();
  mm = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(b, len, bb);
  BN_bin2bn(m, len, mm);

  BN_mod_sub(rr, aa, bb, mm, ctx);
  BN_bn2binpad(rr, r, len);

  BN_free(rr);
  BN_free(mm);
  BN_free(bb);
  BN_free(aa);
  BN_CTX_free(ctx);

  return 0;
}

int sys_cx_math_modm(uint8_t *v, unsigned int len_v, const uint8_t *m,
                     unsigned int len_m)
{
  BIGNUM *aa, *rr, *mm;
  BN_CTX *ctx = BN_CTX_new();
  aa = BN_new();
  mm = BN_new();
  rr = BN_new();

  BN_bin2bn(v, len_v, aa);
  BN_bin2bn(m, len_m, mm);
  BN_mod(rr, aa, mm, ctx);
  BN_bn2binpad(rr, v, len_v);

  BN_free(rr);
  BN_free(mm);
  BN_free(aa);
  BN_CTX_free(ctx);

  /* XXX: this function should return void */
  return 0xdeadbeef;
}

int sys_cx_math_mult(uint8_t *r, const uint8_t *a, const uint8_t *b,
                     unsigned int len)
{
  BIGNUM *aa, *bb, *rr;
  BN_CTX *ctx = BN_CTX_new();
  aa = BN_new();
  bb = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(b, len, bb);

  BN_mul(rr, aa, bb, ctx);
  BN_bn2binpad(rr, r, 2 * len);

  BN_free(rr);
  BN_free(bb);
  BN_free(aa);
  BN_CTX_free(ctx);

  /* XXX: this function should return void */
  return 0xdeadbeef;
}

int sys_cx_math_multm(uint8_t *r, const uint8_t *a, const uint8_t *b,
                      const uint8_t *m, unsigned int len)
{
  BIGNUM *aa, *bb, *rr, *mm;
  BN_CTX *ctx = BN_CTX_new();
  aa = BN_new();
  bb = BN_new();
  mm = BN_new();
  rr = BN_new();

  BN_bin2bn(a, len, aa);
  BN_bin2bn(b, len, bb);
  BN_bin2bn(m, len, mm);

  BN_mod_mul(rr, aa, bb, mm, ctx);
  BN_bn2binpad(rr, r, len);

  BN_free(rr);
  BN_free(mm);
  BN_free(bb);
  BN_free(aa);
  BN_CTX_free(ctx);

  /* XXX: this function should return void */
  return 0xdeadbeef;
}

int sys_cx_math_is_prime(const uint8_t *r, unsigned int len)
{
  BIGNUM *rr;
  BN_CTX *ctx = BN_CTX_new();
  int ret;

  rr = BN_new();
  BN_bin2bn(r, len, rr);

  ret = BN_is_prime_ex(rr, 64, ctx, NULL);

  BN_free(rr);
  BN_CTX_free(ctx);

  return ret;
}
