from typing import Callable

from .interface import TransportLayer, TransportType
from .nfc import NFC
from .usb import USB


def build_transport(cb: Callable, transport: TransportType) -> TransportLayer:
    if transport is TransportType.NFC:
        return NFC(cb, transport)
    else:
        return USB(cb, transport)


__all__ = ["build_transport", "TransportType"]
