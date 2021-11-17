#include <err.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <cmocka.h>

#include "bolos/cx_ec.h"
#include "bolos/cx_utils.h"
#include "emulate.h"
#include "utils.h"

#define MAX_PATH_LEN 10

typedef struct {
  cx_curve_t curve;
  const char *seed;
  const char *path;
  const char *master_key;
  const char *private_key;
} eip2333_test_vector;

/* abandon abandon abandon abandon abandon abandon abandon abandon abandon
 * abandon abandon about */
static const char *default_seed =
    "5eb00bbddcf069084889a8ab9155568165f5c453ccb85e70811aaed6f6da5fc19a5ac40b38"
    "9cd370d086206dec8aa6c43daea6690f20ad3d8d48b2d2ce9e38e4";

static const eip2333_test_vector test_vectors[] = {
  { .curve = CX_CURVE_BLS12_381_G1,
    .seed = "c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e53495531f"
            "09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04",
    .path = "0",
    .master_key =
        "d7359d57963ab8fbbde1852dcf553fedbc31f464d80ee7d40ae683122b45070",
    .private_key =
        "2d18bd6c14e6d15bf8b5085c9b74f3daae3b03cc2014770a599d8c1539e50f8e" },
  { .curve = CX_CURVE_BLS12_381_G1,
    .seed = "3141592653589793238462643383279502884197169399375105820974944592",
    .path = "3141592653",
    .master_key =
        "41c9e07822b092a93fd6797396338c3ada4170cc81829fdfce6b5d34bd5e7ec7",
    .private_key =
        "384843fad5f3d777ea39de3e47a8f999ae91f89e42bffa993d91d9782d152a0f" },
  { .curve = CX_CURVE_BLS12_381_G1,
    .seed = "0099FF991111002299DD7744EE3355BBDD8844115566CC55663355668888CC00",
    .path = "4294967295",
    .master_key =
        "3cfa341ab3910a7d00d933d8f7c4fe87c91798a0397421d6b19fd5b815132e80",
    .private_key =
        "40e86285582f35b28821340f6a53b448588efa575bc4d88c32ef8567b8d9479b" },
  { .curve = CX_CURVE_BLS12_381_G1,
    .seed = "d4e56740f876aef8c010b86a40d5f56745a118d0906a34e69aec8c0db1cb8fa3",
    .path = "42",
    .master_key =
        "2a0e28ffa5fbbe2f8e7aad4ed94f745d6bf755c51182e119bb1694fe61d3afca",
    .private_key =
        "455c0dc9fccb3395825d92a60d2672d69416be1c2578a87a7a3d3ced11ebb88d" },
};

typedef struct {
  cx_curve_t curve;
  const char *path;
  const char *master_key;
  const char *private_key;
  int path_len;
} bolos_test_vector;

static const bolos_test_vector bolos_test_vectors[] = {
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "2279076436",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "3ce8d66c9a88bb69965b3fadd6fd6a082e229d83785b8eb27f870130f4119a01",
    .path_len = 1 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "1790007780",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "217c7e19bebdf5d75663053af019ff749308cf48342a7b68fd7b8c646454ae75",
    .path_len = 1 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "402925204/2022908491/811087075/4219449576/1359796551/1875190447/"
            "3319944253/1156799604/1048845022",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "03eceb8c92d370a5ddf18b70eb0f6efc9c9ff4a5a232c855d53687db54093e50",
    .path_len = 9 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "2605148735/1656345783/3789526568/1614004818/136402010",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "6bc611a74c6af0253672eb8d5b509445a28cb7ea0da0df1f2f282d984a56703b",
    .path_len = 5 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "2532562383/3454788701",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "06dadb36cb7ca0ab0e8feda712c5e24dd2d349acb483016f160632ab08a123cc",
    .path_len = 2 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "1790266083/3443467625/2148519483",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "3fbc0fdb099a09a4e87d382ce66c2ab2b77a28289149e8239f2cfbabec1fef55",
    .path_len = 3 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "3459928194/1125685408/883337204/2864731307/3154651183/4108799398/"
            "919357545/2643733504/3462197333",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "665a1e627507716dcc6450df784d5a5ad2a6dee9915cbe748cb25062cc5876fa",
    .path_len = 9 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "3102405809/3717939214/403135262/712451447/3581393835/2379203608/"
            "3531344477/2878389195/321709481/1995844076",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "51b8b07bcc45b91c9d1fc6bd40ea19a74c6fbfb9a65d17b8148fe5af7859ad3e",
    .path_len = 10 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "2573716869/726225336/282572390/3491052743/3295449708/2378277868/"
            "1744489832/2391039873/2595430532",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "2ed85d9522e2b1e6a614901862d0547086455c632d375d4a9cd9a90bcdce8c87",
    .path_len = 9 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "3010300252/2741121125",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "362198470e99101c3e52b7855f33b3e173f95047ca445ece7bca86d805e043fa",
    .path_len = 2 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "4035565060/1099291519/2304280193/948468110/2716464225/3624499214/"
            "419894182/2790172958/211514137",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "5b78478ce9fb2c2aa4ec74776a2986bde881d96c04132ea9c08b1889dea9132f",
    .path_len = 9 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "3271528109/4135414213/1509797200",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "6dbe188e3e5f0b8fd5b83314924066ae4f43993f89d8dbaa36ed6edf237eaabe",
    .path_len = 3 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "835454411/3976701487/1774895332/1691472163/1650639831/2590215707/"
            "4082910115/3320782262/1914822140",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "61609898a95a58e0e82f3079e7b9d544a7c3bfb1764de89a5b823d8c07b23fec",
    .path_len = 9 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "3402326874/1631771538/3926620064/1135208208",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "3edb490e91c5ffdc054859272ce44a848c6e63d9e9caf90bac1ae099576ab97c",
    .path_len = 4 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "1133795433/843579193/2474203142/1396595878/2149462513",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "1c508b7c2e9c0cec8ae6fd6814cae3f386af8135e07445925f9b2d8fadbdf08e",
    .path_len = 5 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "2555287527/3388587387/3107743649/1989740811/3273976259/1516162297/"
            "2126678017",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "6e7d4b99701b44b1c3690a01dd603641a48a4c657cc72391a408da994db67d29",
    .path_len = 7 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "1018688040/1665183640/2867624941/672451332/2828286738/1242107022/"
            "2220222715/2326415856/1545775870/4232323255",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "0097ebe70b6bf42fdb3246fc9d2a9ac710bcd877fd0c349314ef2805a1021482",
    .path_len = 10 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "1366738206/3493596613/1309807041/1297120969/1523821670/2845978711/"
            "63646400/1173594587",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "4762a085a100411d5d4675938726dfd944826a90e75d620fff01cfba065b9135",
    .path_len = 8 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "1717688574/1427918720/2117515491/3667484657/333362278",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "05043ba7731cd27775017fe409d9632f638c2923ba7a15091d4d2c310a6b0896",
    .path_len = 5 },
  { .curve = CX_CURVE_BLS12_381_G1,
    .path = "1429896867/3196812878",
    .master_key =
        "528ca2018585232a3212491b7e015a6b3d3d7a2b5144c2ea6ab761d8ac8a1cff",
    .private_key =
        "55354ecb9c4d9de10cc61765334771b0963bd48c9074f675b520c9d27289a111",
    .path_len = 2 },
};

static void test_eip_vector(const eip2333_test_vector *v)
{
  uint8_t expected_key[32], key[32];
  unsigned int path[10];
  int path_len;

  assert_int_equal(setenv("SPECULOS_SEED", v->seed, 1), 0);

  path_len = get_path(v->path, path, MAX_PATH_LEN);
  assert_int_equal(path_len, 1);

  sys_os_perso_derive_eip2333(v->curve, path, path_len, key);

  assert_int_equal(
      hexstr2bin(v->private_key, expected_key, sizeof(expected_key)),
      sizeof(expected_key));
  assert_memory_equal(key, expected_key, sizeof(expected_key));
}

static void test_eip2333_derive(void **state __attribute__((unused)))
{
  unsigned int i;

  assert_int_equal(setenv("SPECULOS_SEED", default_seed, 1), 0);

  for (i = 0; i < ARRAY_SIZE(test_vectors); i++) {
    test_eip_vector(&test_vectors[i]);
  }
}

static void test_bolos_vector(const bolos_test_vector *v)
{
  uint8_t expected_key[32], key[32];
  unsigned int path[10];
  int path_len;

  path_len = get_path(v->path, path, MAX_PATH_LEN);
  assert_int_equal(path_len, v->path_len);

  sys_os_perso_derive_eip2333(v->curve, path, path_len, key);

  assert_int_equal(
      hexstr2bin(v->private_key, expected_key, sizeof(expected_key)),
      sizeof(expected_key));
  assert_memory_equal(key, expected_key, sizeof(expected_key));
}

static void test_eip2333_derive_bolos_vectors(void **state
                                              __attribute__((unused)))
{
  unsigned int i;

  assert_int_equal(setenv("SPECULOS_SEED", default_seed, 1), 0);

  for (i = 0; i < ARRAY_SIZE(bolos_test_vectors); i++) {
    test_bolos_vector(&bolos_test_vectors[i]);
  }
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_eip2333_derive),
                                      cmocka_unit_test(
                                          test_eip2333_derive_bolos_vectors) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
