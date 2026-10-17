"""Print a line range of a document, each line truncated, optionally only lines matching a regex.

Usage: python3 -I F2b1_peek.py <doc.md> <start> <end> [width] [regex]
"""
import re
import sys

path, start, end = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
width = int(sys.argv[4]) if len(sys.argv) > 4 else 160
pat = re.compile(sys.argv[5], re.I) if len(sys.argv) > 5 else None
with open(path, encoding='utf-8') as f:
    lines = f.read().split('\n')
for i in range(start, min(end, len(lines)) + 1):
    line = lines[i - 1]
    if pat and not pat.search(line):
        continue
    if not line.strip():
        continue
    print(f'{i}: {line[:width]}')
