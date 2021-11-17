#!/usr/bin/python2

'''
VNC client with skin support.

Speculos must be launched with the --vnc-port argument in order to expose the
VNC server. Usage example:

  $ ./speculos.py ./apps/nanos#btc#1.5#00000000.elf --vnc-port 5902 --headless &
  $ ./tools/winamp.py --skin tools/skins/nano-blue.png

This script is highly experimental. Skins are PNG files where the first black
128x32 rectangle is replaced by the screen.

Requirements: python-gtk-vnc. Unfortunately, there seems to be no Python 3
version of gtkvnc.

This script is based on the example from
https://github.com/jwendell/gtk-vnc/blob/master/examples/gvncviewer.py
'''

import argparse
import gtk
import gtkvnc
import time
import cairo

from gtk import gdk
from PIL import Image

OFFSET_X, OFFSET_Y = 0, 0
SCREEN_WIDTH, SCREEN_HEIGHT = 128, 32


def button_press_event(box, event):
    global OFFSET_X, OFFSET_Y

    if event.button == 1:
        window = box.get_parent()
        x, y = window.get_position()
        OFFSET_X = event.x_root - x
        OFFSET_Y = event.y_root - y

        arrow = gdk.Cursor(gdk.FLEUR)
        root = window.get_root_window()
        root.set_cursor(arrow)


def button_release_event(box, event):
    if event.button == 1:
        arrow = gdk.Cursor(gdk.LEFT_PTR)
        root = window.get_root_window()
        root.set_cursor(arrow)


def motion_notify_event(box, event):
    global OFFSET_X, OFFSET_Y

    window = box.get_parent()
    x = int(event.x_root - OFFSET_X)
    y = int(event.y_root - OFFSET_Y)
    window.move(x, y)


def vnc_screenshot(src, ev, vnc):
    if ev.keyval == gtk.gdk.keyval_from_name("s"):
        filename = "/tmp/speculos-{}.png".format(int(time.time()))
        pix = vnc.get_pixbuf()
        pix.save(filename, "png")
        print("[*] screenshot saved to {}".format(filename))

    return False


def vnc_grab(src, window):
    pass


def vnc_ungrab(src, window):
    pass


def vnc_connected(src):
    print("Connected to server")


def vnc_initialized(src, window):
    print("Connection initialized")
    window.show_all()


def vnc_disconnected(src):
    print("Disconnected from server")
    gtk.main_quit()


def expose(widget, event):
    """
    https://stackoverflow.com/questions/4889045/how-to-get-transparent-background-in-window-with-pygtk-and-pycairo
    """

    window = widget.window.get_parent()
    cr = window.cairo_create()

    # Sets the operator to clear which deletes everything below where an object is drawn
    cr.set_operator(cairo.OPERATOR_CLEAR)
    cr.rectangle(0.0, 0.0, IMG_WIDTH, IMG_HEIGHT)
    cr.fill()

    cr.set_operator(cairo.OPERATOR_OVER)
    img = cairo.ImageSurface.create_from_png(IMG_PATH)
    cr.set_source_surface(img, 0, 0)
    cr.rectangle(0.0, 0.0, IMG_WIDTH, IMG_HEIGHT)
    cr.paint()
    cr.fill()


def is_black_rectangle(pixels, x, y, width, height):
    black = (0, 0, 0, 255)
    for i in range(0, SCREEN_WIDTH):
        for j in range(0, SCREEN_HEIGHT):
            if pixels[x+i, y+j] != black:
                return False
    return True


def find_screen_position(image):
    width, height = image.size
    pixels = image.load()
    for x in range(0, width - SCREEN_WIDTH):
        for y in range(0, height - SCREEN_HEIGHT):
            if is_black_rectangle(pixels, x, y, SCREEN_WIDTH, SCREEN_HEIGHT):
                return x, y

    # if not found, place the screen at the center of the image
    print("[-] can't found the screen position in the skin")
    x = (width - SCREEN_WIDTH) / 2
    y = (height - SCREEN_HEIGHT) / 2
    return x, y


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('-o', '--host', default='127.0.0.1', help='VNC host')
    parser.add_argument('-p', '--port', default='5902', help='VNC port')
    parser.add_argument('-s', '--skin', default=None, help='PNG image')
    args = parser.parse_args()

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    vnc = gtkvnc.Display()

    if args.skin:
        image = Image.open(args.skin)
        IMG_PATH = args.skin
        IMG_WIDTH, IMG_HEIGHT = image.size
        x, y = find_screen_position(image)
        image.close()

        window.set_default_size(IMG_WIDTH, IMG_HEIGHT)
        fix = gtk.Fixed()
        fix.put(vnc, x, y)
        box = gtk.EventBox()
        window.add(box)
        box.add(fix)
    else:
        window.add(vnc)
        box = window

    box.connect("button_press_event", button_press_event)
    box.connect("button_release_event", button_release_event)
    box.connect("motion_notify_event", motion_notify_event)
    window.connect("key-press-event", vnc_screenshot, vnc)

    # borderless
    window.set_decorated(False)

    # transparent background
    screen = window.get_screen()
    rgba = screen.get_rgba_colormap()
    window.set_colormap(rgba)

    vnc.realize()
    vnc.set_pointer_grab(False)
    vnc.set_keyboard_grab(True)

    # Example to change grab key combination to Ctrl+Alt+g
    grab_keys = [gtk.keysyms.Control_L, gtk.keysyms.Alt_L, gtk.keysyms.g]
    vnc.set_grab_keys(grab_keys)

    print("Connecting to %s %s" % (args.host, args.port))
    vnc.open_host(args.host, args.port)

    vnc.connect("vnc-pointer-grab", vnc_grab, window)
    vnc.connect("vnc-pointer-ungrab", vnc_ungrab, window)

    vnc.connect("vnc-connected", vnc_connected)
    vnc.connect("vnc-initialized", vnc_initialized, window)
    vnc.connect("vnc-disconnected", vnc_disconnected)

    if args.skin:
        vnc.connect("expose-event", expose)

    gtk.main()
