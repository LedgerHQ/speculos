#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "bolos/cx.h"
#include "bolos/cx_ec.h"
#include "bolos/cx_hash.h"
#include "bolos/cx_rng_rfc6979.h"
#include "emulate.h"
#include "utils.h"

// Test vectors from https://tools.ietf.org/html/rfc6979#appendix-A.2
typedef struct {
  cx_md_t digest;
  const char *msg;
  const char *private_key;
  const char *k;
  const char *r;
  const char *s;
} rfc6979_test_vector;

static void test_ecdsa_rfc6979_secp256r1(void **UNUSED(state))
{
  const rfc6979_test_vector tvs[] = {
    { CX_SHA224, "sample",
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721",
      "103F90EE9DC52E5E7FB5132B7033C63066D194321491862059967C715985D473",
      "53B2FFF5D1752B2C689DF257C04C40A587FABABB3F6FC2702F1343AF7CA9AA3F",
      "B9AFB64FDC03DC1A131C7D2386D11E349F070AA432A4ACC918BEA988BF75C74C" },
    { CX_SHA256, "sample",
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721",
      "A6E3C57DD01ABE90086538398355DD4C3B17AA873382B0F24D6129493D8AAD60",
      "EFD48B2AACB6A8FD1140DD9CD45E81D69D2C877B56AAF991C34D0EA84EAF3716",
      "F7CB1C942D657C41D436C7A1B6E29F65F3E900DBB9AFF4064DC4AB2F843ACDA8" },
    { CX_SHA384, "sample",
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721",
      "09F634B188CEFD98E7EC88B1AA9852D734D0BC272F7D2A47DECC6EBEB375AAD4",
      "0EAFEA039B20E9B42309FB1D89E213057CBF973DC0CFC8F129EDDDC800EF7719",
      "4861F0491E6998B9455193E34E7B0D284DDD7149A74B95B9261F13ABDE940954" },
    { CX_SHA512, "sample",
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721",
      "5FA81C63109BADB88C1F367B47DA606DA28CAD69AA22C4FE6AD7DF73A7173AA5",
      "8496A60B5E9B47C825488827E0495B0E3FA109EC4568FD3F8D1097678EB97F00",
      "2362AB1ADBE2B8ADF9CB9EDAB740EA6049C028114F2460F96554F61FAE3302FE" },
    { CX_SHA224, "test",
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721",
      "669F4426F2688B8BE0DB3A6BD1989BDAEFFF84B649EEB84F3DD26080F667FAA7",
      "C37EDB6F0AE79D47C3C27E962FA269BB4F441770357E114EE511F662EC34A692",
      "C820053A05791E521FCAAD6042D40AEA1D6B1A540138558F47D0719800E18F2D" },
    { CX_SHA256, "test",
      "C9AFA9D845BA75166B5C215767B1D6934E50C3DB36E89B127B8A622B120F6721",
      "D16B6AE827F17175E040871A1C7EC3500192C4C92677336EC2537ACAEE0008E0",
      "F1ABB023518351CD71D881567B1EA663ED3EFCF6C5132B354F28D3B0B7D38367",
      "019F4113742A2B14BD25926B49C649155F267E60D3814B4C0CC84250E46F0083" },
  };

  cx_ecfp_private_key_t private_key;
  uint8_t raw_private_key[32];
  uint8_t r[32], s[32], k[32], computed_k[32];
  uint8_t sig[128];
  uint8_t digest[CX_SHA512_SIZE];
  const uint8_t *computed_r = NULL, *computed_s = NULL;
  size_t rlen, slen, sig_len;
  cx_hash_ctx hash_ctx;

  for (size_t i = 0; i < sizeof(tvs) / sizeof(tvs[0]); i++) {
    const rfc6979_test_vector *tv = &tvs[i];
    const cx_curve_domain_t *domain = cx_ecfp_get_domain(CX_CURVE_SECP256R1);
    assert_non_null(domain);
    assert_int_equal(domain->length, 32);
    size_t len = domain->length;

    assert_int_equal(hexstr2bin(tv->private_key, raw_private_key, len), len);
    assert_int_equal(hexstr2bin(tv->r, r, len), len);
    assert_int_equal(hexstr2bin(tv->s, s, len), len);
    assert_int_equal(hexstr2bin(tv->k, k, len), len);

    assert_int_equal(sys_cx_ecfp_init_private_key(CX_CURVE_SECP256R1,
                                                  raw_private_key, len,
                                                  &private_key),
                     len);

    assert_int_equal(spec_cx_hash_init(&hash_ctx, tv->digest), 1);
    assert_int_equal(
        spec_cx_hash_update(&hash_ctx, (uint8_t *)tv->msg, strlen(tv->msg)), 1);
    size_t md_size = spec_cx_hash_get_size(&hash_ctx);
    assert_non_null(md_size);
    assert_int_equal(spec_cx_hash_final(&hash_ctx, digest), 1);

    cx_rnd_rfc6979_ctx_t rfc_ctx = {};
    spec_cx_rng_rfc6979_init(&rfc_ctx, tv->digest, raw_private_key, len, digest,
                             md_size, domain->n, len);
    spec_cx_rng_rfc6979_next(&rfc_ctx, computed_k, len);
    assert_memory_equal(k, computed_k, len);

    sig_len = sys_cx_ecdsa_sign(&private_key, CX_RND_RFC6979 | CX_NO_CANONICAL,
                                tv->digest, digest, md_size, sig, 128, NULL);
    assert_int_not_equal(sig_len, 0);
    assert_int_equal(spec_cx_ecfp_decode_sig_der(sig, sig_len, len, &computed_r,
                                                 &rlen, &computed_s, &slen),
                     1);
    assert_int_equal(rlen, len);
    assert_int_equal(slen, len);
    assert_memory_equal(r, computed_r, len);
    assert_memory_equal(s, computed_s, len);
  }
}

static void test_ecdsa_rfc6979_secp384r1(void **UNUSED(state))
{
  const rfc6979_test_vector tvs[] = {
    { CX_SHA224, "sample",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "A4E4D2F0E729EB786B31FC20AD5D849E304450E0AE8E3E341134A5C1AFA03CAB8083EE4E"
      "3C45B06A5899EA56C51B5879",
      "42356E76B55A6D9B4631C865445DBE54E056D3B3431766D0509244793C3F9366450F76EE"
      "3DE43F5A125333A6BE060122",
      "9DA0C81787064021E78DF658F2FBB0B042BF304665DB721F077A4298B095E4834C082C03"
      "D83028EFBF93A3C23940CA8D" },
    { CX_SHA256, "sample",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "180AE9F9AEC5438A44BC159A1FCB277C7BE54FA20E7CF404B490650A8ACC414E37557234"
      "2863C899F9F2EDF9747A9B60",
      "21B13D1E013C7FA1392D03C5F99AF8B30C570C6F98D4EA8E354B63A21D3DAA33BDE1E888"
      "E63355D92FA2B3C36D8FB2CD",
      "F3AA443FB107745BF4BD77CB3891674632068A10CA67E3D45DB2266FA7D1FEEBEFDC63EC"
      "CD1AC42EC0CB8668A4FA0AB0" },
    { CX_SHA384, "sample",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "94ED910D1A099DAD3254E9242AE85ABDE4BA15168EAF0CA87A555FD56D10FBCA2907E3E8"
      "3BA95368623B8C4686915CF9",
      "94EDBB92A5ECB8AAD4736E56C691916B3F88140666CE9FA73D64C4EA95AD133C81A64815"
      "2E44ACF96E36DD1E80FABE46",
      "99EF4AEB15F178CEA1FE40DB2603138F130E740A19624526203B6351D0A3A94FA329C145"
      "786E679E7B82C71A38628AC8" },
    { CX_SHA512, "sample",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "92FC3C7183A883E24216D1141F1A8976C5B0DD797DFA597E3D7B32198BD35331A4E96653"
      "2593A52980D0E3AAA5E10EC3",
      "ED0959D5880AB2D869AE7F6C2915C6D60F96507F9CB3E047C0046861DA4A799CFE30F35C"
      "C900056D7C99CD7882433709",
      "512C8CCEEE3890A84058CE1E22DBC2198F42323CE8ACA9135329F03C068E5112DC7CC3EF"
      "3446DEFCEB01A45C2667FDD5" },
    { CX_SHA224, "test",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "18FA39DB95AA5F561F30FA3591DC59C0FA3653A80DAFFA0B48D1A4C6DFCBFF6E3D33BE4D"
      "C5EB8886A8ECD093F2935726",
      "E8C9D0B6EA72A0E7837FEA1D14A1A9557F29FAA45D3E7EE888FC5BF954B5E62464A9A817"
      "C47FF78B8C11066B24080E72",
      "07041D4A7A0379AC7232FF72E6F77B6DDB8F09B16CCE0EC3286B2BD43FA8C6141C53EA5A"
      "BEF0D8231077A04540A96B66" },
    { CX_SHA256, "test",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "0CFAC37587532347DC3389FDC98286BBA8C73807285B184C83E62E26C401C0FAA48DD070"
      "BA79921A3457ABFF2D630AD7",
      "6D6DEFAC9AB64DABAFE36C6BF510352A4CC27001263638E5B16D9BB51D451559F918EEDA"
      "F2293BE5B475CC8F0188636B",
      "2D46F3BECBCC523D5F1A1256BF0C9B024D879BA9E838144C8BA6BAEB4B53B47D51AB373F"
      "9845C0514EEFB14024787265" },
    { CX_SHA384, "test",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "015EE46A5BF88773ED9123A5AB0807962D193719503C527B031B4C2D225092ADA71F4A45"
      "9BC0DA98ADB95837DB8312EA",
      "8203B63D3C853E8D77227FB377BCF7B7B772E97892A80F36AB775D509D7A5FEB0542A7F0"
      "812998DA8F1DD3CA3CF023DB",
      "DDD0760448D42D8A43AF45AF836FCE4DE8BE06B485E9B61B827C2F13173923E06A739F04"
      "0649A667BF3B828246BAA5A5" },
    { CX_SHA512, "test",
      "6B9D3DAD2E1B8C1C05B19875B6659F4DE23C3B667BF297BA9AA47740787137D896D5724E"
      "4C70A825F872C9EA60D2EDF5",
      "3780C4F67CB15518B6ACAE34C9F83568D2E12E47DEAB6C50A4E4EE5319D1E8CE0E2CC8A1"
      "36036DC4B9C00E6888F66B6C",
      "A0D5D090C9980FAF3C2CE57B7AE951D31977DD11C775D314AF55F76C676447D06FB6495C"
      "D21B4B6E340FC236584FB277",
      "976984E59B4C77B0E8E4460DCA3D9F20E07B9BB1F63BEEFAF576F6B2E8B224634A2092CD"
      "3792E0159AD9CEE37659C736" },
  };

  cx_ecfp_384_private_key_t private_key;
  uint8_t raw_private_key[48];
  uint8_t r[48], s[48];
  uint8_t sig[128];
  uint8_t digest[CX_SHA512_SIZE];
  const uint8_t *computed_r = NULL, *computed_s = NULL;
  size_t rlen, slen, sig_len;
  cx_hash_ctx hash_ctx;

  for (size_t i = 0; i < sizeof(tvs) / sizeof(tvs[0]); i++) {
    const rfc6979_test_vector *tv = &tvs[i];
    const cx_curve_domain_t *domain = cx_ecfp_get_domain(CX_CURVE_SECP384R1);
    assert_non_null(domain);
    assert_int_equal(domain->length, 48);
    size_t len = domain->length;

    assert_int_equal(hexstr2bin(tv->private_key, raw_private_key, len), len);
    assert_int_equal(hexstr2bin(tv->r, r, len), len);
    assert_int_equal(hexstr2bin(tv->s, s, len), len);

    assert_int_equal(
        sys_cx_ecfp_init_private_key(CX_CURVE_SECP384R1, raw_private_key, len,
                                     (cx_ecfp_private_key_t *)&private_key),
        len);

    assert_int_equal(spec_cx_hash_init(&hash_ctx, tv->digest), 1);
    assert_int_equal(
        spec_cx_hash_update(&hash_ctx, (uint8_t *)tv->msg, strlen(tv->msg)), 1);
    size_t md_size = spec_cx_hash_get_size(&hash_ctx);
    assert_non_null(md_size);
    assert_int_equal(spec_cx_hash_final(&hash_ctx, digest), 1);

    sig_len = sys_cx_ecdsa_sign((cx_ecfp_private_key_t *)&private_key,
                                CX_RND_RFC6979 | CX_NO_CANONICAL, tv->digest,
                                digest, md_size, sig, 128, NULL);
    assert_int_not_equal(sig_len, 0);
    assert_int_equal(spec_cx_ecfp_decode_sig_der(sig, sig_len, len, &computed_r,
                                                 &rlen, &computed_s, &slen),
                     1);

    assert_int_equal(rlen, len);
    assert_int_equal(slen, len);
    assert_memory_equal(r, computed_r, len);
    assert_memory_equal(s, computed_s, len);
  }
}

static void test_ecdsa_rfc6979_secp256k1(void **UNUSED(state))
{
  // There are no "official" test vectors for secp256k1.
  // Tests were taken from
  // http://www.f2fpro.com/newx/vendor/mdanter/ecc/tests/specs/secp-256k1.yml
  cx_ecfp_private_key_t private_key;
  uint8_t raw_private_key[32];
  uint8_t r[32], s[32];
  uint8_t sig[128];
  uint8_t md[CX_SHA256_SIZE];
  const uint8_t *computed_r = NULL, *computed_s = NULL;
  size_t rlen, slen, sig_len;

  const cx_curve_domain_t *domain = cx_ecfp_get_domain(CX_CURVE_SECP256K1);
  assert_non_null(domain);
  assert_int_equal(domain->length, 32);
  size_t len = domain->length;

  assert_int_equal(
      hexstr2bin(
          "0000000000000000000000000000000000000000000000000000000000000001",
          raw_private_key, len),
      len);
  assert_int_equal(
      hexstr2bin(
          "934b1ea10a4b3c1757e2b0c017d0b6143ce3c9a7e6a4a49860d7a6ab210ee3d8", r,
          len),
      len);
  assert_int_equal(
      hexstr2bin(
          "dbbd3162d46e9f9bef7feb87c16dc13b4f6568a87f4e83f728e2443ba586675c", s,
          len),
      len);

  assert_int_equal(sys_cx_ecfp_init_private_key(
                       CX_CURVE_SECP256K1, raw_private_key, len, &private_key),
                   len);
  sys_cx_hash_sha256((uint8_t *)"Satoshi Nakamoto", 16, md, CX_SHA256_SIZE);

  sig_len =
      sys_cx_ecdsa_sign(&private_key, CX_RND_RFC6979 | CX_NO_CANONICAL,
                        CX_SHA256, md, CX_SHA256_SIZE, sig, sizeof(sig), NULL);
  assert_int_not_equal(sig_len, 0);
  assert_int_equal(spec_cx_ecfp_decode_sig_der(sig, sig_len, len, &computed_r,
                                               &rlen, &computed_s, &slen),
                   1);

  assert_int_equal(rlen, len);
  assert_int_equal(slen, len);
  assert_memory_equal(r, computed_r, len);
  assert_memory_equal(s, computed_s, len);

  assert_int_equal(
      hexstr2bin(
          "0000000000000000000000000000000000000000000000000000000000000001",
          raw_private_key, len),
      len);
  assert_int_equal(
      hexstr2bin(
          "8600dbd41e348fe5c9465ab92d23e3db8b98b873beecd930736488696438cb6b", r,
          len),
      len);
  assert_int_equal(
      hexstr2bin(
          "ab8019bbd8b6924cc4099fe625340ffb1eaac34bf4477daa39d0835429094520", s,
          len),
      len);

  assert_int_equal(sys_cx_ecfp_init_private_key(
                       CX_CURVE_SECP256K1, raw_private_key, len, &private_key),
                   len);
  sys_cx_hash_sha256((uint8_t *)"All those moments will be lost in time, like "
                                "tears in rain. Time to die...",
                     74, md, CX_SHA256_SIZE);

  sig_len =
      sys_cx_ecdsa_sign(&private_key, CX_RND_RFC6979 | CX_NO_CANONICAL,
                        CX_SHA256, md, CX_SHA256_SIZE, sig, sizeof(sig), NULL);
  assert_int_not_equal(sig_len, 0);
  assert_int_equal(spec_cx_ecfp_decode_sig_der(sig, sig_len, len, &computed_r,
                                               &rlen, &computed_s, &slen),
                   1);

  assert_int_equal(rlen, len);
  assert_int_equal(slen, len);
  assert_memory_equal(r, computed_r, len);
  assert_memory_equal(s, computed_s, len);

  assert_int_equal(
      hexstr2bin(
          "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140",
          raw_private_key, len),
      len);
  assert_int_equal(
      hexstr2bin(
          "fd567d121db66e382991534ada77a6bd3106f0a1098c231e47993447cd6af2d0", r,
          len),
      len);
  assert_int_equal(
      hexstr2bin(
          "94c632f14e4379fc1ea610a3df5a375152549736425ee17cebe10abbc2a2826c", s,
          len),
      len);

  assert_int_equal(sys_cx_ecfp_init_private_key(
                       CX_CURVE_SECP256K1, raw_private_key, len, &private_key),
                   len);
  sys_cx_hash_sha256((uint8_t *)"Satoshi Nakamoto", 16, md, CX_SHA256_SIZE);

  sig_len =
      sys_cx_ecdsa_sign(&private_key, CX_RND_RFC6979 | CX_NO_CANONICAL,
                        CX_SHA256, md, CX_SHA256_SIZE, sig, sizeof(sig), NULL);
  assert_int_not_equal(sig_len, 0);
  assert_int_equal(spec_cx_ecfp_decode_sig_der(sig, sig_len, len, &computed_r,
                                               &rlen, &computed_s, &slen),
                   1);

  assert_int_equal(rlen, len);
  assert_int_equal(slen, len);
  assert_memory_equal(r, computed_r, len);
  assert_memory_equal(s, computed_s, len);

  assert_int_equal(
      hexstr2bin(
          "f8b8af8ce3c7cca5e300d33939540c10d45ce001b8f252bfbc57ba0342904181",
          raw_private_key, len),
      len);
  assert_int_equal(
      hexstr2bin(
          "7063ae83e7f62bbb171798131b4a0564b956930092b33b07b395615d9ec7e15c", r,
          len),
      len);
  assert_int_equal(
      hexstr2bin(
          "a72033e1ff5ca1ea8d0c99001cb45f0272d3be7525d3049c0d9e98dc7582b857", s,
          len),
      len);

  assert_int_equal(sys_cx_ecfp_init_private_key(
                       CX_CURVE_SECP256K1, raw_private_key, len, &private_key),
                   len);
  sys_cx_hash_sha256((uint8_t *)"Alan Turing", 11, md, CX_SHA256_SIZE);

  sig_len =
      sys_cx_ecdsa_sign(&private_key, CX_RND_RFC6979 | CX_NO_CANONICAL,
                        CX_SHA256, md, CX_SHA256_SIZE, sig, sizeof(sig), NULL);
  assert_int_not_equal(sig_len, 0);
  assert_int_equal(spec_cx_ecfp_decode_sig_der(sig, sig_len, len, &computed_r,
                                               &rlen, &computed_s, &slen),
                   1);

  assert_int_equal(rlen, len);
  assert_int_equal(slen, len);
  assert_memory_equal(r, computed_r, len);
  assert_memory_equal(s, computed_s, len);
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_ecdsa_rfc6979_secp256r1),
    cmocka_unit_test(test_ecdsa_rfc6979_secp384r1),
    cmocka_unit_test(test_ecdsa_rfc6979_secp256k1)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
