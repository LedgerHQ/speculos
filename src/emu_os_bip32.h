#ifndef _EMU_OS_BIP32_H
#define _EMU_OS_BIP32_H

#include <stdint.h>

/* for cx_curve_t */
#include "cx_ec.h"

#define HDW_NORMAL 0
#define HDW_ED25519_SLIP10 1
#define HDW_SLIP21 2

void expand_seed_bip32(cx_curve_t curve, uint8_t *seed, unsigned int seed_length, uint8_t *result, const cx_curve_domain_t *domain);
int unhex(uint8_t *dst, size_t dst_size, const char *src, size_t src_size);

#endif // _EMU_OS_BIP32_H
