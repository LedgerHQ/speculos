from dataclasses import dataclass
from typing import Any, Dict, NamedTuple, Optional, Tuple

# from speculos.mcu.apdu import ApduServer
# from speculos.mcu.seproxyhal import SeProxyHal
# from speculos.mcu.button_tcp import FakeButton
# from speculos.mcu.finger_tcp import FakeFinger
# from speculos.mcu.vnc import VNC
# from speculos.api.api import ApiRunner


@dataclass
class Model:
    name: str
    screen_size: Tuple[int, int]
    box_position: Tuple[int, int]
    box_size: Tuple[int, int]


MODELS: Dict[str, Model] = {
    'nanos': Model('Nano S', (128, 32), (20, 13), (100, 26)),
    'nanox': Model('Nano X', (128, 64), (5, 5), (10, 10)),
    'nanosp': Model('Nano SP', (128, 64), (5, 5), (10, 10)),
    'blue': Model('Blue', (320, 480), (13, 13), (26, 26)),
    'stax': Model('Stax', (400, 672), (13, 13), (26, 26)),
}


@dataclass
class DisplayArgs:
    color: str
    model: str
    ontop: bool
    rendering: bool
    keymap: str
    pixel_size: int
    x: Optional[int]
    y: Optional[int]


class ServerArgs(NamedTuple):
    apdu: Any  # ApduServer
    apirun: Any  # Optional[ApiRunner]
    button: Any  # Optional[FakeButton]
    finger: Any  # Optional[FakeFinger]
    seph: Any  # SeProxyHal
    vnc: Any  # Optional[VNC]
