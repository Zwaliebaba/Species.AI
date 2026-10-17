"""P11 helper: ASCII preview of an 8/24-bit BMP (rows printed top of image first), downsampled.
Usage: python3 -I P11_bmpascii.py <file.bmp> [step]"""
import sys, struct
p = sys.argv[1]; step = int(sys.argv[2]) if len(sys.argv) > 2 else 8
b = open(p, 'rb').read()
off = struct.unpack_from('<I', b, 10)[0]
w, h, planes, bpp = struct.unpack_from('<iiHH', b, 18)
ncol = (off - 54) // 4
pal = [b[54 + 4*i: 54 + 4*i + 3] for i in range(ncol)]
rowsize = ((w * bpp + 31) // 32) * 4
def px(x, y):  # y = file row (0 = bottom)
    o = off + y * rowsize
    if bpp == 8:
        c = pal[b[o + x]]; return (c[2], c[1], c[0])
    o += x * 3; return (b[o+2], b[o+1], b[o])
chars = ' .:-=+*#%@'
for y in range(h - 1, -1, -step):
    line = ''
    for x in range(0, w, step):
        r, g, bl = px(x, y); v = (r + g + bl) / 3
        line += chars[min(9, int(v / 25.6))]
    print(line)
