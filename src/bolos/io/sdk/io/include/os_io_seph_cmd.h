/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include "seproxyhal_protocol.h"
#include "stdint.h"
#include "ux.h"

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
  TUNE_RESERVED,
  TUNE_BOOT,
  TUNE_CHARGING,
  TUNE_LEDGER_MOMENT,
  TUNE_ERROR,
  TUNE_NEUTRAL,
  TUNE_LOCK,
  TUNE_SUCCESS,
  TUNE_LOOK_AT_ME,
  TUNE_TAP_CASUAL,
  TUNE_TAP_NEXT,
  TUNE_CARD_CONNECT,
  NB_TUNES // Keep at last position
} tune_index_e;

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

int os_io_seph_cmd_general_status(void);
int os_io_seph_cmd_more_time(void);
int os_io_seph_cmd_setup_ticker(unsigned int interval_ms);
int os_io_seph_cmd_device_shutdown(uint8_t critical_battery);
int os_io_seph_cmd_se_reset(void);
int os_io_seph_cmd_usb_disconnect(void);
int os_io_seph_cmd_mcu_status(void);
int os_io_seph_cmd_mcu_go_to_bootloader(void);
int os_io_seph_cmd_mcu_lock(void);
int os_io_seph_cmd_mcu_protect(void);
void os_io_seph_cmd_raw_apdu(const uint8_t *buffer, uint16_t length);

#ifdef HAVE_SHIP_MODE
int os_io_seph_cmd_set_ship_mode(void);
#endif // HAVE_SHIP_MODE

#ifdef HAVE_PRINTF
void os_io_seph_cmd_printf(const char *str, uint16_t charcount);
#endif // HAVE_PRINTF

#ifdef HAVE_SE_TOUCH
int os_io_seph_cmd_set_touch_state(uint8_t enable);
#endif // HAVE_SE_TOUCH

#ifdef HAVE_PIEZO_SOUND
int os_io_seph_cmd_piezo_play_tune(tune_index_e tune_index);
#endif // HAVE_PIEZO_SOUND

#ifdef HAVE_SERIALIZED_NBGL
void os_io_seph_cmd_serialized_nbgl(const uint8_t *buffer, uint16_t length);
#endif // HAVE_SERIALIZED_NBGL

#ifdef HAVE_NOR_FLASH
void os_io_seph_cmd_spi_cs(uint8_t select);
#endif // HAVE_NOR_FLASH

#ifdef HAVE_BLE
int os_io_seph_cmd_ble_start_factory_test(void);
int os_io_ble_cmd_enable(uint8_t enable);
int os_io_ble_cmd_clear_bond_db(void);
int os_io_ble_cmd_name_changed(void);
int os_io_ux_cmd_ble_accept_pairing(uint8_t status);
int os_io_ux_cmd_redisplay(void);
void os_io_ux_cmd_ble_pairing_request(uint8_t type, char *pairing_info,
                                      uint8_t pairing_info_len);
void os_io_ux_cmd_ble_pairing_status(uint8_t pairing_status);
#endif // HAVE_BLE

#ifdef HAVE_NFC
int os_io_nfc_cmd_power(uint8_t power_type);
int os_io_nfc_cmd_stop(void);
int os_io_nfc_cmd_start_ce(void);
int os_io_nfc_cmd_start_reader(void);
#endif // HAVE_NFC
