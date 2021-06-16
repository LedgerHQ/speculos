import json
import os
import pkg_resources
import pytest
import requests
import subprocess
import time

from collections import namedtuple

AppInfo = namedtuple("AppInfo", ["filepath", "device", "name", "version", "hash"])

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
API_URL = "http://127.0.0.1:5000"


class App:
    def __init__(self, app_info: AppInfo):
        self.path = app_info.filepath
        self.process = None
        self.name = app_info.name
        self.model = app_info.device
        self.sdk = app_info.version

    def run(self, headless=True, args=[]):
        """Launch an app."""

        # if the app is already running, do nothing
        if self.process:
            return

        cmd = [os.path.join(SCRIPT_DIR, "..", "..", "speculos.py")]
        cmd += args
        if headless:
            cmd += ["--display", "headless"]
        cmd += ["--model", self.model]
        cmd += ["--sdk", self.sdk]
        cmd += [self.path]

        print("[*]", cmd)
        self.process = subprocess.Popen(cmd)
        time.sleep(1)

    def stop(self):
        # if the app has already quit, do nothing
        if not self.process:
            return

        if self.process.poll() is None:
            self.process.terminate()
            time.sleep(0.2)
        if self.process.poll() is None:
            self.process.kill()
        self.process.wait()


@pytest.fixture(scope="class")
def app(request):
    """Run the API tests on the default btc.elf app."""

    app_dir = os.path.join(SCRIPT_DIR, os.pardir, os.pardir, "apps")
    filepath = os.path.realpath(os.path.join(app_dir, "btc.elf"))
    info = [filepath] + os.path.basename(filepath).split("#")
    info = AppInfo(*info)

    _app = App(info)
    _app.run()
    yield _app
    _app.stop()


class TestApi:
    @staticmethod
    def get_automation_data(name):
        path = os.path.join("resources", name)
        path = pkg_resources.resource_filename(__name__, path)
        with open(path, "rb") as fp:
            data = fp.read()
        return data

    def test_automation_valid(self, app):
        data = TestApi.get_automation_data("automation.json")
        with requests.post(f"{API_URL}/automation", data=data) as response:
            assert response.status_code == 200
            assert response.json() == {}

    def test_automation_invalid_path(self, app):
        with requests.post(f"{API_URL}/automation", data=b"file:/etc/passwd") as response:
            assert response.status_code == 400

    def test_automation_invalid_json(self, app):
        with requests.post(f"{API_URL}/automation", data=b"x") as response:
            assert response.status_code == 400

    @staticmethod
    def press_button(button):
        data = json.dumps({"action": "press-and-release"}).encode()
        with requests.post(f"{API_URL}/button/{button}", data=data) as response:
            assert response.status_code == 200

    def test_button(self, app):
        for button in ["right", "left", "both"]:
            TestApi.press_button(button)

    def test_finger(self, app):
        data = json.dumps({"x": 0, "y": 0, "action": "press-and-release"}).encode()
        with requests.post(f"{API_URL}/finger", data=data) as response:
            assert response.status_code == 200

    def test_events(self, app):
        """
        Read a stream of events while pressing the button 'right' and left 3
        times.
        """

        with requests.session() as r:
            with r.get(f"{API_URL}/events?stream=true", stream=True) as stream:
                assert stream.status_code == 200

                for i in range(0, 3):
                    TestApi.press_button("right")
                    data = stream.raw.readline()
                    assert json.loads(data)

                for i in range(0, 3):
                    TestApi.press_button("left")
                    data = stream.raw.readline()
                    assert json.loads(data)

            with r.get(f"{API_URL}/events") as response:
                assert json.loads(response.content)

            with r.delete(f"{API_URL}/events") as response:
                assert response.status_code == 200

            with r.get(f"{API_URL}/events") as response:
                assert json.loads(response.content) == {"events": []}

    def test_screenshot(self, app):
        with requests.get(f"{API_URL}/screenshot") as response:
            assert response.status_code == 200
            assert response.headers["Content-Type"] == "image/png"
            assert response.content.startswith(b"\x89PNG")

    def test_apdu(self, app):
        # Send GET_RANDOM APDU to get 16 bytes of random
        with requests.post(f"{API_URL}/apdu", json={"data": "e0c0000010"}) as response:
            assert response.status_code == 200
            data = bytes.fromhex(response.json()["data"])
            assert len(data) == 18 and data[-2:] == b"\x90\x00"

    def test_apdu_invalid_data(self, app):
        with requests.post(f"{API_URL}/apdu", json={"data": "xyz"}) as response:
            assert response.status_code == 400
        with requests.post(f"{API_URL}/apdu") as response:  # Missing data field
            assert response.status_code == 400
