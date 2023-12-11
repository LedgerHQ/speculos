#pragma once

#include <stdint.h>

#define MAX_SEED_SIZE 64

void init_env_seed();
size_t get_env_seed(uint8_t *seed, size_t max_size);

void init_env_rng();
unsigned int get_env_rng();
