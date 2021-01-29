#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "bolos/cx.h"
#include "bolos/cx_ec.h"
#include "utils.h"

static uint8_t const C_ED25519_G[] = {
  // uncompressed
  0x04,
  // x
  0x21, 0x69, 0x36, 0xd3, 0xcd, 0x6e, 0x53, 0xfe, 0xc0, 0xa4, 0xe2, 0x31, 0xfd,
  0xd6, 0xdc, 0x5c, 0x69, 0x2c, 0xc7, 0x60, 0x95, 0x25, 0xa7, 0xb2, 0xc9, 0x56,
  0x2d, 0x60, 0x8f, 0x25, 0xd5, 0x1a,
  // y
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
  0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
  0x66, 0x66, 0x66, 0x66, 0x66, 0x58
};

static uint8_t const C_Curve25519_G[] = {
  // compressed
  0x02,
  // x
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x09
};

typedef struct {
  const char *secret_key;
  const char *public_key;
  const char *msg;
  const char *signature;
} eddsa_test_vector;

/* clang-format off */
static const eddsa_test_vector rfc8032_test_vectors[] = {
  {"9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60",
   "d75a980182b10ab7d54bfed3c964073a0ee172f3daa62325af021a68f707511a", "",
   "e5564300c360ac729086e2cc806e828a84877f1eb8e5d974d873e065224901555fb8821590a33bacc61e39701cf9b46bd25bf5f0595bbe2"
   "4655141438e7a100b"},
  {"4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb",
   "3d4017c3e843895a92b70aa74d1b7ebc9c982ccf2ec4968cc0cd55f12af4660c", "72",
   "92a009a9f0d4cab8720e820b5f642540a2b27b5416503f8fb3762223ebdb69da085ac1e43e15996e458f3613d0f11d8c387b2eaeb4302ae"
   "eb00d291612bb0c00"},
  {"c5aa8df43f9f837bedb7442f31dcb7b166d38535076f094b85ce3a2e0b4458f7",
   "fc51cd8e6218a1a38da47ed00230f0580816ed13ba3303ac5deb911548908025", "af82",
   "6291d657deec24024827e69c3abe01a30ce548a284743a445e3680d7db5ac3ac18ff9b538d16f290ae67f760984dc6594a7c15e9716ed28"
   "dc027beceea1ec40a"},
  {"0d4a05b07352a5436e180356da0ae6efa0345ff7fb1572575772e8005ed978e9",
   "e61a185bcef2613a6c7cb79763ce945d3b245d76114dd440bcf5f2dc1aa57057", "cbc77b",
   "d9868d52c2bebce5f3fa5a79891970f309cb6591e3e1702a70276fa97c24b3a8e58606c38c9758529da50ee31b8219cba45271c689afa60b0"
   "ea26c99db19b00ccbc77b"},
  {"f5e5767cf153319517630f226876b86c8160cc583bc013744c6bf255f5cc0ee5",
   "278117fc144c72340f67d0f2316e8386ceffbf2b2428c9c51fef7c597f1d426e",
   "08b8b2b733424243760fe426a4b54908632110a66c2f6591eabd3345e3e4eb98fa6e264bf09efe12ee50f8f54e9f77b1e355f6c50544e23fb"
   "1433ddf73be84d879de7c0046dc4996d9e773f4bc9efe5738829adb26c81b37c93a1b270b20329d658675fc6ea534e0810a4432826bf58c94"
   "1efb65d57a338bbd2e26640f89ffbc1a858efcb8550ee3a5e1998bd177e93a7363c344fe6b199ee5d02e82d522c4feba15452f80288a821a5"
   "79116ec6dad2b3b310da903401aa62100ab5d1a36553e06203b33890cc9b832f79ef80560ccb9a39ce767967ed628c6ad573cb116dbefefd7"
   "5499da96bd68a8a97b928a8bbc103b6621fcde2beca1231d206be6cd9ec7aff6f6c94fcd7204ed3455c68c83f4a41da4af2b74ef5c53f1d8a"
   "c70bdcb7ed185ce81bd84359d44254d95629e9855a94a7c1958d1f8ada5d0532ed8a5aa3fb2d17ba70eb6248e594e1a2297acbbb39d502f1a"
   "8c6eb6f1ce22b3de1a1f40cc24554119a831a9aad6079cad88425de6bde1a9187ebb6092cf67bf2b13fd65f27088d78b7e883c8759d2c4f5c"
   "65adb7553878ad575f9fad878e80a0c9ba63bcbcc2732e69485bbc9c90bfbd62481d9089beccf80cfe2df16a2cf65bd92dd597b0707e0917a"
   "f48bbb75fed413d238f5555a7a569d80c3414a8d0859dc65a46128bab27af87a71314f318c782b23ebfe808b82b0ce26401d2e22f04d83d12"
   "55dc51addd3b75a2b1ae0784504df543af8969be3ea7082ff7fc9888c144da2af58429ec96031dbcad3dad9af0dcbaaaf268cb8fcffead94f"
   "3c7ca495e056a9b47acdb751fb73e666c6c655ade8297297d07ad1ba5e43f1bca32301651339e22904cc8c42f58c30c04aafdb038dda0847d"
   "d988dcda6f3bfd15c4b4c4525004aa06eeff8ca61783aacec57fb3d1f92b0fe2fd1a85f6724517b65e614ad6808d6f6ee34dff7310fdc82ae"
   "bfd904b01e1dc54b2927094b2db68d6f903b68401adebf5a7e08d78ff4ef5d63653a65040cf9bfd4aca7984a74d37145986780fc0b16ac451"
   "649de6188a7dbdf191f64b5fc5e2ab47b57f7f7276cd419c17a3ca8e1b939ae49e488acba6b965610b5480109c8b17b80e1b7b750dfc7598d"
   "5d5011fd2dcc5600a32ef5b52a1ecc820e308aa342721aac0943bf6686b64b2579376504ccc493d97e6aed3fb0f9cd71a43dd497f01f17c0e"
   "2cb3797aa2a2f256656168e6c496afc5fb93246f6b1116398a346f1a641f3b041e989f7914f90cc2c7fff357876e506b50d334ba77c225bc3"
   "07ba537152f3f1610e4eafe595f6d9d90d11faa933a15ef1369546868a7f3a45a96768d40fd9d03412c091c6315cf4fde7cb68606937380db"
   "2eaaa707b4c4185c32eddcdd306705e4dc1ffc872eeee475a64dfac86aba41c0618983f8741c5ef68d3a101e8a3b8cac60c905c15fc910840"
   "b94c00a0b9d0",
   "0aab4c900501b3e24d7cdf4663326a3a87df5e4843b2cbdb67cbf6e460fec350aa5371b1508f9f4528ecea23c436d94b5e8fcd4f681e30a6a"
   "c00a9704a188a03"},
  {"833fe62409237b9d62ec77587520911e9a759cec1d19755b7da901b96dca3d42",
   "ec172b93ad5e563bf4932c70e1245034c35467ef2efd4d64ebf819683467e2bf",
   "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2"
   "a9ac94fa54ca49f",
   "dc2a4459e7369633a52b1bf277839a00201009a3efbf3ecb69bea2186c26b58909351fc9ac90b3ecfdfbc7c66431e0303dca179c138ac17ad"
   "9bef1177331a704"}};
/* clang-format on */

void test_scalar_mult_ed25519(void **state __attribute__((unused)))
{
  uint8_t Pxy[65];
  uint8_t s[32];
  uint8_t expected[65];

  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x0f\x8a\xa5\xdb\x64\x07\x2f\x35\x91\xac\xf3\x1e\xc9\xd7\x2b\x62"
         "\xce\x9a\x31\xb8\xc2\xdd\x8e\x96\x73\xaf\x1a\x48\x2a\x86\x62\xef",
         sizeof(s));

  memcpy(expected,
         "\x04"
         "\x5f\xe2\x35\x09\xe2\x30\x63\x09\x53\xad\xcb\x5c\x4a\xdb\x97\x41"
         "\xbb\x53\x97\xbb\xf6\x29\xc0\xdb\x1f\x1d\x49\xf7\xcb\x2e\x0a\x19"
         "\x1b\x23\x68\xaf\x41\x3e\x50\x2f\x14\x21\x1e\x1d\x79\x68\xc6\x5e"
         "\x0d\x71\xa2\xe7\x24\x9b\x11\xc3\x5c\x33\xa9\x01\xe6\xb3\x78\x41",
         65);

  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));

  assert_memory_equal(Pxy, expected, sizeof(expected));

  // Verify that 1*G = G
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
         sizeof(s));
  memcpy(expected, C_ED25519_G, sizeof(Pxy));
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // Verify that order*G = (0,1)
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x14\xde\xf9\xde\xa2\xf7\x9c\xd6\x58\x12\x63\x1a\x5c\xf5\xd3\xed",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
         65);
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // Test vectors from https://tools.ietf.org/html/rfc8032#section-7.1
  // Pre-compute private = SHA512(secret), truncated, adjusted and in Big Endian
  // secret = 9d61b19deffd5a60ba844af492ec2cc44449c5697b326919703bac031cae7f60
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x4f\xe9\x4d\x90\x06\xf0\x20\xa5\xa3\xc0\x80\xd9\x68\x27\xff\xfd"
         "\x3c\x01\x0a\xc0\xf1\x2e\x7a\x42\xcb\x33\x28\x4f\x86\x83\x7c\x30",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x55\xd0\xe0\x9a\x2b\x9d\x34\x29\x22\x97\xe0\x8d\x60\xd0\xf6\x20"
         "\xc5\x13\xd4\x72\x53\x18\x7c\x24\xb1\x27\x86\xbd\x77\x76\x45\xce"
         "\x1a\x51\x07\xf7\x68\x1a\x02\xaf\x25\x23\xa6\xda\xf3\x72\xe1\x0e"
         "\x3a\x07\x64\xc9\xd3\xfe\x4b\xd5\xb7\x0a\xb1\x82\x01\x98\x5a\xd7",
         65);
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // secret = 4ccd089b28ff96da9db6c346ec114e0f5b8a319f35aba624da8cf6ed4fb8a6fb
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x51\x2e\x50\x2e\xb0\x24\x9a\x25\x5e\x1c\x82\x7f\x3b\x6b\x6c\x7f"
         "\x0a\x79\xf4\xca\x85\x75\xa9\x15\x28\xd5\x82\x58\xd7\x9e\xbd\x68",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x74\xad\x28\x20\x5b\x4f\x38\x4b\xc0\x81\x3e\x65\x85\x86\x4e\x52"
         "\x80\x85\xf9\x1f\xb6\xa5\x09\x6f\x24\x4a\xe0\x1e\x57\xde\x43\xae"
         "\x0c\x66\xf4\x2a\xf1\x55\xcd\xc0\x8c\x96\xc4\x2e\xcf\x2c\x98\x9c"
         "\xbc\x7e\x1b\x4d\xa7\x0a\xb7\x92\x5a\x89\x43\xe8\xc3\x17\x40\x3d",
         65);
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));

  // secret = 833fe62409237b9d62ec77587520911e9a759cec1d19755b7da901b96dca3d42
  memcpy(Pxy, C_ED25519_G, sizeof(Pxy));
  memcpy(s,
         "\x45\xb6\x41\x72\xc7\x52\x8f\x1a\xf4\xa5\xa8\x5d\xd6\xdb\xd8\x72"
         "\x92\xa0\x07\x9b\xf1\x13\x57\x0b\xec\x4b\xe0\x59\x4f\xce\xdd\x30",
         sizeof(s));
  memcpy(expected,
         "\x04"
         "\x58\xb4\x01\xb9\xdf\x6f\x65\xa3\x46\x25\x40\x0a\x43\xfa\x6e\x89"
         "\xdd\x5a\xe7\x44\x0e\x98\x99\xc9\xc9\x6e\xea\x99\x5b\x72\xfc\x2f"
         "\x3f\xe2\x67\x34\x68\x19\xf8\xeb\x64\x4d\xfd\x2e\xef\x67\x54\xc3"
         "\x34\x50\x24\xe1\x70\x2c\x93\xf4\x3b\x56\x5e\xad\x93\x2b\x17\xec",
         65);
  sys_cx_ecfp_scalar_mult(CX_CURVE_Ed25519, Pxy, sizeof(Pxy), s, sizeof(s));
  assert_memory_equal(Pxy, expected, sizeof(expected));
}

void test_scalar_mult_curve25519(void **state __attribute__((unused)))
{
  uint8_t Px[33];
  uint8_t s[32];
  uint8_t expected[33];

  // The two next reference points have been computed thanks to Sage.
  memcpy(Px, C_Curve25519_G, sizeof(Px));
  memcpy(s,
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x0A",
         sizeof(s));
  memcpy(expected,
         "\x02"
         "\x41\xED\xA6\x55\xB1\x59\x06\x04\x71\xFB\x4C\xE5\xD7\xCB\x3F\xE4"
         "\x3E\xE5\x18\x43\xD2\x08\x0E\x03\x83\xCE\x42\x89\x2C\x3A\x9C\x7B",
         33);

  sys_cx_ecfp_scalar_mult(CX_CURVE_Curve25519, Px, sizeof(Px), s, sizeof(s));

  assert_memory_equal(Px, expected, sizeof(expected));

  memcpy(Px, C_Curve25519_G, sizeof(Px));
  memcpy(s,
         "\x45\x07\xA3\xD5\xC4\x34\xD8\x30\xCA\x32\xF6\x16\x1A\x23\x65\xF2"
         "\x81\x1D\xF9\xE0\x46\x51\x97\x1B\xBF\x7F\xAB\xE1\x1C\x37\x49\x57",
         sizeof(s));
  memcpy(expected,
         "\x02"
         "\x33\x5C\x6B\x38\xA0\xD0\x1C\x6F\x8A\x61\xCC\x1D\x4C\x6D\x41\x94"
         "\x8C\x7E\xC7\x73\x15\x86\x4B\x42\x26\xC6\xCE\xBA\x14\x1D\x4D\xBA",
         33);

  sys_cx_ecfp_scalar_mult(CX_CURVE_Curve25519, Px, sizeof(Px), s, sizeof(s));

  assert_memory_equal(Px, expected, sizeof(expected));

  // Verify that 1*G = G
  memcpy(Px, C_Curve25519_G, sizeof(Px));
  memcpy(s,
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
         "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01",
         sizeof(s));
  memcpy(expected, C_Curve25519_G, sizeof(expected));
  sys_cx_ecfp_scalar_mult(CX_CURVE_Curve25519, Px, sizeof(Px), s, sizeof(s));
  assert_memory_equal(Px, expected, sizeof(expected));
}

static void test_eddsa_get_public_key(cx_curve_t curve, cx_md_t md,
                                      const eddsa_test_vector *tv,
                                      size_t tv_len)
{
  cx_ecfp_512_private_key_t private_key;
  cx_ecfp_512_public_key_t pub;
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(curve);
  assert_non_null(domain);
  size_t mpi_size = domain->length;

  uint8_t secret_key[57];
  uint8_t expected_key[114];
  size_t i;

  for (i = 0; i < tv_len; i++) {
    assert_int_equal(hexstr2bin(tv[i].secret_key, secret_key, mpi_size),
                     mpi_size);
    assert_int_equal(hexstr2bin(tv[i].public_key, expected_key, mpi_size),
                     mpi_size);

    assert_int_equal(
        sys_cx_ecfp_init_private_key(curve, secret_key, mpi_size,
                                     (cx_ecfp_private_key_t *)&private_key),
        mpi_size);

    sys_cx_eddsa_get_public_key((cx_ecfp_private_key_t *)&private_key, md,
                                (cx_ecfp_public_key_t *)&pub);
    assert_int_equal(pub.W_len, 1 + 2 * mpi_size);
    assert_int_equal(pub.W[0], 4);
    sys_cx_edward_compress_point(curve, (uint8_t *)&pub.W, pub.W_len);

    assert_int_equal(pub.W[0], 2);
    assert_memory_equal(expected_key, &pub.W[1], mpi_size);
  }
}

static void test_ed25519_get_public_key(void **state __attribute__((unused)))
{
  test_eddsa_get_public_key(CX_CURVE_Ed25519, CX_SHA512, rfc8032_test_vectors,
                            ARRAY_SIZE(rfc8032_test_vectors));
}

void test_eddsa_recover_x(void **state __attribute__((unused)))
{
  cx_ecfp_private_key_t private_key;
  cx_ecfp_public_key_t pub1;

  uint8_t secret_key[32];
  uint8_t buf[1 + 32 + 32];

  const eddsa_test_vector *tv;
  size_t i;

  for (i = 0; i < ARRAY_SIZE(rfc8032_test_vectors); i++) {
    tv = &rfc8032_test_vectors[i];
    assert_int_equal(hexstr2bin(tv->secret_key, secret_key, 32), 32);
    assert_int_equal(hexstr2bin(tv->public_key, &buf[1], 32), 32);

    assert_int_equal(sys_cx_ecfp_init_private_key(CX_CURVE_Ed25519, secret_key,
                                                  32, &private_key),
                     32);
    assert_int_equal(
        sys_cx_ecfp_generate_pair(CX_CURVE_Ed25519, &pub1, &private_key, 1), 0);
    assert_int_equal(pub1.W_len, 1 + 32 + 32);

    buf[0] = 2;
    sys_cx_edward_decompress_point(CX_CURVE_Ed25519, buf, sizeof(buf));
    assert_memory_equal(buf, pub1.W, pub1.W_len);
  }
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_scalar_mult_ed25519),
    cmocka_unit_test(test_scalar_mult_curve25519),
    cmocka_unit_test(test_ed25519_get_public_key),
    cmocka_unit_test(test_eddsa_recover_x),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
