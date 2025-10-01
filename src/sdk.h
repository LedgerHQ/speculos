#pragma once

#define FIRST_SUPPORTED_API_LEVEL 22
#define LAST_SUPPORTED_API_LEVEL  25

extern int g_api_level;

typedef enum {
  MODEL_NANO_SP,
  MODEL_NANO_X,
  MODEL_STAX,
  MODEL_FLEX,
  MODEL_APEX_P,
  MODEL_COUNT
} hw_model_t;

extern hw_model_t hw_model;
