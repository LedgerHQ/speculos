#pragma once

#include <stddef.h>
#include <stdint.h>

#define TESTS_PATH    "../../../tests/syscalls/"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

int hex2num(char c);
int hex2byte(const char *hex);
size_t hexstr2bin(const char *hex, uint8_t *buf, size_t max_len);
