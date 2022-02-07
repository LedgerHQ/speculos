from abc import ABC, abstractmethod
from collections import namedtuple
from typing import Dict, Union

from .apdu import ApduServer
from .seproxyhal import SeProxyHal
from .button_tcp import FakeButton
from .finger_tcp import FakeFinger
from .vnc import VNC

Server = Union[ApduServer, FakeButton, FakeFinger, SeProxyHal, VNC]

DisplayArgs = namedtuple("DisplayArgs", "color model ontop rendering keymap pixel_size x y")
ServerArgs = namedtuple("ServerArgs", "apdu apirun button finger seph vnc")

Model = namedtuple('Model', 'name screen_size box_position box_size')
MODELS = {
    'nanos': Model('Nano S', (128, 32), (20, 13), (100, 26)),
    'nanox': Model('Nano X', (128, 64), (5, 5), (10, 10)),
    'nanosp': Model('Nano SP', (128, 64), (5, 5), (10, 10)),
    'blue': Model('Blue', (320, 480), (13, 13), (26, 26)),
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
    }

    def __init__(self, model):
        self.pixels = {}
        self.model = model
        self.screenshot = Screenshot(MODELS[model].screen_size)

    def draw_point(self, x, y, color):
        # There are only 2 colors on the Nano S and the Nano X but the one
        # passed in argument isn't always valid. Fix it here.
        if color != 0x000000:
            color = FrameBuffer.COLORS.get(self.model, color)
        self.pixels[(x, y)] = color

    def screenshot_update_pixels(self):
        self.screenshot.update(self.pixels)

    def take_screenshot(self):
        return self.screenshot.get_image()


class Display(ABC):
    def __init__(self, display: DisplayArgs, server: ServerArgs) -> None:
        self.notifiers: Dict[int, Server] = {}
        self.apdu = server.apdu
        self.seph = server.seph
        self.model = display.model
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
