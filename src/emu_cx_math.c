#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <openssl/bn.h>

/*
cx_math_invintm
cx_math_invprimem
cx_math_modm
cx_math_next_prime
cx_math_mult
cx_math_powm
cx_math_sub
cx_math_subm
*/

int sys_cx_math_cmp(const uint8_t *a, const uint8_t *b, unsigned int len)
{
  unsigned int i;

  for (i=0; i<len; i++) {
    if (a[i] != b[i]) {
      return a[i]<b[i]?-1:1;
    }
  }
  return 0;
}

int sys_cx_math_is_zero(const uint8_t *a, unsigned int len) {
  unsigned int i;

  for (i=0; i<len; i++) {
    if (a[i] != 0) {
      return 0;
    }
  }
  return 1;
}

int sys_cx_math_addm(uint8_t *r, const uint8_t *a, const uint8_t *b, const uint8_t *m, unsigned int len)
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

  BN_free(mm);
  BN_free(bb);
  BN_free(aa);
  BN_CTX_free(ctx);

  /* XXX: this function should return void */
  return 0xdeadbeef;
}

int sys_cx_math_modm(uint8_t *v, unsigned int len_v, const uint8_t *m, unsigned int len_m)
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

  BN_free(mm);
  BN_free(aa);
  BN_CTX_free(ctx);

  /* XXX: this function should return void */
  return 0xdeadbeef;
}

int sys_cx_math_multm(uint8_t *r,
                      const uint8_t *a,
                      const uint8_t *b,
                      const uint8_t *m,
                      unsigned int len)
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

  BN_free(mm);
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

  BN_CTX_free(ctx);

  return ret;
}
