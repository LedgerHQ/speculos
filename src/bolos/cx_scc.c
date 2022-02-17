#include <stdbool.h>

#include "bolos/exception.h"
#include "cx.h"
#include "cx_hmac.h"

/* ======================================================================== */
/* ===  MISC  === MISC === MISC === MISC ===  MISC  === MISC ===  MISC  === */
/* ======================================================================== */

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void cx_scc_assert_param(bool cond)
{
  if (!cond) {
    THROW(INVALID_PARAMETER);
  }
}

/* ========================================================================= */
/* === CHECK === CHECK === CHECK === CHECK === CHECK === CHECK === CHECK === */
/* ========================================================================= */

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void cx_scc_struct_check_hash(const cx_hash_t *hash)
{
  unsigned int os;
#if 0
  unsigned int bs;
#endif

  switch (hash->algo) {

#if 0
  case CX_SHA224:
#endif
  case CX_SHA256:
    cx_scc_assert_param(((const cx_sha256_t *)hash)->blen < 64);
    return;

  case CX_RIPEMD160:
    cx_scc_assert_param(((const cx_ripemd160_t *)hash)->blen < 64);
    return;

#if 0
  case CX_SHA384:
#endif
  case CX_SHA512:
    cx_scc_assert_param(((const cx_sha512_t *)hash)->blen < 128);
    return;
#if 0

  case CX_KECCAK:
  case CX_SHA3:
    os = ((cx_sha3_t*)hash)->output_size;
    cx_scc_assert_param((os == 128/8) || (os == 224/8) || (os == 256/8) || (os == 384/8) || (os == 512/8));
    bs = ((cx_sha3_t*)hash)->block_size;
    cx_scc_assert_param(bs == (1600>>3)-2*os);
    cx_scc_assert_param(((cx_sha3_t*)hash)->blen < 200);
    return;

  case CX_SHA3_XOF:
    os = ((cx_sha3_t*)hash)->block_size;
    cx_scc_assert_param( (os == (1600-2*256)>>3) || (os == (1600-2*128)>>3));
    cx_scc_assert_param(((cx_sha3_t*)hash)->blen < 200);
    return;

  case CX_GROESTL: {
    os = ((cx_groestl_t*)hash)->output_size;
    cx_scc_assert_param((os == 224/8) || (os == 256/8) || (os == 384/8) ||(os == 512/8));
    struct hashState_s *ctx = &((cx_groestl_t*)hash)->ctx;
    cx_scc_assert_param((ctx->hashlen <= 512 / 8) &&
                        (ctx->buf_ptr < ctx->statesize) &&
                        (ctx->columns <= COLS1024) &&
                        (ctx->rounds <= 14/*ROUNDS1024*/) &&
                        (ctx->statesize <=SIZE1024));
     }
    return ;
#endif

  case CX_BLAKE2B: {
    os = ((const cx_blake2b_t *)hash)->output_size;
    cx_scc_assert_param((os >= 8 / 8) && (os <= 512 / 8));
    const struct blake2b_state__ *ctx = &((const cx_blake2b_t *)hash)->ctx;
    cx_scc_assert_param((ctx->buflen <= BLAKE2B_BLOCKBYTES) &&
                        (ctx->outlen <= BLAKE2B_BLOCKBYTES));
  }
    return;

  default:
    break;
  }

  THROW(INVALID_PARAMETER);
}
/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void cx_scc_struct_check_hashmac(const cx_hmac_t *hmac)
{
  cx_md_t hash_algorithm = hmac->hash_ctx.header.algo;

  if (hash_algorithm != CX_SHA256
#if 0
      && hash_algorithm != CX_SHA224
      && hash_algorithm != CX_SHA384
#endif
      && hash_algorithm != CX_SHA512 && hash_algorithm != CX_RIPEMD160) {
    THROW(INVALID_PARAMETER);
  }
  cx_scc_struct_check_hash((const cx_hash_t *)&hmac->hash_ctx);
}

#if 0
/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void  cx_scc_struct_check_deskey(const cx_des_key_t* key) {
  switch(key->size) {
  case 8:
  case 16:
    return;
  default:
    THROW(INVALID_PARAMETER);
  }
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void  cx_scc_struct_check_aeskey(const cx_aes_key_t* key) {
  switch(key->size) {
  case 16:
  case 24:
  case 32:
    return;
  default:
    THROW(INVALID_PARAMETER);
  }
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void  cx_scc_struct_check_rsa_pubkey(const cx_rsa_public_key_t *key) {
 switch(key->size) {
   case 128:
   case 256:
   case 384:
   case 512:
     return;
   default:
     THROW(INVALID_PARAMETER);
 }
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void  cx_scc_struct_check_rsa_privkey(const cx_rsa_private_key_t *key) {
 switch(key->size) {
   case 128:
   case 256:
   case 384:
   case 512:
     return;
   default:
     THROW(INVALID_PARAMETER);
 }
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void  cx_scc_struct_check_ecfp_privkey(const cx_ecfp_private_key_t *key) {
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(key->curve);
  cx_scc_assert_param(key->d_len <= domain->length);
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
void  cx_scc_struct_check_ecfp_pubkey(const cx_ecfp_public_key_t *key) {
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(key->curve);
  cx_scc_assert_param(key->W_len <= 1+2*domain->length);
}


/* ======================================================================== */
/* ===  SIZE  === SIZE ===  SIZE  === SIZE === SIZE ===  SIZE  === SIZE === */
/* ======================================================================== */
/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_derive_key_size2(unsigned int mode, cx_curve_t curve) {
  if ((curve == CX_CURVE_Ed25519) && ((mode&HDW_ED25519_SLIP10)==0)) {
    return 64;
  }
  return 32;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_derive_key_size(cx_curve_t curve) {
  return cx_scc_derive_key_size2(HDW_NORMAL, curve);
}
#endif

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_hash(const cx_hash_t *hash)
{
  switch (hash->algo) {

#if 0
  case CX_SHA224:
#endif
  case CX_SHA256:
    return sizeof(cx_sha256_t);

  case CX_RIPEMD160:
    return sizeof(cx_ripemd160_t);

#if 0
  case CX_SHA384:
  case CX_SHA512:
    return sizeof(cx_sha512_t);

  case CX_KECCAK:
  case CX_SHA3:
  case CX_SHA3_XOF:
    return sizeof(cx_sha3_t);

  case CX_GROESTL:
    return sizeof(cx_groestl_t);
#endif

  case CX_BLAKE2B:
    return sizeof(cx_blake2b_t);

  default:
    break;
  }

  THROW(INVALID_PARAMETER);
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_hmac(const cx_hmac_t *hmac)
{
  switch (hmac->hash_ctx.header.algo) {
  case CX_SHA256:
#if 0
  case CX_SHA224:
  case CX_SHA384:
  case CX_SHA512:
#endif
  case CX_RIPEMD160:
    return sizeof(cx_hmac_ctx);
  default:
    THROW(INVALID_PARAMETER);
  }
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
#if 0
int cx_scc_struct_size_rsa_pubkey(const cx_rsa_public_key_t *key) {
  switch(key->size) {
  case 128:
    return sizeof(cx_rsa_1024_public_key_t);
  case 256:
    return sizeof(cx_rsa_2048_public_key_t);
  case 384:
    return sizeof(cx_rsa_3072_public_key_t);
  case 512:
    return sizeof(cx_rsa_4096_public_key_t);
  default:
    THROW(INVALID_PARAMETER);
  }
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_rsa_privkey(const cx_rsa_private_key_t *key) {
  switch(key->size) {
    case 128:
      return sizeof(cx_rsa_1024_private_key_t);
    case 256:
      return sizeof(cx_rsa_2048_private_key_t);
    case 384:
      return sizeof(cx_rsa_3072_private_key_t);
    case 512:
      return sizeof(cx_rsa_4096_private_key_t);
    default:
      THROW(INVALID_PARAMETER);
  }
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_ecfp_pubkey(const cx_ecfp_public_key_t *key) {
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(key->curve);
  unsigned int size = domain->length;
  return sizeof(struct cx_ecfp_public_key_s)+(1+(2*size));
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_ecfp_privkey(const cx_ecfp_private_key_t *key) {
  const cx_curve_domain_t * domain = cx_ecfp_get_domain(key->curve);
  unsigned int size = domain->length;
  if (key->d_len > size) {
    size = key->d_len;
  }
  return sizeof(struct cx_ecfp_private_key_s)+size;
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_ecfp_pubkey_from_curve(cx_curve_t curve) {
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(curve);
  unsigned int size = domain->length;
  return sizeof(struct cx_ecfp_public_key_s)+(1+(2*size));
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_ecfp_pubkey_from_pvkey(const cx_ecfp_private_key_t *key) {
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(key->curve);
  unsigned int size = domain->length;
  return sizeof(struct cx_ecfp_public_key_s)+(1+(2*size));
}

/* ----------------------------------------------------------------------- */
/*                                                                         */
/* ----------------------------------------------------------------------- */
int cx_scc_struct_size_ecfp_privkey_from_curve(cx_curve_t curve) {
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(curve);
  unsigned int size = domain->length;
  return sizeof(struct cx_ecfp_private_key_s)+size;
}
#endif
