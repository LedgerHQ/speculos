import base64
import functools
import json
import logging
import os
import string
from dataclasses import dataclass
from typing import Dict, List, Mapping
from speculos.observer import TextEvent
from . import bagl_font

from construct import Struct, Int8ul, Int16ul

nbgl_area_t = Struct(
    "x0" / Int16ul,
    "y0" / Int16ul,
    "width" / Int16ul,
    "height" / Int16ul,
    "color" / Int8ul,
    "bpp" / Int8ul,
)

MIN_WORD_CONFIDENCE_LVL = 0  # percent
NEW_LINE_THRESHOLD = 10  # pixels

BitMap = bytes
BitVector = str  # a string of '1' and '0'
Width = int  # width
CharWidth = Width
Char = str  # a single character string (chars are 1-char strings in Python)


@dataclass
class BitMapChar:
    char: bagl_font.FontCharacter
    bitmap: bytes


__FONT_MAP: Dict[int, Mapping[str, BitMapChar]] = {}

DISPLAY_CHARS = string.ascii_letters + string.digits + string.punctuation


def cache_font(f):
    __font_char_cache = {}

    @functools.wraps(f)
    def wrapper(byte_string: bytes):
        if byte_string not in __font_char_cache:
            __font_char_cache[byte_string] = f(byte_string)
        return __font_char_cache[byte_string]
    return wrapper


def split(bits: BitVector, n: Width) -> List[BitVector]:
    """
    Split a bit array (string of '1' and '0')
    into an arbitrary list of n-sized bit arrays.

    Useful if you want to display a character from a bitmap
    """
    output = []
    bit_list = bits[:]
    while bit_list:
        bit_list_to_append = bit_list[:n]
        # do padding
        # bit_list_to_append += '0' * (n - len(bit_list_to_append))
        output.append(bit_list_to_append)
        bit_list = bit_list[n:]
    return output


def split_bytes(bytes_elems: BitMap, n: CharWidth):
    """Useful to display a bitmap character"""
    bits = "".join("{0:08b}".format(x)[::-1] for x in bytes_elems)
    return split(bits, n)


def get_font(font_id: int) -> bagl_font.Font:
    font = bagl_font.get(font_id)
    if not font:
        raise ValueError(f"Could not get font with font_id={font_id}")
    return font


def get_char(font: bagl_font.Font, char: str) -> BitMapChar:
    if len(char) != 1 or char not in DISPLAY_CHARS:
        raise ValueError(f"could not print {char}")
    return get_font_map(font)[char]


def display_char(font: bagl_font.Font, char: str) -> None:
    bm_char = get_char(font, char)
    print("\n".join(split_bytes(bm_char.bitmap, font.bpp * bm_char.char.char_width)))


def get_font_map(font: bagl_font.Font):
    if font.font_id not in __FONT_MAP:
        __FONT_MAP[font.font_id] = _get_font_map(font)
    return __FONT_MAP[font.font_id]


def _get_font_map(font: bagl_font.Font) -> Mapping[Char, BitMapChar]:
    font_map = {}
    for ord_char, font_char in zip(
        range(font.first_char, font.last_char), font.characters
    ):
        font_map[chr(ord_char)] = BitMapChar(
            font_char,
            bytes(
                font.bitmap[
                    font_char.bitmap_offset:(
                        font_char.bitmap_offset
                        + font_char.bitmap_byte_count
                    )
                ]
            ),
        )
    return font_map


class OCR:

    # Maximum space for a letter to be considered part of the same word
    MAX_BLANK_SPACE = 12
    # Font names for LNX & LNS+ (most used ones first)
    FONT_NAMES_NANOX_NANOSP = ["bagl_font_open_sans_regular_11px",
                               "bagl_font_open_sans_extrabold_11px",
                               "bagl_font_open_sans_light_16px"]
    # Font names for stax (most used ones first)
    FONT_NAMES_STAX = ["nbgl_font_inter_regular_24",
                       "nbgl_font_inter_semibold_24",
                       "nbgl_font_inter_medium_32",
                       "nbgl_font_inter_regular_24_1bpp",
                       "nbgl_font_inter_semibold_24_1bpp",
                       "nbgl_font_inter_medium_32_1bpp",
                       "nbgl_font_hmalpha_mono_medium_32"]

    def __init__(self,
                 fonts_path: str,
                 model: str,
                 api_level: int):
        self.events: List[TextEvent] = []
        # To keep track of loaded JSON fonts
        self.json_fonts: List[Dict] = []
        # By default use legacy OCR (built-in font bitmaps)
        self.legacy = True
        # Maximum space for a letter to be considered part of the same word
        self.max_blank_space = OCR.MAX_BLANK_SPACE
        # Store the model of the device
        self.model = model
        # With API_LEVEL 5 and >= 11, we can read fonts in JSON files
        if api_level == 5 or api_level >= 11:
            # Build font_names list using api_level & model
            if self.model == "stax":
                names = OCR.FONT_NAMES_STAX
                struct_name = 'nbgl_font_character'
                self.max_blank_space = 24       # Characters are bigger on Stax
            else:
                names = OCR.FONT_NAMES_NANOX_NANOSP
                struct_name = 'bagl_font_character'

            for name in names:
                # Add the fonts path (JSON files are in speculos/fonts)
                json_name = os.path.join(fonts_path, name)
                # Add api level information and file extension
                json_name += f"-api-level-{api_level}.json"
                if os.path.exists(json_name):
                    self.get_json_font(json_name, struct_name)

            if len(self.json_fonts) != 0:
                self.legacy = False
            else:
                logger = logging.getLogger("OCR")
                logger.warning("WARNING: didn't find any JSON font files => "
                               "OCR will not work properly!\n")

    def get_json_font(self, name, struct_name) -> None:
        """
        Read the JSON file and parse all character information
        """
        with open(name, "r") as json_file:
            font_info = json.load(json_file, strict=False)
            font_info = font_info[0]

        # Deserialize bitmap
        bitmap = base64.b64decode(font_info['bitmap'])
        if struct_name not in font_info:
            logger = logging.getLogger("OCR")
            logger.warning(f"WARNING: didn't find field '{struct_name}' "
                           f"in {name} => this font will be ignored!\n")
            return
        # Build BitMapChar
        font_map = {}
        for character in font_info[struct_name]:
            char = character['char']
            offset = character['bitmap_offset']
            count = character['bitmap_byte_count']
            # Add this entry in font_map
            font_map[chr(char)] = BitMapChar(
                char,
                bytes(bitmap[offset:(offset + count)]),
            )
            self.json_fonts.append(font_map)

    @staticmethod
    def find_char_from_bitmap_legacy(bitmap: BitMap) -> str:
        """
        Find a character from a bitmap
        >>> font = get_font(4)
        >>> char = get_char(font, 'c')
        >>> find_char_from_bitmap(char.bitmap)
        'c'
        """
        all_values = []
        for font in bagl_font.FONTS:
            font_map = get_font_map(font)
            for character_value, bitmap_struct in font_map.items():
                if bitmap_struct.bitmap.startswith(bitmap):
                    # sometimes (but not always) the bitmap being passed is shortened
                    # by one '\x00' byte, not matching the exact bitmap
                    # provided in the font. Hence the 'residual' computation
                    residual_bytes: bytes = bitmap_struct.bitmap[len(bitmap):]
                    if all(b == 0 for b in residual_bytes):
                        all_values.append(character_value)
            if all_values:
                char = max([x for x in all_values])
                if char == "\x80":
                    char = " "
                return char
        return ""

    def find_bitmap_legacy(self, x: int, y: int, w: int, h: int,
                           bitmap: bytes) -> None:
        char = self.find_char_from_bitmap_legacy(bitmap)
        if char:
            if self.events and y <= self.events[-1].y:
                self.events[-1].text += char
            else:
                # create a new TextEvent if there are no events yet
                # or if there is a new line
                self.events.append(TextEvent(char, x, y, w, h))

    def find_char_from_bitmap(self, bitmap: BitMap) -> str:
        """
        Parse loaded JSON fonts and compare font bitmaps with the one provided
        """
        all_values = []
        for font_map in self.json_fonts:
            for character_value, bitmap_struct in font_map.items():
                if bitmap_struct.bitmap == bitmap:
                    all_values.append(character_value)
        if all_values:
            char = max([x for x in all_values])
            if char == "\x80":
                char = " "
            return char
        return ""

    def store_char_in_last_event(self, x: int, y: int, w: int, h: int, char: str) -> None:
        """
        Add current character to last event
        """
        self.events[-1].text += char
        # Update w for all chars in self.events[-1]
        x2 = x + w - 1
        self.events[-1].w = x2 - self.events[-1].x + 1
        # Update y & h, if needed, for all chars in self.events[-1]
        y1 = y
        if y1 > self.events[-1].y:
            # Keep the lowest Y in Y1
            y1 = self.events[-1].y
        y2 = y + h - 1
        if y2 < (self.events[-1].y + self.events[-1].h):
            # Keep the highest Y in Y2
            y2 = self.events[-1].y + self.events[-1].h - 1
        self.events[-1].y = y1
        self.events[-1].h = y2 - y1 + 1

    def find_bitmap(self, x: int, y: int, w: int, h: int, bitmap: bytes) -> None:
        """
        Check if provided bitmap is identical to a char in loaded fonts.
        """
        # Space is now encoded as an empty character (no 'space' wasted :)
        if len(bitmap) == 0:
            char = ' '
        else:
            char = self.find_char_from_bitmap(bitmap)
        if char:
            # a char was found: is it on the same line than previous one?
            # Compute difference with X coord from previous event
            # if x_diff > self.max_blank_space the char is not on same sentence
            if self.events:
                x_diff = x - (self.events[-1].x + self.events[-1].w)
                if x_diff < 0:
                    x_diff = -x_diff
            # Try to find if that char can be added to previous event
            if self.events and y < (self.events[-1].y + self.events[-1].h) \
               and x_diff < self.max_blank_space:
                # Add this character to previous event
                self.store_char_in_last_event(x, y, w, h, char)
            else:
                # create a new TextEvent if there are no events yet or if there is a new line
                self.events.append(TextEvent(char, x, y, w, h))

    def analyze_bitmap(self, data: bytes) -> None:
        """
        data contain information about the latest displayed bitmap.
        Extract needed information (x, y, w, h & bitmap content) then parse
        known fonts to find any corresponding character.
        """
        if self.model == "stax":
            # Can be called via SephTag.NBGL_DRAW_IMAGE or SephTag.NBGL_DRAW_IMAGE_RLE
            # In both cases, data contains:
            # - area (sizeof(nbgl_area_t))
            # - compressed bitmap (buffer_len)
            # - 2 bytes of different meaning depending on SephTag
            area = nbgl_area_t.parse(data[0:nbgl_area_t.sizeof()])
            x, y, w, h = area.x0, area.y0, area.width, area.height
            bitmap = data[nbgl_area_t.sizeof():-2]
        else:
            if data[0] != 0:
                return

            x = int.from_bytes(data[1:3], byteorder="big", signed=True)
            y = int.from_bytes(data[3:5], byteorder="big", signed=True)
            w = int.from_bytes(data[5:7], byteorder="big", signed=True)
            h = int.from_bytes(data[7:9], byteorder="big", signed=True)
            bpp = int.from_bytes(data[9:10], byteorder="big")
            color_size = 4 * (1 << bpp)
            bitmap = data[10+color_size:]

            # h may no reflect the real height: use number of lines displayed
            h = (len(bitmap) * 8) // w
            if (len(bitmap) * 8) % w:
                h += 1

        if self.legacy:
            self.find_bitmap_legacy(x, y, w, h, bitmap)
        else:
            self.find_bitmap(x, y, w, h, bitmap)

    def get_events(self) -> List[TextEvent]:
        events = self.events.copy()
        self.events = []
        return events
