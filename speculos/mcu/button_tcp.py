'''
TCP server to emulate buttons:

- L/R: press left/right button
- l/r: release left/right button

Usage example:

    $ echo -n RrRrRr | nc -nv 127.0.0.1 1235
    $ echo -n LRlr | nc -nv 127.0.0.1 1235
'''

import logging
import socket
import time

from .display import DisplayNotifier, IODevice


class FakeButtonClient(IODevice):
    actions = {
        'L': (1, True),
        'l': (1, False),
        'R': (2, True),
        'r': (2, False),
    }

    def __init__(self, sock: socket.socket):
        self.socket = sock
        self.logger = logging.getLogger("button")

    @property
    def file(self):
        return self.socket

    def _cleanup(self, screen: DisplayNotifier):
        screen.remove_notifier(self.fileno)
        self.logger.debug("connection closed with fake button client")

    def can_read(self, screen: DisplayNotifier):
        packet = self.file.recv(1)
        if packet == b'':
            self._cleanup(screen)
            return

        for c in packet:
            c = chr(c)
            if c in self.actions.keys():
                key, pressed = self.actions[c]
                self.logger.debug(f"button {key} release: {pressed}")
                screen.display.seph.handle_button(key, pressed)
                time.sleep(0.1)
            else:
                self.logger.debug(f"ignoring byte {c!r}")


class FakeButton(IODevice):
    def __init__(self, port: int):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(('0.0.0.0', port))  # lgtm [py/bind-socket-all-network-interfaces]
        self.socket.listen(5)
        self.logger = logging.getLogger("button")

    @property
    def file(self):
        return self.socket

    def can_read(self, screen: DisplayNotifier):
        c, addr = self.file.accept()
        self.logger.debug("New client from %s", addr)
        client = FakeButtonClient(c)
        screen.add_notifier(client)
