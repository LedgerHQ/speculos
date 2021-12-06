from contextlib import contextmanager
from PIL import Image, ImageChops
from typing import Generator, List, Optional, Tuple, Type
from types import TracebackType
import json
import logging
import requests
import socket
import sys
import subprocess
import time

logger = logging.getLogger("speculos-client")
logger.setLevel(logging.INFO)


class ApduException(Exception):
    def __init__(self, sw: int, data: bytes) -> None:
        super().__init__(f"Exception: invalid status 0x{sw:x}")
        self.sw = sw
        self.data = data


class ClientException(Exception):
    pass


def check_status_code(response: requests.Response, url: str) -> None:
    if response.status_code != 200:
        raise ClientException(
            f"HTTP request on {url} failed, status={response.status_code}, error={response.content!r}"
        )


class ApduResponse:
    def __init__(self, response: requests.Response) -> None:
        self.response = response

    def receive(self) -> bytes:
        check_status_code(self.response, "/apdu")
        data, status = split_apdu(bytes.fromhex(self.response.json()["data"]))
        if status != 0x9000:
            raise ApduException(status, data)
        return data


def split_apdu(data: bytes) -> Tuple[bytes, int]:
    if len(data) < 2:
        raise ClientException(f"APDU response length is shorter than 2 ({data!r})")
    status = int.from_bytes(data[-2:], "big")
    return data[:-2], status


def screenshot_equal(path1: str, path2: str) -> bool:
    """Compare two images and return True if they are equal."""

    with Image.open(path1) as img1:
        with Image.open(path2) as img2:
            diff_img = ImageChops.difference(img1, img2)
    return diff_img.getbbox() is None


class Api:
    def __init__(self, api_url: str) -> None:
        self.api_url = api_url
        self.timeout = 2000
        self.session = requests.Session()
        self.stream = None

    def open_stream(self) -> None:
        assert self.stream is None
        self.stream = self.session.get(f"{self.api_url}/events?stream=true", stream=True)
        check_status_code(self.stream, "/events")

    def close_stream(self) -> None:
        if self.stream:
            self.stream.close()
        self.stream = None

    def get_next_event(self) -> dict:
        """
        A subset of the event stream format is recognized by this function and the event is expected to be encoded in
        JSON.

        https://html.spec.whatwg.org/multipage/server-sent-events.html#parsing-an-event-stream
        """
        data = b""
        while True:
            line = self.stream.raw.readline()
            if line == b"\n":
                break
            if not line.startswith(b"data: "):
                raise ClientException(f"unexpected stream event line ({line!r})")
            data += line[6:]

        event = json.loads(data)
        if not isinstance(event, dict):
            raise ClientException(f"Invalid event ({event})")

        return event

    def wait_for_text_event(self, text: str) -> dict:
        """Wait until an event containing the specified text is received."""

        while True:
            event = self.get_next_event()
            if text in event["text"]:
                break
        return event

    def press_and_release(self, button: str) -> None:
        assert button in ["left", "right", "both"]
        data = {"action": "press-and-release"}
        with self.session.post(f"{self.api_url}/button/{button}", json=data) as response:
            check_status_code(response, f"/button/{button}")

    def finger_touch(self, x: int, y: int) -> None:
        data = {"action": "press-and-release", "x": x, "y": y}
        with self.session.post(f"{self.api_url}/finger", json=data) as response:
            check_status_code(response, "/finger")

    def get_screenshot(self) -> bytes:
        with self.session.get(f"{self.api_url}/screenshot") as response:
            check_status_code(response, "/screenshot")
            return response.content

    def _apdu_exchange(self, data: bytes) -> bytes:
        with self.session.post(f"{self.api_url}/apdu", json={"data": data.hex()}) as response:
            apdu_response = ApduResponse(response)
            return apdu_response.receive()

    def _apdu_exchange_nowait(self, data: bytes) -> requests.Response:
        return self.session.post(f"{self.api_url}/apdu", json={"data": data.hex()}, stream=True)

    def set_automation_rules(self, rules: dict) -> None:
        with self.session.post(f"{self.api_url}/automation", json=rules) as response:
            check_status_code(response, "/automation")


class SpeculosInstance:
    def __init__(self, app: str, args: Optional[List[str]] = None) -> None:
        self.app = app
        if args is None:
            self.args: List[str] = []
        else:
            self.args = list(args)
        self.process: Optional[subprocess.Popen] = None

        if "--display" not in self.args:
            self.args += ["--display", "headless"]

        if "--api-port" not in self.args:
            self.port = 5000
        else:
            n = self.args.index("--api-port")
            self.port = int(self.args[n + 1])

    def _wait_until_ready(self) -> None:
        connected = False
        for i in range(0, 20):
            try:
                s = socket.create_connection(("127.0.0.1", self.port))
                connected = True
                break
            except ConnectionRefusedError:
                time.sleep(0.1)

        if not connected:
            raise ClientException(f"Failed to connect to the speculos instance on port {self.port}")

        s.close()

    def start(self) -> None:
        cmd = [sys.executable or "python3", "-m", "speculos"] + self.args + [self.app]
        logger.info(f"starting speculos with command: {' '.join(cmd)}")
        self.process = subprocess.Popen(cmd)
        self._wait_until_ready()

    def stop(self) -> None:
        logger.info("stopping speculos")
        if self.process is None:
            return

        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=0.2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()

        self.process = None


class SpeculosClient(Api, SpeculosInstance):
    def __init__(self, app: str, args: List[str] = [], api_url: str = "http://127.0.0.1:5000") -> None:
        SpeculosInstance.__init__(self, app, args)
        Api.__init__(self, api_url)

    def start(self) -> None:
        """
        Start speculos to emulate the application. `stop` must be called to ensure proper
        termination of speculos.
        Note that context `with` statement can be used with `SpeculosClient` instance instead of
        using `start` and `stop` methods.
        """
        SpeculosInstance.start(self)
        self.open_stream()

    def stop(self) -> None:
        """ Terminate speculos instance started with `start` method. """
        self.close_stream()
        SpeculosInstance.stop(self)

    def __enter__(self) -> "SpeculosClient":
        self.start()
        return self

    def __exit__(
        self, exc_type: Optional[Type[BaseException]],
        exc_val: Optional[BaseException], exc_tb: Optional[TracebackType]
    ) -> None:
        self.stop()

    def apdu_exchange(self, cla: int, ins: int, data: bytes = b"", p1: int = 0, p2: int = 0) -> bytes:
        apdu = bytes([cla, ins, p1, p2, len(data)]) + data
        return Api._apdu_exchange(self, apdu)

    @contextmanager
    def apdu_exchange_nowait(
        self, cla: int, ins: int, data: bytes = b"", p1: int = 0, p2: int = 0
    ) -> Generator[ApduResponse, None, None]:
        apdu = bytes([cla, ins, p1, p2, len(data)]) + data
        response = None
        try:
            response = Api._apdu_exchange_nowait(self, apdu)
            yield ApduResponse(response)
        finally:
            if response:
                response.close()
