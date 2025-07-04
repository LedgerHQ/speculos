#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "bolos/io/io.h"
#include "bolos/touch.h"
#include "emulate.h"
#include "os_utils.h"
#include "seproxyhal_protocol.h"

enum seph_state_t {
  SEPH_STATE_IDLE = 0,
  SEPH_STATE_WAIT_EVENT,
  SEPH_STATE_SEND_CMD,
};

typedef struct {
  uint16_t tx_packet_length;
  uint16_t tx_packet_offset;
  uint16_t tx_packet_max_length;
  uint8_t tx_packet_tag;

  uint8_t state; // seph_state_t

  uint16_t rx_packet_length;
  uint16_t rx_packet_max_length;
  uint8_t rx_packet[OS_IO_SEPH_BUFFER_SIZE];
} seph_info_t;

#define OS_IO_FLAG_CACHE 1

static seph_info_t G_seph_info = {
  .state = SEPH_STATE_SEND_CMD,
  .tx_packet_tag = 0,
  .tx_packet_length = 0,
  .tx_packet_offset = 0,
  .tx_packet_max_length = OS_IO_SEPH_BUFFER_SIZE,
  .rx_packet_length = 0,
  .rx_packet_max_length = OS_IO_SEPH_BUFFER_SIZE,
};

static ssize_t readall(int fd, void *buf, size_t count)
{
  ssize_t n;

  while (count > 0) {
    n = read(fd, buf, count);
    if (n == 0) {
      warnx("read from seph fd failed: fd closed");
      return -1;
    } else if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      warn("read from seph fd failed");
      return -1;
    }
    count -= n;
  }

  return 0;
}

static ssize_t writeall(int fd, const void *buf, size_t count)
{
  const char *p;
  ssize_t i;

  if (count == 0) {
    return 0;
  }

  p = buf;
  do {
    i = write(fd, p, count);
    if (i == 0) {
      warnx("write to seph fd failed: fd closed");
      return -1;
    } else if (i == -1) {
      if (errno == EINTR) {
        continue;
      }
      warn("write to seph fd failed");
      return -1;
    }
    count -= i;
    p += i;
  } while (count > 0);

  return 0;
}

int sys_os_io_init(os_io_init_t *init)
{
  return os_io_init(init);
}

int sys_os_io_start(void)
{
  return os_io_start();
}

int sys_os_io_stop(void)
{
  return os_io_stop();
}

int sys_os_io_rx_evt(unsigned char *buffer, unsigned short buffer_max_length,
                     unsigned int *timeout_ms, bool check_se_event)
{
  return os_io_rx_evt(buffer, buffer_max_length, timeout_ms, check_se_event);
}

int sys_os_io_tx_cmd(unsigned char type, const unsigned char *buffer,
                     unsigned short length, unsigned int *timeout_ms)
{
  return os_io_tx_cmd(type, buffer, length, timeout_ms);
}

int sys_os_io_seph_tx(const unsigned char *buffer, unsigned short length,
                      unsigned int *timeout_ms)
{
  (void)timeout_ms;

  int tx_length = 0;

  if (!buffer || !length) {
    return -22; // EINVAL
  }

  if (G_seph_info.state == SEPH_STATE_WAIT_EVENT) {
    // Status already sent before
    if ((G_seph_info.tx_packet_length == 0) &&
        ((buffer[0] & SEPROXYHAL_TAG_STATUS_MASK) ==
         SEPROXYHAL_TAG_GENERAL_STATUS)) {
      // Trying to send a new status in a row, just ignore it
      tx_length = length;
      goto end;
    } else {
      // Wrong state
      tx_length = -1; // EPERM
      goto end;
    }
  }

  // Check packet length
  if (G_seph_info.tx_packet_length == 0) {
    // Start of the packet
    if (length < 3) {
      // We need at least the opcode and the size in packet
      tx_length = -23; // EINVAL
      goto end;
    }

    G_seph_info.tx_packet_length = U2BE(buffer, 1) + 3;
    if (G_seph_info.tx_packet_length > G_seph_info.tx_packet_max_length) {
      G_seph_info.tx_packet_length = 0;
      tx_length = -25; // EINVAL
      goto end;
    }
    G_seph_info.tx_packet_tag = buffer[0];
    G_seph_info.tx_packet_offset = 0;
  }

  if ((G_seph_info.tx_packet_offset + length) > G_seph_info.tx_packet_length) {
    tx_length = -24; // EINVAL
    goto end;
  }

  writeall(SEPH_FILENO, buffer, length);
  tx_length = length;

  uint8_t is_status_packet =
      ((G_seph_info.tx_packet_tag & SEPROXYHAL_TAG_STATUS_MASK) ==
       SEPROXYHAL_TAG_GENERAL_STATUS);

  G_seph_info.tx_packet_offset += length;

  if (G_seph_info.tx_packet_offset == G_seph_info.tx_packet_length) {
    // Whole packet sent
    G_seph_info.tx_packet_length = 0;
    G_seph_info.tx_packet_tag = 0;
    if (is_status_packet) {
      G_seph_info.state = SEPH_STATE_WAIT_EVENT;
    }
  }

end:
  return tx_length;
}

int sys_os_io_seph_se_rx_event(unsigned char *buffer, unsigned short max_length,
                               unsigned int *timeout_ms, bool check_se_event,
                               unsigned int flags)
{
  (void)timeout_ms;
  (void)check_se_event;

  int rx_length = 0;

  if (!buffer || (max_length < G_seph_info.rx_packet_max_length)) {
    return -26; // EINVAL
  }

  if (flags & OS_IO_FLAG_CACHE) {
    memcpy(buffer, G_seph_info.rx_packet, G_seph_info.rx_packet_length);
    rx_length = G_seph_info.rx_packet_length;
    goto end;
  }

  if (G_seph_info.state != SEPH_STATE_WAIT_EVENT) {
    // Wrong state
    rx_length = -1; // EPERM
    goto end;
  }

  ssize_t res = readall(SEPH_FILENO, G_seph_info.rx_packet, 3);
  if (res < 0) {
    printf("Readall error\n");
    _exit(1);
  }

  // Header of the seph packet has been received, wait for the packet's body
  G_seph_info.rx_packet_length =
      (G_seph_info.rx_packet[1] << 8) | G_seph_info.rx_packet[2];
  if (G_seph_info.rx_packet_length > (G_seph_info.rx_packet_max_length - 3)) {
    rx_length = -12; // ENOMEM
    goto end;
  }

  G_seph_info.rx_packet_length += 3;

  res = readall(SEPH_FILENO, &G_seph_info.rx_packet[3],
                G_seph_info.rx_packet_length - 3);
  if (res < 0) {
    printf("Readall error\n");
    _exit(1);
  }

  buffer[0] = OS_IO_PACKET_TYPE_SEPH;
  memcpy(&buffer[1], G_seph_info.rx_packet, G_seph_info.rx_packet_length);
  rx_length = G_seph_info.rx_packet_length + 1;

  G_seph_info.state = SEPH_STATE_SEND_CMD;
  G_seph_info.tx_packet_length = 0;

end:
  if (rx_length >= 0) {
    catch_touch_info_from_seph(&buffer[1], rx_length - 1);
  }

  return rx_length;
}
