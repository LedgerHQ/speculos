#pragma once

#include <stdint.h>

#define MAX_SEED_SIZE   64
#define MAX_STRING_SIZE 128
#define MAX_CERT_SIZE   6 + 33 * 2

typedef enum {
  BOLOS_TAG_APPNAME = 0x01,
  BOLOS_TAG_APPVERSION = 0x02
} BOLOS_TAG;

typedef struct {
  size_t length;
  char name[MAX_STRING_SIZE];
} env_sized_name_t;

typedef struct {
  uint8_t length;
  uint8_t buffer[MAX_CERT_SIZE];
} env_user_certificate_t;

size_t env_get_seed(uint8_t *seed, size_t max_size);
unsigned int env_get_rng();
cx_ecfp_private_key_t *env_get_user_private_key(unsigned int index);
env_user_certificate_t *env_get_user_certificate(unsigned int index);
size_t env_get_app_tag(char *dst, size_t length, BOLOS_TAG tag);

void init_environment();
