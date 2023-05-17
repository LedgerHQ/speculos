import select
from typing import List, Optional

from speculos.abstractions import Display, DisplayArgs, GraphicLibrary, ServerArgs
from . import bagl
from . import nbgl
from .display import FrameBuffer, MODELS
from .readerror import ReadError
from .vnc import VNC


class Headless(Display):
    def __init__(self, display: DisplayArgs, server: ServerArgs) -> None:
        super().__init__(display, server)
        self._init_notifiers(server)

        self.m = HeadlessPaintWidget(self.model, server.vnc)
        self._gl: GraphicLibrary
        if display.model != "stax":
            self._gl = bagl.Bagl(self.m, MODELS[self.model].screen_size, self.model)
        else:
            self._gl = nbgl.NBGL(self.m, MODELS[self.model].screen_size, self.model, display.force_full_ocr)

    @property
    def gl(self) -> GraphicLibrary:
        return self._gl

    def display_status(self, data: bytes) -> List[TextEvent]:
        assert isinstance(self.gl, bagl.Bagl)
        ret = self.gl.display_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work
        return ret

    def display_raw_status(self, data: bytes) -> None:
        assert isinstance(self.gl, bagl.Bagl)
        self.gl.display_raw_status(data)
        if MODELS[self.model].name == 'blue':
            self.screen_update()    # Actually, this method doesn't work

    def screen_update(self) -> bool:
        assert isinstance(self.gl, bagl.Bagl)
        return self.gl.refresh()

    def run(self) -> None:
        while True:
            _rlist = self.notifiers.keys()
            if not _rlist:
                break

            rlist, _, _ = select.select(_rlist, [], [])
            try:
                for fd in rlist:
                    self.notifiers[fd].can_read(fd, self)

            # This exception occur when can_read have no more data available
            except ReadError:
                break


class HeadlessPaintWidget(FrameBuffer):
    def __init__(self, model: str, vnc: Optional[VNC] = None):
        super().__init__(model)
        self.vnc = vnc

    def update(self,
               _0: Optional[int] = None,
               _1: Optional[int] = None,
               _2: Optional[int] = None,
               _3: Optional[int] = None) -> bool:
        if self.pixels:
            self._redraw()
            self.pixels = {}
            return True
        return False

    def _redraw(self) -> None:
        if self.vnc:
            self.vnc.redraw(self.pixels)
        self.update_screenshot()
