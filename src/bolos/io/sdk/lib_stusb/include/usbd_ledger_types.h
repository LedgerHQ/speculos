/* @BANNER@ */

#ifndef USBD_LEDGER_TYPES_H
#define USBD_LEDGER_TYPES_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/

/* Exported defines   --------------------------------------------------------*/
#define MAX_DESCRIPTOR_SIZE (256)

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
  uint8_t ep_in_addr;
  uint16_t ep_in_size;
  uint8_t ep_out_addr;
  uint16_t ep_out_size;
  uint8_t ep_type;
} usbd_end_point_info_t;

typedef uint8_t (*usbd_class_init_t)(USBD_HandleTypeDef *pdev, void *cookie);
typedef uint8_t (*usbd_class_de_init_t)(USBD_HandleTypeDef *pdev, void *cookie);
typedef uint8_t (*usbd_class_setup_t)(USBD_HandleTypeDef *pdev, void *cookie,
                                      USBD_SetupReqTypedef *req);
typedef uint8_t (*usbd_ep0_rx_ready_t)(USBD_HandleTypeDef *pdev, void *cookie);
typedef uint8_t (*usbd_class_data_in_t)(USBD_HandleTypeDef *pdev, void *cookie,
                                        uint8_t ep_num);
typedef uint8_t (*usbd_class_data_out_t)(USBD_HandleTypeDef *pdev, void *cookie,
                                         uint8_t ep_num, uint8_t *packet,
                                         uint16_t packet_length);

typedef uint8_t (*usbd_send_packet_t)(USBD_HandleTypeDef *pdev, void *cookie,
                                      uint8_t packet_type,
                                      const uint8_t *packet,
                                      uint16_t packet_length,
                                      uint32_t timeout_ms);
typedef uint8_t (*usbd_is_busy_t)(void *cookie);

typedef int32_t (*usbd_data_ready_t)(USBD_HandleTypeDef *pdev, void *cookie,
                                     uint8_t *buffer, uint16_t max_length);

typedef void (*usbd_setting_t)(uint32_t class_id, uint8_t *buffer,
                               uint16_t length, void *cookie);

typedef struct usbd_class_info_t_ {
  uint8_t type; // usbd_ledger_class_mask_e

  const usbd_end_point_info_t *end_point;

  usbd_class_init_t init;
  usbd_class_de_init_t de_init;
  usbd_class_setup_t setup;
  usbd_ep0_rx_ready_t ep0_rx_ready;
  usbd_class_data_in_t data_in;
  usbd_class_data_out_t data_out;

  usbd_send_packet_t send_packet;
  usbd_is_busy_t is_busy;

  usbd_data_ready_t data_ready;

  usbd_setting_t setting;

  uint8_t interface_descriptor_size;
  const uint8_t *interface_descriptor;

  uint8_t interface_association_descriptor_size;
  const uint8_t *interface_association_descriptor;

  uint8_t bos_descriptor_size;
  const uint8_t *bos_descriptor;

  void *cookie;
} usbd_class_info_t;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

#endif // USBD_LEDGER_TYPES_H
