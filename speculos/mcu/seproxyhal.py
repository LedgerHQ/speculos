from collections import namedtuple
import logging
import sys
import time
import threading
import socket
from enum import IntEnum
from typing import List, Callable, Optional, Tuple

from . import usb
from .ocr import OCR
from .readerror import ReadError, WriteError
from .automation import Automation, TextEvent
from .automation_server import AutomationServer


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
    PLAY_TUNE = 0x56

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


class TimeTickerDaemon(threading.Thread):
    def __init__(self, add_tick: Callable, *args, **kwargs):
        """
        Initializes the Ticker Daemon
        The goal of this daemon is to emulate the time spent in the application by sending it
        ticker events at a regular interval TICKER_DELAY.
        This daemon can be paused and resumed through its API to stop the flow of time in the MCU

        :param add_tick: The callback function to add a Ticker Event to the packet manager
        :type add_tick: Backend
        """
        super().__init__(name="time_ticker", daemon=True)
        self._paused = False
        self._resume_cond = threading.Condition()
        self.add_tick = add_tick

    def pause(self):
        """
        Pause time emulation done by the daemon, no ticker event will be sent until resume
        """
        self._paused = True

    def resume(self):
        """
        Resume time emulation done by the daemon, no ticker event will be sent until resume
        """
        self._paused = False
        with self._resume_cond:
            self._resume_cond.notify()

    def _wait_if_time_paused(self):
        """
        Internal function to handle the pause
        """
        while self._paused:
            with self._resume_cond:
                self._resume_cond.wait()

    def run(self):
        """
        Main thread function
        """
        while True:
            self._wait_if_time_paused()
            self.add_tick()
            time.sleep(TICKER_DELAY)


class PacketDaemon(threading.Thread):
    def __init__(self, s: socket.socket, status_event: threading.Event, *args, **kwargs):
        super().__init__(name="packet", daemon=True)
        self.s = s
        self.queue_condition = threading.Condition()
        self.queue: List[Tuple[SephTag, bytes]] = []
        self.status_event = status_event
        self.logger = logging.getLogger("seproxyhal.packet")
        self.stop = False
        self.ticks_count = 0

    def _send_packet(self, tag: SephTag, data: bytes = b''):
        """Send a packet to the app."""

        size: bytes = len(data).to_bytes(2, 'big')
        packet: bytes = tag.to_bytes(1, 'big') + size + data
        self.logger.debug("send {}" .format(packet.hex()))
        try:
            self.s.sendall(packet)
        except BrokenPipeError:
            self.stop = True
        except OSError:
            self.stop = True

    def queue_packet(self, tag: SephTag, data: bytes = b'', priority: bool = False):
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

        # Drop ticker packet if one is already present in the queue.
        # For instance, the app might be stuck if a breakpoint is hit within a debugger.
        # It avoids flooding the app on resume.
        for tag, _ in self.queue:
            if tag == SephTag.TICKER_EVENT:
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

            if tag == SephTag.TICKER_EVENT:
                self.ticks_count += 1

        self.logger.debug("exiting")

    def get_processed_ticks_count(self):
        return self.ticks_count


class SeProxyHal:
    def __init__(self,
                 s: socket.socket,
                 automation: Optional[Automation] = None,
                 automation_server: Optional[AutomationServer] = None,
                 transport: str = 'hid'):
        self.s = s
        self.logger = logging.getLogger("seproxyhal")
        self.printf_queue = ''
        self.automation = automation
        self.automation_server = automation_server
        self.events: List[TextEvent] = []

        self.status_event = threading.Event()
        self.packet_thread = PacketDaemon(self.s, self.status_event)
        self.packet_thread.start()

        self.time_ticker_thread = TimeTickerDaemon(self.packet_thread.add_tick)
        self.time_ticker_thread.start()

        self.usb = usb.USB(self.packet_thread.queue_packet, transport=transport)

        self.ocr = OCR()

        # A list of callback methods when an APDU response is received
        self.apdu_callbacks: List[Callable] = []

    def _recvall(self, size: int):
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

    def _send_packet(self, tag: SephTag, data: bytes = b''):
        '''Send packet to the app.'''

        size = len(data).to_bytes(2, 'big')
        packet = tag.to_bytes(1, 'big') + size + data
        try:
            self.s.sendall(packet)
        except BrokenPipeError:
            raise WriteError("Broken pipe, failed to send data to the app")

    def apply_automation_helper(self, event: TextEvent):
        if self.automation_server:
            self.automation_server.broadcast(event)

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

    def _close(self, s: socket.socket, screen):
        screen.remove_notifier(self.s.fileno())
        self.s.close()

    def can_read(self, s: socket.socket, screen):
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
                    if screen.model != "stax" and screen.screen_update():
                        if screen.model in ["nanox", "nanosp"]:
                            self.events += self.ocr.get_events()
                    elif screen.model == "stax":
                        self.events += self.ocr.get_events()

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
                    # Pause flow of time while the OCR is running
                    self.time_ticker_thread.pause()
                    self.ocr.analyze_bitmap(data)
                    self.time_ticker_thread.resume()
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
            elif tag == 0x6a:
                screen.nbgl.hal_draw_rect(data)
            elif tag == 0x6b:
                screen.nbgl.hal_refresh(data)

                if not screen.nbgl.disable_tesseract:
                    # Pause flow of time while the OCR is running
                    self.time_ticker_thread.pause()
                    screen.nbgl.m.update_screenshot()
                    screen_size, image_data = screen.nbgl.m.take_screenshot()
                    self.ocr.analyze_image(screen_size, image_data)
                    self.time_ticker_thread.resume()

            elif tag == 0x6c:
                screen.nbgl.hal_draw_line(data)
            elif tag == 0x6d:
                screen.nbgl.hal_draw_image(data)
            elif tag == 0x6e:
                screen.nbgl.hal_draw_image_file(data)
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

        elif tag == SephTag.PLAY_TUNE:
            pass

        else:
            self.logger.error(f"unknown tag: {tag:#x}")
            sys.exit(0)

    def handle_button(self, button: int, pressed: bool):
        '''Forward button press/release from the GUI to the app.'''

        if pressed:
            self.packet_thread.queue_packet(SephTag.BUTTON_PUSH_EVENT, (button << 1).to_bytes(1, 'big'))
        else:
            self.packet_thread.queue_packet(SephTag.BUTTON_PUSH_EVENT, (0 << 1).to_bytes(1, 'big'))

    def handle_finger(self, x: int, y: int, pressed: bool):
        '''Forward finger press/release from the GUI to the app.'''

        if pressed:
            packet = SephTag.FINGER_EVENT_TOUCH.to_bytes(1, 'big')
        else:
            packet = SephTag.FINGER_EVENT_RELEASE.to_bytes(1, 'big')
        packet += x.to_bytes(2, 'big')
        packet += y.to_bytes(2, 'big')

        self.packet_thread.queue_packet(SephTag.FINGER_EVENT, packet)

    def handle_wait(self, delay: float):
        '''Wait for a specified delay, taking account real time seen by the app.'''
        start = self.packet_thread.get_processed_ticks_count()
        while (self.packet_thread.get_processed_ticks_count() - start) * TICKER_DELAY < delay:
            time.sleep(TICKER_DELAY)

    def to_app(self, packet: bytes):
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
