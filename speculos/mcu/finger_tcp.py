'''
TCP server to emulate finger actions on screen:

args are sent in csv format (x,y,pressed) and can be chained.

Usage example:

    $ echo -n 128,300,1 | nc -nv 127.0.0.1 1236
    $ echo -n 128,300,0,110,41,0 | nc -nv 127.0.0.1 1236
'''

import logging
import socket
from typing import List

from .display import DisplayNotifier, IODevice


class FakeFingerClient(IODevice):
    def __init__(self, sock: socket.socket):
        self.socket = sock
        self.logger = logging.getLogger("finger")

    @property
    def file(self):
        return self.socket

    def _cleanup(self, screen: DisplayNotifier):
        screen.remove_notifier(self.socket.fileno())
        self.logger.debug("connection closed with fake button client")

    def can_read(self, screen: DisplayNotifier):
        packet = self.socket.recv(100)
        if packet == b'':
            self._cleanup(screen)
            return

        _actions: List[str] = packet.decode("ascii").split(',')
        actions: List[List[str]] = [_actions[i * 3:(i + 1) * 3] for i in range(len(_actions) // 3)]

        for action in actions:
            x = int(action[0])
            y = int(action[1])
            pressed = int(action[2])
            self.logger.debug(f"touch event on ({x},{y}) coordinates, {'pressed' if pressed else 'release'}")
            screen.display.seph.handle_finger(x, y, pressed)


class FakeFinger(IODevice):
    def __init__(self, port: int):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(('0.0.0.0', port))  # lgtm [py/bind-socket-all-network-interfaces]
        self.socket.listen(5)
        self.logger = logging.getLogger("finger")

    @property
    def file(self):
        return self.socket

    def can_read(self, screen: DisplayNotifier) -> None:
        c, addr = self.file.accept()
        self.logger.debug("New client from %s", addr)
        client = FakeFingerClient(c)
        screen.add_notifier(client)
