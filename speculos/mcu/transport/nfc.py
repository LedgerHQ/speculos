"""
Forward NFC packets between the MCU and the SE
"""

import enum
import logging
from typing import List, Optional

from .interface import TransportLayer, TransportType


class SephNfcTag(enum.IntEnum):
    NFC_APDU_EVENT = 0x1C
    NFC_EVENT = 0x1E


class NFC(TransportLayer):
    def __init__(self, send_cb, transport: TransportType):
        super().__init__(send_cb, transport)
        self.MTU = 140
        self.rx_sequence = 0
        self.rx_size = 0
        self.rx_data: bytes = b''
        self.logger = logging.getLogger("NFC")

    def config(self, data: bytes) -> None:
        self.logger.warning("USB-specific 'config' method called on NFC transport. Ignored.")

    def prepare(self, data: bytes) -> None:
        self.logger.warning("USB-specific 'prepare' method called on NFC transport. Ignored.")

    def handle_rapdu(self, data: bytes) -> Optional[bytes]:
        """concatenate apdu chunks into full apdu"""
        # example of data
        # 0000050000002b3330000409312e302e302d72633104e600000008362e312e302d646508352e312e302d6465010001009000

        # only APDU packets are supported
        if data[2] != 0x05:
            return None

        sequence = int.from_bytes(data[3:5], 'big')
        assert self.rx_sequence == sequence, f"Unexpected sequence number:{sequence}"

        if sequence == 0:
            self.rx_size = int.from_bytes(data[5:7], "big")
            self.rx_data = data[7:]
        else:
            self.rx_data += data[5:]

        if len(self.rx_data) == self.rx_size:
            # prepare for next call
            self.rx_sequence = 0
            return self.rx_data
        else:
            self.rx_sequence += 1
        return None

    def send(self, data: bytes) -> None:
        chunks: List[bytes] = []
        data_len = len(data)

        while len(data) > 0:
            size = self.MTU - 5
            chunks.append(data[:size])
            data = data[size:]

        for i, chunk in enumerate(chunks):
            # Ledger protocol header
            header = bytes([0x00, 0x00, 0x05])  # APDU
            header += i.to_bytes(2, "big")
            # first packet contains the size of full buffer
            if i == 0:
                header += data_len.to_bytes(2, "big")

            self._send_cb(SephNfcTag.NFC_APDU_EVENT, header + chunk)
