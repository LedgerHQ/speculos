#pragma once

#include "cx.h"
#include <stddef.h>

void test_cavp_short_msg(const char *filename, cx_md_t md_type);
void test_cavp_long_msg(const char *filename, cx_md_t md_type);
void test_cavp_monte(cx_md_t md_type, uint8_t *initial_seed, const uint8_t *expected_seed);

void test_cavp_short_msg_with_size(const char *filename, cx_md_t md_type, size_t digest_size);
void test_cavp_long_msg_with_size(const char *filename, cx_md_t md_type, size_t digest_size);
void test_cavp_monte_with_size(cx_md_t md_type, uint8_t *initial_seed, const uint8_t *expected_seed, size_t digest_size);
