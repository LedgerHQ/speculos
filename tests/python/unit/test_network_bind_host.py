"""Defaults for Speculos network bind host (localhost-only unless opted out)."""

from unittest import TestCase

from speculos.mcu.apdu import ApduServer
from speculos.mcu.button_tcp import FakeButton
from speculos.mcu.finger_tcp import FakeFinger


class TestNetworkBindHost(TestCase):
    def test_apdu_server_defaults_to_loopback(self):
        server = ApduServer(port=0)
        try:
            self.assertEqual(server.socket.getsockname()[0], "127.0.0.1")
        finally:
            server.socket.close()

    def test_button_defaults_to_loopback(self):
        server = FakeButton(port=0)
        try:
            self.assertEqual(server.socket.getsockname()[0], "127.0.0.1")
        finally:
            server.socket.close()

    def test_finger_defaults_to_loopback(self):
        server = FakeFinger(port=0)
        try:
            self.assertEqual(server.socket.getsockname()[0], "127.0.0.1")
        finally:
            server.socket.close()

    def test_button_accepts_all_interfaces(self):
        server = FakeButton(port=0, host="0.0.0.0")
        try:
            self.assertEqual(server.socket.getsockname()[0], "0.0.0.0")
        finally:
            server.socket.close()
