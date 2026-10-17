"""Phase 4: repoint links whose target document became a folder or was split.

Usage: python3 -I FX-11_links.py <DESIGN_ROOT> [--apply] <file or folder relative to the root> ...

FX-11 copy of the coordinator's p4_fix_links.py, restricted to the files named on the command line.

A broken relative link to `x.md` where `x/` is now a folder is repointed to the file in
`x/` that holds the link's anchor (README.md when there is no anchor). Links to a split
document (SPLITS) are repointed the same way among the split files. Without --apply the
script only prints what it would change. Links it cannot resolve are listed as UNRESOLVED.
"""
import glob
import os
import re
import sys
from collections import Counter

ROOT = os.path.abspath(sys.argv[1])
APPLY = '--apply' in sys.argv
ONLY = [os.path.join(ROOT, a) for a in sys.argv[2:] if a != '--apply']
SPLITS = {
    # first entry: the target of an anchorless link; the rest are searched for anchors
    'reference/02_units/creatures_and_hazards.md': ['reference/02_units/README.md', 'reference/02_units/creatures.md',
                                                    'reference/02_units/hazards_and_projectiles.md'],
}
FENCE = re.compile(r'^[ \t]*(```|~~~)')
LINK = re.compile(r'(?<!!)(\[(?:[^\]\[]|\[[^\]]*\])*\]\()([^)\s]+)((?:\s+"[^"]*")?\))')


def slug(h):
    h = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', h)
    h = re.sub(r'<[^>]+>', '', h).strip().lower()
    return re.sub(r'[^\w\- ]', '', h).replace(' ', '-')


_anch = {}


def anchors(path):
    if path not in _anch:
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
            out.update(re.findall(r'<a\s+(?:id|name)="([^"]+)"', line))
        _anch[path] = out
    return _anch[path]


def candidates(dest):
    r = os.path.relpath(dest, ROOT).replace(os.sep, '/')
    if r in SPLITS:
        return [os.path.join(ROOT, x) for x in SPLITS[r]]
    if dest.endswith('.md') and os.path.isdir(dest[:-3]):
        d = dest[:-3]
        files = sorted(glob.glob(os.path.join(d, '*.md')))
        return sorted(files, key=lambda p: os.path.basename(p) != 'README.md')
    return None


changed, unresolved, files_changed = Counter(), [], 0
for src in sorted(glob.glob(os.path.join(ROOT, '**', '*.md'), recursive=True)):
    if not any(src == o or src.startswith(o.rstrip('/') + '/') for o in ONLY):
        continue
    lines = open(src, encoding='utf-8').read().split('\n')
    base, inside, dirty = os.path.dirname(src), False, False
    for i, line in enumerate(lines):
        if FENCE.match(line):
            inside = not inside
            continue
        if inside or '](' not in line:
            continue

        def fix(m):
            global dirty
            target = m.group(2)
            if re.match(r'^[a-z][a-z0-9+.-]*:', target, re.I) or target.startswith('#'):
                return m.group(0)
            path, _, anchor = target.partition('#')
            dest = os.path.normpath(os.path.join(base, path))
            if os.path.exists(dest):
                return m.group(0)
            cands = candidates(dest)
            if not cands:
                unresolved.append(f'{os.path.relpath(src, ROOT)}:{i + 1} {target}')
                return m.group(0)
            pick = None
            if anchor:
                pick = next((c for c in cands if anchor in anchors(c)), None)
                if pick is None:
                    unresolved.append(f'{os.path.relpath(src, ROOT)}:{i + 1} {target} (anchor not found in the split files)')
                    pick = cands[0]
                    anchor = ''
            else:
                pick = cands[0]
            new = os.path.relpath(pick, base).replace(os.sep, '/') + (f'#{anchor}' if anchor else '')
            changed[f'{path} -> {os.path.relpath(pick, ROOT)}'] += 1
            dirty = True
            text = m.group(1)
            if text[1:-2] in (path, os.path.basename(path)):
                text = '[' + (new.split('#')[0] if text[1:-2] == path else os.path.basename(pick)) + ']('
            return text + new + m.group(3)

        dirty = False
        new_line = LINK.sub(fix, line)
        if new_line != line:
            lines[i] = new_line
            files_changed += 1
    if APPLY:
        open(src, 'w', encoding='utf-8').write('\n'.join(lines))

for k, v in sorted(changed.items(), key=lambda kv: -kv[1]):
    print(f'{v:4} {k}')
print(f'links repointed: {sum(changed.values())} on {files_changed} lines; unresolved: {len(unresolved)}; applied: {APPLY}')
for u in unresolved:
    print('UNRESOLVED', u)
