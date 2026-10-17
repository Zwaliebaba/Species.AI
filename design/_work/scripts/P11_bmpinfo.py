"""P11 helper: print BMP header info (w,h,bpp,compression) for files given.
Usage: python3 -I P11_bmpinfo.py <file>..."""
import sys, struct, os
for p in sys.argv[1:]:
    b = open(p, 'rb').read(64)
    if b[:2] != b'BM': print(os.path.basename(p), 'not BMP'); continue
    off = struct.unpack_from('<I', b, 10)[0]
    hs = struct.unpack_from('<I', b, 14)[0]
    if hs == 40 or hs > 40:
        w, h, planes, bpp, comp = struct.unpack_from('<iiHHI', b, 18)
    else:
        w, h, planes, bpp = struct.unpack_from('<HHHH', b, 18); comp = 0
    print(f'{os.path.basename(p)},{w},{h},{bpp},{comp},{hs}')
