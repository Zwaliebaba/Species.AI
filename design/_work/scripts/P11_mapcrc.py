"""P11 helper: recompute Multiwinia map CRC IDs (LevelFile::CalculeCRC) and compare to code/mapcrc.h.
Usage: python3 -I P11_mapcrc.py <SRC_ROOT>
Model: CRC-32 reflected poly 0xEDB88320, init 0, no final xor, over file bytes + one 0xFF byte
(BinaryReader::ReadU8 returns EOF cast to u8 and ReadFileData counts it), capped at 100001 bytes."""
import sys, os, re
src = sys.argv[1]
tab = []
for i in range(256):
    c = i
    for _ in range(8):
        c = (c >> 1) ^ 0xEDB88320 if c & 1 else c >> 1
    tab.append(c)
def crc(data, start=0):
    c = start
    for b in data:
        c = tab[(c ^ b) & 0xff] ^ (c >> 8)
    return c & 0xffffffff
ids = {}
for line in open(os.path.join(src, 'code/mapcrc.h')):
    m = re.match(r'#define\s+MAPID_(\S+)\s+(0x[0-9a-f]+)', line.strip())
    if m: ids[m.group(1)] = int(m.group(2), 16)
lv = os.path.join(src, 'data/levels')
ok = bad = 0
for fn in sorted(os.listdir(lv)):
    p = os.path.join(lv, fn)
    if not os.path.isfile(p): continue
    data = open(p, 'rb').read()
    buf = (data + b'\xff')[:100001]
    v = crc(buf)
    key = fn[:-4].upper()
    exp = ids.get(key)
    status = 'NOID' if exp is None else ('OK' if exp == v else 'MISMATCH')
    if status == 'OK': ok += 1
    elif status == 'MISMATCH': bad += 1
    print(f'{fn},{len(data)},0x{v:08x},{"" if exp is None else hex(exp)},{status}')
print(f'# ok={ok} mismatch={bad}', file=sys.stderr)
