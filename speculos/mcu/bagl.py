import binascii
import logging
from collections import namedtuple
from construct import Aligned, Struct, Int8ul, Int16ul, Int32ul, Padded

from . import bagl_font
from . import bagl_glyph
from .automation import TextEvent

bagl_component_t = Aligned(4, Struct(
    "type" / Int8ul,
    "userid" / Int8ul,
    "x" / Int16ul,
    "y" / Int16ul,
    "width" / Int16ul,
    "height" / Int16ul,
    "stroke" / Int8ul,
    "radius" / Int8ul,
    "fill" / Padded(4, Int8ul),
    "fgcolor" / Int32ul,
    "bgcolor" / Int32ul,
    "font_id" / Int16ul,
    "icon_id" / Int8ul,
))

BAGL_FILL = 1

BAGL_FILL_CIRCLE_1_OCTANT = 1
BAGL_FILL_CIRCLE_2_OCTANT = 2
BAGL_FILL_CIRCLE_3_OCTANT = 4
BAGL_FILL_CIRCLE_4_OCTANT = 8
BAGL_FILL_CIRCLE_5_OCTANT = 16
BAGL_FILL_CIRCLE_6_OCTANT = 32
BAGL_FILL_CIRCLE_7_OCTANT = 64
BAGL_FILL_CIRCLE_8_OCTANT = 128
BAGL_FILL_CIRCLE = 0xFF
BAGL_FILL_CIRCLE_3PI2_2PI = (BAGL_FILL_CIRCLE_1_OCTANT | BAGL_FILL_CIRCLE_2_OCTANT)
BAGL_FILL_CIRCLE_PI_3PI2 = (BAGL_FILL_CIRCLE_3_OCTANT | BAGL_FILL_CIRCLE_4_OCTANT)
BAGL_FILL_CIRCLE_0_PI2 = (BAGL_FILL_CIRCLE_5_OCTANT | BAGL_FILL_CIRCLE_6_OCTANT)
BAGL_FILL_CIRCLE_PI2_PI = (BAGL_FILL_CIRCLE_7_OCTANT | BAGL_FILL_CIRCLE_8_OCTANT)

BAGL_NONE = 0
BAGL_BUTTON = 1
BAGL_LABEL = 2
BAGL_RECTANGLE = 3
BAGL_LINE = 4
BAGL_ICON = 5
BAGL_CIRCLE = 6
BAGL_LABELINE = 7
BAGL_FLAG_TOUCHABLE = 0x80
BAGL_TYPE_FLAGS_MASK = 0x80

BAGL_FONT_ALIGNMENT_HORIZONTAL_MASK = 0xC000
BAGL_FONT_ALIGNMENT_LEFT = 0x0000
BAGL_FONT_ALIGNMENT_RIGHT = 0x4000
BAGL_FONT_ALIGNMENT_CENTER = 0x8000
BAGL_FONT_ALIGNMENT_VERTICAL_MASK = 0x3000
BAGL_FONT_ALIGNMENT_TOP = 0x0000
BAGL_FONT_ALIGNMENT_BOTTOM = 0x1000
BAGL_FONT_ALIGNMENT_MIDDLE = 0x2000

SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS_START = 0x00

DrawState = namedtuple('DrawState', 'x y width height colors bpp xx yy')


class Bagl:
    def __init__(self, m, size):
        self.m = m
        self.SCREEN_WIDTH, self.SCREEN_HEIGHT = size
        self.draw_state = DrawState(0, 0, 0, 0, [], 0, 0, 0)
        self.logger = logging.getLogger("bagl")

    def refresh(self) -> bool:
        return self.m.update()

    def hal_draw_bitmap_within_rect(self, x, y, width, height, colors, bpp, bitmap, restore=None):
        if bpp == 3 or bpp > 4:
            return

        if x >= self.SCREEN_WIDTH or y >= self.SCREEN_HEIGHT:
            return

        if x + width > self.SCREEN_WIDTH:
            width = self.SCREEN_WIDTH - x

        if y + height > self.SCREEN_HEIGHT:
            height = self.SCREEN_HEIGHT - y

        pixel_mask = (1 << bpp) - 1

        requested_xw = width + x
        requested_yh = height + y

        if not restore:
            xx, yy = x, y
        else:
            xx, yy = restore

        bitmap = list(bitmap)
        while bitmap:
            ch = bitmap.pop(0)
            for i in range(0, 8, bpp):
                if xx >= 0 and xx < self.SCREEN_WIDTH and yy >= 0 and yy < self.SCREEN_HEIGHT:
                    pixel_color_index = (ch >> i) & pixel_mask
                    color = colors[pixel_color_index]
                    self.m.draw_point(xx, yy, color)

                xx += 1
                if xx >= requested_xw:
                    xx = x
                    yy += 1
                    if yy >= requested_yh:
                        self.draw_state = DrawState(x, y, width, height, colors, bpp, xx, yy)
                        return

        self.draw_state = DrawState(x, y, width, height, colors, bpp, xx, yy)

    def hal_draw_rect(self, color, x, y, width, height):
        if x + width > self.SCREEN_WIDTH or x < 0:
            if x > self.SCREEN_WIDTH:
                return
            else:
                width = self.SCREEN_WIDTH - x
        if y + height > self.SCREEN_HEIGHT or y < 0:
            if y > self.SCREEN_HEIGHT:
                return
            else:
                height = self.SCREEN_HEIGHT - y

        YX = (y//8)*self.SCREEN_WIDTH + x
        YXlinemax = YX + width
        xx = x
        requested_x = x

        i = width * height
        while i > 0:
            i -= 1
            self.m.draw_point(xx, y, color)
            YX += 1
            xx += 1
            if YX >= YXlinemax:
                y += 1
                height -= 1
                YX = (y // 8) * self.SCREEN_WIDTH + x
                YXlinemax = YX + width
                xx = requested_x
            if height == 0:
                break

    @staticmethod
    def compute_line_width(font_id, width, text, text_encoding):
        font = bagl_font.get(font_id)
        if not font:
            logging.error("font not found")
            return 0

        xx = 0

        text = text.replace(b'\r\n', b'\n')
        for ch in text:
            ch_width = 0

            if ch < font.first_char or ch > font.last_char:
                if ch in ['\r', '\n']:
                    return xx

                if ch >= 0xc0:
                    ch_width = ch & 0x3f
                elif ch >= 0x80:
                    if ch & 0x20:
                        font_symbol_id = bagl_font.BAGL_FONT_SYMBOLS_1
                    else:
                        font_symbol_id = bagl_font.BAGL_FONT_SYMBOLS_0
                    font_symbols = bagl_font.get(font_symbol_id)
                    if font_symbols:
                        ch_width = font.characters[ch & 0x1f].char_width
            else:
                ch -= font.first_char
                ch_width = font.characters[ch].char_width

            if xx + ch_width > width and width > 0:
                return xx

            xx += ch_width

        return xx

    def draw_string(self, font_id, fgcolor, bgcolor, x, y, width, height, text, text_encoding):
        font = bagl_font.get(font_id)
        if not font:
            self.logger.error("unsupported font {}".format(font_id & bagl_font.BAGL_FONT_ID_MASK))
            return 0

        if font.bpp > 1:
            color_count = (1 << font.bpp) - 1
            colors = [bgcolor]
            for i in range(1, color_count):
                colors.append(0)

            for off in range(0, 3):
                cfg = (fgcolor >> (off * 8)) & 0xFF
                cbg = (bgcolor >> (off * 8)) & 0xFF

                crange = max(cfg, cbg) - min(cfg, cbg) + 1
                cinc = crange // color_count

                if cfg > cbg:
                    for i in range(1, color_count):
                        colors[i] |= min(0xff, cbg + i * cinc) << (off * 8)
                else:
                    for i in range(1, color_count):
                        colors[i] |= min(0xff, cfg + (color_count - i) * cinc) << (off * 8)

            colors.append(fgcolor)
        else:
            colors = [bgcolor, fgcolor]

        width += x
        height += y
        xx = x

        text = text.replace(b'\r\n', b'\n')

        for ch in text:
            ch_height = font.char_height
            ch_kerning = 0
            ch_width = 0
            ch_bitmap = None
            ch_y = y

            if ch < font.first_char or ch > font.last_char:
                if ch in ['\r', '\n']:
                    y += ch_height
                    if y + ch_height > height:
                        return (y << 16) | (xx & 0xFFFF)
                    xx = x
                    continue

                if ch >= 0xc0:
                    ch_width = ch & 0x3f
                elif ch >= 0x80:
                    if ch & 0x20:
                        font_symbol_id = bagl_font.BAGL_FONT_SYMBOLS_1
                    else:
                        font_symbol_id = bagl_font.BAGL_FONT_SYMBOLS_0
                    font_symbols = bagl_font.get(font_symbol_id)
                    if font_symbols:
                        ch_bitmap = font.bitmap[font.characters[ch & 0x1f].bitmap_offset:]
                        ch_width = font.characters[ch & 0x1f].char_width
                        ch_height = font_symbols.char_height
                        ch_y = y + font.baseline_height - font_symbols.baseline_height
            else:
                ch -= font.first_char
                ch_bitmap = font.bitmap[font.characters[ch].bitmap_offset:]
                ch_width = font.characters[ch].char_width
                ch_kerning = font.char_kerning

            if xx + ch_width > width:
                y += ch_height
                if y + ch_height > height:
                    return (y << 16) | (xx & 0xFFFF)
                xx = x
                ch_y = y

            if ch_bitmap:
                self.hal_draw_bitmap_within_rect(xx, ch_y, ch_width, ch_height, colors, font.bpp, ch_bitmap)
            else:
                self.hal_draw_rect(bgcolor, xx, ch_y, ch_width, ch_height)

            xx += ch_width + ch_kerning

        return (y << 16) | (xx & 0xFFFF)

    def _draw_circle_helper(self, color, x_center, y_center, radius, octants, radiusint, colorint):
        x, y = radius, 0
        decisionOver2 = 1 - x
        dradius = radius-radiusint
        last_x = x
        drawint = (radiusint > 0 and dradius > 0)

        while y <= x:
            if octants & 1:
                if drawint:
                    self.hal_draw_rect(colorint, x_center,   y+y_center, x-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center+x-(dradius-1), y+y_center, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center,   y+y_center-1, x, 1)
            if octants & 2:
                if drawint:
                    if last_x != x:
                        self.hal_draw_rect(colorint, x_center,   x+y_center, y-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center+y-(dradius-1), x+y_center, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center,   x+y_center-1, y, 1)
            if octants & 4:
                if drawint:
                    self.hal_draw_rect(colorint, x_center-x, y+y_center, x-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center-x-(dradius-1), y+y_center, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center-x, y+y_center-1, x, 1)
            if octants & 8:
                if drawint:
                    if last_x != x:
                        self.hal_draw_rect(colorint, x_center-y, x+y_center, y-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center-y-(dradius-1), x+y_center, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center-y, x+y_center-1, y, 1)
            if octants & 16:
                if drawint:
                    self.hal_draw_rect(colorint, x_center,   y_center-y, x-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center+x-(dradius-1), y_center-y, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center,   y_center-y, x, 1)
            if octants & 32:
                if drawint:
                    if last_x != x:
                        self.hal_draw_rect(colorint, x_center,   y_center-x, y-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center+y-(dradius-1), y_center-x, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center,   y_center-x, y, 1)
            if octants & 64:
                if drawint:
                    self.hal_draw_rect(colorint, x_center-x, y_center-y, x-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center-x-(dradius-1), y_center-y, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center-x, y_center-y, x, 1)
            if octants & 128:
                if drawint:
                    if last_x != x:
                        self.hal_draw_rect(colorint, x_center-y, y_center-x, y-(dradius-1), 1)
                    self.hal_draw_rect(color, x_center-y-(dradius-1), y_center-x, dradius, 1)
                else:
                    self.hal_draw_rect(color, x_center-y, y_center-x, y, 1)

            last_x = x
            y += 1
            if decisionOver2 <= 0:
                decisionOver2 += 2 * y + 1
            else:
                x -= 1
                decisionOver2 += 2 * (y - x) + 1

    def _display_bagl_icon(self, component, context):
        if component.icon_id != 0:
            self.logger.debug(f"icon_id {component.icon_id}")
            glyph = bagl_glyph.get(component.icon_id)
            if not glyph:
                self.logger.error(f"glyph {component.icon_id:#x} not found")
                return

            if len(context) != 0:
                # assert (1 << glyph.bpp) * 4 == len(context)
                colors = []
                for i in range(0, 1 << glyph.bpp):
                    color = int.from_bytes(context[i*4:(i+1)*4], byteorder='big')
                    colors.append(color)
            else:
                colors = glyph.colors

            self.hal_draw_bitmap_within_rect(
                component.x + (component.width // 2 - glyph.width // 2),
                component.y + (component.height // 2 - glyph.height // 2),
                glyph.width,
                glyph.height,
                colors,
                glyph.bpp,
                glyph.bitmap)
        else:
            if len(context) == 0:
                self.logger.info("len context == 0 {}".format(binascii.hexlify(context)))
                return

            bpp = context[0]
            if bpp > 2:
                return
            colors = []
            n = 1
            for i in range(0, 1 << bpp):
                color = int.from_bytes(context[n:n+4], byteorder='big')
                colors.append(color)
                n += 4
            bitmap = context[n:]
            bitmap_length_bits = bpp * component.width * component.height
            assert len(bitmap) * 8 >= bitmap_length_bits
            self.hal_draw_bitmap_within_rect(component.x, component.y,
                                             component.width, component.height,
                                             colors,
                                             bpp,
                                             bitmap)

    def _display_bagl_rectangle(self, component, context, context_encoding, halignment, valignment):
        radius = min(component.radius, min(component.width//2, component.height//2))
        if component.fill != BAGL_FILL:
            coords = [
                # centered top to bottom
                (component.x+radius, component.y, component.width-2*radius, component.height),
                # left to center rect
                (component.x, component.y+radius, radius, component.height-2*radius),
                # center rect to right
                (component.x+component.width-radius-1, component.y+radius, radius, component.height-2*radius),
            ]
            for (x, y, width, height) in coords:
                self.hal_draw_rect(component.bgcolor, x, y, width, height)
            coords = [
                # outline. 4 rectangles (with last pixel of each corner not set)
                # top, bottom, left, right
                (component.x+radius, component.y, component.width-2*radius, component.stroke),
                (component.x+radius, component.y+component.height-1, component.width-2*radius, component.stroke),
                (component.x, component.y+radius, component.stroke, component.height-2*radius),
                (component.x+component.width-1, component.y+radius, component.stroke, component.height-2*radius),
            ]
            for (x, y, width, height) in coords:
                self.hal_draw_rect(component.fgcolor, x, y, width, height)
        else:
            coords = [
                # centered top to bottom
                (component.x+radius, component.y, component.width-2*radius, component.height),
                # left to center rect
                (component.x, component.y+radius, radius, component.height-2*radius),
                # center rect to right
                (component.x+component.width-radius, component.y+radius, radius, component.height-2*radius),
            ]
            for (x, y, width, height) in coords:
                self.hal_draw_rect(component.fgcolor, x, y, width, height)

        if radius > 1:
            radiusint = 0
            if component.fill != BAGL_FILL and component.stroke < radius:
                radiusint = radius - component.stroke
            self._draw_circle_helper(component.fgcolor,
                                     component.x + radius,
                                     component.y + radius,
                                     radius,
                                     BAGL_FILL_CIRCLE_PI2_PI,
                                     radiusint,
                                     component.bgcolor)
            self._draw_circle_helper(component.fgcolor,
                                     component.x + component.width - radius - component.stroke,
                                     component.y + radius,
                                     radius,
                                     BAGL_FILL_CIRCLE_0_PI2,
                                     radiusint,
                                     component.bgcolor)
            self._draw_circle_helper(component.fgcolor,
                                     component.x + radius,
                                     component.y + component.height - radius - component.stroke,
                                     radius,
                                     BAGL_FILL_CIRCLE_PI_3PI2,
                                     radiusint,
                                     component.bgcolor)
            self._draw_circle_helper(component.fgcolor,
                                     component.x + component.width - radius - component.stroke,
                                     component.y + component.height - radius - component.stroke,
                                     radius,
                                     BAGL_FILL_CIRCLE_3PI2_2PI,
                                     radiusint,
                                     component.bgcolor)

        if context:
            fgcolor, bgcolor = component.fgcolor, component.bgcolor
            if component.fill == BAGL_FILL:
                fgcolor, bgcolor = bgcolor, fgcolor
            stroke = max(1, component.stroke * 2)
            self.draw_string(component.font_id,
                             fgcolor,
                             bgcolor,
                             component.x + halignment,	         	 # XXX: take icon_width into account
                             component.y + valignment,
                             component.width - halignment - stroke,	 # XXX: take icon_width into account
                             component.height - valignment - stroke,
                             context,
                             context_encoding)

    def _display_bagl_labeline(self, component, text, halignment, valignment, baseline, char_height, strwidth, type_):
        if component.fill == BAGL_FILL:
            y = component.y
            height = component.height
            if type_ == BAGL_LABELINE:
                y -= baseline
                height = char_height
            self.hal_draw_rect(component.bgcolor, component.x, y, halignment, height)
            self.hal_draw_rect(component.bgcolor, component.x + halignment + strwidth,
                               y, component.width - (halignment + strwidth), height)

        if len(text) == 0:
            return

        # XXX
        context_encoding = 0
        y = component.y
        height = component.height
        if type_ == BAGL_LABELINE:
            y -= baseline
        else:
            y += valignment
            height += valignment
        self.draw_string(component.font_id,
                         component.fgcolor,
                         component.bgcolor,
                         component.x + halignment,
                         y,
                         component.width - halignment,
                         component.height,
                         text,
                         context_encoding)

        return [TextEvent(text.decode("utf-8", "ignore"), component.x + halignment, y)]

    def _display_get_alignment(self, component, context, context_encoding):
        halignment = 0
        valignment = 0
        baseline = 0
        char_height = 0
        strwidth = 0

        if component.type == BAGL_ICON:
            return (halignment, valignment, baseline, char_height, strwidth)

        font = bagl_font.get(component.font_id)
        if font:
            baseline = font.baseline_height
            char_height = font.char_height

            if context:
                strwidth = Bagl.compute_line_width(component.font_id,
                                                   component.width + 100,
                                                   context,
                                                   context_encoding)

        haligned = (component.font_id & BAGL_FONT_ALIGNMENT_HORIZONTAL_MASK)
        if haligned == BAGL_FONT_ALIGNMENT_RIGHT:
            halignment = component.width - strwidth
        elif haligned == BAGL_FONT_ALIGNMENT_CENTER:
            halignment = component.width // 2 - strwidth // 2
        elif haligned == BAGL_FONT_ALIGNMENT_LEFT:
            halignment = 0
        else:
            halignment = 0

        valigned = (component.font_id & BAGL_FONT_ALIGNMENT_VERTICAL_MASK)
        if valigned == BAGL_FONT_ALIGNMENT_BOTTOM:
            valignment = component.height - baseline
        elif valigned == BAGL_FONT_ALIGNMENT_MIDDLE:
            valignment = component.height // 2 - baseline // 2 - 1
        elif valigned == BAGL_FONT_ALIGNMENT_TOP:
            valignment = 0
        else:
            valignment = 0

        return (halignment, valignment, baseline, char_height, strwidth)

    def display_status(self, data):
        component = bagl_component_t.parse(data)
        context = data[bagl_component_t.sizeof():]
        context_encoding = 0  # XXX
        self.logger.debug("component: {}".format(component))

        ret = self._display_get_alignment(component, context, context_encoding)
        (halignment, valignment, baseline, char_height, strwidth) = ret

        ret = None
        type_ = component.type & (~BAGL_TYPE_FLAGS_MASK)
        if type_ == BAGL_NONE:
            # TODO
            # self.renderer.clear()
            pass
        elif type_ == BAGL_RECTANGLE:
            self._display_bagl_rectangle(component, context, context_encoding, halignment, valignment)
        elif type_ == BAGL_LABEL:
            self._display_bagl_labeline(component, context, halignment, valignment, baseline, char_height,
                                        strwidth, type_)
        elif type_ == BAGL_LABELINE:
            ret = self._display_bagl_labeline(component, context, halignment, valignment, baseline, char_height,
                                              strwidth, type_)
        elif type_ == BAGL_ICON:
            self._display_bagl_icon(component, context)
        return ret

    def display_raw_status(self, data):
        if data[0] == SEPROXYHAL_TAG_SCREEN_DISPLAY_RAW_STATUS_START:
            x = int.from_bytes(data[1:3], byteorder='big', signed=True)
            y = int.from_bytes(data[3:5], byteorder='big', signed=True)
            w = int.from_bytes(data[5:7], byteorder='big')
            h = int.from_bytes(data[7:9], byteorder='big')
            bpp = int.from_bytes(data[9:10], byteorder='big')

            color_size = 4 * (1 << bpp)
            colors = []
            for i in range(10, 10 + color_size, 4):
                color = int.from_bytes(data[i:i+4], byteorder='little')
                colors.append(color)
            bitmap = data[10+color_size:]

            self.hal_draw_bitmap_within_rect(x, y, w, h, colors, bpp, bitmap)

        else:
            bitmap = data[1:]

            x = self.draw_state.x
            y = self.draw_state.y
            w = self.draw_state.width
            h = self.draw_state.height
            colors = self.draw_state.colors
            bpp = self.draw_state.bpp
            restore = (self.draw_state.xx, self.draw_state.yy)

            self.hal_draw_bitmap_within_rect(x, y, w, h, colors, bpp, bitmap, restore)
