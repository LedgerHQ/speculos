import json
import os
import pkg_resources
import pytest
import re
import requests
from collections import namedtuple

from speculos.client import SpeculosClient

AppInfo = namedtuple("AppInfo", ["filepath", "device", "name", "version", "hash"])

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
API_URL = "http://127.0.0.1:5000"


@pytest.fixture(scope="class")
def client(request):
    """Run the API tests on the default btc.elf app."""

    app_dir = os.path.join(SCRIPT_DIR, os.pardir, os.pardir, "apps")
    filepath = os.path.realpath(os.path.join(app_dir, "btc.elf"))
    info = [filepath] + os.path.basename(filepath).split("#")
    info = AppInfo(*info)

    args = ["--model", info.device, "--sdk", info.version]

    with SpeculosClient(app=filepath, args=args) as _client:
        yield _client


@pytest.mark.usefixtures("client")
class TestApi:
    @staticmethod
    def get_automation_data(name):
        path = os.path.join("resources", name)
        path = pkg_resources.resource_filename(__name__, path)
        with open(path, "rb") as fp:
            data = fp.read()
        return data

    def test_automation_valid(self):
        data = TestApi.get_automation_data("automation.json")
        with requests.post(f"{API_URL}/automation", data=data) as response:
            assert response.status_code == 200
            assert response.json() == {}

    def test_automation_invalid_path(self):
        with requests.post(f"{API_URL}/automation", data=b"file:/etc/passwd") as response:
            assert response.status_code == 400

    def test_automation_invalid_json(self):
        with requests.post(f"{API_URL}/automation", data=b"x") as response:
            assert response.status_code == 400

    @staticmethod
    def press_button(button):
        data = json.dumps({"action": "press-and-release"}).encode()
        with requests.post(f"{API_URL}/button/{button}", data=data) as response:
            assert response.status_code == 200

    def test_button(self):
        for button in ["right", "left", "both"]:
            TestApi.press_button(button)

    def test_finger(self):
        data = json.dumps({"x": 0, "y": 0, "action": "press-and-release"}).encode()
        with requests.post(f"{API_URL}/finger", data=data) as response:
            assert response.status_code == 200

    def test_events(self):
        """
        Read a stream of events while pressing the button 'right' and left 3
        times.
        """

        def get_next_event(stream):
            """
            Return the next event.

            A subset of the event stream format is recognized by this function
            and the event is expected to be encoded in JSON.
            """

            data = b""
            while True:
                line = stream.raw.readline()
                if line == b"\n":
                    break
                assert line.startswith(b"data: ")
                data += line[6:]
            event = json.loads(data)
            assert isinstance(event, dict)
            return event

        with requests.Session() as r:
            with r.get(f"{API_URL}/events?stream=true", stream=True) as stream:
                assert stream.status_code == 200

                texts = [("Version", ".*"), ("About",), ("Quit",)]
                for i in range(0, 3):
                    TestApi.press_button("right")
                    for text in texts[i]:
                        event = get_next_event(stream)
                        assert re.match(text, event["text"])

                texts = [("About",), ("Version", ".*"), ("Bitcoin", "is ready")]
                for i in range(0, 3):
                    TestApi.press_button("left")
                    for text in texts[i]:
                        event = get_next_event(stream)
                        assert re.match(text, event["text"])

            with r.get(f"{API_URL}/events") as response:
                assert json.loads(response.content)

            with r.delete(f"{API_URL}/events") as response:
                assert response.status_code == 200

            with r.get(f"{API_URL}/events") as response:
                assert json.loads(response.content) == {"events": []}

    def test_screenshot(self):
        with requests.get(f"{API_URL}/screenshot") as response:
            assert response.status_code == 200
            assert response.headers["Content-Type"] == "image/png"
            assert response.content.startswith(b"\x89PNG")

    def test_apdu(self):
        # Send GET_RANDOM APDU to get 16 bytes of random
        with requests.post(f"{API_URL}/apdu", json={"data": "e0c0000010"}) as response:
            assert response.status_code == 200
            data = bytes.fromhex(response.json()["data"])
            assert len(data) == 18 and data[-2:] == b"\x90\x00"

    def test_apdu_invalid_data(self):
        with requests.post(f"{API_URL}/apdu", json={"data": "xyz"}) as response:
            assert response.status_code == 400
        with requests.post(f"{API_URL}/apdu") as response:  # Missing data field
            assert response.status_code == 400
