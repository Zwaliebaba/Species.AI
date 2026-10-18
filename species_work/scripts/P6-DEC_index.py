"""P6-DEC: rebuild the decision-register README and refresh the per-file index rows.

Usage: python3 -I species_work/scripts/P6-DEC_index.py <design_next/decisions>

Writes only inside the decisions folder given as the argument:
  - README.md (head text below, totals, register-file table, full index);
  - the index rows of each register file (anchor, title and status of each DEC row).
"""
import os
import re
import sys

FILES = [
  ('verdicts_and_fixes.md', 'Headline verdicts, simulation-integrity rules, subsystem scope verdicts, rules kept as the reference states them, reference defects that Species fixes, design-critical uncertainties', 'DEC-001–119'),
  ('verdicts_and_fixes_2.md', 'Verdicts and owner decisions (continued)', 'DEC-120–199'),
  ('deferred/networking.md', 'Deferred: networking', 'DEC-200–219'),
  ('deferred/process_tick_and_roadmap.md', 'Deferred: process, tick rate and roadmap', 'DEC-220–239'),
  ('deferred/numerics_lobby_terrain_services.md', 'Deferred: numerics, lobby, terrain runtime, services, persistence', 'DEC-240–259'),
  ('simulation_gap_rules.md', 'Gap rules: simulation', 'DEC-260–299'),
  ('open_questions.md', 'Open questions', 'DEC-300–449'),
  ('units_buildings_crates_economy.md', 'Units, buildings, crates and population', 'DEC-450–549'),
  ('controls_bots_maps_meta_audio.md', 'Controls, bots, maps, front end and audio', 'DEC-550–649'),
  ('simulation_terrain_render_assets.md', 'Simulation, terrain, rendering and assets', 'DEC-650–749'),
  ('vfx_audio_ui.md', 'Animation and VFX, audio engine, UI', 'DEC-750–799'),
  ('data_sequences_platform_debug.md', 'Data, sequences, platform and diagnostics', 'DEC-800–849'),
  ('gameplay_gap_rules.md', 'Gap rules: gameplay', 'DEC-850–899'),
  (None, 'Reserved: no register file numbers into this block', 'DEC-900–949'),
  ('engine_gap_rules.md', 'Gap rules: engine', 'DEC-950–999'),
]

STATUSES = ('Open', 'Resolved', 'Dropped')

HEAD = """# Decision register

> Status: Decision register · Related: [conventions](../CONVENTIONS.md) §3 and §7, [deferred decisions](deferred/README.md), [vision brief](../../vision/species-vision-brief.md)

Entries live in the files listed below; add a decision to the file of its area, in the format of [CONVENTIONS](../CONVENTIONS.md) §7, and add its row to the index of that file and to the index below. **Open** decisions state the behaviour the documents specify now (the reference default), so implementation never blocks on them. **Resolved** decisions state the fixed rule. **Dropped** entries record out-of-scope items and what Species does instead. Decisions marked "Decide by: design phase" are answered by the Species design phase.

Totals: **{total}** decisions: {open} Open, {resolved} Resolved, {dropped} Dropped.

## Numbering

- A DEC id is stable: it is never renumbered or reused, and a decision that no longer applies becomes Dropped and keeps its id, so that every citation stays valid.
- A new decision of the reference library takes the next free id of the register file that owns its topic, within that file's block in the table below.
- The block DEC-900 to DEC-949 is reserved: no register file numbers into it.
- The Species design phase numbers its decisions with four-digit ids, starting at 1000 (CONVENTIONS §3).

## Register files

| File | Scope | Block | Entries |
|---|---|---|---|
"""


def slug(heading):
  h = re.sub(r'!\[([^\]]*)\]\([^)]*\)', r'\1', heading)
  h = re.sub(r'\[([^\]]*)\]\([^)]*\)', r'\1', h)
  h = re.sub(r'<[^>]+>', '', h)
  h = h.strip().lower()
  h = re.sub(r'[^\w\- ]', '', h)
  return h.replace(' ', '-')


def parse(path):
  """Return {id: (title, status, anchor)} in file order."""
  out = {}
  lines = open(path, encoding='utf-8').read().split('\n')
  in_code = False
  for i, line in enumerate(lines):
    if line.startswith('```'):
      in_code = not in_code
    if in_code:
      continue
    m = re.match(r'^### DEC-(\d{3}) (.+)$', line)
    if not m:
      continue
    num, title = int(m.group(1)), m.group(2).strip()
    status = None
    for j in range(i + 1, min(i + 8, len(lines))):
      s = re.match(r'^- Status: (\w+)', lines[j])
      if s:
        status = s.group(1)
        break
    if status not in STATUSES:
      raise SystemExit(f'{path}: DEC-{num:03d} has no valid status line')
    out[num] = (title, status, slug(line[4:]))
  return out


def refresh_rows(path, decs):
  text = open(path, encoding='utf-8').read()
  lines = text.split('\n')
  changed = 0
  for i, line in enumerate(lines):
    m = re.match(r'^\| (\[)?DEC-(\d{3})(?:\]\(#[^)]*\))? \|', line)
    if not m:
      continue
    num = int(m.group(2))
    if num not in decs:
      continue
    title, status, anchor = decs[num]
    cells = [c.strip() for c in line.strip().strip('|').split('|')]
    cells[0] = f'[DEC-{num:03d}](#{anchor})' if m.group(1) else f'DEC-{num:03d}'
    cells[1] = title
    for k in range(2, len(cells)):
      if cells[k] in STATUSES:
        cells[k] = status
    new = '| ' + ' | '.join(cells) + ' |'
    if new != line:
      lines[i] = new
      changed += 1
  if changed:
    open(path, 'w', encoding='utf-8', newline='\n').write('\n'.join(lines))
  return changed


def main():
  root = sys.argv[1]
  every = {}
  table = []
  for rel, scope, block in FILES:
    if rel is None:
      table.append(f'| — | {scope} | {block} | 0 |')
      continue
    decs = parse(os.path.join(root, rel))
    for n in decs:
      if n in every:
        raise SystemExit(f'DEC-{n:03d} defined twice')
      every[n] = (rel,) + decs[n]
    lo, hi = (int(x) for x in re.findall(r'\d+', block))
    for n in decs:
      if not lo <= n <= hi:
        raise SystemExit(f'{rel}: DEC-{n:03d} lies outside its block {block}')
    table.append(f'| [{rel}]({rel}) | {scope} | {block} | {len(decs)} |')
    c = refresh_rows(os.path.join(root, rel), decs)
    print(f'{rel}: {len(decs)} entries, {c} index rows refreshed')
  counts = {s: sum(1 for v in every.values() if v[2] == s) for s in STATUSES}
  out = HEAD.format(total=len(every), open=counts['Open'], resolved=counts['Resolved'], dropped=counts['Dropped'])
  out += '\n'.join(table) + '\n\n## Index\n\n| Id | Title | Status | File |\n|---|---|---|---|\n'
  for n in sorted(every):
    rel, title, status, anchor = every[n]
    out += f'| DEC-{n:03d} | [{title}]({rel}#{anchor}) | {status} | {rel} |\n'
  open(os.path.join(root, 'README.md'), 'w', encoding='utf-8', newline='\n').write(out)
  print(f'README.md: {len(every)} decisions, {counts}')


if __name__ == '__main__':
  main()
