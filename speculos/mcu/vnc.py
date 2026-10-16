"""
Forward framebuffer to a VNC server (vnc/vnc_server) and receive mouse events.

This VNC server can be tested with Remmina for instance (with color depth set to
true color: 24 bbp).
"""

import logging
import struct
import subprocess
import sys
from array import array
from pathlib import Path
from typing import IO

from .display import DisplayNotifier, IODevice, PixelColorMapping

# y, x, color, end of line: the layout vnc_server reads.
_DRAW_EVENT = struct.Struct("<HHIB")
# Colors are 24 bits, so this never matches one: the first refresh is sent whole.
_UNSENT = 0xFFFFFFFF


class VNC(IODevice):
    def __init__(
        self,
        port: int,
        screen_size: tuple[int, int],
        password: str | None = None,
        verbose: bool = False,
    ):
        self.logger = logging.getLogger("vnc")

        if not 1 <= port <= 65535:
            raise ValueError(f"Invalid port number: {port}")
        if password is not None and any(c in password for c in ("\x00", "\n", "\r")):
            raise ValueError("VNC password contains invalid characters")

        self._width, self._height = screen_size
        if not (isinstance(self._width, int) and isinstance(self._height, int) and self._width > 0 and self._height > 0):
            raise ValueError(f"Invalid screen size: {screen_size}")

        server = Path(__file__).parent.parent / "resources" / "vnc_server"
        if not server.is_file():
            raise FileNotFoundError(f"VNC server binary not found: {server}")
        cmd = [str(server)]

        # custom options
        cmd += ["-s", f"{self._width}x{self._height}"]
        if verbose:
            cmd += ["-v"]

        # libvncserver options
        cmd += [
            "--",
            "-rfbport",
            str(port),
            "-rfbportv6",
            str(port),
        ]
        if password is not None:
            cmd += ["-passwd", password]

        # What the VNC server holds, so only changed pixels are sent to it.
        self._sent = array("I", [_UNSENT]) * (self._width * self._height)

        self.subprocess = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE)  # noqa: S603

    @property
    def file(self) -> IO[bytes]:
        if self.subprocess.stdout is None:
            raise ValueError("VNC subprocess stdout is not available")
        return self.subprocess.stdout

    def redraw(self, pixels: PixelColorMapping, default_color: int, clear: bool) -> None:
        """Forward the pixels drawn since the last refresh to the VNC server.

        The rest of the screen keeps what was last sent, unless it was cleared to default_color.
        """

        width, height = self._width, self._height
        sent = self._sent
        pack = _DRAW_EVENT.pack
        events = []

        if clear or sent[0] == _UNSENT:
            frame = array("I", [default_color]) * (width * height)
            for (x, y), color in pixels.items():
                frame[y * width + x] = color
            self._sent = frame
            for y in range(height):
                start = y * width
                new_row = frame[start : start + width]
                old_row = sent[start : start + width]
                # Comparing whole rows runs in C; most rows of a refresh are unchanged.
                if new_row == old_row:
                    continue
                for x in range(width):
                    color = new_row[x]
                    if color != old_row[x]:
                        events.append(pack(y, x, color, 0x0A))
        else:
            for (x, y), color in pixels.items():
                i = y * width + x
                if sent[i] != color:
                    sent[i] = color
                    events.append(pack(y, x, color, 0x0A))

        if not events:
            return
        if self.subprocess.stdin is None:
            raise ValueError("VNC subprocess stdin is not available")
        self.subprocess.stdin.write(b"".join(events))
        self.subprocess.stdin.flush()

    def can_read(self, screen: DisplayNotifier) -> None:
        """Process a new keyboard or mouse event message from the VNC server."""

        data = b""
        while len(data) != 6:
            tmp = self.file.read(6 - len(data))
            if not tmp:
                self.logger.info("connection with vnc stdout closed")
                sys.exit(0)
            data += tmp

        if data[4] in [0x00, 0x01]:
            # mouse
            x = int.from_bytes(data[0:2], "little")
            y = int.from_bytes(data[2:4], "little")
            pressed = data[4] != 0x00
            screen.display.seph.handle_finger(x, y, pressed)
        elif data[4] in [0x10, 0x11]:
            # keyboard
            button = data[0]
            pressed = data[4] == 0x11
            screen.display.seph.handle_button(button, pressed)
        else:
            self.logger.error("invalid message from the VNC server")
