/* @BANNER@ */

#ifndef USBD_LEDGER_HID_U2F_H
#define USBD_LEDGER_HID_U2F_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"
#include "usbd_ledger_types.h"
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/
typedef enum {
  USBD_LEDGER_HID_U2F_SETTING_ID_VERSIONS = 0,
  USBD_LEDGER_HID_U2F_SETTING_ID_CAPABILITIES_FLAG = 1,
  USBD_LEDGER_HID_U2F_SETTING_ID_FREE_CID = 2,
} usdb_ledger_hid_u2f_setting_id_e;

/* Exported defines   --------------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const usbd_class_info_t USBD_LEDGER_HID_U2F_class_info;

/* Exported functions prototypes--------------------------------------------- */
uint8_t USBD_LEDGER_HID_U2F_init(USBD_HandleTypeDef *pdev, void *cookie);
uint8_t USBD_LEDGER_HID_U2F_de_init(USBD_HandleTypeDef *pdev, void *cookie);
uint8_t USBD_LEDGER_HID_U2F_setup(USBD_HandleTypeDef *pdev, void *cookie,
                                  USBD_SetupReqTypedef *req);
uint8_t USBD_LEDGER_HID_U2F_ep0_rx_ready(USBD_HandleTypeDef *pdev,
                                         void *cookie);
uint8_t USBD_LEDGER_HID_U2F_data_in(USBD_HandleTypeDef *pdev, void *cookie,
                                    uint8_t ep_num);
uint8_t USBD_LEDGER_HID_U2F_data_out(USBD_HandleTypeDef *pdev, void *cookie,
                                     uint8_t ep_num, uint8_t *packet,
                                     uint16_t packet_length);

uint8_t USBD_LEDGER_HID_U2F_send_message(USBD_HandleTypeDef *pdev, void *cookie,
                                         uint8_t packet_type,
                                         const uint8_t *message,
                                         uint16_t message_length,
                                         uint32_t timeout_ms);
uint8_t USBD_LEDGER_HID_U2F_is_busy(void *cookie);

int32_t USBD_LEDGER_HID_U2F_data_ready(USBD_HandleTypeDef *pdev, void *cookie,
                                       uint8_t *buffer, uint16_t max_length);

void USBD_LEDGER_HID_U2F_setting(uint32_t id, uint8_t *buffer, uint16_t length,
                                 void *cookie);

#endif // USBD_LEDGER_HID_U2F_H
