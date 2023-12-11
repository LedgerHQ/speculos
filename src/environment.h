#pragma once

#include <stdint.h>

#define MAX_SEED_SIZE 64

typedef struct {
    uint8_t length;
    uint8_t buffer[];
} env_user_certificate_t;

size_t env_get_seed(uint8_t *seed, size_t max_size);
unsigned int env_get_rng();
cx_ecfp_private_key_t* env_get_user_private_key(unsigned int index);
env_user_certificate_t* env_get_user_certificate(unsigned int index);

void init_environment();
