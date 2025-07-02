/* @BANNER@ */

#pragma once

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/
enum {
  APDU_STATUS_WAITING,
  APDU_STATUS_NEED_MORE_DATA,
  APDU_STATUS_COMPLETE,
};

/* Exported types, structures, unions ----------------------------------------*/
typedef struct ledger_protocol_s {
  uint8_t type;

  const uint8_t *tx_apdu_buffer;
  uint16_t tx_apdu_length;
  uint16_t tx_apdu_sequence_number;
  uint16_t tx_apdu_offset;

  uint8_t *tx_chunk_buffer;
  uint8_t tx_chunk_buffer_size;
  uint8_t tx_chunk_length;

  uint8_t *rx_apdu_buffer;
  uint16_t rx_apdu_buffer_size;
  uint8_t rx_apdu_status;
  uint16_t rx_apdu_sequence_number;
  uint16_t rx_apdu_length;
  uint16_t rx_apdu_offset;

  uint16_t mtu;
  uint8_t mtu_negotiated;
} ledger_protocol_t;

/* Exported defines   --------------------------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */
void LEDGER_PROTOCOL_init(ledger_protocol_t *data, uint8_t type);
void LEDGER_PROTOCOL_rx(ledger_protocol_t *data, uint8_t *buffer,
                        uint16_t length);
void LEDGER_PROTOCOL_tx(ledger_protocol_t *data, const uint8_t *buffer,
                        uint16_t length);
