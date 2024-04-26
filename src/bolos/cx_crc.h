#pragma once

#include <stddef.h>

#define CX_CRC32_INIT 0xFFFFFFFF

typedef enum crc_type_e {
  CRC_TYPE_CRC16_CCITT_FALSE = 0,
  CRC_TYPE_CRC32 = 1
} crc_type_t;

unsigned short sys_cx_crc16_update(unsigned short crc, const void *buf,
                                   size_t len);

unsigned int sys_cx_crc32_update(unsigned int crc, const void *buf, size_t len);
