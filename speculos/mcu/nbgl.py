import gzip
import logging
import sys
from construct import Struct, Int8ul, Int16ul
from enum import IntEnum
from typing import Tuple

from .display import FrameBuffer, GraphicLibrary


class NbglColor(IntEnum):
    BLACK = 0
    DARK_GRAY = 1
    LIGHT_GRAY = 2
    WHITE = 3


nbgl_area_t = Struct(
    "x0" / Int16ul,
    "y0" / Int16ul,
    "width" / Int16ul,
    "height" / Int16ul,
    "color" / Int8ul,
    "bpp" / Int8ul,
)


class NBGL(GraphicLibrary):

    @staticmethod
    def to_screen_color(color: int, bpp: int) -> int:
        color_table = {
            1: 0xFFFFFF,
            2: 0x555555,
            4: 0x111111
        }
        assert bpp in color_table, f"BPP should be in {color_table.keys()}, but is '{bpp}'"
        return color * color_table[bpp]

    def __init__(self,
                 fb: FrameBuffer,
                 size: Tuple[int, int],
                 model: str,
                 force_full_ocr: bool):
        super().__init__(fb, size, model)
        self.force_full_ocr = force_full_ocr
        self.logger = logging.getLogger("NBGL")

    def __assert_area(self, area) -> None:
        if area.y0 % 4 or area.height % 4:
            raise AssertionError("X(%d) or height(%d) not 4 aligned " % (area.y0, area.height))
        if area.x0 > self.SCREEN_WIDTH or (area.x0+area.width) > self.SCREEN_WIDTH:
            raise AssertionError("left edge (%d) or right edge (%d) out of screen" % (area.x0, (area.x0 + area.width)))
        if area.y0 > self.SCREEN_HEIGHT or (area.y0+area.height) > self.SCREEN_HEIGHT:
            raise AssertionError("top edge (%d) or bottom edge (%d) out of screen" % (area.y0, (area.y0 + area.height)))

    def hal_draw_rect(self, data: bytes) -> None:
        area = nbgl_area_t.parse(data)
        if self.force_full_ocr:
            # We need all text shown in black with white background
            area.color = 3

        self.__assert_area(area)
        for x in range(area.x0, area.x0+area.width):
            for y in range(area.y0, area.y0+area.height):
                self.fb.draw_point(x, y, NBGL.to_screen_color(area.color, 2))

    def refresh(self, data: bytes) -> bool:
        area = nbgl_area_t.parse(data)
        self.__assert_area(area)
        return self.fb.update(area.x0, area.y0, area.width, area.height)

    def hal_draw_line(self, data: bytes) -> None:
        area = nbgl_area_t.parse(data[0:nbgl_area_t.sizeof()])
        self.__assert_area(area)
        mask = data[-2]
        color = data[-1]

        back_color = NBGL.to_screen_color(area.color, 2)
        front_color = NBGL.to_screen_color(color, 2)
        for x in range(area.x0, area.x0+area.width):
            for y in range(area.y0, area.y0+area.height):
                if (mask >> (y-area.y0)) & 0x1:
                    self.fb.draw_point(x, y, front_color)
                else:
                    self.fb.draw_point(x, y, back_color)

    @staticmethod
    def get_color_from_color_map(color, color_map, bpp):
        # #define GET_COLOR_MAP(__map__,__col__) ((__map__>>(__col__*2))&0x3)
        return NBGL.to_screen_color((color_map >> (color*2)) & 0x3, bpp)

    @staticmethod
    def get_4bpp_color_from_color_index(index, front_color, back_color):
        COLOR_MAPS_4BPP = {
            # Manually hardcoced color maps
            (NbglColor.BLACK, NbglColor.WHITE): [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15],
            (NbglColor.WHITE, NbglColor.BLACK): [15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0],
            (NbglColor.DARK_GRAY, NbglColor.WHITE): [5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 11, 12, 13, 14, 15],
            (NbglColor.LIGHT_GRAY, NbglColor.WHITE): [10, 10, 11, 11, 12, 12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 15],
            (NbglColor.LIGHT_GRAY, NbglColor.BLACK): [10, 9, 8, 7, 6, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0],
            (NbglColor.DARK_GRAY, NbglColor.BLACK): [10, 9, 8, 7, 6, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0],

            # Default computed color maps
            (NbglColor.BLACK, NbglColor.BLACK): [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            (NbglColor.BLACK, NbglColor.DARK_GRAY): [0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5, 5],
            (NbglColor.BLACK, NbglColor.LIGHT_GRAY): [0, 1, 2, 2, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 10],
            (NbglColor.DARK_GRAY, NbglColor.BLACK): [5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 1, 1, 1, 0, 0, 0],
            (NbglColor.DARK_GRAY, NbglColor.DARK_GRAY): [5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5],
            (NbglColor.DARK_GRAY, NbglColor.LIGHT_GRAY): [5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 9, 9, 9, 10, 10, 10],
            (NbglColor.LIGHT_GRAY, NbglColor.DARK_GRAY): [10, 10, 9, 9, 9, 8, 8, 8, 7, 7, 6, 6, 6, 5, 5, 5],
            (NbglColor.LIGHT_GRAY, NbglColor.LIGHT_GRAY):
                [10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10],
            (NbglColor.WHITE, NbglColor.DARK_GRAY): [15, 14, 13, 13, 12, 11, 11, 10, 9, 9, 8, 7, 7, 6, 5, 5],
            (NbglColor.WHITE, NbglColor.LIGHT_GRAY): [15, 15, 14, 14, 14, 13, 13, 13, 12, 12, 11, 11, 11, 10, 10, 10],
            (NbglColor.WHITE, NbglColor.WHITE): [15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15],
        }

        mapped_index = COLOR_MAPS_4BPP[(NbglColor(front_color), NbglColor(back_color))][index]
        return NBGL.to_screen_color(mapped_index, 4)

    @staticmethod
    def nbgl_bpp_to_read_bpp(abpp):
        if abpp == 0:
            bpp = 1
        elif abpp == 1:
            bpp = 2
        elif abpp == 2:
            bpp = 4
        else:
            return 0
        return bpp

    def draw_image(self, area, bpp, transformation, buffer, color_map):
        if self.force_full_ocr:
            # Avoid white on black text
            if (bpp == 4) and (color_map == NbglColor.WHITE) and (area.color == NbglColor.BLACK):
                area.color = NbglColor.WHITE
                color_map = NbglColor.BLACK
            elif bpp != 4 and color_map == 3:
                area.color = NbglColor.WHITE
                color_map = 0

        if transformation == 0:
            x = area.x0 + area.width - 1
            y = area.y0
        elif transformation == 1:  # h mirror
            x = area.x0 + area.width - 1
            y = area.y0 + area.height - 1
        elif transformation == 2:  # v mirror
            x = area.x0
            y = area.y0
        elif transformation == 3:  # hw mirror
            x = area.x0
            y = area.y0 + area.height - 1
        elif transformation == 4:  # 90 clockwise
            x = area.x0
            y = area.y0
        else:
            # error
            self.logger.error("Unknown transformation '%d'", transformation)
            sys.exit(-2)

        if bpp == 1:
            bit_step = 1
            mask = 0x1
            color_map = color_map << 2 | area.color
            bpp = 2
        elif bpp == 2:
            bit_step = 2
            mask = 0x3
        else:
            bit_step = 4
            mask = 0xF

        for byte in buffer:
            for i in range(0, 8, bit_step):
                nib = (byte >> (8-bit_step-i)) & mask
                if color_map and bpp < 4:
                    pixel_color = NBGL.get_color_from_color_map(nib, color_map, bpp)
                elif bpp == 4:
                    pixel_color = NBGL.get_4bpp_color_from_color_index(nib, color_map, area.color)
                else:
                    pixel_color = NBGL.to_screen_color(nib, bpp)

                self.fb.draw_point(x, y, pixel_color)

                if transformation == 0:
                    if y < area.y0 + area.height-1:
                        y = y + 1
                    else:
                        y = area.y0
                        x = x - 1
                elif transformation == 1:  # h mirror
                    if y > area.y0:
                        y = y - 1
                    else:
                        y = area.y0 + area.height - 1
                        x = x - 1
                elif transformation == 2:  # v mirror
                    if y < area.y0 + area.height - 1:
                        y = y + 1
                    else:
                        y = area.y0
                        x = x + 1
                elif transformation == 3:  # hw mirror
                    if y > area.y0:
                        y = y - 1
                    else:
                        y = area.y0 + area.height - 1
                        x = x + 1
                elif transformation == 4:  # 90 clockwise
                    if x < area.x0 + area.width:
                        x = x + 1
                    else:
                        x = area.x0
                        y = y + 1
                else:
                    # error
                    pass

    def hal_draw_image(self, data: bytes):
        area = nbgl_area_t.parse(data[0:nbgl_area_t.sizeof()])
        self.__assert_area(area)
        bpp = NBGL.nbgl_bpp_to_read_bpp(area.bpp)
        bit_size = (area.width * area.height * bpp)
        buffer_size = (bit_size // 8) + ((bit_size % 8) > 0)
        buffer = data[nbgl_area_t.sizeof(): nbgl_area_t.sizeof()+buffer_size]
        transformation: int = data[nbgl_area_t.sizeof()+buffer_size]
        color_map = data[nbgl_area_t.sizeof()+buffer_size + 1]  # front color in case of BPP4
        self.draw_image(area, bpp, transformation, buffer, color_map)

    def hal_draw_image_file(self, data):
        area = nbgl_area_t.parse(data[0:nbgl_area_t.sizeof()])
        self.__assert_area(area)
        data = data[nbgl_area_t.sizeof():]
        area.width = (data[1] << 8) | data[0]
        area.height = (data[3] << 8) | data[2]
        area.bpp = data[4] >> 4
        bpp = NBGL.nbgl_bpp_to_read_bpp(area.bpp)
        compression = data[4] & 0xF
        if compression:
            buffer_size = data[5] | (data[6] << 8) | (data[7] << 16)
        else:
            buffer_size = int((area.width * area.height * bpp) / 8)
        buffer = data[8: 8 + buffer_size]

        if not compression:
            data = nbgl_area_t.build(area) + data[8: 8 + buffer_size + 1]
            return self.hal_draw_image(data)
        output_buffer = []
        while len(buffer) > 0:
            compressed_chunk_len = 256 * buffer[1] + buffer[0]
            buffer = buffer[2:]
            output_buffer += gzip.decompress(bytes(buffer[:compressed_chunk_len]))
            buffer = buffer[compressed_chunk_len:]

        data = nbgl_area_t.build(area) + bytes(output_buffer) + b'\0' + data[-1].to_bytes(1, 'big')
        self.hal_draw_image(data)
        # decompress

    # -------------------------------------------------------------------------
    # All following RLE related methods can be found in the SDK (rle_custom.py)
    # -------------------------------------------------------------------------
    @staticmethod
    def remove_duplicates(pairs):
        """
        Check if there are some duplicated pairs (same values) and merge them.
        """
        index = len(pairs) - 1
        while index >= 1:
            repeat1, value1 = pairs[index-1]
            repeat2, value2 = pairs[index]
            # Do we have identical consecutives values?
            if value1 == value2:
                repeat1 += repeat2
                # Merge them and remove last entry
                pairs[index-1] = (repeat1, value1)
                pairs.pop(index)
            index -= 1

        return pairs

    # -------------------------------------------------------------------------
    @staticmethod
    def decode_pass1(data):
        """
        Decode array of tuples containing (repeat, val).
        Return an array of values.
        """
        output = []
        for repeat, value in data:
            for _ in range(repeat):
                output.append(value)

        return output

    # -------------------------------------------------------------------------
    def decode_pass2_4bpp(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        11RRRRRR
        10RRVVVV WWWWXXXX YYYYZZZZ
        0RRRVVVV
        With:
          * 11RRRRRR
            - RRRRRRR is repeat count - 1 of White (0xF) quartets (max=63+1)
          * 10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
            - RR is repeat count - 3 of quartets (max=3+3 => 6 quartets)
            - VVVV: value of 1st 4BPP pixel
            - WWWW: value of 2nd 4BPP pixel
            - XXXX: value of 3rd 4BPP pixel
            - YYYY: value of 4th 4BPP pixel
            - ZZZZ: value of 5th 4BPP pixel
            - QQQQ: value of 6th 4BPP pixel
          * 0RRRVVVV
            - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
            - VVVV: value of the 4BPP pixel
        """
        pairs = []
        index = 0
        while index < len(data):
            byte = data[index]
            index += 1
            # Is it a big duplication of whites or singles?
            if byte & 0x80:
                # Is it a big duplication of whites?
                if byte & 0x40:
                    # 11RRRRRR
                    byte &= 0x3F
                    repeat = 1 + byte
                    value = 0x0F
                # We need to decode singles
                else:
                    # 10RRVVVV WWWWXXXX YYYYZZZZ
                    count = (byte & 0x30)
                    count >>= 4
                    count += 3
                    value = byte & 0x0F
                    pairs.append((1, value))
                    count -= 1
                    while count > 0 and index < len(data):
                        byte = data[index]
                        index += 1
                        value = byte >> 4
                        value &= 0x0F
                        pairs.append((1, value))
                        count -= 1
                        if count > 0:
                            value = byte & 0x0F
                            pairs.append((1, value))
                            count -= 1
                    continue
            else:
                # 0RRRVVVV
                value = byte & 0x0F
                byte >>= 4
                byte &= 0x07
                repeat = 1 + byte

            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

    # -------------------------------------------------------------------------
    def decode_pass2_1bpp(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The provided packed values will contain ZZZZOOOO with
        - ZZZZ: number of consecutives 0, from 0 to 15
        - OOOO: number of consecutives 1, from 0 to 15
        """
        pairs = []
        for byte in data:
            ones = byte & 0x0F
            byte >>= 4
            zeros = byte & 0x0F
            if zeros:
                pairs.append((zeros, 0))
            if ones:
                pairs.append((ones, 1))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs
    # -------------------------------------------------------------------------
    @staticmethod
    def values_to_4bpp(data):
        """
        Takes values (assumed from 0x00 to 0x0F) in data and returns an array
        of bytes containing quartets with values concatenated.
        """
        output = bytes()
        remaining_values = len(data)
        index = 0
        while remaining_values > 1:
            byte = data[index] & 0x0F
            index += 1
            byte <<= 4
            byte |= data[index] & 0x0F
            index += 1
            remaining_values -= 2
            output += bytes([byte])

        # Is there a remaining quartet ?
        if remaining_values != 0:
            byte = data[index] & 0x0F
            byte <<= 4  # Store it in the MSB
            output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    @staticmethod
    def values_to_1bpp(data):
        """
        Takes values (bytes containing 0 or 1) in data and returns an array
        of bytes containing bits concatenated.
        (first pixel is bit 10000000 of first byte)
        """
        output = bytes()
        remaining_values = len(data)
        index = 0
        bits = 7
        byte = 0
        while remaining_values > 0:
            pixel = data[index] & 1
            index += 1
            byte |= pixel << bits
            bits -= 1
            if bits < 0:
                # We read 8 pixels: store them and get ready for next ones
                output += bytes([byte])
                bits = 7
                byte = 0
            remaining_values -= 1

        # Is there some remaining pixels stored?
        if bits != 7:
            output += bytes([byte])

        nb_bytes = len(data)//8
        if len(data) % 8:
            nb_bytes += 1
        assert len(output) == nb_bytes

        return output

    # -------------------------------------------------------------------------
    def decode_rle_4bpp(self, data):
        """
        Input: array of compressed bytes
        - decode to pairs using custom RLE
        - convert the tuples of (repeat, value) to values
        - convert to an array of packed pixels
        Output: array of packed pixels
        """
        pairs = self.decode_pass2_4bpp(data)
        values = self.decode_pass1(pairs)
        decoded = self.values_to_4bpp(values)

        return decoded

    # -------------------------------------------------------------------------
    def decode_rle_1bpp(self, data):
        """
        Input: array of compressed bytes
        - decode to pairs using custom RLE
        - convert the tuples of (repeat, value) to values
        - convert to an array of packed pixels
        Output: array of packed pixels
        """
        pairs = self.decode_pass2_1bpp(data)
        values = self.decode_pass1(pairs)
        decoded = self.values_to_1bpp(values)

        return decoded

    # -------------------------------------------------------------------------
    def hal_draw_image_rle(self, data):
        """
        Draw a bitmap (4BPP or 1BPP) which has been encoded via custom RLE.
        Input:
        data contains (check sys_nbgl_front_draw_img_rle in src/bolos/nbgl.c)
        - area (sizeof(nbgl_area_t))
        - compressed bitmap (buffer_len)
        - foreground_color (1 byte)
        - nb_skipped_bytes (1 byte)
        """
        area = nbgl_area_t.parse(data[0:nbgl_area_t.sizeof()])
        self.__assert_area(area)
        bitmap = data[nbgl_area_t.sizeof():-2]
        bpp = NBGL.nbgl_bpp_to_read_bpp(area.bpp)
        # We may have to skip initial transparent pixels (bytes, in that case)
        nb_skipped_bytes = data[nbgl_area_t.sizeof() + len(bitmap) + 1]
        # Uncompress RLE data into buffer
        if bpp == 4:
            buffer = bytes([0xFF] * nb_skipped_bytes)
            buffer += self.decode_rle_4bpp(bitmap)
        else:
            buffer = bytes([0x00] * nb_skipped_bytes)
            buffer += self.decode_rle_1bpp(bitmap)

        # Display the uncompressed image
        transformation = 0 # NO_TRANSFORMATION
        color_map = data[nbgl_area_t.sizeof() + len(bitmap)]  # front color in case of BPP4
        self.draw_image(area, bpp, transformation, buffer, color_map)
