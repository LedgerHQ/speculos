#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#include <setjmp.h>

#define EXCEPTION             1
#define INVALID_PARAMETER     2
#define EXCEPTION_OVERFLOW    3
#define EXCEPTION_SECURITY    4
#define INVALID_CRC           5
#define INVALID_CHECKSUM      6
#define INVALID_COUNTER       7
#define NOT_SUPPORTED         8
#define INVALID_STATE         9
#define TIMEOUT               10
#define EXCEPTION_PIC         11
#define EXCEPTION_APPEXIT     12
#define EXCEPTION_IO_OVERFLOW 13
#define EXCEPTION_IO_HEADER   14
#define EXCEPTION_IO_STATE    15
#define EXCEPTION_IO_RESET    16
#define EXCEPTION_CXPORT      17
#define EXCEPTION_SYSTEM      18
#define NOT_ENOUGH_SPACE      19

/* Since there is no kernel in this emulator, THROW is identical from syscalls
 * and from the app itself. */
#define THROW(exception)	os_longjmp(exception)

struct try_context_s {
  unsigned int jmp_buf[10];
  unsigned short ex;
};

static inline __attribute__((noreturn)) void os_longjmp(unsigned int exception)
{
  struct try_context_s *current_ctx;

  __asm volatile ("mov %0, r9":"=r"(current_ctx));

  longjmp((struct __jmp_buf_tag *)current_ctx->jmp_buf, exception);
}

#endif
