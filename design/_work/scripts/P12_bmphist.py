# Usage: python3 -I P12_bmphist.py <bmp>  -> colour histogram (palettised 4/8-bit and 24-bit BMP)
import sys, struct, collections
p=sys.argv[1]; d=open(p,'rb').read()
off=struct.unpack_from('<I',d,10)[0]; w,h,pl,bpp=struct.unpack_from('<iiHH',d,18)
pal=[]
if bpp<=8:
    n=(off-54)//4
    pal=[(d[54+4*i+2],d[54+4*i+1],d[54+4*i]) for i in range(n)]
rowbytes=((w*bpp+31)//32)*4
c=collections.Counter()
for y in range(h):
    r=d[off+y*rowbytes: off+(y+1)*rowbytes]
    for x in range(w):
        if bpp==4: v=(r[x//2]>>4) if x%2==0 else (r[x//2]&15); col=pal[v]
        elif bpp==8: col=pal[r[x]]
        elif bpp==24: col=(r[3*x+2],r[3*x+1],r[3*x])
        else: col=(r[4*x+2],r[4*x+1],r[4*x])
        c[col]+=1
print(p,w,h,bpp,'distinct',len(c))
for col,n in c.most_common(8): print(' ',col,n)
