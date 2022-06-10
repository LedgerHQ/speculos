#include <err.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/sha.h>

#include "bolos/exception.h"
#include "cx.h"
#include "cx_curve25519.h"
#include "cx_ec.h"
#include "cx_ed25519.h"
#include "cx_hash.h"
#include "cx_rng_rfc6979.h"
#include "cx_utils.h"
#include "emulate.h"

#define CX_CURVE_RANGE(i, dom)                                                 \
  (((i) > (CX_CURVE_##dom##_START)) && ((i) < (CX_CURVE_##dom##_END)))


#include "cx_const_brainpool.c"
#include "cx_const_secp.c"
#include "cx_ecbls12381.c"
#include "cx_ecedd25519.c"
#include "cx_const_sec256k1.c"
#include "cx_const_stark.c"

static cx_curve_domain_t const *const C_cx_allCurves[] = {
  (const cx_curve_domain_t *)&C_cx_secp256k1,
  (const cx_curve_domain_t *)&C_cx_secp256r1,
  (const cx_curve_domain_t *)&C_cx_secp384r1,
  (const cx_curve_domain_t *)&C_cx_Ed25519,
  (const cx_curve_domain_t *)&C_cx_BLS12_381_G1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP256r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP256t1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP320r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP320t1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP384r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP384t1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP512r1,
  (const cx_curve_domain_t *)&C_cx_BrainpoolP512t1,
  (const cx_curve_domain_t *)&C_cx_Stark256
};

static cx_curve_weierstrass_t const *const C_cx_all_Weierstrass_Curves[] = {
  (const cx_curve_weierstrass_t *) &C_cx_secp256k1,
  (const cx_curve_weierstrass_t *)&C_cx_secp256r1,
  (const cx_curve_weierstrass_t *)&C_cx_secp384r1,
  (const cx_curve_weierstrass_t *) &C_cx_Ed25519,
  (const cx_curve_weierstrass_t *) &C_cx_BLS12_381_G1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP256r1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP256t1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP320r1,
  (const cx_curve_weierstrass_t *)& C_cx_BrainpoolP320t1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP384r1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP384t1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP512r1,
  (const cx_curve_weierstrass_t *)&C_cx_BrainpoolP512t1,
  (const cx_curve_weierstrass_t *)&C_cx_Stark256
};

static int nid_from_curve(cx_curve_t curve)
{
  int nid;

  switch (curve) {
  case CX_CURVE_SECP256K1:
    nid = NID_secp256k1;
    break;
  case CX_CURVE_SECP256R1:
    nid = NID_X9_62_prime256v1;
    break;
  case CX_CURVE_SECP384R1:
    nid = NID_secp384r1;
    break;
  case CX_CURVE_BrainPoolP256R1:
    nid = NID_brainpoolP256r1;
    break;
  case CX_CURVE_BrainPoolP256T1:
    nid = NID_brainpoolP256t1;
    break;
  case CX_CURVE_BrainPoolP320R1:
    nid = NID_brainpoolP320r1;
    break;
  case CX_CURVE_BrainPoolP320T1:
    nid = NID_brainpoolP320t1;
    break;
  case CX_CURVE_BrainPoolP384R1:
    nid = NID_brainpoolP384r1;
    break;
  case CX_CURVE_BrainPoolP384T1:
    nid = NID_brainpoolP384t1;
    break;
  case CX_CURVE_BrainPoolP512R1:
    nid = NID_brainpoolP512r1;
    break;
  case CX_CURVE_BrainPoolP512T1:
    nid = NID_brainpoolP512t1;
    break;
  case CX_CURVE_Stark256:
	nid = NID_Stark256;
	break;
#if 0
  case CX_CURVE_SECP521R1:
    nid = NID_secp521r1;
    break;
#endif
  default:
    nid = -1;
    errx(1, "cx_ecfp_generate_pair unsupported curve");
    break;
  }
  return nid;
}

/* Unexported functions from OpenSSL, in ec/curve25519.c. Dirty hack... */
int ED25519_sign(uint8_t *out_sig, const uint8_t *message, size_t message_len,
                 const uint8_t public_key[32], const uint8_t private_key[32]);
int ED25519_verify(const uint8_t *message, size_t message_len,
                   const uint8_t signature[64], const uint8_t public_key[32]);
void ED25519_public_from_private(uint8_t out_public_key[32],
                                 const uint8_t private_key[32]);

int sys_cx_eddsa_get_public_key(const cx_ecfp_private_key_t *pv_key,
                                cx_md_t hashID, cx_ecfp_public_key_t *pu_key)
{
  uint8_t digest[SHA512_DIGEST_LENGTH];

  if (hashID != CX_SHA512) {
    errx(1, "cx_eddsa_get_public_key: invalid hashId (0x%x)", hashID);
    return -1;
  }

  if (pv_key->d_len != 32) {
    errx(1, "cx_eddsa_get_public_key: invalid key size (0x%zx)", pv_key->d_len);
    return -1;
  }

  SHA512(pv_key->d, pv_key->d_len, digest);

  digest[0] &= 0xf8;
  digest[31] &= 0x7f;
  digest[31] |= 0x40;

  le2be(digest, 32);

  pu_key->curve = CX_CURVE_Ed25519;
  pu_key->W_len = 1 + 2 * 32;
  pu_key->W[0] = 0x04;
  memcpy(pu_key->W + 1, C_cx_Ed25519_Bx, sizeof(C_cx_Ed25519_Bx));
  memcpy(pu_key->W + 1 + 32, C_cx_Ed25519_By, sizeof(C_cx_Ed25519_By));

  return sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, pu_key->W, pu_key->W_len,
                                 digest, 32);
}

int sys_cx_eddsa_sign(const cx_ecfp_private_key_t *pvkey,
                      int mode __attribute__((unused)), cx_md_t hashID,
                      const unsigned char *hash, unsigned int hash_len,
                      const unsigned char *ctx __attribute__((unused)),
                      unsigned int ctx_len __attribute__((unused)),
                      unsigned char *sig, unsigned int sig_len,
                      unsigned int *info __attribute__((unused)))
{
  uint8_t public_key[32];

  /* Only SHA-512 is supported in Speculos, as it is the only supported hash
   * in OpenSSL */
  if (hashID != CX_SHA512) {
    return -1;
  }
  if (sig_len < 64) {
    return -1;
  }
  /* Key must be a Ed25519 private key */
  if (pvkey->curve != CX_CURVE_Ed25519 || pvkey->d_len != 32) {
    return -1;
  }
  ED25519_public_from_private(public_key, pvkey->d);
  if (ED25519_sign(sig, hash, hash_len, public_key, pvkey->d) == 1) {
    return 64;
  }
  return -1;
}

int sys_cx_eddsa_verify(const cx_ecfp_public_key_t *pu_key,
                        int mode __attribute__((unused)), cx_md_t hashID,
                        const unsigned char *hash, unsigned int hash_len,
                        const unsigned char *ctx __attribute__((unused)),
                        unsigned int ctx_len __attribute__((unused)),
                        const unsigned char *sig, unsigned int sig_len)
{
  cx_ecfp_public_key_t pub;

  /* Only SHA-512 is supported in Speculos, as it is the only supported hash
   * in OpenSSL */
  if (hashID != CX_SHA512) {
    return 0;
  }
  if (sig_len != 64) {
    return 0;
  }
  if (pu_key->curve != CX_CURVE_Ed25519 || pu_key->W_len != 1 + 2 * 32) {
    return 0;
  }

  /* pass a compressed key to ED25519_verify */
  memcpy(&pub, pu_key, sizeof(pub));
  if (pub.W[0] == 0x04) {
    if (sys_cx_edward_compress_point(pub.curve, pub.W, pub.W_len) != 0) {
      return 0;
    }
  }

  return ED25519_verify(hash, hash_len, sig, pub.W + 1);
}


const cx_curve_weierstrass_t *cx_ecfp_get_weierstrass(cx_curve_t curve)
{
  unsigned int i;
  for (i = 0; i < sizeof(C_cx_allCurves) / sizeof(cx_curve_domain_t *); i++) {
    if (C_cx_all_Weierstrass_Curves[i]->curve == curve) {
      return C_cx_all_Weierstrass_Curves[i];
    }
  }
  THROW(INVALID_PARAMETER);
  return NULL;
}

/* get generic curve group from parameters*/
/* curve must be stored in weierstrass form in C_cx_all_Weierstrass_Curves*/
 int cx_generic_curve(const cx_curve_weierstrass_t *i_weier, BN_CTX *ctx, EC_GROUP **o_group)
{
    BIGNUM *p,*a,*b, *x,  *y, *order;
    EC_POINT *P = NULL;

    /* Allocate a BN for each parameter*/
	p = BN_new();	a = BN_new();	b = BN_new();	x = BN_new();	y = BN_new();	order = BN_new();

	BN_bin2bn(i_weier->p, i_weier->length, p);
	BN_bin2bn(i_weier->a, i_weier->length, a);
	BN_bin2bn(i_weier->b, i_weier->length, b);
	BN_bin2bn(i_weier->Gx, i_weier->length, x);
	BN_bin2bn(i_weier->Gy, i_weier->length, y);
	BN_bin2bn(i_weier->n, i_weier->length, order);

	*o_group = EC_GROUP_new_curve_GFp(p, a,b, ctx);

	P = EC_POINT_new(*o_group);
    	// if(!EC_POINT_set_compressed_coordinates_GFp(Stark, P, x, 1, ctx))

    	if(!EC_POINT_set_affine_coordinates_GFp(*o_group, P,x,y, ctx)){
    		errx(1, "Set affine coordinates failed");
    		return -1;}
     	 if(!EC_POINT_is_on_curve(*o_group, P, ctx)){
     		errx(1, "generator not on curve: check curve parameters p,a,b,x,y,q");
     		return -1;}
    	if(!EC_GROUP_set_generator(*o_group, P, order, BN_value_one()) ){
    		errx(1,"\n generator set failed); //ici pour openSSL OK=1 !!!!");
    		return -1;}

    	 if(!EC_GROUP_check(*o_group,ctx))
    	 {
    	    		 errx(1, "EC_GROUP_check() failed");
    	    	     return -1;
    	  }

	/* free BN parameters*/
    BN_free(p);BN_free(a);BN_free(b);BN_free(x);BN_free(y);BN_free(order);
    /* free generator*/
    EC_POINT_free(P);

    return 0;
}

 int sys_cx_ecfp_generate_pair2(cx_curve_t curve,
                                cx_ecfp_public_key_t *public_key,
                                cx_ecfp_private_key_t *private_key,
                                int keep_private, cx_md_t hashID)
 {
   EC_GROUP *Stark=NULL;

   EC_POINT *pub;
   BN_CTX *ctx;
   EC_KEY *key;
   BIGNUM *bn;
   int nid, ret;
   ctx = BN_CTX_new();

   const cx_curve_weierstrass_t *weier=cx_ecfp_get_weierstrass(curve);

   if (curve == CX_CURVE_Ed25519) {
     return sys_cx_eddsa_get_public_key(private_key, hashID, public_key);
   } else {

     nid = nid_from_curve(curve);
     if(nid==-1)
     {
     	errx(1, "nid not supported, refer to manual 'how to add a curve to speculos.txt' ");
     	return -1;
     }

     	key = EC_KEY_new();

     	if(cx_generic_curve(weier, ctx, &Stark)!=0)
     		return -1;

     	if(Stark==NULL)
     		  errx(1, "setting ecc group failed");

     	ret=EC_KEY_set_group(key, Stark);//here for openSSL OK=1 !!!!
     	  if (ret !=1) {
     	      errx(1, "ssl: EC_KEY_set_group");
     	    }

     	pub = EC_POINT_new(Stark);
     	if(pub==NULL) return -1;


     if (!keep_private) {
       if (EC_KEY_generate_key(key) == 0) {
         errx(1, "ssl: EC_KEY_generate_key");
       }

       const BIGNUM *priv = EC_KEY_get0_private_key(key);
       if (BN_num_bytes(priv) > (int)weier->length) {
         errx(1, "ssl: invalid bn");
       }
       private_key->curve = curve;
       private_key->d_len = BN_bn2bin(priv, private_key->d);
     } else {
       BIGNUM *priv;

       priv = BN_new();


       if(BN_bin2bn(private_key->d, private_key->d_len, priv)==NULL) {
     	  errx(1, "ssl : BN_bin2bn of private key");
       }

       if (EC_KEY_set_private_key(key, priv) == 0) {
         errx(1, "ssl : EC_KEY_set_private_key");
       }


       if (EC_POINT_mul(Stark, pub, priv, NULL, NULL, ctx) == 0) {
         errx(1, "ssl: EC_POINT_mul");
       }

       if (EC_KEY_set_public_key(key, pub) == 0) {
         errx(1, "ssl: EC_KEY_set_public_key");
       }

       BN_free(priv);
       EC_POINT_free(pub);
     }

     bn = BN_new();
     EC_POINT_point2bn(Stark, EC_KEY_get0_public_key(key),
                       POINT_CONVERSION_UNCOMPRESSED, bn, ctx);

     if (BN_num_bytes(bn) > 2 * (int)weier->length + 1) {
       errx(1, "ssl: invalid bn");
     }
     public_key->curve = curve;
     public_key->W_len = BN_bn2bin(bn, public_key->W);
     EC_POINT_point2oct(Stark, EC_KEY_get0_public_key(key),
                       POINT_CONVERSION_UNCOMPRESSED, public_key->W, public_key->W_len, ctx);

     BN_free(bn);
     EC_KEY_free(key);
     BN_CTX_free(ctx);

   return 0;
 }
 }

int sys_cx_ecfp_generate_pair(cx_curve_t curve,
                              cx_ecfp_public_key_t *public_key,
                              cx_ecfp_private_key_t *private_key,
                              int keep_private)
{
  return sys_cx_ecfp_generate_pair2(curve, public_key, private_key,
                                    keep_private, CX_SHA512);
}

int sys_cx_ecfp_init_private_key(cx_curve_t curve, const uint8_t *raw_key,
                                 unsigned int key_len,
                                 cx_ecfp_private_key_t *key)
{
  /* TODO: missing checks */

  if (raw_key != NULL) {
    key->d_len = key_len;
    memmove(key->d, raw_key, key_len);
  } else {
    key_len = 0;
  }

  key->curve = curve;

  return key_len;
}

static int asn1_read_len(const uint8_t **p, const uint8_t *end, size_t *len)
{
  /* Adapted from secp256k1 */
  int lenleft;
  unsigned int b1;
  *len = 0;

  if (*p >= end) {
    return 0;
  }

  b1 = *((*p)++);
  if (b1 == 0xff) {
    /* X.690-0207 8.1.3.5.c the value 0xFF shall not be used. */
    return 0;
  }
  if ((b1 & 0x80u) == 0) {
    /* X.690-0207 8.1.3.4 short form length octets */
    *len = b1;
    return 1;
  }
  if (b1 == 0x80) {
    /* Indefinite length is not allowed in DER. */
    return 0;
  }
  /* X.690-207 8.1.3.5 long form length octets */
  lenleft = b1 & 0x7Fu;
  if (lenleft > end - *p) {
    return 0;
  }
  if (**p == 0) {
    /* Not the shortest possible length encoding. */
    return 0;
  }
  if ((size_t)lenleft > sizeof(size_t)) {
    /* The resulting length would exceed the range of a size_t, so
     * certainly longer than the passed array size.
     */
    return 0;
  }
  while (lenleft > 0) {
    if ((*len >> ((sizeof(size_t) - 1) * 8)) != 0) {
      /* (*len << 8) overflows the capacity of size_t */
      return 0;
    }
    *len = (*len << 8u) | **p;
    if (*len + lenleft > (size_t)(end - *p)) {
      /* Result exceeds the length of the passed array. */
      return 0;
    }
    (*p)++;
    lenleft--;
  }
  if (*len < 128) {
    /* Not the shortest possible length encoding. */
    return 0;
  }
  return 1;
}

static int asn1_read_tag(const uint8_t **p, const uint8_t *end, size_t *len,
                         int tag)
{
  if ((end - *p) < 1) {
    return 0;
  }

  if (**p != tag) {
    return 0;
  }

  (*p)++;
  return asn1_read_len(p, end, len);
}

static int asn1_parse_integer(const uint8_t **p, const uint8_t *end,
                              const uint8_t **n, size_t *n_len)
{
  size_t len;
  int ret = 0;

  if (!asn1_read_tag(p, end, &len, 0x02)) { /* INTEGER */
    goto end;
  }

  if (((*p)[0] & 0x80u) == 0x80u) {
    /* Truncated, missing leading 0 (negative number) */
    goto end;
  }

  if ((*p)[0] == 0 && len >= 2 && ((*p)[1] & 0x80u) == 0) {
    /* Zeroes have been prepended to the integer */
    goto end;
  }

  while (**p == 0 && *p != end && len > 0) { /* Skip leading null bytes */
    (*p)++;
    len--;
  }

  *n = *p;
  *n_len = len;

  *p += len;
  ret = 1;

end:
  return ret;
}

int spec_cx_ecfp_decode_sig_der(const uint8_t *input, size_t input_len,
                                size_t max_size, const uint8_t **r,
                                size_t *r_len, const uint8_t **s, size_t *s_len)
{
  size_t len;
  int ret = 0;
  const uint8_t *input_end = input + input_len;

  const uint8_t *p = input;

  if (!asn1_read_tag(&p, input_end, &len, 0x30)) { /* SEQUENCE */
    goto end;
  }

  if (p + len != input_end) {
    goto end;
  }

  if (!asn1_parse_integer(&p, input_end, r, r_len) ||
      !asn1_parse_integer(&p, input_end, s, s_len)) {
    goto end;
  }

  if (p != input_end) { /* Check if bytes have been appended to the sequence */
    goto end;
  }

  if (*r_len > max_size || *s_len > max_size) {
    return 0;
  }
  ret = 1;
end:
  return ret;
}

const cx_curve_domain_t *cx_ecfp_get_domain(cx_curve_t curve)
{
  unsigned int i;
  for (i = 0; i < sizeof(C_cx_allCurves) / sizeof(cx_curve_domain_t *); i++) {
    if (C_cx_allCurves[i]->curve == curve) {
      return C_cx_allCurves[i];
    }
  }
  THROW(INVALID_PARAMETER);
}


int sys_cx_ecdsa_verify(const cx_ecfp_public_key_t *key, int UNUSED(mode),
                        cx_md_t UNUSED(hashID), const uint8_t *hash,
                        unsigned int hash_len, const uint8_t *sig,
                        unsigned int sig_len)
{
  const cx_curve_weierstrass_t *domain;
  unsigned int size;
  const uint8_t *r, *s;
  size_t rlen, slen;
  //int nid = 0;
  BN_CTX *ctx;
  ctx = BN_CTX_new();
  EC_GROUP *Stark=NULL;

  domain = (const cx_curve_weierstrass_t *)cx_ecfp_get_domain(key->curve);
  size = domain->length; // bits  -> bytes

  if (!spec_cx_ecfp_decode_sig_der(sig, sig_len, size, &r, &rlen, &s, &slen)) {
    return 0;
  }

  // Load ECDSA signature
  BIGNUM *sig_r = BN_new();
  BIGNUM *sig_s = BN_new();
  BN_bin2bn(r, rlen, sig_r);
  BN_bin2bn(s, slen, sig_s);
  ECDSA_SIG *ecdsa_sig = ECDSA_SIG_new();
  ECDSA_SIG_set0(ecdsa_sig, sig_r, sig_s);

  // Set public key
  BIGNUM *x = BN_new();
  BIGNUM *y = BN_new();
  BN_bin2bn(key->W + 1, domain->length, x);
  BN_bin2bn(key->W + domain->length + 1, domain->length, y);

	if(cx_generic_curve(domain, ctx, &Stark)!=0)
		return -1;

  //nid = nid_from_curve(key->curve);
  EC_KEY *ec_key=EC_KEY_new();
  EC_KEY_set_group(ec_key, Stark);

//  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_public_key_affine_coordinates(ec_key, x, y);

  int ret = ECDSA_do_verify(hash, hash_len, ecdsa_sig, ec_key);
  if (ret != 1) {
    ret = 0;
  }

  ECDSA_SIG_free(ecdsa_sig);

  BN_free(y);
  BN_free(x);
  EC_KEY_free(ec_key);
  return ret;
}

int spec_cx_ecfp_encode_sig_der(unsigned char *sig, unsigned int sig_len,
                                unsigned char *r, unsigned int r_len,
                                unsigned char *s, unsigned int s_len)
{
  unsigned int offset;

  while ((*r == 0) && r_len) {
    r++;
    r_len--;
  }
  while ((*s == 0) && s_len) {
    s++;
    s_len--;
  }
  if (!r_len || !s_len) {
    return 0;
  }

  // check sig_len
  offset = 3 * 2 + r_len + s_len;
  if (*r & 0x80) {
    offset++;
  }
  if (*s & 0x80) {
    offset++;
  }
  if (sig_len < offset) {
    return 0;
  }

  // r
  offset = 2;
  if (*r & 0x80) {
    sig[offset + 2] = 0;
    memmove(sig + offset + 3, r, r_len);
    r_len++;
  } else {
    memmove(sig + offset + 2, r, r_len);
  }
  sig[offset] = 0x02;
  sig[offset + 1] = r_len;

  // s
  offset += 2 + r_len;
  if (*s & 0x80) {
    sig[offset + 2] = 0;
    memmove(sig + offset + 3, s, s_len);
    s_len++;
  } else {
    memmove(sig + offset + 2, s, s_len);
  }
  sig[offset] = 0x02;
  sig[offset + 1] = s_len;

  // head
  sig[0] = 0x30;
  sig[1] = 2 + r_len + 2 + s_len;

  return 2 + sig[1];
}

int sys_cx_ecdsa_sign(const cx_ecfp_private_key_t *key, int mode,
                      cx_md_t hashID, const uint8_t *hash,
                      unsigned int hash_len, uint8_t *sig, unsigned int sig_len,
                      unsigned int *info)
{
  int nid = 0;
  uint8_t *buf_r, *buf_s;
  const BIGNUM *r, *s;
  const cx_hash_info_t *hash_info;
  bool do_rfc6979_signature;
  BN_CTX *ctx = BN_CTX_new();

  const cx_curve_weierstrass_t *domain =(const cx_curve_weierstrass_t *) cx_ecfp_get_domain(key->curve);
  nid = nid_from_curve(key->curve);
  if (nid < 0) {
    return 0;
  }

  switch (mode & CX_MASK_RND) {
  case CX_RND_TRNG:
    // Use deterministic signatures with "TRNG" mode in Speculos, in order to
    // avoid signing objects with a weak PRNG, only if the hash is compatible
    // with cx_hmac_init (hash_info->output_size is defined).
    // Otherwise, fall back to OpenSSL signature.
    hash_info = spec_cx_hash_get_info(hashID);
    if (hash_info == NULL || hash_info->output_size == 0) {
      do_rfc6979_signature = false;
      break;
    }
    __attribute__((fallthrough));
  case CX_RND_RFC6979:
    do_rfc6979_signature = true;
    break;
  default:
    THROW(INVALID_PARAMETER);
  }

  cx_rnd_rfc6979_ctx_t rfc_ctx = {};
  uint8_t rnd[CX_RFC6979_MAX_RLEN];
  BIGNUM *q = BN_new();
  BIGNUM *x = BN_new();
  BIGNUM *k = BN_new();
  BIGNUM *kinv = BN_new();
  BIGNUM *rp = BN_new();
  BIGNUM *kg_y = BN_new();

  BN_bin2bn(domain->n, domain->length, q);
  BN_bin2bn(key->d, key->d_len, x);

  EC_GROUP *Stark=NULL;

	if(cx_generic_curve(domain, ctx, &Stark)!=0)
		return -1;

//nid = nid_from_curve(key->curve);
	EC_KEY *ec_key=EC_KEY_new();
	EC_KEY_set_group(ec_key, Stark);

//  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
//EC_KEY_set_public_key_affine_coordinates(ec_key, x, y);


//  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_private_key(ec_key, x);

  const EC_GROUP *group = EC_KEY_get0_group(ec_key);
  EC_POINT *kg = EC_POINT_new(group);
  ECDSA_SIG *ecdsa_sig;

  if (do_rfc6979_signature) {
    if (domain->length >= CX_RFC6979_MAX_RLEN) {
      errx(1, "rfc6979_ecdsa_sign: curve too large");
    }
    spec_cx_rng_rfc6979_init(&rfc_ctx, hashID, key->d, key->d_len, hash, hash_len,
                        domain->n, domain->length);
  }
  do {
    if (do_rfc6979_signature) {
      spec_cx_rng_rfc6979_next(&rfc_ctx, rnd, domain->length);
      BN_bin2bn(rnd, domain->length, k);
    } else {
      BN_rand(k, domain->length, 0, 0);
    }
    if (!EC_POINT_mul(group, kg, k, NULL, NULL, ctx)) {
      errx(1, "ssl: EC_POINT_mul");
    }
    if (!EC_POINT_get_affine_coordinates(group, kg, rp, kg_y, ctx)) {
      errx(1, "ssl: EC_POINT_get_affine_coordinates");
    }
    if (BN_cmp(rp, q) >= 0) {
      if (!BN_nnmod(rp, rp, q, ctx)) {
        errx(1, "ssl: BN_nnmod");
      }
      if (info != NULL) {
        *info |= CX_ECCINFO_xGTn;
      }
    }
    if (BN_is_zero(rp)) {
      continue;
    }
    if (info != NULL && BN_is_odd(kg_y)) {
      *info |= CX_ECCINFO_PARITY_ODD;
    }

    BN_mod_inverse(kinv, k, q, ctx);
    ecdsa_sig = ECDSA_do_sign_ex(hash, hash_len, kinv, rp, ec_key);
  } while (ecdsa_sig == NULL);
  BN_CTX_free(ctx);
  BN_free(q);
  BN_free(x);
  BN_free(k);
  BN_free(kinv);
  BN_free(rp);
  BN_free(kg_y);
  EC_POINT_free(kg);

  // normalize signature (s < n/2) if needed
  BIGNUM *halfn = BN_new();
  const BIGNUM *n = EC_GROUP_get0_order(group);
  BN_rshift1(halfn, n);

  BIGNUM *normalized_s = BN_new();
  ECDSA_SIG_get0(ecdsa_sig, &r, &s);
  if ((mode & CX_NO_CANONICAL) == 0 && BN_cmp(s, halfn) > 0) {
    fprintf(stderr, "cx_ecdsa_sign: normalizing s > n/2\n");
    BN_sub(normalized_s, n, s);

    if (info != NULL) {
      *info ^= CX_ECCINFO_PARITY_ODD; // Inverse the bit
    }
  } else {
    normalized_s = BN_copy(normalized_s, s);
  }

  buf_r = malloc(domain->length);
  buf_s = malloc(domain->length);
  BN_bn2binpad(r, buf_r, domain->length);
  BN_bn2binpad(normalized_s, buf_s, domain->length);
  int ret = spec_cx_ecfp_encode_sig_der(sig, sig_len, buf_r, domain->length, buf_s,
                                   domain->length);
  free(buf_r);
  free(buf_s);
  ECDSA_SIG_free(ecdsa_sig);
  BN_free(normalized_s);
  BN_free(halfn);
  EC_KEY_free(ec_key);
  return ret;
}

unsigned long sys_cx_ecfp_init_public_key(cx_curve_t curve,
                                          const unsigned char *rawkey,
                                          unsigned int key_len,
                                          cx_ecfp_public_key_t *key)
{
  const cx_curve_domain_t *domain;
  unsigned int expected_key_len;
  unsigned int size;

  domain = cx_ecfp_get_domain(curve);
  size = domain->length;

  memset(key, 0, sizeof(cx_ecfp_public_key_t));

  if (rawkey != NULL) {
    expected_key_len = 0;
    if (rawkey[0] == 0x02) {
      if (CX_CURVE_RANGE(curve, TWISTED_EDWARD) ||
          CX_CURVE_RANGE(curve, MONTGOMERY)) {
        expected_key_len = 1 + size;
      }
    } else if (rawkey[0] == 0x04) {
      if (CX_CURVE_RANGE(curve, WEIERSTRASS) ||
          CX_CURVE_RANGE(curve, TWISTED_EDWARD)) {
        expected_key_len = 1 + size * 2;
      }
    }
    if (expected_key_len == 0 || key_len != expected_key_len) {
      THROW(INVALID_PARAMETER);
    }
  } else {
    key_len = 0;
  }
  // init key
  key->curve = curve;
  key->W_len = key_len;
  memcpy(key->W, rawkey, key_len);

  return key_len;
}

static int ecdh_simple_compute_key_hack(unsigned char *pout, size_t *poutlen,
                                        const EC_POINT *pub_key,
                                        const EC_KEY *ecdh)
{
  BN_CTX *ctx;
  EC_POINT *tmp = NULL;
  BIGNUM *x = NULL, *y = NULL;
  int xLen, yLen;
  const BIGNUM *priv_key;
  const EC_GROUP *group;
  size_t group_size;
  int ret = 0;
  unsigned char *buf = NULL;

  if ((ctx = BN_CTX_new()) == NULL) {
    goto err;
  }
  BN_CTX_start(ctx);
  x = BN_CTX_get(ctx);
  if (x == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
    goto err;
  }

  y = BN_CTX_get(ctx);
  if (y == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
    goto err;
  }

  priv_key = EC_KEY_get0_private_key(ecdh);
  if (priv_key == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_NO_PRIVATE_VALUE);
    goto err;
  }

  group = EC_KEY_get0_group(ecdh);
  group_size = (EC_GROUP_get_degree(group) + 7) / 8;

  if (EC_KEY_get_flags(ecdh) & EC_FLAG_COFACTOR_ECDH) {
    if (!EC_GROUP_get_cofactor(group, x, NULL) ||
        !BN_mul(x, x, priv_key, ctx)) {
      ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
      goto err;
    }
    priv_key = x;
  }

  if ((tmp = EC_POINT_new(group)) == NULL) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_MALLOC_FAILURE);
    goto err;
  }

  if (!EC_POINT_mul(group, tmp, NULL, pub_key, priv_key, ctx)) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_POINT_ARITHMETIC_FAILURE);
    goto err;
  }

  if (!EC_POINT_get_affine_coordinates(group, tmp, x, y, ctx)) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_POINT_ARITHMETIC_FAILURE);
    goto err;
  }

  if (*poutlen < (1 + 32 + 32)) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, EC_R_INVALID_ARGUMENT);
    goto err;
  }

  pout[0] = 0x04;
  xLen = BN_bn2binpad(x, pout + 1, group_size);
  if (xLen < 0) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_BN_LIB);
    goto err;
  }
  yLen = BN_bn2binpad(y, pout + 1 + xLen, group_size);
  if (yLen < 0) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_BN_LIB);
    goto err;
  }
  *poutlen = 1 + xLen + yLen;
  buf = NULL;

  ret = 1;

err:
  EC_POINT_clear_free(tmp);
  BN_CTX_end(ctx);
  BN_CTX_free(ctx);
  OPENSSL_free(buf);
  return ret;
}

int sys_cx_ecdh(const cx_ecfp_private_key_t *key, int mode,
                const uint8_t *public_point, size_t UNUSED(P_len),
                uint8_t *secret, size_t secret_len)
{
  const cx_curve_domain_t *domain;
  EC_KEY *privkey, *peerkey;
  // uint8_t point[65];
  BIGNUM *x, *y;
  int nid;

  domain = cx_ecfp_get_domain(key->curve);

  x = BN_new();
  BN_bin2bn(key->d, key->d_len, x);
  nid = nid_from_curve(key->curve);
  privkey = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_private_key(privkey, x);
  BN_free(x);

  x = BN_new();
  y = BN_new();
  BN_bin2bn(public_point + 1, domain->length, x);
  BN_bin2bn(public_point + domain->length + 1, domain->length, y);

  nid = nid_from_curve(key->curve);
  peerkey = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_public_key_affine_coordinates(peerkey, x, y);

  BN_free(y);
  BN_free(x);

  switch (mode & CX_MASK_EC) {
  case CX_ECDH_POINT:
    if (!ecdh_simple_compute_key_hack(
            secret, &secret_len, EC_KEY_get0_public_key(peerkey), privkey)) {
      THROW(INVALID_PARAMETER);
    }
    break;
  case CX_ECDH_X:
    secret_len = ECDH_compute_key(
        secret, secret_len, EC_KEY_get0_public_key(peerkey), privkey, NULL);
    break;
  }

  EC_KEY_free(privkey);
  EC_KEY_free(peerkey);

  /* XXX: not used in the result */
  /*switch (mode & CX_MASK_EC) {
  case CX_ECDH_POINT:
    if (public_point[0] == 4) {
      len = 1 + 2 * domain->length;
    } else {
      len = 1 + domain->length;
    }
    break;

  case CX_ECDH_X:
    len = domain->length;
    break;

  default:
    THROW(INVALID_PARAMETER);
    break;
  }*/
  return secret_len;
}

static int cx_weierstrass_mult(cx_curve_t curve, BIGNUM *qx, BIGNUM *qy,
                               BIGNUM *px, BIGNUM *py, BIGNUM *k)
{
  EC_POINT *p, *q;
  EC_GROUP *group = NULL;
  BN_CTX *ctx;
  int ret = 0;

  if (curve == CX_CURVE_SECP256K1) {
    group = EC_GROUP_new_by_curve_name(NID_secp256k1);
  } else if (curve == CX_CURVE_256R1) {
    group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
  } else {
    return 0;
  }
  if (group == NULL) {
    errx(1, "cx_weierstrass_mult: EC_GROUP_new_by_curve_name() failed");
  }

  p = EC_POINT_new(group);
  q = EC_POINT_new(group);
  if (p == NULL || q == NULL) {
    errx(1, "cx_weierstrass_mult: EC_POINT_new() failed");
  }

  ctx = BN_CTX_new();
  if (ctx == NULL) {
    errx(1, "cx_weierstrass_mult: BN_CTX_new() failed");
  }

  if (EC_POINT_set_affine_coordinates(group, p, px, py, ctx) != 1) {
    goto err;
  }
  if (EC_POINT_mul(group, q, NULL, p, k, ctx) != 1) {
    goto err;
  }
  if (EC_POINT_get_affine_coordinates(group, q, qx, qy, ctx) != 1) {
    goto err;
  }

  ret = 1;

err:
  BN_CTX_free(ctx);
  EC_GROUP_free(group);
  EC_POINT_free(p);
  EC_POINT_free(q);
  return ret;
}

int sys_cx_ecfp_scalar_mult(cx_curve_t curve, unsigned char *P,
                            unsigned int P_len, const unsigned char *k,
                            unsigned int k_len)
{
  BIGNUM *Px, *Py, *Qx, *Qy, *e;

  /* TODO: ensure that the point is valid */

  const uint8_t PTy = P[0];

  if (curve == CX_CURVE_Curve25519) {
    if (PTy != 0x02) {
      errx(1, "cx_ecfp_scalar_mult: only compressed points for Curve25519 are "
              "supported.");
    }
    if (P_len != (32 + 1) || k_len != 32) {
      errx(1, "cx_ecfp_scalar_mult: expected P_len == 33 and k_len == 32");
    }
    if (scalarmult_curve25519(&P[1], k, &P[1]) != 0) {
      errx(1, "cx_ecfp_scalar_mult: scalarmult_x25519 failed");
    }
    return P_len;
  }

  if (P_len != 65) {
    errx(1, "cx_ecfp_scalar_mult: invalid P_len (%u)", P_len);
  }

  Px = BN_new();
  Py = BN_new();
  Qx = BN_new();
  Qy = BN_new();
  e = BN_new();

  if (Px == NULL || Py == NULL || Qx == NULL || Qy == NULL || e == NULL) {
    errx(1, "cx_ecfp_scalar_mult: BN_new() failed");
  }

  BN_bin2bn(P + 1, 32, Px);
  BN_bin2bn(P + 33, 32, Py);
  BN_bin2bn(k, k_len, e);

  switch (curve) {
  case CX_CURVE_Ed25519: {
    if (scalarmult_ed25519(Qx, Qy, Px, Py, e) != 0) {
      errx(1, "cx_ecfp_scalar_mult: scalarmult_ed25519 failed");
    }
  } break;
  case CX_CURVE_SECP256K1:
  case CX_CURVE_SECP256R1:
    if (PTy != 0x04) {
      errx(1, "cx_ecfp_scalar_mult: compressed points for Weierstrass curves "
              "are not supported yet");
    }
    if (cx_weierstrass_mult(curve, Qx, Qy, Px, Py, e) != 1) {
      errx(1, "cx_ecfp_scalar_mult: cx_weierstrass_mult failed");
    }
    break;
  default:
    errx(1, "cx_ecfp_scalar_mult: TODO: unsupported curve (0x%x)", curve);
    break;
  }

  BN_bn2binpad(Qx, P + 1, 32);
  BN_bn2binpad(Qy, P + 33, 32);

  BN_free(Qy);
  BN_free(Qx);
  BN_free(Py);
  BN_free(Px);
  BN_free(e);

  return 0;
}

int sys_cx_ecfp_add_point(cx_curve_t curve, uint8_t *R, const uint8_t *P,
                          const uint8_t *Q, size_t X_len)
{
  POINT PP, QQ, RR;
  int ret;

  /* TODO: ensure that points are valid */

  PP.x = BN_new();
  PP.y = BN_new();
  QQ.x = BN_new();
  QQ.y = BN_new();
  RR.x = BN_new();
  RR.y = BN_new();
  if (PP.x == NULL || PP.y == NULL || QQ.x == NULL || QQ.y == NULL ||
      RR.x == NULL || RR.y == NULL) {
    errx(1, "cx_ecfp_scalar_mult: BN_new() failed");
    return -1;
  }

  switch (curve) {
  case CX_CURVE_Ed25519:
    if (X_len != 65) {
      errx(1, "cx_ecfp_add_point: invalid X_len (%zu)", X_len);
      ret = -1;
      break;
    }

    if (P[0] != 0x04 || Q[0] != 0x4) {
      errx(1, "cx_ecfp_add_point: points must be uncompressed");
      ret = -1;
      break;
    }

    BN_bin2bn(P + 1, 32, PP.x);
    BN_bin2bn(P + 33, 32, PP.y);
    BN_bin2bn(Q + 1, 32, QQ.x);
    BN_bin2bn(Q + 33, 32, QQ.y);

    if (edwards_add(&RR, &PP, &QQ) != 0) {
      errx(1, "cx_ecfp_add_point: edwards_add failed");
      ret = -1;
      break;
    }

    R[0] = 0x04;
    BN_bn2binpad(RR.x, R + 1, 32);
    BN_bn2binpad(RR.y, R + 33, 32);
    ret = 0;
    break;

  default:
    errx(1, "cx_ecfp_add_point: TODO: unsupported curve (0x%x)", curve);
    ret = -1;
    break;
  }

  BN_free(RR.y);
  BN_free(RR.x);
  BN_free(QQ.y);
  BN_free(QQ.x);
  BN_free(PP.y);
  BN_free(PP.x);

  return ret;
}
