import sys, os, struct
for d in sys.argv[1:]:
    for f in sorted(os.listdir(d)):
        p = os.path.join(d, f)
        with open(p,'rb') as fh:
            h = fh.read(54)
        if h[:2] != b'BM': print(f"{p}|not-bmp"); continue
        w,hh = struct.unpack('<ii', h[18:26]); bpp = struct.unpack('<H', h[28:30])[0]
        print(f"{os.path.basename(d)}/{f}|{w}x{abs(hh)}|{bpp}bpp|{os.path.getsize(p)}")
