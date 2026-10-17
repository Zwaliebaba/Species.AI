# Usage: python3 -I P12_bmpinfo.py <file-or-dir>...   -> CSV path,width,height,bpp,compression,ncolours
import sys, os, struct
def info(p):
    with open(p,'rb') as f:
        h=f.read(54)
    if len(h)<26 or h[:2]!=b'BM': return None
    off=struct.unpack_from('<I',h,10)[0]; bisz=struct.unpack_from('<I',h,14)[0]
    if bisz==12:
        w,hh,pl,bpp=struct.unpack_from('<HHHH',h,18); comp=0; ncol=(off-26)//3
    else:
        w,hh,pl,bpp,comp=struct.unpack_from('<iiHHI',h,18); ncol=(off-54)//4
    return w,hh,bpp,comp,ncol
print('path,width,height,bpp,compression,palette_entries')
for a in sys.argv[1:]:
    paths=[a] if os.path.isfile(a) else sorted(os.path.join(a,x) for x in os.listdir(a))
    for p in paths:
        if not p.lower().endswith('.bmp'): continue
        r=info(p)
        print(p+','+(','.join(map(str,r)) if r else 'not-bmp'))
