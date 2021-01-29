#pragma once

#include <stdint.h>

// Integers are in big-endian form
int scalarmult_curve25519(uint8_t out[32], const uint8_t scalar[32],
                          const uint8_t point[32]);
