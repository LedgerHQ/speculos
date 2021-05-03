from typing import Any, List, Union, Tuple, Mapping, Optional
from dataclasses import dataclass
import functools
import string
import uuid
try:
    import PIL.Image
except ImportError:
    pass


from . import bagl_font
from . import display


XCoordinate = int
YCoordinate = int
ASCIIScreen = str
Screen = List[List[Any]]
FrameBuffer = Mapping[Tuple[XCoordinate, YCoordinate], int]
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
        if byte_string not in cache:
            __font_char_cache[byte_string] = f(byte_string)
        return __font_char_cache[byte_string]
    return wrapper



@dataclass
class BitMapChar:
    char: bagl_font.FontCharacter
    bitmap: bytes


def get_blank_screen(model: str, empty: Union[str, int] = "0") -> Screen:
    width, height = display.MODELS[model].screen_size
    return [[empty] * width for _ in range(height)]


def dump_nanox_screen_to_png(screen: Screen, capture_id: Optional[str] = None):
    """
    :param screen: a python array of bits materializing a B&W screen
    :param capture_id: an optional string with which to construct the image filename
    :return: PIL.Image object
    """
    width = len(screen[0]) if screen else 0
    height = len(screen) if width else 0
    image = PIL.Image.new("1", (width, height), "black")
    pixels = image.load()
    for i in range(image.width):
        for j in range(image.height):
            if bool(screen[j][i]):
                pixels[i, j] = 1
    if not capture_id:
        capture_id = uuid.uuid4().hex[:4]
    image.save(f"output-{capture_id}.png")
    return image


def framebuffer_pixels_to_ascii(fb_pixels: FrameBuffer, model="nanox") -> ASCIIScreen:
    """Return an ASCII-ART version of the passed frame-buffer.
    The frame-buffer is a mapping [[(X,Y)] -> boolean] because nanoX
    has a one-color display.

    - 'x' is for a white pixel
    - ' ' is for a dark pixel
    - 'O' is for a pixel that has not been written through APDU instruction
    """
    screen = get_blank_screen(model, empty="O")
    for (X, Y), value in fb_pixels.items():
        screen[Y][X] = "x" if value else " "
    return "\n".join("".join(line) for line in screen)


def framebuffer_pixels_to_screen(fb_pixels: FrameBuffer, model="nanox") -> Screen:
    screen = get_blank_screen(model, empty=0)
    for (X, Y), value in fb_pixels.items():
        screen[Y][X] = 1 if value else 0
    return screen


def save_fb_to_image(fb) -> "PIL.Image":
    return dump_nanox_screen_to_png(framebuffer_pixels_to_screen(fb))


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


def get_font_char(char: str, font: bagl_font.Font):
    ord_char = ord(char)
    if ord_char > font.last_char or ord_char > font.last_char:
        raise ValueError(f"char {char} is not displayable with font_id {font.font_id}")


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
            return max([x.lower() for x in all_values])
