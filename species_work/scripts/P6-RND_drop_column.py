"""P6-RND: drop one column from one Markdown table in one of P6-RND's own files.

Usage: python3 -I P6-RND_drop_column.py <file.md> <caption line start> <column header>
The table is the first one after the line that starts with the caption; the column is found by its exact header text.
Cells are split on '|' that is not escaped as '\\|'. Nothing is written unless exactly one caption matches.
"""
import pathlib
import re
import sys

target = pathlib.Path(sys.argv[1])
caption = sys.argv[2]
header = sys.argv[3]
lines = target.read_text(encoding='utf-8').split('\n')
starts = [k for k, line in enumerate(lines) if line.startswith(caption)]
if len(starts) != 1:
  sys.exit(f'caption matches {len(starts)} lines; nothing written')
k = starts[0] + 1
while k < len(lines) and not lines[k].startswith('|'):
  k += 1
SPLIT = re.compile(r'(?<!\\)\|')


def cells(line):
  parts = SPLIT.split(line)
  return parts[1:-1]


head = [c.strip() for c in cells(lines[k])]
if head.count(header) != 1:
  sys.exit(f'header {header!r} not found once in {head}; nothing written')
col = head.index(header)
end = k
while end < len(lines) and lines[end].startswith('|'):
  row = cells(lines[end])
  if len(row) != len(head):
    sys.exit(f'line {end + 1} has {len(row)} cells, expected {len(head)}; nothing written')
  del row[col]
  lines[end] = '|' + '|'.join(row) + '|'
  end += 1
target.write_text('\n'.join(lines), encoding='utf-8')
print(f'dropped column {header!r} from {end - k} lines starting at line {k + 1} of {target}')
