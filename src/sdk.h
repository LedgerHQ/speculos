#pragma once

typedef enum {
  SDK_NANO_X_1_2,
  SDK_NANO_X_2_0,
  SDK_NANO_X_2_0_2,
  SDK_NANO_S_1_5,
  SDK_NANO_S_1_6,
  SDK_NANO_S_2_0,
  SDK_NANO_S_2_1,
  SDK_BLUE_1_5,
  SDK_BLUE_2_2_5,
  SDK_NANO_SP_1_0,
  SDK_NANO_SP_1_0_3,
  // Unified SDK versions only below, do not add unrelated versions.
  // Make sure to add all API_LEVEL as apilevelstr2sdkver() relies on it
  // SDK_API_LEVEL_START
  SDK_API_LEVEL_1,
  SDK_API_LEVEL_2,
  SDK_API_LEVEL_3,
  SDK_API_LEVEL_4,
  SDK_API_LEVEL_5,
  SDK_API_LEVEL_6,
  SDK_API_LEVEL_7,
  SDK_API_LEVEL_8,
  SDK_API_LEVEL_9,
  SDK_API_LEVEL_10,
  SDK_API_LEVEL_11,
  SDK_API_LEVEL_12,
  SDK_API_LEVEL_13,
  SDK_API_LEVEL_14,
  SDK_API_LEVEL_15,
  SDK_API_LEVEL_16,
  SDK_API_LEVEL_17,
  SDK_API_LEVEL_18,
  SDK_API_LEVEL_19,
  SDK_API_LEVEL_20,
  SDK_COUNT
} sdk_version_t;

extern sdk_version_t sdk_version;

typedef enum {
  MODEL_NANO_S,
  MODEL_NANO_SP,
  MODEL_NANO_X,
  MODEL_BLUE,
  MODEL_STAX,
  MODEL_FLEX,
  MODEL_COUNT
} hw_model_t;

extern hw_model_t hw_model;
