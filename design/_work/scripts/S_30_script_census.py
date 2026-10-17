"""S_30 script census: one row per data/scripts/*.txt file.

Usage: python3 -I S_30_script_census.py <multiwinia_src_root>

For each script file it reports physical lines (= frames consumed by the
one-line-per-frame interpreter, lower bound), executable lines, opcode counts,
Say phrase count and first..last id, the sum of explicit `Wait` seconds and of
`CamMove` durations, and every live (uncommented) and commented reference to
the file name found in data/levels/*.txt, data/game*.txt, data/purity/*.txt and
code/**/*.cpp. Output: Markdown table rows on stdout.
"""
import os
import re
import sys
import collections

SEP = re.compile(r'[ \t:,]+')


def tokens(line):
    line = line.split('#')[0].replace('\r', '').replace('\n', '').strip()
    if not line:
        return []
    return [t for t in SEP.split(line) if t]


def census(src):
    sdir = os.path.join(src, 'data', 'scripts')
    ref_files = []
    for d, pat in ((os.path.join(src, 'data', 'levels'), r'.*\.txt$'),
                   (os.path.join(src, 'data'), r'^game.*\.txt$'),
                   (os.path.join(src, 'data', 'purity'), r'.*\.txt$')):
        if os.path.isdir(d):
            for f in sorted(os.listdir(d)):
                if re.match(pat, f) and os.path.isfile(os.path.join(d, f)):
                    ref_files.append(os.path.join(d, f))
    for root, _, files in os.walk(os.path.join(src, 'code')):
        for f in files:
            if f.endswith('.cpp'):
                ref_files.append(os.path.join(root, f))
    texts = {}
    for p in ref_files:
        with open(p, encoding='latin-1') as fh:
            texts[p] = fh.read().splitlines()

    rows = []
    for f in sorted(os.listdir(sdir)):
        p = os.path.join(sdir, f)
        with open(p, encoding='latin-1') as fh:
            raw = fh.read().splitlines()
        ops = collections.Counter()
        says = []
        wait = 0.0
        move = 0.0
        exe = 0
        for line in raw:
            t = tokens(line)
            if not t:
                continue
            exe += 1
            op = t[0]
            ops[op] += 1
            low = op.lower()
            if low == 'say' and len(t) > 1:
                says.append(t[1])
            if low == 'wait':
                try:
                    wait += float(t[1]) if len(t) > 1 else 0.0
                except ValueError:
                    pass
            if low == 'cammove' and len(t) > 2:
                try:
                    move += float(t[2])
                except ValueError:
                    pass
        live, dead = [], []
        for rp, lines in texts.items():
            rel = os.path.relpath(rp, src).replace(os.sep, '/')
            for i, ln in enumerate(lines, 1):
                if f.lower() in ln.lower():
                    stripped = ln.strip()
                    commented = stripped.startswith('#') or stripped.startswith('//')
                    (dead if commented else live).append(f'{rel}:L{i}')
        rows.append((f, len(raw), exe, ops, says, wait, move, live, dead))
    return rows


def main():
    src = sys.argv[1]
    print('| Script | Phys. lines | Exec. lines | Commands (opcode×count) | Say ids (n: first..last) | ΣWait s | ΣCamMove s | Live references | Commented-out references |')
    print('|---|---|---|---|---|---|---|---|---|')
    for f, phys, exe, ops, says, wait, move, live, dead in census(src):
        cmds = ', '.join(f'{k}×{v}' for k, v in sorted(ops.items(), key=lambda kv: kv[0].lower()))
        s = f'{len(says)}: `{says[0]}`..`{says[-1]}`' if says else '0'
        print(f'| `{f}` | {phys} | {exe} | {cmds} | {s} | {wait:g} | {move:g} | '
              f'{"; ".join(live) if live else "none"} | {"; ".join(dead) if dead else "none"} |')


if __name__ == '__main__':
    main()
