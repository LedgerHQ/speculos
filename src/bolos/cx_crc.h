#pragma once

#include <stddef.h>

unsigned short sys_cx_crc16_update(unsigned short crc, const void *buf,
                                   size_t len);

unsigned int sys_cx_crc32_update(unsigned int crc, const void *buf, size_t len);
