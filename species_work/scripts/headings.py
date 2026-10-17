"""List every Markdown heading of the given documents with its section extent.

Usage: python3 -I headings.py <out.csv> <doc.md>...

Columns: file, level, heading, anchor, start_line, end_line, bytes
- end_line is the last line before the next heading of the same or a higher
  level (so an H2 extent includes its H3 children).
- bytes is the UTF-8 size of lines start_line..end_line.
- anchor follows GitHub's slug rules, with -1, -2 suffixes for duplicates.
Headings inside fenced code blocks are ignored.
"""
import csv
import os
import re
import sys

HEADING = re.compile(r'^(#{1,6})\s+(.*?)\s*#*\s*$')
FENCE = re.compile(r'^\s*(```|~~~)')


def slug(text, seen):
    s = text.strip().lower()
    s = re.sub(r'[^\w\- ]', '', s)
    s = s.replace(' ', '-')
    n = seen.get(s, 0)
    seen[s] = n + 1
    return s if n == 0 else f'{s}-{n}'


def scan(path):
    with open(path, encoding='utf-8') as f:
        lines = f.read().split('\n')
    heads = []
    in_fence = False
    for i, line in enumerate(lines, start=1):
        if FENCE.match(line):
            in_fence = not in_fence
            continue
        if in_fence:
            continue
        m = HEADING.match(line)
        if m:
            heads.append([len(m.group(1)), m.group(2), i])
    seen = {}
    rows = []
    for idx, (level, text, start) in enumerate(heads):
        end = len(lines)
        for level2, _, start2 in heads[idx + 1:]:
            if level2 <= level:
                end = start2 - 1
                break
        size = sum(len(l.encode('utf-8')) + 1 for l in lines[start - 1:end])
        rows.append([os.path.basename(path), level, text, slug(text, seen), start, end, size])
    return rows


def main():
    out, docs = sys.argv[1], sys.argv[2:]
    with open(out, 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(['file', 'level', 'heading', 'anchor', 'start_line', 'end_line', 'bytes'])
        total = 0
        for d in docs:
            rows = scan(d)
            total += len(rows)
            w.writerows(rows)
    print(f'{total} headings from {len(docs)} documents -> {out}')


if __name__ == '__main__':
    main()
