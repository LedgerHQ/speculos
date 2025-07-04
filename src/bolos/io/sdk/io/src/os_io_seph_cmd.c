/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "os_io_seph_cmd.h"
#include "os.h"
#include "os_io.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/
#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
// #define LOG_IO(...)
#else // !HAVE_PRINTF
#define LOG_IO(...)
#endif // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static const unsigned char seph_io_general_status[] = {
  SEPROXYHAL_TAG_GENERAL_STATUS,
  0,
  2,
  SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND >> 8,
  SEPROXYHAL_TAG_GENERAL_STATUS_LAST_COMMAND,
};

static const unsigned char seph_io_request_status[] = {
  SEPROXYHAL_TAG_REQUEST_STATUS,
  0,
  0,
};

static const unsigned char seph_io_se_power_off[] = {
  SEPROXYHAL_TAG_SE_POWER_OFF,
  0,
  0,
};

static const unsigned char seph_io_usb_disconnect[] = {
  SEPROXYHAL_TAG_USB_CONFIG,
  0,
  1,
  SEPROXYHAL_TAG_USB_CONFIG_DISCONNECT,
};

static const unsigned char seph_io_cmd_mcu_bootloader[] = {
  SEPROXYHAL_TAG_MCU,
  0,
  1,
  SEPROXYHAL_TAG_MCU_TYPE_BOOTLOADER,
};

static const unsigned char seph_io_cmd_mcu_lock[] = {
  SEPROXYHAL_TAG_MCU,
  0,
  1,
  SEPROXYHAL_TAG_MCU_TYPE_LOCK,
};

static const unsigned char seph_io_mcu_protect[] = {
  SEPROXYHAL_TAG_MCU,
  0,
  1,
  SEPROXYHAL_TAG_MCU_TYPE_PROTECT,
};

#ifdef HAVE_SHIP_MODE
static const unsigned char seph_io_cmd_set_ship_mode[] = {
  SEPROXYHAL_TAG_SET_SHIP_MODE,
  0,
  0,
};
#endif // HAVE_SHIP_MODE

static const unsigned char seph_io_cmd_more_time[] = {
  SEPROXYHAL_TAG_MORE_TIME,
  0,
  0,
};

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
int os_io_seph_cmd_general_status(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_general_status,
                      sizeof(seph_io_general_status), NULL);
}

int os_io_seph_cmd_more_time(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_cmd_more_time,
                      sizeof(seph_io_cmd_more_time), NULL);
}

int os_io_seph_cmd_setup_ticker(unsigned int interval_ms)
{
  uint8_t buffer[5];
  buffer[0] = SEPROXYHAL_TAG_SET_TICKER_INTERVAL;
  buffer[1] = 0;
  buffer[2] = 2;
  buffer[3] = (interval_ms >> 8) & 0xff;
  buffer[4] = (interval_ms)&0xff;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 5, NULL);
}

int os_io_seph_cmd_device_shutdown(uint8_t critical_battery)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_DEVICE_OFF;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = critical_battery;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

int os_io_seph_cmd_se_reset(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_se_power_off,
                      sizeof(seph_io_se_power_off), NULL);
}

int os_io_seph_cmd_usb_disconnect(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_usb_disconnect,
                      sizeof(seph_io_usb_disconnect), NULL);
}

int os_io_seph_cmd_mcu_status(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_request_status,
                      sizeof(seph_io_request_status), NULL);
}

int os_io_seph_cmd_mcu_go_to_bootloader(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_cmd_mcu_bootloader,
                      sizeof(seph_io_cmd_mcu_bootloader), NULL);
}

int os_io_seph_cmd_mcu_lock(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_cmd_mcu_lock,
                      sizeof(seph_io_cmd_mcu_lock), NULL);
}

int os_io_seph_cmd_mcu_protect(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_mcu_protect,
                      sizeof(seph_io_mcu_protect), NULL);
}

void os_io_seph_cmd_raw_apdu(const uint8_t *buffer, uint16_t length)
{
  if (length) {
    unsigned char hdr[3];
    hdr[0] = SEPROXYHAL_TAG_RAPDU;
    hdr[1] = length >> 8;
    hdr[2] = length;
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, 3, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, length, NULL);
  }
}

#ifdef HAVE_SHIP_MODE
int os_io_seph_cmd_set_ship_mode(void)
{
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_io_cmd_set_ship_mode,
                      sizeof(seph_io_cmd_set_ship_mode), NULL);
}
#endif // HAVE_SHIP_MODE

#ifdef HAVE_PRINTF
void os_io_seph_cmd_printf(const char *str, uint16_t charcount)
{
  if (charcount) {
    unsigned char hdr[3];
    hdr[0] = SEPROXYHAL_TAG_PRINTF;
    hdr[1] = charcount >> 8;
    hdr[2] = charcount;
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, 3, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, (const uint8_t *)str, charcount, NULL);
  }
}
#endif // HAVE_PRINTF

#ifdef HAVE_SE_TOUCH
int os_io_seph_cmd_set_touch_state(uint8_t enable)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_SET_TOUCH_STATE;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = (enable ? 0x01 : 0x00);

  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}
#endif // HAVE_SE_TOUCH

#ifdef HAVE_PIEZO_SOUND
int os_io_seph_cmd_piezo_play_tune(tune_index_e tune_index)
{
  int status = 0;

  uint8_t buffer[4];
  if (tune_index >= NB_TUNES) {
    status = -22; // EINVAL
    goto end;
  }

  uint32_t sound_setting = os_setting_get(OS_SETTING_PIEZO_SOUND, NULL, 0);

  if ((!IS_NOTIF_ENABLED(sound_setting)) && (tune_index < TUNE_TAP_CASUAL)) {
    goto end;
  }
  if ((!IS_TAP_ENABLED(sound_setting)) && (tune_index >= TUNE_TAP_CASUAL)) {
    goto end;
  }

  buffer[0] = SEPROXYHAL_TAG_PLAY_TUNE;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = (uint8_t)tune_index;
  status = os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);

end:
  return status;
}

void io_seproxyhal_play_tune(tune_index_e tune_index)
{
  (void)os_io_seph_cmd_piezo_play_tune(tune_index);
}
#endif // HAVE_PIEZO_SOUND

#ifdef HAVE_SERIALIZED_NBGL
void os_io_seph_cmd_serialized_nbgl(const uint8_t *buffer, uint16_t length)
{
  if (length) {
    unsigned char hdr[3];
    hdr[0] = SEPROXYHAL_TAG_NBGL_SERIALIZED;
    hdr[1] = length >> 8;
    hdr[2] = length;
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, 3, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, length, NULL);
  }
}
#endif // HAVE_SERIALIZED_NBGL

#ifdef HAVE_NOR_FLASH
void os_io_seph_cmd_spi_cs(uint8_t select)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_SPI_CS;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = select;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}
#endif // HAVE_NOR_FLASH

#ifdef HAVE_BLE
int os_io_seph_cmd_ble_start_factory_test(void)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_BLE_RADIO_POWER;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = SEPROXYHAL_TAG_BLE_RADIO_POWER_FACTORY_TEST;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}
int os_io_ble_cmd_enable(uint8_t enable)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = (enable ? ITC_IO_BLE_START : ITC_IO_BLE_STOP);
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

int os_io_ble_cmd_clear_bond_db(void)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = ITC_IO_BLE_RESET_PAIRINGS;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

int os_io_ble_cmd_name_changed(void)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = ITC_IO_BLE_BLE_NAME_CHANGED;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

int os_io_ux_cmd_ble_accept_pairing(unsigned char status)
{
  uint8_t buffer[5];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 2;
  buffer[3] = ITC_UX_ACCEPT_BLE_PAIRING;
  buffer[4] = status;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 5, NULL);
}

int os_io_ux_cmd_redisplay(void)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = ITC_UX_REDISPLAY;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

void os_io_ux_cmd_ble_pairing_request(uint8_t type, char *pairing_info,
                                      uint8_t pairing_info_len)
{
  uint8_t hdr[5];
  hdr[0] = SEPROXYHAL_TAG_ITC_CMD;
  hdr[1] = 0;
  hdr[2] = pairing_info_len + 2;
  hdr[3] = ITC_UX_ASK_BLE_PAIRING;
  hdr[4] = type;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, hdr, 5, NULL);
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, (uint8_t *)pairing_info,
               pairing_info_len, NULL);
}

void os_io_ux_cmd_ble_pairing_status(uint8_t pairing_status)
{
  uint8_t buffer[5];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 2;
  buffer[3] = ITC_UX_BLE_PAIRING_STATUS;
  buffer[4] = pairing_status;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 5, NULL);
}
#endif // HAVE_BLE

#ifdef HAVE_NFC
int os_io_nfc_cmd_power(uint8_t power_type)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_NFC_POWER;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = power_type;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

int os_io_nfc_cmd_stop(void)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = ITC_IO_NFC_STOP;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

int os_io_nfc_cmd_start_ce(void)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = ITC_IO_NFC_START_CE;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}

int os_io_nfc_cmd_start_reader(void)
{
  uint8_t buffer[4];
  buffer[0] = SEPROXYHAL_TAG_ITC_CMD;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = ITC_IO_NFC_START_READER;
  return os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);
}
#endif // HAVE_NFC
