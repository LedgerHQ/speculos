#pragma once

typedef enum {
  SDK_NANO_X_1_2,
  SDK_NANO_S_1_5,
  SDK_NANO_S_1_6,
  SDK_NANO_S_2_0,
  SDK_BLUE_2_2_5,
  SDK_COUNT
} sdk_version_t;

extern sdk_version_t sdk_version;
