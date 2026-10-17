#!/usr/bin/env python3
"""S27 helper: scan Ogg Vorbis headers under the given directories.

Reports channel-count and sample-rate distributions and the largest decoded
PCM size (mono 16-bit = 2 bytes/sample/channel), using the identification
header (channels, rate) and the granule position of the last Ogg page
(total PCM frames). Read-only; pass directories as arguments.
Usage: python3 -I S_27_ogg_scan.py <dir> [<dir> ...]
"""
import os
import struct
import sys
from collections import Counter


def scan(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data[:4] != b'OggS':
        return None
    # identification header: first page payload starts after the segment table
    nseg = data[26]
    hdr = 27 + nseg
    pkt = data[hdr:hdr + 30]
    if pkt[1:7] != b'vorbis':
        return None
    channels = pkt[11]
    rate = struct.unpack('<I', pkt[12:16])[0]
    last = data.rfind(b'OggS')
    granule = struct.unpack('<q', data[last + 6:last + 14])[0]
    return channels, rate, granule


def main(dirs):
    chans, rates = Counter(), Counter()
    total = 0
    total_bytes = {}
    biggest = (0, '')
    odd_rate = []
    for d in dirs:
        for name in sorted(os.listdir(d)):
            if not name.lower().endswith('.ogg'):
                continue
            p = os.path.join(d, name)
            r = scan(p)
            if r is None:
                print('UNPARSED', p)
                continue
            c, rate, frames = r
            total += 1
            chans[c] += 1
            rates[rate] += 1
            if rate != 44100:
                odd_rate.append((p, rate))
            size = frames * c * 2
            total_bytes[d] = total_bytes.get(d, 0) + size
            if size > biggest[0]:
                biggest = (size, p, frames, rate)
    print('files', total)
    print('channels', dict(chans))
    print('rates', dict(rates))
    print('non-44.1k', odd_rate)
    for d, b in total_bytes.items():
        print('decoded total %s: %.1f MiB' % (os.path.basename(d.rstrip('/')), b / 1048576.0))
    size, p, frames, rate = biggest
    print('largest decoded: %s %.1f MiB (%d frames, %.1f s)' % (p, size / 1048576.0, frames, frames / float(rate)))


if __name__ == '__main__':
    main(sys.argv[1:])
