import select
from typing import List, Optional

from speculos.observer import TextEvent
from . import bagl
from . import nbgl
from .display import Display, DisplayNotifier, FrameBuffer, GraphicLibrary, MODELS
from .readerror import ReadError
from .struct import DisplayArgs, ServerArgs
from .vnc import VNC


class Headless(Display):
    def __init__(self, display: DisplayArgs, server: ServerArgs) -> None:
        super().__init__(display, server)

        self.m = HeadlessPaintWidget(self.model, server.vnc)
        self._bagl_gl: GraphicLibrary
        self._nbgl_gl: GraphicLibrary
        self._bagl_gl = bagl.Bagl(self.m, MODELS[self.model].screen_size, self.model)
        self._nbgl_gl = nbgl.NBGL(self.m, MODELS[self.model].screen_size, self.model)

    @property
    def bagl_gl(self):
        return self._bagl_gl

    @property
    def nbgl_gl(self):
        return self._nbgl_gl

    def display_status(self, data: bytes) -> List[TextEvent]:
        ret = self.bagl_gl.display_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work
        return ret

    def display_raw_status(self, data: bytes) -> None:
        self.bagl_gl.display_raw_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work

    def screen_update(self) -> bool:
        return self.bagl_gl.refresh()


class HeadlessPaintWidget(FrameBuffer):
    def __init__(self, model: str, vnc: Optional[VNC] = None):
        super().__init__(model)
        self.vnc = vnc

    def update(self,
               _0: Optional[int] = None,
               _1: Optional[int] = None,
               _2: Optional[int] = None,
               _3: Optional[int] = None) -> bool:
        if self.pixels or self.draw_default_color:
            self._redraw()
            self.pixels = {}
            self.draw_default_color = False
            return True
        return False

    def _redraw(self) -> None:
        if self.vnc:
            self.vnc.redraw(self.pixels, self.default_color)
        self.update_screenshot()


class HeadlessNotifier(DisplayNotifier):

    def __init__(self, display_args: DisplayArgs, server_args: ServerArgs) -> None:
        super().__init__(display_args, server_args)
        self._set_display_class(Headless)

    def run(self) -> None:
        while True:
            _rlist = self.notifiers.keys()
            if not _rlist:
                break

            rlist, _, _ = select.select(_rlist, [], [])
            try:
                for fd in rlist:
                    self.notifiers[fd].can_read(self)

            # This exception occur when can_read have no more data available
            except ReadError:
                break
