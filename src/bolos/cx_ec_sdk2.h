#pragma once
/*******************************************************************************
 *   Ledger Nano S - Secure firmware
 *   (c) 2021 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#define HAVE_SECP256K1_CURVE
#define HAVE_SECP256R1_CURVE
#define HAVE_SECP384R1_CURVE
#define HAVE_SECP521R1_CURVE

#define HAVE_BRAINPOOL_P256R1_CURVE
#define HAVE_BRAINPOOL_P256T1_CURVE
#define HAVE_BRAINPOOL_P320R1_CURVE
#define HAVE_BRAINPOOL_P320T1_CURVE
#define HAVE_BRAINPOOL_P384R1_CURVE
#define HAVE_BRAINPOOL_P384T1_CURVE
#define HAVE_BRAINPOOL_P512R1_CURVE
#define HAVE_BRAINPOOL_P512T1_CURVE

#define HAVE_ED25519_CURVE
#define HAVE_ED448_CURVE

#define HAVE_CV25519_CURVE
#define HAVE_CV448_CURVE

/**
 *
 */
#define CX_ECCINFO_PARITY_ODD 1
#define CX_ECCINFO_xGTn       2

/** List of supported elliptic curves */
enum cx_curve_e {
  CX_CURVE_NONE,

  /* ------------------------ */
  /* --- Type Weierstrass --- */
  /* ------------------------ */
  /** Low limit (not included) of Weierstrass curve ID */
  CX_CURVE_WEIERSTRASS_START = 0x20,

/** Secp.org */
#ifdef HAVE_SECP256K1_CURVE
  CX_CURVE_SECP256K1 = 0x21,
#define CX_CURVE_256K1 CX_CURVE_SECP256K1
#endif

#ifdef HAVE_SECP256R1_CURVE
  CX_CURVE_SECP256R1 = 0x22,
#define CX_CURVE_256R1    CX_CURVE_SECP256R1
#define CX_CURVE_NISTP256 CX_CURVE_SECP256R1
#endif

#ifdef HAVE_SECP384R1_CURVE
  CX_CURVE_SECP384R1 = 0x23,
#define CX_CURVE_NISTP384 CX_CURVE_SECP384R1
#endif

#ifdef HAVE_SECP521R1_CURVE
  CX_CURVE_SECP521R1 = 0x24,
#define CX_CURVE_NISTP521 CX_CURVE_SECP521R1
#endif

#ifdef HAVE_BRAINPOOL_P256T1_CURVE
  /** BrainpoolP256t1 */
  CX_CURVE_BrainPoolP256T1 = 0x31,
#endif

#ifdef HAVE_BRAINPOOL_P256R1_CURVE
  /** BrainpoolP256r1 */
  CX_CURVE_BrainPoolP256R1 = 0x32,
#endif

#ifdef HAVE_BRAINPOOL_P320T1_CURVE
  /** BrainpoolP320t1 */
  CX_CURVE_BrainPoolP320T1 = 0x33,
#endif

#ifdef HAVE_BRAINPOOL_P320R1_CURVE
  /** BrainpoolP320r1 */
  CX_CURVE_BrainPoolP320R1 = 0x34,
#endif

#ifdef HAVE_BRAINPOOL_P384T1_CURVE
  /** BrainpoolP384t1 */
  CX_CURVE_BrainPoolP384T1 = 0x35,
#endif

#ifdef HAVE_BRAINPOOL_P384R1_CURVE
  /** Brainpool384r1 */
  CX_CURVE_BrainPoolP384R1 = 0x36,
#endif

#ifdef HAVE_BRAINPOOL_P512T1_CURVE
  /** BrainpoolP512t1 */
  CX_CURVE_BrainPoolP512T1 = 0x37,
#endif

#ifdef HAVE_BRAINPOOL_P512R1_CURVE
  /** BrainpoolP512r1 */
  CX_CURVE_BrainPoolP512R1 = 0x38,
#endif

  CX_CURVE_BLS12_381_G1 = 0x39,

  /** High limit (not included) of Weierstrass curve ID */
  CX_CURVE_WEIERSTRASS_END = 0x6F,

  /* ---------------------------- */
  /* --- Type Twister Edwards --- */
  /* ---------------------------- */
  /** Low limit (not included) of  Twister Edwards curve ID */
  CX_CURVE_TWISTED_EDWARDS_START = 0x70,

/** Ed25519 curve */
#ifdef HAVE_ED25519_CURVE
  CX_CURVE_Ed25519 = 0x71,
#endif

#ifdef HAVE_ED25519_CURVE
  CX_CURVE_Ed448 = 0x72,
#endif

  CX_CURVE_TWISTED_EDWARDS_END = 0x7F,
  /** High limit (not included) of Twister Edwards  curve ID */

  /* ----------------------- */
  /* --- Type Montgomery --- */
  /* ----------------------- */
  /** Low limit (not included) of Montgomery curve ID */
  CX_CURVE_MONTGOMERY_START = 0x80,

/** Curve25519 curve */
#ifdef HAVE_CV25519_CURVE
  CX_CURVE_Curve25519 = 0x81,
#endif
#ifdef HAVE_CV448_CURVE
  CX_CURVE_Curve448 = 0x82,
#endif

  CX_CURVE_MONTGOMERY_END = 0x8F

  /** High limit (not included) of Montgomery curve ID */
};

/** Convenience type. See #cx_curve_e. */
typedef enum cx_curve_e cx_curve_t;

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

/**
 * Weierstrass curve :     y^3=x^2+a*x+b        over F(p)
 *
 */
struct cx_curve_weierstrass_s {
  CX_CURVE_HEADER
};
/** Convenience type. See #cx_curve_weierstrass_s. */
typedef struct cx_curve_weierstrass_s cx_curve_weierstrass_t;

/*
 * Twisted Edwards curve : a*x^2+y^2=1+d*x2*y2  over F(q)
 */
struct cx_curve_twisted_edwards_s {
  CX_CURVE_HEADER
};
/** Convenience type. See #cx_curve_twisted_edwards_s. */
typedef struct cx_curve_twisted_edwards_s cx_curve_twisted_edwards_t;

/*
 * Montgomery curve: B*y^2= x^3 + A*x^2 + x over F(q)
 */
struct cx_curve_montgomery_s {
  CX_CURVE_HEADER
};
/** Convenience type. See #cx_curve_montgomery_s. */
typedef struct cx_curve_montgomery_s cx_curve_montgomery_t;

/** Abstract type for elliptic curve domain */
struct cx_curve_domain_s {
  CX_CURVE_HEADER
};
/** Convenience type. See #cx_curve_domain_s. */
typedef struct cx_curve_domain_s cx_curve_domain_t;

// cx_bn_t is an index on the corresponding cx_mpi_array;
typedef uint32_t cx_bn_t;

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
