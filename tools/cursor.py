#!/usr/bin/env python3

"""
Export bitmap and colors from a 32x32 PNG file to a .h file.
"""

import os
import sys

from PIL import Image

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <cursor.png> <cursor.h>", file=sys.stderr)
        sys.exit(0)

    filename = sys.argv[1]
    im = Image.open(filename)
    x, y = im.size

    if (x, y) != (32, 32):
        raise ValueError(f"Expected image size (32, 32), got ({x}, {y})")

    var = os.path.splitext(os.path.basename(filename))[0]

    im2 = im.convert("P")
    pixels = im2.load()
    buf = f"char cursor_{var}[] = {{"
    for j in range(0, x):
        for i in range(0, y):
            if pixels[i, j] not in [0, 0xFF]:
                buf += "'x',"
            else:
                buf += "' ',"
    buf += "};\n"

    im = im.convert("RGB")
    pixels = im.load()

    buf += f"char color_{var}[] = {{"
    for j in range(0, x):
        for i in range(0, y):
            r, g, b = pixels[i, j]
            buf += f"0x{r:02x},"
            buf += f"0x{g:02x},"
            buf += f"0x{b:02x},"
            buf += f"0x{0:02x},"
    buf += "};\n"

    with open(sys.argv[2], "w") as fp:
        fp.write(buf)
