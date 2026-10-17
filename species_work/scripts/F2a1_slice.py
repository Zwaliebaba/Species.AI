"""Print a line range of a file, truncating long lines.
Usage: python3 -I F2a1_slice.py <file> <start> <end> [maxcols]"""
import sys
path, a, b = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
w = int(sys.argv[4]) if len(sys.argv) > 4 else 220
with open(path, encoding='utf-8') as f:
    lines = f.read().split('\n')
for i in range(a, min(b, len(lines)) + 1):
    s = lines[i - 1]
    print(f'{i}: {s[:w]}' + ('…' if len(s) > w else ''))
