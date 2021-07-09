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


class FakeButtonClient:
    actions = {
        'L': (1, True),
        'l': (1, False),
        'R': (2, True),
        'r': (2, False),
    }

    def __init__(self, s):
        self.s = s
        self.logger = logging.getLogger("button")

    def _close(self, screen):
        screen.remove_notifier(self.s.fileno())
        self.logger.debug("connection closed with fake button client")

    def can_read(self, s, screen):
        packet = self.s.recv(1)
        if packet == b'':
            self._close(screen)
            return

        for c in packet:
            c = chr(c)
            if c in self.actions.keys():
                key, pressed = self.actions[c]
                self.logger.debug(f"button {key} release: {pressed}")
                screen.seph.handle_button(key, pressed)
                time.sleep(0.1)
            else:
                self.logger.debug(f"ignoring byte {c!r}")


class FakeButton:
    def __init__(self, port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind(('0.0.0.0', port))  # lgtm [py/bind-socket-all-network-interfaces]
        self.s.listen(5)
        self.logger = logging.getLogger("button")

    def can_read(self, s, screen):
        c, addr = self.s.accept()
        self.logger.debug(f"new client from {addr}")
        client = FakeButtonClient(c)
        screen.add_notifier(client)
