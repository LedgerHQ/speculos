#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef uint8_t bolos_bool_t;

typedef uint32_t bolos_err_t;

#define BOLOS_TRUE  0xaa
#define BOLOS_FALSE 0x55

#define BOLOS_UX_OK     0xAA
#define BOLOS_UX_CANCEL 0x55
#define BOLOS_UX_ERROR  0xD6

/* Value returned by os_ux to notify the application that the processed event
 * must be discarded and not processed by the application. Generally due to
 * handling of power management/dim/locking */
#define BOLOS_UX_IGNORE 0x97
// a modal has destroyed the display, app needs to redraw its screen
#define BOLOS_UX_REDRAW 0x69
// ux has not finished processing yet (not a final status)
#define BOLOS_UX_CONTINUE 0
