# flake8: noqa
import struct
from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum, IntEnum
from typing import Dict, Type, Union

# Common types


class NbglColor(IntEnum):
    BLACK = (0,)
    DARK_GRAY = (1,)
    LIGHT_GRAY = (2,)
    WHITE = 3


class NbglBpp(IntEnum):
    BPP_1 = (0,)
    BPP_2 = 1
    BPP_4 = 2


class NbglDirection(IntEnum):
    VERTICAL = (0,)
    HORIZONTAL = 1


class NbglState(IntEnum):
    OFF_STATE = (0,)
    ON_STATE = 1


class NbglQrCodeVersion(IntEnum):
    QRCODE_V4 = (0,)
    QRCODE_V10 = 1


class NbglRadius(IntEnum):
    RADIUS_20_PIXELS = 0
    RADIUS_28_PIXELS = 1
    RADIUS_32_PIXELS = 2
    RADIUS_40_PIXELS = 3
    RADIUS_44_PIXELS = 4
    RADIUS_1_PIXEL = 0
    RADIUS_3_PIXELS = 1
    RADIUS_0_PIXELS = 0xFF


class NbglKeyboardMode(IntEnum):
    MODE_LETTERS = (0,)
    MODE_DIGITS = (1,)
    MODE_SPECIAL = (2,)
    MODE_NONE = 3


class NbglObjType(IntEnum):
    (SCREEN,) = (0,)  # Main screen
    (CONTAINER,) = (1,)  # Empty container
    (IMAGE,) = (2,)  # Bitmap (x and width must be multiple of 4)
    (LINE,) = (3,)  # Vertical or Horizontal line
    (TEXT_AREA,) = (4,)  # Area to contain text line(s)
    (BUTTON,) = (5,)  # Rounded rectangle button with icon and/or text
    (SWITCH,) = (6,)  # Switch to turn on/off something
    (PAGE_INDICATOR,) = (7,)  # horizontal bar to indicate navigation across pages

    (PROGRESS_BAR,) = (
        8,
    )  # horizontal bar to indicate progression of something (between 0% and 100%)
    (RADIO_BUTTON,) = (9,)  # Indicator to inform whether something is on or off
    (QR_CODE,) = (10,)  # QR Code
    (KEYBOARD,) = (11,)  # Keyboard
    (KEYPAD,) = (12,)  # Keypad
    (SPINNER,) = (13,)  # Spinner
    IMAGE_FILE = (14,)  # Image file (with Ledger compression)


class NbglAlignment(IntEnum):
    NO_ALIGNMENT = (0,)
    TOP_LEFT = (1,)
    TOP_MIDDLE = (2,)
    TOP_RIGHT = (3,)
    MID_LEFT = (4,)
    CENTER = (5,)
    MID_RIGHT = (6,)
    BOTTOM_LEFT = (7,)
    BOTTOM_MIDDLE = (8,)
    BOTTOM_RIGHT = (9,)
    LEFT_TOP = (10,)
    LEFT_BOTTOM = (11,)
    RIGHT_TOP = (12,)
    RIGHT_BOTTOM = (13,)


class NbglFontId(IntEnum):
    BAGL_FONT_INTER_REGULAR_24px = (0,)
    BAGL_FONT_INTER_SEMIBOLD_24px = (1,)
    BAGL_FONT_INTER_MEDIUM_32px = (2,)
    BAGL_FONT_INTER_REGULAR_24px_1bpp = (3,)
    BAGL_FONT_INTER_SEMIBOLD_24px_1bpp = (4,)
    BAGL_FONT_INTER_MEDIUM_32px_1bpp = (5,)
    BAGL_FONT_OPEN_SANS_EXTRABOLD_11px_1bpp = (8,)
    BAGL_FONT_OPEN_SANS_LIGHT_16px_1bpp = (9,)
    BAGL_FONT_OPEN_SANS_REGULAR_11px_1bpp = 10
    BAGL_FONT_INTER_REGULAR_28px = (11,)
    BAGL_FONT_INTER_SEMIBOLD_28px = (12,)
    BAGL_FONT_INTER_MEDIUM_36px = (13,)
    BAGL_FONT_INTER_REGULAR_28px_1bpp = (14,)
    BAGL_FONT_INTER_SEMIBOLD_28px_1bpp = 15
    BAGL_FONT_INTER_MEDIUM_36px_1bpp = (16,)
    BAGL_FONT_NANOTEXT_MEDIUM_18px_1bpp = 17
    BAGL_FONT_NANOTEXT_BOLD_18px_1bpp = 18
    BAGL_FONT_NANODISPLAY_SEMIBOLD_24px_1bpp = (19,)

    BAGL_FONT_OBFUSCATED_INTER_REGULAR_24px = (20,)
    BAGL_FONT_OBFUSCATED_INTER_SEMIBOLD_24px = (21,)
    BAGL_FONT_OBFUSCATED_INTER_MEDIUM_32px = (22,)
    BAGL_FONT_OBFUSCATED_INTER_REGULAR_24px_1bpp = (23,)
    BAGL_FONT_OBFUSCATED_INTER_SEMIBOLD_24px_1bpp = (24,)
    BAGL_FONT_OBFUSCATED_INTER_MEDIUM_32px_1bpp = (25,)
    BAGL_FONT_OBFUSCATED_OPEN_SANS_EXTRABOLD_11px_1bpp = (26,)
    BAGL_FONT_OBFUSCATED_OPEN_SANS_LIGHT_16px_1bpp = (27,)
    BAGL_FONT_OBFUSCATED_OPEN_SANS_REGULAR_11px_1bpp = (28,)
    BAGL_FONT_OBFUSCATED_INTER_REGULAR_28px = (29,)
    BAGL_FONT_OBFUSCATED_INTER_SEMIBOLD_28px = (30,)
    BAGL_FONT_OBFUSCATED_INTER_MEDIUM_36px = (31,)
    BAGL_FONT_OBFUSCATED_INTER_REGULAR_28px_1bpp = (32,)
    BAGL_FONT_OBFUSCATED_INTER_SEMIBOLD_28px_1bpp = (33,)
    BAGL_FONT_OBFUSCATED_INTER_MEDIUM_36px_1bpp = (34,)
    BAGL_FONT_OBFUSCATED_NANOTEXT_MEDIUM_18px_1bpp = (35,)
    BAGL_FONT_OBFUSCATED_NANOTEXT_BOLD_18px_1bpp = (36,)
    BAGL_FONT_OBFUSCATED_NANODISPLAY_SEMIBOLD_24px_1bpp = (37,)


class NbglStyle(IntEnum):
    NO_STYLE = (0,)
    INVERTED_COLORS = 1


def parse_str(data: bytes) -> str:
    """
    Utility function to parse a NULL terminated string
    from input bytes. If the string is not terminated by NULL,
    take the truncated string instead.

    Returns the string
    """
    return data.split(b"\x00")[0].decode()


class NbglGenericJsonSerializable(ABC):
    """
    Base type for all NBGL objects.
    Implements generic json dict serialization / deserialization functions.
    """

    def to_json_dict(self) -> Dict:
        """
        Returns the json dict version of the object.
        """
        result = {}
        for name, val in self.__dict__.items():
            if issubclass(type(val), NbglObj):
                result[name] = val.to_json_dict()
            elif issubclass(type(val), Enum):
                result[name] = val.name
            else:
                result[name] = val
        return result

    @classmethod
    def from_json_dict(cls, data: Dict):
        """
        Get an instance of the class, from its json dict version.
        """
        fields = {}
        for field_name, field_obj in cls.__dataclass_fields__.items():  # type: ignore[attr-defined]
            field_type = field_obj.type
            if issubclass(field_type, Enum):
                fields[field_name] = field_type[data[field_name]]
            elif issubclass(field_type, NbglObj):
                fields[field_name] = NbglArea.from_json_dict(data[field_name])
            else:
                fields[field_name] = data[field_name]
        return cls(**fields)

    @classmethod
    @abstractmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        """
        Get an instance of the class, from raw bytes.
        """
        pass


class NbglObj(NbglGenericJsonSerializable):
    pass


# Nbgl objects


@dataclass
class NbglArea(NbglObj):
    width: int
    height: int
    x0: int
    y0: int
    background_color: NbglColor
    bpp: NbglBpp

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        x0, y0, width, height, color_n, bpp_n = struct.unpack(">HHHHBB", data[:10])
        color = NbglColor(color_n)
        bpp = NbglBpp(bpp_n)
        return cls(width, height, x0, y0, color, bpp)

    @staticmethod
    def size():
        return struct.calcsize(">HHHHBB")


@dataclass
class NbglScreen(NbglObj):
    area: NbglArea

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        return cls(area=area)


@dataclass
class NbglContainer(NbglObj):
    area: NbglArea
    layout: NbglDirection
    nb_children: int
    force_clean: bool

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        layout, nb_children, force_clean = struct.unpack(
            ">BB?", data[NbglArea.size() :]
        )
        return cls(
            area=area,
            layout=NbglDirection(layout),
            nb_children=nb_children,
            force_clean=force_clean,
        )


@dataclass
class NbglLine(NbglObj):
    area: NbglArea
    direction: NbglDirection
    line_color: NbglColor
    thickness: int
    offset: int

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        direction, line_color, thickness, offset = struct.unpack(
            ">BBBB", data[NbglArea.size() :]
        )
        return cls(
            area=area,
            direction=NbglDirection(direction),
            line_color=NbglColor(line_color),
            thickness=thickness,
            offset=offset,
        )


@dataclass
class NbglRadioButton(NbglObj):
    area: NbglArea
    active_color: NbglColor
    border_color: NbglColor
    state: NbglState

    @classmethod
    def from_bytes(cls, is_stax: bool, data):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        active_color, border_color, state = struct.unpack(
            ">BBB", data[NbglArea.size() :]
        )
        return cls(
            area=area,
            active_color=NbglColor(active_color),
            border_color=NbglColor(border_color),
            state=NbglState(state),
        )


@dataclass
class NbglSwitch(NbglObj):
    area: NbglArea
    on_color: NbglColor
    off_color: NbglColor
    state: NbglState

    @classmethod
    def from_bytes(cls, is_stax: bool, data):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        on_color, off_color, state = struct.unpack(">BBB", data[NbglArea.size() :])
        return cls(
            area=area,
            on_color=NbglColor(on_color),
            off_color=NbglColor(off_color),
            state=NbglState(state),
        )


@dataclass
class NbglProgressBar(NbglObj):
    area: NbglArea
    with_border: bool
    state: int

    @classmethod
    def from_bytes(cls, is_stax: bool, data):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        with_border, state = struct.unpack(">?B", data[NbglArea.size() :])
        return cls(area=area, with_border=with_border, state=state)


@dataclass
class NbglPageIndicator(NbglObj):
    area: NbglArea
    active_page: int
    nb_pages: int

    @classmethod
    def from_bytes(cls, is_stax: bool, data):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        active_page, nb_pages = struct.unpack(">BB", data[NbglArea.size() :])
        return cls(area=area, active_page=active_page, nb_pages=nb_pages)


@dataclass
class NbglButton(NbglObj):
    area: NbglArea
    inner_color: NbglColor
    border_color: NbglColor
    foreground_color: NbglColor
    radius: NbglRadius
    font_id: NbglFontId
    localized: bool
    text: str

    @classmethod
    def from_bytes(cls, is_stax: bool, data):
        cnt = NbglArea.size()
        area = NbglArea.from_bytes(is_stax, data[0:cnt])
        params_template = ">BBBBB?"
        params_size = struct.calcsize(params_template)
        inner_color, border_color, foreground_color, radius, font_id, localized = (
            struct.unpack(params_template, data[cnt : cnt + params_size])
        )

        cnt += params_size
        text = parse_str(data[cnt:])

        return cls(
            area=area,
            inner_color=NbglColor(inner_color),
            border_color=NbglColor(border_color),
            foreground_color=NbglColor(foreground_color),
            radius=NbglRadius(radius),
            font_id=NbglFontId(font_id),
            localized=localized,
            text=text,
        )


@dataclass
class NbglTextArea(NbglObj):
    area: NbglArea
    text_color: NbglColor
    text_alignment: NbglAlignment
    style: NbglStyle
    font_id: NbglFontId
    localized: bool
    auto_hide_long_line: bool
    len: int
    text: str

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        # Parse area
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        cnt = NbglArea.size()

        # Parse pattern
        params_pattern = ">BBBB??H"
        params_size = struct.calcsize(params_pattern)
        text_color, alignment, style, font_id, localized, auto_hide_long_line, len = (
            struct.unpack(params_pattern, data[cnt : cnt + params_size])
        )
        cnt += params_size

        # Parse text
        text = parse_str(data[cnt:])
        return cls(
            area=area,
            text_color=NbglColor(text_color),
            text_alignment=NbglAlignment(alignment),
            style=NbglStyle(style),
            font_id=NbglFontId(font_id),
            localized=localized,
            auto_hide_long_line=auto_hide_long_line,
            len=len,
            text=text,
        )


@dataclass
class NbglSpinner(NbglObj):
    area: NbglArea
    position: int

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        (position,) = struct.unpack(">B", data[NbglArea.size() :])
        return cls(area=area, position=position)


@dataclass
class NbglImage(NbglObj):
    area: NbglArea
    width: int
    height: int
    bpp: int
    isFile: int
    foreground_color: NbglColor

    @classmethod
    def from_bytes(cls, is_stax: bool, data):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])

        (
            width,
            height,
            bpp,
            isFile,
            size,
        ) = struct.unpack(">HHBBI", data[NbglArea.size() : NbglArea.size() + 10])
        (foreground_color,) = struct.unpack(">B", data[NbglArea.size() + 10 :])
        return cls(
            area=area,
            width=width,
            height=height,
            bpp=bpp,
            isFile=isFile,
            foreground_color=NbglColor(foreground_color),
        )


@dataclass
class NbglImageFile(NbglObj):
    area: NbglArea

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        area = NbglArea.from_bytes(is_stax, data[0:])
        return cls(area)


@dataclass
class NbglQrCode(NbglObj):
    area: NbglArea
    foreground_color: NbglColor
    version: NbglQrCodeVersion
    text: str

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        # Parse area
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        cnt = NbglArea.size()

        # Parse QR code color and version
        foreground_color, version = struct.unpack(">BB", data[cnt : cnt + 2])
        cnt += 2

        # Parse text
        text = parse_str(data[cnt:])

        # Return
        return cls(
            area=area,
            foreground_color=NbglColor(foreground_color),
            version=NbglQrCodeVersion(version),
            text=text,
        )


@dataclass
class NbglKeyboard(NbglObj):
    area: NbglArea
    text_color: NbglColor
    border_color: NbglColor
    letters_only: bool
    upper_case: bool
    mode: NbglKeyboardMode
    key_mask: int
    selected_char_index: int

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        if is_stax:
            selected_char_index = 0
            text_color, border_color, letters_only, upper_case, mode, key_mask = (
                struct.unpack(">BBBBBI", data[NbglArea.size() :])
            )
        else:
            upper_case = False
            (
                text_color,
                border_color,
                letters_only,
                selected_char_index,
                mode,
                key_mask,
            ) = struct.unpack(">BBBBBI", data[NbglArea.size() :])

        return cls(
            area=area,
            text_color=NbglColor(text_color),
            border_color=NbglColor(border_color),
            letters_only=letters_only,
            upper_case=upper_case,
            mode=NbglKeyboardMode(mode),
            key_mask=key_mask,
            selected_char_index=selected_char_index,
        )


@dataclass
class NbglKeypad(NbglObj):
    area: NbglArea
    text_color: NbglColor
    border_color: NbglColor
    enable_backspace: bool
    enable_validate: bool
    enable_digits: bool
    shuffled: bool
    selectedKey: int

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        area = NbglArea.from_bytes(is_stax, data[0 : NbglArea.size()])
        if is_stax:
            (
                text_color,
                border_color,
                enable_backspace,
                enable_validate,
                enable_digits,
                shuffled,
            ) = struct.unpack(">BBBBBB", data[NbglArea.size() : NbglArea.size() + 6])
            selectedKey = 0  # unused on Stax
        else:
            text_color = NbglColor.WHITE
            border_color = NbglColor.BLACK
            enable_digits = True
            enable_backspace, enable_validate, shuffled, selectedKey = struct.unpack(
                ">BBBB", data[NbglArea.size() : NbglArea.size() + 4]
            )
        return cls(
            area=area,
            text_color=NbglColor(text_color),
            border_color=NbglColor(border_color),
            enable_backspace=enable_backspace,
            enable_validate=enable_validate,
            enable_digits=enable_digits,
            shuffled=shuffled,
            selectedKey=selectedKey,
        )


# Mapping of NbglObjType and their associated class.
NBGL_OBJ_TYPES: Dict[NbglObjType, Type[NbglObj]] = {
    NbglObjType.SCREEN: NbglScreen,
    NbglObjType.CONTAINER: NbglContainer,
    NbglObjType.LINE: NbglLine,
    NbglObjType.IMAGE: NbglImage,
    NbglObjType.IMAGE_FILE: NbglImageFile,
    NbglObjType.QR_CODE: NbglQrCode,
    NbglObjType.RADIO_BUTTON: NbglRadioButton,
    NbglObjType.TEXT_AREA: NbglTextArea,
    NbglObjType.BUTTON: NbglButton,
    NbglObjType.SWITCH: NbglSwitch,
    NbglObjType.PAGE_INDICATOR: NbglPageIndicator,
    NbglObjType.PROGRESS_BAR: NbglProgressBar,
    NbglObjType.KEYBOARD: NbglKeyboard,
    NbglObjType.KEYPAD: NbglKeypad,
    NbglObjType.SPINNER: NbglSpinner,
}

# Nbgl events


class NbglEventType(IntEnum):
    """
    Available serialized Nbgl events
    """

    NBGL_DRAW_OBJ = (0,)
    NBGL_REFRESH_AREA = 1


@dataclass
class NbglRefreshAreaEvent(NbglGenericJsonSerializable):
    area: NbglArea

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        return cls(area=NbglArea.from_bytes(is_stax, data))


@dataclass
class NbglDrawObjectEvent(NbglGenericJsonSerializable):
    obj: NbglObj
    id: int

    @classmethod
    def from_bytes(cls, is_stax: bool, data: bytes):
        # the first byte is the object id
        # the second one is the object type
        obj_id = int(data[0])
        obj_type = NbglObjType(data[1])
        class_type = NBGL_OBJ_TYPES[obj_type]

        return cls(obj=class_type.from_bytes(is_stax, data[2:]), id=obj_id)

    def to_json_dict(self) -> Dict:
        for obj_type, obj_class in NBGL_OBJ_TYPES.items():
            if obj_class == type(self.obj):
                return {
                    "obj": {"type": obj_type.name, "content": self.obj.to_json_dict()},
                    "id": self.id,
                }
        # Object not serializable
        return {}

    @classmethod
    def from_json_dict(cls, data: Dict):
        obj_id = data["id"]
        obj_data = data["obj"]
        obj_type = NBGL_OBJ_TYPES[NbglObjType[obj_data["type"]]]

        return cls(obj=obj_type.from_json_dict(obj_data["content"]), id=obj_id)


# Public functions


NbglEvent = Union[NbglRefreshAreaEvent, NbglDrawObjectEvent]


def deserialize_nbgl_bytes(is_stax: bool, data: bytes) -> NbglEvent:
    """
    Return a NbglRefreshAreaEvent or a NbglDrawObjectEvent,
    from input bytes.
    """
    event_type = NbglEventType(int(data[0]))

    if event_type == NbglEventType.NBGL_DRAW_OBJ:
        return NbglDrawObjectEvent.from_bytes(is_stax, data[1:])
    elif event_type == NbglEventType.NBGL_REFRESH_AREA:
        return NbglRefreshAreaEvent.from_bytes(is_stax, data[1:])


def deserialize_nbgl_json(data: Dict) -> NbglEvent:
    """
    Return a NbglRefreshAreaEvent or a NbglDrawObjectEvent,
    from input json-like dictionary.
    """
    event_type = NbglEventType[data["event"]]

    if event_type == NbglEventType.NBGL_DRAW_OBJ:
        return NbglDrawObjectEvent.from_json_dict(data)
    elif event_type == NbglEventType.NBGL_REFRESH_AREA:
        return NbglRefreshAreaEvent.from_json_dict(data)


def serialize_nbgl_json(data: NbglEvent) -> Dict:
    """
    Return a json-like dictionary from
    input NbglRefreshAreaEvent / NbglDrawObjectEvent
    """
    EVENT_TYPES = {
        NbglRefreshAreaEvent: NbglEventType.NBGL_REFRESH_AREA,
        NbglDrawObjectEvent: NbglEventType.NBGL_DRAW_OBJ,
    }

    result = {"event": EVENT_TYPES[type(data)].name}
    result.update(data.to_json_dict())
    return result
