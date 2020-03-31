import struct
import time


class TestRamPage:
    '''Tests for Vault app.'''

    def test_access_ram_page(self, app, stop_app):
        app.run(headless=True)

        response, status = app.exchange(bytes.fromhex("e0010000"))
        assert status == 0x9000
