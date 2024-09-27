"""
Forward NFC packets between the MCU and the SE
"""

from abc import ABC, abstractmethod
from construct import Int8ub, Int16ub, Int16ul, Struct
import binascii
import enum
import logging


class SephNfcTag(enum.IntEnum):
    NFC_APDU_EVENT = 0x1C
    NFC_EVENT = 0x1E


class NFC:
    def __init__(self, _queue_event_packet):
        self._queue_event_packet = _queue_event_packet
        self.packets_to_send = []
        self.MTU=140
        self.rx_sequence = 0
        self.rx_size = 0
        self.rx_data = []

        self.logger = logging.getLogger("nfc")


    def handle_rapdu_chunk(self, data):
        """concatenate apdu chunks into full apdu"""

        # example of data
        # 0000050000002b3330000409312e302e302d72633104e600000008362e312e302d646508352e312e302d6465010001009000

        # only APDU packets are suported
        if data[2] != 0x05:
            return None

        sequence = int.from_bytes(data[3:5], 'big')
        if self.rx_sequence != sequence:
            print(f"Unexpected sequence number:{sequence}")
            return None

        if sequence == 0:
            self.rx_size = int.from_bytes(data[5:7], "big")
            self.rx_data = data[7:]
        else:
            self.rx_data.append(data[5:])

        if len(self.rx_data) == self.rx_size:
            #prepare for next call
            self.rx_sequence = 0
            return self.rx_data
        
        return None


    def apdu(self, data):
        chunks: List[bytes] = []
        data_len = len(data)

        while len(data) > 0:
            size = self.MTU-5
            chunks.append(data[:size])
            data = data[size:]

        for i, chunk in enumerate(chunks):
            # ledger protocol header
            header = bytes([0x00, 0x00, 0x05]) # APDU
            header += i.to_bytes(2, "big")
            # first packet contains the size of full buffer
            if i == 0:
                header += data_len.to_bytes(2, "big")

            self._queue_event_packet(SephNfcTag.NFC_APDU_EVENT, header+chunk)
