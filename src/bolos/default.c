#include <err.h>
#include <stddef.h>
#include <sys/mman.h>

#include "emulate.h"

#define PAGE_SIZE 4096

unsigned long sys_os_global_pin_invalidate(void)
{
  /* return void actually */
  return 0;
}

/*
 * TODO: ensure that source and destination address are valid.
 * TODO: map these data to the host filesystem.
 */
int sys_nvm_write(void *dst_addr, void *src_addr, size_t src_len)
{
  ptrdiff_t diff;
  size_t size;
  void *p;

  p = (void *)((unsigned long)dst_addr & (~(PAGE_SIZE - 1)));
  diff = (ptrdiff_t)dst_addr - (ptrdiff_t)p;
  size = src_len + diff;

  if (mprotect(p, size, PROT_WRITE) != 0) {
    err(1, "nvm_write: mprotect(PROT_WRITE)");
  }

  if (src_addr != NULL) {
    memcpy(dst_addr, src_addr, src_len);
  } else {
    memset(dst_addr, 0, src_len);
  }

  if (mprotect(p, size, PROT_READ | PROT_EXEC) != 0) {
    err(1, "nvm_write: mprotect(PROT_READ | PROT_EXEC)");
  }

  /* XXX: this function return void */
  return 0xdeadbeef;
}

int sys_nvm_erase(void *dst_addr, size_t src_len)
{
  return sys_nvm_write(dst_addr, NULL, src_len);
}

int sys_nvm_erase_page(unsigned int page_adr __attribute__((unused)))
{
  // TODO !
  return 0;
}
