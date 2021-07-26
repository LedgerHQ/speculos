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


@pytest.fixture(scope="function")
def client(client_vnc):
    return client_vnc


@pytest.mark.additional_args("--vnc-port", "5900")
def test_vnc_no_password(client):
    vnc = Vnc(5900)
    vnc.auth()


@pytest.mark.additional_args("--vnc-port", "5900", "--vnc-password", "secret")
def test_vnc_with_password(client):
    vnc = Vnc(5900)
    vnc.auth("secret")
