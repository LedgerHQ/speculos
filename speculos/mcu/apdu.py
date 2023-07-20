from __future__ import annotations

'''
Forward packets between an external application and the emulated device.

Internally, it makes use of a TCP socket server to allow these 2 components to
communicate.
'''

import errno
import logging
import socket
from typing import Optional

from .display import IODevice, DisplayNotifier


class ApduServer(IODevice):
    def __init__(self, host: str = '127.0.0.1', port: int = 9999, hid: bool = False):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind((host, port))  # lgtm [py/bind-socket-all-network-interfaces]
        self.socket.listen()
        self.client: Optional[ApduClient] = None

    @property
    def file(self):
        return self.socket

    def can_read(self, screen: DisplayNotifier):
        c, _ = self.file.accept()
        self.client = ApduClient(c)
        screen.add_notifier(self.client)

    def forward_to_client(self, packet: bytes):
        if self.client is not None:
            self.client.forward_to_client(packet)


class ApduClient(IODevice):
    def __init__(self, sock: socket.socket):
        self._socket = sock
        self.logger = logging.getLogger("apdu")

    @property
    def file(self):
        return self._socket

    def _recvall(self, size):
        data = b''
        while size > 0:
            try:
                tmp = self.file.recv(size)
            except ConnectionResetError:
                tmp = b''
            if len(tmp) == 0:
                self.logger.debug("connection with client closed")
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

    def can_read(self, screen: DisplayNotifier) -> None:
        '''Forward APDU packet to the app'''
        packet = self.recv_packet()
        if packet is None:
            screen.remove_notifier(self.fileno)
            self.file.close()
            return

        self.logger.info("> {}".format(packet.hex()))
        screen.display.forward_to_app(packet)

    def forward_to_client(self, packet):
        '''Encode and forward APDU to the client.'''

        self.logger.info("< {}".format(packet.hex()))

        size = (len(packet) - 2) & 0xffffffff
        packet = size.to_bytes(4, 'big') + packet
        try:
            self.file.sendall(packet)
        except OSError as e:
            if e.errno == errno.EBADF:
                # the connection with the client was closed, ignore any error
                pass
            else:
                raise
