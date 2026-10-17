"""RES-2 helper: list uses of game-layer names inside `namespace Engine` blocks of the sketches.

Usage: python3 -I RES-2_edges.py <components folder> <names file>
The names file holds one game-layer (Species) name per line; the names declared in `namespace Species` blocks of
the sketches are added to it. Comments are ignored.
"""
import os
import re
import sys

DECL = re.compile(r'^\s*(?:template\s*<[^>]*>\s*)?(?:enum class|class|struct|using)\s+(\w+)')


def blocks(path):
    t = open(path, encoding='utf-8').read()
    for m in re.finditer(r'```(?:cpp|hlsl)\n(.*?)```', t, re.S):
        start = t[:m.start()].count('\n') + 2
        ns = None
        for k, line in enumerate(m.group(1).split('\n')):
            s = line.strip()
            mm = re.match(r'namespace (\w+)', s)
            if mm:
                ns = mm.group(1)
                continue
            if s.startswith('} // namespace'):
                ns = None
                continue
            yield ns, start + k, line


def main():
    root, names_file = sys.argv[1], sys.argv[2]
    names = set(l.strip() for l in open(names_file, encoding='utf-8') if l.strip())
    files = [os.path.join(r, n) for r, _, ns in os.walk(root) for n in ns if n.endswith('.md')]
    for p in files:
        for ns, _, line in blocks(p):
            if ns == 'Species':
                m = DECL.match(line)
                if m:
                    names.add(m.group(1))
    pat = re.compile(r'(?<![\w:])(' + '|'.join(sorted(map(re.escape, names), key=len, reverse=True)) + r')\b')
    for p in sorted(files):
        for ns, n, line in blocks(p):
            if ns != 'Engine':
                continue
            code = line.split('//')[0]
            hits = sorted(set(pat.findall(code)))
            if hits:
                print(f'{os.path.relpath(p, root)}:{n}: {", ".join(hits)}')


if __name__ == '__main__':
    main()
