#pragma once

#include <stddef.h>

unsigned short sys_cx_crc16_update(unsigned short crc, const void *buf,
                                   size_t len);
