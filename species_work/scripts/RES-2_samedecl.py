"""RES-2 helper: compare the cpp sketch lines of a Markdown file between two versions, ignoring whitespace,
`Engine::` qualifiers, namespace lines and lone braces. Prints the lines only in the old or only in the new version.

Usage: python3 -I RES-2_samedecl.py <old.md> <new.md>
"""
import collections
import re
import sys


def lines(path):
    t = open(path, encoding='utf-8').read()
    out = []
    for block in re.findall(r'```cpp\n(.*?)```', t, re.S):
        for l in block.split('\n'):
            l = re.sub(r'\s+', ' ', l.replace('Engine::', '')).strip()
            if l and not l.startswith('namespace') and not l.startswith('} // namespace') and l != '{':
                out.append(l)
    return collections.Counter(out)


def main():
    a, b = lines(sys.argv[1]), lines(sys.argv[2])
    for l in (a - b).elements():
        print('- ' + l)
    for l in (b - a).elements():
        print('+ ' + l)


if __name__ == '__main__':
    main()
