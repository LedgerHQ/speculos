import binascii
import socket
import subprocess
import sys
import time
import os

from collections import namedtuple


AppInfo = namedtuple("AppInfo", ["filepath", "device", "name", "version", "hash"])

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))


class APDUClient:
    # default APDU TCP server
    HOST, PORT = ("127.0.0.1", 9999)

    def __init__(self, app_info: AppInfo):
        self.path = app_info.filepath
        self.process = None
        self.name = app_info.name
        self.model = app_info.device
        self.sdk = app_info.version
        self.revision = app_info.hash
        self.touch_events = []

    def run(
        self,
        headless=True,
        finger_port=0,
        deterministic_rng="",
        seed="",
        rampage="",
        args=[],
    ):
        """Launch an app."""

        # if the app is already running, do nothing
        if self.process:
            return

        cmd = [os.path.join(SCRIPT_DIR, "..", "..", "speculos.py")]
        cmd += args
        if headless:
            cmd += ["--display", "headless"]
        if finger_port:
            cmd += ["--finger-port", str(finger_port)]
        if deterministic_rng:
            cmd += ["--deterministic-rng", deterministic_rng]
        if seed:
            cmd += ["--seed", seed]
        if rampage:
            cmd += ["--rampage", rampage]
        cmd += ["--model", self.model]
        cmd += ["--sdk", self.sdk]
        cmd += [self.path]

        print("[*]", cmd)
        self.process = subprocess.Popen(cmd)
        time.sleep(1)

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
        data = b""
        while size > 0:
            try:
                tmp = s.recv(size)
            except ConnectionResetError:
                tmp = b""
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

        size = int.from_bytes(data, byteorder="big")
        packet = self._recvall(s, size)
        status = int.from_bytes(self._recvall(s, 2), "big")
        return packet, status

    def _exchange(self, s, packet, verbose):
        packet = len(packet).to_bytes(4, "big") + packet
        if verbose:
            print("[>]", binascii.hexlify(packet))
        s.sendall(packet)

        data, status = self._recv_packet(s)

        if verbose:
            print("[<]", binascii.hexlify(data), hex(status))

        return data, status

    def exchange(self, packet, verbose=False):
        """Exchange a packet with the app APDU TCP server."""

        for i in range(0, 5):
            try:
                s = socket.create_connection((self.HOST, self.PORT), timeout=1.0)
                connected = True
                break
            except ConnectionRefusedError:
                time.sleep(0.2)
                connected = False

        assert connected
        s.settimeout(1.5)

        data, status = self._exchange(s, packet, verbose)

        s.close()
        return data, status
