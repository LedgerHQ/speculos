#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/mman.h>

#include "emulate.h"
#include "launcher.h"

#define PAGE_SIZE 4096

unsigned long sys_os_global_pin_invalidate(void)
{
  /* return void actually */
  return 0;
}

/*
 * TODO: ensure that source address is valid.
 */
int sys_nvm_write(void *dst_addr, void *src_addr, size_t src_len)
{
  ptrdiff_t diff;
  size_t size;
  void *p;

  /* Checking the destination boundaries */
  unsigned long app_nvram_offset =
      get_app_nvram_address() - get_app_text_load_addr();
  if ((dst_addr < get_memory_code_address() + app_nvram_offset) ||
      (dst_addr + src_len >=
       get_memory_code_address() + app_nvram_offset + get_app_nvram_size())) {
    errx(1, "App NVRAM write attempt out of boundaries\n");
  }
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

  if (get_app_save_nvram()) {
    /* Writing data to host file */
    FILE *fptr = fopen(get_app_nvram_file_name(), "r+");
    if (fptr == NULL) {
      fptr = fopen(get_app_nvram_file_name(), "w");
      if (fptr == NULL) {
        err(1, "Failed to open the app NVRAM file %s\n",
            get_app_nvram_file_name());
      }
    }
    unsigned long data_offset =
        dst_addr - (get_memory_code_address() + app_nvram_offset);
    fseek(fptr, data_offset, SEEK_SET);
    if (fwrite(src_addr, 1, src_len, fptr) != src_len) {
      errx(1, "App NVRAM write attempt failed\n");
    }
    fclose(fptr);
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
