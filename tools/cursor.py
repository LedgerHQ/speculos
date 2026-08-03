#!/usr/bin/env python3

"""
Export bitmap and colors from a 32x32 PNG file to a .h file.
"""

import sys
from pathlib import Path

from PIL import Image

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <cursor.png> <cursor.h>", file=sys.stderr)
        sys.exit(0)

    input_path = Path(sys.argv[1]).resolve()
    if not input_path.is_file():
        print(f"Error: input file not found: {input_path}", file=sys.stderr)
        sys.exit(1)
    if input_path.suffix.lower() != ".png":
        print(f"Error: input file must be a .png file: {input_path}", file=sys.stderr)
        sys.exit(1)

    output_path = Path(sys.argv[2]).resolve()
    if output_path.suffix.lower() != ".h":
        print(f"Error: output file must be a .h file: {output_path}", file=sys.stderr)
        sys.exit(1)

    im = Image.open(input_path)
    x, y = im.size

    if (x, y) != (32, 32):
        raise ValueError(f"Expected image size (32, 32), got ({x}, {y})")

    var = input_path.stem

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

    with output_path.open("w") as fp:
        fp.write(buf)
