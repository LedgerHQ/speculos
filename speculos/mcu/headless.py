import select
from typing import Optional

from . import bagl
from . import nbgl
from .display import Display, DisplayArgs, FrameBuffer, Model, MODELS, ServerArgs
from .readerror import ReadError
from .vnc import VNC


class Headless(Display):
    def __init__(self, display: DisplayArgs, server: ServerArgs) -> None:
        super().__init__(display, server)
        self._init_notifiers(server)

        self.m = HeadlessPaintWidget(self, self.model, server.vnc)
        if display.model != "stax":
            self.bagl = bagl.Bagl(self.m, MODELS[self.model].screen_size)
        else:
            self.nbgl = nbgl.NBGL(self.m, MODELS[self.model].screen_size, display.force_full_ocr,
                                  display.disable_tesseract)

    def display_status(self, data):
        ret = self.bagl.display_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work
        return ret

    def display_raw_status(self, data):
        self.bagl.display_raw_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work

    def screen_update(self) -> bool:
        return self.bagl.refresh()

    def run(self):
        while True:
            rlist = self.notifiers.keys()
            if not rlist:
                break

            rlist, _, _ = select.select(rlist, [], [])
            try:
                for fd in rlist:
                    self.notifiers[fd].can_read(fd, self)

            # This exception occur when can_read have no more data available
            except ReadError:
                break


class HeadlessPaintWidget(FrameBuffer):
    def __init__(self, parent: Headless, model: Model, vnc: Optional[VNC] = None) -> None:
        super().__init__(model)
        self.vnc = vnc

    def update(self, x=None, y=None, w=None, h=None):
        if self.pixels:
            self._redraw()
            self.pixels = {}
            return True
        return False

    def _redraw(self):
        if self.vnc:
            self.vnc.redraw(self.pixels)

        self.screenshot_update_pixels()

    def update_screenshot(self):
        return self.screenshot_update_pixels()
