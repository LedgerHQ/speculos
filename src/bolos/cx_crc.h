#pragma once

#include <stddef.h>

typedef enum {
  CRC_TYPE_CRC16_CCITT_FALSE = 0,
  CRC_TYPE_CRC32 = 1,
} crc_type_t;

uint16_t sys_cx_crc16_update(uint16_t crc, const void *buf, size_t len);
uint32_t sys_cx_crc32_update(uint32_t crc, const void *buf, size_t len);
uint32_t sys_cx_crc32_hw(const void *_buf, size_t len);
uint32_t sys_cx_crc_hw(crc_type_t crc_type, uint32_t crc_state, const void *buf,
                       size_t len);
