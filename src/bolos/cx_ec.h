#pragma once

#include "cx_bn.h"
#include "cx_hash.h"

#define _SDK_2_0_

#define CX_CURVE_RANGE(i, dom)                                                 \
  (((i) > (CX_CURVE_##dom##_START)) && ((i) < (CX_CURVE_##dom##_END)))

#define CX_MASK_EC               (7 << 12)
#define CX_ECDH_POINT            (1 << 12)
#define CX_ECDH_X                (2 << 12)
#define CX_ECSCHNORR_ISO14888_XY (3 << 12)
#define CX_ECSCHNORR_ISO14888_X  (4 << 12)
#define CX_ECSCHNORR_BSI03111    (5 << 12)
#define CX_ECSCHNORR_LIBSECP     (6 << 12)
#define CX_ECSCHNORR_Z           (7 << 12)

/** Convenience type. See #cx_curve_e. */
/* the following curves are not wicheproof tested and thus should be considered
 * as 'experimental feature: sec521r1 startk montgomery curves
 */

static uint8_t const C_cofactor_1[] = { 0, 0, 0, 1 };
static uint8_t const C_cofactor_4[] = { 0, 0, 0, 4 };
static uint8_t const C_cofactor_8[] = { 0, 0, 0, 8 };

/** List of supported elliptic curves */
enum cx_curve_e {
  CX_CURVE_NONE,

  /* ------------------------ */
  /* --- Type Weierstrass --- */
  /* ------------------------ */
  /** Low limit (not included) of Weierstrass curve ID */
  CX_CURVE_WEIERSTRASS_START = 0x20,

  /** Secp.org */
  CX_CURVE_SECP256K1 = 0x21,
#define CX_CURVE_256K1 CX_CURVE_SECP256K1

  CX_CURVE_SECP256R1 = 0x22,
#define CX_CURVE_256R1    CX_CURVE_SECP256R1
#define CX_CURVE_NISTP256 CX_CURVE_SECP256R1

  CX_CURVE_SECP384R1 = 0x23,
#define CX_CURVE_NISTP384 CX_CURVE_SECP384R1

  CX_CURVE_SECP521R1 = 0x24,
#define CX_CURVE_NISTP521 CX_CURVE_SECP521R1

  /** BrainpoolP256t1 */
  CX_CURVE_BrainPoolP256T1 = 0x31,

  /** BrainpoolP256r1 */
  CX_CURVE_BrainPoolP256R1 = 0x32,

  /** BrainpoolP320t1 */
  CX_CURVE_BrainPoolP320T1 = 0x33,

  /** BrainpoolP320r1 */
  CX_CURVE_BrainPoolP320R1 = 0x34,

  /** BrainpoolP384t1 */
  CX_CURVE_BrainPoolP384T1 = 0x35,

  /** Brainpool384r1 */
  CX_CURVE_BrainPoolP384R1 = 0x36,

  /** BrainpoolP512t1 */
  CX_CURVE_BrainPoolP512T1 = 0x37,

  /** BrainpoolP512r1 */
  CX_CURVE_BrainPoolP512R1 = 0x38,

  CX_CURVE_BLS12_381_G1 = 0x39,
  /* STARK */
  CX_CURVE_Stark256 = 0x3A,

  /** High limit (not included) of Weierstrass curve ID */
  CX_CURVE_WEIERSTRASS_END = 0x6F,

  /* ---------------------------- */
  /* --- Type Twister Edwards --- */
  /* ---------------------------- */
  /** Low limit (not included) of  Twister Edwards curve ID */
  CX_CURVE_TWISTED_EDWARDS_START = 0x70,

  /** Ed25519 curve */
  CX_CURVE_Ed25519 = 0x71,

  CX_CURVE_Ed448 = 0x72,

  CX_CURVE_TWISTED_EDWARDS_END = 0x7F,
  /** High limit (not included) of Twister Edwards  curve ID */

  /* ----------------------- */
  /* --- Type Montgomery --- */
  /* ----------------------- */
  /** Low limit (not included) of Montgomery curve ID */
  CX_CURVE_MONTGOMERY_START = 0x80,

  /** Curve25519 curve */
  CX_CURVE_Curve25519 = 0x81,
  CX_CURVE_Curve448 = 0x82,

  CX_CURVE_MONTGOMERY_END = 0x8F

  /** High limit (not included) of Montgomery curve ID */
};

#define CX_CURVE_RANGE(i, dom)                                                 \
  (((i) > (CX_CURVE_##dom##_START)) && ((i) < (CX_CURVE_##dom##_END)))

/** Return true if curve is a short Weierstrass curve */
#define CX_CURVE_IS_WEIERSTRASS(c)                                             \
  (((c) > CX_CURVE_WEIERSTRASS_START) && ((c) < CX_CURVE_WEIERSTRASS_END))

/** Return true if curve is a twisted Edwards curve */
#define CX_CURVE_IS_TWISTED_EDWARDS(c)                                         \
  (((c) > CX_CURVE_TWISTED_EDWARDS_START) &&                                   \
   ((c) < CX_CURVE_TWISTED_EDWARDS_END))

/** Return true if curve is a Montgomery curve */
#define CX_CURVE_IS_MONTGOMERY(c)                                              \
  (((c) > CX_CURVE_MONTGOMERY_START) && ((c) < CX_CURVE_MONTGOMERY_END))

/** Convenience type. See #cx_curve_e. */
typedef enum cx_curve_e cx_curve_t;

#define CX_ECCINFO_PARITY_ODD 1
#define CX_ECCINFO_xGTn       2

#define CX_CURVE_HEADER                                                        \
  /** Curve Identifier. See #cx_curve_e */                                     \
  cx_curve_t curve;                                                            \
  /** Curve size in bits */                                                    \
  unsigned int bit_size;                                                       \
  /** component length in bytes */                                             \
  unsigned int length;                                                         \
  /**  a coef */                                                               \
  const uint8_t *a;                                                            \
  /**  b/d coef */                                                             \
  const uint8_t *b;                                                            \
  /** Curve field */                                                           \
  const uint8_t *p;                                                            \
  /** Point Generator x coordinate */                                          \
  const uint8_t *Gx;                                                           \
  /** Point Generator y coordinate */                                          \
  const uint8_t *Gy;                                                           \
  /** Curve order */                                                           \
  const uint8_t *n;                                                            \
  /**  cofactor */                                                             \
  const uint8_t *h;                                                            \
  /** @internal 2nd Montgomery constant for Curve order */                     \
  const uint8_t *Hn;                                                           \
  /** @internal 2nd Montgomery constant for Field */                           \
  const uint8_t *Hp;

struct cx_curve_weierstrass_s {
  CX_CURVE_HEADER
};

/*Note that the reuse of   CX_CURVE_HEADER is such that coefficient b won't be
 * used */
/* Convenience for code compacity*/
/* TODO: find a better factorization of code*/
struct cx_curve_twisted_edwards_s {
  CX_CURVE_HEADER;
  const unsigned char *I;
  const unsigned char *d;
  /** @internal  (q+3)/8 or (q+1)/4*/
  const unsigned char *Qq;
};

typedef struct cx_curve_twisted_edwards_s cx_curve_twisted_edwards_t;

/** Convenience type. See #cx_curve_weierstrass_s. */
typedef struct cx_curve_weierstrass_s cx_curve_weierstrass_t;

struct cx_curve_domain_s {
  CX_CURVE_HEADER;
};
/** Convenience type. See #cx_curve_domain_s. */
typedef struct cx_curve_domain_s cx_curve_domain_t;

/** Up to 256 bits Public Elliptic Curve key */
struct cx_ecfp_256_public_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t W_len;
  /** Public key value starting at offset 0 */
  unsigned char W[65];
};
/** Up to 256 bits Private Elliptic Curve key */
struct cx_ecfp_256_private_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[32];
};
/** Up to 256 bits Extended Private Elliptic Curve key */
struct cx_ecfp_256_extended_private_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[64];
};
/** Convenience type. See #cx_ecfp_256_public_key_s. */
typedef struct cx_ecfp_256_public_key_s cx_ecfp_256_public_key_t;
/** temporary def type. See #cx_ecfp_256_private_key_s. */
typedef struct cx_ecfp_256_private_key_s cx_ecfp_256_private_key_t;
/** Convenience type. See #cx_ecfp_256_extended_private_key_s. */
typedef struct cx_ecfp_256_extended_private_key_s
    cx_ecfp_256_extended_private_key_t;

/* Do not use those types anymore for declaration, they will become abstract */
typedef struct cx_ecfp_256_public_key_s cx_ecfp_public_key_t;
typedef struct cx_ecfp_256_private_key_s cx_ecfp_private_key_t;

/** Up to 384 bits Public Elliptic Curve key */
struct cx_ecfp_384_public_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t W_len;
  /** Public key value starting at offset 0 */
  uint8_t W[97];
};
/** Up to 384 bits Private Elliptic Curve key */
struct cx_ecfp_384_private_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t d_len;
  /** Public key value starting at offset 0 */
  uint8_t d[48];
};
/** Convenience type. See #cx_ecfp_384_public_key_s. */
typedef struct cx_ecfp_384_private_key_s cx_ecfp_384_private_key_t;
/** Convenience type. See #cx_ecfp_384_private_key_s. */
typedef struct cx_ecfp_384_public_key_s cx_ecfp_384_public_key_t;

/** Up to 512 bits Public Elliptic Curve key */
struct cx_ecfp_512_public_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t W_len;
  /** Public key value starting at offset 0 */
  unsigned char W[129];
};
/** Up to 512 bits Private Elliptic Curve key */
struct cx_ecfp_512_private_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[64];
};
/** Up to 512 bits Extended Private Elliptic Curve key */
struct cx_ecfp_512_extented_private_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[128];
};
/** Convenience type. See #cx_ecfp_512_public_key_s. */
typedef struct cx_ecfp_512_public_key_s cx_ecfp_512_public_key_t;
/** Convenience type. See #cx_ecfp_512_private_key_s. */
typedef struct cx_ecfp_512_private_key_s cx_ecfp_512_private_key_t;
/** Convenience type. See #cx_ecfp_512_extented_private_key_s. */
typedef struct cx_ecfp_512_extented_private_key_s
    cx_ecfp_512_extented_private_key_t;

/** Up to 640 bits Public Elliptic Curve key */
struct cx_ecfp_640_public_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t W_len;
  /** Public key value starting at offset 0 */
  unsigned char W[161];
};
/** Up to 640 bits Private Elliptic Curve key */
struct cx_ecfp_640_private_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t curve;
  /** Public key length in bytes */
  size_t d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[80];
};
/** Convenience type. See #cx_ecfp_640_public_key_s. */
typedef struct cx_ecfp_640_public_key_s cx_ecfp_640_public_key_t;
/** Convenience type. See #cx_ecfp_640_private_key_s. */
typedef struct cx_ecfp_640_private_key_s cx_ecfp_640_private_key_t;

/*
 * Montgomery curve: B*y^2= x^3 + A*x^2 + x over F(q)
 */
struct cx_curve_montgomery_s {
  CX_CURVE_HEADER
};
/** Convenience type. See #cx_curve_montgomery_s. */
typedef struct cx_curve_montgomery_s cx_curve_montgomery_t;

/**
 *
 */
struct cx_ec_point_s {
  cx_curve_t curve;
  cx_bn_t x;
  cx_bn_t y;
  cx_bn_t z;
};

/** Convenience type. See #cx_ec_point_s. */
typedef struct cx_ec_point_s cx_ecpoint_t;

enum cx_curve_dom_param_s {
  CX_CURVE_PARAM_NONE = 0,
  CX_CURVE_PARAM_A = 1,
  CX_CURVE_PARAM_B = 2,
  CX_CURVE_PARAM_Field = 3,
  CX_CURVE_PARAM_Gx = 4,
  CX_CURVE_PARAM_Gy = 5,
  CX_CURVE_PARAM_Order = 6,
  CX_CURVE_PARAM_Cofactor = 7,
};
typedef enum cx_curve_dom_param_s cx_curve_dom_param_t;

extern cx_curve_domain_t const *const C_cx_allCurves[];

extern cx_curve_weierstrass_t const *const C_cx_all_Weierstrass_Curves[];

const cx_curve_domain_t *cx_ecfp_get_domain(cx_curve_t curve);

int sys_cx_ecfp_add_point(cx_curve_t curve, uint8_t *R, const uint8_t *P,
                          const uint8_t *Q, size_t X_len);
unsigned long sys_cx_ecfp_init_public_key(cx_curve_t curve,
                                          const unsigned char *rawkey,
                                          unsigned int key_len,
                                          cx_ecfp_public_key_t *key);
int sys_cx_ecfp_generate_pair(cx_curve_t curve,
                              cx_ecfp_public_key_t *public_key,
                              cx_ecfp_private_key_t *private_key,
                              int keep_private);
int sys_cx_ecfp_generate_pair2(cx_curve_t curve,
                               cx_ecfp_public_key_t *public_key,
                               cx_ecfp_private_key_t *private_key,
                               int keep_private, cx_md_t hashID);

int sys_cx_ecfp_init_private_key(cx_curve_t curve, const uint8_t *raw_key,
                                 unsigned int key_len,
                                 cx_ecfp_private_key_t *key);

int sys_cx_ecfp_scalar_mult(cx_curve_t curve, unsigned char *P,
                            unsigned int P_len, const unsigned char *k,
                            unsigned int k_len);
int sys_cx_edward_compress_point(cx_curve_t curve, uint8_t *P, size_t P_len);
int sys_cx_eddsa_get_public_key(const cx_ecfp_private_key_t *pv_key,
                                cx_md_t hashID, cx_ecfp_public_key_t *pu_key);
int sys_cx_edward_decompress_point(cx_curve_t curve, uint8_t *P, size_t P_len);

const cx_curve_weierstrass_t *cx_ecfp_get_weierstrass(cx_curve_t curve);
/*
extern cx_curve_domain_t const *const C_cx_allCurves[];
extern cx_curve_weierstrass_t const *const C_cx_all_Weierstrass_Curves[];
*/
