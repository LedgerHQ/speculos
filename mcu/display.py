from abc import ABC, abstractmethod
from collections import namedtuple
from typing import Dict, Union

from .apdu import ApduServer
from .seproxyhal import SeProxyHal
from .button_tcp import FakeButton
from .finger_tcp import FakeFinger
from .vnc import VNC

Server = Union[ApduServer, FakeButton, FakeFinger, SeProxyHal, VNC]

DisplayArgs = namedtuple("DisplayArgs", "color model ontop rendering keymap pixel_size record_frames")
ServerArgs = namedtuple("ServerArgs", "apdu button finger seph vnc")

Model = namedtuple('Model', 'name screen_size box_position box_size')
MODELS = {
    'nanos': Model('Nano S', (128, 32), (20, 13), (100, 26)),
    'nanox': Model('Nano X', (128, 64), (5, 5), (10, 10)),
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


class FrameBuffer(ABC):
    COLORS = {
        "nanos": 0x00fffb,
        "nanox": 0xdddddd,
    }

    def __init__(self, model):
        self.pixels = {}
        self.model = model

    def draw_point(self, x, y, color):
        # There are only 2 colors on the Nano S and the Nano X but the one
        # passed in argument isn't always valid. Fix it here.
        if color != 0x000000:
            color = FrameBuffer.COLORS.get(self.model, color)
        self.pixels[(x, y)] = color

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
    def screen_update(self):
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
