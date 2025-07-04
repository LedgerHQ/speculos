/* @BANNER@ */

#ifndef USBD_LEDGER_HID_KBD_H
#define USBD_LEDGER_HID_KBD_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"
#include "usbd_ledger_types.h"
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const usbd_class_info_t USBD_LEDGER_HID_KBD_class_info;

/* Exported functions prototypes--------------------------------------------- */
uint8_t USBD_LEDGER_HID_KBD_init(USBD_HandleTypeDef *pdev, void *cookie);
uint8_t USBD_LEDGER_HID_KBD_de_init(USBD_HandleTypeDef *pdev, void *cookie);
uint8_t USBD_LEDGER_HID_KBD_setup(USBD_HandleTypeDef *pdev, void *cookie,
                                  USBD_SetupReqTypedef *req);
uint8_t USBD_LEDGER_HID_KBD_ep0_rx_ready(USBD_HandleTypeDef *pdev,
                                         void *cookie);
uint8_t USBD_LEDGER_HID_KBD_data_in(USBD_HandleTypeDef *pdev, void *cookie,
                                    uint8_t ep_num);
uint8_t USBD_LEDGER_HID_KBD_data_out(USBD_HandleTypeDef *pdev, void *cookie,
                                     uint8_t ep_num, uint8_t *packet,
                                     uint16_t packet_length);

uint8_t USBD_LEDGER_HID_KBD_send_packet(USBD_HandleTypeDef *pdev, void *cookie,
                                        uint8_t packet_type,
                                        const uint8_t *packet,
                                        uint16_t packet_length,
                                        uint32_t timeout_ms);

#endif // USBD_LEDGER_HID_KBD_H
