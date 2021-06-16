"""
Forward USB packets between the MCU and the SE using the (custom) SE Proxy HAL
protocol.
"""

from abc import ABC, abstractmethod
from construct import Int8ub, Int16ub, Int16ul, Struct
import binascii
import enum
import logging


class UsbReq(enum.IntEnum):
    RECIPIENT_DEVICE = 0x00
    SET_ADDRESS = 0x05
    SET_CONFIGURATION = 0x09


class SephUsbTag(enum.IntEnum):
    XFER_SETUP = 0x01
    XFER_IN = 0x02
    XFER_OUT = 0x04
    XFER_EVENT = 0x10
    PREPARE_DIR_IN = 0x20


class SephUsbConfig(enum.IntEnum):
    CONNECT = 0x01
    DISCONNECT = 0x02
    ADDR = 0x03
    ENDPOINTS = 0x04


class SephUsbPrepare(enum.IntEnum):
    SETUP = 0x10
    IN = 0x20
    OUT = 0x30
    STALL = 0x40
    UNSTALL = 0x80


class HidEndpoint(enum.IntEnum):
    OUT_ADDR = 0x00
    IN_ADDR = 0x80


class UsbDevState(enum.IntEnum):
    DISCONNECTED = 0
    DEFAULT = 1
    ADDRESSED = 2
    CONFIGURED = 3


class UsbInterface(enum.IntEnum):
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


class HidPacket:
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


class Transport(ABC):
    def __init__(self, interface, send_xfer):
        self.interface = interface
        self.send_xfer = send_xfer

    @abstractmethod
    def xfer(self, data):
        pass

    @abstractmethod
    def build_xfer(self, data):
        pass

    @abstractmethod
    def prepare(self, data):
        pass

    def config(self, tag):
        pass

    @property
    def endpoint_in(self):
        return HidEndpoint.IN_ADDR | self.interface

    @property
    def endpoint_out(self):
        return HidEndpoint.OUT_ADDR | self.interface


class U2f(Transport):
    def __init__(self, send_xfer):
        super().__init__(UsbInterface.U2F, send_xfer)

    def build_xfer(self, tag, data):
        packet = usb_header.build(dict(endpoint=self.endpoint_out, tag=tag, length=len(data)))
        packet += data
        return packet

    def xfer(self, data):
        assert len(data) == USB_SIZE
        packet = self.build_xfer(SephUsbTag.XFER_OUT, data)
        self.send_xfer(packet)

    def prepare(self, data):
        assert len(data) == USB_SIZE
        packet = self.build_xfer(SephUsbTag.XFER_IN, b'')
        self.send_xfer(packet)
        return data


class Hid(Transport):
    USB_CHANNEL = 0x0101
    USB_COMMAND = 0x05

    def __init__(self, send_xfer):
        super().__init__(UsbInterface.HID, send_xfer)
        self.hid_packet = HidPacket()

    def _build_header(self, data, length, seq):
        header = hid_header.build(dict(channel=self.USB_CHANNEL, command=self.USB_COMMAND, seq=seq, length=length))
        if seq != 0:
            # strip hid_header.length
            header = header[:-2]
        return header

    def build_xfer(self, tag, data, seq=0, length=USB_SIZE):
        header = self._build_header(data, length, seq)
        size = len(header) + len(data)

        packet = usb_header.build(dict(endpoint=self.endpoint_out, tag=tag, length=size))
        packet += header
        packet += data

        return packet

    def xfer(self, data):
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

            packet = self.build_xfer(SephUsbTag.XFER_OUT, chunk, seq, length)
            self.send_xfer(packet)

            offset += len(chunk)
            seq += 1

    def config(self, tag):
        if tag == UsbDevState.DISCONNECTED:
            self.hid_packet.reset(0)

    def prepare(self, data):
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
        packet = self.build_xfer(SephUsbTag.XFER_IN, b'', self.hid_packet.seq)
        self.send_xfer(packet)

        if self.hid_packet.complete():
            answer = self.hid_packet.data
            self.hid_packet.reset(0)
        else:
            answer = None

        return answer


class USB:
    def __init__(self, _queue_event_packet, transport='hid'):
        self._queue_event_packet = _queue_event_packet
        self.packets_to_send = []
        self.state = UsbDevState.DISCONNECTED

        if transport.lower() == 'hid':
            self.transport = Hid(self.send_xfer)
        elif transport.lower() == 'u2f':
            self.transport = U2f(self.send_xfer)
        else:
            raise ValueError(f"Unsupported USB transport {transport!r}")

        self.logger = logging.getLogger("usb")

    def send_xfer(self, packet):
        # don't send packets until the endpoint is configured
        if self.state != UsbDevState.CONFIGURED or len(self.packets_to_send) > 0:
            self.packets_to_send.append(packet)
            return

        self.logger.debug("[SEND_XFER] {}".format(binascii.hexlify(packet)))
        self._queue_event_packet(SephUsbTag.XFER_EVENT, packet)

    def _send_setup(self, breq, wValue):
        data = usb_header.build(dict(endpoint=self.transport.endpoint_out, tag=SephUsbTag.XFER_SETUP, length=0))
        data += usb_setup.build(dict(bmreq=UsbReq.RECIPIENT_DEVICE, breq=breq, wValue=wValue, wIndex=0, wLength=0))
        self.logger.debug("[SEND_SETUP] {}".format(binascii.hexlify(data)))
        self._queue_event_packet(SephUsbTag.XFER_EVENT, data)

    def _flush_packets(self):
        packets_to_send = self.packets_to_send
        self.packets_to_send = []
        for packet in packets_to_send:
            self.send_xfer(packet)

    def config(self, data):
        """Parse a config packet. If the endpoint address is set, configure it."""

        tag = SephUsbConfig(data[0])
        self.logger.debug("[CONFIG] {} {}".format(repr(tag), binascii.hexlify(data)))

        # The USB stack is shut down with USB_power(0) before being powered on.
        # Wait for the first CONNECT config message to ensure that USBD_Start()
        # has been called.
        if tag == SephUsbConfig.CONNECT:
            if self.state == UsbDevState.DISCONNECTED:
                self.state = UsbDevState.ADDRESSED
                self.logger.debug("set_address sent")
                self._send_setup(UsbReq.SET_ADDRESS, 1)

        elif tag == SephUsbConfig.DISCONNECT:
            self.state = UsbDevState.DISCONNECTED
            self.transport.config(tag)

        elif tag == SephUsbConfig.ADDR:
            if self.state == UsbDevState.ADDRESSED:
                self.state = UsbDevState.CONFIGURED
                self._send_setup(UsbReq.SET_CONFIGURATION, 1)
                self.logger.debug("configured")

        elif tag == SephUsbConfig.ENDPOINTS:
            # once the endpoint is configured, queued packets can be sent
            endpoint = data[2]
            if endpoint == self.transport.endpoint_out:
                self._flush_packets()

    def prepare(self, data):
        """Send or receive a packet chunk."""

        header = usb_header.parse(data[:3])
        answer = None
        tag = SephUsbPrepare(header.tag)
        self.logger.debug("[PREPARE] {} {} {}".format(repr(self.state), repr(tag), binascii.hexlify(data)))

        if tag == SephUsbPrepare.IN:
            if header.endpoint == self.transport.endpoint_in:
                assert header.length == USB_SIZE
                data = data[usb_header.sizeof():]
                answer = self.transport.prepare(data)

        return answer

    def xfer(self, data):
        self.transport.xfer(data)
