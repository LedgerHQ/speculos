import socket
import struct
from unittest.mock import MagicMock

import pytest

from speculos.mcu.seproxyhal import SephTag, SeProxyHal


@pytest.fixture
def seph():
    """A SeProxyHal instance connected to a socketpair standing in for the app."""
    seph_sock, app_sock = socket.socketpair()
    seph = SeProxyHal(seph_sock, model="stax")
    yield seph, app_sock
    app_sock.close()
    seph_sock.close()


def send_seph_packet(app_sock: socket.socket, tag: SephTag, data: bytes) -> None:
    """Send a raw seproxyhal packet, as the app under emulation would."""
    packet = tag.to_bytes(1, "big") + len(data).to_bytes(2, "big") + data
    app_sock.sendall(packet)


class TestSeProxyHal:
    @staticmethod
    def send_text_line(
        seph: SeProxyHal, app_sock: socket.socket, text: bytes, x: int = 10, y: int = 20, w: int = 30, h: int = 40
    ) -> None:
        data = text + struct.pack(">4H", x, y, w, h)
        send_seph_packet(app_sock, SephTag.NBGL_SEND_SPECULOS_TEXT_LINE, data)
        seph.can_read(MagicMock())

    def test_nbgl_text_line(self, seph):
        seph, app_sock = seph
        self.send_text_line(seph, app_sock, b"Address")

        events = seph.ocr.get_events()
        if len(events) != 1:
            raise AssertionError(f"Expected 1 event, got {events}")
        if (events[0].text, events[0].x, events[0].y, events[0].w, events[0].h) != ("Address", 10, 20, 30, 40):
            raise ValueError(f"Unexpected event {events[0]}")

    def test_nbgl_text_line_invalid_utf8(self, seph):
        """Invalid UTF-8 bytes are replaced and the text still reaches the OCR layer."""
        seph, app_sock = seph
        self.send_text_line(seph, app_sock, b"Wallet id @: tpsze\xd1\x65\x91\x8f")

        events = seph.ocr.get_events()
        if len(events) != 1:
            raise AssertionError(f"Expected 1 event, got {events}")
        if events[0].text != "Wallet id @: tpsze\ufffde\ufffd\ufffd":
            raise ValueError(f"Unexpected text {events[0].text!r}")
