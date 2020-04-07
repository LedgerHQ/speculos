import select

from . import bagl
from .display import Display, FrameBuffer, MODELS, RENDER_METHOD

class HeadlessPaintWidget(FrameBuffer):
    def __init__(self, parent, model, vnc=None):
        super().__init__(model)
        self.vnc = vnc

    def update(self):
        if self.pixels:
            self._redraw()
            self.pixels = {}

    def _redraw(self):
        if self.vnc:
            self.vnc.redraw(self.pixels)

class Headless(Display):
    def __init__(self, apdu, seph, button_tcp=None, finger_tcp=None, model='nanos', rendering=RENDER_METHOD.FLUSHED, vnc=None, **_):
        super().__init__(apdu, seph, model, rendering)
        self._init_notifiers(apdu, seph, button_tcp, finger_tcp, vnc)

        m = HeadlessPaintWidget(self, self.model, vnc)
        self.bagl = bagl.Bagl(m, MODELS[self.model].screen_size)

    def display_status(self, data):
        ret = self.bagl.display_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work
        return ret

    def display_raw_status(self, data):
        self.bagl.display_raw_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work

    def screen_update(self):
        self.bagl.refresh()

    def run(self):
        while True:
            rlist = self.notifiers.keys()
            if not rlist:
                break

            rlist, _, _ = select.select(rlist, [], [])
            for fd in rlist:
                self.notifiers[fd].can_read(fd, self)
