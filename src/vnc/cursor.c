/*
 * Handle custom cursors and a konami code.
 *
 * This feature is highly experimental and consumes additional bandwidth, use it
 * at your own risk.
 */

#ifdef CUSTOM_CURSOR

#include <err.h>
#include <rfb/rfb.h>
#include <rfb/rfbclient.h>
#include <signal.h>
#include <stdbool.h>

#include "cursor.h"

#include "cursors/approved.h"
#include "cursors/bitcoin.h"
#include "cursors/blue.h"
#include "cursors/fabrice.h"
#include "cursors/pizza.h"
#include "cursors/star.h"
#include "cursors/sword.h"
#include "cursors/verynice.h"

#include "cursors/frame_00.h"
#include "cursors/frame_01.h"
#include "cursors/frame_02.h"
#include "cursors/frame_03.h"
#include "cursors/frame_04.h"
#include "cursors/frame_05.h"
#include "cursors/frame_06.h"
#include "cursors/frame_07.h"
#include "cursors/frame_08.h"
#include "cursors/frame_09.h"
#include "cursors/frame_10.h"
#include "cursors/frame_11.h"
#include "cursors/frame_12.h"
#include "cursors/frame_13.h"
#include "cursors/frame_14.h"
#include "cursors/frame_15.h"
#include "cursors/frame_16.h"
#include "cursors/frame_17.h"
#include "cursors/frame_18.h"
#include "cursors/frame_19.h"
#include "cursors/frame_20.h"
#include "cursors/frame_21.h"

#define CURSOR_WIDTH  32
#define CURSOR_HEIGHT 32

#define ANIMATED_CURSOR_INTERVAL 150

#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(*(a))))

static int animate_step;
static rfbCursorPtr current_cursor;
static bool konami_code_enabled = false;

static unsigned int konami_index;
static rfbKeySym konami_code[] = {
  XK_Up, XK_Up, XK_Down, XK_Down, XK_Left, XK_Right, XK_Left, XK_Right,
  // XK_B,
  // XK_A,
};

static char *cursor_list[] = { "approved", "bitcoin",  "blue",
                               "fabrice",  "pizza",    "star",
                               "sword",    "verynice", "flower" };

static char *flower_bitmaps[] = {
  cursor_frame_00, cursor_frame_01, cursor_frame_02, cursor_frame_03,
  cursor_frame_04, cursor_frame_05, cursor_frame_06, cursor_frame_07,
  cursor_frame_08, cursor_frame_09, cursor_frame_10, cursor_frame_11,
  cursor_frame_12, cursor_frame_13, cursor_frame_14, cursor_frame_15,
  cursor_frame_16, cursor_frame_17, cursor_frame_18, cursor_frame_19,
  cursor_frame_20, cursor_frame_21,
};

static char *flower_colors[] = {
  color_frame_00, color_frame_01, color_frame_02, color_frame_03,
  color_frame_04, color_frame_05, color_frame_06, color_frame_07,
  color_frame_08, color_frame_09, color_frame_10, color_frame_11,
  color_frame_12, color_frame_13, color_frame_14, color_frame_15,
  color_frame_16, color_frame_17, color_frame_18, color_frame_19,
  color_frame_20, color_frame_21,
};

static rfbCursorPtr MakeRichCursor(rfbScreenInfoPtr rfbScreen,
                                   char *cursor_bitmap, char *cursor_color)
{
  rfbCursorPtr c;

  if (rfbScreen->cursor->cleanupSource) {
    // rfbFreeCursor(rfbScreen->cursor);
  }

  rfbScreen->cursor =
      rfbMakeXCursor(CURSOR_WIDTH, CURSOR_HEIGHT, cursor_bitmap, cursor_bitmap);
  c = rfbScreen->cursor;
  c->xhot = 16;
  c->yhot = 24;
  c->cleanupSource = TRUE;

  if (c->richSource == NULL) {
    c->richSource = malloc(CURSOR_WIDTH * CURSOR_HEIGHT * bytes_per_pixel);
    if (c->richSource == NULL) {
      err(1, "malloc");
    }
  }
  c->cleanupRichSource = TRUE;
  memcpy(c->richSource, cursor_color,
         CURSOR_WIDTH * CURSOR_HEIGHT * bytes_per_pixel);

  current_cursor = c;

  return c;
}

static rfbCursorPtr getCursorPtr(rfbClientPtr cl)
{
  return current_cursor;
}

static void animate_cursor(int dummy)
{
  animate_step = (animate_step + 2) % 22;

  MakeRichCursor(screen, flower_bitmaps[animate_step],
                 flower_colors[animate_step]);

  rfbSetCursor(screen, current_cursor);
}

static void setup_animated_cursor(rfbScreenInfoPtr screen)
{
  struct itimerval it_val;

  if (signal(SIGALRM, animate_cursor) == SIG_ERR) {
    warn("signal(SIGALRM)");
    return;
  }

  it_val.it_value.tv_sec = ANIMATED_CURSOR_INTERVAL / 1000;
  it_val.it_value.tv_usec = (ANIMATED_CURSOR_INTERVAL * 1000) % 1000000;
  it_val.it_interval = it_val.it_value;
  if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
    warn("setitimer()");
    return;
  }

  screen->getCursorPtr = getCursorPtr;
}

static void disable_animated_cursor(rfbScreenInfoPtr screen)
{
  if (signal(SIGALRM, SIG_DFL) == SIG_ERR) {
    warn("signal(SIGALRM)");
    return;
  }

  if (setitimer(ITIMER_REAL, NULL, NULL) == -1) {
    warn("setitimer()");
    return;
  }
}

#define set_cursor_helper(x)                                                   \
  do {                                                                         \
    if (strcmp(p, #x) == 0) {                                                  \
      MakeRichCursor(screen, cursor_##x, color_##x);                           \
      rfbSetCursor(screen, current_cursor);                                    \
      screen->getCursorPtr = getCursorPtr;                                     \
      return;                                                                  \
    }                                                                          \
  } while (0)

void set_cursor(rfbScreenInfoPtr screen, char *p)
{
  set_cursor_helper(approved);
  set_cursor_helper(bitcoin);
  set_cursor_helper(blue);
  set_cursor_helper(fabrice);
  set_cursor_helper(pizza);
  set_cursor_helper(star);
  set_cursor_helper(sword);
  set_cursor_helper(verynice);

  if (strcmp(p, "flower") == 0) {
    setup_animated_cursor(screen);
  }
}

void set_cursor_from_keysym(rfbScreenInfoPtr screen, unsigned int index)
{
  if (index >= ARRAY_SIZE(cursor_list)) {
    return;
  }

  set_cursor(screen, cursor_list[index]);
}

void handle_konami_code(rfbBool down, rfbKeySym keySym)
{
  if (!konami_code_enabled) {
    if (keySym == konami_code[konami_index] && down) {
      konami_index++;
      // fprintf(stderr, "konami = %d\n", konami_index);
      if (konami_index == sizeof(konami_code) / sizeof(konami_code[0])) {
        konami_code_enabled = true;
        setup_animated_cursor(screen);
      }
    } else if (down) {
      konami_index = 0;
      if (keySym == konami_code[0]) {
        konami_index++;
      }
    }
  } else {
    if (keySym >= XK_0 && keySym <= XK_9 && down) {
      disable_animated_cursor(screen);
      set_cursor_from_keysym(screen, keySym - XK_0);
    }
  }
}

#endif
