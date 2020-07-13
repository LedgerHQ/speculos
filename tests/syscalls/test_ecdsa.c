#include <malloc.h>
#include <setjmp.h>
#include <stdbool.h>
#include <string.h>

#include <cmocka.h>

#include "cx.h"
#include "cx_ec.h"
#include "cx_hash.h"
#include "emulate.h"
#include "utils.h"

#define cx_ecfp_init_private_key sys_cx_ecfp_init_private_key
#define cx_ecfp_generate_pair sys_cx_ecfp_generate_pair
#define cx_ecdsa_sign sys_cx_ecdsa_sign
#define cx_ecdsa_verify sys_cx_ecdsa_verify

typedef struct {
  const char *secret_key;
  const char *public_key;
  const char *hash_to_sign;
} ecdsa_test_vector;

const ecdsa_test_vector secp256k1_test_vector[] = {
  {"32FD0928F588847821BCA61BC5E566C8BF0061DAF91BB1AB6E6BA91E909E6B93",
    "04060896C5E7F38A73047AF1CC872134975FC73CE5A5A4BB87FCC8EB00D05BB5D"
    "D6938D08D6ED446B17BCCC7BBBD34A2339A60B4E01476B0F295FC6C72979CA082",
    "D2837A5C7D52BF9F472B16BD851D6C09579A80FE5E4FBF293A988C117EE90BB0"}
};

const ecdsa_test_vector secp256r1_test_vector[] = {
  {"1C8E56B389A574305550B419A93F7A8616DE4287C545C78B0A4A8C1D62B61173",
    "04F9BD34285B635892E3A4A0B32810F71473070AF329005E1D58EBF95B82F236E"
    "A1192E7F9B3F0A461C6ABFBEE51EE4D8517F8D47D0211C1C0623DACC7E9BF37BF",
    "D2837A5C7D52BF9F472B16BD851D6C09579A80FE5E4FBF293A988C117EE90BB0"}
};

// Use BLAKE2b-512('')
const ecdsa_test_vector secp256k1_blake2b512_test_vector[] = {
  {"32FD0928F588847821BCA61BC5E566C8BF0061DAF91BB1AB6E6BA91E909E6B93",
    "04060896C5E7F38A73047AF1CC872134975FC73CE5A5A4BB87FCC8EB00D05BB5D"
    "D6938D08D6ED446B17BCCC7BBBD34A2339A60B4E01476B0F295FC6C72979CA082",
    "786A02F742015903C6C6FD852552D272912F4740E15847618A86E217F71F5419D"
    "25E1031AFEE585313896444934EB04B903A685B1448B755D56F701AFE9BE2CE"}
};


void test_ecdsa(cx_curve_t curve, cx_md_t md, const ecdsa_test_vector* tv, size_t tv_len) {

  cx_ecfp_private_key_t privateKey;
  cx_ecfp_public_key_t publicKey;
  uint8_t secret_key[32];
  uint8_t public_key[65];
  uint8_t hash_to_sign[64];
  uint32_t info = 0;
  int sig_len;
  uint8_t signature[80];
  size_t hash_len;

  if (md == CX_SHA256) {
    hash_len = 32;
  } else if (md == CX_BLAKE2B) {
    hash_len = 64;
  } else {
    fail();
  }

  for (unsigned int i = 0; i < tv_len; i++) {
    assert_int_equal(hexstr2bin(tv[i].secret_key, secret_key, sizeof(secret_key)), sizeof(secret_key));
    assert_int_equal(hexstr2bin(tv[i].public_key, public_key, sizeof(public_key)), sizeof(public_key));
    assert_int_equal(hexstr2bin(tv[i].hash_to_sign, hash_to_sign, hash_len), hash_len);

    assert_int_equal(cx_ecfp_init_private_key(curve, secret_key, sizeof(secret_key), &privateKey), 32);
    assert_int_equal(cx_ecfp_generate_pair(curve, &publicKey, &privateKey, 1), 0);
    assert_memory_equal(publicKey.W, public_key, sizeof(public_key));
    
    /* Sign the digest using the previously derived keypair*/
    sig_len = cx_ecdsa_sign(&privateKey, CX_RND_TRNG | CX_LAST, md,
                            hash_to_sign, hash_len, signature,
                            sizeof(signature), &info);
    /* Verify the signature correctness against the public key */
    assert_int_equal(cx_ecdsa_verify(&publicKey, CX_LAST, md, hash_to_sign,
                                     hash_len, signature, sig_len),
                     1);
  }
};

static void test_ecdsa_secp256k1(void **state __attribute__((unused))) {
  test_ecdsa(CX_CURVE_256K1, CX_SHA256, secp256k1_test_vector, 
             sizeof(secp256k1_test_vector) / sizeof(secp256k1_test_vector[0]));
}

static void test_ecdsa_secp256r1(void **state __attribute__((unused))) {
  test_ecdsa(CX_CURVE_256R1, CX_SHA256, secp256r1_test_vector, 
  sizeof(secp256r1_test_vector) / sizeof(secp256r1_test_vector[0]));
}

static void test_blake2b_secp256k1(void **state __attribute__((unused))) {
  test_ecdsa(CX_CURVE_256K1, CX_BLAKE2B, secp256k1_blake2b512_test_vector,
             sizeof(secp256k1_blake2b512_test_vector) / sizeof(secp256k1_blake2b512_test_vector[0]));
}

int main() {
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_ecdsa_secp256k1),
      cmocka_unit_test(test_ecdsa_secp256r1),
      cmocka_unit_test(test_blake2b_secp256k1),
  };
  make_openssl_random_deterministic();
  return cmocka_run_group_tests(tests, NULL, NULL);
}
