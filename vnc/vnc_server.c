/*
 * Bridge between speculos and a VNC server.
 *
 * I try hard to embed a VNC server inside speculos but it's especially
 * difficult because:
 *
 * - There is no Python project
 * - The only decent VNC server library (libvncserver) is written in C and there
 *   is no Python bindings.
 * - libvncserver relies on threads and that doesn't work well with Python (with
 *   ctypes or Python bindings).
 *
 * Hence this quick and dirty bridge.
 */

#include <err.h>
#include <errno.h>
#include <rfb/rfbclient.h>
#include <rfb/rfb.h>
#include <signal.h>
#include <stdbool.h>
#include <termios.h>

#include "cursor.h"

#define DEFAULT_PORT	  5900
#define DEFAULT_WIDTH	  320
#define DEFAULT_HEIGHT	  480

static unsigned int width = DEFAULT_WIDTH;
static unsigned int height = DEFAULT_HEIGHT;

static char *framebuffer;
static bool prev_pressed;
static int x_min, y_min, x_max, y_max;

rfbScreenInfoPtr screen;

struct mouse_event {
    unsigned short x;
    unsigned short y;
    char pressed;
    char eol;
} __attribute__((packed));

struct draw_event {
    unsigned short x;
    unsigned short y;
    int color;
    char eol;
} __attribute__((packed));

static ssize_t readall(int fd, void *buf, size_t count)
{
	unsigned char *p;
	ssize_t n;
	size_t i;

	p = buf;
	i = count;
	while (i > 0) {
		n = read(fd, p, i);
		if (n == 0) {
            errx(1, "readall: connection closed");
		} else if (n == -1) {
			if (errno == EINTR)
				continue;
            err(1, "readall");
		}
		i -= n;
		p += n;
	}

	return count;
}

static void reset_event(void)
{
    x_min = 0x7fffffff;
    y_min = 0x7fffffff;
    x_max = -1;
    y_max = -1;
}

static void update_event(int x, int y)
{
    if (x < x_min)
        x_min = x;
    if (y < y_min)
        y_min = y;

    if (x > x_max)
        x_max = x;
    if (y > y_max)
        y_max = y;
}

/* A mouse event was received by the VNC server, forward it to speculos. */
static void ptrAddEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
    struct mouse_event event;
    bool pressed;

    pressed = (buttonMask & 1);

    if ((prev_pressed && !pressed) || (!prev_pressed && pressed)) {
        event.x = x;
        event.y = y;
        event.pressed = pressed;
        event.eol = '\n';

        if (write(STDOUT_FILENO, &event, sizeof(event)) != sizeof(event)) {
            err(1, "write");
        }
        fflush(stdout);
    }

    prev_pressed = pressed;
}

/*
 * A keyboard event was received by the VNC server, forward it to speculos.
 *
 * The mouse event structure is reuse, it should actually be an union.
 */
static void kbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientPtr cl)
{
    struct mouse_event event;

#ifdef CUSTOM_CURSOR
    handle_konami_code(down, keySym);
#endif

    if (keySym != XK_Left && keySym != XK_Right) {
        return;
    }

    memset(&event, 0, sizeof(event));

    event.x = (keySym == XK_Left) ? 1 : 2;
    event.pressed = down ? 0x11 : 0x10;
    event.eol = '\n';

    if (write(STDOUT_FILENO, &event, sizeof(event)) != sizeof(event)) {
        err(1, "write");
    }
    fflush(stdout);
}

/* Speculos updated its framebuffer, update the local one. */
static void draw_point(int x, int y, int color)
{
    framebuffer[(y + width * x) * 4 + 0] = (color >> 16) & 0xff;
    framebuffer[(y + width * x) * 4 + 1] = (color >> 8) & 0xff;
    framebuffer[(y + width * x) * 4 + 2] = (color >> 0) & 0xff;
}

static void usage(char *argv0)
{
    fprintf(stderr, "Usage: %s [-s <size] [-p <port>] [-v]\n\n"
            "-p <port>: tcp port (default: %d)\n"
            "-s <size>: screen size (default: %dx%d)\n"
            "-v:        verbose (display libvncserver logs, default: false)\n",
            argv0,
            DEFAULT_PORT,
            DEFAULT_WIDTH, DEFAULT_HEIGHT);
    exit(EXIT_FAILURE);
}

static int parse_size(char *s, unsigned int *width, unsigned int *height)
{
    return sscanf(s, "%ux%u", width, height) != 2;
}

int main(int argc, char **argv)
{
    struct draw_event event;
    struct timeval timeout;
    int nfds, opt, port;
    sigset_t mask;
    bool verbose;
    fd_set fds;

    verbose = false;
    port = DEFAULT_PORT;

    while ((opt = getopt(argc, argv, "s:p:v")) != -1) {
        switch (opt) {
        case 's':
            if (parse_size(optarg, &width, &height) != 0) {
                fprintf(stderr, "invalid size (%s)\n", optarg);
                return EXIT_FAILURE;
            }
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'v':
            verbose = true;
            break;
        default:
            usage(argv[0]);
            break;
        }
    }

    rfbLogEnable(verbose);

    framebuffer = malloc(width * height * bytes_per_pixel);
    if (framebuffer == NULL) {
        err(1, "malloc");
    }
    /* initialize the framebuffer to white */
    memset(framebuffer, 0xff, width * height * bytes_per_pixel);

    screen = rfbGetScreen(&argc, argv, width, height, bits_per_sample, samples_per_pixel, bytes_per_pixel);

    screen->kbdAddEvent = kbdAddEvent;
    screen->ptrAddEvent = ptrAddEvent;
    screen->port = port;
    screen->ipv6port = port;

#ifdef CUSTOM_CURSOR
    {
        char *cursor_name = getenv("VNC_CURSOR");
        if (cursor_name != NULL) {
            set_cursor(screen, cursor_name);
        }
    }
#endif

    rfbNewFramebuffer(screen, framebuffer, width, height, bits_per_sample, samples_per_pixel, bytes_per_pixel);
    rfbInitServer(screen);

    if ((sigemptyset(&mask) == -1) || (sigaddset(&mask, SIGALRM) == -1)) {
        warn("sigemptyset");
    }

    while (1) {
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000;
        memcpy(&fds, &screen->allFds, sizeof(fds));
        FD_SET(STDIN_FILENO, &fds);

        /* wait for event on stdin or on the libvncserver sockets */
        nfds = select(screen->maxFd + 1, &fds, NULL, NULL, &timeout);
        if (nfds == -1) {
            if (errno != EINTR)
                continue;
        }

        if (FD_ISSET(STDIN_FILENO, &fds)) {
            FD_ZERO(&fds);
            FD_SET(STDIN_FILENO, &fds);

            reset_event();

            /* receive every available messages from speculos */
            timeout.tv_sec = 0;
            timeout.tv_usec = 0;
            while (select(STDIN_FILENO + 1, &fds, NULL, NULL, &timeout) == 1) {
                if (readall(STDIN_FILENO, &event, sizeof(event)) != sizeof(event)) {
                    err(1, "read");
                }
                draw_point(event.x, event.y, event.color);
                update_event(event.x, event.y);
            }

            /* tell the VNC server to refresh this part of the screen */
            if (x_min != 0x7fffffff && y_min != 0x7fffffff) {
                rfbMarkRectAsModified(screen, y_min, x_min, y_max+1, x_max+1);
            }
        }

        if (!rfbIsActive(screen)) {
            fprintf(stderr, "screen is not active anymore\n");
            break;
        }

        /* block signals because a call to select() in rfbProcessEvents()
         * doesn't handle EINTR */
        if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
            warn("sigprocmask(SIG_BLOCK)");
        }

        /* process libvncserver events */
        rfbProcessEvents(screen, screen->deferUpdateTime * 1000);

        /* unblock signals */
        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
            warn("sigprocmask(SIG_UNBLOCK)");
        }
    }

    return EXIT_SUCCESS;
}
