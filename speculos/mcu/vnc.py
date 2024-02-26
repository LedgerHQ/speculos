'''
Forward framebuffer to a VNC server (vnc/vnc_server) and receive mouse events.

This VNC server can be tested with Remmina for instance (with color depth set to
true color: 24 bbp).
'''

import os
import logging
import subprocess
import sys
from typing import IO, Optional, Tuple

from .display import DisplayNotifier, IODevice, PixelColorMapping


class VNC(IODevice):
    def __init__(self,
                 port: int,
                 screen_size: Tuple[int, int],
                 password: Optional[str] = None,
                 verbose: bool = False):
        self.logger = logging.getLogger("vnc")

        self._width, self._height = screen_size
        path = os.path.dirname(os.path.realpath(__file__))
        server = os.path.join(path, '../resources/vnc_server')
        cmd = [server]

        # custom options
        cmd += ['-s', f'{self._width}x{self._height}']
        if verbose:
            cmd += ['-v']

        # libvncserver options
        cmd += [
            '--',
            '-rfbport', f'{port}',
            '-rfbportv6', f'{port}',
        ]
        if password is not None:
            cmd += ['-passwd', password]

        self.subprocess = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)

    @property
    def file(self) -> IO[bytes]:
        assert self.subprocess.stdout is not None
        return self.subprocess.stdout

    def redraw(self, pixels: PixelColorMapping, default_color: int) -> None:
        '''The framebuffer was updated, forward everything to the VNC server.'''

        # int.to_bytes() is super slow, hence the manual encoding
        buf = bytearray(len(pixels) * 9)
        i = 0
        for x in range(0, self._width):
            for y in range(0, self._height):
                color = pixels.get((x, y), default_color)
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

        assert self.subprocess.stdin is not None
        self.subprocess.stdin.write(buf)
        self.subprocess.stdin.flush()

    def can_read(self, screen: DisplayNotifier) -> None:
        '''Process a new keyboard or mouse event message from the VNC server.'''

        data = b''
        while len(data) != 6:
            tmp = self.file.read(6 - len(data))
            if not tmp:
                self.logger.info("connection with vnc stdout closed")
                sys.exit(0)
            data += tmp

        if data[4] in [0x00, 0x01]:
            # mouse
            x = int.from_bytes(data[0:2], 'little')
            y = int.from_bytes(data[2:4], 'little')
            pressed = (data[4] != 0x00)
            screen.display.seph.handle_finger(x, y, pressed)
        elif data[4] in [0x10, 0x11]:
            # keyboard
            button = data[0]
            pressed = (data[4] == 0x11)
            screen.display.seph.handle_button(button, pressed)
        else:
            self.logger.error("invalid message from the VNC server")
