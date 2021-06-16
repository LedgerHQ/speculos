#include <malloc.h>
#include <stdbool.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "bolos/cx_ec.h"
#include "bolos/cx_hash.h"

#include "utils.h"

#define cx_ecfp_init_private_key sys_cx_ecfp_init_private_key
#define cx_ecfp_generate_pair    sys_cx_ecfp_generate_pair
#define cx_ecfp_generate_pair2   sys_cx_ecfp_generate_pair2
#define cx_eddsa_sign            sys_cx_eddsa_sign
#define cx_eddsa_verify          sys_cx_eddsa_verify

typedef struct {
  const char *secret_key;
  const char *public_key;
  const char *msg;
  const char *signature;
} eddsa_test_vector;

/* clang-format off */
const eddsa_test_vector rfc8032_test_vectors[] = {
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

const eddsa_test_vector rfc8032_ed448_test_vectors[] = {
    // 0 bytes
    {"6c82a562cb808d10d632be89c8513ebf6c929f34ddfa8c9f63c9960ef6e348a3528c8a3fc"
     "c2f044e39a3fc5b94492f8f032e7549a20098f95b",
     "5fd7449b59b461fd2ce787ec616ad46a1da1342485a70e1f8a0ea75d80e96778edf124769"
     "b46c7061bd6783df1e50f6cd1fa1abeafe8256180",
     "",
     "533a37f6bbe457251f023c0d88f976ae2dfb504a843e34d2074fd823d41a591f2b233f034"
     "f628281f2fd7a22ddd47d7828c59bd0a21bfd3980ff0d2028d4b18a9df63e006c5d1c2d34"
     "5b925d8dc00b4104852db99ac5c7cdda8530a113a0f4dbb61149f05a7363268c71d95808f"
     "f2e652600"},
    // 1 byte
    {"c4eab05d357007c632f3dbb48489924d552b08fe0c353a0d4a1f00acda2c463afbea67c5e"
     "8d2877c5e3bc397a659949ef8021e954e0a12274e",
     "43ba28f430cdff456ae531545f7ecd0ac834a55d9358c0372bfa0c6c6798c0866aea01eb0"
     "0742802b8438ea4cb82169c235160627b4c3a9480",
     "03",
     "26b8f91727bd62897af15e41eb43c377efb9c610d48f2335cb0bd0087810f4352541b143c"
     "4b981b7e18f62de8ccdf633fc1bf037ab7cd779805e0dbcc0aae1cbcee1afb2e027df36bc"
     "04dcecbf154336c19f0af7e0a6472905e799f1953d2a0ff3348ab21aa4adafd1d234441cf"
     "807c03a00"},
    // 11 bytes
    {"cd23d24f714274e744343237b93290f511f6425f98e64459ff203e8985083ffdf60500553"
     "abc0e05cd02184bdb89c4ccd67e187951267eb328",
     "dcea9e78f35a1bf3499a831b10b86c90aac01cd84b67a0109b55a36e9328b1e365fce161d"
     "71ce7131a543ea4cb5f7e9f1d8b00696447001400",
     "0c3e544074ec63b0265e0c",
     "1f0a8888ce25e8d458a21130879b840a9089d999aaba039eaf3e3afa090a09d389dba82c4"
     "ff2ae8ac5cdfb7c55e94d5d961a29fe0109941e00b8dbdeea6d3b051068df7254c0cdc129"
     "cbe62db2dc957dbb47b51fd3f213fb8698f064774250a5028961c9bf8ffd973fe5d5c2064"
     "92b140e00"},
    // 12 bytes
    {"258cdd4ada32ed9c9ff54e63756ae582fb8fab2ac721f2c8e676a72768513d939f63dddb5"
     "5609133f29adf86ec9929dccb52c1c5fd2ff7e21b",
     "3ba16da0c6f2cc1f30187740756f5e798d6bc5fc015d7c63cc9510ee3fd44adc24d8e968b"
     "6e46e6f94d19b945361726bd75e149ef09817f580",
     "64a65f3cdedcdd66811e2915",
     "7eeeab7c4e50fb799b418ee5e3197ff6bf15d43a14c34389b59dd1a7b1b85b4ae90438aca"
     "634bea45e3a2695f1270f07fdcdf7c62b8efeaf00b45c2c96ba457eb1a8bf075a3db28e5c"
     "24f6b923ed4ad747c3c9e03c7079efb87cb110d3a99861e72003cbae6d6b8b827e4e6c143"
     "064ff3c00"},
    // 13 bytes
    {"7ef4e84544236752fbb56b8f31a23a10e42814f5f55ca037cdcc11c64c9a3b2949c1bb607"
     "00314611732a6c2fea98eebc0266a11a93970100e",
     "b3da079b0aa493a5772029f0467baebee5a8112d9d3a22532361da294f7bb3815c5dc59e1"
     "76b4d9f381ca0938e13c6c07b174be65dfa578e80",
     "64a65f3cdedcdd66811e2915e7",
     "6a12066f55331b6c22acd5d5bfc5d71228fbda80ae8dec26bdd306743c5027cb4890810c1"
     "62c027468675ecf645a83176c0d7323a2ccde2d80efe5a1268e8aca1d6fbc194d3f77c449"
     "86eb4ab4177919ad8bec33eb47bbb5fc6e28196fd1caf56b4e7e0ba5519234d047155ac72"
     "7a1053100"},
    // 64 bytes
    {"d65df341ad13e008567688baedda8e9dcdc17dc024974ea5b4227b6530e339bff21f99e68"
     "ca6968f3cca6dfe0fb9f4fab4fa135d5542ea3f01",
     "df9705f58edbab802c7f8363cfe5560ab1c6132c20a9f1dd163483a26f8ac53a39d6808bf"
     "4a1dfbd261b099bb03b3fb50906cb28bd8a081f00",
     "bd0f6a3747cd561bdddf4640a332461a4a30a12a434cd0bf40d766d9c6d458e5512204a30"
     "c17d1f50b5079631f64eb3112182da3005835461113718d1a5ef944",
     "554bc2480860b49eab8532d2a533b7d578ef473eeb58c98bb2d0e1ce488a98b18dfde9b9b"
     "90775e67f47d4a1c3482058efc9f40d2ca033a0801b63d45b3b722ef552bad3b4ccb667da"
     "350192b61c508cf7b6b5adadc2c8d9a446ef003fb05cba5f30e88e36ec2703b349ca229c2"
     "670833900"},
    // 256 bytes
    {"2ec5fe3c17045abdb136a5e6a913e32ab75ae68b53d2fc149b77e504132d37569b7e766ba"
     "74a19bd6162343a21c8590aa9cebca9014c636df5",
     "79756f014dcfe2079f5dd9e718be4171e2ef2486a08f25186f6bff43a9936b9bfe12402b0"
     "8ae65798a3d81e22e9ec80e7690862ef3d4ed3a00",
     "15777532b0bdd0d1389f636c5f6b9ba734c90af572877e2d272dd078aa1e567cfa80e1292"
     "8bb542330e8409f3174504107ecd5efac61ae7504dabe2a602ede89e5cca6257a7c77e27a"
     "702b3ae39fc769fc54f2395ae6a1178cab4738e543072fc1c177fe71e92e25bf03e4ecb72"
     "f47b64d0465aaea4c7fad372536c8ba516a6039c3c2a39f0e4d832be432dfa9a706a6e5c7"
     "e19f397964ca4258002f7c0541b590316dbc5622b6b2a6fe7a4abffd96105eca76ea7b988"
     "16af0748c10df048ce012d901015a51f189f3888145c03650aa23ce894c3bd889e030d565"
     "071c59f409a9981b51878fd6fc110624dcbcde0bf7a69ccce38fabdf86f3bef6044819de1"
     "1",
     "c650ddbb0601c19ca11439e1640dd931f43c518ea5bea70d3dcde5f4191fe53f00cf96654"
     "6b72bcc7d58be2b9badef28743954e3a44a23f880e8d4f1cfce2d7a61452d26da05896f0a"
     "50da66a239a8a188b6d825b3305ad77b73fbac0836ecc60987fd08527c1a8e80d5823e65c"
     "afe2a3d00"},
    // 1023 bytes
    {"872d093780f5d3730df7c212664b37b8a0f24f56810daa8382cd4fa3f77634ec44dc54f1c"
     "2ed9bea86fafb7632d8be199ea165f5ad55dd9ce8",
     "a81b2e8a70a5ac94ffdbcc9badfc3feb0801f258578bb114ad44ece1ec0e799da08effb81"
     "c5d685c0c56f64eecaef8cdf11cc38737838cf400",
     "6ddf802e1aae4986935f7f981ba3f0351d6273c0a0c22c9c0e8339168e675412a3debfaf4"
     "35ed651558007db4384b650fcc07e3b586a27a4f7a00ac8a6fec2cd86ae4bf1570c41e6a4"
     "0c931db27b2faa15a8cedd52cff7362c4e6e23daec0fbc3a79b6806e316efcc7b68119bf4"
     "6bc76a26067a53f296dafdbdc11c77f7777e972660cf4b6a9b369a6665f02e0cc9b6edfad"
     "136b4fabe723d2813db3136cfde9b6d044322fee2947952e031b73ab5c603349b307bdc27"
     "bc6cb8b8bbd7bd323219b8033a581b59eadebb09b3c4f3d2277d4f0343624acc817804728"
     "b25ab797172b4c5c21a22f9c7839d64300232eb66e53f31c723fa37fe387c7d3e50bdf981"
     "3a30e5bb12cf4cd930c40cfb4e1fc622592a49588794494d56d24ea4b40c89fc0596cc9eb"
     "b961c8cb10adde976a5d602b1c3f85b9b9a001ed3c6a4d3b1437f52096cd1956d042a597d"
     "561a596ecd3d1735a8d570ea0ec27225a2c4aaff26306d1526c1af3ca6d9cf5a2c98f47e1"
     "c46db9a33234cfd4d81f2c98538a09ebe76998d0d8fd25997c7d255c6d66ece6fa56f1114"
     "4950f027795e653008f4bd7ca2dee85d8e90f3dc315130ce2a00375a318c7c3d97be2c8ce"
     "5b6db41a6254ff264fa6155baee3b0773c0f497c573f19bb4f4240281f0b1f4f7be857a4e"
     "59d416c06b4c50fa09e1810ddc6b1467baeac5a3668d11b6ecaa901440016f389f80acc4d"
     "b977025e7f5924388c7e340a732e554440e76570f8dd71b7d640b3450d1fd5f0410a18f9a"
     "3494f707c717b79b4bf75c98400b096b21653b5d217cf3565c9597456f70703497a078763"
     "829bc01bb1cbc8fa04eadc9a6e3f6699587a9e75c94e5bab0036e0b2e711392cff0047d0d"
     "6b05bd2a588bc109718954259f1d86678a579a3120f19cfb2963f177aeb70f2d484482626"
     "2e51b80271272068ef5b3856fa8535aa2a88b2d41f2a0e2fda7624c2850272ac4a2f561f8"
     "f2f7a318bfd5caf9696149e4ac824ad3460538fdc25421beec2cc6818162d06bbed0c40a3"
     "87192349db67a118bada6cd5ab0140ee273204f628aad1c135f770279a651e24d8c14d75a"
     "6059d76b96a6fd857def5e0b354b27ab937a5815d16b5fae407ff18222c6d1ed263be68c9"
     "5f32d908bd895cd76207ae726487567f9a67dad79abec316f683b17f2d02bf07e0ac8b5bc"
     "6162cf94697b3c27cd1fea49b27f23ba2901871962506520c392da8b6ad0d99f7013fbc06"
     "c2c17a569500c8a7696481c1cd33e9b14e40b82e79a5f5db82571ba97bae3ad3e0479515b"
     "b0e2b0f3bfcd1fd33034efc6245eddd7ee2086ddae2600d8ca73e214e8c2b0bdb2b047c6a"
     "464a562ed77b73d2d841c4b34973551257713b753632efba348169abc90a68f42611a4012"
     "6d7cb21b58695568186f7e569d2ff0f9e745d0487dd2eb997cafc5abf9dd102e62ff66cba"
     "87",
     "e301345a41a39a4d72fff8df69c98075a0cc082b802fc9b2b6bc503f926b65bddf7f4c8f1"
     "cb49f6396afc8a70abe6d8aef0db478d4c6b2970076c6a0484fe76d76b3a97625d79f1ce2"
     "40e7c576750d295528286f719b413de9ada3e8eb78ed573603ce30d8bb761785dc30dbc32"
     "0869e1a00"}};
/* clang-format on */

enum wycheproof_result { kValid, kInvalid, kAcceptable };

static bool GetWycheproofCurve(const char *curve, cx_curve_t *out)
{
  if (strcmp(curve, "secp256k1") == 0) {
    *out = CX_CURVE_SECP256K1;
  } else if (strcmp(curve, "secp521r1") == 0) {
    *out = CX_CURVE_SECP521R1;
  } else if (strcmp(curve, "brainpoolP256r1") == 0) {
    *out = CX_CURVE_BrainPoolP256R1;
  } else if (strcmp(curve, "brainpoolP320r1") == 0) {
    *out = CX_CURVE_BrainPoolP320R1;
  } else if (strcmp(curve, "edwards25519") == 0) {
    *out = CX_CURVE_Ed25519;
  } else {
    printf("%s\n", curve);
    return false;
  }
  return true;
}

static bool GetWycheproofResult(const char *result, enum wycheproof_result *out)
{
  if (strcmp(result, "valid") == 0) {
    *out = kValid;
  } else if (strcmp(result, "invalid") == 0) {
    *out = kInvalid;
  } else if (strcmp(result, "acceptable") == 0) {
    *out = kAcceptable;
  } else {
    return false;
  }
  return true;
}

#define BUF_SIZE        1024
#define MAX_LINE_LENGTH 4096

static void test_eddsa_wycheproof(void **state)
{
  (void)state;

  uint8_t sk[32], pk[32];
  cx_ecfp_256_public_key_t public_key = { 0 };
  cx_ecfp_256_private_key_t private_key = { 0 };
  char *line = malloc(MAX_LINE_LENGTH);
  assert_non_null(line);

  FILE *f = fopen(TESTS_PATH "wycheproof/eddsa.data", "r");
  assert_non_null(f);

  enum wycheproof_result res = 0;

  // Format:
  // curve : secret_key : public_key : msg : signature : result
  while (fgets(line, MAX_LINE_LENGTH, f) != NULL) {
    char *pos1 = strchr(line, ':');
    assert_non_null(pos1);
    char *pos2 = strchr(pos1 + 1, ':');
    assert_non_null(pos2);
    char *pos3 = strchr(pos2 + 1, ':');
    assert_non_null(pos3);
    char *pos4 = strchr(pos3 + 1, ':');
    assert_non_null(pos4);
    char *pos5 = strchr(pos4 + 1, ':');
    assert_non_null(pos5);
    char *pos6 = strchr(pos5 + 1, '\n');

    *pos1 = *pos2 = *pos3 = *pos4 = *pos5 = 0;
    if (pos6) {
      *pos6 = 0;
      if (*(pos6 - 1) == '\r') {
        *(pos6 - 1) = 0;
      }
    }

    size_t sk_len = strlen(pos1 + 1) / 2;
    size_t pk_len = strlen(pos2 + 1) / 2;
    size_t msg_len = strlen(pos3 + 1) / 2;
    size_t signature_len = strlen(pos4 + 1) / 2;
    cx_curve_t curve = 0;

    assert_int_equal(sk_len, 32);
    assert_int_equal(pk_len, 32);
    uint8_t *msg = malloc(msg_len);
    assert_non_null(msg);
    uint8_t *signature = malloc(signature_len);
    assert_non_null(signature);

    assert_int_equal(hexstr2bin(pos1 + 1, sk, sk_len), sk_len);
    assert_int_equal(hexstr2bin(pos2 + 1, pk, pk_len), pk_len);
    assert_int_equal(hexstr2bin(pos3 + 1, msg, msg_len), msg_len);
    assert_int_equal(hexstr2bin(pos4 + 1, signature, signature_len),
                     signature_len);

    // cx_eddsa throws an exception if signatures have an invalid size.
    // This prevents a case when the supplied signature is empty, has a missing
    // r, etc.
    if (signature_len != 2 * 32) {
      continue;
    }

    assert_true(GetWycheproofCurve(line, &curve));
    assert_true(GetWycheproofResult(pos5 + 1, &res));
    assert_int_equal(cx_ecfp_init_private_key(curve, sk, 32, &private_key), 32);
    assert_int_equal(cx_ecfp_generate_pair(curve, &public_key, &private_key, 1),
                     0);
    assert_int_equal(public_key.W_len, 65);

    if (res == kValid) {
      assert_int_equal(cx_eddsa_verify(&public_key, 0, CX_SHA512, msg, msg_len,
                                       NULL, 0, signature, signature_len),
                       1);
    } else if (res == kInvalid) {
      assert_int_equal(cx_eddsa_verify(&public_key, 0, CX_SHA512, msg, msg_len,
                                       NULL, 0, signature, signature_len),
                       0);
    }

    free(msg);
    free(signature);
  }
  fclose(f);
  free(line);
}

static void test_eddsa_sign(cx_curve_t curve, cx_md_t md,
                            const eddsa_test_vector *tv, size_t tv_len)
{
  cx_ecfp_512_private_key_t private_key;
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(curve);
  assert_non_null(domain);
  size_t mpi_size = domain->length;

  uint8_t secret_key[57];
  uint8_t signature[114], expected_signature[114];
  uint8_t msg[BUF_SIZE];
  size_t msg_len;

  for (unsigned int i = 0; i < tv_len; i++) {
    assert_int_equal(hexstr2bin(tv[i].secret_key, secret_key, mpi_size),
                     mpi_size);
    msg_len = strlen(tv[i].msg) / 2;
    assert_int_equal(hexstr2bin(tv[i].msg, msg, BUF_SIZE), msg_len);
    assert_int_equal(
        hexstr2bin(tv[i].signature, expected_signature, mpi_size * 2),
        mpi_size * 2);

    assert_int_equal(
        cx_ecfp_init_private_key(curve, secret_key, mpi_size,
                                 (cx_ecfp_private_key_t *)&private_key),
        mpi_size);
    assert_int_equal(cx_eddsa_sign((cx_ecfp_private_key_t *)&private_key, 0, md,
                                   msg, msg_len, NULL, 0, signature,
                                   mpi_size * 2, NULL),
                     mpi_size * 2);
    assert_memory_equal(signature, expected_signature, mpi_size * 2);
  }
}

static void test_eddsa_verify(cx_curve_t curve, cx_md_t md,
                              const eddsa_test_vector *tv, size_t tv_len)
{
  cx_ecfp_512_private_key_t private_key;
  cx_ecfp_512_public_key_t public_key;
  const cx_curve_domain_t *domain = cx_ecfp_get_domain(curve);
  assert_non_null(domain);
  size_t mpi_size = domain->length;

  uint8_t secret_key[57];
  uint8_t expected_signature[114];
  uint8_t msg[BUF_SIZE];
  size_t msg_len;

  for (unsigned int i = 0; i < tv_len; i++) {
    assert_int_equal(hexstr2bin(tv[i].secret_key, secret_key, mpi_size),
                     mpi_size);
    msg_len = strlen(tv[i].msg) / 2;
    assert_int_equal(hexstr2bin(tv[i].msg, msg, BUF_SIZE), msg_len);
    assert_int_equal(
        hexstr2bin(tv[i].signature, expected_signature, mpi_size * 2),
        mpi_size * 2);
    assert_int_equal(
        cx_ecfp_init_private_key(curve, secret_key, mpi_size,
                                 (cx_ecfp_private_key_t *)&private_key),
        mpi_size);
    assert_int_equal(
        cx_ecfp_generate_pair2(curve, (cx_ecfp_public_key_t *)&public_key,
                               (cx_ecfp_private_key_t *)&private_key, 1, md),
        0);
    assert_int_equal(cx_eddsa_verify((cx_ecfp_public_key_t *)&public_key, 0, md,
                                     msg, msg_len, NULL, 0, expected_signature,
                                     mpi_size * 2),
                     1);
  }
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

  for (unsigned int i = 0; i < tv_len; i++) {
    assert_int_equal(hexstr2bin(tv[i].secret_key, secret_key, mpi_size),
                     mpi_size);
    assert_int_equal(hexstr2bin(tv[i].public_key, expected_key, mpi_size),
                     mpi_size);

    assert_int_equal(
        cx_ecfp_init_private_key(curve, secret_key, mpi_size,
                                 (cx_ecfp_private_key_t *)&private_key),
        mpi_size);

    sys_cx_eddsa_get_public_key((cx_ecfp_private_key_t *)&private_key, md,
                                (cx_ecfp_public_key_t *)&pub);
    assert_int_equal(pub.W_len, 1 + 2 * mpi_size);
    assert_int_equal(pub.W[0], 4);
    assert_int_equal(
        sys_cx_edward_compress_point(curve, (uint8_t *)&pub.W, pub.W_len), 0);

    assert_int_equal(pub.W[0], 2);
    assert_memory_equal(expected_key, &pub.W[1], mpi_size);
  }
}

static void test_ed25519_sign(void **state)
{
  (void)state;

  test_eddsa_sign(CX_CURVE_Ed25519, CX_SHA512, rfc8032_test_vectors,
                  sizeof(rfc8032_test_vectors) /
                      sizeof(rfc8032_test_vectors[0]));
}

static void test_ed25519_verify(void **state)
{
  (void)state;

  test_eddsa_verify(CX_CURVE_Ed25519, CX_SHA512, rfc8032_test_vectors,
                    sizeof(rfc8032_test_vectors) /
                        sizeof(rfc8032_test_vectors[0]));
}

static void test_ed25519_get_public_key(void **state)
{
  (void)state;
  test_eddsa_get_public_key(CX_CURVE_Ed25519, CX_SHA512, rfc8032_test_vectors,
                            sizeof(rfc8032_test_vectors) /
                                sizeof(rfc8032_test_vectors[0]));
}

int main()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_ed25519_sign), cmocka_unit_test(test_ed25519_verify),
    cmocka_unit_test(test_ed25519_get_public_key),
    cmocka_unit_test(test_eddsa_wycheproof)
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
