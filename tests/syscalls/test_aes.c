/*
import binascii
import os
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend

key = b'k' * 16
iv = b'\x00' * 16

backend = default_backend()
cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=backend)
encryptor = cipher.encryptor()
ct = encryptor.update(b'c' * 16 + b'd' * 16 + b'e' * 16) + encryptor.finalize()
print(binascii.hexlify(ct))
decryptor = cipher.decryptor()
dt = decryptor.update(ct) + decryptor.finalize()
print(binascii.hexlify(dt))

cipher = Cipher(algorithms.AES(key), modes.ECB(), backend=backend)
encryptor = cipher.encryptor()
ct = encryptor.update(b'c' * 16 + b'd' * 16 + b'e' * 16) + encryptor.finalize()
print(binascii.hexlify(ct))
decryptor = cipher.decryptor()
dt = decryptor.update(ct) + decryptor.finalize()
print(binascii.hexlify(dt))
*/

#include <malloc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <setjmp.h>
// must come after setjmp.h
#include <cmocka.h>

#include "bolos/cx.h"
#include "bolos/cx_aes.h"
#include "emulate.h"

#include "utils.h"

void test_aes_cbc1(void **state __attribute__((unused)))
{
  cx_aes_key_t key;
  uint8_t in[CX_AES_BLOCK_SIZE * 3], out[CX_AES_BLOCK_SIZE * 3];
  const uint8_t expected[CX_AES_BLOCK_SIZE * 3] = {
    0x3c, 0x8c, 0x4b, 0xa,  0xa2, 0x1b, 0x8,  0x5c, 0x53, 0x8a, 0x99, 0x45,
    0x79, 0x14, 0x2f, 0x24, 0xe9, 0x5f, 0x3c, 0xf9, 0xa9, 0xd0, 0x3d, 0xc7,
    0xa,  0xaa, 0x65, 0x6d, 0xc8, 0xcb, 0xb7, 0x7b, 0xb7, 0xcc, 0x42, 0xcf,
    0x7d, 0xf0, 0x51, 0x74, 0xf6, 0xde, 0x25, 0xb9, 0x8f, 0x94, 0x63, 0x9b
  };
  uint8_t rawkey[CX_AES_BLOCK_SIZE];
  int ret;

  memset(rawkey, 'k', sizeof(rawkey));
  memset(in + CX_AES_BLOCK_SIZE * 0, 'c', CX_AES_BLOCK_SIZE);
  memset(in + CX_AES_BLOCK_SIZE * 1, 'd', CX_AES_BLOCK_SIZE);
  memset(in + CX_AES_BLOCK_SIZE * 2, 'e', CX_AES_BLOCK_SIZE);

  sys_cx_aes_init_key(rawkey, sizeof(rawkey), &key);

  ret = sys_cx_aes(&key, CX_LAST | CX_ENCRYPT | CX_PAD_NONE | CX_CHAIN_CBC, in,
                   sizeof(in), out, sizeof(out));
  assert_int_equal(ret, sizeof(in));

  assert_memory_equal(out, expected, sizeof(expected));

  ret = sys_cx_aes(&key, CX_LAST | CX_DECRYPT | CX_PAD_NONE | CX_CHAIN_CBC, out,
                   sizeof(out), out, sizeof(out));
  assert_int_equal(ret, sizeof(out));

  assert_memory_equal(out, in, sizeof(in));
}

void test_aes_ecb1(void **state __attribute__((unused)))
{
  cx_aes_key_t key;
  uint8_t in[CX_AES_BLOCK_SIZE * 3], out[CX_AES_BLOCK_SIZE * 3];
  const uint8_t expected[CX_AES_BLOCK_SIZE * 3] = {
    0x3c, 0x8c, 0x4b, 0xa,  0xa2, 0x1b, 0x8,  0x5c, 0x53, 0x8a, 0x99, 0x45,
    0x79, 0x14, 0x2f, 0x24, 0x5a, 0x54, 0xef, 0x77, 0xf4, 0x9f, 0x28, 0x5e,
    0x48, 0xe2, 0xf5, 0x38, 0x86, 0x3e, 0xc4, 0x30, 0xce, 0x9b, 0xb1, 0x56,
    0x7a, 0xf6, 0x48, 0xbb, 0xd1, 0x72, 0xc8, 0x6b, 0xd5, 0xbf, 0x9c, 0xa7
  };
  uint8_t rawkey[CX_AES_BLOCK_SIZE];
  int ret;

  memset(rawkey, 'k', sizeof(rawkey));
  memset(in + CX_AES_BLOCK_SIZE * 0, 'c', CX_AES_BLOCK_SIZE);
  memset(in + CX_AES_BLOCK_SIZE * 1, 'd', CX_AES_BLOCK_SIZE);
  memset(in + CX_AES_BLOCK_SIZE * 2, 'e', CX_AES_BLOCK_SIZE);

  sys_cx_aes_init_key(rawkey, sizeof(rawkey), &key);

  ret = sys_cx_aes(&key, CX_LAST | CX_ENCRYPT | CX_PAD_NONE | CX_CHAIN_ECB, in,
                   sizeof(in), out, sizeof(out));
  assert_int_equal(ret, sizeof(in));

  assert_memory_equal(out, expected, sizeof(expected));
}

int main(void)
{
  const struct CMUnitTest tests[] = { cmocka_unit_test(test_aes_cbc1),
                                      cmocka_unit_test(test_aes_ecb1) };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
