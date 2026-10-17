#!/usr/bin/env python3
"""S06 helper: build the full ControlType binding reference table for
06_player_control_and_input.md from drafts/P10_input_camera_bindings.csv, and
independently cross-check the CSV against the source data:

  * code/lib/input/control_types.inc  (164 DEF_CONTROL_TYPE rows, order = enum value)
  * data/input/input_preferences.txt  (raw '=' / '~' lines per control, in file order)

Every raw '=' line of the PC default file must appear (verbatim, modulo whitespace and the
P10 '[=down: ...]' annotation) in the CSV's kbm/xinput/other columns, unless it is one of the
lines the engine rejects (no driver accepts 'SpaceNav'). Mismatches are printed to stderr.

Usage: python3 -I S_06_bindings_table.py <SRC root> <bindings.csv> <out.md>
"""
import csv, re, sys

SRC, CSV_PATH, OUT = sys.argv[1:4]

# ---------------------------------------------------------------- control_types.inc
controls = []
with open(SRC + '/code/lib/input/control_types.inc', encoding='latin-1') as f:
    for line in f:
        s = line.strip()
        if s.startswith('//'):
            continue
        m = re.match(r'DEF_CONTROL_TYPE\(\s*(\w+)\s*,\s*(INPUT_TYPE_\w+)\s*\)', s)
        if m:
            controls.append((m.group(1), m.group(2).replace('INPUT_TYPE_', '')))

rows = list(csv.DictReader(open(CSV_PATH, encoding='utf-8')))
problems = []
if len(controls) != 164:
    problems.append('control_types.inc has %d controls, expected 164' % len(controls))
if len(rows) != len(controls):
    problems.append('CSV rows %d != controls %d' % (len(rows), len(controls)))
for i, (name, typ) in enumerate(controls):
    if i < len(rows):
        r = rows[i]
        if r['control'] != name or int(r['idx']) != i:
            problems.append('row %d: CSV %s/%s vs inc %s' % (i, r['idx'], r['control'], name))
        if r['type'] != typ:
            problems.append('row %d %s: CSV type %s vs inc %s' % (i, name, r['type'], typ))

# ---------------------------------------------------------------- raw PC default file
raw = {}
with open(SRC + '/data/input/input_preferences.txt', encoding='latin-1') as f:
    for ln, line in enumerate(f, 1):
        s = line.split('#', 1)[0].strip()
        if not s:
            continue
        m = re.match(r'(\w+)[\s:,]*([=~])\s*(.*)$', s)
        if not m:
            continue
        ctl, op, spec = m.group(1), m.group(2), ' '.join(m.group(3).split())
        if op == '=':
            raw.setdefault(ctl.lower(), []).append((ln, spec))


def norm(cell):
    out = []
    for part in cell.split(' | '):
        part = re.sub(r'\s*\[=down:[^\]]*\]', '', part).strip()
        if part:
            out.append(' '.join(part.split()))
    return out

checked = 0
for r in rows:
    have = norm(r['kbm_default']) + norm(r['xinput_default']) + norm(r['other_default'])
    for ln, spec in raw.get(r['control'].lower(), []):
        checked += 1
        if spec.startswith('SpaceNav'):
            continue  # rejected by the engine: no SpaceNav driver
        if spec not in have:
            problems.append('input_preferences.txt:%d %s = %s missing from CSV' % (ln, r['control'], spec))
known = {c[0].lower() for c in controls}
for ctl in raw:
    if ctl not in known:
        problems.append('binding for unknown control %s' % ctl)

# ---------------------------------------------------------------- markdown


def cell(v, code=True):
    v = (v or '').strip()
    if not v:
        return '—'
    parts = [p.strip() for p in v.split(' | ')]
    fixed = []
    for p in parts:
        note = ''
        m = re.search(r'\s*\[=down:[^\]]*\]', p)
        if m:
            p = p[:m.start()] + p[m.end():]
            note = ' ⚑'
        fixed.append(('`%s`' % p.replace('|', '\\|') if code else p) + note)
    return ' · '.join(fixed)


def consumers(v):
    v = (v or '').strip()
    if not v:
        return '*none (dead in FULL)*'
    return ', '.join('`code/%s`' % c for c in v.split(';'))


lines = []
lines.append('| # | ControlType | Type | Purpose (source comment) | Keyboard / mouse default | Pad (XInput) default | Other drivers | Pad icon (`~`) | Live consumers (FULL build) |')
lines.append('|---|---|---|---|---|---|---|---|---|')
for r in rows:
    lines.append('| %s | `%s` | %s | %s | %s | %s | %s | %s | %s |' % (
        r['idx'], r['control'], r['type'],
        (r['comment'] or '—').replace('|', '\\|').strip('"'),
        cell(r['kbm_default']), cell(r['xinput_default']), cell(r['other_default']),
        ('`%s`' % r['icon']) if r['icon'] else '—', consumers(r['live_consumers'])))

lines.append('')
lines.append('VARIANTS')
lines.append('| # | ControlType | Darwinia-scheme file: KB/M | Darwinia-scheme file: pad | Mac file KB/M [B:DEAD] | `de_DE` primary override | `fr_FR` primary override |')
lines.append('|---|---|---|---|---|---|---|')
nvar = 0
for r in rows:
    vals = [r['darwinia_kbm_diff'], r['darwinia_xinput_diff'], r['mac_kbm_diff'],
            r['de_DE_primary_override'], r['fr_FR_primary_override']]
    if any(v.strip() for v in vals):
        nvar += 1
        lines.append('| %s | `%s` | %s | %s | %s | %s | %s |' % (
            r['idx'], r['control'], cell(vals[0]), cell(vals[1]), cell(vals[2]),
            cell(vals[3]), cell(vals[4])))

with open(OUT, 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines) + '\n')

print('controls=%d rows=%d raw_lines_checked=%d variant_rows=%d problems=%d'
      % (len(controls), len(rows), checked, nvar, len(problems)))
for p in problems:
    print('PROBLEM:', p, file=sys.stderr)
