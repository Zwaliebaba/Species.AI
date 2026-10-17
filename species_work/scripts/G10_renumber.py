"""G10: renumber the R-AUD requirements consecutively per file, in reading
order, starting at each file's range start, and rewrite every reference.

Usage: python3 -I G10_renumber.py <folder> <extra files...>
The file order and range starts are fixed below. Extra files (the catalogue
template and generator) receive the same reference rewrite.
"""
import os
import re
import sys

ORDER = [('README.md', 1), ('event_model_and_sound_data.md', 20), ('event_catalogue.md', 120),
         ('positional_priorities_and_mix.md', 200), ('front_end_hud_voice_and_ambience.md', 300),
         ('music_hooks_and_sound_design_brief.md', 400)]
RANGE_END = {1: 19, 20: 119, 120: 199, 200: 299, 300: 399, 400: 499}
DEF = re.compile(r'\*\*R-AUD-(\d{3})\*\*')


def main():
    folder = sys.argv[1]
    extras = sys.argv[2:]
    mapping = {}
    spans = {}
    for name, start in ORDER:
        t = open(os.path.join(folder, name), encoding='utf-8').read()
        ids = DEF.findall(t)
        n = start
        for i in ids:
            if i in mapping:
                raise SystemExit('duplicate definition R-AUD-%s' % i)
            mapping[i] = '%03d' % n
            n += 1
        if n - 1 > RANGE_END[start]:
            raise SystemExit('%s overflows its range' % name)
        spans[name] = (start, n - 1)

    def sub(m):
        return 'R-AUD-' + mapping.get(m.group(1), m.group(1))
    targets = [os.path.join(folder, n) for n, _s in ORDER] + extras
    for p in targets:
        t = open(p, encoding='utf-8').read()
        t2 = re.sub(r'R-AUD-(\d{3})', sub, t)
        if p.endswith('README.md'):
            for name, (a, b) in spans.items():
                t2 = re.sub(r'(\[%s\]\(%s\) \|[^\n]*\| )R-AUD-\d{3} to R-AUD-\d{3}( \|)' % (re.escape(name), re.escape(name)),
                            lambda m: '%sR-AUD-%03d to R-AUD-%03d%s' % (m.group(1), a, b, m.group(2)), t2)
        open(p, 'w', encoding='utf-8').write(t2)
    changed = sum(1 for k, v in mapping.items() if k != v)
    print('renumbered', len(mapping), 'changed', changed, 'spans', spans)


if __name__ == '__main__':
    main()
