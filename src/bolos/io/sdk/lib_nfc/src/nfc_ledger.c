/* @BANNER@ */

#ifdef HAVE_NFC
/* Includes ------------------------------------------------------------------*/
#include "nfc_ledger.h"
#include "ledger_protocol.h"
#include "os.h"
#include "os_io.h"
#include "os_io_seph_cmd.h"
#include "os_utils.h"
#include "seproxyhal_protocol.h"
#include <string.h>

/* Private enumerations ------------------------------------------------------*/
typedef enum {
  NFC_STATE_IDLE = 0xA0,
  NFC_STATE_RUNNING,
  NFC_STATE_STOPPED,
} nfc_state_e;

/* Private defines------------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
  nfc_state_e state;
  uint8_t mode; // nfc_ledger_mode_e
  ledger_protocol_t protocol_data;
} nfc_ledger_data_t;

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
#else // !HAVE_PRINTF
#define LOG_IO(...)
#endif // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static void nfc_ledger_send_rapdu(uint8_t *buffer, uint16_t length,
                                  uint32_t timeout_ms);

/* Exported variables --------------------------------------------------------*/
uint8_t NFC_LEDGER_io_buffer[OS_IO_BUFFER_SIZE + 1];

/* Private variables ---------------------------------------------------------*/
static nfc_ledger_data_t nfc_ledger_data;
static uint8_t
    ledger_protocol_chunk_buffer[156 + 2]; // TODO_IO : BLE buffer size, can be
                                           // changed though

/* Private functions ---------------------------------------------------------*/
static void nfc_ledger_send_rapdu(uint8_t *buffer, uint16_t length,
                                  uint32_t timeout_ms)
{
  UNUSED(timeout_ms);

  if (length) {
    unsigned char hdr[3];
    hdr[0] = SEPROXYHAL_TAG_NFC_RAPDU;
    hdr[1] = length >> 8;
    hdr[2] = length;
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, 3, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, length, NULL);
  }
}

/* Exported functions --------------------------------------------------------*/

void NFC_LEDGER_init(uint8_t force_restart)
{
  if (force_restart) {
    memset(&nfc_ledger_data, 0, sizeof(nfc_ledger_data));
    nfc_ledger_data.state = NFC_STATE_IDLE;
    LOG_IO("NFC_LEDGER_init deep\n");
  } else {
    LOG_IO("NFC_LEDGER_init\n");
  }
}

void NFC_LEDGER_start(uint8_t mode)
{
  LOG_IO("NFC_LEDGER_start %d\n", mode);

  if ((nfc_ledger_data.state == NFC_STATE_IDLE) ||
      (nfc_ledger_data.mode != mode)) {
    memset(&nfc_ledger_data.protocol_data, 0,
           sizeof(nfc_ledger_data.protocol_data));
    nfc_ledger_data.protocol_data.rx_apdu_buffer = NFC_LEDGER_io_buffer;
    nfc_ledger_data.protocol_data.rx_apdu_buffer_size =
        sizeof(NFC_LEDGER_io_buffer);
    nfc_ledger_data.protocol_data.tx_chunk_buffer =
        ledger_protocol_chunk_buffer;
    nfc_ledger_data.protocol_data.tx_chunk_buffer_size =
        sizeof(ledger_protocol_chunk_buffer);
    nfc_ledger_data.protocol_data.mtu = sizeof(ledger_protocol_chunk_buffer);
    nfc_ledger_data.state = NFC_STATE_STOPPED;
    nfc_ledger_data.mode = mode;
  }
  if ((nfc_ledger_data.state == NFC_STATE_STOPPED) &&
      (os_setting_get(OS_SETTING_FEATURES, NULL, 0) &
       OS_SETTING_FEATURES_NFC_ENABLED)) {
    if (nfc_ledger_data.mode == NFC_LEDGER_MODE_CARD_EMULATION) {
      LEDGER_PROTOCOL_init(&nfc_ledger_data.protocol_data,
                           OS_IO_PACKET_TYPE_NFC_APDU);
      nfc_ledger_data.state = NFC_STATE_RUNNING;
      os_io_nfc_cmd_power(SEPROXYHAL_TAG_NFC_POWER_ON_CE);
    } else if (nfc_ledger_data.mode == NFC_LEDGER_MODE_READER) {
      LEDGER_PROTOCOL_init(&nfc_ledger_data.protocol_data,
                           OS_IO_PACKET_TYPE_NFC_APDU_RSP);
      nfc_ledger_data.state = NFC_STATE_RUNNING;
      os_io_nfc_cmd_power(SEPROXYHAL_TAG_NFC_POWER_ON_READER);
    }
  }
}

void NFC_LEDGER_stop(void)
{
  os_io_nfc_cmd_power(SEPROXYHAL_TAG_NFC_POWER_OFF);
  nfc_ledger_data.state = NFC_STATE_STOPPED;
}

int NFC_LEDGER_rx_seph_apdu_evt(uint8_t *seph_buffer,
                                uint16_t seph_buffer_length,
                                uint8_t *apdu_buffer,
                                uint16_t apdu_buffer_max_length)
{
  int status = 0;

  if (seph_buffer_length < 2) {
    return -1;
  }

  uint16_t length = U2BE(seph_buffer, 2);
  LEDGER_PROTOCOL_rx(&nfc_ledger_data.protocol_data, &seph_buffer[4], length);

  if (nfc_ledger_data.protocol_data.rx_apdu_status == APDU_STATUS_COMPLETE) {
    if (apdu_buffer_max_length < nfc_ledger_data.protocol_data.rx_apdu_length) {
      status = -1;
    } else {
      memmove(apdu_buffer, nfc_ledger_data.protocol_data.rx_apdu_buffer,
              nfc_ledger_data.protocol_data.rx_apdu_length);
      status = nfc_ledger_data.protocol_data.rx_apdu_length;
    }
    nfc_ledger_data.protocol_data.rx_apdu_status = APDU_STATUS_WAITING;
  }

  return status;
}

uint32_t NFC_LEDGER_send(const uint8_t *packet, uint16_t packet_length,
                         uint32_t timeout_ms)
{
  uint32_t status = 0;

  LEDGER_PROTOCOL_tx(&nfc_ledger_data.protocol_data, packet, packet_length);
  if (nfc_ledger_data.protocol_data.tx_chunk_length >= 2) {
    nfc_ledger_send_rapdu(nfc_ledger_data.protocol_data.tx_chunk_buffer,
                          nfc_ledger_data.protocol_data.tx_chunk_length,
                          timeout_ms);
  }

  while (nfc_ledger_data.protocol_data.tx_apdu_buffer) {
    LEDGER_PROTOCOL_tx(&nfc_ledger_data.protocol_data, NULL, 0);
    if (nfc_ledger_data.protocol_data.tx_chunk_length >= 2) {
      nfc_ledger_send_rapdu(nfc_ledger_data.protocol_data.tx_chunk_buffer,
                            nfc_ledger_data.protocol_data.tx_chunk_length,
                            timeout_ms);
    }
  }

  return status;
}

#endif // HAVE_NFC
