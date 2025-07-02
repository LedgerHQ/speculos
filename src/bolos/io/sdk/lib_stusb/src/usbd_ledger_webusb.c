/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_ledger_webusb.h"
#include "ledger_protocol.h"
#include "usbd_ioreq.h"
#include "usbd_ledger.h"

#ifdef HAVE_WEBUSB

#pragma GCC diagnostic ignored "-Wcast-qual"

/* Private enumerations ------------------------------------------------------*/
enum ledger_webusb_state_t {
  LEDGER_WEBUSB_STATE_IDLE,
  LEDGER_WEBUSB_STATE_BUSY,
};

/* Private defines------------------------------------------------------------*/
#define LEDGER_WEBUSB_EPIN_ADDR  (0x83)
#define LEDGER_WEBUSB_EPIN_SIZE  (0x40)
#define LEDGER_WEBUSB_EPOUT_ADDR (0x03)
#define LEDGER_WEBUSB_EPOUT_SIZE (0x40)

#define WINUSB_GET_COMPATIBLE_ID_FEATURE          (0x04)
#define WINUSB_GET_EXTENDED_PROPERTIES_OS_FEATURE (0x05)
#define MS_OS_20_DESCRIPTOR_INDEX                 (0x07)

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
  uint8_t state; // ledger_webusb_state_t
  ledger_protocol_t protocol_data;
} ledger_webusb_handle_t;

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ledger_webusb_handle_t ledger_webusb_handle;

/* Exported variables --------------------------------------------------------*/

const usbd_end_point_info_t LEDGER_WEBUSB_end_point_info = {
  .ep_in_addr = LEDGER_WEBUSB_EPIN_ADDR,
  .ep_in_size = LEDGER_WEBUSB_EPIN_SIZE,
  .ep_out_addr = LEDGER_WEBUSB_EPOUT_ADDR,
  .ep_out_size = LEDGER_WEBUSB_EPOUT_SIZE,
  .ep_type = USBD_EP_TYPE_INTR,
};

// clang-format off
const uint8_t LEDGER_WEBUSB_bos_descriptor[] = {
    // Webusb device capability descriptor
    0x18,                        // bLength
    0x10,                        // bDescriptorType       : Device Capability
    0x05,                        // bDevCapabilityType    : Platform
    0x00,                        // bReserved
    0x38, 0xB6, 0x08, 0x34,      // PlatformCapablityUUID : WebUSB Platform Capability UUID (3408B638-09A9-47A0-8BFD-A0768815B665)
    0xA9, 0x09, 0xA0, 0x47,      // PlatformCapablityUUID
    0x8B, 0xFD, 0xA0, 0x76,      // PlatformCapablityUUID
    0x88, 0x15, 0xB6, 0x65,      // PlatformCapablityUUID
    0x00,                        // bcdVersion            : WebUSB version 1.0
    0x01,                        // bcdVersion
    USB_REQ_WEBUSB_VENDOR_CODE,  // bVendorCode
    0x00,                        // iLandingPage

    // Microsoft OS 2.0 Platform Capability Descriptor
    0x1C,                        // bLength
    0x10,                        // bDescriptorType       : Device Capability
    0x05,                        // bDevCapabilityType    : Platform
    0x00,                        // bReserved
    0xDF, 0x60, 0xDD, 0xD8,      // PlatformCapablityUUID : Microsoft OS 2.0 Platform Capability UUID (D8DD60DF-4589-4CC7-9CD2-659D9E648A9F)
    0x89, 0x45, 0xC7, 0x4C,      // PlatformCapablityUUID
    0x9C, 0xD2, 0x65, 0x9D,      // PlatformCapablityUUID
    0x9E, 0x64, 0x8A, 0x9F,      // PlatformCapablityUUID
    0x00, 0x00, 0x03, 0x06,      // dwWindowsVersion      : minimum 8.1 (0x06030000)
    0xb2,                        // wMSOSDescriptorSetTotalLength : TODO_IO : should be processed at run time
    0x00,                        // wMSOSDescriptorSetTotalLength
    USB_REQ_WINUSB_VENDOR_CODE,  // bVendorCode
    0x00,                        // bAltEnumCode : alternate enumeration not supported
};

const uint8_t LEDGER_WEBUSB_descriptors[23] = {
    /************** Interface descriptor ******************************/
    USB_LEN_IF_DESC,           // bLength
    USB_DESC_TYPE_INTERFACE,   // bDescriptorType    : interface
    0x00,                      // bInterfaceNumber   : 0 (dynamic)
    0x00,                      // bAlternateSetting  : 0
    0x02,                      // bNumEndpoints      : 2
    0xFF,                      // bInterfaceClass    : Vendor
    0xFF,                      // bInterfaceSubClass : Vendor
    0xFF,                      // bInterfaceProtocol : Vendor
    USBD_IDX_PRODUCT_STR,      // iInterface         : no string

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,           // bLength
    USB_DESC_TYPE_ENDPOINT,    // bDescriptorType
    LEDGER_WEBUSB_EPIN_ADDR,   // bEndpointAddress
    USBD_EP_TYPE_INTR,         // bmAttributes
    LEDGER_WEBUSB_EPIN_SIZE,   // wMaxPacketSize
    0x00,                      // wMaxPacketSize
    0x01,                      // bInterval

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,           // bLength
    USB_DESC_TYPE_ENDPOINT,    // bDescriptorType
    LEDGER_WEBUSB_EPOUT_ADDR,  // bEndpointAddress
    USBD_EP_TYPE_INTR,         // bmAttributes
    LEDGER_WEBUSB_EPOUT_SIZE,  // wMaxPacketSize
    0x00,                      // wMaxPacketSize
    0x01,                      // bInterval
};

const uint8_t USBD_LEDGER_WINUSB_string_descriptor[] = {
    0x12,                                        // bLength
    USB_DESC_TYPE_STRING,                        // bDescriptorType
    'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,  // wData : MSFT100<VENDOR_CODE>
    '1', 0x00, '0', 0x00, '0', 0x00,             //
    USB_REQ_WINUSB_VENDOR_CODE, 0x00             //
};

const uint8_t USBD_LEDGER_WINUSB_compat_id_feature_descriptor[] = {
    0x28, 0x00, 0x00, 0x00,                    // dwLength
    0x00, 0x01,                                // bcdVersion
    0x04, 0x00,                                // wIndex
    0x01,                                      // bNumSections
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // reserved

    0x01,                                      // bInterfaceNumber (dynamic) // TODO_IO
    0x01,                                      // bReserved
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,  // Compatible ID String
    0x00, 0x00, 0x00, 0x00,                    // Sub-compatible ID String
    0x00, 0x00, 0x00, 0x00,                    //
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,        // reserved
};

const uint8_t USBD_LEDGER_WINUSB_extended_properties_feature_descriptor[] = {
    0x92, 0x00, 0x00, 0x00,                      // dwLength
    0x00, 0x01,                                  // bcdVersion
    0x05, 0x00,                                  // wIndex
    0x01, 0x00,                                  // wNumFeatures

    0x88, 0x00, 0x00, 0x00,                      // dwLength:
    0x07, 0x00, 0x00, 0x00,                      // dwPropertyDataType: Multiple NULL-terminated Unicode strings (REG_MULTI_SZ)
    0x2a, 0x00,                                  // wPropertyNameLength
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00,  // PropertyName 'DeviceInterfaceGUIDs'
    'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00,  //
    't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,  //
    'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,  //
    'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00,  //
    0x00, 0x00,                                  //
    0x50, 0x00,                                  // wPropertyDataLength
    '{', 0x00, '1', 0x00, '3', 0x00, 'd', 0x00,  // PropertyData : '{13d63400-2C97-0004-0000-4c6564676572}'
    '6', 0x00, '3', 0x00, '4', 0x00, '0', 0x00,  //
    '0', 0x00, '-', 0x00, '2', 0x00, 'C', 0x00,  //
    '9', 0x00, '7', 0x00, '-', 0x00, '0', 0x00,  //
    '0', 0x00, '0', 0x00, '4', 0x00, '-', 0x00,  //
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,  //
    '-', 0x00, '4', 0x00, 'c', 0x00, '6', 0x00,  //
    '5', 0x00, '6', 0x00, '4', 0x00, '6', 0x00,  //
    '7', 0x00, '6', 0x00, '5', 0x00, '7', 0x00,  //
    '2', 0x00, '}', 0x00,                        //
    0x00, 0x00, 0x00, 0x00                       //
};

const uint8_t USBD_LEDGER_WINUSB_ms_os_20_descriptor_set[] = {
    // Microsoft OS 2.0 descriptor set header
    0x0a, 0x00,                                  // wLength
    0x00, 0x00,                                  // wDescriptorType  : MSOS20_SET_HEADER_DESCRIPTOR
    0x00, 0x00, 0x03, 0x06,                      // dwWindowsVersion : minimum 8.1 (0x06030000)
    0xb2, 0x00,                                  // wTotalLength     : The size of entire MS OS 2.0 descriptor set

    // Microsoft OS 2.0 configuration subset header
    0x08, 0x00,                                  // wLength
    0x01, 0x00,                                  // wDescriptorType : MS_OS_20_SUBSET_HEADER_CONFIGURATION
    0x00,                                        // bConfigurationValue
    0x00,                                        // bReserved
    0xa8, 0x00,                                  // wTotalLength

    // Microsoft OS 2.0 function subset header
    0x08, 0x00,                                  // wLength
    0x02, 0x00,                                  // wDescriptorType : MS_OS_20_SUBSET_HEADER_FUNCTION
    0x01,                                        // bFirstInterface (dynamic) // TODO_IO
    0x00,                                        // bReserved
    0xa0, 0x00,                                  // wSubsetLength

    // Microsoft OS 2.0 compatible ID descriptor
    0x14, 0x00,                                  // wLength
    0x03, 0x00,                                  // MS_OS_20_FEATURE_COMPATIBLE_ID
    'W', 'I', 'N', 'U', 'S', 'B', 0x00, 0x00,    // Compatible ID String
    0x00, 0x00, 0x00, 0x00,                      // Sub-compatible ID String
    0x00, 0x00, 0x00, 0x00,                      //

    // Microsoft OS 2.0 registry property descriptor
    0x84, 0x00,                                  // wLength:
    0x04, 0x00,                                  // wDescriptorType: MS_OS_20_FEATURE_REG_PROPERTY
    0x07, 0x00,                                  // wPropertyDataType: Multiple NULL-terminated Unicode strings (REG_MULTI_SZ)
    0x2a, 0x00,                                  // wPropertyNameLength
    'D', 0x00, 'e', 0x00, 'v', 0x00, 'i', 0x00,  // PropertyName 'DeviceInterfaceGUIDs'
    'c', 0x00, 'e', 0x00, 'I', 0x00, 'n', 0x00,  //
    't', 0x00, 'e', 0x00, 'r', 0x00, 'f', 0x00,  //
    'a', 0x00, 'c', 0x00, 'e', 0x00, 'G', 0x00,  //
    'U', 0x00, 'I', 0x00, 'D', 0x00, 's', 0x00,  //
    0x00, 0x00,                                  //
    0x50, 0x00,                                  // wPropertyDataLength
    '{', 0x00, 'C', 0x00, 'E', 0x00, '8', 0x00,  // PropertyData : '{CE809264-4B24-4E81-A8B2-57ED01D580E1}'
    '0', 0x00, '9', 0x00, '2', 0x00, '6', 0x00,  //
    '4', 0x00, '-', 0x00, '4', 0x00, 'B', 0x00,  //
    '2', 0x00, '4', 0x00, '-', 0x00, '4', 0x00,  //
    'E', 0x00, '8', 0x00, '1', 0x00, '-', 0x00,  //
    'A', 0x00, '8', 0x00, 'B', 0x00, '2', 0x00,  //
    '-', 0x00, '5', 0x00, '7', 0x00, 'E', 0x00,  //
    'D', 0x00, '0', 0x00, '1', 0x00, 'D', 0x00,  //
    '5', 0x00, '8', 0x00, '0', 0x00, 'E', 0x00,  //
    '1', 0x00, '}', 0x00,                        //
    0x00, 0x00, 0x00, 0x00                       //
};
// clang-format on

const usbd_class_info_t USBD_LEDGER_WEBUSB_class_info = {
  .type = USBD_LEDGER_CLASS_WEBUSB,

  .end_point = &LEDGER_WEBUSB_end_point_info,

  .init = USBD_LEDGER_WEBUSB_init,
  .de_init = USBD_LEDGER_WEBUSB_de_init,
  .setup = USBD_LEDGER_WEBUSB_setup,
  .data_in = USBD_LEDGER_WEBUSB_data_in,
  .data_out = USBD_LEDGER_WEBUSB_data_out,

  .send_packet = USBD_LEDGER_WEBUSB_send_packet,
  .is_busy = USBD_LEDGER_WEBUSB_is_busy,

  .data_ready = USBD_LEDGER_WEBUSB_data_ready,

  .setting = NULL,

  .interface_descriptor = LEDGER_WEBUSB_descriptors,
  .interface_descriptor_size = sizeof(LEDGER_WEBUSB_descriptors),

  .interface_association_descriptor = NULL,
  .interface_association_descriptor_size = 0,

  .bos_descriptor = LEDGER_WEBUSB_bos_descriptor,
  .bos_descriptor_size = sizeof(LEDGER_WEBUSB_bos_descriptor),

  .cookie = &ledger_webusb_handle,
};

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
uint8_t USBD_LEDGER_WEBUSB_init(USBD_HandleTypeDef *pdev, void *cookie)
{
  if (!cookie) {
    return USBD_FAIL;
  }

  UNUSED(pdev);

  ledger_webusb_handle_t *handle = (ledger_webusb_handle_t *)PIC(cookie);

  memset(handle, 0, sizeof(ledger_webusb_handle_t));

  memset(&handle->protocol_data, 0, sizeof(handle->protocol_data));
  handle->protocol_data.rx_apdu_buffer = USBD_LEDGER_io_buffer;
  handle->protocol_data.rx_apdu_buffer_size = sizeof(USBD_LEDGER_io_buffer);
  handle->protocol_data.tx_chunk_buffer = USBD_LEDGER_protocol_chunk_buffer;
  handle->protocol_data.tx_chunk_buffer_size =
      sizeof(USBD_LEDGER_protocol_chunk_buffer);
  handle->protocol_data.mtu = sizeof(USBD_LEDGER_protocol_chunk_buffer);

  LEDGER_PROTOCOL_init(&handle->protocol_data,
                       OS_IO_PACKET_TYPE_USB_WEBUSB_APDU);

  USBD_LL_PrepareReceive(pdev, LEDGER_WEBUSB_EPOUT_ADDR, NULL,
                         LEDGER_WEBUSB_EPOUT_SIZE);

  return USBD_OK;
}

uint8_t USBD_LEDGER_WEBUSB_de_init(USBD_HandleTypeDef *pdev, void *cookie)
{
  UNUSED(pdev);
  UNUSED(cookie);

  return USBD_OK;
}

uint8_t USBD_LEDGER_WEBUSB_setup(USBD_HandleTypeDef *pdev, void *cookie,
                                 USBD_SetupReqTypedef *req)
{
  if (!pdev || !req) {
    return USBD_FAIL;
  }

  UNUSED(cookie);

  uint8_t ret = USBD_OK;

  if ((req->bRequest == USB_REQ_GET_DESCRIPTOR) &&
      ((req->wValue >> 8) == USB_DESC_TYPE_STRING) &&
      ((req->wValue & 0xFF) == USBD_IDX_WINUSB_STR)) {
    USBD_CtlSendData(pdev, (uint8_t *)PIC(USBD_LEDGER_WINUSB_string_descriptor),
                     sizeof(USBD_LEDGER_WINUSB_string_descriptor));
  } else if (req->bRequest == USB_REQ_WINUSB_VENDOR_CODE) {
    switch (req->wIndex) {
    case WINUSB_GET_COMPATIBLE_ID_FEATURE:
      USBD_CtlSendData(
          pdev, (uint8_t *)PIC(USBD_LEDGER_WINUSB_compat_id_feature_descriptor),
          sizeof(USBD_LEDGER_WINUSB_compat_id_feature_descriptor));
      break;

    case WINUSB_GET_EXTENDED_PROPERTIES_OS_FEATURE:
      USBD_CtlSendData(
          pdev,
          (uint8_t *)PIC(
              USBD_LEDGER_WINUSB_extended_properties_feature_descriptor),
          sizeof(USBD_LEDGER_WINUSB_extended_properties_feature_descriptor));
      break;

    case MS_OS_20_DESCRIPTOR_INDEX:
      USBD_CtlSendData(
          pdev, (uint8_t *)PIC(USBD_LEDGER_WINUSB_ms_os_20_descriptor_set),
          sizeof(USBD_LEDGER_WINUSB_ms_os_20_descriptor_set));
      break;

    default:
      ret = USBD_FAIL;
    }
  } else {
    ret = USBD_FAIL;
  }

  return ret;
}

uint8_t USBD_LEDGER_WEBUSB_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie)
{
  UNUSED(pdev);
  UNUSED(cookie);

  return USBD_OK;
}

uint8_t USBD_LEDGER_WEBUSB_data_in(USBD_HandleTypeDef *pdev, void *cookie,
                                   uint8_t ep_num)
{
  if (!cookie) {
    return USBD_FAIL;
  }

  UNUSED(pdev);
  UNUSED(ep_num);

  ledger_webusb_handle_t *handle = (ledger_webusb_handle_t *)PIC(cookie);

  if (handle->protocol_data.tx_apdu_buffer) {
    LEDGER_PROTOCOL_tx(&handle->protocol_data, NULL, 0);
    if (handle->protocol_data.tx_chunk_length >= 2) {
      handle->state = LEDGER_WEBUSB_STATE_BUSY;
      USBD_LL_Transmit(pdev, LEDGER_WEBUSB_EPIN_ADDR,
                       handle->protocol_data.tx_chunk_buffer,
                       LEDGER_WEBUSB_EPIN_SIZE, 0);
    }
  }
  if (!handle->protocol_data.tx_apdu_buffer) {
    handle->protocol_data.tx_chunk_length = 0;
    handle->state = LEDGER_WEBUSB_STATE_IDLE;
  }

  return USBD_OK;
}

uint8_t USBD_LEDGER_WEBUSB_data_out(USBD_HandleTypeDef *pdev, void *cookie,
                                    uint8_t ep_num, uint8_t *packet,
                                    uint16_t packet_length)
{
  if (!pdev || !cookie || !packet) {
    return USBD_FAIL;
  }

  UNUSED(ep_num);

  ledger_webusb_handle_t *handle = (ledger_webusb_handle_t *)PIC(cookie);

  LEDGER_PROTOCOL_rx(&handle->protocol_data, packet, packet_length);

  USBD_LL_PrepareReceive(pdev, LEDGER_WEBUSB_EPOUT_ADDR, NULL,
                         LEDGER_WEBUSB_EPOUT_SIZE);

  return USBD_OK;
}

uint8_t USBD_LEDGER_WEBUSB_send_packet(USBD_HandleTypeDef *pdev, void *cookie,
                                       uint8_t packet_type,
                                       const uint8_t *packet,
                                       uint16_t packet_length,
                                       uint32_t timeout_ms)
{
  if (!pdev || !cookie || !packet) {
    return USBD_FAIL;
  }

  UNUSED(packet_type);

  uint8_t ret = USBD_OK;
  ledger_webusb_handle_t *handle = (ledger_webusb_handle_t *)PIC(cookie);

  LEDGER_PROTOCOL_tx(&handle->protocol_data, packet, packet_length);

  if (pdev->dev_state == USBD_STATE_CONFIGURED) {
    if (handle->state == LEDGER_WEBUSB_STATE_IDLE) {
      if (handle->protocol_data.tx_chunk_length >= 2) {
        handle->state = LEDGER_WEBUSB_STATE_BUSY;
        ret = USBD_LL_Transmit(pdev, LEDGER_WEBUSB_EPIN_ADDR,
                               handle->protocol_data.tx_chunk_buffer,
                               LEDGER_WEBUSB_EPIN_SIZE, timeout_ms);
      } else {
        ret = USBD_FAIL;
      }
    } else {
      ret = USBD_BUSY;
    }
  } else {
    ret = USBD_FAIL;
  }

  return ret;
}

uint8_t USBD_LEDGER_WEBUSB_is_busy(void *cookie)
{
  ledger_webusb_handle_t *handle = (ledger_webusb_handle_t *)PIC(cookie);

  if (handle->state == LEDGER_WEBUSB_STATE_BUSY) {
    return 1;
  }

  return 0;
}

int32_t USBD_LEDGER_WEBUSB_data_ready(USBD_HandleTypeDef *pdev, void *cookie,
                                      uint8_t *buffer, uint16_t max_length)
{
  int32_t status = 0;

  UNUSED(pdev);

  if (!cookie || !buffer) {
    return -1;
  }
  ledger_webusb_handle_t *handle = (ledger_webusb_handle_t *)PIC(cookie);

  if (handle->protocol_data.rx_apdu_status == APDU_STATUS_COMPLETE) {
    if (max_length < handle->protocol_data.rx_apdu_length) {
      status = -1;
    } else {
      memmove(buffer, handle->protocol_data.rx_apdu_buffer,
              handle->protocol_data.rx_apdu_length);
      status = handle->protocol_data.rx_apdu_length;
    }
    handle->protocol_data.rx_apdu_status = APDU_STATUS_WAITING;
  }

  return status;
}

#endif // HAVE_WEBUSB
