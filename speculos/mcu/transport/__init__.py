from typing import Callable

from .interface import TransportLayer, TransportType
from .nfc import NFC
from .usb import HID, U2F


def build_transport(cb: Callable, transport: TransportType) -> TransportLayer:
    if transport is TransportType.NFC:
        return NFC(cb, transport)
    elif transport is TransportType.U2F:
        return U2F(cb, transport)
    else:
        return HID(cb, transport)


__all__ = ["build_transport", "TransportType"]
