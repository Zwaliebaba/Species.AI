"""Phase 4: write the design root README.md (the library's front page).

Usage: python3 -I p4_write_readme.py <DESIGN_ROOT> <gate_report.md>

The document map (files and sizes), the id counts and the gate table are computed from the tree and
from the gate report, so the page stays true after every regeneration.
"""
import glob
import os
import re
import sys

ROOT = os.path.abspath(sys.argv[1])
GATE = open(sys.argv[2], encoding='utf-8').read()


def kb(n):
    return f'{n / 1024:.0f} KB'


def size(path):
    if os.path.isfile(path):
        return os.path.getsize(path), 1
    total = files = 0
    for p in glob.glob(os.path.join(path, '**', '*'), recursive=True):
        if os.path.isfile(p):
            total += os.path.getsize(p)
            files += 1
    return total, files


def title(path):
    p = os.path.join(path, 'README.md') if os.path.isdir(path) else path
    if not os.path.exists(p):
        return ''
    m = re.search(r'^#\s+(.+)$', open(p, encoding='utf-8').read(), re.M)
    return m.group(1).strip() if m else ''


def rows(folder):
    out = []
    base = os.path.join(ROOT, folder)
    for name in sorted(os.listdir(base)):
        p = os.path.join(base, name)
        if name == 'README.md' or name.startswith('.'):
            continue
        if os.path.isdir(p) or name.endswith(('.md', '.csv')):
            b, n = size(p)
            link = f'{folder}/{name}/README.md' if os.path.isdir(p) else f'{folder}/{name}'
            out.append(f'| [{title(p) or name}]({link}) | {n} | {kb(b)} |')
    return out


text = {p: open(p, encoding='utf-8').read() for p in glob.glob(os.path.join(ROOT, '**', '*.md'), recursive=True)}
alltext = '\n'.join(text.values())
m = re.search(r'(\d[\d,]*) requirement ids defined', GATE)
n_req = m.group(1) if m else '?'
m = re.search(r'(\d[\d,]*) tuning keys', GATE)
n_keys = m.group(1) if m else '?'
m = re.search(r'(\d[\d,]*) asset ids \((\d[\d,]*) manifest rows\)', GATE)
n_assets = f'{m.group(1)} ids, {m.group(2)} files' if m else '?'
dec_text = '\n'.join(t for p, t in text.items() if '/decisions/' in p.replace(os.sep, '/') and not p.endswith('README.md'))
decs = re.findall(r'^#{2,4} DEC-(\d{3,4})\s', dec_text, re.M)
statuses = re.findall(r'^#{2,4} DEC-\d{3,4}\s.*\n(?:.*\n){0,7}?- \*{0,2}Status\*{0,2}:\*{0,2}\s*([A-Za-z]+)', dec_text, re.M)
adrs = glob.glob(os.path.join(ROOT, 'ADR', 'ADR-[0-9][0-9][0-9]-*.md'))
adr_status = []
for p in adrs:
    m = re.search(r'^status:\s*(\w+)', open(p, encoding='utf-8').read(), re.M)
    adr_status.append((m.group(1) if m else '').lower())
gates = [l for l in GATE.split('\n') if l.startswith('| ') and re.match(r'^\| \d+ ', l)]
n_docs = len(text)
total_b = sum(os.path.getsize(p) for p in glob.glob(os.path.join(ROOT, '**', '*'), recursive=True) if os.path.isfile(p))

out = f"""# Species reference library

> Status: Reference library, input to the Species design phase · Related: [vision brief](../vision/species-vision-brief.md) · [design-phase prompt](../prompts/species-design-phase.md) · [CONVENTIONS](CONVENTIONS.md) · [Requirements index](REQUIREMENTS_INDEX.md)

This library is the **clean reference** that the Species design phase builds on. It is **not the Species game design.** It specifies, faithfully and in Species terms, the mechanics, numbers, states, look and feel of a reference crowd-battle skirmish (`reference/`), and the engine baseline that implements them on Windows and Direct3D 12 (`architecture/components/` and `ADR/`). The design phase, run with `prompts/species-design-phase.md`, adopts, adapts or drops each requirement by id to design Species: the persistent world of settlements described in the [vision brief](../vision/species-vision-brief.md), whose owner decisions O1–O30 bind every document here.

How the code is written and built (naming, the MSBuild solution, the dependency allow-list, ADR rules) is decided by `AGENTS.md` at the repository root; where a baseline ADR and `AGENTS.md` disagree, `AGENTS.md` wins.

## Reading guide

1. **[CONVENTIONS](CONVENTIONS.md)**: normative language, the id scheme and prefix registry, tuning tables, units (world units `wu` and ticks), naming and the allowed terms.
2. **[Reference overview](reference/00_reference_overview.md)**: the core loop and the vocabulary; then the **[simulation model](reference/12_simulation_model/README.md)** that every other reference document relies on (tick, identity, update order, randomness).
3. **The match**: [skirmish rules and modes](reference/01_skirmish_rules_and_modes/README.md), [units](reference/02_units/README.md), [buildings and world objects](reference/03_buildings_and_world_objects/README.md), [supply crates, powers and weapons](reference/04_supply_crates_powers_and_weapons/README.md), [population flow](reference/05_population_flow.md); then control, bots, maps, front end, audio and the tutorial ([reference index](reference/README.md)).
4. **The engine baseline**: the [component index](architecture/components/README.md) and the [ADR index and technology baseline](ADR/README.md).
5. **Open questions**: the [decision register](decisions/README.md). An Open entry states the behaviour every document specifies today, so implementation never blocks on it; the [deferred decisions](decisions/deferred/README.md) are the world-game questions the design phase answers first.
6. **Assets**: [GameAssets.md](assets/GameAssets.md) lists every asset to create, with its Species file name, format, specification and priority; the [manifest](assets/asset_manifest.csv) is its machine-readable form.

## Document map

{n_docs} documents, {kb(total_b)} in total. Sizes include every file of a folder.

| Area | Files | Size |
|---|---|---|
"""
for folder, label in [('reference', 'Reference mechanics'), ('architecture/components', 'Engine components'),
                      ('ADR', 'Architecture decision records'), ('decisions', 'Decision register'),
                      ('assets', 'Assets'), ('glossary', 'Glossary'), ('requirements_index', 'Requirements index pages')]:
    b, n = size(os.path.join(ROOT, folder))
    out += f'| [{label}]({folder}/README.md) | {n} | {kb(b)} |\n'
out += '\n### Reference mechanics\n\n| Document | Files | Size |\n|---|---|---|\n' + '\n'.join(rows('reference')) + '\n'
out += '\n### Engine components\n\n| Component | Files | Size |\n|---|---|---|\n' + '\n'.join(rows('architecture/components')) + '\n'
out += f"""
## Ids in brief

| Id | Form | Where | Count |
|---|---|---|---|
| Requirement | `R-UNIT-014` (reference), `TER-012` (component): prefix registry in [CONVENTIONS](CONVENTIONS.md) §3.1 | `**ID** (MUST) …` in the owning document; [index](REQUIREMENTS_INDEX.md) | {n_req} |
| Tuning key | `ref.unit.worker.move_speed`, `ter.generator.octaves` | the tuning table of the owning document | {n_keys} |
| Decision | `DEC-NNN`, Open, Resolved or Dropped | [decisions/](decisions/README.md) | {len(decs)} ({statuses.count('Open')} Open, {statuses.count('Resolved')} Resolved, {statuses.count('Dropped')} Dropped) |
| Architecture decision | `ADR-NNN` (MADR 4) | [ADR/](ADR/README.md) | {len(adrs)} ({adr_status.count('accepted')} Accepted, {adr_status.count('proposed')} Proposed, {adr_status.count('rejected')} Rejected) |
| Asset | `sprite.worker.front`, `sfx.unit.builder.presence_loop` | [assets/asset_manifest.csv](assets/asset_manifest.csv) | {n_assets} |
| String id | `ui.lobby.ready`, `unit.worker.name` | the string tables of [assets/](assets/GameAssets.md) | listed in GameAssets §18.1 |

Ids are stable: they are never renumbered or reused, and the design phase cites them when it adopts, adapts or drops a rule.

## Status

The design gates (`python3 Tools/DesignCheck/CheckDesign.py Design` from the repository root) pass on this library:

| Gate | Result | Failures | Summary |
|---|---|---|---|
""" + '\n'.join(gates) + '\n'
open(os.path.join(ROOT, 'README.md'), 'w', encoding='utf-8').write(out)
print(f'README.md written: {len(out.encode()) // 1024} KB, {n_docs} documents, {n_req} requirements, {len(decs)} DECs, {len(adrs)} ADRs')
