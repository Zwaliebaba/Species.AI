"""Peek into ORIG sections for routing (F2a2).

Usage:
  python3 -I F2a2_peek.py <headings.csv> <orig_dir> <file_prefix> outline
      H2/H3/H4 outline with line ranges, bytes and build-tag / keyword counts.
  python3 -I F2a2_peek.py <headings.csv> <orig_dir> <file_prefix> lines <start> <end> [maxchars]
      Print lines start..end (each truncated to maxchars, default 220).
  python3 -I F2a2_peek.py <headings.csv> <orig_dir> <file_prefix> grep <start> <end> <regex>
      Print matching lines (truncated) inside start..end.
"""
import csv
import os
import re
import sys

TAGS = ['B:FULL', 'B:LEGACY', 'B:DEAD', 'B:D3D', 'B:DEBUG', 'B:DEMO', 'B:EDITOR']
KW = re.compile(r'campaign|tank ?battle|darwinia\b|mission_|map_|lockstep|desync|sync|metaserver|'
                r'authkey|auth key|demo|gesture|music|achievement|leaderboard|steam|lobby', re.I)


def load(headcsv, prefix):
    rows = [r for r in csv.DictReader(open(headcsv, encoding='utf-8'))
            if r['file'].startswith(prefix)]
    return rows


def main():
    headcsv, orig, prefix, mode = sys.argv[1:5]
    rows = load(headcsv, prefix)
    fname = rows[0]['file']
    lines = open(os.path.join(orig, fname), encoding='utf-8').read().split('\n')
    if mode == 'outline':
        maxlvl = int(sys.argv[5]) if len(sys.argv) > 5 else 3
        lo = int(sys.argv[6]) if len(sys.argv) > 6 else 1
        hi = int(sys.argv[7]) if len(sys.argv) > 7 else len(lines)
        for r in rows:
            lvl = int(r['level'])
            s, e = int(r['start_line']), int(r['end_line'])
            if lvl > maxlvl or s < lo or s > hi:
                continue
            seg = '\n'.join(lines[s - 1:e])
            tc = {t: seg.count('[' + t) + seg.count(t + ']') for t in TAGS}
            tc = ' '.join(f'{t[2:]}={n}' for t, n in tc.items() if n)
            kw = {}
            for m in KW.finditer(seg):
                k = m.group(0).lower()
                kw[k] = kw.get(k, 0) + 1
            kws = ' '.join(f'{k}={n}' for k, n in sorted(kw.items()))
            print(f"{'  ' * (lvl - 2)}H{lvl} {s}-{e} {r['bytes']}B | {r['heading'][:80]} | {tc} | {kws}")
    elif mode == 'lines':
        s, e = int(sys.argv[5]), int(sys.argv[6])
        mx = int(sys.argv[7]) if len(sys.argv) > 7 else 220
        for i in range(s, min(e, len(lines)) + 1):
            print(f'{i}: {lines[i - 1][:mx]}')
    elif mode == 'grep':
        s, e, rx = int(sys.argv[5]), int(sys.argv[6]), re.compile(sys.argv[7], re.I)
        for i in range(s, min(e, len(lines)) + 1):
            if rx.search(lines[i - 1]):
                print(f'{i}: {lines[i - 1][:200]}')


if __name__ == '__main__':
    main()
