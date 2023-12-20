#pragma once

#include <stddef.h>

#include "bolos/cx.h"

void test_cavp_short_msg(const char *filename, cx_md_t md_type);
void test_cavp_long_msg(const char *filename, cx_md_t md_type);
void test_cavp_monte(cx_md_t md_type, uint8_t *initial_seed,
                     const uint8_t *expected_seed);

void test_cavp_short_msg_with_size(const char *filename, cx_md_t md_type,
                                   size_t digest_size);
void test_cavp_long_msg_with_size(const char *filename, cx_md_t md_type,
                                  size_t digest_size);
void test_cavp_monte_with_size(cx_md_t md_type, uint8_t *initial_seed,
                               const uint8_t *expected_seed,
                               size_t digest_size);

/* Test with a single hash function, that combines init, update and final */
typedef int (*single_hash_t)(const unsigned char *in, unsigned int len,
                             unsigned char *out, unsigned int out_len);
void test_cavp_short_msg_with_single(const char *filename, single_hash_t hash,
                                     size_t digest_size);
