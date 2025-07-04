/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_ledger_hid.h"
#include "ledger_protocol.h"
#include "usbd_ioreq.h"
#include "usbd_ledger.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

/* Private enumerations ------------------------------------------------------*/
enum ledger_hid_state_t {
  LEDGER_HID_STATE_IDLE,
  LEDGER_HID_STATE_BUSY,
};

/* Private defines------------------------------------------------------------*/
#define LEDGER_HID_EPIN_ADDR  (0x82)
#define LEDGER_HID_EPIN_SIZE  (0x40)
#define LEDGER_HID_EPOUT_ADDR (0x02)
#define LEDGER_HID_EPOUT_SIZE (0x40)

#define HID_DESCRIPTOR_TYPE        (0x21)
#define HID_REPORT_DESCRIPTOR_TYPE (0x22)

// HID Class-Specific Requests
#define REQ_SET_REPORT   (0x09)
#define REQ_GET_REPORT   (0x01)
#define REQ_SET_IDLE     (0x0A)
#define REQ_GET_IDLE     (0x02)
#define REQ_SET_PROTOCOL (0x0B)
#define REQ_GET_PROTOCOL (0x03)

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
  uint8_t protocol;
  uint8_t idle_state;
  uint8_t alt_setting;
  uint8_t state; // ledger_hid_state_t
  ledger_protocol_t protocol_data;
} ledger_hid_handle_t;

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static ledger_hid_handle_t ledger_hid_handle;

/* Exported variables --------------------------------------------------------*/
const usbd_end_point_info_t LEDGER_HID_end_point_info = {
  .ep_in_addr = LEDGER_HID_EPIN_ADDR,
  .ep_in_size = LEDGER_HID_EPIN_SIZE,
  .ep_out_addr = LEDGER_HID_EPOUT_ADDR,
  .ep_out_size = LEDGER_HID_EPOUT_SIZE,
  .ep_type = USBD_EP_TYPE_INTR,
};

// clang-format off
const uint8_t LEDGER_HID_report_descriptor[34] = {
    0x06, 0xA0, 0xFF,            // Usage page      : vendor defined
    0x09, 0x01,                  // Usage ID        : vendor defined
    0xA1, 0x01,                  // Collection      : application

    // The Input report
    0x09, 0x03,                  // Usage ID        : vendor defined
    0x15, 0x00,                  // Logical Minimum : 0
    0x26, 0xFF, 0x00,            // Logical Maximum : 255
    0x75, 0x08,                  // Report Size     : 8 bits
    0x95, LEDGER_HID_EPIN_SIZE,  // Report Count    : 64 fields
    0x81, 0x08,                  // Input           : Data, Array, Absolute, Wrap

    // The Output report
    0x09, 0x04,                   // Usage ID        : vendor defined
    0x15, 0x00,                   // Logical Minimum : 0
    0x26, 0xFF, 0x00,             // Logical Maximum : 255
    0x75, 0x08,                   // Report Size     : 8 bits
    0x95, LEDGER_HID_EPOUT_SIZE,  // Report Count    : 64 fields
    0x91, 0x08,                   // Output          : Data, Array, Absolute, Wrap

    0xC0                          // Collection      : end
};

const uint8_t LEDGER_HID_descriptors[32] = {
    /************** Interface descriptor ******************************/
    USB_LEN_IF_DESC,                       // bLength
    USB_DESC_TYPE_INTERFACE,               // bDescriptorType    : interface
    0x00,                                  // bInterfaceNumber   : 0 (dynamic)
    0x00,                                  // bAlternateSetting  : 0
    0x02,                                  // bNumEndpoints      : 2
    0x03,                                  // bInterfaceClass    : HID
    0x00,                                  // bInterfaceSubClass : none
    0x00,                                  // bInterfaceProtocol : none
    USBD_IDX_PRODUCT_STR,                  // iInterface         :

    /************** HID descriptor ************************************/
    0x09,                                  // bLength
    HID_DESCRIPTOR_TYPE,                   // bDescriptorType : HID
    0x11,                                  // bcdHID
    0x01,                                  // bcdHID          : V1.11
    0x00,                                  // bCountryCode    : not supported
    0x01,                                  // bNumDescriptors : 1
    HID_REPORT_DESCRIPTOR_TYPE,            // bDescriptorType : report
    sizeof(LEDGER_HID_report_descriptor),  // wItemLength
    0x00,

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,                       // bLength
    USB_DESC_TYPE_ENDPOINT,                // bDescriptorType
    LEDGER_HID_EPIN_ADDR,                  // bEndpointAddress
    USBD_EP_TYPE_INTR,                     // bmAttributes
    LEDGER_HID_EPIN_SIZE,                  // wMaxPacketSize
    0x00,                                  // wMaxPacketSize
    0x01,                                  // bInterval

    /************** Endpoint descriptor *******************************/
    USB_LEN_EP_DESC,                       // bLength
    USB_DESC_TYPE_ENDPOINT,                // bDescriptorType
    LEDGER_HID_EPOUT_ADDR,                 // bEndpointAddress
    USBD_EP_TYPE_INTR,                     // bmAttributes
    LEDGER_HID_EPOUT_SIZE,                 // wMaxPacketSize
    0x00,                                  // wMaxPacketSize
    0x01,                                  // bInterval
};
// clang-format on

const usbd_class_info_t USBD_LEDGER_HID_class_info = {
  .type = USBD_LEDGER_CLASS_HID,

  .end_point = &LEDGER_HID_end_point_info,

  .init = USBD_LEDGER_HID_init,
  .de_init = USBD_LEDGER_HID_de_init,
  .setup = USBD_LEDGER_HID_setup,
  .data_in = USBD_LEDGER_HID_data_in,
  .data_out = USBD_LEDGER_HID_data_out,

  .send_packet = USBD_LEDGER_HID_send_packet,
  .is_busy = USBD_LEDGER_HID_is_busy,

  .data_ready = USBD_LEDGER_HID_data_ready,

  .interface_descriptor = LEDGER_HID_descriptors,
  .interface_descriptor_size = sizeof(LEDGER_HID_descriptors),

  .interface_association_descriptor = NULL,
  .interface_association_descriptor_size = 0,

  .bos_descriptor = NULL,
  .bos_descriptor_size = 0,

  .cookie = &ledger_hid_handle,
};

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
uint8_t USBD_LEDGER_HID_init(USBD_HandleTypeDef *pdev, void *cookie)
{
  if (!cookie) {
    return USBD_FAIL;
  }

  UNUSED(pdev);

  ledger_hid_handle_t *handle = (ledger_hid_handle_t *)PIC(cookie);

  memset(handle, 0, sizeof(ledger_hid_handle_t));

  memset(&handle->protocol_data, 0, sizeof(handle->protocol_data));
  handle->protocol_data.rx_apdu_buffer = USBD_LEDGER_io_buffer;
  handle->protocol_data.rx_apdu_buffer_size = sizeof(USBD_LEDGER_io_buffer);
  handle->protocol_data.tx_chunk_buffer = USBD_LEDGER_protocol_chunk_buffer;
  handle->protocol_data.tx_chunk_buffer_size =
      sizeof(USBD_LEDGER_protocol_chunk_buffer);
  handle->protocol_data.mtu = sizeof(USBD_LEDGER_protocol_chunk_buffer);

  LEDGER_PROTOCOL_init(&handle->protocol_data, OS_IO_PACKET_TYPE_USB_HID_APDU);

  USBD_LL_PrepareReceive(pdev, LEDGER_HID_EPOUT_ADDR, NULL,
                         LEDGER_HID_EPOUT_SIZE);

  return USBD_OK;
}

uint8_t USBD_LEDGER_HID_de_init(USBD_HandleTypeDef *pdev, void *cookie)
{
  UNUSED(pdev);
  UNUSED(cookie);

  return USBD_OK;
}

uint8_t USBD_LEDGER_HID_setup(USBD_HandleTypeDef *pdev, void *cookie,
                              USBD_SetupReqTypedef *req)
{
  if (!pdev || !cookie || !req) {
    return USBD_FAIL;
  }

  uint8_t ret = USBD_OK;
  ledger_hid_handle_t *handle = (ledger_hid_handle_t *)PIC(cookie);

  uint8_t request =
      req->bmRequest & (USB_REQ_TYPE_MASK | USB_REQ_RECIPIENT_MASK);

  // HID Standard Requests
  if (request == (USB_REQ_TYPE_STANDARD | USB_REQ_RECIPIENT_INTERFACE)) {
    switch (req->bRequest) {
    case USB_REQ_GET_STATUS:
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        uint16_t status_info = 0x0000;
        USBD_CtlSendData(pdev, (uint8_t *)(void *)&status_info,
                         sizeof(status_info));
      } else {
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_DESCRIPTOR:
      if (req->wValue == ((uint16_t)(HID_DESCRIPTOR_TYPE << 8))) {
        USBD_CtlSendData(
            pdev, (uint8_t *)PIC(&LEDGER_HID_descriptors[USB_LEN_IF_DESC]),
            MIN(LEDGER_HID_descriptors[USB_LEN_IF_DESC], req->wLength));
      } else if (req->wValue == ((uint16_t)(HID_REPORT_DESCRIPTOR_TYPE << 8))) {
        USBD_CtlSendData(
            pdev, (uint8_t *)PIC(LEDGER_HID_report_descriptor),
            MIN(sizeof(LEDGER_HID_report_descriptor), req->wLength));
      } else {
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_GET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        USBD_CtlSendData(pdev, &handle->alt_setting,
                         sizeof(handle->alt_setting));
      } else {
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_SET_INTERFACE:
      if (pdev->dev_state == USBD_STATE_CONFIGURED) {
        handle->alt_setting = (uint8_t)(req->wValue);
      } else {
        ret = USBD_FAIL;
      }
      break;

    case USB_REQ_CLEAR_FEATURE:
      break;

    default:
      ret = USBD_FAIL;
      break;
    }
  }
  // HID Class-Specific Requests
  else if (request == (USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_INTERFACE)) {
    switch (req->bRequest) {
    case REQ_SET_PROTOCOL:
      handle->protocol = (uint8_t)(req->wValue);
      break;

    case REQ_GET_PROTOCOL:
      USBD_CtlSendData(pdev, &handle->protocol, sizeof(handle->protocol));
      break;

    case REQ_SET_IDLE:
      handle->idle_state = (uint8_t)(req->wValue >> 8);
      break;

    case REQ_GET_IDLE:
      USBD_CtlSendData(pdev, &handle->idle_state, sizeof(handle->idle_state));
      break;

    default:
      ret = USBD_FAIL;
      break;
    }
  } else {
    ret = USBD_FAIL;
  }

  return ret;
}

uint8_t USBD_LEDGER_HID_ep0_rx_ready(USBD_HandleTypeDef *pdev, void *cookie)
{
  UNUSED(pdev);
  UNUSED(cookie);

  return USBD_OK;
}

uint8_t USBD_LEDGER_HID_data_in(USBD_HandleTypeDef *pdev, void *cookie,
                                uint8_t ep_num)
{
  if (!cookie) {
    return USBD_FAIL;
  }

  UNUSED(pdev);
  UNUSED(ep_num);

  ledger_hid_handle_t *handle = (ledger_hid_handle_t *)PIC(cookie);

  if (handle->protocol_data.tx_apdu_buffer) {
    LEDGER_PROTOCOL_tx(&handle->protocol_data, NULL, 0);
    if (handle->protocol_data.tx_chunk_length >= 2) {
      handle->state = LEDGER_HID_STATE_BUSY;
      USBD_LL_Transmit(pdev, LEDGER_HID_EPIN_ADDR,
                       handle->protocol_data.tx_chunk_buffer,
                       LEDGER_HID_EPIN_SIZE, 0);
    }
  }
  if (!handle->protocol_data.tx_apdu_buffer) {
    handle->protocol_data.tx_chunk_length = 0;
    handle->state = LEDGER_HID_STATE_IDLE;
  }

  return USBD_OK;
}

uint8_t USBD_LEDGER_HID_data_out(USBD_HandleTypeDef *pdev, void *cookie,
                                 uint8_t ep_num, uint8_t *packet,
                                 uint16_t packet_length)
{
  if (!pdev || !cookie || !packet) {
    return USBD_FAIL;
  }

  UNUSED(ep_num);

  ledger_hid_handle_t *handle = (ledger_hid_handle_t *)PIC(cookie);

  LEDGER_PROTOCOL_rx(&handle->protocol_data, packet, packet_length);

  USBD_LL_PrepareReceive(pdev, LEDGER_HID_EPOUT_ADDR, NULL,
                         LEDGER_HID_EPOUT_SIZE);

  return USBD_OK;
}

uint8_t USBD_LEDGER_HID_send_packet(USBD_HandleTypeDef *pdev, void *cookie,
                                    uint8_t packet_type, const uint8_t *packet,
                                    uint16_t packet_length, uint32_t timeout_ms)
{
  if (!pdev || !cookie || !packet) {
    return USBD_FAIL;
  }

  UNUSED(packet_type);

  uint8_t ret = USBD_OK;
  ledger_hid_handle_t *handle = (ledger_hid_handle_t *)PIC(cookie);

  LEDGER_PROTOCOL_tx(&handle->protocol_data, packet, packet_length);

  if (pdev->dev_state == USBD_STATE_CONFIGURED) {
    if (handle->state == LEDGER_HID_STATE_IDLE) {
      if (handle->protocol_data.tx_chunk_length >= 2) {
        handle->state = LEDGER_HID_STATE_BUSY;
        ret = USBD_LL_Transmit(pdev, LEDGER_HID_EPIN_ADDR,
                               handle->protocol_data.tx_chunk_buffer,
                               LEDGER_HID_EPIN_SIZE, timeout_ms);
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

uint8_t USBD_LEDGER_HID_is_busy(void *cookie)
{
  ledger_hid_handle_t *handle = (ledger_hid_handle_t *)PIC(cookie);

  if (handle->state == LEDGER_HID_STATE_BUSY) {
    return 1;
  }

  return 0;
}

int32_t USBD_LEDGER_HID_data_ready(USBD_HandleTypeDef *pdev, void *cookie,
                                   uint8_t *buffer, uint16_t max_length)
{
  int32_t status = 0;

  UNUSED(pdev);

  if (!cookie || !buffer) {
    return -1;
  }
  ledger_hid_handle_t *handle = (ledger_hid_handle_t *)PIC(cookie);

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
