"""
Forward USB packets between the MCU and the SE using the (custom) SE Proxy HAL
protocol.
"""

from construct import *
import binascii
import enum
import logging

class UsbReq(enum.IntEnum):
    RECIPIENT_DEVICE  = 0x00
    SET_ADDRESS       = 0x05
    SET_CONFIGURATION = 0x09

class SephUsbTag(enum.IntEnum):
    XFER_SETUP     = 0x01
    XFER_IN        = 0x02
    XFER_OUT       = 0x04
    XFER_EVENT     = 0x10
    PREPARE_DIR_IN = 0x20

class SephUsbConfig(enum.IntEnum):
    CONNECT    = 0x01
    DISCONNECT = 0x02
    ADDR       = 0x03
    ENDPOINTS  = 0x04

class SephUsbPrepare(enum.IntEnum):
    SETUP   = 0x10
    IN      = 0x20
    OUT     = 0x30
    STALL   = 0x40
    UNSTALL = 0x80

class HidEndpoint(enum.IntEnum):
    OUT_ADDR = 0x02
    IN_ADDR  = 0x82

class UsbDevState(enum.IntEnum):
    DISCONNECTED = 0
    DEFAULT      = 1
    ADDRESSED    = 2
    CONFIGURED   = 3

USB_CHANNEL = 0x0101
USB_COMMAND = 0x05
USB_SIZE    = 0x40

usb_header = Struct(
    "endpoint" / Int8ub,
    "tag"      / Int8ub,
    "length"   / Int8ub,
)

usb_setup = Struct(
    "bmreq"   / Int8ub,
    "breq"    / Int8ub,
    "wValue"  / Int16ul,
    "wIndex"  / Int16ub,
    "wLength" / Int16ub,
)

hid_header = Struct(
    "channel" / Int16ub,
    "command" / Int8ub,
    "seq"     / Int16ub,
    "length"  / Int16ub,
)

class UsbPacket:
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

class USB:
    def __init__(self, _queue_event_packet, debug=False):
        self.usb_packet = UsbPacket()
        self._queue_event_packet = _queue_event_packet
        self.packets_to_send = []
        self.state = UsbDevState.DISCONNECTED

        level = logging.INFO
        if debug:
            level = logging.DEBUG
        logging.basicConfig(level=level, format='%(asctime)s.%(msecs)03d - USB: %(message)s', datefmt='%H:%M:%S')

    def _send_xfer(self, tag, packet=b'', length=USB_SIZE, seq=0):
        data = hid_header.build(dict(channel=USB_CHANNEL, command=USB_COMMAND, seq=seq, length=length))
        size = len(data) + len(packet)
        if seq != 0:
            # strip hid_header.length
            size -= 2
            data = data[:-2]
        data = usb_header.build(dict(endpoint=HidEndpoint.OUT_ADDR, tag=tag, length=size)) + data
        data += packet
        logging.debug('[SEND_XFER] {}'.format(binascii.hexlify(data)))
        self._queue_event_packet(SephUsbTag.XFER_EVENT, data)

    def _send_xfer_out(self, packet):
        seq = 0
        offset = 0
        while offset < len(packet):
            size = USB_SIZE - hid_header.sizeof() - 10
            if seq != 0:
                size += 2
            chunk = packet[offset:offset+size]
            chunk = chunk.ljust(size, b'\x00')
            if seq == 0:
                length = len(packet)
            else:
                length = len(chunk)
            self._send_xfer(SephUsbTag.XFER_OUT, seq=seq, packet=chunk, length=length)
            offset += len(chunk)
            seq += 1

    def _send_setup(self, breq, wValue):
        data = usb_header.build(dict(endpoint=HidEndpoint.OUT_ADDR, tag=SephUsbTag.XFER_SETUP, length=0))
        data += usb_setup.build(dict(bmreq=UsbReq.RECIPIENT_DEVICE, breq=breq, wValue=wValue, wIndex=0, wLength=0))
        logging.debug('[SEND_SETUP] {}'.format(binascii.hexlify(data)))
        self._queue_event_packet(SephUsbTag.XFER_EVENT, data)

    def _flush_packets(self):
        for packet in self.packets_to_send:
            self._send_xfer_out(packet)
        self.packets_to_send = []

    def config(self, data):
        """Parse a config packet. If the endpoint address is set, configure it."""

        tag = SephUsbConfig(data[0])
        logging.debug('[CONFIG] {} {}'.format(repr(tag), binascii.hexlify(data)))

        # The USB stack is shut down with USB_power(0) before being powered on.
        # Wait for the first CONNECT config message to ensure that USBD_Start()
        # has been called.
        if self.state == UsbDevState.DISCONNECTED:
            if tag == SephUsbConfig.CONNECT:
                self.state = UsbDevState.DEFAULT
            return

        if tag == SephUsbConfig.ADDR:
            if self.state == UsbDevState.ADDRESSED:
                self.state = UsbDevState.CONFIGURED
                self._send_setup(UsbReq.SET_CONFIGURATION, 1)
                logging.debug('configured!')
        elif tag == SephUsbConfig.ENDPOINTS:
            # once the endpoint is configured, queued packets can be sent
            endpoint = data[2]
            if endpoint == HidEndpoint.OUT_ADDR:
                self._flush_packets()

    def prepare(self, data):
        """Send or receive a packet chunk."""

        header = usb_header.parse(data[:3])
        answer = None
        tag = SephUsbPrepare(header.tag)
        logging.debug('[PREPARE] {} {} {}'.format(repr(self.state), repr(tag), binascii.hexlify(data)))

        if tag == SephUsbPrepare.IN:
            if header.endpoint == HidEndpoint.IN_ADDR:
                assert header.length == 64
                data = data[usb_header.sizeof():]

                hid = hid_header.parse(data)
                assert hid.channel == USB_CHANNEL
                assert hid.command == USB_COMMAND

                if hid.seq == 0:
                    self.usb_packet.reset(hid.length)
                    chunk = data[hid_header.sizeof():]
                else:
                    assert hid.seq == self.usb_packet.seq
                    chunk = data[hid_header.sizeof() - 2:]

                self.usb_packet.append_data(chunk)
                self._send_xfer(SephUsbTag.XFER_IN, seq=self.usb_packet.seq)

                if self.usb_packet.complete():
                    answer = self.usb_packet.data
                    self.usb_packet.reset(0)

        return answer

    def xfer(self, packet):
        if self.state != UsbDevState.CONFIGURED or len(self.packets_to_send) > 0:
            # don't send packets until the endpoint is configured
            self.packets_to_send.append(packet)
            if self.state == UsbDevState.DEFAULT:
                self.state = UsbDevState.ADDRESSED
                logging.debug('set_address sent!')
                self._send_setup(UsbReq.SET_ADDRESS, 1)
        else:
            self._send_xfer_out(packet)
