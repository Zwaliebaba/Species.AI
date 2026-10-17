# List entries of a RAR v4 archive by walking block headers (no extraction).
import sys, struct
def main(path):
    d = open(path, 'rb').read()
    assert d[:7] == b'Rar!\x1a\x07\x00', 'not RAR4'
    off = 7
    while off + 7 <= len(d):
        crc, htype, flags, hsize = struct.unpack_from('<HBHH', d, off)
        add = 0
        if htype == 0x73:
            print('ARCHIVE_HEADER flags=0x%04x (solid=%d volume=%d locked=%d)' % (flags, bool(flags & 0x8), bool(flags & 0x1), bool(flags & 0x4)))
        elif htype == 0x74:
            pack, unp, hostos, fcrc, ftime, unpver, method, nsize, attr = struct.unpack_from('<IIBIIBBHI', d, off + 7)
            p = off + 32
            if flags & 0x100:
                hp, hu = struct.unpack_from('<II', d, p); p += 8
                pack += hp << 32; unp += hu << 32
            name = d[p:p + nsize].split(b'\x00')[0].decode('latin-1')
            print('FILE %-60s unp=%8d pack=%8d method=0x%02x ver=%d flags=0x%04x' % (name, unp, pack, method, unpver, flags))
            add = pack
        elif htype == 0x7b:
            print('END')
            break
        else:
            if flags & 0x8000:
                add = struct.unpack_from('<I', d, off + 7)[0]
        if hsize == 0:
            break
        off += hsize + add
if __name__ == '__main__':
    main(sys.argv[1])
