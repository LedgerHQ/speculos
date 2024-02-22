#!/usr/bin/env python3
# coding: utf-8
# -----------------------------------------------------------------------------
"""
This module contain tools to test different custom RLE coding/decoding.
"""
# -----------------------------------------------------------------------------
import argparse
import sys


# -----------------------------------------------------------------------------
# Regular RLE encoding
# -----------------------------------------------------------------------------
class RLECustomBase:
    """
    Class handling regular RLE encoding (1st pass)
    """
    CMD_FILL = 0
    CMD_COPY = 1

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__()

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        self.encoded = None

    # -------------------------------------------------------------------------
    def __enter__(self):
        """
        Return an instance to this object.
        """
        return self

    # -------------------------------------------------------------------------
    def __exit__(self, exec_type, exec_value, traceback):
        """
        Do all the necessary cleanup.
        """

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
    def bpp4_to_values(data):
        """
        Expand each bytes of data into 2 quartets.
        Return an array of values (from 0x00 to 0x0F)
        """
        output = []
        for byte in data:
            lsb_bpp4 = byte & 0x0F
            byte >>= 4
            output.append(byte & 0x0F)
            output.append(lsb_bpp4)

        return output

    # -------------------------------------------------------------------------
    @staticmethod
    def values_to_bpp4(data):
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
    def bpp1_to_values(data):
        """
        Expand each bytes of data into 8 bits, each stored in a byte
        Return an array of values (containing bytes values 0 or 1)
        """
        output = []
        for byte in data:
            # first pixel is in bit 10000000
            for shift in range(7, -1, -1):
                pixel = byte >> shift
                pixel &= 1
                output.append(pixel)

        assert len(output) == (len(data)*8)

        return output

    # -------------------------------------------------------------------------
    @staticmethod
    def values_to_bpp1(data):
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

        nb_bytes = (len(data)+7)//8

        assert len(output) == nb_bytes

        return output

    # -------------------------------------------------------------------------
    def bpp_to_values(self, data):
        """
        Expand each bytes of data, containing pixels (4BPP or 1BPP)
        Return an array of values (0x00 to 0x0F or 0x00 to 0x01)
        """
        if self.bpp == 4:
            return self.bpp4_to_values(data)

        return self.bpp1_to_values(data)

    # -------------------------------------------------------------------------
    def values_to_bpp(self, data):
        """
        Takes values (0x00 to 0x0F or 0x00 to 0x01) in data and returns an
        array of bytes containing pixels concatenated.
        """
        if self.bpp == 4:
            return self.values_to_bpp4(data)

        return self.values_to_bpp1(data)

    # -------------------------------------------------------------------------
    @staticmethod
    def encode_pass2(pairs, max_count=128):
        """
        Encode tuples containing (repeat, val) into packed values.
        (empty method intended to be overwritten)
        """
        if max_count:
            pass

        return pairs

    # -------------------------------------------------------------------------
    @staticmethod
    def decode_pass2(data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        (empty method intended to be overwritten)
        """
        return data

    # -------------------------------------------------------------------------
    def encode_pass1(self, data):
        """
        Encode array of values using 'normal' RLE.
        Return an array of tuples containing (repeat, val)
        """
        output = []
        previous_value = -1
        count = 0
        for value in data:
            # Same value than before?
            if value == previous_value:
                count += 1
            else:
                # Store previous result
                if count:
                    pair = (count, previous_value)
                    output.append(pair)
                # Start a new count
                previous_value = value
                count = 1

        # Store previous result
        if count:
            pair = (count, previous_value)
            output.append(pair)

        if self.verbose:
            sys.stdout.write(f"Nb values: {len(output)}\n")
            for repeat, value in output:
                sys.stdout.write(f"{repeat:3d} x 0x{value:02X}\n")

        return output

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
    def encode(self, data):
        """
        Input: array of packed pixels
        - convert to an array of values
        - convert to tuples of (repeat, value)
        - encode using custom RLE
        Output: array of compressed bytes
        """
        values = self.bpp_to_values(data)
        pairs = self.encode_pass1(values)

        # Second pass
        self.encoded = self.encode_pass2(pairs)

        # Decode to check everything is fine
        decompressed = self.decode(self.encoded)
        if decompressed != data:
            # This will never happen in prod environment, only when testing
            sys.stdout.write("Encoding/Decoding is WRONG!!!\n")
            sys.exit(111)

        return self.encoded

    # -------------------------------------------------------------------------
    def decode(self, data):
        """
        Input: array of compressed bytes
        - decode to pairs using custom RLE
        - convert the tuples of (repeat, value) to values
        - convert to an array of packed pixels
        Output: array of packed pixels
        """
        pairs = self.decode_pass2(data)
        values = self.decode_pass1(pairs)

        decoded = self.values_to_bpp(values)

        return decoded

    # -------------------------------------------------------------------------
    @staticmethod
    def get_encoded_size(data):
        """
        Return the encoded size.
        (to be overwritten if something more complex than len() need to be done)
        """
        return len(data)


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustom1 (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    * for 1 BPP: RRRRRRRV
      - RRRRRRR: repeat count - 1 => allow to store 1 to 256 repeat counts
      - V: value of the 1BPP pixel (0 or 1)
    * for 4 BPP: RRRRVVVV
    - RRRR: repeat count - 1 => allow to store 1 to 16 repeat counts
    - VVVV: value of the 4BPP pixel
    """
    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    @staticmethod
    def encode_pass2(pairs, max_count=16):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
        RRRRVVVV
        - RRRR: repeat count - 1 => allow to store 1 to 16 repeat counts
        - VVVV: value of the 4BPP pixel
        """
        output = bytes()
        for repeat, value in pairs:
            # We can't store more than a repeat count of max_count
            while repeat > max_count:
                count = max_count - 1
                byte = count << 4
                byte |= value & 0x0F
                output += bytes([byte])
                repeat -= max_count
            if repeat:
                count = repeat - 1
                byte = count << 4
                byte |= value & 0x0F
                output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        RRRRVVVV
        - RRRR: repeat count - 1 => allow to store 1 to 16 repeat counts
        - VVVV: value of the 4BPP pixel
        """
        pairs = []
        for byte in data:
            value = byte & 0x0F
            byte >>= 4
            byte &= 0x0F
            repeat = 1 + byte
            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte +
# - white handling
# -----------------------------------------------------------------------------
class RLECustom2 (RLECustom1):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    1XXXXXXX
    0XXXXXXX
    With:
    * 1RRRRRRR
        - RRRRRRRR is repeat count - 1 of White (0xF) quartets
    * 0RRRVVVV
        - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
        - VVVV: value of the 4BPP pixel
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    @staticmethod
    def encode_pass2(pairs, max_count=128):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
        1XXXXXXX
        0XXXXXXX
        With:
          * 1RRRRRRR
            - RRRRRRRR is repeat count - 1 of White (0xF) quartets (max=127+1)
          * 0RRRVVVV
            - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
            - VVVV: value of the 4BPP pixel
        """
        output = bytes()
        for repeat, value in pairs:
            # Handle white
            if value == 0x0F:
                # We can't store more than a repeat count of 128
                while repeat > max_count:
                    byte = 0x80
                    byte |= max_count - 1
                    output += bytes([byte])
                    repeat -= max_count
                if repeat:
                    byte = 0x80
                    byte |= repeat - 1
                    output += bytes([byte])
            else:
                # We can't store more than a repeat count of 8
                while repeat > 8:
                    count = 8 - 1
                    byte = count << 4
                    byte |= value & 0x0F
                    output += bytes([byte])
                    repeat -= 8
                if repeat:
                    count = repeat - 1
                    byte = count << 4
                    byte |= value & 0x0F
                    output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        1XXXXXXX
        0XXXXXXX
        With:
          * 1RRRRRRR
            - RRRRRRRR is repeat count - 1 of White (0xF) quartets
          * 0RRRVVVV
            - RRR: repeat count - 1 => allow to store 1 to 8 repeat counts
            - VVVV: value of the 4BPP pixel
        """
        pairs = []
        for byte in data:
            # Is it a big duplication of whites ?
            if byte & 0x80:
                byte &= 0x7F
                repeat = 1 + byte
                value = 0x0F
            else:
                value = byte & 0x0F
                byte >>= 4
                byte &= 0x07
                repeat = 1 + byte
            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte +
# - white handling
# - copy range of bytes
# -----------------------------------------------------------------------------
class RLECustom3 (RLECustom2):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    11RRRRRR
    10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
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

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    @staticmethod
    def encode_pass2(pairs, max_count=64):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
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
        # First, generate data in 10RRRRRR/0RRRVVVV format
        single_output = RLECustom2.encode_pass2(pairs, max_count)

        # Now, parse array to find consecutives singles (0000VVVV)
        output = bytes()
        index = 0
        while index < len(single_output):
            # Do we have some 'singles'?
            byte = single_output[index]
            if (byte & 0xF0) != 0x00:
                # No, just store it
                if byte & 0x80:
                    byte |= 0x40    # 11RRRRRR
                output += bytes([byte])
                index += 1
            else:
                # Check how many 'singles' we have
                count = 1
                while ((index+count) < len(single_output)) and \
                      (single_output[index+count] & 0xF0) == 0x00:
                    count += 1
                # No more than 6 singles
                if count > 6:
                    # Special case: if count = 8 then do 5+3
                    if count == 8:
                        count = 5       # to allow storing next 3 singles!!
                    else:
                        count = 6
                # Do we have at least 3 singles?
                if count >= 3:
                    # Store 10RRVVVV WWWWXXXX YYYYZZZZ QQQQ0000
                    # 3 singles => 1 + 1 bytes
                    # 4 singles => 1 + 2 bytes (1 quartet unused)
                    # 5 singles => 1 + 2 bytes
                    # 6 singles => 1 + 3 bytes (1 quartet unused)
                    msb_byte = count - 3
                    msb_byte <<= 4
                    msb_byte |= 0x80
                    byte |= msb_byte
                    # Store first byte
                    output += bytes([byte])
                    index += 1
                    count -= 1
                    while count > 0:
                        byte = single_output[index]         # No need to mask
                        index += 1
                        count -= 1
                        byte <<= 4
                        # Do we have an other quartet?
                        if count > 0:
                            byte |= single_output[index]    # No need to mask
                            index += 1
                            count -= 1
                        # Store the quartet(s)
                        output += bytes([byte])
                else:
                    # No, just store that single
                    output += bytes([byte])
                    index += 1

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
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


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustom4 (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain:
    RRRRRRRV
    - RRRRRRRRR: repeat count - 1 => allow to store 1 to 128 repeat counts
    - V: value of the 1BPP pixel
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

    # -------------------------------------------------------------------------
    @staticmethod
    def encode_pass2(pairs, max_count=128):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated bytes will contain:
        RRRRRRRV
        - RRRRRRRRR: repeat count - 1 => allow to store 1 to 128 repeat counts
        - V: value of the 1BPP pixel
        """
        output = bytes()
        for repeat, value in pairs:
            # We can't store more than a repeat count of max_count
            while repeat > max_count:
                count = max_count - 1
                byte = count << 1
                byte |= value & 1
                output += bytes([byte])
                repeat -= max_count
            if repeat:
                count = repeat - 1
                byte = count << 1
                byte |= value & 1
                output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The bytes provided contains:
        RRRRRRRV
        - RRRRRRRRR: repeat count - 1 => allow to store 1 to 128 repeat counts
        - V: value of the 1BPP pixel
        """
        pairs = []
        for byte in data:
            value = byte & 1
            byte >>= 1
            byte &= 0x7F
            repeat = 1 + byte
            pairs.append((repeat, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustomN (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass)
    The generated bytes will contain triplets with:
    - Command (Fill or Copy)
    - Repeat count
    - Value: value(s) to be Filled/Copied
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        self.commands = []
        self.repeat = []
        self.values = []

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=64):
        """
        Encode tuples containing (repeat, val) into packed values.
        The generated packed values will contain:
        - Command (Fill or Copy)
        - Repeat count
        - Value: value(s) to be Filled/Copied
        """
        infos = []
        index = 0
        if self.bpp == 1:
            # threshold = 7
            threshold = 2
        else:
            threshold = 3

        while index < len(pairs):
            repeat, value = pairs[index]
            index += 1
            # We can't store more than a repeat count of max_count
            while repeat >= max_count:
                pixels = [value]
                info = (self.CMD_FILL, max_count, pixels)
                infos.append(info)
                repeat -= max_count

            # Is it still interesting to use FILL commands?
            if repeat >= threshold:
                pixels = [value]
                info = (self.CMD_FILL, repeat, pixels)
                infos.append(info)
            # No, use COPY commands
            elif repeat:
                pixels = []
                for _ in range(repeat):
                    pixels.append(value)

                # Can we merge next pixels into this COPY command?
                while index < len(pairs) and pairs[index][0] < threshold:
                    repeat2, value2 = pairs[index]
                    index += 1

                    while repeat2 > 0:
                        pixels.append(value2)
                        repeat2 -= 1

                        if len(pixels) == max_count:
                            info = (self.CMD_COPY, len(pixels), pixels)
                            infos.append(info)
                            pixels = []

                # Store remaining pixels
                if len(pixels) > 0:
                    info = (self.CMD_COPY, len(pixels), pixels)
                    infos.append(info)

        if self.verbose:
            sys.stdout.write(f"Nb values: {len(infos)}\n")
            for command, repeat, pixels in infos:
                if command == self.CMD_FILL:
                    value = pixels[0]
                    sys.stdout.write(f"FILL {repeat:3d} x 0x{value:02X}\n")
                else:
                    sys.stdout.write(f"COPY {repeat:3d} x {pixels}\n")

        return infos

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The provided packed values will contain:
        - Command (Fill or Copy)
        - Repeat count
        - Value: value(s) to be Filled/Copied
        """
        pairs = []
        for command, repeat, pixels in data:
            if command == self.CMD_FILL:
                value = pixels[0]
                pairs.append((repeat, value))
            else:
                # Store pixel by pixel => duplicates will be merged later
                for value in pixels:
                    pairs.append((1, value))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs

    # -------------------------------------------------------------------------
    def get_encoded_size(self, data):
        """
        Return the encoded size.
        """
        # Size needed to store the commands
        cmd_size = len(data) // 8
        if len(data) % 8:
            cmd_size += 1

        # Size needed to store the repeat count
        count_size = len(data) // 2
        if len(data) % 2:
            count_size += 1

        # Compute size needed by pixels
        total_pixels = 0
        bits_for_pixels = 0
        for command, repeat, pixels in data:
            total_pixels += repeat
            if command == self.CMD_FILL:
                bits_for_pixels += self.bpp
            else:
                bits_for_pixels += self.bpp * repeat
                assert repeat == len(pixels)

        # Size needed by pixels
        pixels_size = bits_for_pixels // 8
        if bits_for_pixels % 8:
            pixels_size += 1

        sys.stdout.write(f"Nb pixels: {total_pixels}\n")
        sys.stdout.write(f"sizes: cmd={cmd_size}, count={count_size}"
                         f", data={pixels_size}\n")

        return cmd_size + count_size + pixels_size


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustomA (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass) for 1 BPP only
    The generated bytes will contain Alternances counts ZZZZOOOO with
    - ZZZZ: number of consecutives 0, from 0 to 15
    - OOOO: number of consecutives 1, from 0 to 15
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        if self.bpp != 1:
            sys.stdout.write("The class RLECustomA is for 1 BPP data only!\n")
            sys.exit(1)

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=15):
        """
        Encode Alternance counts between pixels (nb of 0, then nb of 1, etc)
        The generated packed values will contain ZZZZOOOO ZZZZOOOO with
        - ZZZZ: number of consecutives 0, from 0 to 15
        - OOOO: number of consecutives 1, from 0 to 15
        """
        # First step: store alternances, and split if count > 15
        next_pixel = 0
        alternances = []
        for repeat, value in pairs:
            # Store a count of 0 pixel if next pixel is not the one expected
            if value != next_pixel:
                alternances.append(0)
                next_pixel ^= 1

            while repeat > max_count:
                # Store 15 pixels of value next_pixel
                alternances.append(max_count)
                repeat -= max_count
                # Store 0 pixel of alternate value
                alternances.append(0)

            if repeat:
                alternances.append(repeat)
                next_pixel ^= 1

        if False and self.verbose:
            sys.stdout.write(f"Nb values: {len(alternances)}\n")
            next_pixel = 0
            for repeat in alternances:
                sys.stdout.write(f"{repeat:2d} x {next_pixel}\n")
                next_pixel ^= 1

        # Now read all those values and store them into quartets
        output = bytes()
        index = 0

        while index < len(alternances):
            zeros = alternances[index]
            index += 1
            if index < len(alternances):
                ones = alternances[index]
                index += 1
            else:
                ones = 0

            byte = (zeros << 4) | ones
            output += bytes([byte])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
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


# -----------------------------------------------------------------------------
# Custom RLE encoding: pack repeat count & value into 1 byte
# -----------------------------------------------------------------------------
class RLECustomB (RLECustomBase):
    """
    Class handling custom RLE encoding (2nd pass) for 1 BPP only
    The generated bytes will contain Alternances counts ZZZZZZZZ OOOOOOOO with
    - ZZZZZZZZ: number of consecutives 0, from 0 to 255
    - OOOOOOOO: number of consecutives 1, from 0 to 255
    TODO: check the same with 6 bits for repeat count
    ZZZZZZOO OOOOZZZZ ZZOOOOOO
    - ZZZZZZ: number of consecutives 0, from 0 to 63
    - OOOOOO: number of consecutives 1, from 0 to 63
    """

    # -------------------------------------------------------------------------
    def __init__(self, bpp=1, verbose=False):
        super().__init__(bpp, verbose)

        # Store parameters:
        self.bpp = bpp
        self.verbose = verbose

        if self.bpp != 1:
            sys.stdout.write("The class RLECustomB is for 1 BPP data only!\n")
            sys.exit(1)

    # -------------------------------------------------------------------------
    def encode_pass2(self, pairs, max_count=255):
        """
        Encode Alternance counts between pixels (nb of 0, then nb of 1, etc)
        The generated packed values will contain ZZZZZZZZ OOOOOOOO with
        - ZZZZZZZZ: number of consecutives 0, from 0 to 255
        - OOOOOOOO: number of consecutives 1, from 0 to 255
        """
        # First step: store alternances, and split if count > 255
        next_pixel = 0
        alternances = []
        for repeat, value in pairs:
            # Store a count of 0 pixel if next pixel is not the one expected
            if value != next_pixel:
                alternances.append(0)
                next_pixel ^= 1

            while repeat > max_count:
                # Store 255 pixels of value next_pixel
                alternances.append(max_count)
                repeat -= max_count
                # Store 0 pixel of alternate value
                alternances.append(0)

            if repeat:
                alternances.append(repeat)
                next_pixel ^= 1

        if self.verbose:
            sys.stdout.write(f"Nb values: {len(alternances)}\n")

        if False and self.verbose:
            sys.stdout.write(f"Nb values: {len(alternances)}\n")
            next_pixel = 0
            for repeat in alternances:
                sys.stdout.write(f"{repeat:2d} x {next_pixel}\n")
                next_pixel ^= 1

        # Now read all those values and store them into bytes
        output = bytes()
        index = 0

        while index < len(alternances):
            zeros = alternances[index]
            index += 1
            if index < len(alternances):
                ones = alternances[index]
                index += 1
            else:
                ones = 0

            output += bytes([zeros])
            output += bytes([ones])

        return output

    # -------------------------------------------------------------------------
    def decode_pass2(self, data):
        """
        Decode packed bytes into array of tuples containing (repeat, value).
        The generated bytes will contain Alternances counts ZZZZZZZZ OOOOOOOO with
        - ZZZZZZZZ: number of consecutives 0, from 0 to 255
        - OOOOOOOO: number of consecutives 1, from 0 to 255
        """
        pairs = []
        for index, repeat in enumerate(data):
            if index & 1:
                pairs.append((repeat, 1))
            else:
                pairs.append((repeat, 0))

        # There was a limitation on repeat count => remove duplicates
        pairs = self.remove_duplicates(pairs)

        return pairs


# -----------------------------------------------------------------------------
# Entry point for easy RLE encoding/decoding
# -----------------------------------------------------------------------------
class RLECustom:
    """
    Class handling custom RLE encoding
    """
    # -------------------------------------------------------------------------
    @classmethod
    def encode(cls, packed_pixels, bpp, verbose=False):
        """
        Try different encoding algorithms to compress 4 or 1BPP pixels.
        Input:
        - array of packed pixels
        - number of bpp (1 or 4)
        Output: Tuple containing:
        - method used to encode data
        - bytes array containing encoded data
        """
        # Default values: no encoding
        method = 0
        compressed = packed_pixels
        # Try different methods depending on BPP
        if bpp == 4:
            # For now, just compress 4 BPP bitmap with RLECustom3
            rle = RLECustom3(bpp, verbose)
            compressed = rle.encode(packed_pixels)
            method = 1

        elif bpp == 1:
            # For now, just compress 1 BPP bitmap with RLECustomA
            rle = RLECustomA(bpp, verbose)
            compressed = rle.encode(packed_pixels)
            method = 1

        return method, compressed

    # -------------------------------------------------------------------------
    @classmethod
    def decode(cls, method, encoded_data, bpp, verbose=False):
        """
        Decompress previously encoded data using the provided method.
        Input: array of compressed bytes
        Output: array of packed pixels
        """
        # Is data really encoded?
        decoded = encoded_data
        if method != 1:
            pass
        elif bpp == 4:
            # Just one method for the moment
            rle = RLECustom3(bpp, verbose)
            decoded = rle.decode(encoded_data)
        elif bpp == 1:
            # Just one method for the moment
            rle = RLECustomA(bpp, verbose)
            decoded = rle.decode(encoded_data)

        return decoded


# -----------------------------------------------------------------------------
# Program entry point:
# -----------------------------------------------------------------------------
if __name__ == "__main__":

    # -------------------------------------------------------------------------
    def main(args):
        """
        Main method.
        """
        # ascii 0x0040 (88 bytes)
        data = bytes([
            0x00, 0x7F, 0xE0, 0x00, 0x1F, 0xFF, 0x00, 0x07,
            0xC0, 0x38, 0x00, 0xF0, 0x01, 0x80, 0x1E, 0x00,
            0x18, 0x03, 0x80, 0x01, 0x80, 0x30, 0x00, 0x38,
            0x67, 0x1F, 0xFF, 0x86, 0x61, 0xFF, 0xF0, 0x6E,
            0x0C, 0x03, 0x07, 0xC1, 0xC0, 0x38, 0x3C, 0x18,
            0x01, 0x83, 0xC1, 0x80, 0x18, 0x3C, 0x18, 0x01,
            0x83, 0xC1, 0x80, 0x18, 0x3E, 0x1C, 0x03, 0x87,
            0x60, 0xF0, 0xF0, 0x67, 0x07, 0xFE, 0x0E, 0x30,
            0x1F, 0x80, 0xC3, 0x80, 0x00, 0x1C, 0x1E, 0x00,
            0x07, 0x80, 0xF0, 0x00, 0xF0, 0x07, 0xC0, 0x3E,
            0x00, 0x1F, 0xFF, 0x80, 0x00, 0x7F, 0xE0, 0x00,
        ])
        with RLECustomA(args.bpp, args.verbose) as rle:
            compressed = rle.encode(data)
            encoded_size = rle.get_encoded_size(compressed)
            sys.stdout.write(f"Encoded size: {encoded_size} bytes "
                             f"(instead of {len(data)})\n")
        # No need to check if decoding is fine, already done when encoding

        return 0

    # -------------------------------------------------------------------------
    # Parse arguments:
    parser = argparse.ArgumentParser(
        description="Test custom RLE methods (Build #220223.1003)")

    parser.add_argument(
        "-b", "--bpp",
        dest="bpp", type=int,
        default=1,
        help="Number of bits per pixel ('%(default)s' by default)")

    parser.add_argument(
        "-v", "--verbose",
        action='store_true',
        help="Add verbosity to output ('%(default)s' by default)")

    # Call main function:
    EXIT_CODE = main(parser.parse_args())

    sys.exit(EXIT_CODE)
