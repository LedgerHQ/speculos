#include <malloc.h>
#include <setjmp.h>
#include <stdbool.h>
#include <string.h>

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/objects.h>

#include <cmocka.h>

#include "bolos/cx.h"
#include "bolos/cx_ec.h"
#include "bolos/cx_hash.h"
#include "emulate.h"
#include "utils.h"

#define cx_ecfp_init_private_key sys_cx_ecfp_init_private_key
#define cx_ecfp_generate_pair    sys_cx_ecfp_generate_pair
#define cx_ecdsa_sign            sys_cx_ecdsa_sign
#define cx_ecdsa_verify          sys_cx_ecdsa_verify

typedef struct {
  const char *secret_key;
  const char *public_key;
  const char *hash_to_sign;
} ecdsa_test_vector;

const ecdsa_test_vector secp256k1_test_vector[] = {
  { "32FD0928F588847821BCA61BC5E566C8BF0061DAF91BB1AB6E6BA91E909E6B93",
    "04060896C5E7F38A73047AF1CC872134975FC73CE5A5A4BB87FCC8EB00D05BB5D"
    "D6938D08D6ED446B17BCCC7BBBD34A2339A60B4E01476B0F295FC6C72979CA082",
    "D2837A5C7D52BF9F472B16BD851D6C09579A80FE5E4FBF293A988C117EE90BB0" }
};

const ecdsa_test_vector secp256r1_test_vector[] = {
  { "1C8E56B389A574305550B419A93F7A8616DE4287C545C78B0A4A8C1D62B61173",
    "04F9BD34285B635892E3A4A0B32810F71473070AF329005E1D58EBF95B82F236E"
    "A1192E7F9B3F0A461C6ABFBEE51EE4D8517F8D47D0211C1C0623DACC7E9BF37BF",
    "D2837A5C7D52BF9F472B16BD851D6C09579A80FE5E4FBF293A988C117EE90BB0" }
};

// Use BLAKE2b-512('')
const ecdsa_test_vector secp256k1_blake2b512_test_vector[] = {
  { "32FD0928F588847821BCA61BC5E566C8BF0061DAF91BB1AB6E6BA91E909E6B93",
    "04060896C5E7F38A73047AF1CC872134975FC73CE5A5A4BB87FCC8EB00D05BB5D"
    "D6938D08D6ED446B17BCCC7BBBD34A2339A60B4E01476B0F295FC6C72979CA082",
    "786A02F742015903C6C6FD852552D272912F4740E15847618A86E217F71F5419D"
    "25E1031AFEE585313896444934EB04B903A685B1448B755D56F701AFE9BE2CE" }
};

void test_ecdsa(cx_curve_t curve, cx_md_t md, const ecdsa_test_vector *tv,
                size_t tv_len)
{
  cx_ecfp_private_key_t privateKey;
  cx_ecfp_public_key_t publicKey;
  uint8_t secret_key[32];
  uint8_t public_key[65];
  uint8_t hash_to_sign[64];
  uint32_t info = 0;
  int sig_len;
  uint8_t signature[80];
  size_t hash_len;
  uint8_t r_len, s_len;
  BIGNUM *q, *pt_x, *sig_s, *inv_r, *z;
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(curve);
  int y_bit;
  EC_GROUP *group;
  EC_POINT *sig_pt, *pubkey;
  BN_CTX *ctx;
  uint8_t recovered_pubkey[80];
  size_t rec_pubkey_size;

  if (md == CX_SHA256) {
    hash_len = 32;
  } else if (md == CX_BLAKE2B) {
    hash_len = 64;
  } else {
    hash_len = 0;
    fail();
  }

  for (unsigned int i = 0; i < tv_len; i++) {
    assert_int_equal(
        hexstr2bin(tv[i].secret_key, secret_key, sizeof(secret_key)),
        sizeof(secret_key));
    assert_int_equal(
        hexstr2bin(tv[i].public_key, public_key, sizeof(public_key)),
        sizeof(public_key));
    assert_int_equal(hexstr2bin(tv[i].hash_to_sign, hash_to_sign, hash_len),
                     hash_len);

    assert_int_equal(cx_ecfp_init_private_key(curve, secret_key,
                                              sizeof(secret_key), &privateKey),
                     32);
    assert_int_equal(cx_ecfp_generate_pair(curve, &publicKey, &privateKey, 1),
                     0);
    assert_memory_equal(publicKey.W, public_key, sizeof(public_key));

    /* Sign the digest using the previously derived keypair*/
    sig_len =
        cx_ecdsa_sign(&privateKey, CX_RND_TRNG | CX_LAST, md, hash_to_sign,
                      hash_len, signature, sizeof(signature), &info);
    /* Verify the signature correctness against the public key */
    assert_int_equal(cx_ecdsa_verify(&publicKey, CX_LAST, md, hash_to_sign,
                                     hash_len, signature, sig_len),
                     1);

    /* Recover the public key from the hash and the signature
     * (used in Ethereum ECDSA recoverable signatures)
     */
    q = BN_new();
    pt_x = BN_new();
    sig_s = BN_new();
    inv_r = BN_new();
    z = BN_new();
    ctx = BN_CTX_new();
    assert_non_null(q);
    assert_non_null(pt_x);
    assert_non_null(sig_s);
    assert_non_null(inv_r);
    assert_non_null(z);
    assert_non_null(ctx);

    BN_bin2bn(domain->n, domain->length, q);

    assert_int_equal(signature[0], 0x30);
    assert_int_equal(signature[1], sig_len - 2);
    assert_int_equal(signature[2], 0x02);
    r_len = signature[3];
    assert_int_equal(signature[4 + r_len], 0x02);
    s_len = signature[5 + r_len];

    BN_bin2bn(signature + 4, r_len, pt_x);
    BN_bin2bn(signature + 6 + r_len, s_len, sig_s);

    BN_mod_inverse(inv_r, pt_x, q, ctx);

    if (info & CX_ECCINFO_xGTn) {
      BN_add(pt_x, pt_x, q);
    }

    y_bit = (info & CX_ECCINFO_PARITY_ODD) ? 1 : 0;

    switch (curve) {
    case CX_CURVE_SECP256K1:
      group = EC_GROUP_new_by_curve_name(NID_secp256k1);
      break;
    case CX_CURVE_SECP256R1:
      group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
      break;
    default:
      group = NULL;
      fail();
    }
    sig_pt = EC_POINT_new(group);
    pubkey = EC_POINT_new(group);
    EC_POINT_set_compressed_coordinates_GFp(group, sig_pt, pt_x, y_bit, ctx);

    /* pubkey = sig_pt * ((s * inv_r) % q) - generator * ((hash * inv_r) % q)
     * and EC_POINT_mul(group, result, n, pt, m, ctx) computes generator*n +
     * pt*m. Computes z = (- hash * inv_r) % q, and ((s * inv_r) % q) into sig_s
     */
    BN_bin2bn(hash_to_sign, hash_len, z);
    if (8 * hash_len > domain->bit_size) {
      BN_rshift(z, z, 8 * hash_len - domain->bit_size);
    }
    BN_mod_mul(z, z, inv_r, q, ctx);
    BN_mod_sub(z, q, z, q, ctx);
    BN_mod_mul(sig_s, sig_s, inv_r, q, ctx);
    EC_POINT_mul(group, pubkey, z, sig_pt, sig_s, ctx);
    rec_pubkey_size =
        EC_POINT_point2oct(group, pubkey, POINT_CONVERSION_UNCOMPRESSED,
                           recovered_pubkey, sizeof(recovered_pubkey), ctx);
    assert_int_equal(rec_pubkey_size, 65);
    assert_memory_equal(recovered_pubkey, public_key, sizeof(public_key));

    EC_POINT_free(sig_pt);
    EC_GROUP_free(group);
    BN_free(q);
    BN_free(pt_x);
    BN_free(sig_s);
    BN_free(inv_r);
    BN_free(z);
    BN_CTX_free(ctx);
  }
};

static void test_ecdsa_secp256k1(void **state __attribute__((unused)))
{
  test_ecdsa(CX_CURVE_256K1, CX_SHA256, secp256k1_test_vector,
             sizeof(secp256k1_test_vector) / sizeof(secp256k1_test_vector[0]));
}

static void test_ecdsa_secp256r1(void **state __attribute__((unused)))
{
  test_ecdsa(CX_CURVE_256R1, CX_SHA256, secp256r1_test_vector,
             sizeof(secp256r1_test_vector) / sizeof(secp256r1_test_vector[0]));
}

static void test_blake2b_secp256k1(void **state __attribute__((unused)))
{
  test_ecdsa(CX_CURVE_256K1, CX_BLAKE2B, secp256k1_blake2b512_test_vector,
             sizeof(secp256k1_blake2b512_test_vector) /
                 sizeof(secp256k1_blake2b512_test_vector[0]));
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_ecdsa_secp256k1),
    cmocka_unit_test(test_ecdsa_secp256r1),
    cmocka_unit_test(test_blake2b_secp256k1),
  };
  make_openssl_random_deterministic();
  return cmocka_run_group_tests(tests, NULL, NULL);
}
