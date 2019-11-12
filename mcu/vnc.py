'''
Forward framebuffer to a VNC server (vnc/vnc_server) and receive mouse events.

This VNC server can be tested with Remmina for instance (with color depth set to
true color: 24 bbp).
'''

import os
import subprocess
import sys

from .display import MODELS

class VNC:
    def __init__(self, port, model):
        width, height = MODELS[model].screen_size
        path = os.path.dirname(os.path.realpath(__file__))
        server = os.path.join(path, '../build/vnc/vnc_server')
        cmd = [ server, '-p', str(port), '-s', f'{width}x{height}' ]
        self.p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

        # required by Screen.add_notifier
        self.s = self.p.stdout

    def redraw(self, pixels):
        '''The framebuffer was updated, forward everything to the VNC server.'''

        # int.to_bytes() is super slow, hence the manual encoding
        buf = bytearray(len(pixels) * 9)
        i = 0
        for (x, y), color in pixels.items():
            buf[i + 0] = y & 0xff
            buf[i + 1] = (y >> 8) & 0xff
            buf[i + 2] = x & 0xff
            buf[i + 3] = (x >> 8) & 0xff
            buf[i + 4] = color & 0xff
            buf[i + 5] = (color >> 8) & 0xff
            buf[i + 6] = (color >> 16) & 0xff
            buf[i + 7] = (color >> 24) & 0xff
            buf[i + 8] = 0x0a
            i += 9

        self.p.stdin.write(buf)
        self.p.stdin.flush()

    def can_read(self, s, screen):
        '''Process a new keyboard or mouse event message from the VNC server.'''

        assert s == self.s.fileno()
        assert s == self.p.stdout.fileno()

        data = b''
        while len(data) != 6:
            tmp = self.p.stdout.read(6 - len(data))
            if not tmp:
                print("connection with vnc stdout closed")
                sys.exit(0)
            data += tmp

        if data[4] in [ 0x00, 0x01 ]:
            # mouse
            x = int.from_bytes(data[0:2], 'little')
            y = int.from_bytes(data[2:4], 'little')
            pressed = (data[4] != 0x00)
            screen.seph.handle_finger(x, y, pressed)
        elif data[4] in [ 0x10, 0x11 ]:
            # keyboard
            button = data[0]
            pressed = (data[4] == 0x11)
            screen.seph.handle_button(button, pressed)
        else:
            print('invalid message from the VNC server')
