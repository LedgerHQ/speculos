/* @BANNER@ */

/* Includes ------------------------------------------------------------------*/
#include "os_io_seph_ux.h"
#include "os.h"
#include "os_io.h"
#include "os_io_seph_cmd.h"
#include "seproxyhal_protocol.h"
#include "ux.h"

#ifdef HAVE_IO_USB
#include "usbd_ledger.h"
#endif // HAVE_IO_USB

#ifdef HAVE_PRINTF
#define LOG_IO PRINTF
// #define LOG_IO(...)
#else // !HAVE_PRINTF
#define LOG_IO(...)
#endif // !HAVE_PRINTF

/* Private enumerations ------------------------------------------------------*/

/* Private types, structures, unions -----------------------------------------*/

/* Private defines------------------------------------------------------------*/

/* Private macros-------------------------------------------------------------*/

/* Private functions prototypes ----------------------------------------------*/
#ifdef HAVE_BAGL
static void
os_io_seph_ux_display_bagl_icon(const bagl_component_t *icon_component,
                                const bagl_icon_details_t *icon_det);
#endif // HAVE_BAGL

/* Exported variables --------------------------------------------------------*/
#if defined(HAVE_BAGL) || defined(HAVE_NBGL)
ux_seph_os_and_app_t G_ux_os;
#endif

/* Private variables ---------------------------------------------------------*/
#ifdef HAVE_SERIALIZED_NBGL
static uint8_t nbgl_serialize_buffer[SERIALIZED_NBGL_MAX_LEN];
#endif // HAVE_SERIALIZED_NBGL

/* Private functions ---------------------------------------------------------*/
#ifdef HAVE_BAGL
static void
os_io_seph_ux_display_bagl_icon(const bagl_component_t *icon_component,
                                const bagl_icon_details_t *icon_det)
{
  bagl_component_t icon_component_mod;
  const bagl_icon_details_t *icon_details =
      (const bagl_icon_details_t *)PIC(icon_det);

  if (icon_details && icon_details->bitmap) {
    // ensure not being out of bounds in the icon component against the declared
    // icon real size
    memcpy(&icon_component_mod, PIC(icon_component), sizeof(bagl_component_t));
    icon_component_mod.width = icon_details->width;
    icon_component_mod.height = icon_details->height;
    icon_component = &icon_component_mod;

    bagl_draw_glyph(&icon_component_mod, icon_details);

#if defined(HAVE_PRINTF)
    // testing color index size
    unsigned int h = (1 << (icon_details->bpp)) * sizeof(unsigned int);
    // bitmap size
    unsigned int w =
        ((icon_component->width * icon_component->height * icon_details->bpp) /
         8) +
        ((icon_component->width * icon_component->height * icon_details->bpp) %
                 8
             ? 1
             : 0);
    unsigned short length = sizeof(bagl_component_t) + 1 // bpp
                            + h                          // color index
                            + w;                         // image bitmap size
    if (length > (OS_IO_SEPH_BUFFER_SIZE - 4)) {
      PRINTF("ERROR: Inside os_io_seph_ux_display_bagl_icon, icon size (%d) is "
             "too big for the "
             "buffer (%d-4)! (bitmap=0x%x, width=%d, height=%d, bpp=%d)\n",
             length, OS_IO_SEPH_BUFFER_SIZE, icon_details->bitmap,
             icon_details->width, icon_details->height, icon_details->bpp);
      return;
    }

    uint8_t seph_buffer[3];
    seph_buffer[0] = SEPROXYHAL_TAG_DBG_SCREEN_DISPLAY_STATUS;
    seph_buffer[1] = length >> 8;
    seph_buffer[2] = length;
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_buffer, 3, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, (const uint8_t *)icon_component,
                 sizeof(bagl_component_t), NULL);
    seph_buffer[0] = icon_details->bpp;
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_buffer, 1, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH,
                 (const uint8_t *)PIC(icon_details->colors), h, NULL);
    os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH,
                 (const uint8_t *)PIC(icon_details->bitmap), w, NULL);
#else  // !HAVE_PRINTF
    (void)icon_component;
#endif // !HAVE_PRINTF
  }
}
#endif // HAVE_BAGL

/* Exported functions --------------------------------------------------------*/
void io_seph_ux_init_button(void)
{
  G_ux_os.button_mask = 0;
  G_ux_os.button_same_mask_counter = 0;
}

int io_process_itc_ux_event(uint8_t *buffer_in, size_t buffer_in_length)
{
  int status = buffer_in_length;

  switch (buffer_in[3]) {
#ifdef HAVE_BLE
  case ITC_UX_ASK_BLE_PAIRING:
    G_ux_params.ux_id = BOLOS_UX_ASYNCHMODAL_PAIRING_REQUEST;
    G_ux_params.len = sizeof(G_ux_params.u.pairing_request);
    memset(&G_ux_params.u.pairing_request, 0,
           sizeof(G_ux_params.u.pairing_request));
    G_ux_params.u.pairing_request.type = buffer_in[4];
    G_ux_params.u.pairing_request.pairing_info_len = U2BE(buffer_in, 1) - 2;
    memcpy(G_ux_params.u.pairing_request.pairing_info, &buffer_in[5],
           G_ux_params.u.pairing_request.pairing_info_len);
    os_ux(&G_ux_params);
    status = 0;
    break;

  case ITC_UX_BLE_PAIRING_STATUS:
    G_ux_params.ux_id = BOLOS_UX_ASYNCHMODAL_PAIRING_STATUS;
    G_ux_params.len = sizeof(G_ux_params.u.pairing_status);
    G_ux_params.u.pairing_status.pairing_ok = buffer_in[4];
    os_ux(&G_ux_params);
    status = 0;
    break;
#endif // HAVE_BLE

#if !defined(HAVE_BOLOS) && defined(HAVE_BAGL)
  case ITC_UX_REDISPLAY:
    ux_stack_redisplay();
    break;
#endif // HAVE_BOLOS && HAVE_BAGL
#if !defined(HAVE_BOLOS) && defined(HAVE_NBGL)
  case ITC_UX_REDISPLAY:
    nbgl_objAllowDrawing(true);
    nbgl_screenRedraw();
    nbgl_refresh();
    break;
#endif // HAVE_BOLOS && HAVE_NBGL

  default:
    break;
  }

  return status;
}

void io_process_ux_event(uint8_t *buffer_in, size_t buffer_in_length)
{
  UNUSED(buffer_in_length);

  if ((buffer_in[0] == SEPROXYHAL_TAG_TICKER_EVENT) ||
      (buffer_in[0] == SEPROXYHAL_TAG_BUTTON_PUSH_EVENT) ||
      (buffer_in[0] == SEPROXYHAL_TAG_STATUS_EVENT) ||
      (buffer_in[0] == SEPROXYHAL_TAG_FINGER_EVENT) ||
      (buffer_in[0] == SEPROXYHAL_TAG_POWER_BUTTON_EVENT)) {
    G_ux_params.ux_id = BOLOS_UX_EVENT;
    G_ux_params.len = 0;
    os_ux(&G_ux_params);
  } else if (buffer_in[0] == SEPROXYHAL_TAG_ITC_EVENT) {
    io_process_itc_ux_event(buffer_in, buffer_in_length);
  }
}

#if !defined(APP_UX)
unsigned int os_ux_blocking(bolos_ux_params_t *params)
{
  unsigned int ret;

  os_ux(params);
  ret = os_sched_last_status(TASK_BOLOS_UX);
  while (ret == BOLOS_UX_IGNORE || ret == BOLOS_UX_CONTINUE) {
    os_io_handle_ux_event_reject_apdu();
    // only retrieve the current UX state
    ret = os_sched_last_status(TASK_BOLOS_UX);
  }

  return ret;
}
#endif // !APP_UX

#ifdef HAVE_BAGL
void io_seph_ux_display_bagl_element(const bagl_element_t *element)
{
  const bagl_element_t *el = (const bagl_element_t *)PIC(element);
  const char *txt;
  // process automagically address from rom and from ram
  unsigned int type = (el->component.type & ~(BAGL_FLAG_TOUCHABLE));

#if defined(HAVE_INDEXED_STRINGS)
  if (type == BAGL_LABELINE_LOC) {
    txt = (const char *)PIC(get_ux_loc_string(el->index));
  } else
#endif // defined(HAVE_INDEXED_STRINGS)
  {
    txt = (const char *)PIC(el->text);
  }

  if (type != BAGL_NONE) {
    if (txt != NULL) {
      // consider an icon details descriptor is pointed by the context
      if (type == BAGL_ICON && el->component.icon_id == 0) {
        // SECURITY: due to this wild cast, the code MUST be executed on the
        // application side instead of in
        //           the syscall sides to avoid buffer overflows and a real hard
        //           way of checking buffer belonging in the syscall dispatch
        os_io_seph_ux_display_bagl_icon(&el->component,
                                        (const bagl_icon_details_t *)txt);
      } else {
        bagl_draw_with_context(&el->component, txt, strlen(txt),
                               BAGL_ENCODING_DEFAULT);
#if defined(HAVE_PRINTF)
        // LNS case (MCU screen) or SE screen device willing to send bagl packet
        // for automated testing io_seph_send crash when using txt from language
        // packs...
        // ...let's use an intermediate buffer to store txt
#ifdef HAVE_LANGUAGE_PACK
        char txt_buffer[128];
        strlcpy(txt_buffer, txt, sizeof(txt_buffer));
#else  // HAVE_LANGUAGE_PACK
        const char *txt_buffer = txt;
#endif // HAVE_LANGUAGE_PACK
        unsigned short length = sizeof(bagl_component_t) + strlen(txt_buffer);
        if (length > (OS_IO_SEPH_BUFFER_SIZE - 3)) {
          PRINTF("ERROR: Inside io_seph_ux_display_bagl_element, length (%d) "
                 "is too big "
                 "for seph buffer(%d)!\n",
                 length + 3, OS_IO_SEPH_BUFFER_SIZE);
          return;
        }
        uint8_t seph_buffer[3];
        seph_buffer[0] = SEPROXYHAL_TAG_DBG_SCREEN_DISPLAY_STATUS;
        seph_buffer[1] = length >> 8;
        seph_buffer[2] = length;
        os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_buffer, 3, NULL);
        os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, (const uint8_t *)&el->component,
                     sizeof(bagl_component_t), NULL);
        os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, (const uint8_t *)txt_buffer,
                     strlen(txt_buffer), NULL);
#endif // HAVE_PRINTF
      }
    } else {
      bagl_draw_with_context(&el->component, NULL, 0, 0);
#if defined(HAVE_PRINTF)
      // LNS case (MCU screen) or SE screen device willing to send bagl packet
      // for automated testing
      unsigned short length = sizeof(bagl_component_t);
      uint8_t seph_buffer[3];
      seph_buffer[0] = SEPROXYHAL_TAG_DBG_SCREEN_DISPLAY_STATUS;
      seph_buffer[1] = length >> 8;
      seph_buffer[2] = length;
      os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, seph_buffer, 3, NULL);
      os_io_tx_cmd(OS_IO_PACKET_TYPE_SEPH, (const uint8_t *)&el->component,
                   sizeof(bagl_component_t), NULL);
#endif // HAVE_PRINTF
    }
  }
}

void io_seproxyhal_button_push(button_push_callback_t button_callback,
                               unsigned int new_button_mask)
{
  if (button_callback) {
    unsigned int button_mask;
    unsigned int button_same_mask_counter;
    // enable speeded up long push
    if (new_button_mask == G_ux_os.button_mask) {
      // each 100ms ~
      G_ux_os.button_same_mask_counter++;
    }

    // when new_button_mask is 0 and

    // append the button mask
    button_mask = G_ux_os.button_mask | new_button_mask;

    // pre reset variable due to os_sched_exit
    button_same_mask_counter = G_ux_os.button_same_mask_counter;

    // reset button mask
    if (new_button_mask == 0) {
      // reset next state when button are released
      G_ux_os.button_mask = 0;
      G_ux_os.button_same_mask_counter = 0;

      // notify button released event
      button_mask |= BUTTON_EVT_RELEASED;
    } else {
      G_ux_os.button_mask = button_mask;
    }

    // reset counter when button mask changes
    if (new_button_mask != G_ux_os.button_mask) {
      G_ux_os.button_same_mask_counter = 0;
    }

    if (button_same_mask_counter >= BUTTON_FAST_THRESHOLD_CS) {
      // fast bit when pressing and timing is right
      if ((button_same_mask_counter % BUTTON_FAST_ACTION_CS) == 0) {
        button_mask |= BUTTON_EVT_FAST;
      }

      // discard the release event after a fastskip has been detected, to avoid
      // strange at release behavior and also to enable user to cancel an
      // operation by starting triggering the fast skip
      button_mask &= ~BUTTON_EVT_RELEASED;
    }

    // indicate if button have been released
    button_callback(button_mask, button_same_mask_counter);
  }
}
#endif // HAVE_BAGL

#ifdef HAVE_SERIALIZED_NBGL
void io_seph_ux_send_nbgl_serialized(nbgl_serialized_event_type_e event,
                                     nbgl_obj_t *obj)
{
  // Serialize object
  size_t len = 0;
  uint8_t status = nbgl_serializeNbglEvent(event, obj, nbgl_serialize_buffer,
                                           &len, sizeof(nbgl_serialize_buffer));

  // Encode and send
  if (status == NBGL_SERIALIZE_OK) {
    os_io_seph_cmd_serialized_nbgl(nbgl_serialize_buffer, len);
  }
}
#endif // HAVE_SERIALIZED_NBGL
