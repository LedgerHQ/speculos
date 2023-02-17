from typing import List, Mapping
from dataclasses import dataclass
from PIL import Image, ImageOps
from pytesseract import image_to_data, Output
import functools
import string
import cv2
import numpy as np

from .automation import TextEvent
from . import bagl_font

# Tesseract OCR parameters
MIN_WORD_CONFIDENCE_LVL = 0  # percent
NEW_LINE_THRESHOLD = 10  # pixels
# Image pre-processing parameters
# Used for Stax
# Box minimum width and height for inversion.
STAX_BOX_MIN_HEIGHT = 50  # pixels
STAX_BOX_MIN_WIDTH = 100  # pixels
# Used for Nano X, Nano SP
# Image upscale factor.
NANO_IMAGE_UPSCALE_FACTOR = 2
# Aspect ratio threshold for non-text areas removal.
NANO_NON_TEXT_AREA_AR = 1.8
# Minimum non-text area dimension.
NANO_NON_TEXT_AREA_MIN = 5 * 5
# Margin around non-text areas when filling
# rectangle to hide them.
NANO_NON_TEXT_AREA_MARGIN = 2


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


@cache_font
def find_char_from_bitmap(bitmap: BitMap):
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


class OCR:
    def __init__(self):
        self.events: List[TextEvent] = []

    def analyze_bitmap(self, data: bytes):
        if data[0] != 0:
            return

        x = int.from_bytes(data[1:3], byteorder="big", signed=True)
        y = int.from_bytes(data[3:5], byteorder="big", signed=True)
        bpp = int.from_bytes(data[9:10], byteorder="big")
        color_size = 4 * (1 << bpp)
        bitmap = data[10+color_size:]

        char = find_char_from_bitmap(bitmap)
        if char:
            if self.events and y <= self.events[-1].y:
                self.events[-1].text += char
            else:
                # create a new TextEvent if there are no events yet or if there is a new line
                self.events.append(TextEvent(char, x, y))

    def _nano_remove_non_text_areas(self, image: Image) -> Image:
        array = np.array(image)
        # Convert the image to grayscale
        gray = cv2.cvtColor(array, cv2.COLOR_BGR2GRAY)
        # Apply some gaussian blur
        gray = cv2.GaussianBlur(gray, (7, 1), 0)
        # Apply thresholding to binarize the image
        _, thresh = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY_INV + cv2.THRESH_OTSU)
        # Dilate the image to connect nearby text regions
        kernel_dilation = cv2.getStructuringElement(cv2.MORPH_RECT, (5, 3))
        dilation = cv2.dilate(thresh, kernel_dilation, iterations=2)
        # Apply some erosion for more accurate contour detection
        kernel_erosion = np.ones((7, 7), np.uint8)
        erosion = cv2.erode(dilation, kernel_erosion, iterations=1)
        # Find contours in the image
        contours, hierarchy = cv2.findContours(erosion, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
        # Remove non-text areas based on aspect ratio
        m = NANO_NON_TEXT_AREA_MARGIN
        for contour in contours:
            x, y, w, h = cv2.boundingRect(contour)
            ar = w / float(h)
            area = w * h
            # Fill white rectangles over non-text areas.
            if ar < NANO_NON_TEXT_AREA_AR and area > NANO_NON_TEXT_AREA_MIN:
                cv2.rectangle(array, (x - m, y - m), (x + w + m, y + h + m), (255, 255, 255), -1)
        return Image.fromarray(array)

    def _stax_detect_and_invert_box_shapes(self, image: Image) -> Image:
        screen_width, _ = image.size
        array = np.array(image)
        # Convert the image to grayscale
        img = cv2.cvtColor(array, cv2.COLOR_BGR2GRAY)
        # Apply thresholding to binarize the image
        thresh = cv2.threshold(img, 60, 255, cv2.THRESH_BINARY)[1]
        # Find contours (boxes) in the image
        contours, _ = cv2.findContours(thresh, 1, 2)
        for contour in contours:
            x, y, w, h = cv2.boundingRect(contour)
            if w > STAX_BOX_MIN_WIDTH and h > STAX_BOX_MIN_HEIGHT and w < screen_width:
                row1 = y + 1
                row2 = y + h - 1
                col1 = x
                col2 = x + w
                subset = array[row1:row2, col1:col2]
                # Invert subset (box)
                subset = 255 - subset
                array[row1:row2, col1:col2] = subset
        return Image.fromarray(array)

    def analyze_image(self, screen_size: (int, int), data: bytes, device: string):
        image = Image.frombytes("RGB", screen_size, data)
        w, h = image.size
        # Apply image pre-processing for better OCR accuracy.
        if device in ["nanox", "nanosp"]:
            image = ImageOps.invert(image)
            image = image.resize((w*NANO_IMAGE_UPSCALE_FACTOR, h*NANO_IMAGE_UPSCALE_FACTOR))
            image = self._nano_remove_non_text_areas(image)
        elif device == "stax":
            image = self._stax_detect_and_invert_box_shapes(image)
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
                    self.events.append(TextEvent(data['text'][item], x, y))
                    new_text_has_been_added = True

    def get_events(self) -> List[TextEvent]:
        events = self.events.copy()
        self.events = []
        return events
