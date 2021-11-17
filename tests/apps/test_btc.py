#!/usr/bin/env python3

'''
Tests to ensure that speculos launches correctly the BTC apps.
'''

import json
import io
import os
import pkg_resources
import pytest

from enum import IntEnum

import speculos.client

CLA = 0xE0


class Ins(IntEnum):
    GET_PUBLIC_KEY = 0x40
    GET_VERSION = 0xC4


@pytest.fixture(scope="module")
def client(client_btc):
    return client_btc


def read_automation_rules(name):
    path = os.path.join("resources", name)
    path = pkg_resources.resource_filename(__name__, path)
    with open(path, "rb") as fp:
        rules = json.load(fp)
    return rules


def test_btc_get_version(client):
    """Send a get_version APDU to the BTC app."""
    client.apdu_exchange(CLA, Ins.GET_VERSION, b"")


def test_btc_get_public_key_with_user_approval(client, app):
    """Ask for the public key, compare screenshots and press reject."""

    if app.model == "nanos" and app.hash == "00000000":
        pytest.skip(f"skipped on model {app.model} because of animated text")

    bip32_path = bytes.fromhex("8000002C80000000800000000000000000000000")
    payload = bytes([len(bip32_path) // 4]) + bip32_path

    with client.apdu_exchange_nowait(CLA, Ins.GET_PUBLIC_KEY, payload, p1=0x01) as response:
        if app.model == "blue":
            client.wait_for_text_event("CONFIRM ACCOUNT")
        else:
            event = client.wait_for_text_event("Address")
            while "Reject" not in event["text"]:
                client.press_and_release("right")
                event = client.get_next_event()

        screenshot = client.get_screenshot()
        path = pkg_resources.resource_filename(__name__, f"resources/btc_getpubkey_{app.model}.png")
        assert speculos.client.screenshot_equal(path, io.BytesIO(screenshot))

        if app.model == "blue":
            client.finger_touch(110, 440)
        else:
            client.press_and_release("both")

        with pytest.raises(speculos.client.ApduException) as e:
            response.receive()
        assert e.value.sw == 0x6985


def test_btc_automation(client, app):
    """Retrieve the pubkey, which requires a validation"""

    if app.model == "nanos" and app.hash == "00000000":
        pytest.skip(f"skipped on model {app.model} and revision {app.hash}")

    rules = read_automation_rules(f"btc_getpubkey_{app.model}.json")
    client.set_automation_rules(rules)

    payload = bytes.fromhex("058000003180000000800000000000000000000000")
    client.apdu_exchange(CLA, Ins.GET_PUBLIC_KEY, payload, p1=0x01, p2=0x01)
