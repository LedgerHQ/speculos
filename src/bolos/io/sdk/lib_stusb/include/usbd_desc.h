/* @BANNER@ */

#ifndef USBD_DESC_H
#define USBD_DESC_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_def.h"
#include <stdint.h>

/* Exported enumerations -----------------------------------------------------*/

/* Exported types, structures, unions ----------------------------------------*/

/* Exported defines   --------------------------------------------------------*/
#define USBD_LEDGER_VID (0x2C97)

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
extern const USBD_DescriptorsTypeDef LEDGER_Desc;

/* Exported functions prototypes--------------------------------------------- */
uint8_t *USBD_get_descriptor_device(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_lang_id(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_manufacturer(USBD_SpeedTypeDef speed,
                                          uint16_t *length);
uint8_t *USBD_get_descriptor_product(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_serial(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *USBD_get_descriptor_BOS(USBD_SpeedTypeDef speed, uint16_t *length);

void USBD_DESC_init(char *product_str, uint16_t vid, uint16_t pid,
                    uint8_t bcdusb, uint8_t iad,
                    USB_GetBOSDescriptor_t bos_descriptor);

#endif // USBD_DESC_H
