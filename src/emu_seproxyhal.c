#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "emulate.h"
#include "exception.h"

#define SEPROXYHAL_TAG_STATUS_MASK  0x60

static bool status_sent = false;
static uint8_t last_tag;
static size_t next_length;

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

  if (count == 0)
    return 0;

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
  return (unsigned long)status_sent;
}

unsigned long sys_io_seph_is_status_sent(void) __attribute__ ((weak, alias ("sys_io_seproxyhal_spi_is_status_sent")));

unsigned long sys_io_seproxyhal_spi_send(const uint8_t *buffer, uint16_t length)
{
  unsigned long ret;

  if (length == 0) {
    return 0;
  }

  if (next_length == 0) {
    if (length < 3) {
      THROW(INVALID_PARAMETER);
    }
    last_tag = buffer[0];
    next_length = (buffer[1] << 8) | buffer[2];
    next_length += 3;

    if (next_length > 300) {
      THROW(INVALID_PARAMETER);
    }
  } else {
    if (length > next_length) {
      THROW(INVALID_PARAMETER);
    }
  }

  ret = writeall(SEPH_FILENO, buffer, length);

  next_length -= length;
  if (next_length == 0 && (last_tag & SEPROXYHAL_TAG_STATUS_MASK) == SEPROXYHAL_TAG_STATUS_MASK) {
    status_sent = true;
  }

  return ret;
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

  status_sent = false;

  return size;
}

unsigned long sys_io_seph_recv(uint8_t *buffer, uint16_t maxlength, unsigned int flags) __attribute__ ((weak, alias ("sys_io_seproxyhal_spi_recv")));
