import io
from abc import ABC, abstractmethod
from collections import namedtuple
from typing import Dict, Union
from PIL import Image

from .apdu import ApduServer
from .seproxyhal import SeProxyHal
from .button_tcp import FakeButton
from .finger_tcp import FakeFinger
from .vnc import VNC

Server = Union[ApduServer, FakeButton, FakeFinger, SeProxyHal, VNC]

DisplayArgs = namedtuple("DisplayArgs", "color model ontop rendering keymap pixel_size x y force_full_ocr, \
        disable_tesseract")
ServerArgs = namedtuple("ServerArgs", "apdu apirun button finger seph vnc")

Model = namedtuple('Model', 'name screen_size box_position box_size')
MODELS = {
    'nanos': Model('Nano S', (128, 32), (20, 13), (100, 26)),
    'nanox': Model('Nano X', (128, 64), (5, 5), (10, 10)),
    'nanosp': Model('Nano SP', (128, 64), (5, 5), (10, 10)),
    'blue': Model('Blue', (320, 480), (13, 13), (26, 26)),
    'stax': Model('Stax', (400, 672), (13, 13), (26, 26)),
}

COLORS = {
    'LAGOON_BLUE': 0x7ebab5,
    'JADE_GREEN': 0xb9ceac,
    'FLAMINGO_PINK': 0xd8a0a6,
    'SAFFRON_YELLOW': 0xf6a950,
    'MATTE_BLACK': 0x111111,

    'CHARLOTTE_PINK': 0xff5555,
    'ARNAUD_GREEN': 0x79ff79,
    'SYLVE_CYAN': 0x29f3f3,
}


def _screenshot_to_iobytes_value(screen_size, data):
    image = Image.frombytes("RGB", screen_size, data)
    iobytes = io.BytesIO()
    image.save(iobytes, format="PNG")
    return iobytes.getvalue()


class Screenshot:
    def __init__(self, screen_size):
        self.pixels = {}
        self.width, self.height = screen_size
        for y in range(0, self.height):
            for x in range(0, self.width):
                self.pixels[(x, y)] = 0x000000

    def update(self, pixels):
        # Don't call update, replace the object instead
        self.pixels = {**self.pixels, **pixels}

    def get_image(self):
        # Get the pixels object once, as it may be replaced during the loop.
        pixels = self.pixels
        data = bytearray(self.width * self.height * 3)
        for y in range(0, self.height):
            for x in range(0, self.width):
                pos = 3 * (y * self.width + x)
                data[pos:pos + 3] = pixels[(x, y)].to_bytes(3, "big")
        return (self.width, self.height), bytes(data)


class FrameBuffer(ABC):
    COLORS = {
        "nanos": 0x00fffb,
        "nanox": 0xdddddd,
        "nanosp": 0xdddddd,
        "stax": 0xdddddd,
    }

    def __init__(self, model):
        self.pixels = {}
        self.model = model
        self.screenshot = Screenshot(MODELS[model].screen_size)
        # Init published content now, don't wait for the first request
        if self.model == "stax":
            self.update_public_screenshot()

    def draw_point(self, x, y, color):
        # There are only 2 colors on the Nano S and the Nano X but the one
        # passed in argument isn't always valid. Fix it here.
        if self.model != 'stax':
            if color != 0x000000:
                color = FrameBuffer.COLORS.get(self.model, color)
        self.pixels[(x, y)] = color

    def screenshot_update_pixels(self):
        # Update the screenshot object with our current pixels content
        self.screenshot.update(self.pixels)

    def take_screenshot(self):
        self.current_screen_size, self.current_data = self.screenshot.get_image()
        return self.current_screen_size, self.current_data

    def update_public_screenshot(self):
        # Stax only
        # As we lazyly evaluate the published screenshot, we only flag the evaluation update as necessary
        self.recreate_public_screenshot = True

    @property
    def public_screenshot_value(self):
        # Stax only
        # Lazy calculation of the published screenshot, as it is a costly operation
        # and not necessary if no one tries to read the value
        if self.recreate_public_screenshot:
            self.recreate_public_screenshot = False
            self._public_screenshot_value = _screenshot_to_iobytes_value(self.current_screen_size, self.current_data)

        return self._public_screenshot_value

    def get_public_screenshot(self):
        if self.model == "stax":
            # On Stax, we only make the screenshot public on the RESTFUL api when it is consistent with events
            # On top of this, take_screenshot is time consuming on stax, so we'll do as few as possible
            # We return the value calculated last time update_public_screenshot was called
            return self.public_screenshot_value
        else:
            # On nano we have no knowledge of screen refreshes so we can't be scarce on publishes
            # So we publish the raw current content every time. It's ok as take_screenshot is fast on Nano
            screen_size, data = self.take_screenshot()
            return _screenshot_to_iobytes_value(screen_size, data)


class Display(ABC):
    def __init__(self, display: DisplayArgs, server: ServerArgs) -> None:
        self.notifiers: Dict[int, Server] = {}
        self.apdu = server.apdu
        self.seph = server.seph
        self.model = display.model
        self.force_full_ocr = display.force_full_ocr
        self.disable_tesseract = display.disable_tesseract
        self.rendering = display.rendering

    @abstractmethod
    def display_status(self, data):
        pass

    @abstractmethod
    def display_raw_status(self, data):
        pass

    @abstractmethod
    def screen_update(self) -> bool:
        pass

    def add_notifier(self, klass):
        assert klass.s.fileno() not in self.notifiers
        self.notifiers[klass.s.fileno()] = klass

    def remove_notifier(self, fd):
        self.notifiers.pop(fd)

    def _init_notifiers(self, args: ServerArgs) -> None:
        for klass in args._asdict().values():
            if klass:
                self.add_notifier(klass)

    def forward_to_app(self, packet):
        self.seph.to_app(packet)

    def forward_to_apdu_client(self, packet):
        self.apdu.forward_to_client(packet)
