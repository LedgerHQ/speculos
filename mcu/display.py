from abc import ABC, abstractmethod
from collections import namedtuple

Model = namedtuple('Model', 'name screen_size box_position box_size')
MODELS = {
    'nanos': Model('Nano S', (128, 32), (20, 13), (100, 26)),
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

RenderMethods = namedtuple('RenderMethods', 'PROGRESSIVE FLUSHED')
RENDER_METHOD = RenderMethods(0, 1)

class FrameBuffer(ABC):
    def __init__(self, model):
        self.pixels = {}
        self.model = model

    def draw_point(self, x, y, color):
        # There are only 2 colors on Nano S but the one passed in argument isn't
        # always valid. Fix it here.
        if self.model == 'nanos' and color != 0x000000:
            color = 0x00fffb
        self.pixels[(x, y)] = color

class Display(ABC):
    def __init__(self, apdu, seph, model, rendering):
        self.notifiers = {}
        self.apdu = apdu
        self.seph = seph
        self.model = model
        self.rendering = rendering

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
        n = self.notifiers.pop(fd)
        del n

    def _init_notifiers(self, *classes):
        for klass in classes:
            if klass:
                self.add_notifier(klass)

    def forward_to_app(self, packet):
        self.seph.to_app(packet)

    def forward_to_apdu_client(self, packet):
        self.apdu.forward_to_client(packet)
