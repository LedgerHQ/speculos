/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_ioreq.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
USBD_StatusTypeDef USBD_CtlSendData(USBD_HandleTypeDef *pdev, uint8_t *pbuf,
                                    uint32_t len)
{
  // Set EP0 State
  pdev->ep0_state = USBD_EP0_DATA_IN;
  pdev->ep_in[0].total_length = len;
  pdev->ep_in[0].rem_length = len;
  pdev->pData = pbuf;

  // Start the transfer
  (void)USBD_LL_Transmit(pdev, 0x00U, pbuf, MIN(len, pdev->ep_in[0].maxpacket),
                         0);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_CtlContinueSendData(USBD_HandleTypeDef *pdev,
                                            uint8_t *pbuf, uint32_t len)
{
  // Start the next transfer
  (void)USBD_LL_Transmit(pdev, 0x00U, pbuf, MIN(len, pdev->ep_in[0].maxpacket),
                         0);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_CtlPrepareRx(USBD_HandleTypeDef *pdev, uint8_t *pbuf,
                                     uint32_t len)
{
  // Set EP0 State
  pdev->ep0_state = USBD_EP0_DATA_OUT;
  pdev->ep_out[0].total_length = len;
  pdev->ep_out[0].rem_length = len;

  // Start the transfer
  (void)USBD_LL_PrepareReceive(pdev, 0U, pbuf, len);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_CtlContinueRx(USBD_HandleTypeDef *pdev, uint8_t *pbuf,
                                      uint32_t len)
{
  (void)USBD_LL_PrepareReceive(pdev, 0U, pbuf, len);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_CtlSendStatus(USBD_HandleTypeDef *pdev)
{
  // Set EP0 State
  pdev->ep0_state = USBD_EP0_STATUS_IN;

  // Start the transfer
  (void)USBD_LL_Transmit(pdev, 0x00U, NULL, 0U, 0);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_CtlReceiveStatus(USBD_HandleTypeDef *pdev)
{
  // Set EP0 State
  pdev->ep0_state = USBD_EP0_STATUS_OUT;

  // Start the transfer
  (void)USBD_LL_PrepareReceive(pdev, 0U, NULL, 0U);

  return USBD_OK;
}
