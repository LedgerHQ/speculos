#pragma once

#include <stdint.h>

#define MAX_SEED_SIZE 64

size_t get_seed(uint8_t *seed, size_t max_size);
void init_seed();
