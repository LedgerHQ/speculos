#pragma once

#include <signal.h>
#include <stdbool.h>
#include <stddef.h>

/* r0-r3, r12, lr, pc, xpsr stacked on the app stack by the SVC */
#define SVC_STACK_FRAME_SIZE (8 * sizeof(unsigned long))

extern bool trace_syscalls;

int patch_svc(void *p, size_t size);
int patch_svc_instr(unsigned char *addr);
void save_current_context(struct sigcontext *sigcontext);
void replace_current_context(struct sigcontext *sigcontext);
void setup_context(unsigned long parameters, unsigned long f);
int setup_signals(void);
