from __future__ import annotations

import io
from abc import ABC, abstractmethod
try:
    from functools import cache
except ImportError:
    # `functools.cache` does not exists on Python3.8
    from functools import lru_cache
    cache = lru_cache(maxsize=None)
from PIL import Image
from socket import socket
from threading import Lock
from typing import Any, Dict, IO, List, Optional, Tuple, Union

from speculos.observer import TextEvent
from .struct import DisplayArgs, MODELS, Pixel, ServerArgs

PixelColorMapping = Dict[Pixel, int]


class IODevice(ABC):
    """
    An interface every class implementing application IOs (screens, buttons, APDUs, ...) should
    inherit from.

    These classes will be managed by a `DisplayNotifier`, which is responsible to redirect events
    to the correct `IODevice` instance through its `IODevice.can_read` callback.
    """

    @property
    @abstractmethod
    def file(self) -> Union[IO[bytes], socket]:
        """
        Returns the file (Pipe, Socket, ...) tied to this `IODevice`
        """
        raise NotImplementedError()

    @property
    def fileno(self) -> int:
        """
        Returns the file descriptor of the file tied to this `IODevice`
        """
        return self.file.fileno()

    @abstractmethod
    def can_read(self, screen: DisplayNotifier) -> None:
        """
        Callback used by a notifier to trigger `IODevice` events on given screen
        """
        pass


COLORS: Dict[str, int] = {
    'LAGOON_BLUE': 0x7ebab5,
    'JADE_GREEN': 0xb9ceac,
    'FLAMINGO_PINK': 0xd8a0a6,
    'SAFFRON_YELLOW': 0xf6a950,
    'MATTE_BLACK': 0x111111,
    'CHARLOTTE_PINK': 0xff5555,
    'ARNAUD_GREEN': 0x79ff79,
    'SYLVE_CYAN': 0x29f3f3,
}


class FrameBuffer:
    """
    A class responsible for managing the graphic screen of the current application.

    It updates the screen, takes screenshots, manages colors and such.
    """

    COLORS = {
        "nanos": 0x00fffb,
        "nanox": 0xdddddd,
        "nanosp": 0xdddddd,
        "stax": 0xdddddd,
        "flex": 0xdddddd,
    }

    def __init__(self, model: str):
        self.pixels: PixelColorMapping = {}
        self.screenshot_pixels: PixelColorMapping = {}
        self.screenshot_pixels_lock = Lock()
        self.default_color = 0
        self.draw_default_color = False
        self.reset_screeshot_pixels = False
        self._public_screenshot_value = b''
        self.current_data = b''
        self.recreate_public_screenshot = True
        self.model = model
        self.current_screen_size = MODELS[model].screen_size
        self._width, self._height = MODELS[model].screen_size

    @cache
    def check_color(self, color: int) -> int:
        # There are only 2 colors on the Nano S and the Nano X but the one
        # passed in argument isn't always valid. Fix it here.
        if self.model != 'stax' and self.model != 'flex':
            if color != 0x000000:
                color = FrameBuffer.COLORS.get(self.model, color)
        return color

    def draw_point(self, x: int, y: int, color: int) -> None:
        self.pixels[(x, y)] = self.check_color(color)

    def draw_horizontal_line(self, x0: int, y: int, width: int, color: int) -> None:
        for x in range(x0, x0 + width):
            self.pixels[(x, y)] = self.check_color(color)

    def draw_rect(self, x0: int, y0: int, width: int, height: int, color: int) -> List[TextEvent]:
        color = self.check_color(color)

        if x0 == 0 and y0 == 0 and width == self._width and height == self._height:
            self.default_color = color
            self.draw_default_color = True
            self.pixels = {}
            self.reset_screeshot_pixels = True
            return [TextEvent("", 0, 0, 0, 0, True)]

        for x in range(x0, x0 + width):
            for y in range(y0, y0 + height):
                self.pixels[(x, y)] = color

        return []

    def _get_image(self) -> bytes:
        # This call is made from the Speculos API thread
        # Protect screenshot_pixels for concurrent Write during this Read
        with self.screenshot_pixels_lock:
            data = bytearray(self.default_color.to_bytes(3, "big")) * self._width * self._height
            for (x, y), color in self.screenshot_pixels.items():
                pos = 3 * (y * self._width + x)
                data[pos:pos + 3] = color.to_bytes(3, "big")
        return bytes(data)

    def _get_screenshot_iobytes_value(self) -> bytes:
        # Get the pixels object once, as it may be replaced during the loop.
        data = self._get_image()

        image = Image.frombytes("RGB", self.current_screen_size, data)
        iobytes = io.BytesIO()
        image.save(iobytes, format="PNG")
        return iobytes.getvalue()

    def take_screenshot(self) -> Tuple[Tuple[int, int], bytes]:
        return self.current_screen_size, self._get_image()

    def update_screenshot(self) -> None:
        # This call is made from the MCU/Seproxyhal thread
        # Protect screenshot_pixels for concurrent Read during this Write
        with self.screenshot_pixels_lock:
            if self.reset_screeshot_pixels:
                self.screenshot_pixels = {}
                self.reset_screeshot_pixels = False
            self.screenshot_pixels.update(self.pixels)

    def update_public_screenshot(self) -> None:
        # Stax/Flex only
        # As we lazyly evaluate the published screenshot, we only flag the evaluation update as necessary
        self.recreate_public_screenshot = True

    @property
    def public_screenshot_value(self) -> bytes:
        # Stax/Flex only
        # Lazy calculation of the published screenshot, as it is a costly operation
        # and not necessary if no one tries to read the value
        if self.recreate_public_screenshot:
            self.recreate_public_screenshot = False
            self._public_screenshot_value = self._get_screenshot_iobytes_value()

        return self._public_screenshot_value

    def get_public_screenshot(self) -> bytes:
        if self.model == "stax" or self.model == "flex":
            # On Stax/Flex, we only make the screenshot public on the RESTFUL api when it is consistent with events
            # On top of this, take_screenshot is time consuming on stax/flex, so we'll do as few as possible
            # We return the value calculated last time update_public_screenshot was called
            return self.public_screenshot_value
        # On nano we have no knowledge of screen refreshes so we can't be scarce on publishes
        # So we publish the raw current content every time. It's ok as take_screenshot is fast on Nano
        return self._get_screenshot_iobytes_value()

    # Should be declared as an `@abstractmethod` (and also `class FrameBuffer(ABC):`), but in this
    # case multiple inheritance in `screen.PaintWidget(FrameBuffer, QWidget)` will break, as both
    # FrameBuffer and QWidget derive from a different metaclass, and Python cannot figure out which
    # class builder to use.
    def update(self,
               x: Optional[int] = None,
               y: Optional[int] = None,
               w: Optional[int] = None,
               h: Optional[int] = None) -> bool:
        raise NotImplementedError()


class GraphicLibrary(ABC):
    """
    A class interface defining mandatory method a graphical library must implement.

    Currently implemented graphic libraries are `bagl.Bagl` and `nbgl.NBGL`.
    """

    def __init__(self, fb: FrameBuffer, size: Tuple[int, int], model: str):
        self._fb = fb
        self.SCREEN_WIDTH, self.SCREEN_HEIGHT = size
        self.model = model

    @property
    def fb(self) -> FrameBuffer:
        return self._fb

    @abstractmethod
    def refresh(self, data: bytes) -> bool:
        pass

    def update_screenshot(self) -> None:
        self.fb.update_screenshot()

    def update_public_screenshot(self) -> None:
        self.fb.update_public_screenshot()

    def take_screenshot(self) -> Tuple[Tuple[int, int], bytes]:
        return self.fb.take_screenshot()


class Display(ABC):
    """
    A class interface for managing the graphic display of an application.

    Every display type is composed of two classes:
    - A `Display` implementation, which will mostly deal with the graphics
    - A `DisplayNotifier`, which will managed the `IOdevice` tied to the running application.

    Currently, there are 3 display management type, stored in following modules:
    - `screen.py`, displaying the application infos using Qt
    - `screen_text.py`, displaying the application infos using the terminal (through `curses`)
    - `headless.py`, displaying nothing, although the application interface can still be reached
      through VNC if activated.
    """

    def __init__(self, display_args: DisplayArgs, server_args: ServerArgs) -> None:
        self._server_args = server_args
        self._display_args = display_args

    @property
    def apdu(self) -> Any:  # ApduServer:
        return self._server_args.apdu

    @property
    def seph(self) -> Any:  # SeProxyHal:
        return self._server_args.seph

    @property
    def model(self) -> str:
        return self._display_args.model

    @property
    def rendering(self):
        return self._display_args.rendering

    @property
    def use_bagl(self) -> bool:
        return self._display_args.use_bagl

    @property
    @abstractmethod
    def gl(self) -> GraphicLibrary:
        pass

    @abstractmethod
    def display_status(self, data: bytes) -> List[TextEvent]:
        raise NotImplementedError()

    @abstractmethod
    def display_raw_status(self, data: bytes):
        pass

    @abstractmethod
    def screen_update(self) -> bool:
        pass

    def forward_to_app(self, packet: bytes) -> None:
        self.seph.to_app(packet)

    def forward_to_apdu_client(self, packet: bytes) -> None:
        self.apdu.forward_to_client(packet)


class DisplayNotifier(ABC):
    """
    A class interface for managing the events between an application display and the `IODevice`
    subclasses.

    Every display type is composed of two classes:
    - A `Display` implementation, which will mostly deal with the graphics
    - A `DisplayNotifier`, which will managed the `IOdevice` tied to the running application.

    Currently, there are 3 display management type, stored in following modules:
    - `screen.py`, displaying the application infos using Qt
    - `screen_text.py`, displaying the application infos using the terminal (through `curses`)
    - `headless.py`, displaying nothing, although the application interface can still be reached
      through VNC if activated.
    """

    def __init__(self, display_args: DisplayArgs, server_args: ServerArgs) -> None:
        # TODO: this should be Dict[int, IODevice], but in QtScreen, it is
        #       a QSocketNotifier, which has a completely different interface
        #       and is not used in the same way in the mcu/screen.py module.
        self.notifiers: Dict[int, Any] = {}
        self._server_args = server_args
        self._display_args = display_args
        self._display: Display
        self.__init_notifiers()

    def _set_display_class(self, display_class: type):
        self._display = display_class(self._display_args, self._server_args)

    @property
    def display(self) -> Display:
        return self._display

    def add_notifier(self, device: IODevice):
        assert device.fileno not in self.notifiers
        self.notifiers[device.fileno] = device

    def remove_notifier(self, fd: int):
        self.notifiers.pop(fd)

    def __init_notifiers(self) -> None:
        for device in self._server_args._asdict().values():
            if device:
                self.add_notifier(device)

    @abstractmethod
    def run(self) -> None:
        pass
