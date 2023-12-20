import pytest

from enum import IntEnum


CLA = 0xE0


class Ins(IntEnum):
    GET_PUBLIC_KEY = 0x40
    GET_VERSION = 0xC4


@pytest.fixture(scope="module")
def client(client_btc_testnet):
    return client_btc_testnet


def test_btc_lib(client):
    client.apdu_exchange(CLA, Ins.GET_VERSION, b"")
