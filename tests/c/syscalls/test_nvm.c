#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
// must come after setjmp.h
#include <cmocka.h>

#include "emulate.h"
#include "launcher.h"

#define PAGE_SIZE    4096
#define NVRAM_OFFSET PAGE_SIZE
#define NVRAM_SIZE   PAGE_SIZE
#define DATA_OFFSET  16
#define DATA_LEN     16

/* Emulated app code, whose second page is the app NVRAM */
static uint8_t *code;
static char nvram_file_name[] = "/tmp/speculos_test_nvram_XXXXXX";

void *get_memory_code_address(void)
{
  return code;
}

char *get_app_nvram_file_name(void)
{
  return nvram_file_name;
}

bool get_app_save_nvram(void)
{
  return true;
}

unsigned long get_app_nvram_address(void)
{
  return NVRAM_OFFSET;
}

unsigned long get_app_nvram_size(void)
{
  return NVRAM_SIZE;
}

unsigned long get_app_text_load_addr(void)
{
  return 0;
}

static void read_nvram_file(long offset, uint8_t *buffer, size_t len)
{
  FILE *fptr = fopen(nvram_file_name, "rb");

  assert_non_null(fptr);
  assert_int_equal(fseek(fptr, offset, SEEK_SET), 0);
  assert_int_equal(fread(buffer, 1, len, fptr), len);
  fclose(fptr);
}

static int setup(void **state __attribute__((unused)))
{
  int fd;

  code = mmap(NULL, NVRAM_OFFSET + NVRAM_SIZE, PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (code == MAP_FAILED) {
    return -1;
  }
  fd = mkstemp(nvram_file_name);
  if (fd == -1) {
    return -1;
  }
  close(fd);
  return 0;
}

static int teardown(void **state __attribute__((unused)))
{
  unlink(nvram_file_name);
  munmap(code, NVRAM_OFFSET + NVRAM_SIZE);
  return 0;
}

static void test_nvm_write_data(void **state __attribute__((unused)))
{
  uint8_t *dst = code + NVRAM_OFFSET + DATA_OFFSET;
  uint8_t data[DATA_LEN];
  uint8_t saved[DATA_LEN];

  for (size_t i = 0; i < sizeof(data); i++) {
    data[i] = (uint8_t)(i + 1);
  }
  sys_nvm_write(dst, data, sizeof(data));

  assert_memory_equal(dst, data, sizeof(data));
  read_nvram_file(DATA_OFFSET, saved, sizeof(saved));
  assert_memory_equal(saved, data, sizeof(data));
}

static void test_nvm_write_erase(void **state __attribute__((unused)))
{
  uint8_t *dst = code + NVRAM_OFFSET + DATA_OFFSET;
  uint8_t data[DATA_LEN];
  uint8_t zeros[DATA_LEN] = { 0 };
  uint8_t saved[DATA_LEN];

  memset(data, 0xAA, sizeof(data));
  sys_nvm_write(dst, data, sizeof(data));

  /* A NULL source erases the destination */
  sys_nvm_write(dst, NULL, sizeof(data));

  assert_memory_equal(dst, zeros, sizeof(zeros));
  read_nvram_file(DATA_OFFSET, saved, sizeof(saved));
  assert_memory_equal(saved, zeros, sizeof(zeros));
}

int main(void)
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_nvm_write_data),
    cmocka_unit_test(test_nvm_write_erase),
  };
  return cmocka_run_group_tests(tests, setup, teardown);
}
