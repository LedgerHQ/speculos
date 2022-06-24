#pragma once

#include <stdint.h>
#include <unistd.h>

#if UINTPTR_MAX == UINT64_MAX
#define NATIVE_64BITS
#endif

#ifndef NATIVE_64BITS // NO 64BITS
/** 64bits types, native or by-hands, depending on target and/or compiler
 * support.
 * This type is defined here only because sha-3 struct used it INTENALLY.
 * It should never be directly used by other modules.
 */
struct uint64_s {
#ifdef OS_LITTLE_ENDIAN
  unsigned long int l;
  unsigned long int h;
#else
  unsigned long int h;
  unsigned long int l;
#endif
};
typedef struct uint64_s uint64bits_t;
#else
typedef unsigned long long uint64bits_t;
#endif

/* ======================================================================= */
/*                          32 BITS manipulation                           */
/* ======================================================================= */
uint32_t spec_cx_swap_uint32(uint32_t v);
void spec_cx_swap_buffer32(uint32_t *v, size_t len);

#if 0
unsigned long int cx_rotl(unsigned long int x, unsigned char n) ;
unsigned long int cx_rotr(unsigned long int x, unsigned char n) ;
unsigned long int cx_shr(unsigned long int x, unsigned char n) ;
#else
#define CX_INLINE_U32
#define cx_rotl(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
#define cx_rotr(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define cx_shr(x, n)  ((x) >> (n))
#endif

/* ======================================================================= */
/*                          64 BITS manipulation                           */
/* ======================================================================= */

#ifndef NATIVE_64BITS // NO 64BITS

#ifdef OS_LITTLE_ENDIAN
#define _64BITS(h, l)                                                          \
  {                                                                            \
    l, h                                                                       \
  }
#else
#define _64BITS(h, l)                                                          \
  {                                                                            \
    h, l                                                                       \
  }
#endif

#define CLR64(x)                                                               \
  (x).l = 0;                                                                   \
  (x).h = 0
#define ADD64(x, y) cx_add_64(&(x), &(y))
#define ASSIGN64(r, x)                                                         \
  (r).l = (x).l;                                                               \
  (r).h = (x).h

void cx_rotr64(uint64bits_t *x, unsigned int n);

void cx_shr64(uint64bits_t *x, unsigned char n);

void cx_add_64(uint64bits_t *a, uint64bits_t *b);
void spec_cx_swap_uint64(uint64bits_t *v);
void spec_cx_swap_buffer64(uint64bits_t *v, int len);

#else

#define _64BITS(h, l) (h##ULL << 32) | (l##ULL)

#if 0
uint64bits_t cx_rotr64(uint64bits_t x, unsigned int n);
uint64bits_t cx_rotl64(uint64bits_t x, unsigned int n);
uint64bits_t cx_shr64(uint64bits_t x, unsigned int n) ;
#else
#define CX_INLINE_U64

#define cx_rotr64(x, n) (((x) >> (n)) | ((x) << ((64) - (n))))

#define cx_rotl64(x, n) (((x) << (n)) | ((x) >> ((64) - (n))))

#define cx_shr64(x, n) ((x) >> (n))
#endif

uint64bits_t spec_cx_swap_uint64(uint64bits_t v);
void spec_cx_swap_buffer64(uint64bits_t *v, int len);

#endif

void be2le(uint8_t *v, size_t len);
void le2be(uint8_t *v, size_t len);

int sys_cx_math_next_prime(uint8_t *buf, unsigned int len);

int get_path(const char *str_, unsigned int *path, int max_path_len);
