/* @BANNER@ */

#ifndef USBD_DEF_H
#define USBD_DEF_H

/* Includes ------------------------------------------------------------------*/
#include "usbd_conf.h"

/* Exported enumerations -----------------------------------------------------*/
/* Following USB Device Speed */
typedef enum {
  USBD_SPEED_HIGH = 0U,
  USBD_SPEED_FULL = 1U,
  USBD_SPEED_LOW = 2U,
} USBD_SpeedTypeDef;

/* Following USB Device status */
typedef enum {
  USBD_OK = 0U,
  USBD_BUSY,
  USBD_EMEM,
  USBD_FAIL,
  USBD_TIMEOUT,
} USBD_StatusTypeDef;

/* Exported defines   --------------------------------------------------------*/
#define USB_LEN_DEV_QUALIFIER_DESC   0x0AU
#define USB_LEN_DEV_DESC             0x12U
#define USB_LEN_CFG_DESC             0x09U
#define USB_LEN_IF_DESC              0x09U
#define USB_LEN_EP_DESC              0x07U
#define USB_LEN_OTG_DESC             0x03U
#define USB_LEN_LANGID_STR_DESC      0x04U
#define USB_LEN_OTHER_SPEED_DESC_SIZ 0x09U

#define USBD_IDX_LANGID_STR    0x00U
#define USBD_IDX_MFC_STR       0x01U
#define USBD_IDX_PRODUCT_STR   0x02U
#define USBD_IDX_SERIAL_STR    0x03U
#define USBD_IDX_CONFIG_STR    0x04U
#define USBD_IDX_INTERFACE_STR 0x05U
#define USBD_IDX_WINUSB_STR    0xEEU

#define USB_REQ_TYPE_STANDARD 0x00U
#define USB_REQ_TYPE_CLASS    0x20U
#define USB_REQ_TYPE_VENDOR   0x40U
#define USB_REQ_TYPE_MASK     0x60U

#define USB_REQ_RECIPIENT_DEVICE    0x00U
#define USB_REQ_RECIPIENT_INTERFACE 0x01U
#define USB_REQ_RECIPIENT_ENDPOINT  0x02U
#define USB_REQ_RECIPIENT_MASK      0x03U

#define USB_REQ_GET_STATUS         0x00U
#define USB_REQ_CLEAR_FEATURE      0x01U
#define USB_REQ_SET_FEATURE        0x03U
#define USB_REQ_SET_ADDRESS        0x05U
#define USB_REQ_GET_DESCRIPTOR     0x06U
#define USB_REQ_SET_DESCRIPTOR     0x07U
#define USB_REQ_GET_CONFIGURATION  0x08U
#define USB_REQ_SET_CONFIGURATION  0x09U
#define USB_REQ_GET_INTERFACE      0x0AU
#define USB_REQ_SET_INTERFACE      0x0BU
#define USB_REQ_SYNCH_FRAME        0x0CU
#define USB_REQ_WEBUSB_VENDOR_CODE 0x1EU
#define USB_REQ_WINUSB_VENDOR_CODE 0x77U

#define USB_DESC_TYPE_DEVICE                    0x01U
#define USB_DESC_TYPE_CONFIGURATION             0x02U
#define USB_DESC_TYPE_STRING                    0x03U
#define USB_DESC_TYPE_INTERFACE                 0x04U
#define USB_DESC_TYPE_ENDPOINT                  0x05U
#define USB_DESC_TYPE_DEVICE_QUALIFIER          0x06U
#define USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION 0x07U
#define USB_DESC_TYPE_IAD                       0x0BU
#define USB_DESC_TYPE_BOS                       0x0FU

#define USB_CONFIG_REMOTE_WAKEUP 0x02U
#define USB_CONFIG_SELF_POWERED  0x01U

#define USB_FEATURE_EP_HALT       0x00U
#define USB_FEATURE_REMOTE_WAKEUP 0x01U
#define USB_FEATURE_TEST_MODE     0x02U

#define USB_DEVICE_CAPABITY_TYPE 0x10U

#define USB_CONF_DESC_SIZE 0x09U
#define USB_IF_DESC_SIZE   0x09U
#define USB_EP_DESC_SIZE   0x07U
#define USB_IAD_DESC_SIZE  0x08U

#define USB_HS_MAX_PACKET_SIZE 512U
#define USB_FS_MAX_PACKET_SIZE 64U
#define USB_MAX_EP0_SIZE       64U

/*  Device Status */
#define USBD_STATE_DEFAULT    0x01U
#define USBD_STATE_ADDRESSED  0x02U
#define USBD_STATE_CONFIGURED 0x03U
#define USBD_STATE_SUSPENDED  0x04U

/*  EP0 State */
#define USBD_EP0_IDLE       0x00U
#define USBD_EP0_SETUP      0x01U
#define USBD_EP0_DATA_IN    0x02U
#define USBD_EP0_DATA_OUT   0x03U
#define USBD_EP0_STATUS_IN  0x04U
#define USBD_EP0_STATUS_OUT 0x05U
#define USBD_EP0_STALL      0x06U

#define USBD_EP_TYPE_CTRL 0x00U
#define USBD_EP_TYPE_ISOC 0x01U
#define USBD_EP_TYPE_BULK 0x02U
#define USBD_EP_TYPE_INTR 0x03U

/* Exported types, structures, unions ----------------------------------------*/
typedef struct {
  uint8_t bmRequest;
  uint8_t bRequest;
  uint16_t wValue;
  uint16_t wIndex;
  uint16_t wLength;
} USBD_SetupReqTypedef;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumInterfaces;
  uint8_t bConfigurationValue;
  uint8_t iConfiguration;
  uint8_t bmAttributes;
  uint8_t bMaxPower;
} USBD_ConfigDescTypedef;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint16_t wTotalLength;
  uint8_t bNumDeviceCaps;
} USBD_BosDescTypedef;

typedef struct {
  uint8_t bLength;
  uint8_t bDescriptorType;
  uint8_t bEndpointAddress;
  uint8_t bmAttributes;
  uint16_t wMaxPacketSize;
  uint8_t bInterval;
} USBD_EpDescTypedef;

struct _USBD_HandleTypeDef;

/* USB Device class structure */
typedef uint8_t (*USB_Init_t)(struct _USBD_HandleTypeDef *pdev, uint8_t cfgidx);
typedef uint8_t (*USB_DeInit_t)(struct _USBD_HandleTypeDef *pdev,
                                uint8_t cfgidx);

typedef uint8_t (*USB_Setup_t)(struct _USBD_HandleTypeDef *pdev,
                               USBD_SetupReqTypedef *req);
typedef uint8_t (*USB_EP0_TxSent_t)(struct _USBD_HandleTypeDef *pdev);
typedef uint8_t (*USB_EP0_RxReady_t)(struct _USBD_HandleTypeDef *pdev);

typedef uint8_t (*USB_DataIn_t)(struct _USBD_HandleTypeDef *pdev,
                                uint8_t epnum);
typedef uint8_t (*USB_DataOut_t)(struct _USBD_HandleTypeDef *pdev,
                                 uint8_t epnum);
typedef uint8_t (*USB_SOF_t)(struct _USBD_HandleTypeDef *pdev);
typedef uint8_t (*USB_IsoINIncomplete)(struct _USBD_HandleTypeDef *pdev,
                                       uint8_t epnum);
typedef uint8_t (*USB_IsoOUTIncomplete)(struct _USBD_HandleTypeDef *pdev,
                                        uint8_t epnum);

typedef uint8_t *(*USB_GetHSConfigDescriptor_t)(uint16_t *length);
typedef uint8_t *(*USB_GetFSConfigDescriptor_t)(uint16_t *length);
typedef uint8_t *(*USB_GetOtherSpeedConfigDescriptor_t)(uint16_t *length);
typedef uint8_t *(*USB_GetDeviceQualifierDescriptor_t)(uint16_t *length);

typedef struct {
  USB_Init_t Init;
  USB_DeInit_t DeInit;

  USB_Setup_t Setup;
  USB_EP0_TxSent_t EP0_TxSent;
  USB_EP0_RxReady_t EP0_RxReady;

  USB_DataIn_t DataIn;
  USB_DataOut_t DataOut;
  USB_SOF_t SOF;
  USB_IsoINIncomplete IsoINIncomplete;
  USB_IsoOUTIncomplete IsoOUTIncomplete;

  USB_GetHSConfigDescriptor_t GetHSConfigDescriptor;
  USB_GetFSConfigDescriptor_t GetFSConfigDescriptor;
  USB_GetOtherSpeedConfigDescriptor_t GetOtherSpeedConfigDescriptor;
  USB_GetDeviceQualifierDescriptor_t GetDeviceQualifierDescriptor;

} USBD_ClassTypeDef;

/* USB Device descriptors structure */
typedef uint8_t *(*USB_GetDeviceDescriptor_t)(USBD_SpeedTypeDef speed,
                                              uint16_t *length);
typedef uint8_t *(*USB_GetLangIDStrDescriptor_t)(USBD_SpeedTypeDef speed,
                                                 uint16_t *length);
typedef uint8_t *(*USB_GetManufacturerStrDescriptor_t)(USBD_SpeedTypeDef speed,
                                                       uint16_t *length);
typedef uint8_t *(*USB_GetProductStrDescriptor_t)(USBD_SpeedTypeDef speed,
                                                  uint16_t *length);
typedef uint8_t *(*USB_GetSerialStrDescriptor_t)(USBD_SpeedTypeDef speed,
                                                 uint16_t *length);
typedef uint8_t *(*USB_GetConfigurationStrDescriptor_t)(USBD_SpeedTypeDef speed,
                                                        uint16_t *length);
typedef uint8_t *(*USB_GetInterfaceStrDescriptor_t)(USBD_SpeedTypeDef speed,
                                                    uint16_t *length);
typedef uint8_t *(*USB_GetBOSDescriptor_t)(USBD_SpeedTypeDef speed,
                                           uint16_t *length);

typedef struct {
  USB_GetDeviceDescriptor_t GetDeviceDescriptor;
  USB_GetLangIDStrDescriptor_t GetLangIDStrDescriptor;
  USB_GetManufacturerStrDescriptor_t GetManufacturerStrDescriptor;
  USB_GetProductStrDescriptor_t GetProductStrDescriptor;
  USB_GetSerialStrDescriptor_t GetSerialStrDescriptor;
  USB_GetConfigurationStrDescriptor_t GetConfigurationStrDescriptor;
  USB_GetInterfaceStrDescriptor_t GetInterfaceStrDescriptor;
  USB_GetBOSDescriptor_t GetBOSDescriptor;
} USBD_DescriptorsTypeDef;

/* USB Device end point structure */
typedef struct {
  uint32_t status;
  uint32_t total_length;
  uint32_t rem_length;
  uint32_t maxpacket;
  uint16_t is_used;
  uint16_t bInterval;
} USBD_EndpointTypeDef;

/* USB Device handle structure */
typedef struct _USBD_HandleTypeDef {
  uint8_t id;
  uint32_t dev_config;
  uint32_t dev_default_config;
  uint32_t dev_config_status;
  USBD_SpeedTypeDef dev_speed;
  USBD_EndpointTypeDef ep_in[IO_USB_MAX_ENDPOINTS];
  USBD_EndpointTypeDef ep_out[IO_USB_MAX_ENDPOINTS];
  uint32_t ep0_state;
  uint32_t ep0_data_len;
  uint8_t dev_state;
  uint8_t dev_old_state;
  uint8_t dev_address;
  uint8_t dev_connection_status;
  uint32_t dev_remote_wakeup;
  uint8_t ConfIdx;

  USBD_SetupReqTypedef request;
  USBD_DescriptorsTypeDef *pDesc;
  USBD_ClassTypeDef *pClass;
  void *pClassData;
  void *pUserData;
  void *pData;
  void *pBosDesc;
  void *pConfDesc;
} USBD_HandleTypeDef;

/* Exported macros------------------------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Exported functions prototypes--------------------------------------------- */

static __inline uint16_t SWAPBYTE(uint8_t *addr)
{
  uint16_t _SwapVal, _Byte1, _Byte2;
  uint8_t *_pbuff = addr;

  _Byte1 = *(uint8_t *)_pbuff;
  _pbuff++;
  _Byte2 = *(uint8_t *)_pbuff;

  _SwapVal = (_Byte2 << 8) | _Byte1;

  return _SwapVal;
}

#ifndef LOBYTE
#define LOBYTE(x) ((uint8_t)((x)&0x00FFU))
#endif

#ifndef HIBYTE
#define HIBYTE(x) ((uint8_t)(((x)&0xFF00U) >> 8U))
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#if defined(__GNUC__)
#ifndef __weak
#define __weak __attribute__((weak))
#endif /* __weak */
#ifndef __packed
#define __packed __attribute__((__packed__))
#endif /* __packed */
#endif /* __GNUC__ */

#endif // USBD_DEF_H
