#ifndef CX_BLS_H
#define CX_BLS_H

#include <blst.h>
#include <stdint.h>

#define CX_BLS_BLS12381_PARAM_LEN (48)
#define CX_BLS_BLS12381_KEY_LEN   (32)
#define BLS_FIELD_EXT_LEN         (CX_BLS_BLS12381_PARAM_LEN * 2)
#define BLS_COMPRESSED_SIG_LEN    (BLS_FIELD_EXT_LEN)
#define CX_BLS_PROVE              (0x9)
#define CX_BLS_BASE               (0x0)
#define CX_BLS_SIGN_AUG           (0xA)
#define CX_BLS_SIGN_POP           (0xF)

void cx_bls_fp2_from_bendian(blst_fp2 *ret, uint8_t c0[48], uint8_t c1[48]);
void cx_bls_bendian_from_fp2(uint8_t c0[48], uint8_t c1[48], blst_fp2 *a);

void cx_bls_fp2_conditional_move(blst_fp2 *ret, blst_fp2 *a, blst_fp2 *b,
                                 bool choice);
void cx_bls_fp2_copy(blst_fp2 *ret, blst_fp2 *a);
void cx_bls_fp2_set_one(blst_fp2 *a);
bool cx_bls_fp2_is_zero(blst_fp2 *a);
uint8_t cx_bls_fp2_sgn0(blst_fp2 *a);
void cx_bls_fp2_frobenius(blst_fp2 *ret, blst_fp2 *a);
void cx_bls_fp2_pow(blst_fp2 *ret, blst_fp2 *a, uint8_t *exp, size_t exp_len);
bool cx_bls_fp2_sqrt_ratio(blst_fp2 *ret, blst_fp2 *u, blst_fp2 *v);

void cx_bls_g2_psi(blst_p2_affine *ret_point, blst_p2_affine *point);
void cx_bls_g2_psi2(blst_p2_affine *ret_point, blst_p2_affine *point);
void cx_bls_g2_map_to_curve_simple_swu(blst_p2_affine *point, blst_fp2 *u);
void cx_bls_g2_iso_map_3(blst_p2_affine *ret_point, blst_p2_affine *point);
void cx_bls_g2_clear_cofactor(blst_p2_affine *ret_point, blst_p2_affine *point);
void cx_bls_g2_hash_field_to_curve(blst_p2_affine *point, uint8_t *hash,
                                   size_t hash_len);

#endif /* CX_BLS_H */
