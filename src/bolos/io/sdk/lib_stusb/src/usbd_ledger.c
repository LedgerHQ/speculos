/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_ledger.h"
#include "usbd_core.h"
#include "usbd_ctlreq.h"
#include "usbd_desc.h"
#include "usbd_ledger_ccid.h"
#include "usbd_ledger_cdc.h"
#include "usbd_ledger_hid.h"
#include "usbd_ledger_hid_kbd.h"
#include "usbd_ledger_hid_u2f.h"
#include "usbd_ledger_webusb.h"
#ifdef HAVE_BOLOS
#include "cx_rng_internal.h"
#else // !HAVE_BOLOS
#include "lcx_rng.h"
#endif // !HAVE_BOLOS
#include "os_io_seph_cmd.h"
#include "seproxyhal_protocol.h"

#pragma GCC diagnostic ignored "-Wcast-qual"

/* Private enumerations ------------------------------------------------------*/
typedef enum {
  USBD_LEDGER_STATE_INITIALIZED = 0xA0,
  USBD_LEDGER_STATE_RUNNING,
  USBD_LEDGER_STATE_STOPPED,
} usb_ledger_state_t;

/* Private defines------------------------------------------------------------*/
#define USBD_BLUE_PRODUCT_STRING       ("Blue")
#define USBD_NANOS_PRODUCT_STRING      ("Nano S")
#define USBD_NANOX_PRODUCT_STRING      ("Nano X")
#define USBD_NANOS_PLUS_PRODUCT_STRING ("Nano S+")
#define USBD_STAX_PRODUCT_STRING       ("Stax")
#define USBD_FLEX_PRODUCT_STRING       ("Flex")

/* Private types, structures, unions -----------------------------------------*/
typedef struct {
  usb_ledger_state_t state;

  usbd_ledger_product_e product;
  uint16_t vid;
  uint16_t pid;
  char name[20];
  uint8_t bcdusb;
  uint8_t usbd_iad;

  uint8_t nb_of_class;
  usbd_class_info_t *class[USBD_MAX_NUM_INTERFACES];
  uint16_t classes;

  USBD_HandleTypeDef usbd_handle;

  uint16_t usb_ep_xfer_len[IO_USB_MAX_ENDPOINTS];
  uint8_t *usb_ep_xfer_buffer[IO_USB_MAX_ENDPOINTS];
} usbd_ledger_data_t;

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
#else // !HAVE_PRINTF
#define LOG_IO(...)
#endif // !HAVE_PRINTF

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
static uint8_t init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx);
static uint8_t de_init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx);
static uint8_t setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t data_in(USBD_HandleTypeDef *pdev, uint8_t ep_num);
static uint8_t data_out(USBD_HandleTypeDef *pdev, uint8_t ep_num);
static uint8_t ep0_rx_ready(USBD_HandleTypeDef *pdev);

static uint8_t *get_cfg_desc(uint16_t *length);
static uint8_t *get_dev_qualifier_desc(uint16_t *length);
static uint8_t *get_bos_desc(USBD_SpeedTypeDef speed, uint16_t *length);

static void forge_configuration_descriptor(void);
static void forge_bos_descriptor(void);

/* Exported variables --------------------------------------------------------*/
uint8_t USBD_LEDGER_io_buffer[OS_IO_BUFFER_SIZE + 1];
uint8_t USBD_LEDGER_protocol_chunk_buffer[0x40];

/* Private variables ---------------------------------------------------------*/
static const USBD_ClassTypeDef USBD_LEDGER_CLASS = {
  init,                   // Init
  de_init,                // DeInit
  setup,                  // Setup
  NULL,                   // EP0_TxSent
  ep0_rx_ready,           // EP0_RxReady
  data_in,                // DataIn
  data_out,               // DataOut
  NULL,                   // SOF
  NULL,                   // IsoINIncomplete
  NULL,                   // IsoOUTIncomplete
  get_cfg_desc,           // GetHSConfigDescriptor
  get_cfg_desc,           // GetFSConfigDescriptor
  get_cfg_desc,           // GetOtherSpeedConfigDescriptor
  get_dev_qualifier_desc, // GetDeviceQualifierDescriptor
};

static const uint8_t interface_descriptor[USB_LEN_CFG_DESC] = {
  USB_LEN_CFG_DESC,            // bLength
  USB_DESC_TYPE_CONFIGURATION, // bDescriptorType
  0x00,                        // wTotalLength (dynamic)
  0x00,                        // wTotalLength (dynamic)
  0x00,                        // bNumInterfaces (dynamic)
  0x01,                        // bConfigurationValue
  USBD_IDX_PRODUCT_STR,        // iConfiguration
  0xC0,                        // bmAttributes: self powered
  0x32,                        // bMaxPower: 100 mA
};

static const uint8_t device_qualifier_decriptor[USB_LEN_DEV_QUALIFIER_DESC] = {
  USB_LEN_DEV_QUALIFIER_DESC,     // bLength
  USB_DESC_TYPE_DEVICE_QUALIFIER, // bDescriptorType
  0x00,                           // bcdUSB
  0x02,                           // bcdUSB
  0x00,                           // bDeviceClass
  0x00,                           // bDeviceSubClass
  0x00,                           // bDeviceProtocol
  0x40,                           // bMaxPacketSize0
  0x01,                           // bNumConfigurations
  0x00,                           // bReserved
};

static const uint8_t bos_descriptor[] = {
  0x05,              // bLength
  USB_DESC_TYPE_BOS, // bDescriptorType
  0x00,              // wTotalLength (dynamic)
  0x00,              // wTotalLength (dynamic)
  0x00,              // bNumDeviceCaps (dynamic)
};

static usbd_ledger_data_t usbd_ledger_data;
static os_io_init_usb_t usbd_ledger_init_data;

static uint8_t usbd_ledger_descriptor[MAX_DESCRIPTOR_SIZE];
static uint16_t usbd_ledger_descriptor_size;

/* Private functions ---------------------------------------------------------*/
static uint8_t init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx)
{
  uint8_t ret = USBD_OK;
  uint8_t index = 0;
  usbd_end_point_info_t *end_point_info = NULL;

  UNUSED(cfg_idx);
  pdev->pClassData = &usbd_ledger_data;

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    if (class_info) {
      end_point_info = (usbd_end_point_info_t *)PIC(class_info->end_point);

      // Open EP IN
      if (end_point_info->ep_in_addr != 0xFF) {
        USBD_LL_OpenEP(pdev, end_point_info->ep_in_addr,
                       end_point_info->ep_type, end_point_info->ep_in_size);
        pdev->ep_in[end_point_info->ep_in_addr & 0x0F].is_used = 1;
      }

      // Open EP OUT
      if (end_point_info->ep_out_addr != 0xFF) {
        USBD_LL_OpenEP(pdev, end_point_info->ep_out_addr,
                       end_point_info->ep_type, end_point_info->ep_out_size);
        pdev->ep_out[end_point_info->ep_out_addr & 0x0F].is_used = 1;
      }

      if (class_info->init) {
        ret = ((usbd_class_init_t)PIC(class_info->init))(pdev,
                                                         class_info->cookie);
      }
    }
  }

  return ret;
}

static uint8_t de_init(USBD_HandleTypeDef *pdev, uint8_t cfg_idx)
{
  uint8_t ret = USBD_OK;
  uint8_t index = 0;
  usbd_end_point_info_t *end_point_info = NULL;

  UNUSED(cfg_idx);

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    if (class_info) {
      end_point_info = (usbd_end_point_info_t *)PIC(class_info->end_point);

      // Close EP IN
      if (end_point_info->ep_in_addr != 0xFF) {
        USBD_LL_CloseEP(pdev, end_point_info->ep_in_addr);
        pdev->ep_in[end_point_info->ep_in_addr & 0x0F].is_used = 0;
      }

      // Close EP OUT
      if (end_point_info->ep_out_addr != 0xFF) {
        USBD_LL_CloseEP(pdev, end_point_info->ep_out_addr);
        pdev->ep_out[end_point_info->ep_out_addr & 0x0F].is_used = 0;
      }

      if (class_info->de_init) {
        ret = ((usbd_class_de_init_t)PIC(class_info->de_init))(
            pdev, class_info->cookie);
      }
    }
  }

  pdev->pClassData = NULL;

  return ret;
}

static uint8_t setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  uint8_t ret = USBD_FAIL;
  uint16_t index = req->wIndex;

  if (index < usbd_ledger_data.nb_of_class) {
    usbd_class_info_t *class_info = usbd_ledger_data.class[index];
    ret = ((usbd_class_setup_t)PIC(class_info->setup))(pdev, class_info->cookie,
                                                       req);
  } else {
    ret = USBD_FAIL;
    USBD_CtlError(pdev, req);
  }

  return ret;
}

static uint8_t data_in(USBD_HandleTypeDef *pdev, uint8_t ep_num)
{
  uint8_t ret = USBD_OK;
  uint8_t index = 0;

  usbd_class_info_t *class_info = NULL;
  usbd_end_point_info_t *end_point_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    end_point_info = (usbd_end_point_info_t *)PIC(class_info->end_point);
    if (((end_point_info->ep_in_addr & 0x7F) == (ep_num & 0x7F)) &&
        (class_info->data_in)) {
      ret = ((usbd_class_data_in_t)PIC(class_info->data_in))(
          pdev, class_info->cookie, ep_num);
    }
  }

  return ret;
}

static uint8_t data_out(USBD_HandleTypeDef *pdev, uint8_t ep_num)
{
  uint8_t ret = USBD_OK;
  uint8_t index = 0;

  usbd_class_info_t *class_info = NULL;
  usbd_end_point_info_t *end_point_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    end_point_info = (usbd_end_point_info_t *)PIC(class_info->end_point);
    if (((end_point_info->ep_out_addr & 0x7F) == (ep_num & 0x7F)) &&
        (class_info->data_out)) {
      ret = ((usbd_class_data_out_t)PIC(class_info->data_out))(
          pdev, class_info->cookie, ep_num,
          usbd_ledger_data.usb_ep_xfer_buffer[ep_num & 0x7F],
          usbd_ledger_data.usb_ep_xfer_len[ep_num & 0x7F]);
    }
  }

  return ret;
}

static uint8_t ep0_rx_ready(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  return USBD_OK;
}

static uint8_t *get_cfg_desc(uint16_t *length)
{
  forge_configuration_descriptor();
  *length = usbd_ledger_descriptor_size;
  return usbd_ledger_descriptor;
}

static uint8_t *get_dev_qualifier_desc(uint16_t *length)
{
  *length = sizeof(device_qualifier_decriptor);
  return (uint8_t *)PIC(device_qualifier_decriptor);
}

static uint8_t *get_bos_desc(USBD_SpeedTypeDef speed, uint16_t *length)
{
  UNUSED(speed);
  forge_bos_descriptor();
  *length = usbd_ledger_descriptor_size;
  return usbd_ledger_descriptor;
}

static void forge_configuration_descriptor(void)
{
  uint8_t offset = 0;
  uint8_t index = 0;

  memset(usbd_ledger_descriptor, 0, sizeof(usbd_ledger_descriptor));

  memcpy(&usbd_ledger_descriptor[offset], interface_descriptor,
         sizeof(interface_descriptor));
  offset += sizeof(interface_descriptor);

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    if (usbd_ledger_data.class[index]) {
      class_info = usbd_ledger_data.class[index];
      // Interface Association Descriptor
      if (class_info->interface_association_descriptor) {
        memcpy(&usbd_ledger_descriptor[offset],
               PIC(class_info->interface_association_descriptor),
               class_info->interface_association_descriptor_size);
        usbd_ledger_descriptor[offset + 2] = index;
        offset += class_info->interface_association_descriptor_size;
      }
      // Interface Descriptor
      memcpy(&usbd_ledger_descriptor[offset],
             PIC(class_info->interface_descriptor),
             class_info->interface_descriptor_size);
      usbd_ledger_descriptor[offset + 2] = index;
      if (class_info->type == USBD_LEDGER_CLASS_CDC_CONTROL) {
        usbd_ledger_descriptor[offset + 18] = index + 1;
        usbd_ledger_descriptor[offset + 26] = index;
        usbd_ledger_descriptor[offset + 27] = index + 1;
      }
      offset += class_info->interface_descriptor_size;
      usbd_ledger_descriptor[4]++;
    }
  }

  usbd_ledger_descriptor[2] = (uint8_t)(offset >> 0);
  usbd_ledger_descriptor[3] = (uint8_t)(offset >> 8);
  usbd_ledger_descriptor_size = offset;
}

static void forge_bos_descriptor(void)
{
  uint8_t offset = 0;
  uint8_t index = 0;

  memset(usbd_ledger_descriptor, 0, sizeof(usbd_ledger_descriptor));

  memcpy(&usbd_ledger_descriptor[offset], bos_descriptor,
         sizeof(bos_descriptor));
  offset += sizeof(bos_descriptor);

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    if ((class_info) && (class_info->bos_descriptor)) {
      memcpy(&usbd_ledger_descriptor[offset], PIC(class_info->bos_descriptor),
             class_info->bos_descriptor_size);
      offset += class_info->bos_descriptor_size;
      usbd_ledger_descriptor[4]++;
    }
  }

  usbd_ledger_descriptor[2] = (uint8_t)(offset >> 0);
  usbd_ledger_descriptor[3] = (uint8_t)(offset >> 8);
  usbd_ledger_descriptor[4] = 2;
  usbd_ledger_descriptor_size = offset;
}

/* Exported functions --------------------------------------------------------*/
void USBD_LEDGER_init(os_io_init_usb_t *init, uint8_t force_restart)
{
  if (!init) {
    return;
  }

  if ((force_restart) ||
      (usbd_ledger_data.state < USBD_LEDGER_STATE_INITIALIZED)) {
    // First time USB is started or forced to restart
    memset(&usbd_ledger_data, 0, sizeof(usbd_ledger_data));
    usbd_ledger_data.state = USBD_LEDGER_STATE_STOPPED;
    LOG_IO("USBD_LEDGER_init deep\n");
  } else {
    LOG_IO("USBD_LEDGER_init\n");
  }

  memcpy(&usbd_ledger_init_data, init, sizeof(usbd_ledger_init_data));
}

void USBD_LEDGER_start(void)
{
  LOG_IO("USBD_LEDGER_start %d %04X:%04X (%04X)", usbd_ledger_data.state,
         usbd_ledger_init_data.pid, usbd_ledger_init_data.vid,
         usbd_ledger_init_data.class_mask);
  if (strlen(usbd_ledger_init_data.name)) {
    LOG_IO(" %s\n", usbd_ledger_init_data.name);
  } else {
    LOG_IO("\n");
  }

#ifndef HAVE_USB_HIDKBD
  usbd_ledger_init_data.class_mask &= ~USBD_LEDGER_CLASS_HID_KBD;
#endif // !HAVE_USB_HIDKBD
#ifndef HAVE_IO_U2F
  usbd_ledger_init_data.class_mask &= ~USBD_LEDGER_CLASS_HID_U2F;
#endif // !HAVE_IO_U2F
#ifndef HAVE_CCID_USB
  usbd_ledger_init_data.class_mask &= ~USBD_LEDGER_CLASS_CCID_BULK;
#endif // !HAVE_CCID_USB
#ifndef HAVE_WEBUSB
  usbd_ledger_init_data.class_mask &= ~USBD_LEDGER_CLASS_WEBUSB;
#endif // !HAVE_WEBUSB
#ifndef HAVE_CDCUSB
  usbd_ledger_init_data.class_mask &= ~USBD_LEDGER_CLASS_CDC;
#endif // !HAVE_CDCUSB

#ifdef HAVE_IO_U2F
  if (usbd_ledger_init_data.class_mask ==
      (USBD_LEDGER_CLASS_HID | USBD_LEDGER_CLASS_WEBUSB)) {
    // Force U2F for an app if necessary to avoid disconnection/connection
#ifdef HAVE_BOLOS
    if (G_io_state != OS_IO_STATE_DASHBOARD) {
      usbd_ledger_init_data.class_mask |= USBD_LEDGER_CLASS_HID_U2F;
    }
#else  // !HAVE_BOLOS
    usbd_ledger_init_data.class_mask |= USBD_LEDGER_CLASS_HID_U2F;
#endif // !HAVE_BOLOS
  }
#endif // HAVE_IO_U2F

  if ((usbd_ledger_data.state == USBD_LEDGER_STATE_INITIALIZED) ||
      (usbd_ledger_data.classes != usbd_ledger_init_data.class_mask) ||
      (usbd_ledger_init_data.vid &&
       (usbd_ledger_data.vid != usbd_ledger_init_data.vid))) {
    // First time USB is started
    // or wanted classes have changed
    // or vendor ID has changed

    if (usbd_ledger_data.state == USBD_LEDGER_STATE_RUNNING) {
      USB_LEDGER_stop();
      usbd_ledger_data.state = USBD_LEDGER_STATE_STOPPED;
    }

    usbd_ledger_data.bcdusb = 0x00;
    usbd_ledger_data.usbd_iad = 0;
    usbd_ledger_data.nb_of_class = 0;

    // Fill the name
    if (!strlen(usbd_ledger_init_data.name)) {
      strlcpy(usbd_ledger_init_data.name, USBD_BLUE_PRODUCT_STRING,
              sizeof(usbd_ledger_data.name));
#if defined(TARGET_NANOS)
      strlcpy(usbd_ledger_init_data.name, USBD_NANOS_PRODUCT_STRING,
              sizeof(usbd_ledger_data.name));
#endif // TARGET_NANOS
#if defined(TARGET_NANOX)
      strlcpy(usbd_ledger_init_data.name, USBD_NANOX_PRODUCT_STRING,
              sizeof(usbd_ledger_data.name));
#endif // TARGET_NANOX
#if defined(TARGET_NANOS2)
      strlcpy(usbd_ledger_init_data.name, USBD_NANOS_PLUS_PRODUCT_STRING,
              sizeof(usbd_ledger_data.name));
#endif // TARGET_NANOS2
#if defined(TARGET_FATSTACKS) || defined(TARGET_STAX)
      strlcpy(usbd_ledger_init_data.name, USBD_STAX_PRODUCT_STRING,
              sizeof(usbd_ledger_data.name));
#endif // TARGET_FATSTACKS || TARGET_STAX
#if defined(TARGET_FLEX)
      strlcpy(usbd_ledger_init_data.name, USBD_FLEX_PRODUCT_STRING,
              sizeof(usbd_ledger_data.name));
#endif // TARGET_FLEX
    }
    strlcpy(usbd_ledger_data.name, usbd_ledger_init_data.name,
            sizeof(usbd_ledger_data.name));

    // Fill the product type
    usbd_ledger_data.product = USBD_LEDGER_PRODUCT_BLUE;
#if defined(TARGET_NANOS)
    usbd_ledger_data.product = USBD_LEDGER_PRODUCT_NANOS;
#endif // TARGET_NANOS
#if defined(TARGET_NANOX)
    usbd_ledger_data.product = USBD_LEDGER_PRODUCT_NANOX;
#endif // TARGET_NANOX
#if defined(TARGET_NANOS2)
    usbd_ledger_data.product = USBD_LEDGER_PRODUCT_NANOS_PLUS;
#endif // TARGET_NANOS2
#if defined(TARGET_FATSTACKS) || defined(TARGET_STAX)
    usbd_ledger_data.product = USBD_LEDGER_PRODUCT_STAX;
#endif // TARGET_FATSTACKS || TARGET_STAX
#if defined(TARGET_FLEX)
    usbd_ledger_data.product = USBD_LEDGER_PRODUCT_FLEX;
#endif // TARGET_FLEX

    if (usbd_ledger_init_data.vid) {
      usbd_ledger_data.vid = usbd_ledger_init_data.vid;
    } else {
      usbd_ledger_data.vid = USBD_LEDGER_VID;
    }

    if (usbd_ledger_init_data.pid) {
      usbd_ledger_data.pid = usbd_ledger_init_data.pid;
    } else {
      usbd_ledger_data.pid = usbd_ledger_data.product;
    }

    if (usbd_ledger_init_data.class_mask & USBD_LEDGER_CLASS_HID) {
      usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
          (usbd_class_info_t *)PIC(&USBD_LEDGER_HID_class_info);
    }
#ifdef HAVE_USB_HIDKBD
    else if (usbd_ledger_init_data.class_mask & USBD_LEDGER_CLASS_HID_KBD) {
      usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
          (usbd_class_info_t *)PIC(&USBD_LEDGER_HID_KBD_class_info);
    }
#endif // HAVE_USB_HIDKBD
#ifdef HAVE_IO_U2F
    if (usbd_ledger_init_data.class_mask & USBD_LEDGER_CLASS_HID_U2F) {
      usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
          (usbd_class_info_t *)PIC(&USBD_LEDGER_HID_U2F_class_info);
      uint8_t buffer[4];
      buffer[0] = usbd_ledger_init_data.hid_u2f_settings.protocol_version;
      buffer[1] =
          usbd_ledger_init_data.hid_u2f_settings.major_device_version_number;
      buffer[2] =
          usbd_ledger_init_data.hid_u2f_settings.minor_device_version_number;
      buffer[3] =
          usbd_ledger_init_data.hid_u2f_settings.build_device_version_number;
      USBD_LEDGER_setting(USBD_LEDGER_CLASS_HID_U2F,
                          USBD_LEDGER_HID_U2F_SETTING_ID_VERSIONS, buffer, 4);
      buffer[0] = usbd_ledger_init_data.hid_u2f_settings.capabilities_flag;
      USBD_LEDGER_setting(USBD_LEDGER_CLASS_HID_U2F,
                          USBD_LEDGER_HID_U2F_SETTING_ID_CAPABILITIES_FLAG,
                          buffer, 1);
#ifdef HAVE_BOLOS
      cx_rng_internal(buffer, 4);
#else  // !HAVE_BOLOS
      cx_rng(buffer, 4);
#endif // !HAVE_BOLOS
      USBD_LEDGER_setting(USBD_LEDGER_CLASS_HID_U2F,
                          USBD_LEDGER_HID_U2F_SETTING_ID_FREE_CID, buffer, 4);
    }
#endif // HAVE_IO_U2F
#ifdef HAVE_CCID_USB
    if (usbd_ledger_init_data.class_mask & USBD_LEDGER_CLASS_CCID_BULK) {
      usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
          (usbd_class_info_t *)PIC(&USBD_LEDGER_CCID_Bulk_class_info);
    }
#endif // HAVE_CCID_USB
#ifdef HAVE_WEBUSB
    if (usbd_ledger_init_data.class_mask & USBD_LEDGER_CLASS_WEBUSB) {
      usbd_ledger_data.bcdusb = 0x10;
      usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
          (usbd_class_info_t *)PIC(&USBD_LEDGER_WEBUSB_class_info);
    }
#endif // HAVE_WEBUSB
#ifdef HAVE_CDCUSB
    if (usbd_ledger_init_data.class_mask & USBD_LEDGER_CLASS_CDC) {
      usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
          (usbd_class_info_t *)PIC(&USBD_LEDGER_CDC_Control_class_info);
      usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
          (usbd_class_info_t *)PIC(&USBD_LEDGER_CDC_Data_class_info);
      usbd_ledger_data.usbd_iad = 1;
    }
#endif // HAVE_CDCUSB

    usbd_ledger_data.classes = usbd_ledger_init_data.class_mask;

    USBD_DESC_init(usbd_ledger_data.name, usbd_ledger_data.vid,
                   usbd_ledger_data.pid, usbd_ledger_data.bcdusb,
                   usbd_ledger_data.usbd_iad, get_bos_desc);
    USBD_Init(&usbd_ledger_data.usbd_handle,
              (USBD_DescriptorsTypeDef *)&LEDGER_Desc, 0);
    USBD_RegisterClass(&usbd_ledger_data.usbd_handle,
                       (USBD_ClassTypeDef *)&USBD_LEDGER_CLASS);
  }
  if ((usbd_ledger_data.state == USBD_LEDGER_STATE_STOPPED) ||
      (usbd_ledger_data.state == USBD_LEDGER_STATE_INITIALIZED)) {
    USBD_Start(&usbd_ledger_data.usbd_handle);
    usbd_ledger_data.state = USBD_LEDGER_STATE_RUNNING;
  }
}

void USB_LEDGER_stop(void)
{
  USBD_Stop(&usbd_ledger_data.usbd_handle);
  usbd_ledger_data.state = USBD_LEDGER_STATE_STOPPED;
}

void USBD_LEDGER_add_profile(const usbd_class_info_t *class_info,
                             uint8_t bcdusb, uint8_t usbd_iad)
{
  usbd_ledger_data.class[usbd_ledger_data.nb_of_class++] =
      (usbd_class_info_t *)PIC(class_info);
  usbd_ledger_data.bcdusb |= bcdusb;
  if (usbd_iad) {
    usbd_ledger_data.usbd_iad = usbd_iad;
  }

  USBD_DESC_init(usbd_ledger_data.name, usbd_ledger_data.vid,
                 usbd_ledger_data.pid, usbd_ledger_data.bcdusb,
                 usbd_ledger_data.usbd_iad, get_bos_desc);
}

void USBD_LEDGER_rx_evt_reset(void)
{
  USBD_LL_SetSpeed(&usbd_ledger_data.usbd_handle, USBD_SPEED_FULL);
  USBD_LL_Reset(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_sof(void)
{
  USBD_LL_SOF(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_suspend(void)
{
  USBD_LL_Suspend(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_resume(void)
{
  USBD_LL_Resume(&usbd_ledger_data.usbd_handle);
}

void USBD_LEDGER_rx_evt_setup(uint8_t *buffer)
{
  USBD_LL_SetupStage(&usbd_ledger_data.usbd_handle, buffer);
}

void USBD_LEDGER_rx_evt_data_in(uint8_t ep_num, uint8_t *buffer)
{
  USBD_LL_DataInStage(&usbd_ledger_data.usbd_handle, ep_num, buffer);
}

void USBD_LEDGER_rx_evt_data_out(uint8_t ep_num, uint8_t *buffer,
                                 uint16_t length)
{
  usbd_ledger_data.usb_ep_xfer_len[ep_num & 0x7F] = length;
  usbd_ledger_data.usb_ep_xfer_buffer[ep_num & 0x7F] = buffer;
  USBD_LL_DataOutStage(&usbd_ledger_data.usbd_handle, ep_num, buffer);
}

uint32_t USBD_LEDGER_send(uint8_t class_type, uint8_t packet_type,
                          const uint8_t *packet, uint16_t packet_length,
                          uint32_t timeout_ms)
{
  uint32_t status = INVALID_PARAMETER;
  uint8_t usb_status = USBD_OK;
  uint8_t index = 0;

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    if (class_info->type == class_type) {
      usb_status = ((usbd_send_packet_t)PIC(class_info->send_packet))(
          &usbd_ledger_data.usbd_handle, class_info->cookie, packet_type,
          packet, packet_length, timeout_ms);
      break;
    }
  }

  if (usb_status == USBD_OK) {
    status = SWO_SUCCESS;
  } else if (usb_status == USBD_TIMEOUT) {
    status = TIMEOUT;
  }

  return status;
}

int32_t USBD_LEDGER_data_ready(uint8_t *buffer, uint16_t max_length)
{
  uint8_t index = 0;
  int32_t status = 0;

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    if (class_info->data_ready) {
      status = ((usbd_data_ready_t)PIC(class_info->data_ready))(
          &usbd_ledger_data.usbd_handle, class_info->cookie, buffer,
          max_length);
      if (status > 0) {
        return status;
      }
    }
  }

  return 0;
}

int32_t USBD_LEDGER_is_busy(void)
{
  uint8_t index = 0;
  int32_t status = 0;

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    if (class_info->is_busy) {
      status = ((usbd_is_busy_t)PIC(class_info->is_busy))(class_info->cookie);
      if (status) {
        break;
      }
    }
  }

  return status;
}

void USBD_LEDGER_setting(uint32_t class_id, uint32_t setting_id,
                         uint8_t *buffer, uint16_t length)
{
  uint8_t index = 0;

  usbd_class_info_t *class_info = NULL;
  for (index = 0; index < usbd_ledger_data.nb_of_class; index++) {
    class_info = usbd_ledger_data.class[index];
    if ((class_info->type == class_id) && (class_info->setting)) {
      ((usbd_setting_t)PIC(class_info->setting))(setting_id, buffer, length,
                                                 class_info->cookie);
    }
  }
}

int USBD_LEDGER_rx_seph_evt(uint8_t *seph_buffer, uint16_t seph_buffer_length,
                            uint8_t *apdu_buffer,
                            uint16_t apdu_buffer_max_length)
{
  int status = 0;

  if (seph_buffer[1] == SEPROXYHAL_TAG_USB_EVENT) {
    switch (seph_buffer[4]) {
    case SEPROXYHAL_TAG_USB_EVENT_RESET:
      USBD_LEDGER_rx_evt_reset();
      break;

    case SEPROXYHAL_TAG_USB_EVENT_SOF:
      USBD_LEDGER_rx_evt_sof();
      break;

    case SEPROXYHAL_TAG_USB_EVENT_SUSPENDED:
      USBD_LEDGER_rx_evt_suspend();
      break;

    case SEPROXYHAL_TAG_USB_EVENT_RESUMED:
      USBD_LEDGER_rx_evt_resume();
      break;

    default:
      status = -1;
      break;
    }
  } else if (seph_buffer[1] == SEPROXYHAL_TAG_USB_EP_XFER_EVENT) {
    uint8_t epnum = seph_buffer[4] & 0x7F;
    uint16_t length = MIN(seph_buffer[6], seph_buffer_length - 6);

    switch (seph_buffer[5]) {
    case SEPROXYHAL_TAG_USB_EP_XFER_SETUP:
      USBD_LEDGER_rx_evt_setup(&seph_buffer[7]);
      break;

    case SEPROXYHAL_TAG_USB_EP_XFER_IN:
      if (epnum < IO_USB_MAX_ENDPOINTS) {
        USBD_LEDGER_rx_evt_data_in(epnum, &seph_buffer[7]);
      } else {
        status = -1;
      }
      break;

    case SEPROXYHAL_TAG_USB_EP_XFER_OUT:
      if (epnum < IO_USB_MAX_ENDPOINTS) {
        USBD_LEDGER_rx_evt_data_out(epnum, &seph_buffer[7], length);
        status = USBD_LEDGER_data_ready(apdu_buffer, apdu_buffer_max_length);
      } else {
        status = -1;
      }
      break;

    default:
      status = -1;
      break;
    }
  } else {
    return -1;
  }

  return status;
}
