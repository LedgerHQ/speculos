from abc import ABC, abstractmethod
from enum import auto, IntEnum
from typing import Callable, Optional


class TransportType(IntEnum):
    HID = auto()
    NFC = auto()
    U2F = auto()


class TransportLayer(ABC):

    def __init__(self, send_cb: Callable, transport: TransportType):
        self._transport = transport
        self._send_cb = send_cb

    @property
    def type(self) -> TransportType:
        return self._transport

    @abstractmethod
    def config(self, data: bytes) -> None:
        raise NotImplementedError

    @abstractmethod
    def prepare(self, data: bytes) -> Optional[bytes]:
        raise NotImplementedError

    @abstractmethod
    def send(self, data: bytes) -> None:
        raise NotImplementedError

    @abstractmethod
    def handle_rapdu(self, data: bytes) -> Optional[bytes]:
        raise NotImplementedError
