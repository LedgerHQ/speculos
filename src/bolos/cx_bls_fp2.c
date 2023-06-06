/**
 * @file cx_bls_fp2.c
 * @brief GF(p²) operations for BLS12-381.
 *        This file is  based on BLST API and implements the
 *        missing GF(p²) arithmetic operations and helpers.
 * @date 2023-05-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "cx_bls.h"
#include <blst.h>
#include <blst_aux.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static uint8_t FP2_ONE[2][48] = {
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
  { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static uint8_t SQRT_RATIO_C3[] = {
  0x2a, 0x43, 0x7a, 0X4b, 0x8c, 0x35, 0xfc, 0x74, 0xbd, 0x27, 0x8e, 0xaa,
  0x22, 0xf2, 0x5e, 0x9e, 0x2d, 0xc9, 0x0e, 0x50, 0xe7, 0x04, 0x6b, 0x46,
  0x6e, 0x59, 0xe4, 0x93, 0x49, 0xe8, 0xbd, 0x05, 0x0a, 0x62, 0xcf, 0xd1,
  0x6d, 0xdc, 0xa6, 0xef, 0x53, 0x14, 0x93, 0x30, 0x97, 0x8e, 0xf0, 0x11,
  0xd6, 0x86, 0x19, 0xc8, 0x61, 0x85, 0xc7, 0xb2, 0x92, 0xe8, 0x5a, 0x87,
  0x09, 0x1a, 0x04, 0x96, 0x6b, 0xf9, 0x1e, 0xd3, 0xe7, 0x1b, 0x74, 0x31,
  0x62, 0xc3, 0x38, 0x36, 0x21, 0x13, 0xcf, 0xd7, 0xce, 0xd6, 0xb1, 0xd7,
  0x63, 0x82, 0xea, 0xb2, 0x6a, 0xa0, 0x00, 0x01, 0xc7, 0x18, 0xe3
};

static uint8_t SQRT_RATIO_C4[] = { 0x07 };

static uint8_t SQRT_RATIO_C5[] = { 0x04 };

static uint8_t SQRT_RATIO_C6[48] = {
  0x06, 0xaf, 0x0e, 0x04, 0x37, 0xff, 0x40, 0x0b, 0x68, 0x31, 0xe3, 0x6d,
  0x6b, 0xd1, 0x7f, 0xfe, 0x48, 0x39, 0x5d, 0xab, 0xc2, 0xd3, 0x43, 0x5e,
  0x77, 0xf7, 0x6e, 0x17, 0x00, 0x92, 0x41, 0xc5, 0xee, 0x67, 0x99, 0x2f,
  0x72, 0xec, 0x05, 0xf4, 0xc8, 0x10, 0x84, 0xfb, 0xed, 0xe3, 0xcc, 0x09
};

static uint8_t SQRT_RATIO_C7[2][48] = {
  { 0x13, 0xdc, 0x09, 0x69, 0x31, 0x1e, 0x2b, 0xa5, 0x65, 0x92, 0x4c, 0xb0,
    0xb6, 0xf7, 0xbb, 0x98, 0x57, 0xf1, 0x57, 0xe1, 0x7f, 0x0c, 0x8d, 0xb4,
    0xe4, 0x84, 0xfc, 0xb2, 0x7b, 0x8b, 0xe0, 0xb3, 0x6d, 0xfa, 0x03, 0x40,
    0xc4, 0x22, 0xfb, 0x7e, 0xfe, 0x9d, 0x9a, 0x32, 0x34, 0x33, 0x6d, 0x5e },
  { 0x07, 0x1d, 0x42, 0xac, 0x9c, 0x54, 0x00, 0x1a, 0x21, 0xac, 0xf9, 0x18,
    0x7d, 0x46, 0x9d, 0x91, 0x9a, 0x83, 0x0a, 0x2c, 0x96, 0x91, 0x28, 0xd2,
    0x26, 0x59, 0xdc, 0x2f, 0x82, 0x63, 0xf1, 0xca, 0x73, 0xc5, 0xb0, 0xe0,
    0x2c, 0x05, 0xec, 0x38, 0x1b, 0x86, 0x84, 0xa6, 0x76, 0xa8, 0x13, 0x81 }
};

static bool cx_bls_fp2_is_one(blst_fp2 *a)
{
  uint8_t buffer[96];
  cx_bls_bendian_from_fp2(buffer, buffer + 48, a);
  if ((0 == memcmp(buffer, FP2_ONE[0], sizeof(FP2_ONE[0]))) &&
      (0 == memcmp(buffer + 48, FP2_ONE[1], sizeof(FP2_ONE[1])))) {
    return true;
  }
  return false;
}

static bool cx_bls_fp_is_zero(blst_fp *a)
{
  uint8_t buffer[48];

  bool res = true;
  blst_bendian_from_fp(buffer, a);
  for (size_t i = 0; i < sizeof(buffer); i++) {
    res &= (buffer[i] == 0);
  }

  return res;
}

static size_t cx_get_bit_length(uint8_t *buffer, size_t buffer_len)
{
  size_t bit_len = buffer_len * 8;
  uint8_t mask;

  for (size_t i = 0; i < buffer_len; i++) {
    mask = 0x80;
    if (buffer[i] & mask) {
      break;
    }
    bit_len--;
    mask >>= 1;
  }
  return bit_len;
}

void cx_bls_fp2_from_bendian(blst_fp2 *ret, uint8_t c0[48], uint8_t c1[48])
{
  blst_fp_from_bendian(&ret->fp[0], c0);
  blst_fp_from_bendian(&ret->fp[1], c1);
}

void cx_bls_bendian_from_fp2(uint8_t c0[48], uint8_t c1[48], blst_fp2 *a)
{
  blst_bendian_from_fp(c0, &a->fp[0]);
  blst_bendian_from_fp(c1, &a->fp[1]);
}

void cx_bls_fp2_copy(blst_fp2 *ret, blst_fp2 *a)
{
  memcpy(ret, a, sizeof(blst_fp2));
}

void cx_bls_fp2_set_one(blst_fp2 *a)
{
  cx_bls_fp2_from_bendian(a, FP2_ONE[0], FP2_ONE[1]);
}

bool cx_bls_fp2_is_zero(blst_fp2 *a)
{
  uint8_t buffer[96];

  bool res = true;
  cx_bls_bendian_from_fp2(buffer, buffer + 48, a);
  for (size_t i = 0; i < sizeof(buffer); i++) {
    res &= (buffer[i] == 0);
  }

  return res;
}

void cx_bls_fp2_conditional_move(blst_fp2 *ret, blst_fp2 *a, blst_fp2 *b,
                                 bool choice)
{
  // No need for a constant time implementation
  if (choice) {
    cx_bls_fp2_copy(ret, b);
  } else {
    cx_bls_fp2_copy(ret, a);
  }
}

uint8_t cx_bls_fp2_sgn0(blst_fp2 *a)
{
  uint8_t sign_0, zero_0, sign_1;
  uint8_t s;
  uint8_t a0[48], a1[48];

  cx_bls_bendian_from_fp2(a0, a1, a);
  sign_0 = a0[47] & 1;
  zero_0 = (uint8_t)cx_bls_fp_is_zero(&a->fp[0]);
  sign_1 = a1[47] & 1;

  s = sign_0 | (zero_0 & sign_1);

  return s;
}

void cx_bls_fp2_frobenius(blst_fp2 *ret, blst_fp2 *a)
{
  memcpy(&ret->fp[0], &a->fp[0], sizeof(blst_fp));
  blst_fp_cneg(&ret->fp[1], &a->fp[1], true);
}

void cx_bls_fp2_pow(blst_fp2 *ret, blst_fp2 *a, uint8_t *exp, size_t exp_len)
{
  blst_fp2 tmp, base;
  int bit_len;
  uint8_t byte_offset = exp_len - 1;

  cx_bls_fp2_from_bendian(ret, FP2_ONE[0], FP2_ONE[1]);
  cx_bls_fp2_copy(&base, a);
  bit_len = cx_get_bit_length(exp, exp_len);

  while (bit_len > 0) {
    for (size_t bit = 0; bit < 8; bit++) {
      if ((exp[byte_offset] >> bit) & 1) {
        blst_fp2_mul(&tmp, ret, &base);
        cx_bls_fp2_copy(ret, &tmp);
      }
      cx_bls_fp2_copy(&tmp, &base);
      blst_fp2_sqr(&base, &tmp);
      bit_len--;
    }
    byte_offset--;
  }
}

/* Returns true and ret = sqrt(u/v) if (u/v) is square in GF(p²)
 *  Otherwise returns false and ret = sqrt(Z * (u/v))
 *  https://datatracker.ietf.org/doc/html/draft-irtf-cfrg-hash-to-curve-16#name-sqrt_ratio-subroutines
 */
bool cx_bls_fp2_sqrt_ratio(blst_fp2 *ret, blst_fp2 *u, blst_fp2 *v)
{
  blst_fp2 tv1, tv2, tv3, tv3_tmp, tv4, tv4_tmp, tv5, tv5_tmp, c7;
  bool is_qr, e1;

  cx_bls_fp2_from_bendian(&tv1, SQRT_RATIO_C6, SQRT_RATIO_C6);
  cx_bls_fp2_pow(&tv2, v, SQRT_RATIO_C4, sizeof(SQRT_RATIO_C4));
  blst_fp2_sqr(&tv3_tmp, &tv2);
  blst_fp2_mul(&tv3, v, &tv3_tmp);
  blst_fp2_mul(&tv5, u, &tv3);
  cx_bls_fp2_pow(&tv5_tmp, &tv5, SQRT_RATIO_C3, sizeof(SQRT_RATIO_C3));
  blst_fp2_mul(&tv5, &tv5_tmp, &tv2);
  blst_fp2_mul(&tv2, &tv5, v);
  blst_fp2_mul(&tv3, &tv5, u);
  blst_fp2_mul(&tv4, &tv3, &tv2);
  cx_bls_fp2_pow(&tv5, &tv4, SQRT_RATIO_C5, sizeof(SQRT_RATIO_C5));
  is_qr = cx_bls_fp2_is_one(&tv5);
  cx_bls_fp2_from_bendian(&c7, SQRT_RATIO_C7[0], SQRT_RATIO_C7[1]);
  blst_fp2_mul(&tv2, &tv3, &c7);
  blst_fp2_mul(&tv5, &tv4, &tv1);
  cx_bls_fp2_copy(&tv3_tmp, &tv3);
  cx_bls_fp2_conditional_move(&tv3, &tv2, &tv3_tmp, is_qr);
  cx_bls_fp2_copy(&tv4_tmp, &tv4);
  cx_bls_fp2_conditional_move(&tv4, &tv5, &tv4_tmp, is_qr);
  blst_fp2_sqr(&tv5, &tv4);
  e1 = cx_bls_fp2_is_one(&tv5);
  blst_fp2_mul(&tv2, &tv3, &tv1);
  cx_bls_fp2_copy(&tv4_tmp, &tv1);
  blst_fp2_sqr(&tv1, &tv4_tmp);
  blst_fp2_mul(&tv5, &tv4, &tv1);
  cx_bls_fp2_copy(&tv4_tmp, &tv3);
  cx_bls_fp2_conditional_move(&tv3, &tv2, &tv4_tmp, e1);
  cx_bls_fp2_copy(&tv4_tmp, &tv4);
  cx_bls_fp2_conditional_move(&tv4, &tv5, &tv4_tmp, e1);
  e1 = cx_bls_fp2_is_one(&tv4);
  blst_fp2_mul(&tv2, &tv3, &tv1);
  cx_bls_fp2_conditional_move(ret, &tv2, &tv3, e1);
  // No need to calculate tv4 = CMOV(tv5, tv4, e1)
  // as only tv3 is returned

  return is_qr;
}
