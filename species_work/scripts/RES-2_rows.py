"""RES-2 helper: replace or delete Markdown table rows by their unique leading cells, and insert rows after a row.

Usage: python3 -I RES-2_rows.py <file> <rows spec>
Rows spec: blocks separated by a line '%%'. Each block starts with one directive line:
  REPLACE <prefix>   the following lines replace the one row that starts with <prefix> (several lines insert rows)
  DELETE <prefix>    deletes the one row that starts with <prefix>
  AFTER <prefix>     the following lines are inserted after the one row that starts with <prefix>
A prefix must match exactly one line of the file.
"""
import sys


def main():
    path, spec_path = sys.argv[1], sys.argv[2]
    lines = open(path, encoding='utf-8').read().split('\n')
    spec = open(spec_path, encoding='utf-8').read().split('\n%%\n')
    for block in spec:
        block = block.strip('\n')
        if not block:
            continue
        head, *body = block.split('\n')
        op, prefix = head.split(' ', 1)
        hits = [i for i, l in enumerate(lines) if l.startswith(prefix)]
        if len(hits) != 1:
            sys.exit(f'{op} {prefix!r}: {len(hits)} matching lines')
        i = hits[0]
        if op == 'REPLACE':
            lines[i:i + 1] = body
        elif op == 'DELETE':
            del lines[i]
        elif op == 'AFTER':
            lines[i + 1:i + 1] = body
        else:
            sys.exit(f'unknown directive {op}')
    open(path, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
    print(f'applied {len([b for b in spec if b.strip()])} blocks to {path}')


if __name__ == '__main__':
    main()
