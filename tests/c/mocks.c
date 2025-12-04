#include "emulate.h"
#include "sdk.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

hw_model_t hw_model = MODEL_COUNT;
int g_api_level = 0;

// app_flags extracted from app
uint64_t app_flags = 0xFFFFFFFF;

void *get_memory_code_address(void)
{
  return NULL;
}

struct app_s *get_current_app(void)
{
  return NULL;
}

char *get_app_nvram_file_name(void)
{
  return NULL;
}

bool get_app_save_nvram(void)
{
  return false;
}

unsigned long get_app_nvram_address(void)
{
  return 0;
}

unsigned long get_app_nvram_size(void)
{
  return 0;
}

unsigned long get_app_text_load_addr(void)
{
  return 0;
}

unsigned long get_app_derivation_path(uint8_t **derivationPath)
{
  *derivationPath = NULL;
  return 0;
}

void unload_running_app(bool UNUSED(unload_data))
{
}

int replace_current_code(struct app_s *UNUSED(app))
{
  return 0;
}

int run_lib(char *UNUSED(name), unsigned long *UNUSED(parameters))
{
  return 0;
}
