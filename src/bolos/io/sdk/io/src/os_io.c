
/* Includes ------------------------------------------------------------------*/
#include "os_io.h"
#include "checks.h"
#include "errors.h"
#include "os_io_seph_cmd.h"
#include "os_io_seph_ux.h"
#include "os_pin.h"
#include "seproxyhal_protocol.h"
#include <string.h>

#ifdef HAVE_IO_USB
#include "usbd_ledger.h"
#include "usbd_ledger_hid_u2f.h"
#endif // HAVE_IO_USB

#ifdef HAVE_BLE
#include "ble_ledger.h"
#endif // HAVE_BLE

#ifdef HAVE_NFC
#include "nfc_ledger.h"
#endif // HAVE_NFC

#if defined(HAVE_NFC_READER) && !defined(HAVE_BOLOS)
#include "os_io_legacy.h"
#endif // HAVE_NFC_READER && !HAVE_BOLOS

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
// #define LOG_IO(...)
#else // !HAVE_PRINTF
#define LOG_IO(...)
#endif // !HAVE_PRINTF

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
#ifndef USE_OS_IO_STACK
static int process_itc_event(uint8_t *buffer_in, size_t buffer_in_length);
#endif // !USE_OS_IO_STACK

/* Exported variables --------------------------------------------------------*/
#ifndef HAVE_LOCAL_APDU_BUFFER
// apdu buffer must hold a complete apdu to avoid troubles
unsigned char G_io_rx_buffer[OS_IO_BUFFER_SIZE + 1];
unsigned char G_io_tx_buffer[OS_IO_BUFFER_SIZE + 1];
#endif

#ifndef USE_OS_IO_STACK
unsigned char G_io_seph_buffer[OS_IO_SEPH_BUFFER_SIZE + 1];
size_t G_io_seph_buffer_size;
#endif // !USE_OS_IO_STACK

#ifdef HAVE_BOLOS
uint8_t G_io_state;
uint8_t G_io_init_syscall;
#endif // HAVE_BOLOS

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
#ifndef USE_OS_IO_STACK
static int process_itc_event(uint8_t *buffer_in, size_t buffer_in_length)
{
  int status = buffer_in_length;

  switch (buffer_in[3]) {
#ifdef HAVE_BLE
  case ITC_IO_BLE_STOP:
    BLE_LEDGER_stop();
    status = 0;
    break;

  case ITC_IO_BLE_START:
    BLE_LEDGER_start();
    status = 0;
    break;

  case ITC_IO_BLE_RESET_PAIRINGS:
    BLE_LEDGER_reset_pairings();
    status = 0;
    break;

  case ITC_IO_BLE_BLE_NAME_CHANGED:
    // Restart advertising
    BLE_LEDGER_name_changed();
    status = 0;
    break;

  case ITC_UX_ACCEPT_BLE_PAIRING:
    BLE_LEDGER_accept_pairing(buffer_in[4]);
    status = 0;
    break;
#endif // HAVE_BLE

#ifdef HAVE_NFC
  case ITC_IO_NFC_STOP:
    NFC_LEDGER_stop();
    break;

  case ITC_IO_NFC_START_CE:
    NFC_LEDGER_start(NFC_LEDGER_MODE_CARD_EMULATION);
    break;

  case ITC_IO_NFC_START_READER:
    NFC_LEDGER_start(NFC_LEDGER_MODE_READER);
    break;
#endif // HAVE_NFC

#ifdef HAVE_SE_TOUCH
#endif // HAVE_SE_TOUCH

  default:
    break;
  }

  if ((status)
#ifdef HAVE_BOLOS
      && (G_io_state == OS_IO_STATE_DASHBOARD)
#endif // HAVE_BOLOS
  ) {
    status = io_process_itc_ux_event(buffer_in, buffer_in_length);
  }

  return status;
}
#endif // !USE_OS_IO_STACK

/* Exported functions --------------------------------------------------------*/

#ifndef USE_OS_IO_STACK
int os_io_init(os_io_init_t *init)
{
  if (!init) {
    return -1;
  }

  uint8_t force_restart = 0;

#ifdef HAVE_BOLOS
  if (G_io_state == OS_IO_STATE_IDLE) {
    // POR
    G_io_state = OS_IO_STATE_DASHBOARD;
    force_restart = 1;
  } else if (G_io_init_syscall != 0x42) {
    // Dashboard init
    if (G_io_state == OS_IO_STATE_APP_LOW_LEVEL_API) {
      force_restart = 1;
    }
    G_io_state = OS_IO_STATE_DASHBOARD;
  } else {
    // App init
    if (G_io_state == OS_IO_STATE_APP_LOW_LEVEL_API) {
      force_restart = 1;
    }
    G_io_state = OS_IO_STATE_APP_HIGH_LEVEL_API;
  }
  G_io_init_syscall = 0;
#else  // !HAVE_BOLOS
  force_restart = 1;
#endif // !HAVE_BOLOS

#ifdef HAVE_BAGL
  io_seph_ux_init_button();
#endif

#if (!defined(HAVE_BOLOS) && defined(HAVE_MCU_PROTECT))
  os_io_seph_cmd_mcu_protect();
#endif // (!HAVE_BOLOS && HAVE_MCU_PROTECT)

#if !defined(HAVE_BOLOS) && defined(HAVE_PENDING_REVIEW_SCREEN)
  check_audited_app();
#endif // !HAVE_BOLOS && HAVE_PENDING_REVIEW_SCREEN

#ifdef HAVE_IO_USB
  USBD_LEDGER_init(&init->usb, force_restart);
#endif // HAVE_IO_USB

#ifdef HAVE_BLE
  BLE_LEDGER_init(&init->ble, force_restart);
#endif // HAVE_BLE

#ifdef HAVE_NFC
  NFC_LEDGER_init(force_restart);
#endif // HAVE_NFC

#ifndef USE_OS_IO_STACK
  G_io_seph_buffer_size = 0;
#endif // USE_OS_IO_STACK

  return 0;
}

int os_io_start(void)
{
#ifdef HAVE_IO_USB
  USBD_LEDGER_start();
#endif // HAVE_IO_USB

#ifdef HAVE_BLE
  BLE_LEDGER_start();
#endif // HAVE_BLE

#ifdef HAVE_NFC
  NFC_LEDGER_start(NFC_LEDGER_MODE_CARD_EMULATION);
#endif // HAVE_NFC

  return 0;
}

int os_io_stop(void)
{
#ifdef HAVE_IO_USB
  USB_LEDGER_stop();
#endif // HAVE_IO_USB

#ifdef HAVE_BLE
  BLE_LEDGER_stop();
#endif // HAVE_BLE

#ifdef HAVE_NFC
  NFC_LEDGER_stop();
#endif // HAVE_NFC

  return 0;
}

int os_io_rx_evt(unsigned char *buffer, unsigned short buffer_max_length,
                 unsigned int *timeout_ms, bool check_se_event)
{
  int status = 0;
  uint16_t length = 0;

  if (!G_io_seph_buffer_size) {
    status =
        os_io_seph_se_rx_event(G_io_seph_buffer, sizeof(G_io_seph_buffer),
                               (unsigned int *)timeout_ms, check_se_event, 0);
  } else {
    // Cached rx event
    status = G_io_seph_buffer_size;
    G_io_seph_buffer_size = 0;
  }
  if (status == -1) {
    // Wrong state, send cmd to MCU
    status = os_io_seph_cmd_general_status();
    if (status < 0) {
      return status;
    }
    status =
        os_io_seph_se_rx_event(G_io_seph_buffer, sizeof(G_io_seph_buffer),
                               (unsigned int *)timeout_ms, check_se_event, 0);
  }
  if (status < 0) {
    return status;
  }
  if (status > 0) {
    length = (uint16_t)status;
  }

  switch (G_io_seph_buffer[1]) {
#ifdef HAVE_IO_USB
  case SEPROXYHAL_TAG_USB_EVENT:
  case SEPROXYHAL_TAG_USB_EP_XFER_EVENT:
    status = USBD_LEDGER_rx_seph_evt(G_io_seph_buffer, length, buffer,
                                     buffer_max_length);
    break;
#endif // HAVE_IO_USB

#ifdef HAVE_BLE
  case SEPROXYHAL_TAG_BLE_RECV_EVENT:
    status = BLE_LEDGER_rx_seph_evt(G_io_seph_buffer, length, buffer,
                                    buffer_max_length);
    break;
#endif // HAVE_BLE

#ifdef HAVE_NFC
  case SEPROXYHAL_TAG_NFC_APDU_EVENT:
    status = NFC_LEDGER_rx_seph_apdu_evt(G_io_seph_buffer, length, buffer,
                                         buffer_max_length);
#if defined(HAVE_NFC_READER) && !defined(HAVE_BOLOS)
    io_nfc_process_events();
#endif // HAVE_NFC_READER && !HAVE_BOLOS
    break;

#if defined(HAVE_NFC_READER) && !defined(HAVE_BOLOS)
  case SEPROXYHAL_TAG_NFC_EVENT:
    io_nfc_event();
    io_nfc_process_events();
    break;

  case SEPROXYHAL_TAG_TICKER_EVENT:
    io_nfc_ticker();
    io_nfc_process_events();
    break;
#endif // HAVE_NFC_READER && !HAVE_BOLOS
#endif // HAVE_NFC

  case SEPROXYHAL_TAG_CAPDU_EVENT:
    if (length >= buffer_max_length - 1) {
      length = buffer_max_length - 1;
    }
    buffer[0] = OS_IO_PACKET_TYPE_RAW_APDU;
    memmove(&buffer[1], &G_io_seph_buffer[4], length);
    status = length - 3;
    break;

  case SEPROXYHAL_TAG_ITC_EVENT:
    if (length >= buffer_max_length - 1) {
      length = buffer_max_length - 1;
    }
    memmove(buffer, G_io_seph_buffer, length);
    status = process_itc_event(&G_io_seph_buffer[1], status - 1);
    if (status > 0) {
      status = length;
    }
    break;

  default:
    if (length >= buffer_max_length - 1) {
      length = buffer_max_length - 1;
    }
    memmove(buffer, G_io_seph_buffer, length);
    break;
  }

  return status;
}

int os_io_tx_cmd(uint8_t type, const unsigned char *buffer PLENGTH(length),
                 unsigned short length, unsigned int *timeout_ms)
{
  int status = 0;
  switch (type) {
#ifdef HAVE_IO_USB
  case OS_IO_PACKET_TYPE_USB_HID_APDU:
    // TODO_IO test error code
    USBD_LEDGER_send(USBD_LEDGER_CLASS_HID, type, buffer, length, 0);
    break;
#ifdef HAVE_WEBUSB
  case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
    USBD_LEDGER_send(USBD_LEDGER_CLASS_WEBUSB, type, buffer, length, 0);
    break;
#endif // HAVE_WEBUSB
#ifdef HAVE_IO_U2F
  case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
  case OS_IO_PACKET_TYPE_USB_U2F_HID_CBOR:
    USBD_LEDGER_send(USBD_LEDGER_CLASS_HID_U2F, type, buffer, length, 0);
    break;
#endif // HAVE_IO_U2F
#ifdef HAVE_CCID_USB
  case OS_IO_PACKET_TYPE_USB_CCID_APDU:
    USBD_LEDGER_send(USBD_LEDGER_CLASS_CCID_BULK, type, buffer, length, 0);
    break;
#endif // HAVE_CCID_USB
#endif // HAVE_IO_USB

#ifdef HAVE_BLE
  case OS_IO_PACKET_TYPE_BLE_APDU:
    BLE_LEDGER_send(BLE_LEDGER_PROFILE_APDU, buffer, length, 0);
    break;
#endif // HAVE_BLE

#ifdef HAVE_NFC
  case OS_IO_PACKET_TYPE_NFC_APDU:
    NFC_LEDGER_send(buffer, length, 0);
    break;
#endif // HAVE_NFC

  case OS_IO_PACKET_TYPE_RAW_APDU:
    os_io_seph_cmd_raw_apdu((const uint8_t *)buffer, length);
    break;

  case OS_IO_PACKET_TYPE_SEPH:
    status = os_io_seph_tx(buffer, length, (unsigned int *)timeout_ms);
    if (status == -1) {
      // Wrong state, wait for an event from the MCU
      status = os_io_seph_se_rx_event(
          G_io_seph_buffer, sizeof(G_io_seph_buffer),
          (unsigned int *)timeout_ms, false, OS_IO_FLAG_NO_ITC);
      if (status >= 0) {
        G_io_seph_buffer_size = status;
        status = os_io_seph_tx(buffer, length, NULL);
      }
    }
    break;

  default:
    break;
  }

#ifdef HAVE_IO_USB
  if (type & OS_IO_PACKET_TYPE_USB_MASK) {
    uint8_t count = 0;

    while ((count < 5) && USBD_LEDGER_is_busy()) {
      os_io_rx_evt(G_io_seph_buffer, sizeof(G_io_seph_buffer), NULL, false);
      count++;
    }
    if (count < 5) {
      status = length;
    }
  }
#endif // HAVE_IO_USB
#ifdef HAVE_BLE
  if (type & OS_IO_PACKET_TYPE_BLE_MASK) {
    uint8_t count = 0;

    while ((count < 5) && BLE_LEDGER_is_busy()) {
      os_io_rx_evt(G_io_seph_buffer, sizeof(G_io_seph_buffer), NULL, false);
      count++;
    }
    if (count < 5) {
      status = length;
    }
  }
#endif // HAVE_BLE

  return status;
}
#endif // !USE_OS_IO_STACK

unsigned int os_io_handle_ux_event_reject_apdu(void)
{
  uint16_t err = 0x6601;
  unsigned char err_buffer[2];
  int status = os_io_rx_evt(G_io_tx_buffer, sizeof(G_io_tx_buffer), NULL, true);

  if (os_perso_is_pin_set() == BOLOS_TRUE &&
      os_global_pin_is_validated() != BOLOS_TRUE) {
    err = SWO_SEC_PIN_15;
  }

  err_buffer[0] = err >> 8;
  err_buffer[1] = err;

  if (status > 0) {
    switch (G_io_tx_buffer[0]) {
    case OS_IO_PACKET_TYPE_SE_EVT:
    case OS_IO_PACKET_TYPE_SEPH:
      io_process_ux_event(&G_io_tx_buffer[1], status - 1);
      break;

    case OS_IO_PACKET_TYPE_RAW_APDU:
    case OS_IO_PACKET_TYPE_USB_HID_APDU:
    case OS_IO_PACKET_TYPE_USB_WEBUSB_APDU:
    case OS_IO_PACKET_TYPE_USB_U2F_HID_APDU:
    case OS_IO_PACKET_TYPE_USB_CCID_APDU:
    case OS_IO_PACKET_TYPE_BLE_APDU:
    case OS_IO_PACKET_TYPE_NFC_APDU:
      os_io_tx_cmd(G_io_tx_buffer[0], err_buffer, sizeof(err_buffer), 0);
      break;

    default:
      break;
    }
  }

  return status;
}
