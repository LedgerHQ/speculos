/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
  NFC_LEDGER_MODE_CARD_EMULATION = 0x00,
  NFC_LEDGER_MODE_READER = 0x01,
} nfc_ledger_mode_e;

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

void NFC_LEDGER_init(uint8_t force_restart);
void NFC_LEDGER_start(uint8_t mode); // nfc_ledger_mode_e
void NFC_LEDGER_stop(void);

// Rx
int NFC_LEDGER_rx_seph_apdu_evt(uint8_t *seph_buffer,
                                uint16_t seph_buffer_length,
                                uint8_t *apdu_buffer,
                                uint16_t apdu_buffer_max_length);

// Tx
uint32_t NFC_LEDGER_send(const uint8_t *packet, uint16_t packet_length,
                         uint32_t timeout_ms);
