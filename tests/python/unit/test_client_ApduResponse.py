from unittest import TestCase
from unittest.mock import MagicMock

from speculos.client import ApduException, ApduResponse


def make_response(hex_data: str) -> MagicMock:
    response = MagicMock()
    response.status_code = 200
    response.json.return_value = {"data": hex_data}
    return response


class TestApduResponse(TestCase):
    def test_receive_success(self):
        self.assertEqual(ApduResponse(make_response("01029000")).receive(), b"\x01\x02")

    def test_receive_error_raises(self):
        with self.assertRaises(ApduException) as e:
            ApduResponse(make_response("01026985")).receive()
        self.assertEqual(e.exception.sw, 0x6985)
        self.assertEqual(e.exception.data, b"\x01\x02")

    def test_receive_with_status_returns_any_status(self):
        for hex_data, expected in [
            ("01029000", (b"\x01\x02", 0x9000)),
            ("6110", (b"", 0x6110)),
            ("aa6985", (b"\xaa", 0x6985)),
        ]:
            self.assertEqual(ApduResponse(make_response(hex_data)).receive_with_status(), expected)
