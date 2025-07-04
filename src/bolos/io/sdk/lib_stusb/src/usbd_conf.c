/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_conf.h"
#include "os.h"
#include "seproxyhal_protocol.h"
#include "usbd_core.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
static uint8_t ep_in_stall_status;
static uint8_t ep_out_stall_status;

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/

USBD_StatusTypeDef USBD_LL_Init(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  ep_in_stall_status = 0;
  ep_out_stall_status = 0;

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DeInit(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Start(USBD_HandleTypeDef *pdev)
{
  uint8_t buffer[5];

  UNUSED(pdev);

  buffer[0] = SEPROXYHAL_TAG_USB_CONFIG;
  buffer[1] = 0;
  buffer[2] = 2;
  buffer[3] = SEPROXYHAL_TAG_USB_CONFIG_ADDR;
  buffer[4] = 0;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 5, NULL);

  buffer[0] = SEPROXYHAL_TAG_USB_CONFIG;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = SEPROXYHAL_TAG_USB_CONFIG_CONNECT;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Stop(USBD_HandleTypeDef *pdev)
{
  uint8_t buffer[4];

  UNUSED(pdev);

  buffer[0] = SEPROXYHAL_TAG_USB_CONFIG;
  buffer[1] = 0;
  buffer[2] = 1;
  buffer[3] = SEPROXYHAL_TAG_USB_CONFIG_DISCONNECT;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 4, NULL);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_OpenEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr,
                                  uint8_t ep_type, uint16_t ep_mps)
{
  uint8_t buffer[8];

  UNUSED(pdev);

  if ((ep_addr & 0x7F) >= 8) {
    return USBD_FAIL;
  }

  ep_in_stall_status = 0;
  ep_out_stall_status = 0;

  buffer[0] = SEPROXYHAL_TAG_USB_CONFIG;
  buffer[1] = 0;
  buffer[2] = 5;
  buffer[3] = SEPROXYHAL_TAG_USB_CONFIG_ENDPOINTS;
  buffer[4] = 1;
  buffer[5] = ep_addr;
  buffer[6] = 0;

  switch (ep_type) {
  case USBD_EP_TYPE_CTRL:
    buffer[6] = SEPROXYHAL_TAG_USB_CONFIG_TYPE_CONTROL;
    break;

  case USBD_EP_TYPE_ISOC:
    buffer[6] = SEPROXYHAL_TAG_USB_CONFIG_TYPE_ISOCHRONOUS;
    break;

  case USBD_EP_TYPE_BULK:
    buffer[6] = SEPROXYHAL_TAG_USB_CONFIG_TYPE_BULK;
    break;

  case USBD_EP_TYPE_INTR:
    buffer[6] = SEPROXYHAL_TAG_USB_CONFIG_TYPE_INTERRUPT;
    break;

  default:
    buffer[6] = SEPROXYHAL_TAG_USB_CONFIG_TYPE_INTERRUPT;
    break;
  }

  buffer[7] = ep_mps;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 8, NULL);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_CloseEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  uint8_t buffer[8];

  UNUSED(pdev);

  if ((ep_addr & 0x7F) >= 8) {
    return USBD_FAIL;
  }

  buffer[0] = SEPROXYHAL_TAG_USB_CONFIG;
  buffer[1] = 0;
  buffer[2] = 5;
  buffer[3] = SEPROXYHAL_TAG_USB_CONFIG_ENDPOINTS;
  buffer[4] = 1;
  buffer[5] = ep_addr;
  buffer[6] = SEPROXYHAL_TAG_USB_CONFIG_TYPE_DISABLED;
  buffer[7] = 0;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 8, NULL);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_FlushEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  UNUSED(pdev);

  if ((ep_addr & 0x7F) >= 8) {
    return USBD_FAIL;
  }

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_StallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  uint8_t buffer[6];

  UNUSED(pdev);

  if ((ep_addr & 0x7F) >= 8) {
    return USBD_FAIL;
  }

  if (ep_addr & 0x80) {
    ep_in_stall_status |= (1 << (ep_addr & 0x7F));
  } else {
    ep_out_stall_status |= (1 << (ep_addr & 0x7F));
  }

  buffer[0] = SEPROXYHAL_TAG_USB_EP_PREPARE;
  buffer[1] = 0;
  buffer[2] = 3;
  buffer[3] = ep_addr;
  buffer[4] = SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_STALL;
  buffer[5] = 0;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 6, NULL);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_ClearStallEP(USBD_HandleTypeDef *pdev,
                                        uint8_t ep_addr)
{
  uint8_t buffer[6];

  UNUSED(pdev);

  if ((ep_addr & 0x7F) >= 8) {
    return USBD_FAIL;
  }

  if (ep_addr & 0x80) {
    ep_in_stall_status &= ~(1 << (ep_addr & 0x7F));
  } else {
    ep_out_stall_status &= ~(1 << (ep_addr & 0x7F));
  }

  buffer[0] = SEPROXYHAL_TAG_USB_EP_PREPARE;
  buffer[1] = 0;
  buffer[2] = 3;
  buffer[3] = ep_addr;
  buffer[4] = SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_UNSTALL;
  buffer[5] = 0;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 6, NULL);

  return USBD_OK;
}

uint8_t USBD_LL_IsStallEP(USBD_HandleTypeDef *pdev, uint8_t ep_addr)
{
  UNUSED(pdev);

  if ((ep_addr & 0x7F) >= 8) {
    return 0;
  }

  if (ep_addr & 0x80) {
    return ep_in_stall_status & (1 << (ep_addr & 0x7F));
  } else {
    return ep_out_stall_status & (1 << (ep_addr & 0x7F));
  }
}

USBD_StatusTypeDef USBD_LL_SetUSBAddress(USBD_HandleTypeDef *pdev,
                                         uint8_t dev_addr)
{
  uint8_t buffer[5];

  UNUSED(pdev);

  buffer[0] = SEPROXYHAL_TAG_USB_CONFIG;
  buffer[1] = 0;
  buffer[2] = 2;
  buffer[3] = SEPROXYHAL_TAG_USB_CONFIG_ADDR;
  buffer[4] = dev_addr;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 5, 0);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Transmit(USBD_HandleTypeDef *pdev, uint8_t ep_addr,
                                    const uint8_t *pbuf, uint32_t size,
                                    uint32_t timeout_ms)
{
  USBD_StatusTypeDef status = USBD_OK;
  uint8_t buffer[6];

  UNUSED(pdev);

  if ((ep_addr & 0x7F) >= 8) {
    return USBD_FAIL;
  }

  buffer[0] = SEPROXYHAL_TAG_USB_EP_PREPARE;
  buffer[1] = (3 + size) >> 8;
  buffer[2] = (3 + size) >> 0;
  buffer[3] = ep_addr;
  buffer[4] = SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_IN;
  buffer[5] = size;
  if (timeout_ms) {
    if (os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 6,
                     (unsigned int *)&timeout_ms) != TIMEOUT) {
      os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, pbuf, size, NULL);
    } else {
      status = USBD_TIMEOUT;
    }
  } else {
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 6, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, pbuf, size, NULL);
  }

  return status;
}

USBD_StatusTypeDef USBD_LL_PrepareReceive(USBD_HandleTypeDef *pdev,
                                          uint8_t ep_addr, uint8_t *pbuf,
                                          uint32_t size)
{
  uint8_t buffer[6];

  UNUSED(pdev);
  UNUSED(pbuf);

  if ((ep_addr & 0x7F) >= 8) {
    return USBD_FAIL;
  }

  buffer[0] = SEPROXYHAL_TAG_USB_EP_PREPARE;
  buffer[1] = 0;
  buffer[2] = 3;
  buffer[3] = ep_addr;
  buffer[4] = SEPROXYHAL_TAG_USB_EP_PREPARE_DIR_OUT;
  buffer[5] = size;
  os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, buffer, 6, NULL);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_BatteryCharging(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  return USBD_FAIL;
}
