#ifndef _CX_EC_H
#define _CX_EC_H

#define CX_MASK_EC                  (7<<12)
#define CX_ECDH_POINT               (1<<12)
#define CX_ECDH_X                   (2<<12)
#define CX_ECSCHNORR_ISO14888_XY    (3<<12)
#define CX_ECSCHNORR_ISO14888_X     (4<<12)
#define CX_ECSCHNORR_BSI03111       (5<<12)
#define CX_ECSCHNORR_LIBSECP        (6<<12)
#define CX_ECSCHNORR_Z              (7<<12)

/** List of supported elliptic curves */
enum cx_curve_e {
  CX_CURVE_NONE,
  /* ------------------------ */
  /* --- Type Weierstrass --- */
  /* ------------------------ */
  /** Low limit (not included) of Weierstrass curve ID */
  CX_CURVE_WEIERSTRASS_START = 0x20,

  /** Secp.org */
  CX_CURVE_SECP256K1,
  CX_CURVE_SECP256R1,
  #define CX_CURVE_256K1  CX_CURVE_SECP256K1
  #define CX_CURVE_256R1  CX_CURVE_SECP256R1
  CX_CURVE_SECP384R1,
  CX_CURVE_SECP521R1,

  /** BrainPool */
  CX_CURVE_BrainPoolP256T1,
  CX_CURVE_BrainPoolP256R1,
  CX_CURVE_BrainPoolP320T1,
  CX_CURVE_BrainPoolP320R1,
  CX_CURVE_BrainPoolP384T1,
  CX_CURVE_BrainPoolP384R1,
  CX_CURVE_BrainPoolP512T1,
  CX_CURVE_BrainPoolP512R1,

  /* NIST P256 curve*/
  #define CX_CURVE_NISTP256  CX_CURVE_SECP256R1
  #define CX_CURVE_NISTP384  CX_CURVE_SECP384R1
  #define CX_CURVE_NISTP521  CX_CURVE_SECP521R1

  /* ANSSI P256 */
  CX_CURVE_FRP256V1,

  /** High limit (not included) of Weierstrass curve ID */
  CX_CURVE_WEIERSTRASS_END,

  /* --------------------------- */
  /* --- Type Twister Edward --- */
  /* --------------------------- */
  /** Low limit (not included) of  Twister Edward curve ID */
  CX_CURVE_TWISTED_EDWARD_START = 0x40,

  /** Ed25519 curve */
  CX_CURVE_Ed25519,
  CX_CURVE_Ed448,

  CX_CURVE_TWISTED_EDWARD_END,
  /** High limit (not included) of Twister Edward  curve ID */

  /* ----------------------- */
  /* --- Type Montgomery --- */
  /* ----------------------- */
 /** Low limit (not included) of Montgomery curve ID */
  CX_CURVE_MONTGOMERY_START = 0x60,

  /** Curve25519 curve */
  CX_CURVE_Curve25519,
  CX_CURVE_Curve448,

  CX_CURVE_MONTGOMERY_END
  /** High limit (not included) of Montgomery curve ID */
};
/** Convenience type. See #cx_curve_e. */
typedef enum cx_curve_e cx_curve_t;

#define CX_ECCINFO_PARITY_ODD   1
#define CX_ECCINFO_xGTn         2

#define CX_CURVE_HEADER                                   \
  /** Curve Identifier. See #cx_curve_e */                \
  cx_curve_t curve;                                       \
  /** Curve size in bits */                               \
   unsigned int  bit_size;                                \
 /** component lenth in bytes */                          \
  unsigned int  length;                                   \
  /** Curve field */                                      \
  unsigned char *p;                                  \
  /** @internal 2nd Mongtomery constant for Field */      \
  unsigned char *Hp;                                 \
  /** Point Generator x coordinate*/                      \
  unsigned char *Gx;                                 \
  /** Point Generator y coordinate*/                      \
  unsigned char *Gy;                                 \
  /** Curve order*/                                       \
  unsigned char *n;                                  \
  /** @internal 2nd Mongtomery constant for Curve order*/ \
  unsigned char *Hn;                                 \
  /**  cofactor */                                        \
  int                 h

/**
 * Weirstrass curve :     y^3=x^2+a*x+b        over F(p)
 *
 */
struct cx_curve_weierstrass_s {
  CX_CURVE_HEADER;
  /**  a coef */
  unsigned char *a;
  /**  b coef */
  unsigned char *b;
};

/** Convenience type. See #cx_curve_weierstrass_s. */
typedef struct cx_curve_weierstrass_s cx_curve_weierstrass_t;

struct cx_curve_domain_s {
  CX_CURVE_HEADER;
} ;
/** Convenience type. See #cx_curve_domain_s. */
typedef struct cx_curve_domain_s cx_curve_domain_t;

/** Up to 256 bits Public Elliptic Curve key */
struct cx_ecfp_256_public_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t    curve;
  /** Public key length in bytes */
  unsigned int  W_len;
  /** Public key value starting at offset 0 */
  unsigned char W[65];
};
/** Up to 256 bits Private Elliptic Curve key */
struct cx_ecfp_256_private_key_s{
  /** curve ID #cx_curve_e */
  cx_curve_t    curve;
  /** Public key length in bytes */
  unsigned int  d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[32];
};
/** Up to 256 bits Extended Private Elliptic Curve key */
struct cx_ecfp_256_extended_private_key_s{
  /** curve ID #cx_curve_e */
  cx_curve_t    curve;
  /** Public key length in bytes */
  unsigned int  d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[64];
};
/** Convenience type. See #cx_ecfp_256_public_key_s. */
typedef struct cx_ecfp_256_public_key_s cx_ecfp_256_public_key_t;
/** temporary def type. See #cx_ecfp_256_private_key_s. */
typedef struct cx_ecfp_256_private_key_s cx_ecfp_256_private_key_t;
/** Convenience type. See #cx_ecfp_256_extended_private_key_s. */
typedef struct cx_ecfp_256_extended_private_key_s cx_ecfp_256_extended_private_key_t;

/* Do not use those types anymore for declaration, they will become abstract */
typedef struct cx_ecfp_256_public_key_s cx_ecfp_public_key_t;
typedef struct cx_ecfp_256_private_key_s cx_ecfp_private_key_t;

/** Up to 640 bits Public Elliptic Curve key */
struct cx_ecfp_640_public_key_s {
  /** curve ID #cx_curve_e */
  cx_curve_t    curve;
  /** Public key length in bytes */
  unsigned int  W_len;
  /** Public key value starting at offset 0 */
  unsigned char W[161];
};
/** Up to 640 bits Private Elliptic Curve key */
struct cx_ecfp_640_private_key_s{
  /** curve ID #cx_curve_e */
  cx_curve_t    curve;
  /** Public key length in bytes */
  unsigned int  d_len;
  /** Public key value starting at offset 0 */
  unsigned char d[80];
};
/** Convenience type. See #cx_ecfp_640_public_key_s. */
typedef struct cx_ecfp_640_public_key_s cx_ecfp_640_public_key_t                                                           ;
/** Convenience type. See #cx_ecfp_640_private_key_s. */
typedef struct cx_ecfp_640_private_key_s cx_ecfp_640_private_key_t;

const cx_curve_domain_t *cx_ecfp_get_domain(cx_curve_t curve);

unsigned long sys_cx_ecfp_init_public_key(cx_curve_t curve, const unsigned char *rawkey, unsigned int key_len, cx_ecfp_public_key_t *key);
int sys_cx_ecfp_generate_pair(cx_curve_t curve, cx_ecfp_public_key_t *public_key, cx_ecfp_private_key_t *private_key, int keep_private);
int sys_cx_ecfp_init_private_key(cx_curve_t curve, const uint8_t *raw_key, unsigned int key_len, cx_ecfp_private_key_t *key);
int sys_cx_ecdh(const cx_ecfp_private_key_t *key, int mode, const uint8_t *public_point, size_t P_len, uint8_t *secret, size_t secret_len);

#endif
