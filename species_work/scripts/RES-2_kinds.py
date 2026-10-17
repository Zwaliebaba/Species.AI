"""RES-2 helper: set the Kind column of CONVENTIONS table 8.12-A to "engine type" or "game type".

Usage: python3 -I RES-2_kinds.py <CONVENTIONS.md> <engine identifiers file>
Rows whose identifier (the first code span of the Identifier cell) is listed in the file become "engine type";
every other row becomes "game type". Prints the counts and the game rows.
"""
import re
import sys


def main():
    path, names_path = sys.argv[1], sys.argv[2]
    engine = set(l.strip() for l in open(names_path, encoding='utf-8') if l.strip())
    lines = open(path, encoding='utf-8').read().split('\n')
    start = lines.index('Table 8.12-A Engine and game types.')
    counts = {'engine type': 0, 'game type': 0}
    seen = set()
    for i in range(start, len(lines)):
        line = lines[i]
        if line.startswith('Table 8.12-B'):
            break
        m = re.match(r'^\| (.+?) \| (engine type|game type) \| (.+?) \| (.+?) \|$', line)
        if not m:
            continue
        code = re.search(r'`([^`]+)`', m.group(3))
        ident = code.group(1) if code else m.group(3)
        kind = 'engine type' if ident in engine else 'game type'
        if code and ident in engine:
            seen.add(ident)
        counts[kind] += 1
        lines[i] = f'| {m.group(1)} | {kind} | {m.group(3)} | {m.group(4)} |'
    missing = engine - seen
    open(path, 'w', encoding='utf-8', newline='').write('\n'.join(lines))
    print(counts, 'engine names not found:', sorted(missing))


if __name__ == '__main__':
    main()
