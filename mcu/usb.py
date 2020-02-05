"""
Forward USB packets between the MCU and the SE using the (custom) SE Proxy HAL
protocol.
"""

from construct import *
import enum

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
    def __init__(self, _queue_event_packet):
        self.usb_packet = UsbPacket()
        self._queue_event_packet = _queue_event_packet
        self.packets_to_send = []
        self.configured = False
        self.set_address_sent = False

    def _send_xfer(self, tag, packet=b'', length=USB_SIZE, seq=0):
        data = hid_header.build(dict(channel=USB_CHANNEL, command=USB_COMMAND, seq=seq, length=length))
        size = len(data) + len(packet)
        data = usb_header.build(dict(endpoint=HidEndpoint.OUT_ADDR, tag=tag, length=size)) + data
        if seq != 0:
            # strip hid_header.length
            data = data[:-2]
        data += packet
        self._queue_event_packet(SephUsbTag.XFER_EVENT, data)

    def _send_xfer_out(self, packet):
        seq = 0
        offset = 0
        print(len(packet))
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

    def _send_setup(self, breq):
        data = usb_header.build(dict(endpoint=HidEndpoint.OUT_ADDR, tag=SephUsbTag.XFER_SETUP, length=0))
        data += usb_setup.build(dict(bmreq=UsbReq.RECIPIENT_DEVICE, breq=breq, wValue=0x0001, wIndex=0, wLength=0))
        self._queue_event_packet(SephUsbTag.XFER_EVENT, data)

    def config(self, data):
        """Parse a config packet. If the endpoint address is set, configure it."""

        tag = data[0]
        if tag == SephUsbConfig.ADDR:
            self._send_setup(UsbReq.SET_CONFIGURATION)
        elif tag not in [ SephUsbConfig.CONNECT, SephUsbConfig.DISCONNECT, SephUsbConfig.ENDPOINTS ]:
            print('[-] unknown USB config tag: 0x{:02x}'.format(tag))

    def prepare(self, data):
        """Send or receive a packet chunk."""

        assert len(data) >= 3
        header = usb_header.parse(data[:3])
        answer = None

        if header.tag == SephUsbPrepare.IN:
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

        elif header.tag == SephUsbPrepare.OUT:
            # once the endpoint is configured, queued packets can be sent
            self.configured = True
            for packet in self.packets_to_send:
                self._send_xfer_out(packet)
            self.packets_to_send = []

        elif header.tag not in [ SephUsbPrepare.SETUP, SephUsbPrepare.STALL, SephUsbPrepare.UNSTALL ]:
            print('[-] unknown USB prepare tag: 0x{:02x}'.format(tag))

        return answer

    def xfer(self, packet):
        if not self.configured:
            # don't send packets until the endpoint is configured
            self.packets_to_send.append(packet)
            if not self.set_address_sent:
                self.set_address_sent = True
                self._send_setup(UsbReq.SET_ADDRESS)
        else:
            self._send_xfer_out(packet)
