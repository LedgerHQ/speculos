#include <err.h>

#include <openssl/bn.h>
#include <string.h>

#include "cx_utils.h"
/* ======================================================================= */
/*                          32 BITS manipulation                           */
/* ======================================================================= */
#ifndef CX_INLINE_U32
// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
unsigned long int cx_rotl(unsigned long int x, unsigned char n)
{
  return (((x) << (n)) | ((x) >> (32 - (n))));
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
unsigned long int cx_rotr(unsigned long int x, unsigned char n)
{
  return (((x) >> (n)) | ((x) << ((32) - (n))));
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
unsigned long int cx_shr(unsigned long int x, unsigned char n)
{
  return ((x) >> (n));
}
#endif

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
uint32_t spec_cx_swap_uint32(uint32_t v)
{
  return (((v) << 24) & 0xFF000000U) | (((v) << 8) & 0x00FF0000U) |
         (((v) >> 8) & 0x0000FF00U) | (((v) >> 24) & 0x000000FFU);
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
void spec_cx_swap_buffer32(uint32_t *v, size_t len)
{
  while (len--) {
#ifdef ST31
    unsigned int tmp;
    tmp = ((unsigned char *)v)[len * 4 + 3];
    ((unsigned char *)v)[len * 4 + 3] = ((unsigned char *)v)[len * 4];
    ((unsigned char *)v)[len * 4] = tmp;
    tmp = ((unsigned char *)v)[len * 4 + 2];
    ((unsigned char *)v)[len * 4 + 2] = ((unsigned char *)v)[len * 4 + 1];
    ((unsigned char *)v)[len * 4 + 1] = tmp;
#else
    v[len] = spec_cx_swap_uint32(v[len]);
#endif
  }
}

/* ======================================================================= */
/*                          64 BITS manipulation                           */
/* ======================================================================= */
#ifndef NATIVE_64BITS // NO 64BITS
// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
void cx_rotr64(uint64bits_t *x, unsigned int n)
{
  unsigned long int sl_rot, sh_rot;
  if (n >= 32) {
    sl_rot = x->l;
    x->l = x->h;
    x->h = sl_rot;
    n -= 32;
  }
  sh_rot = ((((x->h) & 0xFFFFFFFF) << (32 - n)));
  sl_rot = ((((x->l) & 0xFFFFFFFF) << (32 - n)));
  // rotate
  x->h = ((x->h >> n) | sl_rot);
  x->l = ((x->l >> n) | sh_rot);
}
// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
void cx_shr64(uint64bits_t *x, unsigned char n)
{
  unsigned long int sl_shr;

  if (n >= 32) {
    x->l = (x->h);
    x->h = 0;
    n -= 32;
  }

  sl_shr = ((((x->h) & 0xFFFFFFFF) << (32 - n)));
  x->l = ((x->l) >> n) | sl_shr;
  x->h = ((x->h) >> n);
}

#else

#ifndef CX_INLINE_U64
uint64bits_t cx_rotr64(uint64bits_t x, unsigned int n)
{
  return (((x) >> (n)) | ((x) << ((64) - (n))));
}
uint64bits_t cx_rotl64(uint64bits_t x, unsigned int n)
{
  return (((x) << (n)) | ((x) >> ((64) - (n))));
}
uint64bits_t cx_shr64(uint64bits_t x, unsigned int n)
{
  return ((x) >> (n));
}
#endif

#endif

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------

#ifndef NATIVE_64BITS
void spec_cx_swap_uint64(uint64bits_t *v)
{
  unsigned long int h, l;
  h = v->h;
  l = v->l;
  l = spec_cx_swap_uint32(l);
  h = spec_cx_swap_uint32(h);
  v->h = l;
  v->l = h;
}
#else
uint64bits_t spec_cx_swap_uint64(uint64bits_t v)
{
  uint32_t h, l;
  h = (uint32_t)((v >> 32) & 0xFFFFFFFF);
  l = (uint32_t)(v & 0xFFFFFFFF);
  l = spec_cx_swap_uint32(l);
  h = spec_cx_swap_uint32(h);
  return (((uint64bits_t)l) << 32) | ((uint64bits_t)h);
}
#endif

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
void spec_cx_swap_buffer64(uint64bits_t *v, int len)
{
#ifndef NATIVE_64BITS
  while (len--) {
    spec_cx_swap_uint64(&v[len]);
  }
#else
  uint64bits_t i;
  while (len--) {
    i = *v;
    *v++ = spec_cx_swap_uint64(i);
  }
#endif
}

// --------------------------------------------------------------------------
// -
// --------------------------------------------------------------------------
#ifndef NATIVE_64BITS
void cx_add_64(uint64bits_t *x, uint64bits_t *y)
{
  unsigned int carry;
  unsigned long int addl;

  addl = x->l + y->l;
  addl = ~addl;
  carry =
      ((((x->l & y->l) | (x->l & ~y->l & addl) | (~x->l & y->l & addl)) >> 31) &
       1);

  x->l = x->l + y->l;
  x->h = x->h + y->h;
  if (carry) {
    x->h++;
  }
}
#endif

void be2le(uint8_t *v, size_t len)
{
  uint8_t t;
  int i, j;

  j = len - 1;
  len /= 2;

  for (i = 0; len > 0; i++, j--, len--) {
    t = v[i];
    v[i] = v[j];
    v[j] = t;
    i++;
    j--;
  }
}

void le2be(uint8_t *v, size_t len)
{
  return be2le(v, len);
}

int sys_cx_math_next_prime(uint8_t *buf, unsigned int len)
{
  BN_CTX *ctx;
  BIGNUM *p;
  int ret;

  ctx = BN_CTX_new();
  p = BN_new();

  BN_bin2bn(buf, len, p);

  if (BN_mod_word(p, 2) == 0) {
    if (!BN_sub_word(p, 1)) {
      errx(1, "BN_sub_word");
    }
  }

  do {
    if (!BN_add_word(p, 2)) {
      errx(1, "BN_add_word");
    }

    ret = BN_is_prime_ex(p, 20, ctx, NULL);
  } while (ret == 0);

  if (ret == -1) {
    errx(1, "isprime");
  }

  BN_CTX_free(ctx);

  return 0;
}

// Parsing bip32 and eip2333 derivations path
int get_path(const char *str_, unsigned int *path, int max_path_len)
{
  char *token, *str;
  int path_len;
  size_t len;

  str = strdup(str_);
  if (str == NULL) {
    warn("strdup");
    return -1;
  }

  path_len = 0;
  while (1) {
    token = strtok((path_len == 0) ? str : NULL, "/");
    if (token == NULL) {
      break;
    }

    if (path_len >= max_path_len) {
      return -1;
    }

    len = strlen(token);
    if (len == 0) {
      return -1;
    }

    if (token[len - 1] == '\'') {
      token[len - 1] = '\x00';
      path[path_len] = 0x80000000;
    } else {
      path[path_len] = 0;
    }

    path[path_len] |= strtoul(token, NULL, 10);

    path_len += 1;
  }

  free(str);

  return path_len;
}
