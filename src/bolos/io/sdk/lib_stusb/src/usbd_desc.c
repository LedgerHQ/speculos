/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_desc.h"
#include "usbd_conf.h"
#include "usbd_core.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/
#define USBD_LANGID_STRING       (0x409)
#define USBD_MANUFACTURER_STRING ("Ledger")

#define USBD_SERIAL_STRING_SIZE (2 + 2 * (7 * 2))

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/
const USBD_DescriptorsTypeDef LEDGER_Desc = {
  USBD_get_descriptor_device,       USBD_get_descriptor_lang_id,
  USBD_get_descriptor_manufacturer, USBD_get_descriptor_product,
  USBD_get_descriptor_serial,       USBD_get_descriptor_product,
  USBD_get_descriptor_product,      USBD_get_descriptor_BOS,
};

/* Private variables ---------------------------------------------------------*/
static uint8_t usbd_bcdusb;
static uint16_t usbd_vid;
static uint16_t usbd_pid;
static char *usbd_desc_product_str;
static uint8_t usbd_iad;
static USB_GetBOSDescriptor_t usbd_bos_descriptor;

/* USB Standard Device Descriptor */
static const uint8_t USBD_desc_device_desc[USB_LEN_DEV_DESC] = {
  0x12,                 // bLength
  USB_DESC_TYPE_DEVICE, // bDescriptorType
  0x00,                 // bcdUSB
  0x02,
  0x00,                    // bDeviceClass
  0x00,                    // bDeviceSubClass
  0x00,                    // bDeviceProtocol
  USB_MAX_EP0_SIZE,        // bMaxPacketSize
  LOBYTE(USBD_LEDGER_VID), // idVendor
  HIBYTE(USBD_LEDGER_VID), // idVendor
  0x00,                    // idProduct
  0x00,                    // idProduct
  0x01,                    // bcdDevice rel. 2.00
  0x02,
  USBD_IDX_MFC_STR,     // Index of manufacturer string
  USBD_IDX_PRODUCT_STR, // Index of product string
  USBD_IDX_SERIAL_STR,  // Index of serial number string
  1                     // bNumConfigurations
};

static uint8_t USBD_desc_device_desc_patched[USB_LEN_DEV_DESC];

/* USB Language ID descriptor */
static const uint8_t USBD_desc_lang_id_desc[USB_LEN_LANGID_STR_DESC] = {
  USB_LEN_LANGID_STR_DESC,
  USB_DESC_TYPE_STRING,
  LOBYTE(USBD_LANGID_STRING),
  HIBYTE(USBD_LANGID_STRING),
};

static uint8_t USBD_StringSerial[USBD_SERIAL_STRING_SIZE];

static uint8_t USBD_StrDesc[USBD_MAX_STR_DESC_SIZ];

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
uint8_t *USBD_get_descriptor_device(USBD_SpeedTypeDef speed, uint16_t *length)
{
  UNUSED(speed);

  memcpy(USBD_desc_device_desc_patched, USBD_desc_device_desc,
         sizeof(USBD_desc_device_desc_patched));
  USBD_desc_device_desc_patched[2] = usbd_bcdusb;
  if (usbd_iad) {
    USBD_desc_device_desc_patched[4] = 0xEF;
    USBD_desc_device_desc_patched[5] = 0x02;
    USBD_desc_device_desc_patched[6] = 0x01;
  } else {
    USBD_desc_device_desc_patched[4] = 0x00;
    USBD_desc_device_desc_patched[5] = 0x00;
    USBD_desc_device_desc_patched[6] = 0x00;
  }
  USBD_desc_device_desc_patched[8] = LOBYTE(usbd_vid);
  USBD_desc_device_desc_patched[9] = HIBYTE(usbd_vid);
  USBD_desc_device_desc_patched[10] = LOBYTE(usbd_pid);
  USBD_desc_device_desc_patched[11] = HIBYTE(usbd_pid);

  *length = sizeof(USBD_desc_device_desc_patched);
  return USBD_desc_device_desc_patched;
}

uint8_t *USBD_get_descriptor_lang_id(USBD_SpeedTypeDef speed, uint16_t *length)
{
  UNUSED(speed);

  *length = sizeof(USBD_desc_lang_id_desc);
  return (uint8_t *)USBD_desc_lang_id_desc;
}

uint8_t *USBD_get_descriptor_manufacturer(USBD_SpeedTypeDef speed,
                                          uint16_t *length)
{
  UNUSED(speed);

  USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, (uint8_t *)USBD_StrDesc,
                 length);

  return (uint8_t *)USBD_StrDesc;
}

uint8_t *USBD_get_descriptor_product(USBD_SpeedTypeDef speed, uint16_t *length)
{
  UNUSED(speed);

  USBD_GetString((uint8_t *)(usbd_desc_product_str), (uint8_t *)USBD_StrDesc,
                 length);

  return (uint8_t *)USBD_StrDesc;
}

uint8_t *USBD_get_descriptor_serial(USBD_SpeedTypeDef speed, uint16_t *length)
{
  UNUSED(speed);

  USBD_StringSerial[0] = 10;
  USBD_StringSerial[1] = USB_DESC_TYPE_STRING;
  USBD_StringSerial[2] = 0x30;
  USBD_StringSerial[3] = 0x00;
  USBD_StringSerial[4] = 0x30;
  USBD_StringSerial[5] = 0x00;
  USBD_StringSerial[6] = 0x30;
  USBD_StringSerial[7] = 0x00;
  USBD_StringSerial[8] = 0x31;
  USBD_StringSerial[9] = 0x00;

  *length = USBD_StringSerial[0];

  return (uint8_t *)USBD_StringSerial;
}

uint8_t *USBD_get_descriptor_BOS(USBD_SpeedTypeDef speed, uint16_t *length)
{
  uint8_t *pbuf = NULL;

  if (usbd_bos_descriptor) {
    pbuf = ((USB_GetBOSDescriptor_t)PIC(usbd_bos_descriptor))(speed, length);
  }

  return pbuf;
}

void USBD_DESC_init(char *product_str, uint16_t vid, uint16_t pid,
                    uint8_t bcdusb, uint8_t iad,
                    USB_GetBOSDescriptor_t bos_descriptor)
{
  usbd_bcdusb = bcdusb;
  usbd_vid = vid;
  usbd_pid = pid;
  usbd_desc_product_str = product_str;
  usbd_iad = iad;
  usbd_bos_descriptor = bos_descriptor;
}
