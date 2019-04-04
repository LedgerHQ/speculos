import select

from . import bagl
from .display import Display, MODELS, RENDER_METHOD

class HeadlessPaintWidget():
    def __init__(self, parent, model, vnc=None):
        self.pixels = {}
        self.model = model
        self.vnc = vnc

    def update(self):
        if self.pixels:
            self._redraw()
            self.pixels = {}

    def _redraw(self):
        if self.vnc:
            self.vnc.redraw(self.pixels)

    def draw_point(self, x, y, color):
        # There are only 2 colors on Nano S but the one passed in argument isn't
        # always valid. Fix it here.
        if self.model == 'nanos' and color != 0x000000:
            color = 0x00fffb
        self.pixels[(x, y)] = color

class Headless(Display):
    def __init__(self, apdu, seph, button_tcp, model, rendering, vnc):
        self.apdu = apdu
        self.seph = seph
        self.model = model
        self.rendering = rendering
        self.notifiers = {}

        self._init_notifiers([ apdu, seph ])
        if button_tcp:
            self.add_notifier(button_tcp)
        if vnc:
            self.add_notifier(vnc)

        width, height = MODELS[self.model].screen_size
        m = HeadlessPaintWidget(self, self.model, vnc)
        self.bagl = bagl.Bagl(m, width, height)

    def _init_notifiers(self, classes):
        for klass in classes:
            self.add_notifier(klass)

    def forward_to_app(self, packet):
        self.seph.to_app(packet)

    def forward_to_apdu_client(self, packet):
        self.apdu.forward_to_client(packet)

    def display_status(self, data):
        self.bagl.display_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work

    def display_raw_status(self, data):
        self.bagl.display_raw_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work

    def screen_update(self):
        self.bagl.refresh()

    def add_notifier(self, klass):
        assert klass.s.fileno() not in self.notifiers
        self.notifiers[klass.s.fileno()] = klass

    def remove_notifier(self, fd):
        n = self.notifiers.pop(fd)
        del n

    def run(self):
        while True:
            rlist = self.notifiers.keys()
            if not rlist:
                break

            rlist, _, _ = select.select(rlist, [], [])
            for fd in rlist:
                self.notifiers[fd].can_read(fd, self)

def display(apdu, seph, button_tcp=None, model='nanos', rendering=RENDER_METHOD.FLUSHED, vnc=None, **_):
    headless = Headless(apdu, seph, button_tcp, model, rendering, vnc)
    headless.run()
