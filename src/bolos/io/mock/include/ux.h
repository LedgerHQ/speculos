#pragma once

#include "emulate.h"
#include "os.h"

typedef enum bolos_ux_public_e {
  BOLOS_UX_INITIALIZE = 0,
  BOLOS_UX_EVENT,
  BOLOS_UX_KEYBOARD,
  BOLOS_UX_WAKE_UP,
  BOLOS_UX_STATUS_BAR,

  BOLOS_UX_VALIDATE_PIN,
  BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST, // ask the ux to display a modal to
                                        // accept/reject the current pairing
                                        // request
  BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS,
  BOLOS_UX_IO_RESET,
  BOLOS_UX_DELAY_LOCK, // delay the power-off/lock timer
  NB_BOLOS_UX_IDS
} bolos_ux_public_t;

typedef struct ux_seph_s {
  unsigned int button_mask;
  unsigned int button_same_mask_counter;
#ifdef HAVE_BOLOS
  unsigned int ux_id;
  unsigned int ux_status;
#endif // HAVE_BOLOS
} ux_seph_os_and_app_t;

extern ux_seph_os_and_app_t G_ux_os;
extern bolos_ux_params_t G_ux_params;

#define os_ux sys_os_ux_2_0