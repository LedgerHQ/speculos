#define _GNU_SOURCE /* for memmem() */
#include <err.h>
#include <errno.h>
#include <execinfo.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "emulate.h"
#include "svc.h"

#define HANDLER_STACK_SIZE (SIGSTKSZ * 4)

static ucontext_t *context;
static unsigned long *svc_addr = NULL;
static unsigned int n_svc_call = 0;

bool trace_syscalls;

void save_current_context(struct sigcontext *sigcontext)
{
  memcpy(sigcontext, &context->uc_mcontext, sizeof(*sigcontext));
}

void replace_current_context(struct sigcontext *sigcontext)
{
  memcpy(&context->uc_mcontext, sigcontext, sizeof(*sigcontext));
}

void setup_context(unsigned long parameters, unsigned long f)
{
  context->uc_mcontext.arm_r0 = parameters;
  context->uc_mcontext.arm_pc = f;
}

static void crash_handler(int sig_no)
{
  void *array[10];
  size_t size;

  size = backtrace(array, 10);

  fprintf(stderr, "[-] The app crashed with signal %d\n", sig_no);
  backtrace_symbols_fd(array, size, STDERR_FILENO);

  _exit(1);
}

static bool is_syscall_instruction(unsigned long addr)
{
  unsigned int i;

  for (i = 0; i < n_svc_call; i++) {
    if (svc_addr[i] == addr) {
      return true;
    }
  }

  return false;
}

/*
 * The SVC instruction pushes eight registers to the SP (and after returning
 * from the ISR, the processor automatically pops the eight registers from the
 * stack).
 *
 * Mimic this behavior to ensure that subtle stack overflow bugs happening on
 * real devices can be reproduced.
 */
static void update_svc_stack(bool push)
{
  unsigned long *sp = (unsigned long *)context->uc_mcontext.arm_sp;

  if (push) {
    *--sp = context->uc_mcontext.arm_cpsr;
    *--sp = context->uc_mcontext.arm_pc;
    *--sp = context->uc_mcontext.arm_lr;
    *--sp = context->uc_mcontext.arm_ip;
    *--sp = context->uc_mcontext.arm_r3;
    *--sp = context->uc_mcontext.arm_r2;
    *--sp = context->uc_mcontext.arm_r1;
    *--sp = context->uc_mcontext.arm_r0;
    context->uc_mcontext.arm_sp -= 8 * sizeof(unsigned long);

    /* the OS also saves these registers on the stack */
    *--sp = context->uc_mcontext.arm_fp;
    *--sp = context->uc_mcontext.arm_r10;
    *--sp = context->uc_mcontext.arm_r9;
    *--sp = context->uc_mcontext.arm_r8;
    *--sp = context->uc_mcontext.arm_r7;
    *--sp = context->uc_mcontext.arm_r6;
    *--sp = context->uc_mcontext.arm_r5;
    *--sp = context->uc_mcontext.arm_r4;
    context->uc_mcontext.arm_sp -= 8 * sizeof(unsigned long);
  } else {
    context->uc_mcontext.arm_sp += 16 * sizeof(unsigned long);
  }
}

static void sigill_handler(int sig_no, siginfo_t *UNUSED(info), void *vcontext)
{
  unsigned long pc, syscall, ret;
  unsigned long *parameters;
  int retid;

  context = (ucontext_t *)vcontext;
  syscall = context->uc_mcontext.arm_r0;
  parameters = (unsigned long *)context->uc_mcontext.arm_r1;
  pc = context->uc_mcontext.arm_pc;

  if (!is_syscall_instruction(context->uc_mcontext.arm_pc)) {
    fprintf(stderr, "[*] unhandled instruction at pc 0x%08lx\n", pc);
    fprintf(stderr, "    it would have triggered a crash on a real device\n");
    crash_handler(sig_no);
    _exit(1);
  }

  // fprintf(stderr, "[*] syscall: 0x%08lx (pc: 0x%08lx)\n", syscall, pc);

  update_svc_stack(true);

  ret = 0;
  retid = emulate(syscall, parameters, &ret, trace_syscalls, sdk_version);

  /* handle the os_lib_call syscall specially since it modifies the context
   * directly */
  if (sdk_version == SDK_NANO_S_1_5 || sdk_version == SDK_BLUE_1_5) {
    if (syscall == 0x6000650b) { /* SYSCALL_os_lib_call_ID_IN */
      return;
    }
  } else if (sdk_version == SDK_NANO_X_1_2 || sdk_version == SDK_NANO_X_2_0 ||
             sdk_version == SDK_NANO_X_2_0_2 || sdk_version == SDK_NANO_S_1_6 ||
             sdk_version == SDK_NANO_S_2_0 || sdk_version == SDK_NANO_S_2_1 ||
             sdk_version == SDK_BLUE_2_2_5) {
    if (syscall == 0x6000670d) { /* SYSCALL_os_lib_call_ID_IN */
      return;
    }
  } else if (sdk_version == SDK_NANO_SP_1_0 ||
             sdk_version == SDK_NANO_SP_1_0_3) {
    if (syscall == 0x01000067) { /* SYSCALL_os_lib_call_ID */
      return;
    }
  }

  if (sdk_version == SDK_NANO_S_1_5 || sdk_version == SDK_BLUE_1_5) {
    context->uc_mcontext.arm_r0 = retid;
    context->uc_mcontext.arm_r1 = ret;
  } else if (sdk_version == SDK_NANO_S_2_0 || sdk_version == SDK_NANO_S_2_1 ||
             sdk_version == SDK_NANO_X_2_0 || sdk_version == SDK_NANO_X_2_0_2 ||
             sdk_version == SDK_NANO_SP_1_0 ||
             sdk_version == SDK_NANO_SP_1_0_3) {
    context->uc_mcontext.arm_r0 = ret;
    context->uc_mcontext.arm_r1 = 0;
  } else {
    parameters[0] = retid;
    parameters[1] = ret;
  }

  /* skip undefined (originally svc) instruction */
  context->uc_mcontext.arm_pc += 2;

  update_svc_stack(false);
}

static int setup_alternate_stack(void)
{
  size_t page_size, size;
  stack_t ss = {};
  char *mem;

  /* HANDLER_STACK_SIZE + 2 guard pages before/after */
  page_size = sysconf(_SC_PAGESIZE);
  size = HANDLER_STACK_SIZE + 2 * page_size;
  mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  if (mem == MAP_FAILED) {
    warn("failed to mmap alternate stack pages");
    return -1;
  }

  if (mprotect(mem, page_size, PROT_NONE) != 0 ||
      mprotect(mem + page_size + HANDLER_STACK_SIZE, page_size, PROT_NONE) !=
          0) {
    warn("mprotect guard pages");
    munmap(mem, size);
    return -1;
  }

  ss.ss_sp = mem + page_size;
  ss.ss_size = HANDLER_STACK_SIZE;
  ss.ss_flags = 0;

  if (sigaltstack(&ss, NULL) != 0) {
    warn("sigaltstack");
    munmap(mem, size);
    return -1;
  }

  return 0;
}

int setup_signals(void)
{
  struct sigaction sig_action;

  memset(&sig_action, 0, sizeof(sig_action));

  sig_action.sa_sigaction = sigill_handler;
  sig_action.sa_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
  sigemptyset(&sig_action.sa_mask);

  if (setup_alternate_stack() != 0) {
    warn("setup_alternate_stack()");
    return -1;
  }

  if (sigaction(SIGILL, &sig_action, 0) != 0) {
    warn("sigaction(SIGILL)");
    return -1;
  }

  if (signal(SIGSEGV, &crash_handler) == SIG_ERR) {
    warn("signal(SIGSEGV");
    return -1;
  }

  return 0;
}

/*
 * Replace the SVC instruction with an undefined instruction.
 *
 * It generates a SIGILL upon execution, which is caught to handle that
 * syscall.
 */
int patch_svc(void *p, size_t size)
{
  unsigned char *addr, *end, *next;
  int ret;

  if (mprotect(p, size, PROT_READ | PROT_WRITE) != 0) {
    warn("mprotect(PROT_READ | PROT_WRITE)");
    return -1;
  }

  addr = p;
  end = addr + size;
  ret = 0;
  while (addr < end - 2) {
    next = memmem(addr, end - addr, "\x01\xdf", 2);
    if (next == NULL) {
      break;
    }

    /* instructions are aligned on 2 bytes */
    if ((unsigned long)next & 1) {
      addr = (unsigned char *)next + 1;
      continue;
    }

    svc_addr = realloc(svc_addr, (n_svc_call + 1) * sizeof(unsigned long));
    if (svc_addr == NULL) {
      err(1, "realloc");
    }
    svc_addr[n_svc_call] = (unsigned long)next;

    /* undefined instruction */
    memcpy(next, "\xff\xde", 2);

    fprintf(stderr, "[*] patching svc instruction at %p\n", next);

    addr = (unsigned char *)next + 2;
    n_svc_call++;
  }

  if (mprotect(p, size, PROT_READ | PROT_EXEC) != 0) {
    warn("mprotect(PROT_READ | PROT_EXEC)");
    return -1;
  }

  if (n_svc_call == 0) {
    warnx("failed to find SVC_call");
    return -1;
  }

  return ret;
}
