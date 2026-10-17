"""G10: assemble the event catalogue from its template and the generator JSON.

Usage: python3 -I G10_assemble.py <template.md> <cat.json> <out.md>
"""
import json
import re
import sys

HEADER = ('| Event | Raised when | Sample group | Playback | Min (wu) | Level (dB) | Pitch (×) '
          '| Envelope and loop (s) | Effect |\n|---|---|---|---|---|---|---|---|---|')


def main():
    tpl = open(sys.argv[1], encoding='utf-8').read()
    data = json.load(open(sys.argv[2], encoding='utf-8'))
    tables = data['tables']
    silent = data['silent']
    # per-source silent counts by the event id's own source
    sil_count = {}
    for row in silent:
        ev = re.match(r'\| `([a-z_]+)\.', row).group(1)
        sil_count[ev] = sil_count.get(ev, 0) + 1
    summary = []
    for src, title, kind, nlive, _nb, _ns in data['summary']:
        layers = len(tables.get(src, []))
        if nlive == 0 and sil_count.get(src, 0) == 0:
            continue
        summary.append('| `%s` | %s | %s | %d | %d | %d |' % (src, title, kind, nlive, layers, sil_count.get(src, 0)))
    tot_live = sum(r[3] for r in data['summary'])
    tot_layers = sum(len(v) for v in tables.values())
    summary.append('| total | | | %d | %d | %d |' % (tot_live, tot_layers, len(silent)))
    out = tpl.replace('{{SUMMARY}}', '\n'.join(summary))
    out = out.replace('{{SILENT}}', '\n'.join(silent))
    used = set()
    for src, lines in tables.items():
        for ln in lines:
            for a in re.findall(r'`((?:sfx|amb|music)\.[a-z0-9_.]+)`', ln):
                used.add(a)
        ph = '{{T:%s}}' % src
        if ph in out:
            out = out.replace(ph, HEADER + '\n' + '\n'.join(lines))
        elif lines:
            raise SystemExit('no placeholder for ' + src)
    out = out.replace('{{ASSETS}}', ', '.join('`%s`' % a for a in sorted(used)))
    left = re.findall(r'\{\{[^}]+\}\}', out)
    if left:
        raise SystemExit('unfilled: %s' % left)
    open(sys.argv[3], 'w', encoding='utf-8').write(out)
    print('layers', tot_layers, 'live', tot_live, 'silent', len(silent), 'assets', len(used))


if __name__ == '__main__':
    main()
