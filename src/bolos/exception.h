#pragma once

#include <stdio.h>
#include <unistd.h>

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

#define THROW(exception)                                                       \
  do {                                                                         \
    fprintf(stderr, "\nUnhandled exception %d at %s:%d\n", exception,          \
            __FILE__, __LINE__);                                               \
    fprintf(stderr, "Exiting.\n");                                             \
    _exit(1);                                                                  \
  } while (0)

typedef struct try_context_s try_context_t;

unsigned long sys_try_context_set(try_context_t *context);
unsigned long sys_try_context_get(void);
