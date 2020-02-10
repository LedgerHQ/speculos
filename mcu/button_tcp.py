'''
TCP server to emulate buttons:

- L/R: press left/right button
- l/r: release left/right button

Usage example:

    $ echo -n RrRrRr | nc -nv 127.0.0.1 1235
    $ echo -n LRlr | nc -nv 127.0.0.1 1235
'''

import socket

class FakeButtonClient:
    actions = {
        'L': (1, False),
        'l': (1, True),
        'R': (2, False),
        'r': (2, True),
    }
    def __init__(self, s):
        self.s = s

    def _close(self, screen):
        screen.remove_notifier(self.s.fileno())
        print('[*] connection closed with fake button client')

    def can_read(self, s, screen):
        try:
            packet = self.s.recv(1)
        except:
            self._close(screen)
            return

        if packet == b'':
            self._close(screen)
            return

        for c in packet:
            c = chr(c)
            if c in self.actions.keys():
                key, pressed = self.actions[c]
                print('[*] button %d release: %s' % (key, repr(pressed)))
                screen.seph.handle_button(key, pressed)
            else:
                #print('[*] ignoring byte %s' % repr(c))
                pass

class FakeButton:
    def __init__(self, port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind(('0.0.0.0', port))
        self.s.listen(5)

    def can_read(self, s, screen):
        c, addr = self.s.accept()
        print('[*] new client from', addr)
        client = FakeButtonClient(c)
        screen.add_notifier(client)
