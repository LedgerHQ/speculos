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

static unsigned char const C_cx_secp256k1_a[] = {
  // a:  0x00
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static unsigned char const C_cx_secp256k1_b[] = {
  // b:  0x07
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07
};
static unsigned char const C_cx_secp256k1_p[] = {
  // p:  0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xfc, 0x2f
};
static unsigned char const C_cx_secp256k1_Hp[] = {
  // Hp: 0x000000000000000000000000000000000000000000000001000007a2000e90a1
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x07, 0xa2, 0x00, 0x0e, 0x90, 0xa1
};
static unsigned char const C_cx_secp256k1_Gx[] = {
  // Gx: 0x79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798
  0x79, 0xbe, 0x66, 0x7e, 0xf9, 0xdc, 0xbb, 0xac, 0x55, 0xa0, 0x62,
  0x95, 0xce, 0x87, 0x0b, 0x07, 0x02, 0x9b, 0xfc, 0xdb, 0x2d, 0xce,
  0x28, 0xd9, 0x59, 0xf2, 0x81, 0x5b, 0x16, 0xf8, 0x17, 0x98
};
static unsigned char const C_cx_secp256k1_Gy[] = {
  // Gy:  0x483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8
  0x48, 0x3a, 0xda, 0x77, 0x26, 0xa3, 0xc4, 0x65, 0x5d, 0xa4, 0xfb,
  0xfc, 0x0e, 0x11, 0x08, 0xa8, 0xfd, 0x17, 0xb4, 0x48, 0xa6, 0x85,
  0x54, 0x19, 0x9c, 0x47, 0xd0, 0x8f, 0xfb, 0x10, 0xd4, 0xb8
};
static unsigned char const C_cx_secp256k1_n[] = {
  // n: 0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xfe, 0xba, 0xae, 0xdc, 0xe6, 0xaf, 0x48,
  0xa0, 0x3b, 0xbf, 0xd2, 0x5e, 0x8c, 0xd0, 0x36, 0x41, 0x41
};
static unsigned char const C_cx_secp256k1_Hn[] = {
  // Hn:0x9d671cd581c69bc5e697f5e45bcd07c6741496c20e7cf878896cf21467d7d140
  0x9d, 0x67, 0x1c, 0xd5, 0x81, 0xc6, 0x9b, 0xc5, 0xe6, 0x97, 0xf5,
  0xe4, 0x5b, 0xcd, 0x07, 0xc6, 0x74, 0x14, 0x96, 0xc2, 0x0e, 0x7c,
  0xf8, 0x78, 0x89, 0x6c, 0xf2, 0x14, 0x67, 0xd7, 0xd1, 0x40
};

#define C_cx_secp256k1_h 1

static cx_curve_weierstrass_t const C_cx_secp256k1 = {
  CX_CURVE_SECP256K1,
  256,
  32,
  (unsigned char *)C_cx_secp256k1_p,
  (unsigned char *)C_cx_secp256k1_Hp,
  (unsigned char *)C_cx_secp256k1_Gx,
  (unsigned char *)C_cx_secp256k1_Gy,
  (unsigned char *)C_cx_secp256k1_n,
  (unsigned char *)C_cx_secp256k1_Hn,
  C_cx_secp256k1_h,
  (unsigned char *)C_cx_secp256k1_a,
  (unsigned char *)C_cx_secp256k1_b,
};

static unsigned char const C_cx_secp256r1_a[] = {
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc
};
static unsigned char const C_cx_secp256r1_b[] = {
  0x5a, 0xc6, 0x35, 0xd8, 0xaa, 0x3a, 0x93, 0xe7, 0xb3, 0xeb, 0xbd,
  0x55, 0x76, 0x98, 0x86, 0xbc, 0x65, 0x1d, 0x06, 0xb0, 0xcc, 0x53,
  0xb0, 0xf6, 0x3b, 0xce, 0x3c, 0x3e, 0x27, 0xd2, 0x60, 0x4b
};
static unsigned char const C_cx_secp256r1_p[] = {
  // p:  0xffffffff00000001000000000000000000000000ffffffffffffffffffffffff
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};
static unsigned char const C_cx_secp256r1_Hp[] = {
  // Hp: 0x00000004fffffffdfffffffffffffffefffffffbffffffff0000000000000003
  0x00, 0x00, 0x00, 0x04, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xfb, 0xff, 0xff,
  0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03
};
static unsigned char const C_cx_secp256r1_Gx[] = {
  0x6b, 0x17, 0xd1, 0xf2, 0xe1, 0x2c, 0x42, 0x47, 0xf8, 0xbc, 0xe6,
  0xe5, 0x63, 0xa4, 0x40, 0xf2, 0x77, 0x03, 0x7d, 0x81, 0x2d, 0xeb,
  0x33, 0xa0, 0xf4, 0xa1, 0x39, 0x45, 0xd8, 0x98, 0xc2, 0x96
};
static unsigned char const C_cx_secp256r1_Gy[] = {
  0x4f, 0xe3, 0x42, 0xe2, 0xfe, 0x1a, 0x7f, 0x9b, 0x8e, 0xe7, 0xeb,
  0x4a, 0x7c, 0x0f, 0x9e, 0x16, 0x2b, 0xce, 0x33, 0x57, 0x6b, 0x31,
  0x5e, 0xce, 0xcb, 0xb6, 0x40, 0x68, 0x37, 0xbf, 0x51, 0xf5
};
static unsigned char const C_cx_secp256r1_n[] = {
  // n:  0xffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551
  0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xbc, 0xe6, 0xfa, 0xad, 0xa7, 0x17,
  0x9e, 0x84, 0xf3, 0xb9, 0xca, 0xc2, 0xfc, 0x63, 0x25, 0x51
};
static unsigned char const C_cx_secp256r1_Hn[] = {
  // Hn: 0x66e12d94f3d956202845b2392b6bec594699799c49bd6fa683244c95be79eea2
  0x66, 0xe1, 0x2d, 0x94, 0xf3, 0xd9, 0x56, 0x20, 0x28, 0x45, 0xb2,
  0x39, 0x2b, 0x6b, 0xec, 0x59, 0x46, 0x99, 0x79, 0x9c, 0x49, 0xbd,
  0x6f, 0xa6, 0x83, 0x24, 0x4c, 0x95, 0xbe, 0x79, 0xee, 0xa2
};

#define C_cx_secp256r1_h 1

static cx_curve_weierstrass_t const C_cx_secp256r1 = {
  CX_CURVE_SECP256R1,
  256,
  32,
  (unsigned char *)C_cx_secp256r1_p,
  (unsigned char *)C_cx_secp256r1_Hp,
  (unsigned char *)C_cx_secp256r1_Gx,
  (unsigned char *)C_cx_secp256r1_Gy,
  (unsigned char *)C_cx_secp256r1_n,
  (unsigned char *)C_cx_secp256r1_Hn,
  C_cx_secp256r1_h,
  (unsigned char *)C_cx_secp256r1_a,
  (unsigned char *)C_cx_secp256r1_b,
};

static uint8_t const C_cx_secp384r1_a[] = {
  // a:
  // 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFC
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xfc
};
static uint8_t const C_cx_secp384r1_b[] = {
  // b:
  0xB3, 0x31, 0x2F, 0xA7, 0xE2, 0x3E, 0xE7, 0xE4, 0x98, 0x8E, 0x05, 0x6B,
  0xE3, 0xF8, 0x2D, 0x19, 0x18, 0x1D, 0x9C, 0x6E, 0xFE, 0x81, 0x41, 0x12,
  0x03, 0x14, 0x08, 0x8F, 0x50, 0x13, 0x87, 0x5A, 0xC6, 0x56, 0x39, 0x8D,
  0x8A, 0x2E, 0xD1, 0x9D, 0x2A, 0x85, 0xC8, 0xED, 0xD3, 0xEC, 0x2A, 0xEF
};
static uint8_t const C_cx_secp384r1_p[] = {
  // p:
  // 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFFFF0000000000000000FFFFFFFF
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
};
static uint8_t const C_cx_secp384r1_Hp[] = {
  // Hp:
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00,
  0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xfe, 0x00, 0x00, 0x00, 0x01
};
static uint8_t const C_cx_secp384r1_Gx[] = {
  0xAA, 0x87, 0xCA, 0x22, 0xBE, 0x8B, 0x05, 0x37, 0x8E, 0xB1, 0xC7, 0x1E,
  0xF3, 0x20, 0xAD, 0x74, 0x6E, 0x1D, 0x3B, 0x62, 0x8B, 0xA7, 0x9B, 0x98,
  0x59, 0xF7, 0x41, 0xE0, 0x82, 0x54, 0x2A, 0x38, 0x55, 0x02, 0xF2, 0x5D,
  0xBF, 0x55, 0x29, 0x6C, 0x3A, 0x54, 0x5E, 0x38, 0x72, 0x76, 0x0A, 0xB7
};
static uint8_t const C_cx_secp384r1_Gy[] = {
  0x36, 0x17, 0xDE, 0x4A, 0x96, 0x26, 0x2C, 0x6F, 0x5D, 0x9E, 0x98, 0xBF,
  0x92, 0x92, 0xDC, 0x29, 0xF8, 0xF4, 0x1D, 0xBD, 0x28, 0x9A, 0x14, 0x7C,
  0xE9, 0xDA, 0x31, 0x13, 0xB5, 0xF0, 0xB8, 0xC0, 0x0A, 0x60, 0xB1, 0xCE,
  0x1D, 0x7E, 0x81, 0x9D, 0x7A, 0x43, 0x1D, 0x7C, 0x90, 0xEA, 0x0E, 0x5F
};
static uint8_t const C_cx_secp384r1_n[] = {
  // n:
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xC7, 0x63, 0x4D, 0x81, 0xF4, 0x37, 0x2D, 0xDF, 0x58, 0x1A, 0x0D, 0xB2,
  0x48, 0xB0, 0xA7, 0x7A, 0xEC, 0xEC, 0x19, 0x6A, 0xCC, 0xC5, 0x29, 0x73
};
static uint8_t const C_cx_secp384r1_Hn[] = {
  // Hn:
  0x0c, 0x84, 0xee, 0x01, 0x2b, 0x39, 0xbf, 0x21, 0x3f, 0xb0, 0x5b, 0x7a,
  0x28, 0x26, 0x68, 0x95, 0xd4, 0x0d, 0x49, 0x17, 0x4a, 0xab, 0x1c, 0xc5,
  0xbc, 0x3e, 0x48, 0x3a, 0xfc, 0xb8, 0x29, 0x47, 0xff, 0x3d, 0x81, 0xe5,
  0xdf, 0x1a, 0xa4, 0x19, 0x2d, 0x31, 0x9b, 0x24, 0x19, 0xb4, 0x09, 0xa9
};

#define C_cx_secp384r1_h 1

static cx_curve_weierstrass_t const C_cx_secp384r1 = {
  CX_CURVE_SECP384R1,
  384,
  48,
  (unsigned char *)C_cx_secp384r1_p,
  (unsigned char *)C_cx_secp384r1_Hp,
  (unsigned char *)C_cx_secp384r1_Gx,
  (unsigned char *)C_cx_secp384r1_Gy,
  (unsigned char *)C_cx_secp384r1_n,
  (unsigned char *)C_cx_secp384r1_Hn,
  C_cx_secp384r1_h,
  (unsigned char *)C_cx_secp384r1_a,
  (unsigned char *)C_cx_secp384r1_b,
};

#define BLS12_381_SIZE_u8 48
static uint8_t const C_cx_BLS12_381_G1_a[BLS12_381_SIZE_u8] = { 0 };
static uint8_t const C_cx_BLS12_381_G1_b[BLS12_381_SIZE_u8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04
};
static uint8_t const C_cx_BLS12_381_G1_p[BLS12_381_SIZE_u8] = {
  0x1a, 0x01, 0x11, 0xea, 0x39, 0x7f, 0xe6, 0x9a, 0x4b, 0x1b, 0xa7, 0xb6,
  0x43, 0x4b, 0xac, 0xd7, 0x64, 0x77, 0x4b, 0x84, 0xf3, 0x85, 0x12, 0xbf,
  0x67, 0x30, 0xd2, 0xa0, 0xf6, 0xb0, 0xf6, 0x24, 0x1e, 0xab, 0xff, 0xfe,
  0xb1, 0x53, 0xff, 0xff, 0xb9, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xaa, 0xab
};
static uint8_t const C_cx_BLS12_381_G1_Hp[BLS12_381_SIZE_u8] = {
  0x11, 0x98, 0x8f, 0xe5, 0x92, 0xca, 0xe3, 0xaa, 0x9a, 0x79, 0x3e, 0x85,
  0xb5, 0x19, 0x95, 0x2d, 0x67, 0xeb, 0x88, 0xa9, 0x93, 0x9d, 0x83, 0xc0,
  0x8d, 0xe5, 0x47, 0x6c, 0x4c, 0x95, 0xb6, 0xd5, 0x0a, 0x76, 0xe6, 0xa6,
  0x09, 0xd1, 0x04, 0xf1, 0xf4, 0xdf, 0x1f, 0x34, 0x1c, 0x34, 0x17, 0x46
};
static uint8_t const C_cx_BLS12_381_G1_n[BLS12_381_SIZE_u8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x73, 0xed, 0xa7, 0x53, 0x29, 0x9d, 0x7d, 0x48,
  0x33, 0x39, 0xd8, 0x08, 0x09, 0xa1, 0xd8, 0x05, 0x53, 0xbd, 0xa4, 0x02,
  0xff, 0xfe, 0x5b, 0xfe, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x01
};
static uint8_t const C_cx_BLS12_381_G1_Hn[BLS12_381_SIZE_u8] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x6e, 0x2a, 0x5b, 0xb9, 0xc8, 0xdb, 0x33, 0xe9,
  0x73, 0xd1, 0x3c, 0x71, 0xc7, 0xb5, 0xf4, 0x18, 0x1b, 0x3e, 0x0d, 0x18,
  0x8c, 0xf0, 0x69, 0x90, 0xc6, 0x2c, 0x18, 0x07, 0x43, 0x9b, 0x73, 0xaf
};
static uint8_t const C_cx_BLS12_381_G1_Gx[BLS12_381_SIZE_u8] = {
  0x17, 0xf1, 0xd3, 0xa7, 0x31, 0x97, 0xd7, 0x94, 0x26, 0x95, 0x63, 0x8c,
  0x4f, 0xa9, 0xac, 0x0f, 0xc3, 0x68, 0x8c, 0x4f, 0x97, 0x74, 0xb9, 0x05,
  0xa1, 0x4e, 0x3a, 0x3f, 0x17, 0x1b, 0xac, 0x58, 0x6c, 0x55, 0xe8, 0x3f,
  0xf9, 0x7a, 0x1a, 0xef, 0xfb, 0x3a, 0xf0, 0x0a, 0xdb, 0x22, 0xc6, 0xbb
};
static uint8_t const C_cx_BLS12_381_G1_Gy[BLS12_381_SIZE_u8] = {
  0x08, 0xb3, 0xf4, 0x81, 0xe3, 0xaa, 0xa0, 0xf1, 0xa0, 0x9e, 0x30, 0xed,
  0x74, 0x1d, 0x8a, 0xe4, 0xfc, 0xf5, 0xe0, 0x95, 0xd5, 0xd0, 0x0a, 0xf6,
  0x00, 0xdb, 0x18, 0xcb, 0x2c, 0x04, 0xb3, 0xed, 0xd0, 0x3c, 0xc7, 0x44,
  0xa2, 0x88, 0x8a, 0xe4, 0x0c, 0xaa, 0x23, 0x29, 0x46, 0xc5, 0xe7, 0xe1
};

// The cofactor of BLS12-381 0x396c8c005555e1568c00aaab0000aaab
// does not fit.
#define C_cx_BLS12_381_G1_h 1
cx_curve_weierstrass_t const C_cx_BLS12_381_G1 = {
  CX_CURVE_BLS12_381_G1,
  381,
  48,
  (unsigned char *)C_cx_BLS12_381_G1_p,
  (unsigned char *)C_cx_BLS12_381_G1_Hp,
  (unsigned char *)C_cx_BLS12_381_G1_Gx,
  (unsigned char *)C_cx_BLS12_381_G1_Gy,
  (unsigned char *)C_cx_BLS12_381_G1_n,
  (unsigned char *)C_cx_BLS12_381_G1_Hn,
  C_cx_BLS12_381_G1_h,
  (unsigned char *)C_cx_BLS12_381_G1_a,
  (unsigned char *)C_cx_BLS12_381_G1_b
};

static unsigned char const C_cx_Ed25519_a[] = {
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xec
};
static unsigned char const C_cx_Ed25519_d[] = {
  // d:  0x52036cee2b6ffe738cc740797779e89800700a4d4141d8ab75eb4dca135978a3
  0x52, 0x03, 0x6c, 0xee, 0x2b, 0x6f, 0xfe, 0x73, 0x8c, 0xc7, 0x40,
  0x79, 0x77, 0x79, 0xe8, 0x98, 0x00, 0x70, 0x0a, 0x4d, 0x41, 0x41,
  0xd8, 0xab, 0x75, 0xeb, 0x4d, 0xca, 0x13, 0x59, 0x78, 0xa3
};
static unsigned char const C_cx_Ed25519_q[] = {
  // q:  0x7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffed
  0x7f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xed
};
static unsigned char const C_cx_Ed25519_Hq[] = {
  // Hq: 0x00000000000000000000000000000000000000000000000000000000000005a4
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xa4
};
static unsigned char const C_cx_Ed25519_Bx[] = {
  0x21, 0x69, 0x36, 0xd3, 0xcd, 0x6e, 0x53, 0xfe, 0xc0, 0xa4, 0xe2,
  0x31, 0xfd, 0xd6, 0xdc, 0x5c, 0x69, 0x2c, 0xc7, 0x60, 0x95, 0x25,
  0xa7, 0xb2, 0xc9, 0x56, 0x2d, 0x60, 0x8f, 0x25, 0xd5, 0x1a
};
static unsigned char const C_cx_Ed25519_By[] = {
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x58
};
static unsigned char const C_cx_Ed25519_l[] = {
  // l:  0x1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED
  0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0xDE, 0xF9, 0xDE, 0xA2, 0xF7,
  0x9C, 0xD6, 0x58, 0x12, 0x63, 0x1A, 0x5C, 0xF5, 0xD3, 0xED
};
static unsigned char const C_cx_Ed25519_Hl[] = {
  // Hl: 0x0399411b7c309a3dceec73d217f5be65d00e1ba768859347a40611e3449c0f01
  0x03, 0x99, 0x41, 0x1b, 0x7c, 0x30, 0x9a, 0x3d, 0xce, 0xec, 0x73,
  0xd2, 0x17, 0xf5, 0xbe, 0x65, 0xd0, 0x0e, 0x1b, 0xa7, 0x68, 0x85,
  0x93, 0x47, 0xa4, 0x06, 0x11, 0xe3, 0x44, 0x9c, 0x0f, 0x01
};
static unsigned char const C_cx_Ed25519_I[] = {
  // I: 0x2b8324804fc1df0b2b4d00993dfbd7a72f431806ad2fe478c4ee1b274a0ea0b0
  0x2b, 0x83, 0x24, 0x80, 0x4f, 0xc1, 0xdf, 0x0b, 0x2b, 0x4d, 0x00,
  0x99, 0x3d, 0xfb, 0xd7, 0xa7, 0x2f, 0x43, 0x18, 0x06, 0xad, 0x2f,
  0xe4, 0x78, 0xc4, 0xee, 0x1b, 0x27, 0x4a, 0x0e, 0xa0, 0xb0
};
static unsigned char const C_cx_Ed25519_Qplus3div8[] = {
  // q3: 0x0ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffe
  0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe
};

#define C_cx_Ed25519_h 8

static cx_curve_twisted_edward_t const C_cx_Ed25519 = {
  CX_CURVE_Ed25519,
  256,
  32,
  (unsigned char *)C_cx_Ed25519_q,
  (unsigned char *)C_cx_Ed25519_Hq,
  (unsigned char *)C_cx_Ed25519_Bx,
  (unsigned char *)C_cx_Ed25519_By,
  (unsigned char *)C_cx_Ed25519_l,
  (unsigned char *)C_cx_Ed25519_Hl,
  C_cx_Ed25519_h,
  (unsigned char *)C_cx_Ed25519_a,
  (unsigned char *)C_cx_Ed25519_d,
  (unsigned char *)C_cx_Ed25519_I,
  (unsigned char *)C_cx_Ed25519_Qplus3div8,
};

static cx_curve_domain_t const *const C_cx_allCurves[] = {
  (const cx_curve_domain_t *)&C_cx_secp256k1,
  (const cx_curve_domain_t *)&C_cx_secp256r1,
  (const cx_curve_domain_t *)&C_cx_secp384r1,
  (const cx_curve_domain_t *)&C_cx_Ed25519,
  (const cx_curve_domain_t *)&C_cx_BLS12_381_G1,
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
#if 0
  case CX_CURVE_SECP521R1:
    nid = NID_secp521r1;
    break;
  case CX_CURVE_BrainPoolP256R1:
    nid = NID_brainpoolP256r1;
    break;
  case CX_CURVE_BrainPoolP320R1:
    nid = NID_brainpoolP320r1;
    break;
  case CX_CURVE_BrainPoolP384R1:
    nid = NID_brainpoolP384r1;
    break;
  case CX_CURVE_BrainPoolP512R1:
    nid = NID_brainpoolP512r1;
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
    errx(1, "cx_eddsa_get_public_key: invalid key size (0x%x)", pv_key->d_len);
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

int sys_cx_ecfp_generate_pair2(cx_curve_t curve,
                               cx_ecfp_public_key_t *public_key,
                               cx_ecfp_private_key_t *private_key,
                               int keep_private, cx_md_t hashID)
{
  const EC_GROUP *group;
  EC_POINT *pub;
  BN_CTX *ctx;
  EC_KEY *key;
  BIGNUM *bn;
  int nid;

  if (curve == CX_CURVE_Ed25519) {
    return sys_cx_eddsa_get_public_key(private_key, hashID, public_key);
  } else {
    nid = nid_from_curve(curve);
    key = EC_KEY_new_by_curve_name(nid);
    if (key == NULL) {
      errx(1, "ssl: EC_KEY_new_by_curve_name");
    }

    group = EC_KEY_get0_group(key);

    ctx = BN_CTX_new();

    if (!keep_private) {
      if (EC_KEY_generate_key(key) == 0) {
        errx(1, "ssl: EC_KEY_generate_key");
      }

      bn = (BIGNUM *)EC_KEY_get0_private_key(key);
      if (BN_num_bytes(bn) > (int)sizeof(private_key->d)) {
        errx(1, "ssl: invalid bn");
      }
      private_key->curve = curve;
      private_key->d_len = BN_bn2bin(bn, private_key->d);
    } else {
      BIGNUM *priv;

      priv = BN_new();
      BN_bin2bn(private_key->d, private_key->d_len, priv);
      if (EC_KEY_set_private_key(key, priv) == 0) {
        errx(1, "ssl: EC_KEY_set_private_key");
      }

      pub = EC_POINT_new(group);
      if (EC_POINT_mul(group, pub, priv, NULL, NULL, ctx) == 0) {
        errx(1, "ssl: EC_POINT_mul");
      }

      if (EC_KEY_set_public_key(key, pub) == 0) {
        errx(1, "ssl: EC_KEY_set_public_key");
      }

      BN_free(priv);
      EC_POINT_free(pub);
    }

    bn = BN_new();
    pub = (EC_POINT *)EC_KEY_get0_public_key(key);
    EC_POINT_point2bn(group, pub, POINT_CONVERSION_UNCOMPRESSED, bn, ctx);
    if (BN_num_bytes(bn) > (int)sizeof(public_key->W)) {
      errx(1, "ssl: invalid bn");
    }
    public_key->curve = curve;
    public_key->W_len = BN_bn2bin(bn, public_key->W);

    BN_free(bn);
    EC_KEY_free(key);
    BN_CTX_free(ctx);
  }
  return 0;
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

int cx_ecfp_decode_sig_der(const uint8_t *input, size_t input_len,
                           size_t max_size, const uint8_t **r, size_t *r_len,
                           const uint8_t **s, size_t *s_len)
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
  cx_curve_weierstrass_t *domain;
  unsigned int size;
  const uint8_t *r, *s;
  size_t rlen, slen;
  int nid = 0;

  domain = (cx_curve_weierstrass_t *)cx_ecfp_get_domain(key->curve);
  size = domain->length; // bits  -> bytes

  if (!cx_ecfp_decode_sig_der(sig, sig_len, size, &r, &rlen, &s, &slen)) {
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

  nid = nid_from_curve(key->curve);
  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
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

int cx_ecfp_encode_sig_der(unsigned char *sig, unsigned int sig_len,
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

  const cx_curve_domain_t *domain = cx_ecfp_get_domain(key->curve);
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
    hash_info = cx_hash_get_info(hashID);
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
  BN_CTX *bn_ctx = BN_CTX_new();
  BIGNUM *q = BN_new();
  BIGNUM *x = BN_new();
  BIGNUM *k = BN_new();
  BIGNUM *kinv = BN_new();
  BIGNUM *rp = BN_new();
  BIGNUM *kg_y = BN_new();

  BN_bin2bn(domain->n, domain->length, q);
  BN_bin2bn(key->d, key->d_len, x);
  EC_KEY *ec_key = EC_KEY_new_by_curve_name(nid);
  EC_KEY_set_private_key(ec_key, x);

  const EC_GROUP *group = EC_KEY_get0_group(ec_key);
  EC_POINT *kg = EC_POINT_new(group);
  ECDSA_SIG *ecdsa_sig;

  if (do_rfc6979_signature) {
    if (domain->length >= CX_RFC6979_MAX_RLEN) {
      errx(1, "rfc6979_ecdsa_sign: curve too large");
    }
    cx_rng_rfc6979_init(&rfc_ctx, hashID, key->d, key->d_len, hash, hash_len,
                        domain->n, domain->length);
  }
  do {
    if (do_rfc6979_signature) {
      cx_rng_rfc6979_next(&rfc_ctx, rnd, domain->length);
      BN_bin2bn(rnd, domain->length, k);
    } else {
      BN_rand(k, domain->length, 0, 0);
    }
    if (!EC_POINT_mul(group, kg, k, NULL, NULL, bn_ctx)) {
      errx(1, "ssl: EC_POINT_mul");
    }
    if (!EC_POINT_get_affine_coordinates(group, kg, rp, kg_y, bn_ctx)) {
      errx(1, "ssl: EC_POINT_get_affine_coordinates");
    }
    if (BN_cmp(rp, q) >= 0) {
      if (!BN_nnmod(rp, rp, q, bn_ctx)) {
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

    BN_mod_inverse(kinv, k, q, bn_ctx);
    ecdsa_sig = ECDSA_do_sign_ex(hash, hash_len, kinv, rp, ec_key);
  } while (ecdsa_sig == NULL);
  BN_CTX_free(bn_ctx);
  BN_free(q);
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
  int ret = cx_ecfp_encode_sig_der(sig, sig_len, buf_r, domain->length, buf_s,
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
  xLen = BN_bn2binpad(x, pout + 1, 32);
  if (xLen < 0) {
    ECerr(EC_F_ECDH_SIMPLE_COMPUTE_KEY, ERR_R_BN_LIB);
    goto err;
  }
  yLen = BN_bn2binpad(y, pout + 1 + xLen, 32);
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
      errx(1, "cx_ecfp_add_point: invalid X_len (%u)", X_len);
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
