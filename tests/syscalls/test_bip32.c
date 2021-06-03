#include <err.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include <cmocka.h>

#include "bolos/cx_ec.h"
#include "bolos/cx_utils.h"
#include "bolos/os_bip32.h"
#include "emulate.h"
#include "utils.h"

#define MAX_CHAIN_LEN 5

typedef struct {
  uint32_t index;
  const char *chain_code;
  const char *private_key;
} bip32_chain_vector;

typedef struct {
  cx_curve_t curve;
  const char *seed;
  /* expand_seed_bip32() doesn't compute the chain code */
  const char *chain_code;
  const char *private_key;
  int chain_len;
  const bip32_chain_vector chain[MAX_CHAIN_LEN];
} bip32_test_vector;

/* abandon abandon abandon abandon abandon abandon abandon abandon abandon
 * abandon abandon about */
static const char *default_seed =
    "5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc1"
    "9a5ac40b389cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4";

/* clang-format off */
static const bip32_test_vector test_vectors[] = {
  // Test vector 1 for secp256k1
  {
    .curve = CX_CURVE_256K1,
    .seed = "000102030405060708090a0b0c0d0e0f",
    .chain_code = "873dff81c02f525623fd1fe5167eac3a55a049de3d314bb42ee227ffed37d508",
    .private_key = "e8f32e723decf4051aefac8e2c93c9c5b214313817cdb01a1494b917c8436b35",
    .chain_len = 5,
    .chain = {
      {0x80000000, "47fdacbd0f1097043b78c63c20c34ef4ed9a111d980047ad16282c7ae6236141", "edb2e14f9ee77d26dd93b4ecede8d16ed408ce149b6cd80b0715a2d911a0afea"},
      {0x00000001, "2a7857631386ba23dacac34180dd1983734e444fdbf774041578e9b6adb37c19", "3c6cb8d0f6a264c91ea8b5030fadaa8e538b020f0a387421a12de9319dc93368"},
      {0x80000002, "04466b9cc8e161e966409ca52986c584f07e9dc81f735db683c3ff6ec7b1503f", "cbce0d719ecf7431d88e6a89fa1483e02e35092af60c042b1df2ff59fa424dca"},
      {0x00000002, "cfb71883f01676f587d023cc53a35bc7f88f724b1f8c2892ac1275ac822a3edd", "0f479245fb19a38a1954c5c7c0ebab2f9bdfd96a17563ef28a6a4b1a2a764ef4"},
      {1000000000, "c783e67b921d2beb8f6b389cc646d7263b4145701dadd2161548a8b078e65e9e", "471b76e389e528d6de6d816857e012c5455051cad6660850e58372a6c3e6e7c8"}
    }
  },
  // Test vector 1 for nist256p1
  {
    .curve = CX_CURVE_256R1,
    .seed = "000102030405060708090a0b0c0d0e0f",
    .chain_code = "beeb672fe4621673f722f38529c07392fecaa61015c80c34f29ce8b41b3cb6ea",
    .private_key = "612091aaa12e22dd2abef664f8a01a82cae99ad7441b7ef8110424915c268bc2",
    .chain_len = 5,
    .chain = {
      {0x80000000, "3460cea53e6a6bb5fb391eeef3237ffd8724bf0a40e94943c98b83825342ee11", "6939694369114c67917a182c59ddb8cafc3004e63ca5d3b84403ba8613debc0c"},
      {0x00000001, "4187afff1aafa8445010097fb99d23aee9f599450c7bd140b6826ac22ba21d0c", "284e9d38d07d21e4e281b645089a94f4cf5a5a81369acf151a1c3a57f18b2129"},
      {0x80000002, "98c7514f562e64e74170cc3cf304ee1ce54d6b6da4f880f313e8204c2a185318", "694596e8a54f252c960eb771a3c41e7e32496d03b954aeb90f61635b8e092aa7"},
      {0x00000002, "ba96f776a5c3907d7fd48bde5620ee374d4acfd540378476019eab70790c63a0", "5996c37fd3dd2679039b23ed6f70b506c6b56b3cb5e424681fb0fa64caf82aaa"},
      {1000000000, "b9b7b82d326bb9cb5b5b121066feea4eb93d5241103c9e7a18aad40f1dde8059", "21c4f269ef0a5fd1badf47eeacebeeaa3de22eb8e5b0adcd0f27dd99d34d0119"}
    }
  },
  // Test vector 2 for secp256k1
  {
    .curve = CX_CURVE_SECP256K1,
    .seed = "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
    .chain_code = "60499f801b896d83179a4374aeb7822aaeaceaa0db1f85ee3e904c4defbd9689",
    .private_key = "4b03d6fc340455b363f51020ad3ecca4f0850280cf436c70c727923f6db46c3e",
    .chain_len = 5,
    .chain = {
          {0x00000000, "f0909affaa7ee7abe5dd4e100598d4dc53cd709d5a5c2cac40e7412f232f7c9c", "abe74a98f6c7eabee0428f53798f0ab8aa1bd37873999041703c742f15ac7e1e"},
          {0xffffffff, "be17a268474a6bb9c61e1d720cf6215e2a88c5406c4aee7b38547f585c9a37d9", "877c779ad9687164e9c2f4f0f4ff0340814392330693ce95a58fe18fd52e6e93"},
          {0x00000001, "f366f48f1ea9f2d1d3fe958c95ca84ea18e4c4ddb9366c336c927eb246fb38cb", "704addf544a06e5ee4bea37098463c23613da32020d604506da8c0518e1da4b7"},
          {0xfffffffe, "637807030d55d01f9a0cb3a7839515d796bd07706386a6eddf06cc29a65a0e29", "f1c7c871a54a804afe328b4c83a1c33b8e5ff48f5087273f04efa83b247d6a2d"},
          {0x00000002, "9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271", "bb7d39bdb83ecf58f2fd82b6d918341cbef428661ef01ab97c28a4842125ac23"}
      }
  },
  // Test vector 2 for nist256p1
  {
    .curve = CX_CURVE_SECP256R1,
    .seed = "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c999693908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
    .chain_code = "96cd4465a9644e31528eda3592aa35eb39a9527769ce1855beafc1b81055e75d",
    .private_key = "eaa31c2e46ca2962227cf21d73a7ef0ce8b31c756897521eb6c7b39796633357",
    .chain_len = 5,
    .chain = {
      {0x00000000, "84e9c258bb8557a40e0d041115b376dd55eda99c0042ce29e81ebe4efed9b86a", "d7d065f63a62624888500cdb4f88b6d59c2927fee9e6d0cdff9cad555884df6e"},
      {0xffffffff, "f235b2bc5c04606ca9c30027a84f353acf4e4683edbd11f635d0dcc1cd106ea6", "96d2ec9316746a75e7793684ed01e3d51194d81a42a3276858a5b7376d4b94b9"},
      {0x00000001, "7c0b833106235e452eba79d2bdd58d4086e663bc8cc55e9773d2b5eeda313f3b", "974f9096ea6873a915910e82b29d7c338542ccde39d2064d1cc228f371542bbc"},
      {0xfffffffe, "5794e616eadaf33413aa309318a26ee0fd5163b70466de7a4512fd4b1a5c9e6a", "da29649bbfaff095cd43819eda9a7be74236539a29094cd8336b07ed8d4eff63"},
      {0x00000002, "3bfb29ee8ac4484f09db09c2079b520ea5616df7820f071a20320366fbe226a7", "bb0a77ba01cc31d77205d51d08bd313b979a71ef4de9b062f8958297e746bd67"}
    }
  },
  // Test derivation retry for nist256p1
  {
    .curve = CX_CURVE_SECP256R1,
    .seed = "000102030405060708090a0b0c0d0e0f",
    .chain_code = "beeb672fe4621673f722f38529c07392fecaa61015c80c34f29ce8b41b3cb6ea",
    .private_key = "612091aaa12e22dd2abef664f8a01a82cae99ad7441b7ef8110424915c268bc2",
    .chain_len = 2,
    .chain = {
      {0x80006fa2, "e94c8ebe30c2250a14713212f6449b20f3329105ea15b652ca5bdfc68f6c65c2", "06f0db126f023755d0b8d86d4591718a5210dd8d024e3e14b6159d63f53aa669"},
      {0x00008495, "9e87fe95031f14736774cd82f25fd885065cb7c358c1edf813c72af535e83071", "092154eed4af83e078ff9b84322015aefe5769e31270f62c3f66c33888335f3a"}
    }
  },
  // Test seed retry for nist256p1
  {
    .curve = CX_CURVE_SECP256R1,
    .seed = "a7305bc8df8d0951f0cb224c0e95d7707cbdf2c6ce7e8d481fec69c7ff5e9446",
    .chain_code = "7762f9729fed06121fd13f326884c82f59aa95c57ac492ce8c9654e60efd130c",
    .private_key = "3b8c18469a4634517d6d0b65448f8e6c62091b45540a1743c5846be55d47d88f",
    .chain_len = 0,
    .chain = {}
  },
};
/* clang-format on */

struct bolos_vector {
  int mode;
  int curve;
  const char *path;
  const char *seed_key;
  const char *chain;
  const char *key;
};

/* clang-format off */
static const struct bolos_vector bolos_vectors[] = {
  {
    .mode = 2,
    .curve = 1,
    .path = "256/148'/0/0",
    .seed_key = "",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "dc7866edb7b1fb075f206577e75dd6d06d69cd57fac392c123de0f3a8cf9ab0e0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 2,
    .curve = 1,
    .path = "256/148/0/0",
    .seed_key = "",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "77b36493cc3e93d95765cee578e488f35636aff679731eb30a6d1daaf763b11e0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 2,
    .curve = 1,
    .path = "256/148/1/2",
    .seed_key = "",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "82640869c555feb779a611d815b4259b14a75b1f87ed8e6027245de9ef15abb30000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 2,
    .curve = 2,
    .path = "256/148'/1/2",
    .seed_key = "",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "385c31c09a52d08ec7e0ff0b165569c869604c6e1b887b62beb371e5c61047510000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 2,
    .curve = 1,
    .path = "256/148'/0/0",
    .seed_key = "Seed Key!",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "28733d1173df8301cf7b11a37bcd4b24ae1567265897850079e1adc925656cbd0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 2,
    .curve = 1,
    .path = "256/148/0/0",
    .seed_key = "Seed Key!",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "3c4f9a70d3813f0309549c75525a16ee8f0b7b4ba7b17bd8c3799a33defae4150000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 2,
    .curve = 1,
    .path = "256/148'/1/2",
    .seed_key = "Seed Key!",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "c2f0ccff26956ac87e1840c07aadca070c1dfe1504a9bf9f93847c98aa414e2a0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 2,
    .curve = 2,
    .path = "256/148/1/2",
    .seed_key = "Seed Key!",
    .chain = "0000000000000000000000000000000000000000000000000000000000000000",
    .key = "a6e59329f9b02ca2fcf20de027b73c93bd06af3081c0bdc97f5e2cac9b0955170000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 1,
    .path = "738197632/335544448'/0/0",
    .seed_key = "",
    .chain = "0524d4d89a04e06f0410003751306bbe7a1c4e80608433d12469fe6a92eecb43",
    .key = "f7dd8c0fb5023c6fce668c035560a40914abea017d83b792b15563d6feb1251b0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 1,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "",
    .chain = "336717d2389fd2886088c3dff5ceb66ef6064ad90e0a0d07a867064d1c074864",
    .key = "32cc0b6f100f76d724738926fd406f9b476770ba970c369318943562e70011e60000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 0,
    .path = "738197632/335544448'/0/0",
    .seed_key = "",
    .chain = "cdc590746b238933166da99295ef0de9e048c9ef0717c269616613ac14e52934",
    .key = "64461c084badf0064b2cd6c9aacf010ec073e850170bae02643d1b7f9574baf70000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 0,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "",
    .chain = "eb9bc0b0773fcbb10c4a3078c0a3bc2309ec0b4f28065d6224c5777b6b9e83cd",
    .key = "3cbf2b3d850e1a8f47b35271d0156ca482debef29bdfa5285386d47eefb872040000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 2,
    .path = "4294967295'",
    .seed_key = "",
    .chain = "f5977507899c96ab6d5f63ae730a0f2235cb98b3bced3549a10e19bd1297012d",
    .key = "9855268c84d2eca6266238063eee03ac5d1a44c83eb9341924d5559b19e69d41d6f0b87c340f994230fac06e6bf5b87a0f3f584f9bf157f9845c7545e8d8eb78"
  },
  {
    .mode = 0,
    .curve = 1,
    .path = "738197632/335544448'/0/0",
    .seed_key = "Seed Key!",
    .chain = "be7f1cc8587fbee3ffd42d28df5c7ec3d4e15f8e049d67e28af5ab80234a4681",
    .key = "89890b9ffb3aa9d00cc0b1e58f83724f27f204c498dcfe063ec33b42b86abf860000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 1,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "Seed Key!",
    .chain = "1e87e79f024ab9f3cd02ce197b32a43df9eaf3bdfe05ceb061f2614824efdc17",
    .key = "17d2bbf89decdbad2fe8d5bb50ca253b06382231cf4bcf20e0ff73cfb332c78e0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 0,
    .path = "738197632/335544448'/0/0",
    .seed_key = "Seed Key!",
    .chain = "43c105edbe7d227508fa448d58857ee43f11b490b2709c4769d0ca138a893500",
    .key = "3d43f2a1803099b9a29ed136d48e89cdb85bb6c9a05d87bae8f695810de2dc840000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 0,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "Seed Key!",
    .chain = "cda9bf240372898c7cb1cb42f74934c530b8a494d7e090162e797009799d64a2",
    .key = "f00afad739969f6599f289e69b19b50f6b80f4ad6c0b1f62e25639664c14e9240000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 2,
    .path = "4294967295'",
    .seed_key = "Seed Key!",
    .chain = "16026ac1dc60fcdad4b37e9762f5a48cbd1347737284a028e611877771467b61",
    .key = "a8afc87bcd088d9d59c0649738746672758cc44bc09422c6be7c47cf7e0e444b37676f1b440627a32f7a36ae93c87e07ae747c926cd7ff4fc0984015b0f72bfb"
  },
  {
    .mode = 0,
    .curve = 2,
    .path = "4294967295'",
    .seed_key = "Seed Key!",
    .chain = "16026ac1dc60fcdad4b37e9762f5a48cbd1347737284a028e611877771467b61",
    .key = "a8afc87bcd088d9d59c0649738746672758cc44bc09422c6be7c47cf7e0e444b37676f1b440627a32f7a36ae93c87e07ae747c926cd7ff4fc0984015b0f72bfb"
  },
  {
    .mode = 0,
    .curve = 2,
    .path = "44'/148'/0/0",
    .seed_key = "",
    .chain = "3e7bea48e16cef3f4e67460e0297557f2df425d52aff8fca1c1ad2b976c57afb",
    .key = "e0176952fb330646b3126ee91aa73bf44b4bf18f17ce4ac3a74a53d120e69d413ca5c28877f73b21fb01ebe7799233f40bde61a058fb9a9facfc10ac695c0fc9"
  },
  {
    .mode = 0,
    .curve = 2,
    .path = "44'/148/1/2",
    .seed_key = "",
    .chain = "40418c80e21c94bddfcab2318f9a82fb65739c687e09fee15c172778b04cc1af",
    .key = "207c589bf02f2cbfadaf8024e1dd27a5b77f01571ddb40c4ee78d7d329e69d41a6045bdebc8c836298881804ff424ee187c4141f49f8fda253f8ac3df2cda6bf"
  },
  {
    .mode = 0,
    .curve = 2,
    .path = "44'/148'/0/0",
    .seed_key = "Seed Key!",
    .chain = "a213a2083f9577b519ed1e9312b48b78f0355007b2006637f1f1a6966ff20696",
    .key = "489813484bfcc8c482db67b05eafce7419638098c2d936b893f226548a0e444b5222cb7ea2b90acefee525bb9323ecc7975237837197ed1b1696d19b4ad6fa8b"
  },
  {
    .mode = 0,
    .curve = 2,
    .path = "44'/148/1/2",
    .seed_key = "Seed Key!",
    .chain = "09e4fe1d402e6e70e9084bdb4e9da9ffa99c670c2670e82167e543b6f687a09c",
    .key = "f0aea8cc48adb7929f6c7456dc4a591a30210fe8af67cd138ffc944a8d0e444b0d401c6e0de47decc51bd9285600bd74b1476c3f320551720007c98042656295"
  },
  {
    .mode = 0,
    .curve = 1,
    .path = "44'/148'/0/0",
    .seed_key = "",
    .chain = "8fff97b457b717b0cad899d2818b5d43165f350f31c5d5598cc71bbde707d604",
    .key = "002e694a441a412fc0ca8c3a6dcc27a5da20f69341490bb27cddcd63db5b90ce0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 1,
    .path = "44'/148/1/2",
    .seed_key = "",
    .chain = "7571c62ce4bb03daa49ca3b2fc664687d42e13f67c8eaa6d0654944753b3e158",
    .key = "21bff6fea92f925a32c44617d1ef36bebf5d4657e060709aa6bd888145b1ffdb0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 0,
    .path = "44'/148'/0/0",
    .seed_key = "Seed Key!",
    .chain = "ba47004dd60d1935d7f57b13982ad9700e81144ab3e66dda3fd2ec4aa856b814",
    .key = "b47cb41d578dabcfd72a05166fe1cf2f0f3f6aa81f10268e4aa6e4720df0a8240000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 0,
    .curve = 0,
    .path = "44'/148/1/2",
    .seed_key = "Seed Key!",
    .chain = "67866455728d803b95a055357c803ec205c145be7b459093de4d339a222290ec",
    .key = "f206f57b156380cfe0d48e419bc35c56b4e3e31512c0332c58621aa409aedbb00000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 1,
    .path = "738197632/335544448'/0/0",
    .seed_key = "",
    .chain = "0524d4d89a04e06f0410003751306bbe7a1c4e80608433d12469fe6a92eecb43",
    .key = "f7dd8c0fb5023c6fce668c035560a40914abea017d83b792b15563d6feb1251b0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 1,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "",
    .chain = "336717d2389fd2886088c3dff5ceb66ef6064ad90e0a0d07a867064d1c074864",
    .key = "32cc0b6f100f76d724738926fd406f9b476770ba970c369318943562e70011e60000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 0,
    .path = "738197632/335544448'/0/0",
    .seed_key = "",
    .chain = "cdc590746b238933166da99295ef0de9e048c9ef0717c269616613ac14e52934",
    .key = "64461c084badf0064b2cd6c9aacf010ec073e850170bae02643d1b7f9574baf70000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 0,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "",
    .chain = "eb9bc0b0773fcbb10c4a3078c0a3bc2309ec0b4f28065d6224c5777b6b9e83cd",
    .key = "3cbf2b3d850e1a8f47b35271d0156ca482debef29bdfa5285386d47eefb872040000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 1,
    .path = "738197632/335544448'/0/0",
    .seed_key = "Seed Key!",
    .chain = "be7f1cc8587fbee3ffd42d28df5c7ec3d4e15f8e049d67e28af5ab80234a4681",
    .key = "89890b9ffb3aa9d00cc0b1e58f83724f27f204c498dcfe063ec33b42b86abf860000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 1,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "Seed Key!",
    .chain = "1e87e79f024ab9f3cd02ce197b32a43df9eaf3bdfe05ceb061f2614824efdc17",
    .key = "17d2bbf89decdbad2fe8d5bb50ca253b06382231cf4bcf20e0ff73cfb332c78e0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 0,
    .path = "738197632/335544448'/0/0",
    .seed_key = "Seed Key!",
    .chain = "43c105edbe7d227508fa448d58857ee43f11b490b2709c4769d0ca138a893500",
    .key = "3d43f2a1803099b9a29ed136d48e89cdb85bb6c9a05d87bae8f695810de2dc840000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 0,
    .path = "738197504/335544320'/16777216/33554432",
    .seed_key = "Seed Key!",
    .chain = "cda9bf240372898c7cb1cb42f74934c530b8a494d7e090162e797009799d64a2",
    .key = "f00afad739969f6599f289e69b19b50f6b80f4ad6c0b1f62e25639664c14e9240000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 1,
    .path = "44'/148'/0/0",
    .seed_key = "",
    .chain = "8fff97b457b717b0cad899d2818b5d43165f350f31c5d5598cc71bbde707d604",
    .key = "002e694a441a412fc0ca8c3a6dcc27a5da20f69341490bb27cddcd63db5b90ce0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 0,
    .path = "44'/148'/0/0",
    .seed_key = "",
    .chain = "bba08de0a440987023821d4d82a39c46068e8c8ebe23be0e0329a7dc6467a30d",
    .key = "7b95d7cb3fc819eb4ac356644eb3467d465d5eb5f4703f39670a03139b6cdf560000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 1,
    .path = "44'/148'/0/0",
    .seed_key = "Seed Key!",
    .chain = "1640f709333de9a4572663a80dff8d41147c5eabd02adad2607a052d19d43063",
    .key = "e9eb4cfdc8791ed95239597f4c5f34e2ac5cdb349597d98095f58b8a723e51960000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 0,
    .path = "44'/148'/0/0",
    .seed_key = "Seed Key!",
    .chain = "ba47004dd60d1935d7f57b13982ad9700e81144ab3e66dda3fd2ec4aa856b814",
    .key = "b47cb41d578dabcfd72a05166fe1cf2f0f3f6aa81f10268e4aa6e4720df0a8240000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "738197632'/335544448'/0'/0'",
    .seed_key = "",
    .chain = "b609a7af6a8ae5568bff26a3747aa0c4d8b383144db5c3da28650a37015f2503",
    .key = "9761691a62523b637c55aa267b3c4835b7cdd4bb704b399a0f7290ca570262cb0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "738197504'/335544320'/16777216'/33554432'",
    .seed_key = "",
    .chain = "1a8e8df02b17fd4632529dab6443887359ecf94d547291535952b412cba88420",
    .key = "e36a66d67ea0d2dcf9af54bc4617c0fa0724b42acff17501ac9dd27588bbd7dd0000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "738197632'/335544448'/0'/0'",
    .seed_key = "Seed Key!",
    .chain = "8448c2269776dac7e9d3d3e34e7e3d73ad540d23b51c30cb9e6a0db03dfa2109",
    .key = "5fa451f9c515c226e3fc7056863226b6a369c0721fd10df39dc6d6e1cbfcbf810000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "738197504'/335544320'/16777216'/33554432'",
    .seed_key = "Seed Key!",
    .chain = "1094951fb5c6227787c0e0abbb21780f4b90139cae24b7668f38887042a11adf",
    .key = "dc3334c721aad27324747a46b546bd762a4569e3afeee6d9d82dbb7a4b3fbf430000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "44'/148'/0'/0'",
    .seed_key = "",
    .chain = "ad38cb3640dd5a1e7540030761ec7ade17a8b38a203c37072647ec22eea7a3ba",
    .key = "a044cf4dcc4c6206d64ea3a7dae79337afcd61808dc6239a22c1ba1f4618c0550000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "44'/148'/1'/2'",
    .seed_key = "",
    .chain = "fbc472b0a324f71f264c6b002524a93a690a3d9fd130c9ca949d0ccc1e37b07e",
    .key = "889fc3bc31029c0f09eb6a24f1617af15b919dc9a6b3caac3c57383da094a1570000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "44'/148'/0'/0'",
    .seed_key = "Seed Key!",
    .chain = "5adfad756c48e9e1609a7aac3cf9a2b0b74b58ce15656f67ad6140a005eece76",
    .key = "4f7230415199467747c14b54fd774dbf20990d00e16198302471d11a319e67c40000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "44'/148'/1'/2'",
    .seed_key = "Seed Key!",
    .chain = "6add42bce9e59be732d71642e3e4e3463876b6baab98ea774121fe1caf00a1ce",
    .key = "4b3b4286aa8a4d330dbea15d0894a5194b87058ac8f58512307581b101e3be1c0000000000000000000000000000000000000000000000000000000000000000"
  }
};
/* clang-format on */

/* https://github.com/satoshilabs/slips/blob/master/slip-0010.md#test-vector-1-for-ed25519
 */
/* clang-format off */
static const struct bolos_vector slip10_vectors[] = {
  {
    .mode = 1,
    .curve = 2,
    .path = "",
    .seed_key = "",
    .chain = "90046a93de5380a72b5e45010748567d5ea02bbf6522f979e05c0d8d8ca9fffb",
    .key = "2b4be7f19ee27bbf30c667b642d5f4aa69fd169872f8fc3059c08ebae2eb19e70000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "0'",
    .seed_key = "",
    .chain = "8b59aa11380b624e81507a27fedda59fea6d0b779a778918a2fd3590e16e9c69",
    .key = "68e0fe46dfb67e368c75379acec591dad19df3cde26e63b93a8e704f1dade7a30000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "0'/1'",
    .seed_key = "",
    .chain = "a320425f77d1b5c2505a6b1b27382b37368ee640e3557c315416801243552f14",
    .key = "b1d0bad404bf35da785a64ca1ac54b2617211d2777696fbffaf208f746ae84f20000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "0'/1'/2'",
    .seed_key = "",
    .chain = "2e69929e00b5ab250f49c3fb1c12f252de4fed2c1db88387094a0f8c4c9ccd6c",
    .key = "92a5b23c0b8a99e37d07df3fb9966917f5d06e02ddbd909c7e184371463e9fc90000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "0'/1'/2'/2'",
    .seed_key = "",
    .chain = "8f6d87f93d750e0efccda017d662a1b31a266e4a6f5993b15f5c1f07f74dd5cc",
    .key = "30d1dc7e5fc04c31219ab25a27ae00b50f6fd66622f6e9c913253d6511d1e6620000000000000000000000000000000000000000000000000000000000000000"
  },
  {
    .mode = 1,
    .curve = 2,
    .path = "0'/1'/2'/2'/1000000000'",
    .seed_key = "",
    .chain = "68789923a0cac2cd5a29172a475fe9e0fb14cd6adb5ad98a3fa70333e7afa230",
    .key = "8f94d394a8e8fd6b1bc2f3f49f5c47e385281d5c17e65324b0f62483e37e87930000000000000000000000000000000000000000000000000000000000000000"
  }
};
/* clang-format on */

static void test_bip32_vector(const bip32_test_vector *v)
{
  uint8_t expected_key[32], key[32], expected_chain_code[32], chain_code[32];
  unsigned int path[MAX_CHAIN_LEN];
  extended_private_key extkey;
  int i, path_len;

  memset(&extkey, 0, sizeof(extkey));

  assert_int_equal(setenv("SPECULOS_SEED", v->seed, 1), 0);

  for (i = 0; i < v->chain_len; i++) {
    path[i] = v->chain[i].index;
    path_len = i + 1;

    sys_os_perso_derive_node_bip32(v->curve, path, path_len, key, chain_code);

    assert_int_equal(
        hexstr2bin(v->chain[i].private_key, expected_key, sizeof(expected_key)),
        sizeof(expected_key));
    assert_int_equal(hexstr2bin(v->chain[i].chain_code, expected_chain_code,
                                sizeof(expected_chain_code)),
                     sizeof(expected_chain_code));
    assert_memory_equal(key, expected_key, sizeof(expected_key));
    assert_memory_equal(chain_code, expected_chain_code, sizeof(chain_code));
  }
}

static void test_bip32(void **state __attribute__((unused)))
{
  size_t i;

  for (i = 0; i < ARRAY_SIZE(test_vectors); i++) {
    test_bip32_vector(&test_vectors[i]);
  }
}

static void test_bolos_vector(const struct bolos_vector *v)
{
  uint8_t expected_key[64], key[64], expected_chain[32], chain[32];
  uint32_t path[MAX_CHAIN_LEN];
  unsigned int mode;
  cx_curve_t curve;
  size_t sk_length;
  ssize_t path_len;
  uint8_t *p;

  switch (v->mode) {
  case 0:
    mode = HDW_NORMAL;
    break;
  case 1:
    mode = HDW_ED25519_SLIP10;
    break;
  case 2:
    mode = HDW_SLIP21;
    break;
  default:
    mode = 0;
    fail();
    break;
  }

  switch (v->curve) {
  case 0:
    curve = CX_CURVE_SECP256R1;
    break;
  case 1:
    curve = CX_CURVE_SECP256K1;
    break;
  case 2:
    curve = CX_CURVE_Ed25519;
    break;
  default:
    curve = 0;
    fail();
    break;
  }

  if (mode == HDW_SLIP21) {
    path_len = 1 + strlen(v->path);
    assert_true((size_t)path_len < sizeof(path));
    p = (uint8_t *)path;
    p[0] = '\x00';
    memcpy(p + 1, v->path, path_len - 1);
  } else {
    path_len = get_path(v->path, path, MAX_CHAIN_LEN);
    assert_true(path_len >= 0);
  }

  sk_length = strlen(v->seed_key);

  memset(chain, 0, sizeof(chain));
  memset(key, 0, sizeof(key));

  sys_os_perso_derive_node_with_seed_key(mode, curve, path, path_len, key,
                                         chain, (uint8_t *)v->seed_key,
                                         sk_length);

  assert_int_equal(hexstr2bin(v->key, expected_key, sizeof(expected_key)),
                   sizeof(expected_key));
  assert_int_equal(hexstr2bin(v->chain, expected_chain, sizeof(expected_chain)),
                   sizeof(expected_chain));
  assert_memory_equal(key, expected_key, sizeof(expected_key));
  assert_memory_equal(chain, expected_chain, sizeof(chain));
}

static void test_derive(void **state __attribute__((unused)))
{
  unsigned int i;

  assert_int_equal(setenv("SPECULOS_SEED", default_seed, 1), 0);

  for (i = 0; i < ARRAY_SIZE(bolos_vectors); i++) {
    test_bolos_vector(&bolos_vectors[i]);
  }

  assert_int_equal(
      setenv("SPECULOS_SEED", "000102030405060708090a0b0c0d0e0f", 1), 0);

  for (i = 0; i < ARRAY_SIZE(slip10_vectors); i++) {
    test_bolos_vector(&slip10_vectors[i]);
  }
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_bip32),
    cmocka_unit_test(test_derive),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
