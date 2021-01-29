#pragma once

int sys_cx_math_add(uint8_t *r, const uint8_t *a, const uint8_t *b,
                    unsigned int len);
int sys_cx_math_addm(uint8_t *r, const uint8_t *a, const uint8_t *b,
                     const uint8_t *m, unsigned int len);
int sys_cx_math_cmp(const uint8_t *a, const uint8_t *b, unsigned int len);
int sys_cx_math_invintm(uint8_t *r, uint32_t a, const uint8_t *m, size_t len);
int sys_cx_math_invprimem(uint8_t *r, const uint8_t *a, const uint8_t *m,
                          unsigned int len);
int sys_cx_math_is_prime(const uint8_t *r, unsigned int len);
int sys_cx_math_is_zero(const uint8_t *a, unsigned int len);
int sys_cx_math_modm(uint8_t *v, unsigned int len_v, const uint8_t *m,
                     unsigned int len_m);
int sys_cx_math_mult(uint8_t *r, const uint8_t *a, const uint8_t *b,
                     unsigned int len);
int sys_cx_math_multm(uint8_t *r, const uint8_t *a, const uint8_t *b,
                      const uint8_t *m, unsigned int len);
int sys_cx_math_powm(uint8_t *r, const uint8_t *a, const uint8_t *e,
                     size_t len_e, const uint8_t *m, size_t len);
int sys_cx_math_sub(uint8_t *r, const uint8_t *a, const uint8_t *b, size_t len);
int sys_cx_math_subm(uint8_t *r, const uint8_t *a, const uint8_t *b,
                     const uint8_t *m, size_t len);

#define cx_math_add     sys_cx_math_add
#define cx_math_addm    sys_cx_math_addm
#define cx_math_cmp     sys_cx_math_cmp
#define cx_math_is_zero sys_cx_math_is_zero
#define cx_math_mod     sys_cx_math_modm
#define cx_math_mult    sys_cx_math_mult
#define cx_math_multm   sys_cx_math_multm
