from __future__ import annotations

from PyQt6.QtWidgets import QApplication, QWidget, QMainWindow
from PyQt6.QtGui import QPainter, QColor, QPixmap
from PyQt6.QtGui import QIcon, QKeyEvent, QMouseEvent
from PyQt6.QtCore import QEvent, Qt, QSocketNotifier, QSettings, QRect
from PyQt6.sip import voidptr
from typing import Dict, List, Optional, Union
from enum import IntEnum

from speculos.observer import TextEvent
from . import bagl
from . import nbgl
from .display import COLORS, Display, DisplayNotifier, FrameBuffer, GraphicLibrary, IODevice
from .readerror import ReadError
from dataclasses import dataclass
from .struct import DisplayArgs, MODELS, ServerArgs, Pixel
from .vnc import VNC


class NanoButtons(IntEnum):
    LEFT = 1
    RIGHT = 2


@dataclass
class ButtonMapping:
    keys: Dict[Qt.Key, list[NanoButtons]]


@dataclass
class TouchMapping:
    keys: Dict[Qt.Key, Pixel]


# For Nano, the arrow keys are mapped to physical buttons
#  - Arrow Left key: Button 1
#  - Arrow Right key: Button 2
#  - Arrow Down key: Both buttons pressed together
# For touchable devices, the keys are mapped to touch coordinates:
#  - Arrow Left / Right keys: Previous / Next actions in bottom right corner
#  - Return: Approve button in center
#  - Escape: Cancel button (review) in bottom left corner
#  - Backspace: Back button in top left corner
#  - M: Menu (Setting/Info) in top right corner
KEYS_BINDINGS: Dict[str, Union[ButtonMapping, TouchMapping]] = {
    'nanox': ButtonMapping({
        Qt.Key.Key_Left: [NanoButtons.LEFT],
        Qt.Key.Key_Right: [NanoButtons.RIGHT],
        Qt.Key.Key_Down: [NanoButtons.LEFT, NanoButtons.RIGHT],
    }),
    'nanosp': ButtonMapping({
        Qt.Key.Key_Left: [NanoButtons.LEFT],
        Qt.Key.Key_Right: [NanoButtons.RIGHT],
        Qt.Key.Key_Down: [NanoButtons.LEFT, NanoButtons.RIGHT],
    }),
    'stax': TouchMapping({
        Qt.Key.Key_Left: (240, 625),
        Qt.Key.Key_Right: (360, 625),
        Qt.Key.Key_Return: (200, 530),
        Qt.Key.Key_Escape: (80, 625),
        Qt.Key.Key_Backspace: (35, 35),
        Qt.Key.Key_M: (330, 65),
    }),
    'flex': TouchMapping({
        Qt.Key.Key_Left: (290, 550),
        Qt.Key.Key_Right: (430, 550),
        Qt.Key.Key_Return: (250, 430),
        Qt.Key.Key_Escape: (80, 550),
        Qt.Key.Key_Backspace: (35, 35),
        Qt.Key.Key_M: (400, 65),
    }),
    'apex_p': TouchMapping({
        Qt.Key.Key_Left: (170, 360),
        Qt.Key.Key_Right: (270, 360),
        Qt.Key.Key_Return: (140, 280),
        Qt.Key.Key_Escape: (20, 360),
        Qt.Key.Key_Backspace: (20, 20),
        Qt.Key.Key_M: (255, 45),
    }),
}


DEFAULT_WINDOW_X = 10
DEFAULT_WINDOW_Y = 10


class PaintWidget(FrameBuffer, QWidget):
    def __init__(self, parent, model: str, pixel_size: int, vnc: Optional[VNC] = None):
        QWidget.__init__(self, parent)
        FrameBuffer.__init__(self, model)
        self.pixel_size = pixel_size
        self.mPixmap = QPixmap()
        self.vnc = vnc

    def paintEvent(self, event: QEvent):
        if self.pixels or self.draw_default_color:
            pixmap = QPixmap(self.size() / self.pixel_size)
            pixmap.fill(Qt.GlobalColor.white)
            painter = QPainter(pixmap)
            painter.drawPixmap(0, 0, self.mPixmap)
            self._redraw(painter)
            self.mPixmap = pixmap
            self.pixels = {}
            self.draw_default_color = False

        qp = QPainter(self)
        copied_pixmap = self.mPixmap
        if self.pixel_size != 1:
            # Only call scaled if needed.
            copied_pixmap = self.mPixmap.scaled(
                self.mPixmap.width() * self.pixel_size,
                self.mPixmap.height() * self.pixel_size)
        qp.drawPixmap(0, 0, copied_pixmap)

    def update(self,  # type: ignore[override]
               x: Optional[int] = None,
               y: Optional[int] = None,
               w: Optional[int] = None,
               h: Optional[int] = None) -> bool:
        if x and y and w and h:
            QWidget.update(self, QRect(x, y, w, h))
        else:
            QWidget.update(self)
        return self.pixels != {}

    def _redraw(self, qp):
        if self.draw_default_color:
            qp.fillRect(0, 0, self._width, self._height, QColor.fromRgb(self.default_color))

        for (x, y), color in self.pixels.items():
            qp.setPen(QColor.fromRgb(color))
            qp.drawPoint(x, y)

        if self.vnc is not None:
            self.vnc.redraw(self.pixels, self.default_color)

        self.update_screenshot()


class App(QMainWindow):
    def __init__(self, qt_app: QApplication, display: DisplayArgs, server: ServerArgs) -> None:
        super().__init__()
        self.setWindowTitle('Ledger %s Emulator' % MODELS[display.model].name)

        self.seph = server.seph
        self._width, self._height = MODELS[display.model].screen_size
        self.pixel_size = display.pixel_size
        self.box_position_x, self.box_position_y = MODELS[display.model].box_position
        box_size_x, box_size_y = MODELS[display.model].box_size

        # If the position of the window has been saved in the settings, restore
        # it.
        settings = QSettings("ledger", "speculos")
        # Take in account multiple screens and their geometry:
        current_screen_x = qt_app.primaryScreen().geometry().x()
        current_screen_y = qt_app.primaryScreen().geometry().y()
        if display.x is None:
            window_x = settings.value("window_x", current_screen_x + DEFAULT_WINDOW_X, int)
        else:
            window_x = display.x
        if display.y is None:
            window_y = settings.value("window_y", current_screen_y + DEFAULT_WINDOW_Y, int)
        else:
            window_y = display.y
        window_width = (self._width + box_size_x) * display.pixel_size
        window_height = (self._height + box_size_y) * display.pixel_size

        # Be sure Window is FULLY visible in one of the available screens:
        window_is_visible = False
        for screen in qt_app.screens():
            x1 = screen.geometry().x()
            y1 = screen.geometry().y()
            x2 = x1 + screen.geometry().width() - 1
            y2 = y1 + screen.geometry().height() - 1

            if window_x >= x1 and window_y >= y1 and (window_x + window_width - 1) <= x2 and \
               (window_y + window_height - 1) <= y2:
                window_is_visible = True
                break   # No need to check other screens

        # If the window is not FULLY visible, force default coordinates on current screen:
        if not window_is_visible:
            print("Window is NOT FULLY visible => using default coordinates in current screen.")
            window_x = current_screen_x + DEFAULT_WINDOW_X
            window_y = current_screen_y + DEFAULT_WINDOW_Y

        self.setGeometry(window_x, window_y, window_width, window_height)
        self.setFixedSize(window_width, window_height)

        if display.ontop:
            flags = Qt.CustomizeWindowHint | Qt.WindowStaysOnTopHint
            self.setWindowFlags(flags)

        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), QColor.fromRgb(COLORS[display.color]))
        self.setPalette(p)

        # Add paint widget and paint
        self.widget = PaintWidget(self, display.model, display.pixel_size, server.vnc)
        self.widget.move(self.box_position_x * display.pixel_size, self.box_position_y * display.pixel_size)
        self.widget.resize(self._width * display.pixel_size, self._height * display.pixel_size)
        self.setWindowIcon(QIcon('mcu/icon.png'))
        self.show()
        self._screen: Screen

    def set_screen(self, screen: Screen) -> None:
        self._screen = screen

    def screen_update(self) -> bool:
        return self._screen.screen_update()

    def keyPressEvent(self, event: QKeyEvent):
        self._screen._key_event(event, True)

    def keyReleaseEvent(self, event: QKeyEvent):
        self._screen._key_event(event, False)

    def _get_x_y(self):
        x = self.mouse_offset.x() // self.pixel_size - (self.box_position_x + 1)
        y = self.mouse_offset.y() // self.pixel_size - (self.box_position_y + 1)
        return x, y

    def mousePressEvent(self, event: QMouseEvent):
        '''Get the mouse location.'''

        self.mouse_offset = event.pos()

        x, y = self._get_x_y()
        if x >= 0 and x < self._width and y >= 0 and y < self._height:
            self.seph.handle_finger(x, y, True)

        QApplication.setOverrideCursor(Qt.CursorShape.DragMoveCursor)

    def mouseReleaseEvent(self, event: QMouseEvent):
        self.mouse_offset = event.pos()
        x, y = self._get_x_y()
        if x >= 0 and x < self._width and y >= 0 and y < self._height:
            # Send the release
            self.seph.handle_finger(x, y, False)
        QApplication.restoreOverrideCursor()

    def closeEvent(self, event: QEvent):
        '''
        Called when the window is closed. We save the current window position to
        the settings file in order to restore it upon next speculos execution.
        '''
        settings = QSettings("ledger", "speculos")
        settings.setValue("window_x", self.pos().x())
        settings.setValue("window_y", self.pos().y())


class Screen(Display):
    def __init__(self, display: DisplayArgs, server: ServerArgs) -> None:
        super().__init__(display, server)
        self.app: App
        self._bagl_gl: GraphicLibrary
        self._nbgl_gl: GraphicLibrary

    def set_app(self, app: App) -> None:
        self.app = app
        self.app.set_screen(self)
        model = self._display_args.model
        self._bagl_gl = bagl.Bagl(app.widget, MODELS[model].screen_size, model)
        self._nbgl_gl = nbgl.NBGL(app.widget, MODELS[model].screen_size, model)

    @property
    def m(self) -> QWidget:
        return self.app.widget

    @property
    def bagl_gl(self):
        return self._bagl_gl

    @property
    def nbgl_gl(self):
        return self._nbgl_gl

    def _key_event(self, event: QKeyEvent, pressed) -> None:
        key = Qt.Key(event.key())

        if event.isAutoRepeat():
            # Ignore auto-repeat events
            # (e.g. when holding a key, because the event would be sent multiple times)
            return
        if key == Qt.Key.Key_Q and not pressed:
            self.app.close()
            return
        if self._display_args.model not in KEYS_BINDINGS or \
           key not in KEYS_BINDINGS[self._display_args.model].keys:
            # Ignore unhandled events
            return

        if self._display_args.model.startswith('nano'):
            # Forward the event to seph
            kl = KEYS_BINDINGS[self._display_args.model].keys[key]
            for k in kl:
                self.seph.handle_button(k, pressed)
        else:
            # Forward the event to seph
            x, y = KEYS_BINDINGS[self._display_args.model].keys[key]
            self.seph.handle_finger(x, y, pressed)

    def display_status(self, data: bytes) -> List[TextEvent]:
        ret = self.bagl_gl.display_status(data)
        return ret

    def display_raw_status(self, data: bytes) -> None:
        self.bagl_gl.display_raw_status(data)

    def screen_update(self) -> bool:
        return self.bagl_gl.refresh()


class QtScreenNotifier(DisplayNotifier):
    def __init__(self, display_args: DisplayArgs, server_args: ServerArgs) -> None:
        self._qapp = QApplication([])
        super().__init__(display_args, server_args)
        self._set_display_class(Screen)
        self._app_widget = App(self._qapp, display_args, server_args)
        assert isinstance(self.display, Screen)
        self.display.set_app(self._app_widget)

    def _can_read(self, device: IODevice) -> None:
        try:
            device.can_read(self)
        # This exception occur when can_read have no more data available
        except ReadError:
            self._app_widget.close()

    def add_notifier(self, device: IODevice) -> None:
        n = QSocketNotifier(voidptr(device.fileno), QSocketNotifier.Type.Read, self._qapp)
        n.activated.connect(lambda _: self._can_read(device))
        assert device.fileno not in self.notifiers
        self.notifiers[device.fileno] = n

    def enable_notifier(self, fd: int, enabled: bool = True) -> None:
        n = self.notifiers[fd]
        n.setEnabled(enabled)

    def remove_notifier(self, fd: int) -> None:
        # just in case
        self.enable_notifier(fd, False)
        n = self.notifiers.pop(fd)
        n.disconnect()

    def run(self):
        self._qapp.exec()
        self._qapp.quit()
