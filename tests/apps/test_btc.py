#!/usr/bin/env python3

'''
Tests to ensure that speculos launches correctly the BTC apps.
'''

import binascii
import os
import pkg_resources
import pytest
import socket
import time

class Vnc:
    def __init__(self, port):
        self.s = self.connect(port)

    def connect(self, port):
        for i in range(0, 5):
            try:
                s = socket.create_connection(("127.0.0.1", port), timeout=0.5)
                connected = True
                break
            except ConnectionRefusedError:
                time.sleep(0.2)
                connected = False

        assert connected
        return s

    def auth(self, password=None):
        # recv server protocol version
        data = self.s.recv(4096)
        # send client protocol version
        self.s.sendall(data)
        # receive security types supported
        data = self.s.recv(2)
        if not password:
            assert data == b"\x01\x01"
        else:
            assert data == b"\x01\x02"

class TestBtc:
    '''Tests for Bitcoin app.'''

    @staticmethod
    def get_automation_path(name):
        path = os.path.join("resources", name)
        path = pkg_resources.resource_filename(__name__, path)
        return f"file:{path}"

    def test_btc_get_version(self, app, stop_app):
        '''Send a get_version APDU to the BTC app.'''

        app.run()

        packet = binascii.unhexlify('E0C4000000')
        data, status = app.exchange(packet)
        assert status == 0x9000

    def test_btc_get_public_key_with_user_approval(self, app, stop_app, finger_client):
        if app.model != "blue":
            pytest.skip("Device not supported")

        app.run(finger_port=1236, headless=True)

        finger_client.eventsLoop = ["220,440,1", "220,440,0"]  # x,y,pressed
        finger_client.start()

        bip32_path = bytes.fromhex("8000002C" + "80000000" + "80000000" + "00000000" + "00000000")
        payload = bytes([len(bip32_path)//4]) + bip32_path
        apdu = bytes.fromhex("e0400100") +  bytes([len(payload)]) + payload

        response, status = app.exchange(apdu)
        assert status == 0x9000

    def test_btc_automation(self, app):
        """Retrieve the pubkey, which requires a validation"""

        if app.revision == '00000000' and app.model == 'nanos':
            pytest.skip("unsupported get pubkey ux for this app version")

        args = [ '--automation', TestBtc.get_automation_path(f'btc_getpubkey_{app.model}.json') ]
        app.run(args=args)

        packet = binascii.unhexlify('e040010115058000003180000000800000000000000000000000')
        data, status = app.exchange(packet)
        assert status == 0x9000

        app.stop()

    def test_vnc_no_password(self, app, stop_app):
        port = 5900
        args = [ "--vnc-port", f"{port}" ]
        app.run(args=args)

        vnc = Vnc(port)
        vnc.auth()

    def test_vnc_with_password(self, app, stop_app):
        password = "secret"
        port = 5900
        args = [ "--vnc-port", f"{port}", "--vnc-password", password ]
        app.run(args=args)

        vnc = Vnc(port)
        vnc.auth(password)

class TestBtcTestnet:
    '''Tests for Bitcoin Testnet app.'''

    def test_btc_lib(self, app, stop_app):
        # assumes that the Bitcoin version of the app also exists.
        btc_app = app.path.replace('btc-test', 'btc')
        assert os.path.exists(btc_app)

        args = [ '-l', 'Bitcoin:%s' % btc_app ]
        app.run(args=args)

        packet = binascii.unhexlify('E0C4000000')
        data, status = app.exchange(packet)
        assert status == 0x9000
