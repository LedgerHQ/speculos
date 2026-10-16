import struct
from array import array
from types import SimpleNamespace
from unittest import TestCase

from speculos.mcu.vnc import _UNSENT, VNC


class FakeStdin:
    def __init__(self):
        self.data = bytearray()

    def write(self, data: bytes) -> None:
        self.data += data

    def flush(self) -> None:
        pass


def make_vnc(width: int, height: int) -> VNC:
    # Skip __init__, which starts the vnc_server binary.
    vnc = VNC.__new__(VNC)
    vnc._width, vnc._height = width, height
    vnc._sent = array("I", [_UNSENT]) * (width * height)
    vnc.subprocess = SimpleNamespace(stdin=FakeStdin())
    return vnc


def sent_pixels(vnc: VNC) -> dict:
    data = vnc.subprocess.stdin.data
    pixels = {}
    for i in range(0, len(data), 9):
        y, x, color, _ = struct.unpack_from("<HHIB", data, i)
        pixels[(x, y)] = color
    vnc.subprocess.stdin.data = bytearray()
    return pixels


class TestVncRedraw(TestCase):
    def test_first_redraw_sends_every_pixel(self):
        vnc = make_vnc(4, 3)
        vnc.redraw({(1, 2): 0x123456}, 0xFFFFFF, False)
        expected = {(x, y): 0xFFFFFF for x in range(4) for y in range(3)}
        expected[(1, 2)] = 0x123456
        self.assertEqual(sent_pixels(vnc), expected)

    def test_unchanged_pixels_are_not_sent(self):
        vnc = make_vnc(4, 3)
        vnc.redraw({(1, 2): 0x123456}, 0xFFFFFF, True)
        sent_pixels(vnc)
        vnc.redraw({(1, 2): 0x123456}, 0xFFFFFF, False)
        self.assertEqual(sent_pixels(vnc), {})

    def test_partial_redraw_keeps_the_rest_of_the_screen(self):
        vnc = make_vnc(4, 3)
        vnc.redraw({(1, 2): 0x123456}, 0xFFFFFF, True)
        sent_pixels(vnc)
        vnc.redraw({(3, 0): 0x000000}, 0xFFFFFF, False)
        self.assertEqual(sent_pixels(vnc), {(3, 0): 0x000000})

    def test_clear_resets_the_rest_of_the_screen(self):
        vnc = make_vnc(4, 3)
        vnc.redraw({(1, 2): 0x123456}, 0xFFFFFF, True)
        sent_pixels(vnc)
        vnc.redraw({(3, 0): 0x000000}, 0xFFFFFF, True)
        self.assertEqual(sent_pixels(vnc), {(1, 2): 0xFFFFFF, (3, 0): 0x000000})
