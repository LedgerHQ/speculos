#ifndef _CX_ERRORS_H
#define _CX_ERRORS_H
//-----------------------------------------------------------------------------
// Defines
//-----------------------------------------------------------------------------
// Those defines can be found in the SDK, in cx_errors.h file:
#define CX_CHECK(call)                                                         \
  do {                                                                         \
    error = call;                                                              \
    if (error) {                                                               \
      goto end;                                                                \
    }                                                                          \
  } while (0)

#define CX_CHECK_IGNORE_CARRY(call)                                            \
  do {                                                                         \
    error = call;                                                              \
    if (error && error != CX_CARRY) {                                          \
      goto end;                                                                \
    }                                                                          \
  } while (0)

#define CX_OK 0x00000000

#define CX_CARRY 0xFFFFFF21

#define CX_LOCKED                  0xFFFFFF81
#define CX_UNLOCKED                0xFFFFFF82
#define CX_NOT_LOCKED              0xFFFFFF83
#define CX_NOT_UNLOCKED            0xFFFFFF84
#define CX_INTERNAL_ERROR          0xFFFFFF85
#define CX_INVALID_PARAMETER_SIZE  0xFFFFFF86
#define CX_INVALID_PARAMETER_VALUE 0xFFFFFF87
#define CX_INVALID_PARAMETER       0xFFFFFF88
#define CX_NOT_INVERTIBLE          0xFFFFFF89
#define CX_OVERFLOW                0xFFFFFF8A
#define CX_MEMORY_FULL             0xFFFFFF8B
#define CX_NO_RESIDUE              0xFFFFFF8C

#define CX_EC_INFINITE_POINT 0xFFFFFF41
#define CX_EC_INVALID_POINT  0xFFFFFFA2
#define CX_EC_INVALID_CURVE  0xFFFFFFA3

typedef uint32_t cx_err_t;

/* WARNING */
/* Speculos specific codes for 'magical numbers' removing, the following doesn't
 * appear in bolos. */
/*for old 'throw versions of functions*/
#define CX_NULLSIZE 0
#define CX_KO       -1

#endif
