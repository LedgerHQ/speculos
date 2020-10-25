import sys

from PyQt5.QtWidgets import QApplication, QWidget, QMainWindow
from PyQt5.QtGui import QPainter, QColor, QPixmap
from PyQt5.QtGui import QIcon
from PyQt5.QtCore import Qt, QObject, QSocketNotifier, QSettings

from . import bagl
from .display import Display, FrameBuffer, COLORS, MODELS, RENDER_METHOD

from PIL import Image
from typing import Tuple
import time

BUTTON_LEFT  = 1
BUTTON_RIGHT = 2


class GIFRecorder:
    """
    Records drawed frames to generate an animated GIF file.

    We save frames only when the screen has changed, and not at every refresh
    tick. We also save the frame times so each GIF frame duration can be
    accurate.

    Unfortunately, all the frames are kept in memory until the program ends.
    This is not very efficient, but it should still be small for not being a
    problem.
    """
    def __init__(self, path: str, model: str, size: Tuple[int, int]):
        """
        :param path: GIF output path.
        :param model: Device model name.
        :param size: Screen size, in pixels.
        """
        self.path = path
        self.model = model
        self.size = size
        self.images = []
        self.times = []
        self.image = None
        self.__new_image()
        self.__has_changed = False

    def draw_point(self, x: int, y: int, color: int):
        """
        Draw a pixel.
        :param x: Pixel abscissa
        :param y: Pixel ordinate
        :param color: Pixel color
        """
        # Color is not always the same with NanoS, but the screen is only able
        # to display black and blue. Fix here.
        if self.model == 'nanos' and color != 0:
            color = 0xfbff00
        self.pixels[x, y] = color
        self.__has_changed = True

    def refresh(self):
        """ Called to show the new screen once every pixels have been drawn. """
        if self.__has_changed:
            self.__new_image()
            self.__has_changed = False

    def __new_image(self):
        """ Create a new PIL image for the next GIF frame. """
        if self.image is not None:
            self.image = self.image.copy()
        else:
            self.image = Image.new('RGB', self.size)
        self.pixels = self.image.load()
        self.images.append(self.image)
        # Append the current time of this new image
        self.times.append(time.time())

    def finish(self):
        """ Save all recorded frame to a GIF file. """
        self.times.append(time.time())  # Current time on exit
        # Calculate the duration of all frames
        durations = []
        for i in range(2, len(self.times)):
            a = self.times[i-1]
            b = self.times[i]
            durations.append(int((b - a) * 1000))
        self.images[0].save(self.path, save_all=True,
            append_images=self.images[1:-1], optimize=True, duration=durations,
            loop=0)


class PaintDispatcher:
    """
    Dispatch paint instructions to multiple instances.

    This has been created to have multiple display output: the Qt widget and the
    GIF recorder.
    """
    def __init__(self):
        self.targets = []

    def draw_point(self, x, y, color):
        """
        Draw a pixel.
        :param x: Pixel abscissa
        :param y: Pixel ordinate
        :param color: Pixel color
        """
        for target in self.targets:
            target.draw_point(x, y, color)

    def refresh(self):
        """ Called to show the new screen once every pixels have been drawn. """
        for target in self.targets:
            target.refresh()

    def append(self, target):
        """
        Append a target in the dispatcher.
        :param target: Target to be called during paint events.
        """
        self.targets.append(target)


class PaintWidget(QWidget):
    def __init__(self, parent, model, pixel_size, vnc=None):
        super().__init__(parent)
        self.fb = FrameBuffer(model)
        self.pixel_size = pixel_size
        self.mPixmap = QPixmap()
        self.vnc = vnc
        self.images = []

    def paintEvent(self, event):
        if self.fb.pixels:
            pixmap = QPixmap(self.size() / self.pixel_size)
            pixmap.fill(Qt.white)
            painter = QPainter(pixmap)
            painter.drawPixmap(0, 0, self.mPixmap)
            self._redraw(painter)
            self.mPixmap = pixmap
            self.fb.pixels = {}

        qp = QPainter(self)
        copied_pixmap = self.mPixmap
        if self.pixel_size != 1:
            # Only call scaled if needed.
            copied_pixmap = self.mPixmap.scaled(
                self.mPixmap.width() * self.pixel_size,
                self.mPixmap.height() * self.pixel_size)
        qp.drawPixmap(0, 0, copied_pixmap )

    def _redraw(self, qp):
        for (x, y), color in self.fb.pixels.items():
            qp.setPen(QColor.fromRgb(color))
            qp.drawPoint(x, y)

        if self.vnc:
            self.vnc.redraw(self.fb.pixels)

    def draw_point(self, x, y, color):
        return self.fb.draw_point(x, y, color)

    def refresh(self):
        self.update()

class Screen(Display):
    def __init__(self, app, apdu, seph, button_tcp, finger_tcp, model, rendering, vnc, ):
        self.app = app
        super().__init__(apdu, seph, model, rendering)
        self._init_notifiers(apdu, seph, button_tcp, finger_tcp, vnc)
        self.bagl = bagl.Bagl(app.m, MODELS[model].screen_size)
        self.seph = seph

    def add_notifier(self, klass):
        n = QSocketNotifier(klass.s.fileno(), QSocketNotifier.Read, self.app)
        n.activated.connect(lambda s: klass.can_read(s, self))

        assert klass.s.fileno() not in self.notifiers
        self.notifiers[klass.s.fileno()] = n

    def enable_notifier(self, fd, enabled=True):
        n = self.notifiers[fd]
        n.setEnabled(enabled)

    def remove_notifier(self, fd):
        # just in case
        self.enable_notifier(fd, False)

        n = self.notifiers.pop(fd)
        n.disconnect()
        del n

    def _key_event(self, event, pressed):
        key = event.key()
        if key in [ Qt.Key_Left, Qt.Key_Right ]:
            buttons = { Qt.Key_Left: BUTTON_LEFT, Qt.Key_Right: BUTTON_RIGHT }
            # forward this event to seph
            self.seph.handle_button(buttons[key], pressed)
        elif key == Qt.Key_Down:
            self.seph.handle_button(BUTTON_LEFT, pressed)
            self.seph.handle_button(BUTTON_RIGHT, pressed)
        elif key == Qt.Key_Q and not pressed:
            self.app.close()

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

class App(QMainWindow):
    def __init__(self, apdu, seph, button_tcp, finger_tcp, color, model, ontop, rendering, vnc, pixel_size, gif):
        """
        :param gif: Path to the output GIF file if screen is recorded. If None,
            no output GIF is created.
        """
        super().__init__()

        self.setWindowTitle('Ledger %s Emulator' % MODELS[model].name)

        self.seph = seph
        self.width, self.height = MODELS[model].screen_size
        self.pixel_size = pixel_size
        self.box_position_x, self.box_position_y = MODELS[model].box_position
        box_size_x, box_size_y = MODELS[model].box_size

        # If the position of the window has been saved in the settings, restore
        # it.
        settings = QSettings("ledger", "speculos")
        window_x = settings.value("window_x", 10, int)
        window_y = settings.value("window_y", 10, int)
        window_width = (self.width + box_size_x) * pixel_size
        window_height = (self.height + box_size_y) * pixel_size
        self.setGeometry(window_x, window_y, window_width, window_height)
        self.setFixedSize(window_width, window_height)

        flags = Qt.FramelessWindowHint
        if ontop:
            flags |= Qt.CustomizeWindowHint | Qt.WindowStaysOnTopHint
        self.setWindowFlags(flags)

        self.setAutoFillBackground(True)
        p = self.palette()
        p.setColor(self.backgroundRole(), QColor.fromRgb(COLORS[color]))
        self.setPalette(p)

        self.m = PaintDispatcher()

        # Add paint widget
        self.paint_widget = w = PaintWidget(self, model, pixel_size, vnc)
        w.move(self.box_position_x * pixel_size, self.box_position_y * pixel_size)
        w.resize(self.width * pixel_size, self.height * pixel_size)
        self.m.append(w)

        # Create GIF recorder
        if gif is not None:
            self.gif_recorder = GIFRecorder(gif, model, (self.width, self.height))
            self.m.append(self.gif_recorder)
        else:
            self.gif_recorder = None

        self.screen = Screen(self, apdu, seph, button_tcp, finger_tcp, model, rendering, vnc)

        self.setWindowIcon(QIcon('mcu/icon.png'))

        self.show()

    def screen_update(self):
        self.screen.screen_update()

    def keyPressEvent(self, event):
        self.screen._key_event(event, True)

    def keyReleaseEvent(self, event):
        self.screen._key_event(event, False)

    def _get_x_y(self):
        x = self.mouse_offset.x() // self.pixel_size - (self.box_position_x + 1)
        y = self.mouse_offset.y() // self.pixel_size - (self.box_position_y + 1)
        return x, y

    def mousePressEvent(self, event):
        '''Get the mouse location.'''

        self.mouse_offset = event.pos()

        x, y = self._get_x_y()
        if x >= 0 and x < self.width and y >= 0 and y < self.height:
            self.seph.handle_finger(x, y, True)
        QApplication.setOverrideCursor(Qt.DragMoveCursor)

    def mouseReleaseEvent(self, event):
        x, y = self._get_x_y()
        if x >= 0 and x < self.width and y >= 0 and y < self.height:
            self.seph.handle_finger(x, y, False)
        QApplication.restoreOverrideCursor()

    def mouseMoveEvent(self, event):
        '''Move the window.'''

        x = event.globalX()
        y = event.globalY()
        x_w = self.mouse_offset.x()
        y_w = self.mouse_offset.y()
        self.move(x - x_w, y - y_w)

    def closeEvent(self, event):
        '''
        Called when the window is closed. We save the current window position to
        the settings file in order to restore it upon next speculos execution.
        '''
        settings = QSettings("ledger", "speculos")
        settings.setValue("window_x", self.pos().x())
        settings.setValue("window_y", self.pos().y())
        if self.gif_recorder is not None:
            self.gif_recorder.finish()


class QtScreen:
    def __init__(self, apdu, seph, button_tcp=None, finger_tcp=None, color='MATTE_BLACK', model='nanos', ontop=False, rendering=RENDER_METHOD.FLUSHED, vnc=None, pixel_size=2, gif=None, **_):
        """
        :param gif: Path to the output GIF file if screen is recorded. If None,
            no output GIF is created.
        """
        self.app = QApplication(sys.argv)
        self.app_widget = App(apdu, seph, button_tcp, finger_tcp, color, model, ontop, rendering, vnc, pixel_size, gif)

    def run(self):
        self.app.exec_()
        self.app.quit()
