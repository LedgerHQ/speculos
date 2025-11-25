#include "exception.h"

// custom impl of longjmp, that restores 10 registers from buf_addr (saved in
// custom_setjmp) if val is not 0, it returns 1
void custom_longjmp(jmp_buf_t buf_addr, int val)
{
  (void)buf_addr;
  (void)val;
  __asm__ volatile(
      "ldmia.w r0!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}\n" /* restore
                                                                   callee-saved
                                                                   + ip + lr */
      "mov    sp, ip\n" /* set SP = ip */
      "movs   r0, r1\n" /* r0 = val; set flags */
      "it     eq\n"     /* if (val == 0) */
      "moveq  r0, #1\n" /*   r0 = 1 */
      "bx     lr\n"     /* return */
  );
}

// custom impl of setjmp, that saves 10 registers in buf_addr
int custom_setjmp(jmp_buf_t buf_addr)
{
  (void)buf_addr;
  __asm__ volatile("mov	ip, sp\n"
                   "stmia.w	r0!, {r4, r5, r6, r7, r8, r9, sl, fp, ip, lr}\n"
                   "mov.w	r0, #0\n"
                   "bx	lr\n");
}

/**
 * Make sure to lock sensitive memory before throwing an error
 */
void os_longjmp(unsigned int exception)
{
  try_context_t *ctx = (try_context_t *)sys_try_context_get();

  custom_longjmp(ctx->jmp_buf, exception);
}