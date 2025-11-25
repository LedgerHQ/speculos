#pragma once

#include "errors.h"
#include <stdio.h>
#include <unistd.h>

// error type definition
typedef unsigned short exception_t;

// this is a custom implementation of setjmp and longjmp
typedef unsigned int jmp_buf_t[10];

typedef struct try_context_s {
  jmp_buf_t jmp_buf;

  // link to the previous jmp_buf context
  struct try_context_s *previous;

  // current exception
  exception_t ex;
} try_context_t;

#define THROW(x) os_longjmp(x)

void custom_longjmp(jmp_buf_t buf_addr, int val)
    __attribute__((naked, noreturn));
int custom_setjmp(jmp_buf_t buf_addr) __attribute__((naked));

// longjmp is marked as no return to avoid too much generated code
void os_longjmp(unsigned int exception) __attribute__((noreturn));

try_context_t *sys_try_context_set(try_context_t *context);
try_context_t *sys_try_context_get(void);
