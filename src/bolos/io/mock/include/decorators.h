#pragma once

#ifndef SYSCALL
// #define SYSCALL syscall
#define SYSCALL
#endif

#ifndef TASKSWITCH
// #define TASKSWITCH taskswitch
#define TASKSWITCH
#endif

#ifndef SUDOCALL
// #define SUDOCALL sudocall
#define SUDOCALL
#endif

#ifndef LIBCALL
// #define LIBCALL libcall
#define LIBCALL
#endif

#ifndef SHARED
// #define SHARED shared
#define SHARED
#endif

#ifndef PERMISSION
#define PERMISSION(...)
#endif

#ifndef PLENGTH
#define PLENGTH(...)
#endif

#ifndef CXPORT
#define CXPORT(...)
#endif

#ifndef TASKLEVEL
#define TASKLEVEL(...)
#endif

#ifndef OPT_NONE
#if defined(__clang__)
#define OPT_NONE __attribute__((optnone))
#elif defined(__GNUC__)
#define OPT_NONE __attribute__((optimize("O0")))
#else
#define OPT_NONE
#endif
#endif

#ifndef FALL_THROUGH
#if defined(__GNUC__) && __GNUC__ >= 7 ||                                      \
    defined(__clang__) && __clang_major__ >= 12
#define FALL_THROUGH __attribute__((fallthrough))
#else
#define FALL_THROUGH ((void)0)
#endif
#endif

#ifndef WARN_UNUSED_RESULT
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#endif

#ifndef DEPRECATED
#define DEPRECATED __attribute__((deprecated))
#endif

#ifndef UNIT_TESTING
#define STATIC static
#else
#define STATIC
#endif
