"""Phase 0: generate the adoption-ledger skeleton (design-phase prompt §5.3, Phase 0 step 5).

Usage: python3 -I LedgerSkeleton.py <DESIGN_ROOT>

Writes <DESIGN_ROOT>/gdd/adoption_ledger/<reference document>_ledger.md: one row per reference section that holds
requirements (the deepest numbered heading above each `R-` definition), verdict `pending`, and a README with
the totals. A document that is a folder gets one ledger file named after the folder. Existing ledger files are
never overwritten: the script refuses to run if the folder already holds a ledger file.
"""
import collections
import os
import re
import sys

ROOT = os.path.abspath(sys.argv[1])
REF = os.path.join(ROOT, 'reference')
OUT = os.path.join(ROOT, 'gdd', 'adoption_ledger')
OWNER = {
    '00_reference_overview': 'A0', '01_skirmish_rules_and_modes': 'B9', '02_units': 'B4',
    '03_buildings_and_world_objects': 'B3', '04_supply_crates_powers_and_weapons': 'B4',
    '05_population_flow': 'B1', '06_controls_camera_and_commands': 'C1', '07_bots': 'B7',
    '08_maps_and_level_design': 'A1', '09_front_end_and_match_flow': 'C2', '10_audio_design': 'C4',
    '11_tutorial_and_help': 'C5', '12_simulation_model': 'D2',
}
HEAD = re.compile(r'^(#{2,4}) (\d+(?:\.\d+)*)\s+(.+?)\s*$')
DEF = re.compile(r'^\*\*(R-[A-Z]+-\d{3})\*\*')

if os.path.isdir(OUT) and any(n.endswith('.md') and n != 'README.md' for n in os.listdir(OUT)):
    sys.exit('ledger files already exist; refusing to overwrite')
os.makedirs(OUT, exist_ok=True)

docs = collections.OrderedDict()
for top in sorted(os.listdir(REF)):
    p = os.path.join(REF, top)
    if top == 'README.md':
        continue
    files = [p] if top.endswith('.md') else sorted(
        os.path.join(dp, f) for dp, _, fs in os.walk(p) for f in fs if f.endswith('.md'))
    docs[top.removesuffix('.md')] = files

totals = []
for doc, files in docs.items():
    rows = []
    for path in files:
        rel = os.path.relpath(path, REF).replace(os.sep, '/')
        head, sec = None, collections.OrderedDict()
        for line in open(path, encoding='utf-8'):
            h = HEAD.match(line)
            if h:
                head = (h.group(2), h.group(3))
                continue
            d = DEF.match(line)
            if d and head:
                sec.setdefault(head, []).append(d.group(1))
        for (num, title), ids in sec.items():
            rng = ids[0] if len(ids) == 1 else f'{ids[0]} to {ids[-1]}'
            rows.append(f'| `{rel} §{num}` | pending | | {title}: {len(ids)} requirement{"" if len(ids) == 1 else "s"} ({rng}) |')
    n_req = sum(int(re.search(r': (\d+) requirement', r).group(1)) for r in rows)
    owner = OWNER.get(doc, '?')
    text = [f'# Adoption ledger: {doc.replace("_", " ")}', '',
            f'> Status: Draft · Owner: area {owner} · Reference: [{doc}](../../reference/{doc}{".md" if len(files) == 1 and files[0].endswith(doc + ".md") else "/README.md"}) · '
            'Rules: `prompts/species-design-phase.md` §5.3', '',
            f'{len(rows)} sections, {n_req} requirements. A section row covers every requirement under its heading; an '
            'exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the '
            'Species requirements that replace it), `drop` (gives the reason).', '',
            '| Scope | Verdict | Species requirements | Note |', '|---|---|---|---|'] + rows
    open(os.path.join(OUT, f'{doc}_ledger.md'), 'w', encoding='utf-8').write('\n'.join(text) + '\n')
    totals.append((doc, owner, len(rows), n_req))

readme = ['# Adoption ledger', '',
          '> Status: Draft · Rules: `prompts/species-design-phase.md` §5.3 · Related: [GDD](../README.md)', '',
          'What Species does with every reference requirement, section by section, with exception rows for single '
          'requirements. Every `R-` id has exactly one effective verdict; the design check counts them.', '',
          '| Reference document | Owner | Sections | Requirements | adopt | adopt-skirmish | adapt | drop | pending |',
          '|---|---|---|---|---|---|---|---|---|']
for doc, owner, ns, nr in totals:
    readme.append(f'| [{doc}]({doc}_ledger.md) | {owner} | {ns} | {nr} | 0 | 0 | 0 | 0 | {nr} |')
readme.append(f'| **Total** | | **{sum(t[2] for t in totals)}** | **{sum(t[3] for t in totals)}** | 0 | 0 | 0 | 0 | '
              f'**{sum(t[3] for t in totals)}** |')
open(os.path.join(OUT, 'README.md'), 'w', encoding='utf-8').write('\n'.join(readme) + '\n')
print(f'{len(totals)} ledger files, {sum(t[2] for t in totals)} section rows, {sum(t[3] for t in totals)} requirements')
