#!/usr/bin/env python3
"""Minimal example: drive a Speculos-emulated device over the REST API.

Start Speculos first (see run.sh), then run this script. It sends an APDU,
reads the current screen text, presses a button, and saves a screenshot —
the building blocks of any automated or agentic workflow.

Requires: pip install requests
"""

import sys

import requests

BASE = "http://127.0.0.1:5000"
TIMEOUT = 5  # seconds; Speculos is local so requests should return quickly


def send_apdu(hex_data: str) -> str:
    """Send a raw APDU (hex string) and return the device response (hex)."""
    resp = requests.post(f"{BASE}/apdu", json={"data": hex_data}, timeout=TIMEOUT)
    resp.raise_for_status()
    return resp.json()["data"]


def current_screen_text() -> list[str]:
    """Return the text lines currently displayed on the device screen."""
    resp = requests.get(f"{BASE}/events", params={"currentscreenonly": "true"}, timeout=TIMEOUT)
    resp.raise_for_status()
    return [e.get("text", "") for e in resp.json()["events"]]


def press(button: str) -> None:
    """Press and release a Nano button: 'left', 'right' or 'both'."""
    requests.post(
        f"{BASE}/button/{button}",
        json={"action": "press-and-release"},
        timeout=TIMEOUT,
    )


def touch(x: int, y: int) -> None:
    """Press and release the touchscreen (Stax / Flex / Apex+)."""
    requests.post(
        f"{BASE}/finger",
        json={"action": "press-and-release", "x": x, "y": y},
        timeout=TIMEOUT,
    )


def screenshot(path: str = "screen.png") -> None:
    resp = requests.get(f"{BASE}/screenshot", timeout=TIMEOUT)
    resp.raise_for_status()
    with open(path, "wb") as f:
        f.write(resp.content)


def main() -> int:
    try:
        requests.get(f"{BASE}/events", timeout=TIMEOUT)
    except requests.exceptions.RequestException:
        print("Cannot reach Speculos on", BASE, "- is it running? See run.sh")
        return 1

    print("Screen:", current_screen_text())

    # Example APDU: many apps answer the "get version" command. Adapt it.
    print("APDU response:", send_apdu("e0c0000004"))

    press("right")
    print("Screen after right press:", current_screen_text())

    screenshot()
    print("Saved screenshot to screen.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())
