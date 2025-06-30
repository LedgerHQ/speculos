/* @BANNER@ */

#ifndef USBD_LEDGER_WEBUSB_H
#define USBD_LEDGER_WEBUSB_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"
#include "usbd_ledger_types.h"
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const usbd_class_info_t USBD_LEDGER_WEBUSB_class_info;

/* Exported functions prototypes--------------------------------------------- */
uint8_t USBD_LEDGER_WEBUSB_init(USBD_HandleTypeDef *pdev, void *cookie);
uint8_t USBD_LEDGER_WEBUSB_de_init(USBD_HandleTypeDef *pdev, void *cookie);
uint8_t USBD_LEDGER_WEBUSB_setup(USBD_HandleTypeDef *pdev, void *cookie,
                                 USBD_SetupReqTypedef *req);
uint8_t USBD_LEDGER_WEBUSB_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie);
uint8_t USBD_LEDGER_WEBUSB_data_in(USBD_HandleTypeDef *pdev, void *cookie,
                                   uint8_t ep_num);
uint8_t USBD_LEDGER_WEBUSB_data_out(USBD_HandleTypeDef *pdev, void *cookie,
                                    uint8_t ep_num, uint8_t *packet,
                                    uint16_t packet_length);

uint8_t USBD_LEDGER_WEBUSB_send_packet(USBD_HandleTypeDef *pdev, void *cookie,
                                       uint8_t packet_type,
                                       const uint8_t *packet,
                                       uint16_t packet_length,
                                       uint32_t timeout_ms);
uint8_t USBD_LEDGER_WEBUSB_is_busy(void *cookie);

int32_t USBD_LEDGER_WEBUSB_data_ready(USBD_HandleTypeDef *pdev, void *cookie,
                                      uint8_t *buffer, uint16_t max_length);

#endif // USBD_LEDGER_WEBUSB_H
