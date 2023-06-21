from typing import List, Mapping
from dataclasses import dataclass
from PIL import Image
from pytesseract import image_to_data, Output
import base64
import functools
import json
import os
import string

from .automation import TextEvent
from . import bagl_font

MIN_WORD_CONFIDENCE_LVL = 0  # percent
NEW_LINE_THRESHOLD = 10  # pixels

BitMap = bytes
BitVector = str  # a string of '1' and '0'
Width = int  # width
CharWidth = Width
Char = str  # a single character string (chars are 1-char strings in Python)


__FONT_MAP = {}

DISPLAY_CHARS = string.ascii_letters + string.digits + string.punctuation


def cache_font(f):
    __font_char_cache = {}

    @functools.wraps(f)
    def wrapper(byte_string: bytes):
        if byte_string not in __font_char_cache:
            __font_char_cache[byte_string] = f(byte_string)
        return __font_char_cache[byte_string]
    return wrapper


@dataclass
class BitMapChar:
    char: bagl_font.FontCharacter
    bitmap: bytes


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
    char = get_char(font, char)
    print("\n".join(split_bytes(char.bitmap, font.bpp * char.char.char_width)))


def get_json_font(json_name: str) -> Mapping[Char, BitMapChar]:
    # If no json filename was provided, just return
    if json_name is None:
        return None

    # Add the fonts path (JSON files are in speculos/fonts)
    json_name = os.path.join(OCR.fonts_path, json_name)
    # Add api level information and file extension
    json_name += f"-api-level-{OCR.api_level}.json"

    # Read the JSON file if we found one
    font_info = []
    if os.path.exists(json_name):
        with open(json_name, "r") as json_file:
            font_info = json.load(json_file, strict=False)
            font_info = font_info[0]
            # Deserialize bitmap
            bitmap = base64.b64decode(font_info['bitmap'])
            # Build BitMapChar
            font_map = {}
            for character in font_info['bagl_font_character']:
                char = character['char']
                offset = character['bitmap_offset']
                count = character['bitmap_byte_count']
                # Add this entry in font_map
                font_map[chr(char)] = BitMapChar(
                    char,
                    bytes(bitmap[offset:(offset + count)]),
                )
            OCR.json_fonts.append(font_map)
            return font_map

    return None


def get_font_map(font: bagl_font.Font):
    if font.font_id not in __FONT_MAP:
        __FONT_MAP[font.font_id] = _get_font_map(font)
    return __FONT_MAP[font.font_id]


def _get_font_map(font: bagl_font.Font) -> Mapping[Char, BitMapChar]:
    # Do we have a JSON file containing all the information we want?
    json_font = get_json_font(font.font_name)
    if json_font is not None:
        return json_font

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


def find_char_from_bitmap(bitmap: BitMap):
    """
     Find a character from a bitmap
    >>> font = get_font(4)
    >>> char = get_char(font, 'c')
    >>> find_char_from_bitmap(char.bitmap)
    'c'
    """
    all_values = []
    legacy = True
    for font in bagl_font.FONTS:
        font_map = get_font_map(font)
        if font_map in OCR.json_fonts:
            # This is a loaded JSON font => new behaviour
            for character_value, bitmap_struct in font_map.items():
                if bitmap_struct.bitmap == bitmap:
                    all_values.append(character_value)
                    legacy = False
        else:
            # Not a loaded JSON font => legacy behaviour
            for character_value, bitmap_struct in font_map.items():
                if bitmap_struct.bitmap.startswith(bitmap):
                    # sometimes (but not always) the bitmap being passed is
                    # shortened by one '\0' byte, not matching the exact bitmap
                    # provided in the font. Hence the 'residual' computation
                    residual_bytes: bytes = bitmap_struct.bitmap[len(bitmap):]
                    if all(b == 0 for b in residual_bytes):
                        all_values.append(character_value)
        if all_values:
            char = max([x for x in all_values])
            if char == "\x80":
                char = " "
            return legacy, char

    return legacy, None


class OCR:

    # Maximum space for a letter to be considered part of the same word
    MAX_BLANK_SPACE = 12
    # Location of JSON fonts
    fonts_path = ""
    # To keep track of loaded JSON fonts
    json_fonts = []
    # Current API_LEVEL
    api_level = 0

    def __init__(self, fonts_path=None, api_level=None):
        self.events: List[TextEvent] = []
        # Save fonts path & the API_LEVEL in a class variable
        if fonts_path is not None:
            OCR.fonts_path = fonts_path
        if api_level is not None:
            OCR.api_level = int(api_level)

    def analyze_bitmap(self, data: bytes):
        if data[0] != 0:
            return

        x = int.from_bytes(data[1:3], byteorder="big", signed=True)
        y = int.from_bytes(data[3:5], byteorder="big", signed=True)
        w = int.from_bytes(data[5:7], byteorder="big", signed=True)
        h = int.from_bytes(data[7:9], byteorder="big", signed=True)
        bpp = int.from_bytes(data[9:10], byteorder="big")
        color_size = 4 * (1 << bpp)
        bitmap = data[10+color_size:]

        # h may no reflect the real char height: use number of lines displayed
        h = (len(bitmap) * 8) // w
        if (len(bitmap) * 8) % w:
            h += 1

        # Space is now encoded as an empty character (no 'space' wasted :)
        if len(bitmap) == 0:
            char = ' '
            legacy = False
        else:
            legacy, char = find_char_from_bitmap(bitmap)
        if char:
            if legacy:
                # char is not from a loaded JSON font => keep legacy behaviour
                if self.events and y <= self.events[-1].y:
                    self.events[-1].text += char
                else:
                    # create a new TextEvent if there are no events yet
                    # or if there is a new line
                    self.events.append(TextEvent(char, x, y, w, h))
                return
            # char was found in a loaded JSON font => new behaviour
            if self.events:
                x_diff = x - (self.events[-1].x + self.events[-1].w)
                if x_diff < 0:
                    x_diff = -x_diff
            # if x_diff > MAX_BLANK_SPACE the char is not on same word
            if self.events and y < (self.events[-1].y + self.events[-1].h) \
               and x_diff < OCR.MAX_BLANK_SPACE:
                # Add this character to previous event
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
            else:
                # create a new TextEvent if there are no events yet or if there is a new line
                self.events.append(TextEvent(char, x, y, w, h))

    def analyze_image(self, screen_size: (int, int), data: bytes):
        image = Image.frombytes("RGB", screen_size, data)
        data = image_to_data(image, output_type=Output.DICT)
        new_text_has_been_added = False
        for item in range(len(data["text"])):
            if (data["conf"][item] > MIN_WORD_CONFIDENCE_LVL):
                if new_text_has_been_added and self.events and \
                   data["top"][item] <= self.events[-1].y + NEW_LINE_THRESHOLD:
                    self.events[-1].text += " "+data["text"][item]
                else:
                    x = data["left"][item]
                    y = data["top"][item]
                    w, h = screen_size
                    self.events.append(TextEvent(data['text'][item], x, y, w, h))
                    new_text_has_been_added = True

    def get_events(self) -> List[TextEvent]:
        events = self.events.copy()
        self.events = []
        return events
