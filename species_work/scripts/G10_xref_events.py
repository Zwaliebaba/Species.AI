"""G10: collect sound-event ids that other reference documents already use.

Usage: python3 -I G10_xref_events.py <design_next/reference dir>
Prints, per file, every table row or requirement line that names a sound
event id (`source.event`) next to an sfx./amb. asset id or the word silent.
"""
import os
import re
import sys

EVENT = re.compile(r'`([a-z][a-z0-9_]*\.[a-z][a-z0-9_]*)`')


def main():
    root = sys.argv[1]
    for dirpath, _dirs, names in os.walk(root):
        for n in sorted(names):
            if not n.endswith('.md') or n.startswith('10_audio'):
                continue
            path = os.path.join(dirpath, n)
            with open(path, encoding='utf-8') as fh:
                lines = fh.read().split('\n')
            hits = []
            for i, line in enumerate(lines, 1):
                ids = [e for e in EVENT.findall(line)
                       if not e.startswith(('ref.', 'sfx.', 'amb.', 'music.', 'mesh.', 'ui.', 'unit.', 'building.',
                                            'crate.', 'mode.', 'match.', 'help.', 'icon.', 'fx.', 'sprite.',
                                            'cursor.', 'marker.', 'glyph.', 'creature.'))]
                if not ids:
                    continue
                if ('sfx.' in line or 'silent' in line or 'sound' in line.lower() or 'amb.' in line):
                    hits.append((i, line.strip()[:400]))
            if hits:
                print('=====', os.path.relpath(path, root))
                for i, l in hits:
                    print(i, l)


if __name__ == '__main__':
    main()
