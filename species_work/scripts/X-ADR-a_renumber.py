#!/usr/bin/env python3
"""Shift ADR numbers >= START by +1 in ADR-NNNN references and NNNN- link targets.

Usage: python3 -I X-ADR-a_renumber.py <start> <file> [<file> ...]
Only references (text), never file names on disk, are changed.
"""
import re
import sys


def main():
    start = int(sys.argv[1])
    for path in sys.argv[2:]:
        with open(path, encoding='utf-8') as f:
            text = f.read()

        def ref(m):
            n = int(m.group(1))
            return 'ADR-%04d' % (n + 1 if start <= n <= 49 else n)

        def link(m):
            n = int(m.group(1))
            return '(%04d-' % (n + 1 if start <= n <= 49 else n)

        new = re.sub(r'ADR-(\d{4})', ref, text)
        new = re.sub(r'\((\d{4})-', link, new)
        if new != text:
            with open(path, 'w', encoding='utf-8') as f:
                f.write(new)
            print('updated', path)


if __name__ == '__main__':
    main()
