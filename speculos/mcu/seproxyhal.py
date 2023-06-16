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
from .readerror import ReadError
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

    DBG_SCREEN_DISPLAY_STATUS = 0x5e
    PRINTC_STATUS = 0x5f

    GENERAL_STATUS = 0x60
    GENERAL_STATUS_LAST_COMMAND = 0x0000
    SCREEN_DISPLAY_STATUS = 0x65
    PRINTF_STATUS = 0x66
    SCREEN_DISPLAY_RAW_STATUS = 0x69

    FINGER_EVENT_TOUCH = 0x01
    FINGER_EVENT_RELEASE = 0x02

    # Speculos only, defined in speculos/src/bolos/nbgl.c
    NBGL_DRAW_RECT = 0xFA
    NBGL_REFRESH = 0xFB
    NBGL_DRAW_LINE = 0xFC
    NBGL_DRAW_IMAGE = 0xFD
    NBGL_DRAW_IMAGE_FILE = 0xFE


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


class SocketHelper(threading.Thread):
    def __init__(self, s: socket.socket, status_event: threading.Event, *args, **kwargs):
        super().__init__(name="packet", daemon=True)
        self.s = s
        self.sending_lock = threading.Lock()
        self.queue_condition = threading.Condition()
        self.queue: List[Tuple[SephTag, bytes]] = []
        self.status_event = status_event
        self.logger = logging.getLogger("seproxyhal.packet")
        self.stop = False
        self.ticks_count = 0
        self.tick_requested = False

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

    def read_packet(self):
        data = self._recvall(3)
        if data is None:
            return None, None, None

        tag = data[0]
        size = int.from_bytes(data[1:3], 'big')

        data = self._recvall(size)
        if data is None:
            return None, None, None
        assert len(data) == size

        return tag, size, data

    def send_packet(self, tag: SephTag, data: bytes = b''):
        """Send a packet to the app."""

        size: bytes = len(data).to_bytes(2, 'big')
        packet: bytes = tag.to_bytes(1, 'big') + size + data

        with self.sending_lock:
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
        """Request sending of a ticker event to the app"""
        self.tick_requested = True

        # notify this thread that a new event is available
        with self.queue_condition:
            self.queue_condition.notify()

    def run(self):
        while not self.stop:

            # wait for a event in the queue or a tick to be available
            with self.queue_condition:
                while len(self.queue) == 0 and not self.tick_requested:
                    self.queue_condition.wait()

            # wait for a status notification
            while not self.status_event.is_set():
                self.status_event.wait()
            self.status_event.clear()

            if len(self.queue):
                tag, data = self.queue.pop(0)
            elif self.tick_requested:
                tag, data = SephTag.TICKER_EVENT, b''
            else:
                raise RuntimeError("Unexpected state: no ticker nor event to send on socket")

            self.send_packet(tag, data)

            if tag == SephTag.TICKER_EVENT:
                self.ticks_count += 1
                self.tick_requested = False

        self.logger.debug("exiting")

    def get_processed_ticks_count(self):
        return self.ticks_count


class SeProxyHal:
    def __init__(self,
                 s: socket.socket,
                 automation: Optional[Automation] = None,
                 automation_server: Optional[AutomationServer] = None,
                 transport: str = 'hid',
                 fonts_path: str = None,
                 api_level=None):
        self.s = s
        self.logger = logging.getLogger("seproxyhal")
        self.printf_queue = ''
        self.automation = automation
        self.automation_server = automation_server
        self.events: List[TextEvent] = []
        self.refreshed = False

        self.status_event = threading.Event()
        self.socket_helper = SocketHelper(self.s, self.status_event)
        self.socket_helper.start()

        self.time_ticker_thread = TimeTickerDaemon(self.socket_helper.add_tick)
        self.time_ticker_thread.start()

        self.usb = usb.USB(self.socket_helper.queue_packet, transport=transport)

        self.ocr = OCR(fonts_path, api_level)

        # A list of callback methods when an APDU response is received
        self.apdu_callbacks: List[Callable] = []

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

        tag, size, data = self.socket_helper.read_packet()
        if data is None:
            self._close(s, screen)
            raise ReadError("fd closed")

        self.logger.debug(f"received (tag: {tag:#04x}, size: {size:#04x}): {data!r}")

        if tag == SephTag.GENERAL_STATUS:
            if int.from_bytes(data[:2], 'big') == SephTag.GENERAL_STATUS_LAST_COMMAND:
                if self.refreshed:
                    self.refreshed = False

                    if not screen.nbgl.disable_tesseract:
                        # Run the OCR
                        screen.nbgl.m.update_screenshot()
                        screen_size, image_data = screen.nbgl.m.take_screenshot()
                        self.ocr.analyze_image(screen_size, image_data)

                        # Publish the new screenshot, we'll upload its associated events shortly
                        screen.nbgl.m.update_public_screenshot()

                if screen.model != "stax" and screen.screen_update():
                    if screen.model in ["nanox", "nanosp"]:
                        self.events += self.ocr.get_events()
                elif screen.model == "stax":
                    self.events += self.ocr.get_events()

                # Apply automation rules after having received a GENERAL_STATUS_LAST_COMMAND tag. It allows the
                # screen to be updated before broadcasting the events.
                if self.events:
                    self.apply_automation()

                # signal the sending thread that a status has been received
                self.status_event.set()

            else:
                self.logger.error(f"unknown subtag: {data[:2]!r}")
                sys.exit(0)

        elif tag == SephTag.SCREEN_DISPLAY_STATUS or tag == SephTag.DBG_SCREEN_DISPLAY_STATUS:
            self.logger.debug(f"DISPLAY_STATUS {data!r}")
            events = screen.display_status(data)
            if events:
                self.events += events
            self.socket_helper.send_packet(SephTag.DISPLAY_PROCESSED_EVENT)

        elif tag == SephTag.SCREEN_DISPLAY_RAW_STATUS:
            self.logger.debug("SephTag.SCREEN_DISPLAY_RAW_STATUS")
            screen.display_raw_status(data)
            if screen.model in ["nanox", "nanosp"]:
                self.ocr.analyze_bitmap(data)
            self.socket_helper.send_packet(SephTag.DISPLAY_PROCESSED_EVENT)
            if screen.rendering == RENDER_METHOD.PROGRESSIVE:
                screen.screen_update()

        elif tag == SephTag.PRINTF_STATUS or tag == SephTag.PRINTC_STATUS:
            for b in [chr(b) for b in data]:
                if b == '\n':
                    self.logger.info(f"printf: {self.printf_queue}")
                    self.printf_queue = ''
                else:
                    self.printf_queue += b
            if screen.model in ["blue"]:
                self.socket_helper.send_packet(SephTag.DISPLAY_PROCESSED_EVENT)

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

        elif tag == SephTag.NBGL_DRAW_RECT:
            screen.nbgl.hal_draw_rect(data)

        elif tag == SephTag.NBGL_REFRESH:
            screen.nbgl.hal_refresh(data)
            # Stax only
            # We have refreshed the screen, remember it for the next time we have SephTag.GENERAL_STATUS
            # then we'll perform a new OCR and make public the resulting screenshot / OCR analysis
            self.refreshed = True

        elif tag == SephTag.NBGL_DRAW_LINE:
            screen.nbgl.hal_draw_line(data)

        elif tag == SephTag.NBGL_DRAW_IMAGE:
            screen.nbgl.hal_draw_image(data)

        elif tag == SephTag.NBGL_DRAW_IMAGE_FILE:
            screen.nbgl.hal_draw_image_file(data)

        else:
            self.logger.error(f"unknown tag: {tag:#x}")
            sys.exit(0)

    def handle_button(self, button: int, pressed: bool):
        '''Forward button press/release from the GUI to the app.'''

        if pressed:
            self.socket_helper.queue_packet(SephTag.BUTTON_PUSH_EVENT, (button << 1).to_bytes(1, 'big'))
        else:
            self.socket_helper.queue_packet(SephTag.BUTTON_PUSH_EVENT, (0 << 1).to_bytes(1, 'big'))

    def handle_finger(self, x: int, y: int, pressed: bool):
        '''Forward finger press/release from the GUI to the app.'''

        if pressed:
            packet = SephTag.FINGER_EVENT_TOUCH.to_bytes(1, 'big')
        else:
            packet = SephTag.FINGER_EVENT_RELEASE.to_bytes(1, 'big')
        packet += x.to_bytes(2, 'big')
        packet += y.to_bytes(2, 'big')

        self.socket_helper.queue_packet(SephTag.FINGER_EVENT, packet)

    def handle_wait(self, delay: float):
        '''Wait for a specified delay, taking account real time seen by the app.'''
        start = self.socket_helper.get_processed_ticks_count()
        while (self.socket_helper.get_processed_ticks_count() - start) * TICKER_DELAY < delay:
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
            self.socket_helper.queue_packet(tag, packet)
        else:
            self.usb.xfer(packet)
