import socket
import time
from types import SimpleNamespace
from unittest import TestCase
from unittest.mock import MagicMock

from speculos.client import Api, ClientException


class FakeStream:
    """Event stream returning the given lines, then blocking on its socket until it is shut down."""

    def __init__(self, lines: list[bytes]) -> None:
        self.status_code = 200
        self.lines = list(lines)
        self.sock, self.peer = socket.socketpair()
        self.raw = SimpleNamespace(readline=self.readline, fileno=self.sock.fileno)
        self.closed = False

    def readline(self) -> bytes:
        if self.lines:
            return self.lines.pop(0)
        return self.sock.recv(1)

    def close(self) -> None:
        self.closed = True
        self.sock.close()
        self.peer.close()


def event_lines(text: str) -> list[bytes]:
    return [f'data: {{"text": "{text}", "x": 0, "y": 0}}\n'.encode(), b"\n"]


class TestApiEvents(TestCase):
    def open(self, lines: list[bytes]) -> Api:
        api = Api("http://127.0.0.1:5000")
        api.session = MagicMock()
        api.session.get.return_value = FakeStream(lines)
        api.open_stream()
        self.addCleanup(api.close_stream)
        return api

    def test_get_next_event(self):
        api = self.open(event_lines("Hello") + event_lines("World"))
        self.assertEqual(api.get_next_event(timeout=1)["text"], "Hello")
        self.assertEqual(api.get_next_event(timeout=1)["text"], "World")

    def test_get_next_event_timeout(self):
        api = self.open([])
        start = time.monotonic()
        with self.assertRaises(TimeoutError):
            api.get_next_event(timeout=0.2)
        self.assertLess(time.monotonic() - start, 1)

    def test_wait_for_text_event(self):
        api = self.open(event_lines("Hello") + event_lines("World"))
        self.assertEqual(api.wait_for_text_event("Wor", timeout=1)["text"], "World")

    def test_wait_for_text_event_timeout(self):
        api = self.open(event_lines("Hello"))
        start = time.monotonic()
        with self.assertRaises(TimeoutError):
            api.wait_for_text_event("World", timeout=0.2)
        self.assertLess(time.monotonic() - start, 1)

    def test_invalid_line_raises_on_each_call(self):
        api = self.open([b"garbage\n"])
        for _ in range(2):
            with self.assertRaises(ClientException):
                api.get_next_event(timeout=1)

    def test_close_stream_unblocks_reader(self):
        api = self.open([])
        stream = api.stream
        start = time.monotonic()
        api.close_stream()
        self.assertLess(time.monotonic() - start, 0.5)
        self.assertTrue(stream.closed)
        self.assertIsNone(api.stream)

    def test_get_next_event_without_stream(self):
        api = Api("http://127.0.0.1:5000")
        with self.assertRaises(AssertionError):
            api.get_next_event(timeout=0)
