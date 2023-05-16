from construct import Struct, Int8ul, Int16ul
from enum import IntEnum
import gzip


class NbglColor(IntEnum):
    BLACK = 0,
    DARK_GRAY = 1,
    LIGHT_GRAY = 2,
    WHITE = 3


nbgl_area_t = Struct(
    "x0" / Int16ul,
    "y0" / Int16ul,
    "width" / Int16ul,
    "height" / Int16ul,
    "color" / Int8ul,
    "bpp" / Int8ul,
)


class NBGL:
    def to_screen_color(color, bpp) -> int:
        if bpp == 2:
            return color * 0x555555
        if bpp == 1:
            return color * 0xFFFFFF
        if bpp == 4:
            return color * 0x111111

    def __init__(self, m, size, force_full_ocr, disable_tesseract):
        self.m = m
        # front screen dimension
        self.SCREEN_WIDTH, self.SCREEN_HEIGHT = size
        self.force_full_ocr = force_full_ocr
        self.disable_tesseract = disable_tesseract

    def __assert_area(self, area):
        if area.y0 % 4 or area.height % 4:
            raise AssertionError("X(%d) or height(%d) not 4 aligned " % (area.y0, area.height))
        if area.x0 > self.SCREEN_WIDTH or (area.x0+area.width) > self.SCREEN_WIDTH:
            raise AssertionError("left edge (%d) or right edge (%d) out of screen" % (area.x0, (area.x0 + area.width)))
        if area.y0 > self.SCREEN_HEIGHT or (area.y0+area.height) > self.SCREEN_HEIGHT:
            raise AssertionError("top edge (%d) or bottom edge (%d) out of screen" % (area.y0, (area.y0 + area.height)))

    def hal_draw_rect(self, data):
        area = nbgl_area_t.parse(data)
        if self.force_full_ocr:
            # We need all text shown in black with white background
            area.color = 3

        self.__assert_area(area)
        for x in range(area.x0, area.x0+area.width):
            for y in range(area.y0, area.y0+area.height):
                self.m.draw_point(x, y, NBGL.to_screen_color(area.color, 2))
        return

    def hal_refresh(self, data):
        area = nbgl_area_t.parse(data)
        self.__assert_area(area)
        self.m.update(area.x0, area.y0, area.width, area.height)

    def hal_draw_line(self, data):
        area = nbgl_area_t.parse(data[0:nbgl_area_t.sizeof()])
        self.__assert_area(area)
        mask = data[-2]
        color = data[-1]

        back_color = NBGL.to_screen_color(area.color, 2)
        front_color = NBGL.to_screen_color(color, 2)
        for x in range(area.x0, area.x0+area.width):
            for y in range(area.y0, area.y0+area.height):
                if (mask >> (y-area.y0)) & 0x1:
                    self.m.draw_point(x, y, front_color)
                else:
                    self.m.draw_point(x, y, back_color)

    def get_color_from_color_map(color, color_map, bpp):
        # #define GET_COLOR_MAP(__map__,__col__) ((__map__>>(__col__*2))&0x3)
        return NBGL.to_screen_color((color_map >> (color*2)) & 0x3, bpp)

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

    def hal_draw_image(self, data):
        area = nbgl_area_t.parse(data[0:nbgl_area_t.sizeof()])
        self.__assert_area(area)
        bpp = NBGL.nbgl_bpp_to_read_bpp(area.bpp)
        bit_size = (area.width * area.height * bpp)
        buffer_size = (bit_size // 8) + ((bit_size % 8) > 0)
        buffer = data[nbgl_area_t.sizeof(): nbgl_area_t.sizeof()+buffer_size]
        transformation = data[nbgl_area_t.sizeof()+buffer_size]
        color_map = data[nbgl_area_t.sizeof()+buffer_size + 1]  # front color in case of BPP4

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
            print(transformation)
            exit(-2)
            pass

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

                self.m.draw_point(x, y, pixel_color)

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
