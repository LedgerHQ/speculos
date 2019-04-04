#include <err.h>
#include <stddef.h>
#include <sys/mman.h>

#include "emulate.h"

#define PAGE_SIZE	4096

#define BOLOS_UX_OK     0xB0105011
#define BOLOS_UX_CANCEL 0xB0105022
#define BOLOS_UX_ERROR  0xB0105033
#define BOLOS_UX_IGNORE 0xB0105044
#define BOLOS_UX_REDRAW	0xB0105055
#define BOLOS_UX_CONTINUE 0

/* TODO */
unsigned long sys_os_ux(bolos_ux_params_t *UNUSED(params))
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_global_pin_is_validated(void)
{
  return BOLOS_UX_OK;
}

unsigned long sys_os_global_pin_invalidate(void)
{
  /* return void actually */
  return 0;
}

unsigned long sys_os_perso_isonboarded(void)
{
  return BOLOS_UX_OK;
}

/*
 * TODO: ensure that source and destination address are valid.
 * TODO: map these data to the host filesystem.
 */
int sys_nvm_write(void *dst_addr, void* src_addr, size_t src_len)
{
  ptrdiff_t diff;
  size_t size;
  void *p;

  p = (void *)((unsigned long)dst_addr & (~(PAGE_SIZE-1)));
  diff = (ptrdiff_t)dst_addr - (ptrdiff_t)p;
  size = src_len + diff;

  if (mprotect(p, size, PROT_WRITE) != 0) {
    err(1, "nvm_write: mprotect(PROT_WRITE)");
  }

  memcpy(dst_addr, src_addr, src_len);

  if (mprotect(p, size, PROT_READ | PROT_EXEC) != 0) {
    err(1, "nvm_write: mprotect(PROT_READ | PROT_EXEC)");
  }

  /* XXX: this function return void */
  return 0xdeadbeef;
}
