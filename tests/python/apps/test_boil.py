#!/usr/bin/env python3

'''
Tests to ensure that speculos launches correctly the BTC apps.
'''

import json
import importlib.resources
import io
import os
import pytest

from enum import IntEnum

import speculos.client

CLA = 0xE0


class Ins(IntEnum):
    GET_PUBLIC_KEY = 0x05
    GET_VERSION = 0x03


@pytest.fixture(scope="module")
def client(client_boil):
    return client_boil


def read_automation_rules(name):
    path = importlib.resources.files(__package__) / "resources" / name
    with open(path, "rb") as fp:
        rules = json.load(fp)
    return rules


def test_boil_get_version(client):
    """Send a get_version APDU to the Boilerplate app."""
    client.apdu_exchange(CLA, Ins.GET_VERSION, b"")


def test_boil_get_public_key_with_user_approval(client, app):
    """Ask for the public key, compare screenshots and press reject."""

    payload = bytes.fromhex("058000002c80000001800000000000000000000000")

    with client.apdu_exchange_nowait(CLA, Ins.GET_PUBLIC_KEY, payload, p1=0x01) as response:
        event = client.wait_for_text_event("Verify BOL address")
        while "Cancel" not in event["text"]:
            client.press_and_release("right")
            event = client.get_next_event()
 
        screenshot = client.get_screenshot()
        path = importlib.resources.files(__package__) / "resources" / f"boil_getpubkey_{app.model}.png"
        assert speculos.client.screenshot_equal(path, io.BytesIO(screenshot))

        client.press_and_release("both")

        with pytest.raises(speculos.client.ApduException) as e:
            response.receive()
        assert e.value.sw == 0x6985


def test_boil_automation(client, app):
    """Retrieve the pubkey, which requires a validation"""

    rules = read_automation_rules(f"boil_getpubkey_{app.model}.json")
    client.set_automation_rules(rules)

    payload = bytes.fromhex("058000002c80000001800000000000000000000000")
    client.apdu_exchange(CLA, Ins.GET_PUBLIC_KEY, payload, p1=0x01, p2=0x00)
