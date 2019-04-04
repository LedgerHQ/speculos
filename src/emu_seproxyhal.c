#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "emulate.h"

static ssize_t read_helper(int fd, void *buf, size_t count)
{
  ssize_t n;

  while (1) {
    /* actually issue a syscall which is forwarded to the host */
    n = read(fd, buf, count);
    if (n > 0) {
      break;
    } else if (n == 0) {
      warnx("read from seph fd failed: fd closed");
      return -1;
    } else {
      if (errno == EINTR)
        continue;
      warn("read from seph fd failed");
      return -1;
    }
  }

  return n;
}

static ssize_t writeall(int fd, const void *buf, size_t count)
{
  const char *p;
  ssize_t i;

  p = buf;
  do {
    i = write(fd, p, count);
    if (i == 0) {
      warnx("write to seph fd failed: fd closed");
      return -1;
    } else if (i == -1) {
      if (errno == EINTR)
        continue;
      warn("write to seph fd failed");
      return -1;
    }
    count -= i;
    p += i;
  } while (count > 0);

  return 0;
}

unsigned long sys_io_seproxyhal_spi_is_status_sent(void)
{
  return 0;
}

unsigned long sys_io_seph_is_status_sent(void) __attribute__ ((weak, alias ("sys_io_seproxyhal_spi_is_status_sent")));

unsigned long sys_io_seproxyhal_spi_send(const uint8_t *buffer, uint16_t length)
{
  return writeall(SEPH_FILENO, buffer, length);
}

unsigned long sys_io_seph_send(const uint8_t *buffer, uint16_t length) __attribute__ ((weak, alias ("sys_io_seproxyhal_spi_send")));

/* XXX: use flags */
unsigned long sys_io_seproxyhal_spi_recv(uint8_t *buffer, uint16_t maxlength, unsigned int UNUSED(flags))
{
  ssize_t size;

  //fprintf(stderr, "[*] sys_io_seproxyhal_spi_recv(%p, %d, %d);\n", buffer, maxlength, flags);

  size = read_helper(SEPH_FILENO, buffer, maxlength);

  if (size <= 0)
    _exit(1);

  return size;
}

unsigned long sys_io_seph_recv(uint8_t *buffer, uint16_t maxlength, unsigned int flags) __attribute__ ((weak, alias ("sys_io_seproxyhal_spi_recv")));
