#include <err.h>
#include <string.h>

#include "emulate.h"
#include "touch.h"

#define SEPROXYHAL_TAG_FINGER_EVENT 0x0C

// Last touch infos sent by seph.
// These infos can be returned by touch_get_last_infos syscall.
static io_touch_info_t last_touch_infos;

void catch_touch_info_from_seph(uint8_t *buffer, uint16_t size)
{
  if (size < 5) {
    return;
  }

  if (buffer[0] != SEPROXYHAL_TAG_FINGER_EVENT) {
    return;
  }

  last_touch_infos.state = buffer[3];
  last_touch_infos.x = (buffer[4] << 8) + buffer[5];
  last_touch_infos.y = (buffer[6] << 8) + buffer[7];
}

unsigned long sys_touch_get_last_info(io_touch_info_t *info)
{
  memcpy(info, &last_touch_infos, sizeof(io_touch_info_t));
  return 0;
}
