'''
Forward packets between an external application and the emulated device.

Internally, it makes use of a TCP socket server to allow these 2 components to
communicate.
'''

import binascii
import errno
import socket
import sys

class ApduServer:
    def __init__(self, host='127.0.0.1', port=9999, hid=False):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.s.bind((host, port))
        self.s.listen()

        self.client = None

    def can_read(self, s, screen):
        assert self.s.fileno() == s

        c, _ = self.s.accept()
        self.client = ApduClient(c)
        screen.add_notifier(self.client)

    def forward_to_client(self, packet):
        self.client.forward_to_client(packet)

class ApduClient:
    def __init__(self, s):
        self.s = s

    def _recvall(self, size):
        data = b''
        while size > 0:
            try:
                tmp = self.s.recv(size)
            except ConnectionResetError:
                tmp = b''
            if len(tmp) == 0:
                print("[-] apduthread: connection with client closed", file=sys.stderr)
                return None
            data += tmp
            size -= len(tmp)
        return data

    def recv_packet(self):
        data = self._recvall(4)
        if data is None:
            return None

        size = int.from_bytes(data, byteorder='big')
        packet = self._recvall(size)
        if packet is None:
            return None

        return packet

    def can_read(self, s, screen):
        '''Forward APDU packet to the app'''

        assert self.s.fileno() == s

        packet = self.recv_packet()
        if packet is None:
            screen.remove_notifier(self.s.fileno())
            self.s.close()
            return

        print('[*] packet', repr(packet))
        screen.forward_to_app(packet)

    def forward_to_client(self, packet):
        '''Encode and forward APDU to the client.'''

        print('[*] apdupthread: RAPDU: %s' % binascii.hexlify(packet), file=sys.stderr)

        size = len(packet) - 2
        packet = size.to_bytes(4, 'big') + packet
        try:
            self.s.sendall(packet)
        except OSError as e:
            if e.errno == errno.EBADF:
                # the connection with the client was closed, ignore any error
                pass
            else:
                raise
