/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "usbd_core.h"

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/

/* Exported variables --------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
USBD_StatusTypeDef USBD_Init(USBD_HandleTypeDef *pdev,
                             USBD_DescriptorsTypeDef *pdesc, uint8_t id)
{
  USBD_StatusTypeDef ret;

  // Check whether the USB Host handle is valid
  if (pdev == NULL) {
    return USBD_FAIL;
  }

  // Unlink previous class resources
  pdev->pClass = NULL;
  pdev->pUserData = NULL;
  pdev->pConfDesc = NULL;

  // Assign USBD Descriptors
  if (pdesc != NULL) {
    pdev->pDesc = pdesc;
  }

  // Set Device initial State
  pdev->dev_state = USBD_STATE_DEFAULT;
  pdev->id = id;

  // Initialize low level driver
  ret = USBD_LL_Init(pdev);

  return ret;
}

USBD_StatusTypeDef USBD_DeInit(USBD_HandleTypeDef *pdev)
{
  USBD_StatusTypeDef ret;

  // Disconnect the USB Device
  (void)USBD_LL_Stop(pdev);

  // Set Default State
  pdev->dev_state = USBD_STATE_DEFAULT;

  // Free Class Resources
  if (pdev->pClass != NULL) {
    ((USB_DeInit_t)PIC(pdev->pClass->DeInit))(pdev, (uint8_t)pdev->dev_config);
    pdev->pClass = NULL;
    pdev->pUserData = NULL;
  }

  // Free Device descriptors resources
  pdev->pDesc = NULL;
  pdev->pConfDesc = NULL;

  // DeInitialize low level driver
  ret = USBD_LL_DeInit(pdev);

  return ret;
}

USBD_StatusTypeDef USBD_RegisterClass(USBD_HandleTypeDef *pdev,
                                      USBD_ClassTypeDef *pclass)
{
  uint16_t len = 0U;

  // Check whether the class handle is valid
  if (pclass == NULL) {
    return USBD_FAIL;
  }
  // link the class to the USB Device handle
  pdev->pClass = pclass;

  // Get Device Configuration Descriptor
  if (pdev->pClass->GetFSConfigDescriptor != NULL) {
    pdev->pConfDesc = ((USB_GetFSConfigDescriptor_t)PIC(
        pdev->pClass->GetFSConfigDescriptor))(&len);
  }

  return USBD_OK;
}

USBD_StatusTypeDef USBD_Start(USBD_HandleTypeDef *pdev)
{
  // Start the low level driver
  return USBD_LL_Start(pdev);
}

USBD_StatusTypeDef USBD_Stop(USBD_HandleTypeDef *pdev)
{
  // Disconnect USB Device
  (void)USBD_LL_Stop(pdev);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_RunTestMode(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_SetClassConfig(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_StatusTypeDef ret = USBD_FAIL;

  if (pdev->pClass != NULL) {
    // Set configuration and Start the Class
    ret =
        (USBD_StatusTypeDef)((USB_Init_t)PIC(pdev->pClass->Init))(pdev, cfgidx);
  }

  return ret;
}

USBD_StatusTypeDef USBD_ClrClassConfig(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  if (pdev->pClass != NULL) {
    // Clear configuration and De-initialize the Class process
    ((USB_DeInit_t)PIC(pdev->pClass->DeInit))(pdev, cfgidx);
  }

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_SetupStage(USBD_HandleTypeDef *pdev, uint8_t *psetup)
{
  USBD_StatusTypeDef ret = USBD_OK;

  USBD_ParseSetupRequest(&pdev->request, psetup);

  pdev->ep0_state = USBD_EP0_SETUP;
  pdev->ep0_data_len = pdev->request.wLength;

  switch (pdev->request.bmRequest & 0x1FU) {
  case USB_REQ_RECIPIENT_DEVICE:
    ret = USBD_StdDevReq(pdev, &pdev->request);
    break;

  case USB_REQ_RECIPIENT_INTERFACE:
    ret = USBD_StdItfReq(pdev, &pdev->request);
    break;

  case USB_REQ_RECIPIENT_ENDPOINT:
    ret = USBD_StdEPReq(pdev, &pdev->request);
    break;

  default:
    ret = USBD_LL_StallEP(pdev, (pdev->request.bmRequest & 0x80U));
    break;
  }

  return ret;
}

USBD_StatusTypeDef USBD_LL_DataOutStage(USBD_HandleTypeDef *pdev, uint8_t epnum,
                                        uint8_t *pdata)
{
  USBD_EndpointTypeDef *pep = NULL;
  USBD_StatusTypeDef ret = USBD_OK;

  if (epnum == 0U) {
    pep = &pdev->ep_out[0];

    if (pdev->ep0_state == USBD_EP0_DATA_OUT) {
      if (pep->rem_length > pep->maxpacket) {
        pep->rem_length -= pep->maxpacket;
        (void)USBD_CtlContinueRx(pdev, pdata,
                                 MIN(pep->rem_length, pep->maxpacket));
      } else {
        if (pdev->dev_state == USBD_STATE_CONFIGURED) {
          if (pdev->pClass->EP0_RxReady != NULL) {
            ((USB_EP0_RxReady_t)PIC(pdev->pClass->EP0_RxReady))(pdev);
          }
        }
        (void)USBD_CtlSendStatus(pdev);
      }
    }
  } else {
    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
      if (pdev->pClass->DataOut != NULL) {
        ret = (USBD_StatusTypeDef)((USB_DataOut_t)PIC(pdev->pClass->DataOut))(
            pdev, epnum);
      }
    }
  }

  return ret;
}

USBD_StatusTypeDef USBD_LL_DataInStage(USBD_HandleTypeDef *pdev, uint8_t epnum,
                                       uint8_t *pdata)
{
  USBD_EndpointTypeDef *pep = NULL;
  USBD_StatusTypeDef ret = USBD_OK;
  UNUSED(pdata);

  if (epnum == 0U) {
    pep = &pdev->ep_in[0];

    if (pdev->ep0_state == USBD_EP0_DATA_IN) {
      if (pep->rem_length > pep->maxpacket) {
        pep->rem_length -= pep->maxpacket;
        pdev->pData = &((uint8_t *)pdev->pData)[pep->maxpacket];
        (void)USBD_CtlContinueSendData(pdev, pdev->pData, pep->rem_length);
        // Prepare endpoint for premature end of transfer
        (void)USBD_LL_PrepareReceive(pdev, 0U, NULL, 0U);
      } else {
        // last packet is MPS multiple, so send ZLP packet
        if ((pep->maxpacket == pep->rem_length) &&
            (pep->total_length >= pep->maxpacket) &&
            (pep->total_length < pdev->ep0_data_len)) {
          (void)USBD_CtlContinueSendData(pdev, NULL, 0U);
          pdev->ep0_data_len = 0U;
          // Prepare endpoint for premature end of transfer
          (void)USBD_LL_PrepareReceive(pdev, 0U, NULL, 0U);
        } else {
          if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            if (pdev->pClass->EP0_TxSent != NULL) {
              ((USB_EP0_TxSent_t)PIC(pdev->pClass->EP0_TxSent))(pdev);
            }
          }
          //(void)USBD_LL_StallEP(pdev, 0x80U);
          (void)USBD_CtlReceiveStatus(pdev);
        }
      }
    }
  } else {
    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
      if (pdev->pClass->DataIn != NULL) {
        ret = (USBD_StatusTypeDef)((USB_DataIn_t)PIC(pdev->pClass->DataIn))(
            pdev, epnum);
      }
    }
  }

  return ret;
}

USBD_StatusTypeDef USBD_LL_Reset(USBD_HandleTypeDef *pdev)
{
  // Upon Reset call user call back
  pdev->dev_state = USBD_STATE_DEFAULT;
  pdev->ep0_state = USBD_EP0_IDLE;
  pdev->dev_config = 0U;
  pdev->dev_remote_wakeup = 0U;

  if (pdev->pClassData != NULL) {
    if ((pdev->pClass != NULL) && (pdev->pClass->DeInit != NULL)) {
      (void)((USB_DeInit_t)PIC(pdev->pClass->DeInit))(
          pdev, (uint8_t)pdev->dev_config);
    }
  }

  // Open EP0 OUT
  (void)USBD_LL_OpenEP(pdev, 0x00U, USBD_EP_TYPE_CTRL, USB_MAX_EP0_SIZE);
  pdev->ep_out[0x00U & 0xFU].is_used = 1U;
  pdev->ep_out[0].maxpacket = USB_MAX_EP0_SIZE;

  // Open EP0 IN
  (void)USBD_LL_OpenEP(pdev, 0x80U, USBD_EP_TYPE_CTRL, USB_MAX_EP0_SIZE);
  pdev->ep_in[0x80U & 0xFU].is_used = 1U;
  pdev->ep_in[0].maxpacket = USB_MAX_EP0_SIZE;

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_SetSpeed(USBD_HandleTypeDef *pdev,
                                    USBD_SpeedTypeDef speed)
{
  pdev->dev_speed = speed;

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Suspend(USBD_HandleTypeDef *pdev)
{
  pdev->dev_old_state = pdev->dev_state;
  pdev->dev_state = USBD_STATE_SUSPENDED;

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_Resume(USBD_HandleTypeDef *pdev)
{
  if (pdev->dev_state == USBD_STATE_SUSPENDED) {
    pdev->dev_state = pdev->dev_old_state;
  }

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_SOF(USBD_HandleTypeDef *pdev)
{
  if (pdev->pClass == NULL) {
    return USBD_FAIL;
  }

  if (pdev->dev_state == USBD_STATE_CONFIGURED) {
    if (pdev->pClass->SOF != NULL) {
      (void)((USB_SOF_t)PIC(pdev->pClass->SOF))(pdev);
    }
  }

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_IsoINIncomplete(USBD_HandleTypeDef *pdev,
                                           uint8_t epnum)
{
  if (pdev->pClass == NULL) {
    return USBD_FAIL;
  }

  if (pdev->dev_state == USBD_STATE_CONFIGURED) {
    if (pdev->pClass->IsoINIncomplete != NULL) {
      (void)((USB_IsoINIncomplete)PIC(pdev->pClass->IsoINIncomplete))(pdev,
                                                                      epnum);
    }
  }

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_IsoOUTIncomplete(USBD_HandleTypeDef *pdev,
                                            uint8_t epnum)
{
  if (pdev->pClass == NULL) {
    return USBD_FAIL;
  }

  if (pdev->dev_state == USBD_STATE_CONFIGURED) {
    if (pdev->pClass->IsoOUTIncomplete != NULL) {
      (void)((USB_IsoOUTIncomplete)PIC(pdev->pClass->IsoOUTIncomplete))(pdev,
                                                                        epnum);
    }
  }

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DevConnected(USBD_HandleTypeDef *pdev)
{
  UNUSED(pdev);

  return USBD_OK;
}

USBD_StatusTypeDef USBD_LL_DevDisconnected(USBD_HandleTypeDef *pdev)
{
  // Free Class Resources
  pdev->dev_state = USBD_STATE_DEFAULT;

  if (pdev->pClass != NULL) {
    (void)((USB_DeInit_t)PIC(pdev->pClass->DeInit))(pdev,
                                                    (uint8_t)pdev->dev_config);
  }

  return USBD_OK;
}
