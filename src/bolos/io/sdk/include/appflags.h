#pragma once

#include <stdint.h>

/**
 * Base flag added to loaded application, to allow them to call all syscalls by
 * default (the one requiring no extra permission)
 */
#define APPLICATION_FLAG_MAIN 0x1

/**
 * Flag which combined with ::APPLICATION_FLAG_ISSUER.
 * The application is given full nvram access after the global seed has been
 * destroyed.
 */
#define APPLICATION_FLAG_BOLOS_UPGRADE 0x2

// this flag is set when a valid signature of the loaded application is
// presented at the end of the bolos application load.
#define APPLICATION_FLAG_SIGNED 0x4

// must be set on one application in the registry which is used
#define APPLICATION_FLAG_BOLOS_UX 0x8

// application is allowed to use the raw master seed, if not set, at least a
// level of derivation is required.
#define APPLICATION_FLAG_DERIVE_MASTER 0x10

#define APPLICATION_FLAG_SHARED_NVRAM 0x20
#define APPLICATION_FLAG_GLOBAL_PIN   0x40

// This flag means the application is meant to be debugged and allows for dump
// or core ARM register in case of a fault detection
#define APPLICATION_FLAG_DEBUG 0x80

/**
 * Mark this application as defaultly booting along with the bootloader (no
 * application menu displayed) Only one application can have this at a time. It
 * is managed by the bootloader interface.
 */
#define APPLICATION_FLAG_AUTOBOOT 0x100

/**
 * Application is allowed to change the settings
 */
#define APPLICATION_FLAG_BOLOS_SETTINGS 0x200

#define APPLICATION_FLAG_CUSTOM_CA 0x400

/**
 * The application main can be called in two ways:
 *  - with first arg (stored in r0) set to 0: The application is called from the
 * dashboard
 *  - with first arg (stored in r0) set to != 0 (ram address likely): The
 * application is used as a library from another app.
 */
#define APPLICATION_FLAG_LIBRARY 0x800

/**
 * The application won't be shown on the dashboard (somewhat reasonable for pure
 * library)
 */
#define APPLICATION_FLAG_NO_RUN 0x1000

#if defined(HAVE_LANGUAGE_PACK)
/**
 * This application is in fact a Language pack!
 */
#define APPLICATION_FLAG_LANGUAGE_PACK 0x2000
#endif // defined(HAVE_LANGUAGE_PACK)

/**
 * Application has been loaded using a secure channel opened using the
 * bootloader's issuer public key. This application is ledger legit.
 */
#define APPLICATION_FLAG_ISSUER 0x4000

/**
 * Application is enabled (when not being updated or removed)
 */
#define APPLICATION_FLAG_ENABLED 0x8000

/**
 * Application is actually a Stax background image
 */
#define APPLICATION_FLAG_BACKGROUND_IMG 0x10000

/**
 * Application was not yet audited by ledger
 */
#define APPLICATION_FLAG_NOT_REVIEWED 0x20000

/**
 * Reserved for future use
 */
#define APPLICATION_FLAG_RESERVED_FUTURE 0x40000

#define APPLICATION_FLAGS_MASK    (0x00000000FFFFFFFFULL)
#define APPLICATION_FLAG_NEG_MASK (0xFFFFFFFF00000000ULL)
#define APPLICATION_FLAGS_SHIFT   (32)

#define GET_COMPLEMENTED_APPLICATION_FLAGS(flags)                              \
  (((~((flags)&APPLICATION_FLAGS_MASK) << APPLICATION_FLAGS_SHIFT)) |          \
   ((flags)&APPLICATION_FLAGS_MASK))

#define IS_APPLICATION_FLAGS_SECURE(complemented_flags)                        \
  (((~(complemented_flags) << APPLICATION_FLAGS_SHIFT) &                       \
    APPLICATION_FLAG_NEG_MASK) ==                                              \
   ((complemented_flags)&APPLICATION_FLAG_NEG_MASK))

extern uint64_t app_flags;
