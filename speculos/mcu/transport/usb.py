"""
Forward USB packets between the MCU and the SE using the (custom) SE Proxy HAL
protocol.
"""

import enum
import logging
from abc import abstractmethod
from construct import Int8ub, Int16ub, Int16ul, Struct
from typing import Callable, List, Optional

from .interface import TransportLayer, TransportType


class USBReq(enum.IntEnum):
    RECIPIENT_DEVICE = 0x00
    SET_ADDRESS = 0x05
    SET_CONFIGURATION = 0x09


class SephUSBTag(enum.IntEnum):
    XFER_SETUP = 0x01
    XFER_IN = 0x02
    XFER_OUT = 0x04
    XFER_EVENT = 0x10
    PREPARE_DIR_IN = 0x20


class SephUSBConfig(enum.IntEnum):
    CONNECT = 0x01
    DISCONNECT = 0x02
    ADDR = 0x03
    ENDPOINTS = 0x04


class SephUSBPrepare(enum.IntEnum):
    SETUP = 0x10
    IN = 0x20
    OUT = 0x30
    STALL = 0x40
    UNSTALL = 0x80


class HIDEndpoint(enum.IntEnum):
    OUT_ADDR = 0x00
    IN_ADDR = 0x80


class USBDevState(enum.IntEnum):
    DISCONNECTED = 0
    DEFAULT = 1
    ADDRESSED = 2
    CONFIGURED = 3


class USBInterface(enum.IntEnum):
    GENERIC = 0
    U2F = 1
    HID = 2


USB_SIZE = 0x40

usb_header = Struct(
    "endpoint" / Int8ub,
    "tag" / Int8ub,
    "length" / Int8ub,
)

usb_setup = Struct(
    "bmreq" / Int8ub,
    "breq" / Int8ub,
    "wValue" / Int16ul,
    "wIndex" / Int16ub,
    "wLength" / Int16ub,
)

hid_header = Struct(
    "channel" / Int16ub,
    "command" / Int8ub,
    "seq" / Int16ub,
    "length" / Int16ub,
)


class HIDPacket:
    def __init__(self):
        self.reset(0)

    def reset(self, size):
        self.data = b''
        self.seq = 0
        self.remaining_size = size

    def append_data(self, data):
        self.seq += 1
        if len(data) <= self.remaining_size:
            self.data += data
            self.remaining_size -= len(data)
        else:
            self.data += data[:self.remaining_size]
            self.remaining_size = 0

    def complete(self):
        return self.remaining_size == 0


class USBTransport(TransportLayer):
    INTERFACE = USBInterface.GENERIC

    def __init__(self, send_cb: Callable, transport: TransportType = TransportType.HID):
        super().__init__(send_cb, transport)
        self.packets_to_send: List[bytes] = []
        self.state = USBDevState.DISCONNECTED
        self.logger = logging.getLogger("USB")

    @property
    def endpoint_in(self):
        return HIDEndpoint.IN_ADDR | self.INTERFACE

    @property
    def endpoint_out(self):
        return HIDEndpoint.OUT_ADDR | self.INTERFACE

    def _send_xfer(self, packet: bytes) -> None:
        # don't send packets until the endpoint is configured
        if self.state != USBDevState.CONFIGURED or len(self.packets_to_send) > 0:
            self.packets_to_send.append(packet)
            return

        self.logger.debug("[SEND_XFER] %s", packet.hex())
        self._send_cb(SephUSBTag.XFER_EVENT, packet)

    def _send_setup(self, breq: USBReq, wValue: int):
        data = usb_header.build(dict(endpoint=self.endpoint_out, tag=SephUSBTag.XFER_SETUP, length=0))
        data += usb_setup.build(dict(bmreq=USBReq.RECIPIENT_DEVICE, breq=breq, wValue=wValue, wIndex=0, wLength=0))
        self.logger.debug("[SEND_SETUP] %s", data.hex())
        self._send_cb(SephUSBTag.XFER_EVENT, data)

    def _flush_packets(self) -> None:
        packets_to_send = self.packets_to_send
        self.packets_to_send = []
        for packet in packets_to_send:
            self._send_xfer(packet)

    def handle_rapdu(self, data: bytes) -> Optional[bytes]:
        self.logger.warning("NFC-specific 'handle_apdu' method called on USB transport. Ignored.")
        return None

    @abstractmethod
    def _config(self, data: SephUSBConfig) -> None:
        raise NotImplementedError

    def config(self, data: bytes) -> None:
        """Parse a config packet. If the endpoint address is set, configure it."""

        tag = SephUSBConfig(data[0])
        self.logger.debug("[CONFIG] %s %s", repr(tag), data.hex())

        # The USB stack is shut down with USB_power(0) before being powered on.
        # Wait for the first CONNECT config message to ensure that USBD_Start()
        # has been called.
        if tag == SephUSBConfig.CONNECT:
            if self.state == USBDevState.DISCONNECTED:
                self.state = USBDevState.ADDRESSED
                self.logger.debug("set_address sent")
                self._send_setup(USBReq.SET_ADDRESS, 1)

        elif tag == SephUSBConfig.DISCONNECT:
            self.state = USBDevState.DISCONNECTED
            self._config(tag)

        elif tag == SephUSBConfig.ADDR:
            if self.state == USBDevState.ADDRESSED:
                self.state = USBDevState.CONFIGURED
                self._send_setup(USBReq.SET_CONFIGURATION, 1)
                self.logger.debug("USB configured")

        elif tag == SephUSBConfig.ENDPOINTS:
            # once the endpoint is configured, queued packets can be sent
            endpoint = data[2]
            if endpoint == self.endpoint_out:
                self._flush_packets()

    @abstractmethod
    def _prepare(self, data: bytes) -> Optional[bytes]:
        raise NotImplementedError

    def prepare(self, data: bytes) -> Optional[bytes]:
        """Send or receive a packet chunk."""

        header = usb_header.parse(data[:3])
        answer = None
        tag = SephUSBPrepare(header.tag)
        self.logger.debug("[PREPARE] %s %s %s", repr(self.state), repr(tag), data.hex())

        if tag == SephUSBPrepare.IN:
            if header.endpoint == self.endpoint_in:
                assert header.length == USB_SIZE
                data = data[usb_header.sizeof():]
                answer = self._prepare(data)

        return answer


class U2F(USBTransport):
    INTERFACE = USBInterface.U2F

    def __init__(self, send_cb: Callable, transport: TransportType):
        super().__init__(send_cb, transport)

    def _config(self, data: SephUSBConfig) -> None:
        pass

    def _build_xfer(self, tag: SephUSBTag, data: bytes) -> bytes:
        packet = usb_header.build(dict(endpoint=self.endpoint_out, tag=tag, length=len(data)))
        packet += data
        return packet

    def send(self, data: bytes) -> None:
        assert len(data) == USB_SIZE
        packet = self._build_xfer(SephUSBTag.XFER_OUT, data)
        self._send_xfer(packet)

    def _prepare(self, data: bytes) -> bytes:
        assert len(data) == USB_SIZE
        packet = self._build_xfer(SephUSBTag.XFER_IN, b'')
        self._send_xfer(packet)
        return data


class HID(USBTransport):
    INTERFACE = USBInterface.HID

    USB_CHANNEL = 0x0101
    USB_COMMAND = 0x05

    def __init__(self, send_cb: Callable, transport: TransportType):
        super().__init__(send_cb, transport)
        self.hid_packet = HIDPacket()

    def _build_header(self, data: bytes, length: int, seq: int) -> bytes:
        header = hid_header.build(dict(channel=self.USB_CHANNEL, command=self.USB_COMMAND, seq=seq, length=length))
        if seq != 0:
            # strip hid_header.length
            header = header[:-2]
        return header

    def _build_xfer(self, tag: SephUSBTag, data: bytes, seq: int = 0, length: int = USB_SIZE):
        header = self._build_header(data, length, seq)
        size = len(header) + len(data)

        packet = usb_header.build(dict(endpoint=self.endpoint_out, tag=tag, length=size))
        packet += header
        packet += data

        return packet

    def send(self, data: bytes) -> None:
        seq = 0
        offset = 0
        while offset < len(data):
            size = USB_SIZE - hid_header.sizeof() - 10
            if seq != 0:
                size += 2
            chunk = data[offset:offset+size]
            chunk = chunk.ljust(size, b'\x00')
            if seq == 0:
                length = len(data)
            else:
                length = len(chunk)

            packet = self._build_xfer(SephUSBTag.XFER_OUT, chunk, seq, length)
            self._send_xfer(packet)

            offset += len(chunk)
            seq += 1

    def _config(self, tag: SephUSBConfig) -> None:
        if tag == USBDevState.DISCONNECTED:
            self.hid_packet.reset(0)

    def _prepare(self, data: bytes) -> Optional[bytes]:
        hid = hid_header.parse(data)
        assert hid.channel == self.USB_CHANNEL
        assert hid.command == self.USB_COMMAND

        if hid.seq == 0:
            self.hid_packet.reset(hid.length)
            chunk = data[hid_header.sizeof():]
        else:
            assert hid.seq == self.hid_packet.seq
            chunk = data[hid_header.sizeof() - 2:]

        self.hid_packet.append_data(chunk)
        packet = self._build_xfer(SephUSBTag.XFER_IN, b'', self.hid_packet.seq)
        self._send_xfer(packet)

        if self.hid_packet.complete():
            answer = self.hid_packet.data
            self.hid_packet.reset(0)
        else:
            answer = None

        return answer
