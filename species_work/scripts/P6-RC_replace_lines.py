"""Replace a block of lines in one owned file, refusing if the file changed since it was read.

Usage: python3 -I P6-RC_replace_lines.py <file> <expected sha256> <first line prefix> <last line prefix> <replacement file> [n]

The block runs from the one line that starts with <first line prefix> to the n-th line after it (default 1)
that starts with <last line prefix>, both included. Without n, each prefix must match exactly one line.
"""
import hashlib
import sys


def main():
  path, expected, first, last, repl_path = sys.argv[1:6]
  nth = int(sys.argv[6]) if len(sys.argv) > 6 else 0
  with open(path, 'rb') as fh:
    raw = fh.read()
  actual = hashlib.sha256(raw).hexdigest()
  if actual != expected:
    sys.exit(f'refused: {path} changed since it was read ({actual})')
  text = raw.decode('utf-8')
  lines = text.split('\n')
  starts = [i for i, l in enumerate(lines) if l.startswith(first)]
  ends = [i for i, l in enumerate(lines) if l.startswith(last)]
  if nth and len(starts) == 1:
    after = [i for i in ends if i > starts[0]]
    ends = after[nth - 1:nth]
  if len(starts) != 1 or len(ends) != 1 or ends[0] < starts[0]:
    sys.exit(f'refused: prefixes match {len(starts)} and {len(ends)} lines')
  with open(repl_path, encoding='utf-8') as fh:
    repl = fh.read().rstrip('\n').split('\n')
  lines[starts[0]:ends[0] + 1] = repl
  with open(path, 'w', encoding='utf-8', newline='') as fh:
    fh.write('\n'.join(lines))
  print(f'replaced lines {starts[0] + 1} to {ends[0] + 1} with {len(repl)} lines')


if __name__ == '__main__':
  main()
