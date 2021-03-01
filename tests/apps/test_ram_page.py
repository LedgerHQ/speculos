class TestRamPage:
    '''Tests for Vault app.'''

    app_names = ["ram-page"]

    def test_access_ram_page(self, app):
        app.run(headless=True)
        response, status = app.exchange(bytes.fromhex("e0010000"))
        assert status == 0x9000
