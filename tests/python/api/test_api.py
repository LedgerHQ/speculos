import importlib.resources
import json
import os
import pytest
import re
import requests
import time
from collections import namedtuple

from speculos.client import SpeculosClient

API_URL = "http://127.0.0.1:5000"


@pytest.mark.usefixtures("client")
class TestApi:
    @staticmethod
    def get_automation_data(name):
        path = importlib.resources.files(__package__) / "resources" / name
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

        def get_current_screen_content(session):
            return session.get(f"{API_URL}/events?currentscreenonly=true").content.decode("utf-8")

        with requests.Session() as r:
            with r.get(f"{API_URL}/events?stream=true", stream=True) as stream:
                assert stream.status_code == 200

                texts = [("App settings",), ("App info",), ("Quit app",)]
                for i in range(len(texts)):
                    TestApi.press_button("right")
                    for text in texts[i]:
                        event = get_next_event(stream)
                        assert re.match(text, event["text"])

                texts = [("App info",), ("App settings",), ("Boilerplate", "app is ready")]
                for i in range(len(texts)):
                    TestApi.press_button("left")
                    for text in texts[i]:
                        event = get_next_event(stream)
                        assert re.match(text, event["text"])

            texts = [
                '{"events": [{"text": "Boilerplate", "x": 34, "y": 29, "w": 63, "h": 11, "clear": false}, '
                '{"text": "app is ready", "x": 34, "y": 44, "w": 62, "h": 12, "clear": false}]}\n',
                '{"events": [{"text": "App settings", "x": 29, "y": 36, "w": 72, "h": 12, "clear": false}]}\n'
            ]
            for text in texts:
                content = get_current_screen_content(r)
                assert content == text
                TestApi.press_button("right")
                # Wait for the screen to be updated and parsed
                while content == get_current_screen_content(r):
                    time.sleep(1)

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
        # Send GET_VERSION to get 16 bytes of random
        with requests.post(f"{API_URL}/apdu", json={"data": "e003000000"}) as response:
            assert response.status_code == 200
            data = bytes.fromhex(response.json()["data"])
            assert len(data) == 5 and data[-2:] == b"\x90\x00"

    def test_apdu_invalid_data(self):
        with requests.post(f"{API_URL}/apdu", json={"data": "xyz"}) as response:
            assert response.status_code == 400
        with requests.post(f"{API_URL}/apdu") as response:  # Missing data field
            assert response.status_code == 400
