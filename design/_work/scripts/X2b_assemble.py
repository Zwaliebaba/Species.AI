"""X2b: splice generated markdown tables into the draft between <!-- X2B:<NAME>:BEGIN/END --> markers.
Usage: python3 -I X2b_assemble.py <draft.md> NAME=table.md [NAME=table.md ...]  (idempotent)"""
import sys

def main():
    draft = sys.argv[1]
    s = open(draft, encoding='utf-8').read()
    for arg in sys.argv[2:]:
        name, path = arg.split('=', 1)
        b = '<!-- X2B:%s:BEGIN -->' % name
        e = '<!-- X2B:%s:END -->' % name
        i, j = s.index(b), s.index(e)
        body = open(path, encoding='utf-8').read().strip('\n')
        s = s[:i + len(b)] + '\n' + body + '\n' + s[j:]
    open(draft, 'w', encoding='utf-8').write(s)

if __name__ == '__main__':
    main()
