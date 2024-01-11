"""Tests for Vault app."""

import pytest


@pytest.fixture(scope="module")
def client(client_ram_page):
    return client_ram_page


def test_access_ram_page(client):
    client.apdu_exchange(0xe0, 0x01, b"")
