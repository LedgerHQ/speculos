#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "cx.h"
#include "cx_utils.h"
#include "emulate.h"
#include "environment.h"
#include "exception.h"

#define MAX_SEED_SIZE          64
#define CX_SHA256_SIZE         32
#define CX_EIP2333_SEED_LENGTH 65

// -------------------- EIP-2333 key generation for BLS12-381 curves
// ------------------------- https://eips.ethereum.org/EIPS/eip-2333 This
// algorithm enables to generate a 32 bytes BLS private key It intends to
// replace the BIP32 HD derivation for BLS12-381 signatures since the generated
// key is sometimes greater than the curve order
// -------------------------------------------------------------------------------------------

// Hash a Lamport private key into a compressed Lamport public key
// Input: parent_sk = the BLS secret key of the parent node
//        index = the index of the desired child node
// Output: compressed_lamport_pk = the compressed Lamport public key
// 0. salt = I2OSP(index, 4)
// 1. IKM = I2OSP(parent_SK, 32)
// 2. lamport_0 = IKM_to_lamport_SK(IKM, salt)
// 3. not_IKM = flip_bits(IKM)
// 4. lamport_1 = IKM_to_lamport_SK(not_IKM, salt)
// 5. lamport_PK = ""
// 6. for i  in 1, .., 255
//      lamport_PK = lamport_PK | SHA256(lamport_0[i])
// 7. for i  in 1, .., 255
//      lamport_PK = lamport_PK | SHA256(lamport_1[i])
// 8. compressed_lamport_PK = SHA256(lamport_PK)
// 9. return compressed_lamport_PK

static void cx_parent_sk_to_lamport_pk(const unsigned char *parent_sk,
                                       unsigned int index,
                                       unsigned char *compressed_lamport_pk)
{
  unsigned char salt[4];
  unsigned char local_sk[KEY_LENGTH];
  unsigned char block;
  unsigned char prk[CX_SHA256_SIZE];
  unsigned char tmp[CX_SHA256_SIZE], tmp2[CX_SHA256_SIZE];
  unsigned char k;
  int i, j;
  cx_sha256_t sha256;
  cx_hmac_t hmac_ctx;

  U4BE_ENCODE(salt, 0, index);
  memcpy(local_sk, parent_sk, KEY_LENGTH);
  cx_sha256_init(&sha256);
  for (i = 0; i < 2; i++) {
    block = 1;
    if (i != 0) {
      for (k = 0; k < KEY_LENGTH; k++) {
        local_sk[k] ^= 0xff;
      }
    }
    cx_hkdf_extract(CX_SHA256, local_sk, sizeof(local_sk), salt, sizeof(salt),
                    prk);

    for (j = 1; j < 256; j++) {
      spec_cx_hmac_init(&hmac_ctx, CX_SHA256, prk, CX_SHA256_SIZE);
      if (j != 1) {
        sys_cx_hmac(&hmac_ctx, 0, tmp, CX_SHA256_SIZE, NULL, 0);
      }
      sys_cx_hmac(&hmac_ctx, CX_LAST | CX_NO_REINIT, &block, 1, tmp,
                  CX_SHA256_SIZE);
      sys_cx_hash_sha256(tmp, CX_SHA256_SIZE, tmp2, CX_SHA256_SIZE);
      sys_cx_hash((cx_hash_t *)&sha256, 0, tmp2, CX_SHA256_SIZE, NULL, 0);
      block++;
    }
  }
  sys_cx_hash((cx_hash_t *)&sha256, CX_LAST, NULL, 0, compressed_lamport_pk,
              32);
}

// Derive the secret key of the master node
// Input: seed = the source of entropy for the entire tree (at least 256 bits)
//        seed_len = the seed length
// Output: sk = the secret key of the master node

static void cx_derive_master_sk(const unsigned char *seed,
                                unsigned int seed_len, unsigned char *sk)
{
  unsigned char key_info[2] = { 0x00, 0x30 };
  unsigned char seed_intern[CX_EIP2333_SEED_LENGTH] = { 0 };

  memcpy(seed_intern, seed, seed_len);
  seed_intern[seed_len] = 0;
  cx_hkdf_mod_r(seed_intern, seed_len + 1, NULL, 0, key_info, 2, sk);
}

// Derive the secret key of the child node
// Input: parent_sk = the secret key of the parent node
//        path = path to the index of the desired child node
//        path_len = path length
// Output: child_sk = the secret key of the child node

static int cx_derive_child_sk(const unsigned char *parent_sk,
                              const unsigned int *path, unsigned int path_len,
                              unsigned char *child_sk)
{
  unsigned char lamport_pk[LAMPORT_KEY_LENGTH];
  unsigned char info[2];
  unsigned int i;

  if ((path == NULL) || (path_len < 1)) {
    warnx("eip2333 key generation: invalid path");
    return -1;
  }
  for (i = 0; i < path_len; i++) {
    cx_parent_sk_to_lamport_pk((i == 0 ? parent_sk : child_sk), path[i],
                               lamport_pk);
    lamport_pk[KEY_LENGTH] = 0;
    info[0] = 0x00;
    info[1] = 0x30;
    cx_hkdf_mod_r(lamport_pk, 33, NULL, 0, info, 2, child_sk);
  }

  return 0;
}

unsigned long sys_os_perso_derive_eip2333(cx_curve_t curve,
                                          const unsigned int *path,
                                          unsigned int pathLength,
                                          unsigned char *privateKey)
{

  unsigned char sk[KEY_LENGTH];
  ssize_t seed_size;
  uint8_t seed[MAX_SEED_SIZE];
  int ret = -1;

  // CX_CURVE_BLS12_381_G1 = 0x39 in SDK 2.0
  if ((curve != CX_CURVE_BLS12_381_G1) && (curve != 0x39)) {
    THROW(EXCEPTION);
  }

  seed_size = env_get_seed(seed, sizeof(seed));

  cx_derive_master_sk(seed, seed_size, sk);
  if (privateKey != NULL) {
    ret = cx_derive_child_sk(sk, path, pathLength, privateKey);
  }

  if (ret < 0) {
    THROW(EXCEPTION);
  }

  return 0;
}
