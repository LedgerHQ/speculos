from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass
from socket import socket
from typing import Any, Dict, IO, List, NamedTuple, Optional, Union

# from speculos.mcu.apdu import ApduServer
# from speculos.mcu.seproxyhal import SeProxyHal
# from speculos.mcu.button_tcp import FakeButton
# from speculos.mcu.finger_tcp import FakeFinger
# from speculos.mcu.vnc import VNC
# from speculos.api.api import ApiRunner


class DisplayArgs(NamedTuple):
    color: str
    model: str
    ontop: bool
    rendering: bool
    keymap: str
    pixel_size: int
    x: Optional[int]
    y: Optional[int]
    force_full_ocr: bool
    disable_tesseract: bool


class ServerArgs(NamedTuple):
    apdu: Any  # ApduServer
    apirun: Any  # Optional[ApiRunner]
    button: Any  # Optional[FakeButton]
    finger: Any  # Optional[FakeFinger]
    seph: Any  # SeProxyHal
    vnc: Any  # Optional[VNC]


@dataclass
class TextEvent:
    text: str
    x: int
    y: int
    w: int
    h: int


class ObserverInterface(ABC):

    @abstractmethod
    def send_screen_event(self, event: TextEvent) -> None:
        pass


class BroadcastInterface(ABC):

    @abstractmethod
    def add_client(self, client: ObserverInterface) -> None:
        pass

    @abstractmethod
    def remove_client(self, client: ObserverInterface) -> None:
        pass

    @abstractmethod
    def broadcast(self, event: TextEvent) -> None:
        pass


class IODevice(ABC):

    @property
    @abstractmethod
    def file(self) -> Union[IO[bytes], socket]:
        raise NotImplementedError()

    @property
    def fileno(self) -> int:
        return self.file.fileno()

    @abstractmethod
    def can_read(self, s: int, screen: Display) -> None:
        pass


class Display(ABC):
    def __init__(self, display: DisplayArgs, server: ServerArgs) -> None:
        self.notifiers: Dict[int, Any] = {}
        self.apdu = server.apdu
        self.seph = server.seph
        self.model = display.model
        self.force_full_ocr = display.force_full_ocr
        self.disable_tesseract = display.disable_tesseract
        self.rendering = display.rendering

    @abstractmethod
    def display_status(self, data: bytes) -> List[TextEvent]:
        raise NotImplementedError()

    @abstractmethod
    def display_raw_status(self, data: bytes):
        pass

    @abstractmethod
    def screen_update(self) -> bool:
        pass

    @abstractmethod
    def run(self) -> None:
        pass

    def add_notifier(self, klass: IODevice):
        assert klass.fileno not in self.notifiers
        self.notifiers[klass.fileno] = klass

    def remove_notifier(self, fd: int):
        self.notifiers.pop(fd)

    def _init_notifiers(self, args: ServerArgs) -> None:
        for klass in args._asdict().values():
            if klass:
                self.add_notifier(klass)

    def forward_to_app(self, packet: bytes):
        self.seph.to_app(packet)

    def forward_to_apdu_client(self, packet):
        self.apdu.forward_to_client(packet)
