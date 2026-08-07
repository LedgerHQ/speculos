from dataclasses import dataclass
from typing import Any, NamedTuple

Pixel = tuple[int, int]


@dataclass
class Model:
    name: str
    screen_size: tuple[int, int]
    box_position: Pixel
    box_size: tuple[int, int]


MODELS: dict[str, Model] = {
    "nanox": Model("Nano X", (128, 64), (5, 5), (10, 10)),
    "nanosp": Model("Nano SP", (128, 64), (5, 5), (10, 10)),
    "stax": Model("Stax", (400, 672), (13, 13), (26, 26)),
    "flex": Model("Flex", (480, 600), (13, 13), (26, 26)),
    "apex_p": Model("Apex P", (300, 400), (13, 13), (26, 26)),
}


@dataclass
class DisplayArgs:
    color: str
    model: str
    ontop: bool
    rendering: bool
    keymap: str
    pixel_size: int
    x: int | None
    y: int | None


class ServerArgs(NamedTuple):
    apdu: Any  # ApduServer
    apirun: Any  # Optional[ApiRunner]
    button: Any  # Optional[FakeButton]
    finger: Any  # Optional[FakeFinger]
    seph: Any  # SeProxyHal
    vnc: Any  # Optional[VNC]
