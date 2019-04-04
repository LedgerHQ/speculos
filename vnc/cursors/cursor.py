#!/usr/bin/env python3

'''
Export bitmap and colors from a 32x32 PNG file to a .h file.
'''

from PIL import Image

import os
import sys

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('Usage: %s <cursor.png> <cursor.h>' % sys.argv[0], file=sys.stderr)
        sys.exit(0)

    filename = sys.argv[1]
    im = Image.open(filename)
    x, y = im.size

    assert (x, y) == (32, 32)

    var = os.path.splitext(os.path.basename(filename))[0]

    im2 = im.convert('P')
    pixels = im2.load()
    buf = 'char cursor_%s[] = {' % var
    for j in range(0, x):
        for i in range(0, y):
            if pixels[i, j] not in [0, 0xff]:
                buf += "'x',"
            else:
                buf += "' ',"
    buf += '};\n'

    im = im.convert('RGB')
    pixels = im.load()

    buf += 'char color_%s[] = {' % var
    for j in range(0, x):
        for i in range(0, y):
            r, g, b = pixels[i, j]
            buf += "0x%02x," % r
            buf += "0x%02x," % g
            buf += "0x%02x," % b
            buf += "0x%02x," % 0
    buf += '};\n'

    with open(sys.argv[2], 'w') as fp:
        fp.write(buf)
