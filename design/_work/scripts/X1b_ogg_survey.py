#!/usr/bin/env python3
"""X1b: survey Ogg Vorbis identification headers of data/sounds and data/mwsounds.
Usage: python3 -I X1b_ogg_survey.py <data_dir>
Reports channels, sample rate, nominal bitrate histograms, name conventions."""
import sys, os, struct, collections, re

def vorbis_ident(path):
    with open(path, 'rb') as f:
        b = f.read(4096)
    if b[:4] != b'OggS':
        return None
    nseg = b[26]
    off = 27 + nseg
    pkt = b[off:off + 30]
    if pkt[:7] != b'\x01vorbis':
        return ('notvorbis',)
    ver, ch, rate, bmax, bnom, bmin = struct.unpack_from('<IBIiii', pkt, 7)
    return (ch, rate, bnom)

def main(d):
    for sub in ('sounds', 'mwsounds'):
        chans = collections.Counter(); rates = collections.Counter(); br = collections.Counter()
        names = os.listdir(os.path.join(d, sub))
        bad = []
        upper = [n for n in names if n != n.lower()]
        spaces = [n for n in names if ' ' in n]
        nonogg = [n for n in names if not n.lower().endswith('.ogg')]
        for n in names:
            r = vorbis_ident(os.path.join(d, sub, n))
            if not r or len(r) == 1:
                bad.append(n); continue
            chans[r[0]] += 1; rates[r[1]] += 1; br[(r[2] // 1000) if r[2] > 0 else r[2]] += 1
        print(sub, 'files', len(names), 'channels', dict(chans), 'rates', dict(rates))
        print('   nominal kbps', dict(sorted(br.items())))
        print('   uppercase names', upper[:10], 'spaces', spaces[:10], 'non-ogg', nonogg, 'bad', bad[:10])
    both = set(os.listdir(os.path.join(d, 'sounds'))) & set(os.listdir(os.path.join(d, 'mwsounds')))
    print('names present in both dirs (sounds/ wins):', sorted(both))

if __name__ == '__main__':
    main(sys.argv[1])
