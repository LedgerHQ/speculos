import importlib.resources
import json
import re
import time

import pytest
import requests

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
        with requests.post(f"{API_URL}/automation", data=data, timeout=10) as response:
            if response.status_code != 200:
                raise AssertionError(f"Expected status code 200, got {response.status_code}")
            if response.json() != {}:
                raise ValueError(f"Expected empty JSON response, got {response.json()}")

    def test_automation_invalid_path(self):
        with requests.post(f"{API_URL}/automation", data=b"file:/etc/passwd", timeout=10) as response:
            if response.status_code != 400:
                raise AssertionError(f"Expected status code 400, got {response.status_code}")

    def test_automation_invalid_json(self):
        with requests.post(f"{API_URL}/automation", data=b"x", timeout=10) as response:
            if response.status_code != 400:
                raise AssertionError(f"Expected status code 400, got {response.status_code}")

    @staticmethod
    def press_button(button):
        data = json.dumps({"action": "press-and-release"}).encode()
        with requests.post(f"{API_URL}/button/{button}", data=data, timeout=10) as response:
            if response.status_code != 200:
                raise AssertionError(f"Expected status code 200, got {response.status_code}")

    def test_button(self):
        for button in ["right", "left", "both"]:
            TestApi.press_button(button)

    def test_finger(self):
        data = json.dumps({"x": 0, "y": 0, "action": "press-and-release"}).encode()
        with requests.post(f"{API_URL}/finger", data=data, timeout=10) as response:
            if response.status_code != 200:
                raise AssertionError(f"Expected status code 200, got {response.status_code}")

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
                if not line.startswith(b"data: "):
                    raise ValueError(f"Expected line to start with 'data: ', got {line}")
                data += line[6:]
            event = json.loads(data)
            if not isinstance(event, dict):
                raise ValueError(f"Expected event to be a dict, got {type(event)}")
            return event

        def get_current_screen_content(session):
            return session.get(f"{API_URL}/events?currentscreenonly=true").content.decode("utf-8")

        with requests.Session() as r:
            with r.get(f"{API_URL}/events?stream=true", stream=True) as stream:
                if stream.status_code != 200:
                    raise AssertionError(f"Expected status code 200, got {stream.status_code}")

                texts = [("App settings",), ("App info",), ("Quit app",)]
                for i in range(len(texts)):
                    TestApi.press_button("right")
                    for text in texts[i]:
                        event = get_next_event(stream)
                        if not re.match(text, event["text"]):
                            raise ValueError(f"Expected event text to match '{text}', got '{event['text']}'")

                texts = [
                    ("App info",),
                    ("App settings",),
                    ("Boilerplate", "app is ready"),
                ]
                for i in range(len(texts)):
                    TestApi.press_button("left")
                    for text in texts[i]:
                        event = get_next_event(stream)
                        if not re.match(text, event["text"]):
                            raise ValueError(f"Expected event text to match '{text}', got '{event['text']}'")

            texts = [
                '{"events": [{"text": "Boilerplate", "x": 32, "y": 26, "w": 64, "h": 14, "clear": false}, '
                '{"text": "app is ready", "x": 32, "y": 42, "w": 63, "h": 14, "clear": false}]}\n',
                '{"events": [{"text": "App settings", "x": 28, "y": 34, "w": 72, "h": 14, "clear": false}]}\n',
            ]
            for text in texts:
                content = get_current_screen_content(r)
                if content != text:
                    raise ValueError(f"Expected screen content to be '{text}', got '{content}'")
                TestApi.press_button("right")
                # Wait for the screen to be updated and parsed
                while content == get_current_screen_content(r):
                    time.sleep(1)

            with r.get(f"{API_URL}/events") as response:
                if not json.loads(response.content):
                    raise ValueError(f"Expected non-empty JSON response, got {response.content}")

            with r.delete(f"{API_URL}/events") as response:
                if response.status_code != 200:
                    raise AssertionError(f"Expected status code 200, got {response.status_code}")

            with r.get(f"{API_URL}/events") as response:
                if json.loads(response.content) != {"events": []}:
                    raise ValueError(f"Expected empty events, got {response.content}")

    def test_screenshot(self):
        with requests.get(f"{API_URL}/screenshot", timeout=10) as response:
            if response.status_code != 200:
                raise AssertionError(f"Expected status code 200, got {response.status_code}")
            if response.headers["Content-Type"] != "image/png":
                raise ValueError(f"Expected Content-Type 'image/png', got {response.headers['Content-Type']}")
            if not response.content.startswith(b"\x89PNG"):
                raise ValueError("Expected PNG image data")

    def test_apdu(self):
        # Send GET_VERSION to get 16 bytes of random
        with requests.post(f"{API_URL}/apdu", json={"data": "e003000000"}, timeout=10) as response:
            if response.status_code != 200:
                raise AssertionError(f"Expected status code 200, got {response.status_code}")
            data = bytes.fromhex(response.json()["data"])
            if len(data) != 5 or data[-2:] != b"\x90\x00":
                raise ValueError(f"Expected 5 bytes with status word 0x9000, got {data}")

    def test_apdu_invalid_data(self):
        with requests.post(f"{API_URL}/apdu", json={"data": "xyz"}, timeout=10) as response:
            if response.status_code != 400:
                raise AssertionError(f"Expected status code 400, got {response.status_code}")
        with requests.post(f"{API_URL}/apdu", timeout=10) as response:  # Missing data field
            if response.status_code != 400:
                raise AssertionError(f"Expected status code 400, got {response.status_code}")
