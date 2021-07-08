from contextlib import contextmanager
import json
import logging
import os
import requests
import subprocess
import time

logger = logging.getLogger("speculos-client")
logger.setLevel(logging.INFO)


def split_apdu(data):
    assert len(data) >= 2
    status = int.from_bytes(data[-2:], "big")
    return data[:-2], status


class ApduResponse:
    def __init__(self, response):
        self.response = response

    def receive(self):
        assert self.response.status_code == 200
        data = bytes.fromhex(self.response.json()["data"])
        return split_apdu(data)


class Api:
    def __init__(self, api_url):
        self.api_url = api_url
        self.timeout = 2000
        self.session = requests.session()
        self.stream = self._open_stream()

    def _open_stream(self):
        stream = self.session.get(f"{self.api_url}/events?stream=true", stream=True)
        assert stream.status_code == 200
        return stream

    def _get_next_event(self):
        line = self.stream.raw.readline()
        event = json.loads(line)
        assert event
        return event

    def wait_for_text_event(self, text):
        while True:
            event = self._get_next_event()
            if text in event["text"]:
                break
        return event

    def press_and_release(self, button):
        assert button in ["left", "right", "both"]
        data = {"action": "press-and-release"}
        with self.session.post(f"{self.api_url}/button/{button}", json=data) as response:
            assert response.status_code == 200

    def finger_touch(self, x, y):
        data = {"action": "press-and-release", "x": x, "y": y}
        with self.session.post(f"{self.api_url}/finger", json=data) as response:
            assert response.status_code == 200

    def get_screenshot(self):
        with self.session.get(f"{self.api_url}/screenshot") as response:
            assert response.status_code == 200
            assert response.headers["Content-Type"] == "image/png"
            return response.content

    def apdu_exchange(self, data):
        with self.session.post(f"{self.api_url}/apdu", json={"data": data.hex()}) as response:
            apdu_response = ApduResponse(response)
            return apdu_response.receive()

    def apdu_exchange_async(self, data):
        return self.session.post(f"{self.api_url}/apdu", json={"data": data.hex()}, stream=True)

    def set_automation_rules(self, rules):
        with self.session.post(f"{self.api_url}/automation", data=rules) as response:
            assert response.status_code == 200


class SpeculosInstance:
    def __init__(self, app, args=[]):
        self.app = app
        self.args = args
        if "--display" not in self.args:
            self.args += ["--display", "headless"]
        self.process = None

    def start(self):
        # XXX: to change when speculos becomes a package
        script_dir = os.path.dirname(os.path.realpath(__file__))
        cmd = [os.path.join(script_dir, "..", "speculos.py")]
        cmd += self.args + [self.app]

        logger.info(f"starting speculos with command: {cmd}")
        self.process = subprocess.Popen(cmd)
        time.sleep(1)

    def stop(self):
        logger.info(f"stopping speculos")
        # in case speculos has already exited
        if not self.process:
            return

        if self.process.poll() is None:
            self.process.terminate()
            time.sleep(0.2)
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait()
        self.process = None


class SpeculosClient(Api, SpeculosInstance):
    def __init__(self, app, args=[], api_url="http://127.0.0.1:5000"):
        SpeculosInstance.__init__(self, app, args)
        self.start()
        Api.__init__(self, api_url)

    def apdu_exchange(self, cla, ins, data=b"", p1=0, p2=0):
        apdu = bytes([cla, ins, p1, p2, len(data)]) + data
        return Api.apdu_exchange(self, apdu)

    @contextmanager
    def apdu_exchange_async(self, cla, ins, data=b"", p1=0, p2=0):
        apdu = bytes([cla, ins, p1, p2, len(data)]) + data
        response = None
        try:
            response = Api.apdu_exchange_async(self, apdu)
            yield ApduResponse(response)
        finally:
            if response:
                response.close()
