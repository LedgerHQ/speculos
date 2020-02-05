#!/usr/bin/env python3

'''
Tests to ensure that speculos launches correctly all the apps in app/.

Usage: pytest-3 -v -s ./tests/test_apps.py
'''

import binascii
import pathlib
import pytest
import os
import socket
import subprocess
import sys
import time

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

class App:
    # default APDU TCP server
    HOST, PORT = ('127.0.0.1', 9999)

    def __init__(self, path):
        self.path = path
        self.process = None
        # name example: nanos#btc#1.5#5b6693b8.elf
        self.model, self.name, self.sdk, self.revision = os.path.basename(path).split('#')

    def run(self, headless=True, args=[]):
        '''Launch an app.'''

        # if the app is already running, do nothing
        if self.process:
            return

        cmd = [ os.path.join(SCRIPT_DIR, '..', 'speculos.py') ]
        cmd += args
        if headless:
            cmd += [ '--display', 'headless' ]
        cmd += [ '--model', self.model ]
        cmd += [ '--sdk', self.sdk ]
        cmd += [ self.path ]

        print('[*]', cmd)
        self.process = subprocess.Popen(cmd)

    def stop(self):
        # if the app is already running, do nothing
        if not self.process:
            return

        if self.process.poll() is None:
            self.process.terminate()
            time.sleep(0.2)
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait()

    def _recvall(self, s, size):
        data = b''
        while size > 0:
            try:
                tmp = s.recv(size)
            except ConnectionResetError:
                tmp = b''
            if len(tmp) == 0:
                print("[-] connection with client closed", file=sys.stderr)
                return None
            data += tmp
            size -= len(tmp)
        return data

    def _recv_packet(self, s):
        data = self._recvall(s, 4)
        if data is None:
            return None

        size = int.from_bytes(data, byteorder='big')
        packet = self._recvall(s, size)
        status = int.from_bytes(self._recvall(s, 2), 'big')
        return packet, status

    def _exchange(self, s, packet, verbose):
        packet = len(packet).to_bytes(4, 'big') + packet
        if verbose:
            print('[>]', binascii.hexlify(packet))
        s.sendall(packet)

        data, status = self._recv_packet(s)
        if verbose:
            print('[<]', binascii.hexlify(data), hex(status))

        return data, status

    def exchange(self, packet, verbose=False):
        '''Exchange a packet with the app APDU TCP server.'''

        for i in range(0, 5):
            try:
                s = socket.create_connection((self.HOST, self.PORT), timeout=1.0)
                connected = True
                break
            except ConnectionRefusedError:
                time.sleep(0.2)
                connected = False

        assert connected
        s.settimeout(0.5)

        # unfortunately, the app can take some time to start...
        time.sleep(1.0)

        data, status = self._exchange(s, packet, verbose)

        s.close()
        return data, status

@pytest.fixture(scope="function")
def stop_app(request):
    yield
    app = request.getfixturevalue('app')
    app.stop()

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

class TestVault:
    '''Tests for Vault app.'''

    def test_version(self, app, stop_app):
        app.run()

        # send an invalid packet
        packet = binascii.unhexlify('E0C4000000')
        data, status = app.exchange(packet)
        assert status == 0x6D00

def listapps(app_dir):
    '''List every available apps in the app/ directory.'''

    paths = [ filename for filename in os.listdir(app_dir) if pathlib.Path(filename).suffix == '.elf' ]
    # ignore app whose name doesn't match the expected pattern
    paths = [ path for path in paths if path.count('#') == 3 ]
    return [ App(os.path.join(app_dir, path)) for path in paths ]

def filter_apps(cls, apps):
    '''
    Filter apps by the class name of the test.

    If no filter matches the class name, all available apps are returned.
    '''

    app_filter = {
        'TestBtc': [ 'btc' ],
        'TestBtcTestnet': [ 'btc-test' ],
        'TestVault': [ 'vault' ],
    }

    class_name = cls.__name__.split('.')[-1]
    if class_name in app_filter:
        names = app_filter[class_name]
        apps = [ app for app in apps if app.name in names ]

    return apps

def pytest_generate_tests(metafunc):
    # retrieve the list of apps in the ../apps directory
    app_dir = os.path.join(SCRIPT_DIR, '..', 'apps')
    apps = listapps(app_dir)
    apps = filter_apps(metafunc.cls, apps)

    # if a test function has an app parameter, give the list of app
    if 'app' in metafunc.fixturenames:
        # test are run on each app
        metafunc.parametrize('app', apps, scope='class')
