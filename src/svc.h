#pragma once

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

extern bool trace_syscalls;

int patch_svc(void *p, size_t size);
void save_current_context(struct sigcontext *sigcontext);
void replace_current_context(struct sigcontext *sigcontext);
void setup_context(unsigned long parameters, unsigned long f);
int setup_signals(void);
