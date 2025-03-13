#pragma once

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

struct app_s;

void unload_running_app(bool unload_data);
void *get_memory_code_address(void);
struct app_s *get_current_app(void);
char *get_app_nvram_file_name(void);
bool get_app_save_nvram(void);
unsigned long get_app_nvram_address(void);
unsigned long get_app_nvram_size(void);
unsigned long get_app_text_load_addr(void);
void save_current_context(struct sigcontext *sigcontext);
void replace_current_context(struct sigcontext *sigcontext);
int replace_current_code(struct app_s *app);
int run_lib(char *name, unsigned long *parameters);
