#pragma once

#include <stdint.h>

/* for cx_curve_t */
#include "cx.h"

#define HDW_NORMAL         0
#define HDW_ED25519_SLIP10 1
#define HDW_SLIP21         2

typedef struct {
  uint8_t private_key[64];
  uint8_t chain_code[32];
} extended_private_key;

void expand_seed_bip32(const cx_curve_domain_t *domain, uint8_t *seed,
                       unsigned int seed_length, extended_private_key *key);
