'''
TCP server to emulate finger actions on screen:

args are sent in csv format (x,y,pressed) and can be chained.

Usage example:

    $ echo -n 128,300,1 | nc -nv 127.0.0.1 1236
    $ echo -n 128,300,0,110,41,0 | nc -nv 127.0.0.1 1236
'''

import logging
import socket


class FakeFingerClient:
    def __init__(self, s):
        self.s = s
        self.logger = logging.getLogger("finger")

    def _close(self, screen):
        screen.remove_notifier(self.s.fileno())
        self.logger.debug("connection closed with fake button client")

    def can_read(self, s, screen):
        packet = self.s.recv(100)
        if packet == b'':
            self._close(screen)
            return

        actions = packet.decode("ascii").split(',')
        actions = [actions[i * 3:(i + 1) * 3] for i in range(len(actions) // 3)]

        for action in actions:
            x = int(action[0])
            y = int(action[1])
            pressed = int(action[2])
            self.logger.debug(f"touch event on ({x},{y}) coordinates, {'pressed' if pressed else 'release'}")
            screen.seph.handle_finger(x, y, pressed)


class FakeFinger:
    def __init__(self, port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind(('0.0.0.0', port))  # lgtm [py/bind-socket-all-network-interfaces]
        self.s.listen(5)
        self.logger = logging.getLogger("finger")

    def can_read(self, s, screen):
        c, addr = self.s.accept()
        self.logger.debug(f"new client from {addr}")
        client = FakeFingerClient(c)
        screen.add_notifier(client)
