import binascii
import os
import select
import sys
import time
import threading
from enum import IntEnum

from . import apdu
from . import usb
from .display import RENDER_METHOD

class SephTag(IntEnum):
    BUTTON_PUSH_EVENT           = 0x05
    FINGER_EVENT                = 0x0c
    DISPLAY_PROCESSED_EVENT     = 0x0d
    TICKER_EVENT                = 0x0e
    CAPDU_EVENT                 = 0x16

    MCU                         = 0x31
    USB_CONFIG                  = 0x4f
    USB_EP_PREPARE              = 0x50

    RAPDU                       = 0x53
    GENERAL_STATUS              = 0x60
    GENERAL_STATUS_LAST_COMMAND = 0x0000
    SCREEN_DISPLAY_STATUS       = 0x65
    PRINTF_STATUS               = 0x66
    SCREEN_DISPLAY_RAW_STATUS   = 0x69

    FINGER_EVENT_TOUCH          = 0x01
    FINGER_EVENT_RELEASE        = 0x02

TICKER_DELAY = 0.1

class ServiceExit(Exception):
    '''
    Custom exception which is used to trigger the clean exit of all running
    threads and the main program.
    '''

class DeferedTicketEventSender(threading.Thread):
    def __init__(self, parent, *args, **kwargs):
        super(DeferedTicketEventSender, self).__init__(*args, **kwargs)
        self.parent = parent

    def run(self):
        # print(f"[*] sleeping for {self.parent.sleep_time}")
        time.sleep(self.parent.sleep_time)
        self.parent._send_ticker_event()

class SeProxyHal:
    def __init__(self, s):
        self.s = s
        self.queue = []
        self.last_ticker_sent_at = 0.0
        self.sending_ticker_event = threading.Lock()
        self.status_received = True
        self.usb = usb.USB(self._queue_event_packet)

    def _recvall(self, size):
        data = b''
        while size > 0:
            tmp = self.s.recv(size)
            if len(tmp) == 0:
                print('[-] seproxyhal: fd closed', file=sys.stderr)
                sys.exit(1)
            data += tmp
            size -= len(tmp)
        return data

    def _send_packet(self, tag, data=b''):
        '''Send packet to the app.'''

        size = len(data).to_bytes(2, 'big')
        packet = tag.to_bytes(1, 'big') + size + data
        #print('[*] seproxyhal: send %s' % binascii.hexlify(packet), file=sys.stderr)
        try:
            self.s.sendall(packet)
        except BrokenPipeError:
            # the pipe is closed, which means the app exited
            raise ServiceExit

    def _send_ticker_event(self):
        self.sending_ticker_event.release()
        self.last_ticker_sent_at = time.time()
        # don't send an event if a command is being processed
        if self.status_received:
            self._send_packet(SephTag.TICKER_EVENT)

    def send_ticker_event_defered(self):
        if self.sending_ticker_event.acquire(False):
            self.sleep_time = TICKER_DELAY - (time.time() - self.last_ticker_sent_at)
            if self.sleep_time > 0:
                DeferedTicketEventSender(self, name = "DeferedTicketEvent").start()
            else:
                self._send_ticker_event()
                self.status_received = False

    def send_next_event(self):
        # don't send an event if a command is being processed
        if not self.status_received:
            return

        if len(self.queue) > 0:
            # if we've received events from the outside world, send them
            # whenever the SE is ready (i.e. now as we've received a
            # GENERAL_STATUS)
            tag, data = self.queue.pop(0)
            self._send_packet(tag, data)
            self.status_received = False
        else:
            # if no real event available in the queue, send a ticker event
            # every 100ms
            self.send_ticker_event_defered()

    def can_read(self, s, screen):
        '''
        Handle packet sent by the app.

        This function is called thanks to a screen QSocketNotifier.
        '''

        assert s == self.s.fileno()

        data = self._recvall(3)
        tag = data[0]
        size = int.from_bytes(data[1:3], 'big')

        data = self._recvall(size)
        assert len(data) == size

        #print('[*] seproxyhal: received (tag: 0x%02x, size: 0x%02x): %s' % (tag, size, repr(data)), file=sys.stderr)

        if tag & 0xf0 == SephTag.GENERAL_STATUS:
            self.status_received = True

            if tag == SephTag.GENERAL_STATUS:
                if int.from_bytes(data[:2], 'big') == SephTag.GENERAL_STATUS_LAST_COMMAND:
                    screen.screen_update()

            elif tag == SephTag.SCREEN_DISPLAY_STATUS:
                #print('[*] seproxyhal: DISPLAY_STATUS %s' % repr(data), file=sys.stderr)
                screen.display_status(data)
                self._queue_event_packet(SephTag.DISPLAY_PROCESSED_EVENT)

            elif tag == SephTag.SCREEN_DISPLAY_RAW_STATUS:
                #print('SephTag.SCREEN_DISPLAY_RAW_STATUS', file=sys.stderr)
                screen.display_raw_status(data)
                self._queue_event_packet(SephTag.DISPLAY_PROCESSED_EVENT)
                if screen.rendering == RENDER_METHOD.PROGRESSIVE:
                    screen.screen_update()

            elif tag == SephTag.PRINTF_STATUS:
                for b in data:
                    sys.stdout.write('%c' % chr(b))
                sys.stdout.flush()
                self._queue_event_packet(SephTag.DISPLAY_PROCESSED_EVENT)
                if screen.rendering == RENDER_METHOD.PROGRESSIVE:
                    screen.screen_update()

            else:
                print('unknown tag: 0x%x' % tag)
                sys.exit(0)

            self.send_next_event()

        elif tag == SephTag.RAPDU:
            screen.forward_to_apdu_client(data)

        elif tag == SephTag.USB_CONFIG:
            self.usb.config(data)

        elif tag == SephTag.USB_EP_PREPARE:
            data = self.usb.prepare(data)
            if data:
                screen.forward_to_apdu_client(data)

        elif tag == SephTag.MCU:
            pass

        else:
            print('unknown tag: 0x%x' % tag)
            sys.exit(0)

    def _queue_event_packet(self, tag, data=b''):
        self.queue.append((tag, data))
        self.send_next_event()

    def handle_button(self, button, pressed):
        '''Forward button press/release from the GUI to the app.'''

        if pressed:
            self._queue_event_packet(SephTag.BUTTON_PUSH_EVENT, (button << 1).to_bytes(1, 'big'))
        else:
            self._queue_event_packet(SephTag.BUTTON_PUSH_EVENT, (0 << 1).to_bytes(1, 'big'))

    def handle_finger(self, x, y, pressed):
        '''Forward finger press/release from the GUI to the app.'''

        if pressed:
            packet = SephTag.FINGER_EVENT_TOUCH.to_bytes(1, 'big')
        else:
            packet = SephTag.FINGER_EVENT_RELEASE.to_bytes(1, 'big')
        packet += x.to_bytes(2, 'big')
        packet += y.to_bytes(2, 'big')

        self._queue_event_packet(SephTag.FINGER_EVENT, packet)

    def to_app(self, packet):
        '''
        Forward raw APDU to the app.

        Packets can be forwarded directly to the SE thanks to
        SephTag.CAPDU_EVENT, but it doesn't work with messages are larger
        than G_io_seproxyhal_spi_buffer. Emulating a basic USB stack is more
        reliable, see issue #11.
        '''

        self.usb.xfer(packet)
