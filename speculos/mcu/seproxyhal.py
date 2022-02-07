from collections import namedtuple
from dataclasses import asdict
import logging
import sys
import time
import threading
from enum import IntEnum

from . import usb
from .nanox_ocr import NanoXOCR
from .readerror import ReadError, WriteError


class SephTag(IntEnum):
    BUTTON_PUSH_EVENT = 0x05
    FINGER_EVENT = 0x0c
    DISPLAY_PROCESSED_EVENT = 0x0d
    TICKER_EVENT = 0x0e
    CAPDU_EVENT = 0x16

    MCU = 0x31
    TAG_BLE_SEND = 0x38
    TAG_BLE_RADIO_POWER = 0x44
    SE_POWER_OFF = 0x46
    USB_CONFIG = 0x4f
    USB_EP_PREPARE = 0x50

    REQUEST_STATUS = 0x52
    RAPDU = 0x53
    PRINTC_STATUS = 0x5f

    GENERAL_STATUS = 0x60
    GENERAL_STATUS_LAST_COMMAND = 0x0000
    SCREEN_DISPLAY_STATUS = 0x65
    PRINTF_STATUS = 0x66
    SCREEN_DISPLAY_RAW_STATUS = 0x69

    FINGER_EVENT_TOUCH = 0x01
    FINGER_EVENT_RELEASE = 0x02


TICKER_DELAY = 0.1

RenderMethods = namedtuple('RenderMethods', 'PROGRESSIVE FLUSHED')
RENDER_METHOD = RenderMethods(0, 1)


def ticker(add_tick):
    """
    Function ran in a thread to trigger a tick at a periodic interval.

    There is no need to prevent clock drift.
    """

    logger = logging.getLogger("seproxyhal.ticker")
    flood = False

    while True:
        if not add_tick():
            if not flood:
                logger.warn("skipping ticker events (expected within a debugger)")
                flood = True
        else:
            flood = False
        time.sleep(TICKER_DELAY)


class PacketThread(threading.Thread):
    TICKER_PACKET = (SephTag.TICKER_EVENT, b'')

    def __init__(self, s, status_event, *args, **kwargs):
        super(PacketThread, self).__init__(name="packet", daemon=True)
        self.s = s
        self.queue_condition = threading.Condition()
        self.queue = []
        self.status_event = status_event
        self.logger = logging.getLogger("seproxyhal.packet")
        self.stop = False

    def _send_packet(self, tag, data=b''):
        """Send a packet to the app."""

        size = len(data).to_bytes(2, 'big')
        packet = tag.to_bytes(1, 'big') + size + data
        self.logger.debug("send {}" .format(packet.hex()))
        try:
            self.s.sendall(packet)
        except BrokenPipeError:
            self.stop = True
        except OSError:
            self.stop = True

    def queue_packet(self, tag, data=b'', priority=False):
        """
        Append a packet to the queue of packets to be sent.

        This function is meant to called by other threads.
        """

        if not priority:
            self.queue.append((tag, data))
        else:
            # Some status packets expect a specific event to be answered before
            # any other events. Put it at the beginning of the queue.
            self.queue.insert(0, (tag, data))

        # notify this thread that a new packet is available
        with self.queue_condition:
            self.queue_condition.notify()

    def add_tick(self):
        """Add a ticker event to the queue."""

        # Don't add too many ticker packets to the queue. For instance, the app
        # might be stuck if a breakpoint is hit within a debugger. It avoids
        # flooding the app on resume.
        if self.queue.count(self.TICKER_PACKET) > 10:
            return False

        self.queue_packet(SephTag.TICKER_EVENT)
        return True

    def run(self):
        while not self.stop:
            # wait for a status notification
            while not self.status_event.is_set():
                self.status_event.wait()
            self.status_event.clear()

            # wait for a packet to be available in the queue
            with self.queue_condition:
                while len(self.queue) == 0:
                    self.queue_condition.wait()

            tag, data = self.queue.pop(0)
            self._send_packet(tag, data)

        self.logger.debug("exiting")


class SeProxyHal:
    def __init__(self, s, automation=None, automation_server=None, transport='hid'):
        self.s = s
        self.logger = logging.getLogger("seproxyhal")
        self.printf_queue = ''
        self.automation = automation
        self.automation_server = automation_server
        self.events = []

        self.status_event = threading.Event()
        self.packet_thread = PacketThread(self.s, self.status_event)
        self.packet_thread.start()

        self.ticker_thread = threading.Thread(name="ticker",
                                              target=ticker,
                                              args=(self.packet_thread.add_tick,),
                                              daemon=True)
        self.ticker_thread.start()
        self.usb = usb.USB(self.packet_thread.queue_packet, transport=transport)

        self.nanox_ocr = NanoXOCR()

        # A list of callback methods when an APDU response is received
        self.apdu_callbacks = []

    def _recvall(self, size):
        data = b''
        while size > 0:
            try:
                tmp = self.s.recv(size)
            except ConnectionResetError:
                tmp = b''

            if len(tmp) == 0:
                self.logger.debug("fd closed")
                return None
            data += tmp
            size -= len(tmp)
        return data

    def _send_packet(self, tag, data=b''):
        '''Send packet to the app.'''

        size = len(data).to_bytes(2, 'big')
        packet = tag.to_bytes(1, 'big') + size + data
        try:
            self.s.sendall(packet)
        except BrokenPipeError:
            raise WriteError("Broken pipe, failed to send data to the app")

    def apply_automation_helper(self, event):
        if self.automation_server:
            self.automation_server.broadcast(asdict(event))

        if self.automation:
            actions = self.automation.get_actions(event.text, event.x, event.y)
            for action in actions:
                self.logger.debug(f"applying automation {action}")
                key, args = action[0], action[1:]
                if key == "button":
                    self.handle_button(*args)
                elif key == "finger":
                    self.handle_finger(*args)
                elif key == "setbool":
                    self.automation.set_bool(*args)
                elif key == "exit":
                    self.s.close()
                    sys.exit(0)
                else:
                    assert False

    def apply_automation(self):
        for event in self.events:
            self.apply_automation_helper(event)
        self.events = []

    def _close(self, s, screen):
        screen.remove_notifier(self.s.fileno())
        self.s.close()

    def can_read(self, s, screen):
        '''
        Handle packet sent by the app.

        This function is called thanks to a screen QSocketNotifier.
        '''

        assert s == self.s.fileno()

        data = self._recvall(3)
        if data is None:
            self._close(s, screen)
            raise ReadError("fd closed")

        tag = data[0]
        size = int.from_bytes(data[1:3], 'big')

        data = self._recvall(size)
        if data is None:
            self._close(s, screen)
            raise ReadError("fd closed")
        assert len(data) == size

        self.logger.debug(f"received (tag: {tag:#04x}, size: {size:#04x}): {data!r}")

        if tag & 0xf0 == SephTag.GENERAL_STATUS or tag == SephTag.PRINTC_STATUS:

            if tag == SephTag.GENERAL_STATUS:
                if int.from_bytes(data[:2], 'big') == SephTag.GENERAL_STATUS_LAST_COMMAND:
                    if screen.screen_update():
                        if screen.model in ["nanox", "nanosp"]:
                            self.events += self.nanox_ocr.get_events()

                    # Apply automation rules after having received a GENERAL_STATUS_LAST_COMMAND tag. It allows the
                    # screen to be updated before broadcasting the events.
                    if self.events:
                        self.apply_automation()

            elif tag == SephTag.SCREEN_DISPLAY_STATUS:
                self.logger.debug(f"DISPLAY_STATUS {data!r}")
                events = screen.display_status(data)
                if events:
                    self.events += events
                self.packet_thread.queue_packet(SephTag.DISPLAY_PROCESSED_EVENT, priority=True)

            elif tag == SephTag.SCREEN_DISPLAY_RAW_STATUS:
                self.logger.debug("SephTag.SCREEN_DISPLAY_RAW_STATUS")
                screen.display_raw_status(data)
                if screen.model in ["nanox", "nanosp"]:
                    self.nanox_ocr.analyze_bitmap(data)
                # https://github.com/LedgerHQ/nanos-secure-sdk/blob/1f2706941b68d897622f75407a868b60eb2be8d7/src/os_io_seproxyhal.c#L787
                #
                # io_seproxyhal_spi_recv() accepts any packet from the MCU after
                # having sent SCREEN_DISPLAY_RAW_STATUS. If some event (eg.
                # TICKER_EVENT) is replied before DISPLAY_PROCESSED_EVENT, it
                # will be silently ignored.
                #
                # A DISPLAY_PROCESSED_EVENT should be answered immediately,
                # hence priority=True.
                self.packet_thread.queue_packet(SephTag.DISPLAY_PROCESSED_EVENT, priority=True)
                if screen.rendering == RENDER_METHOD.PROGRESSIVE:
                    screen.screen_update()

            elif tag == SephTag.PRINTF_STATUS or tag == SephTag.PRINTC_STATUS:
                for b in [chr(b) for b in data]:
                    if b == '\n':
                        self.logger.info(f"printf: {self.printf_queue}")
                        self.printf_queue = ''
                    else:
                        self.printf_queue += b
                self.packet_thread.queue_packet(SephTag.DISPLAY_PROCESSED_EVENT, priority=True)
                if screen.rendering == RENDER_METHOD.PROGRESSIVE:
                    screen.screen_update()

            else:
                self.logger.error(f"unknown tag: {tag:#x}")
                sys.exit(0)

            # signal the sending thread that a status has been received
            self.status_event.set()

        elif tag == SephTag.RAPDU:
            screen.forward_to_apdu_client(data)
            for c in self.apdu_callbacks:
                c(data)

        elif tag == SephTag.USB_CONFIG:
            self.usb.config(data)

        elif tag == SephTag.USB_EP_PREPARE:
            data = self.usb.prepare(data)
            if data:
                for c in self.apdu_callbacks:
                    c(data)
                screen.forward_to_apdu_client(data)

        elif tag == SephTag.MCU:
            pass

        elif tag == SephTag.TAG_BLE_SEND:
            pass

        elif tag == SephTag.TAG_BLE_RADIO_POWER:
            self.logger.warn("ignoring BLE tag")

        elif tag == SephTag.SE_POWER_OFF:
            self.logger.warn("received tag SE_POWER_OFF, exiting")
            self._close(s, screen)
            raise ReadError("SE_POWER_OFF")

        elif tag == SephTag.REQUEST_STATUS:
            # Ignore calls to io_seproxyhal_request_mcu_status()
            pass

        else:
            self.logger.error(f"unknown tag: {tag:#x}")
            sys.exit(0)

    def handle_button(self, button, pressed):
        '''Forward button press/release from the GUI to the app.'''

        if pressed:
            self.packet_thread.queue_packet(SephTag.BUTTON_PUSH_EVENT, (button << 1).to_bytes(1, 'big'))
        else:
            self.packet_thread.queue_packet(SephTag.BUTTON_PUSH_EVENT, (0 << 1).to_bytes(1, 'big'))

    def handle_finger(self, x, y, pressed):
        '''Forward finger press/release from the GUI to the app.'''

        if pressed:
            packet = SephTag.FINGER_EVENT_TOUCH.to_bytes(1, 'big')
        else:
            packet = SephTag.FINGER_EVENT_RELEASE.to_bytes(1, 'big')
        packet += x.to_bytes(2, 'big')
        packet += y.to_bytes(2, 'big')

        self.packet_thread.queue_packet(SephTag.FINGER_EVENT, packet)

    def to_app(self, packet):
        '''
        Forward raw APDU to the app.

        Packets can be forwarded directly to the SE thanks to
        SephTag.CAPDU_EVENT, but it doesn't work with messages are larger
        than G_io_seproxyhal_spi_buffer. Emulating a basic USB stack is more
        reliable, see issue #11.
        '''

        if packet.startswith(b'RAW!') and len(packet) > 4:
            tag, packet = packet[4], packet[5:]
            self.packet_thread.queue_packet(tag, packet)
        else:
            self.usb.xfer(packet)
