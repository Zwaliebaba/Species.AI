"""FX-08: repoint broken links of the given files to documents that became folders.

Usage: python3 -I FX-08_relink.py <file.md> [<file.md> ...] [--apply]

For a relative link to `x.md` that no longer exists while the folder `x/` does, the link is
repointed to the file of `x/` that holds the link's anchor (README.md first), or to
`x/README.md` when the link has no anchor. Only the files named are read and written; other
broken links are listed and left alone.
"""
import glob
import os
import re
import sys
from collections import Counter

FENCE = re.compile(r'^[ \t]*(```|~~~)')
LINK = re.compile(r'(?<!!)(\[(?:[^\]\[]|\[[^\]]*\])*\]\()([^)\s]+)((?:\s+"[^"]*")?\))')


def slug(h):
    h = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', h)
    h = re.sub(r'<[^>]+>', '', h).strip().lower()
    return re.sub(r'[^\w\- ]', '', h).replace(' ', '-')


def anchors(path):
    out, seen, inside = set(), Counter(), False
    for line in open(path, encoding='utf-8').read().split('\n'):
        if FENCE.match(line):
            inside = not inside
            continue
        if inside:
            continue
        m = re.match(r'^(#{1,6})\s+(.*?)\s*#*\s*$', line)
        if m:
            s = slug(m.group(2))
            out.add(s if not seen[s] else f'{s}-{seen[s]}')
            seen[s] += 1
    return out


args = [a for a in sys.argv[1:] if a != '--apply']
apply = '--apply' in sys.argv
for src in args:
    base = os.path.dirname(os.path.abspath(src))
    lines = open(src, encoding='utf-8').read().split('\n')
    inside, changed, left = False, 0, []
    for i, line in enumerate(lines):
        if FENCE.match(line):
            inside = not inside
            continue
        if inside or '](' not in line:
            continue

        def fix(m):
            global changed
            t = m.group(2)
            if re.match(r'^[a-z][a-z0-9+.-]*:', t, re.I) or t.startswith('#'):
                return m.group(0)
            path, _, anchor = t.partition('#')
            dest = os.path.normpath(os.path.join(base, path))
            if os.path.exists(dest):
                if anchor and dest.endswith('.md') and anchor not in anchors(dest):
                    left.append(f'{i + 1}: {t} (anchor missing)')
                return m.group(0)
            if not (dest.endswith('.md') and os.path.isdir(dest[:-3])):
                left.append(f'{i + 1}: {t}')
                return m.group(0)
            files = sorted(glob.glob(os.path.join(dest[:-3], '*.md')), key=lambda p: os.path.basename(p) != 'README.md')
            pick = files[0]
            if anchor:
                pick = next((c for c in files if anchor in anchors(c)), None)
                if pick is None:
                    left.append(f'{i + 1}: {t} (anchor not found in the folder)')
                    return m.group(0)
            new = os.path.relpath(pick, base).replace(os.sep, '/') + (f'#{anchor}' if anchor else '')
            text = m.group(1)
            if text[1:-2] in (path, os.path.basename(path)):
                text = '[' + new.split('#')[0] + ']('
            changed += 1
            return text + new + m.group(3)

        lines[i] = LINK.sub(fix, line)
    if apply:
        open(src, 'w', encoding='utf-8').write('\n'.join(lines))
    print(f'{src}: {changed} links repointed; {len(left)} left: {left}')
