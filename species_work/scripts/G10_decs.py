"""G10: print DEC entries by id from the decision register files.

Usage: python3 -I G10_decs.py <decisions_dir> DEC-004 DEC-005 ...
"""
import os
import re
import sys


def main():
    root = sys.argv[1]
    want = set(sys.argv[2:])
    files = []
    for dirpath, _dirs, names in os.walk(root):
        for n in names:
            if n.endswith('.md') and n != 'README.md':
                files.append(os.path.join(dirpath, n))
    for path in sorted(files):
        with open(path, encoding='utf-8') as fh:
            text = fh.read()
        parts = re.split(r'(?m)^(?=### DEC-\d{3})', text)
        for p in parts:
            m = re.match(r'### (DEC-\d{3})', p)
            if m and m.group(1) in want:
                print('=' * 8, os.path.relpath(path, root))
                print(p.strip())
                print()


if __name__ == '__main__':
    main()
