/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include "decorators.h"

#ifdef HAVE_BAGL
#include "ux_bagl.h"
#endif // HAVE_BAGL

#ifdef HAVE_SERIALIZED_NBGL
#include "nbgl_serialize.h"
#endif // HAVE_SERIALIZED_NBGL

#include <stddef.h>
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
  OS_IO_TOUCH_AREA_NONE = 0,
  OS_IO_TOUCH_AREA_LEFT_BORDER = 1,
  OS_IO_TOUCH_AREA_RIGHT_BORDER = 2,
  OS_IO_TOUCH_AREA_TOP_BORDER = 4,
  OS_IO_TOUCH_AREA_BOTTOM_BORDER = 8,
} os_io_touch_area;

typedef enum {
  OS_IO_TOUCH_DEBUG_END = 0,
  OS_IO_TOUCH_DEBUG_READ_RAW_DATA = 1,
  OS_IO_TOUCH_DEBUG_READ_DIFF_DATA = 2,
  OS_IO_TOUCH_DEBUG_READ_SENSOR_BUFFER = 3,
} os_io_touch_debug_mode_t;

/* Exported types, structures, unions ----------------------------------------*/

typedef struct io_touch_info_s {
  uint16_t x;
  uint16_t y;
  uint8_t state;
  uint8_t w;
  uint8_t h;
  uint8_t swipe;
} io_touch_info_t;

/* Exported defines   --------------------------------------------------------*/
#ifdef HAVE_SE_TOUCH
#define SERIALIZED_NBGL_MAX_LEN (200)
#else // !HAVE_SE_TOUCH
#define SERIALIZED_NBGL_MAX_LEN (110)
#endif // HAVE_SE_TOUCH

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
void io_seph_ux_init_button(void);
int io_process_itc_ux_event(uint8_t *buffer_in, size_t buffer_in_length);
void io_process_ux_event(uint8_t *buffer_in, size_t buffer_in_length);

#ifdef HAVE_BAGL
void io_seph_ux_display_bagl_element(const bagl_element_t *element);
#endif // HAVE_BAGL

#ifdef HAVE_SERIALIZED_NBGL
void io_seph_ux_send_nbgl_serialized(nbgl_serialized_event_type_e event,
                                     nbgl_obj_t *obj);
#endif // HAVE_SERIALIZED_NBGL

#ifdef HAVE_SE_TOUCH
SYSCALL void touch_get_last_info(io_touch_info_t *info);
SYSCALL void touch_set_state(bool enable);
SYSCALL uint8_t touch_exclude_borders(uint8_t excluded_borders);
#ifdef HAVE_TOUCH_READ_SENSI_SYSCALL
SYSCALL void touch_read_sensitivity(uint8_t *sensi_data);
#endif // HAVE_TOUCH_READ_SENSI_SYSCALL
#endif // HAVE_SE_TOUCH
