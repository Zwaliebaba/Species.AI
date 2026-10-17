"""Print lines a..b of a file, each truncated to W chars.

Usage: python3 -I F2b2_peek.py <file> <a> <b> [width]
"""
import sys
path, a, b = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
w = int(sys.argv[4]) if len(sys.argv) > 4 else 220
with open(path, encoding='utf-8') as f:
    for i, line in enumerate(f, start=1):
        if i < a:
            continue
        if i > b:
            break
        line = line.rstrip('\n')
        if line.strip():
            print(f"{i}: {line[:w]}{'…' if len(line) > w else ''}")
