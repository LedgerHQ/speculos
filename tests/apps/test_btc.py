#!/usr/bin/env python3

'''
Tests to ensure that speculos launches correctly the BTC apps.
'''

import binascii
import os


class TestBtc:
    '''Tests for Bitcoin app.'''

    def test_btc_get_version(self, app, stop_app):
        '''Send a get_version APDU to the BTC app.'''

        app.run()

        packet = binascii.unhexlify('E0C4000000')
        data, status = app.exchange(packet)
        assert status == 0x9000

        app.stop()

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
