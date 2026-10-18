#!/usr/bin/env python3
"""SelfTestDesignPhase.py: prove that the design-phase gates of CheckDesign.py (gates 12 to 19, --design-phase and
--final) find what they must and nothing else.

Usage:
  python3 -I Tools/DesignCheck/SelfTestDesignPhase.py

It needs no input. Each case writes a small synthetic repository into a temporary directory: a design root (Design/)
with reference documents, ledger files, GDD files, components, rationale files and decisions, Tools/DesignModels/, and
the files a repository scan reads. It runs CheckDesign.py over it and compares the failures and the reported findings
of one gate with what the case expects, message by message. The repository scan runs with a synthetic pattern file,
denylist and allow-list written for the case, so no fixture names anything real. Gates 1 to 11 run too, because the
design-phase gates build on them, but their results over the synthetic root are not compared. Exit status 0 when every
case passes, 1 otherwise.
"""
import importlib.util
import os
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))

CONVENTIONS = '''\
# Conventions

## 3 Requirement IDs

### 3.1 Prefix registry

| Prefix | Scope | Document |
|---|---|---|
| `R-UNIT` | Units | `reference/02_units/` |
| `R-ECON` | Population flow | `reference/05_population_flow.md` |
| `WLD` | World and regions | `gdd/01_world_and_regions.md` |
| `SET` | Settlements | `gdd/02_settlements_and_population.md` |
| `SRV` | Server and hosting | `architecture/components/server_and_hosting.md` |

## 9 Allowed terms

| Term | Paths |
|---|---|
'''

WORKER = '''\
# Worker

> Status: Reference · Prefix: R-UNIT

## 1 Purpose and player experience

The Worker is the population unit.

## 2 Rules

### 2.1 Role

**R-UNIT-001** (MUST) A Worker is an ungrouped actor of its team.

**R-UNIT-002** (MUST) A Worker throws a grenade as R-UNIT-005 states.

### 2.2 Combat

**R-UNIT-003** (MUST) A Worker fights enemy Workers at `ref.unit.worker.range` (100 wu).

#### 2.2.1 Grenades

**R-UNIT-004** (MUST) A Worker carries grenades; R-UNIT-005 gives the fuse.

**R-UNIT-005** (MUST) A grenade explodes when its fuse runs out.

```
### 2.9 Inside a code block
**R-UNIT-099** (MUST) Not a definition: it is inside a code block.
```

## 3 States and flows

The Worker has no states of its own here.

## 4 Tuning table

| Key | Value | Unit | Range or notes | Requirement | DEC |
|---|---|---|---|---|---|
| `ref.unit.worker.range` | 100 | wu | 50 to 150 | R-UNIT-003 | |
'''

POPULATION = '''\
# Population flow

> Status: Reference · Prefix: R-ECON

## 2 Rules

### 2.1 Growth

**R-ECON-001** (MUST) A Spawn Node grows the population.

### Notes on the limit

**R-ECON-002** (MUST) The population never exceeds its limit.
'''

UNITS_README = '''\
# Units

## 2 Rules

The unit documents of this folder hold the rules.
'''

DECISIONS = '''\
# Open questions

### DEC-300 First question
- Status: Open
- Affects: reference/02_units/worker.md (R-UNIT)
- Decide by: design phase

### DEC-301 Second question
- Status: Open
- Decide by: playtest

### DEC-302 Third question
- Status: Resolved
- Decide by: design phase

### DEC-1000 Fourth question
- Status: Open
- Decide by: design phase (wave B)
'''

OWNER = '''\
# Owner decisions

| # | Topic | Decision |
|---|---|---|
| O1 | Scale | Start with a small world. |
| **O2** | Away play | A settlement can be attacked while its owner is away. |

### O31 Storefronts

The owner chose the storefronts.
'''

LEDGER_HEAD = '| Scope | Verdict | Species requirements | Note |\n|---|---|---|---|\n'

BASE = {
  'Design/CONVENTIONS.md': CONVENTIONS,
  'Design/reference/README.md': '# Reference\n',
  'Design/reference/02_units/README.md': UNITS_README,
  'Design/reference/02_units/worker.md': WORKER,
  'Design/reference/05_population_flow.md': POPULATION,
  'Design/decisions/README.md': '# Decision register\n',
  'Design/decisions/open_questions.md': DECISIONS,
  'Design/decisions/owner_decisions.md': OWNER,
  'Design/architecture/components/README.md': '# Engine components\n',
  'Tools/DesignModels/WorldDensity.py': 'print("settlements per region")\n',
  'Tools/DesignCheck/ScanAllow.tsv': 'path\tsha256\treason\n',
}

GDD_SECTIONS = [
  ('1 Purpose and player experience', 'The world is a set of regions.'),
  ('2 Rules', '{rules}'),
  ('3 States and flows', 'A region is open or closed.'),
  ('4 Tuning table', '{tuning}'),
  ('5 Edge cases, exploits and interactions', 'A region with no site holds no settlement.'),
  ('6 Acceptance criteria', 'A world of 12 regions loads (WLD-001).'),
  ('7 Adoption from the reference', 'The ledger lists the verdicts.'),
  ('8 Dependencies', 'The world simulation component.'),
  ('9 Open decisions', 'None.'),
]

GOOD_TUNING = '''\
| Key | Value | Unit | Status | Requirement | DEC | Playtest note |
|---|---|---|---|---|---|---|
| `wld.region.size_max` | 4 | km | final | WLD-002 | | |
| `wld.region.count` | 12 | regions | provisional | WLD-001 | | confirm at 16 players |'''

GOOD_RULES = '''\
**WLD-001** (MUST) A world has `wld.region.count` regions.

**WLD-002** (MUST) A region is at most `wld.region.size_max` across.'''


def gdd(title, status='> Status: Draft · Prefix: WLD · Rationale: ../rationale/A1.md', rules=GOOD_RULES,
        tuning=GOOD_TUNING, omit=()):
  out = [f'# {title}', '']
  if status:
    out += [status, '']
  for head, body in GDD_SECTIONS:
    if head in omit:
      continue
    out += [f'## {head}', '', body.format(rules=rules, tuning=tuning), '']
  return '\n'.join(out)


COMPONENT_HEADINGS = ['1 Responsibilities and boundaries', '2 Interfaces', '3 Data model', '4 Behaviour and algorithms',
                      '5 Concurrency and performance budget', '6 Failure modes, diagnostics and telemetry',
                      '7 Acceptance tests', '8 Open decisions']


def component(title, rationale, scale_path=True, extra=''):
  out = [f'# {title}', '', f'> Status: Draft · Prefix: SRV · Rationale: {rationale}', '']
  for head in COMPONENT_HEADINGS + (['9 Scale path'] if scale_path else []):
    out += [f'## {head}', '', 'Text.', '']
  return '\n'.join(out) + extra


RATIONALE_A1 = '''\
# A1 rationale

## Weakest point

The world may be too large for 16 players.

## Findings and resolutions

| Id | Severity | Category | Finding | Resolution |
|---|---|---|---|---|
| A1-F01 | Critical | scale and cost | The world is too large. | Accepted: WLD-001 caps the regions. |
| A1-F02 | **High** | player experience | Travel is slow. | Rejected: travel time is the pillar. |
| A1-F03 | Medium | player experience | Sites cluster. | |

## Model results

`Tools/DesignModels/WorldDensity.py` gives 3 settlements per region at 16 players.
'''

GOOD_SPECIES = {
  'Design/gdd/README.md': '# Species game design\n\n| Document | Status |\n|---|---|\n| world | Draft |\n',
  'Design/gdd/01_world_and_regions.md': gdd('World and regions'),
  'Design/rationale/README.md': '# Rationale\n',
  'Design/rationale/A1.md': RATIONALE_A1,
  'Design/rationale/D1.md': '# D1 rationale\n\n## Findings and resolutions\n\nNo Critical or High findings.\n',
  'Design/architecture/components/server_and_hosting.md': component('Server and hosting', '../../rationale/D1.md'),
  'Design/architecture/components/renderer/README.md': '# Renderer\n',
  'Design/architecture/components/renderer/extra_pass.md': component('Extra pass', '', scale_path=False, extra='''
## 10 Tuning table

| Key | Value | Unit | Range or notes | Requirement | DEC |
|---|---|---|---|---|---|
| `rnd.extra.count` | 2 | passes | 1 to 4 | | |
'''),
}


def ledger(rows):
  return '# Ledger\n\n' + LEDGER_HEAD + '\n'.join(f'| {" | ".join(r)} |' for r in rows) + '\n'


def scan_files(allow_rows):
  rows = ''.join(f'{p}\t{d}\t{r}\n' for p, d, r in allow_rows)
  return {'Tools/DesignCheck/ScanAllow.tsv': 'path\tsha256\treason\n' + rows}


def load_check_design():
  spec = importlib.util.spec_from_file_location('CheckDesign', os.path.join(HERE, 'CheckDesign.py'))
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


class Runner:
  def __init__(self, cd, config):
    self.cd = cd
    self.config = config
    self.failed = []
    self.n = 0

  def run(self, files, final=False, design_phase=True):
    """Write BASE updated by files (a None value removes a file) into a fresh repository and run every gate."""
    tree = dict(BASE)
    tree.update(files)
    with tempfile.TemporaryDirectory(prefix='designcheck-') as tmp:
      for rel, text in tree.items():
        if text is None:
          continue
        path = os.path.join(tmp, *rel.split('/'))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as fh:
          fh.write(text if isinstance(text, bytes) else text.encode('utf-8'))
      cd = self.cd
      cd.TECH_PATTERNS, cd.IP_DENYLIST, cd.STRINGS_DENYLIST = self.config
      cd.SCAN_ALLOW = os.path.join(tmp, 'Tools', 'DesignCheck', 'ScanAllow.tsv')
      check = cd.Check(os.path.join(tmp, 'Design'), skip_mermaid=True, design_phase=design_phase, final=final)
      check.run()
      return check

  def expect(self, label, check, gate, fails=(), notes=(), info=None):
    """fails and notes: one substring per expected failure or reported finding, in any order; nothing else may
    appear. info: a substring of the gate's summary."""
    self.n += 1
    problems = []
    for kind, want, got in (('failure', fails, check.results.get(gate, [])),
                            ('reported finding', notes, check.notes.get(gate, []))):
      left = list(got)
      for w in want:
        hit = next((g for g in left if w in g), None)
        if hit is None:
          problems.append(f'no {kind} contains "{w}"')
        else:
          left.remove(hit)
      problems += [f'unexpected {kind}: {g}' for g in left]
    summary = check.info.get(gate, '')
    if info is not None and info not in summary:
      problems.append(f'the summary "{summary}" lacks "{info}"')
    self.record(f'gate {gate:2} {label}', problems)

  def record(self, label, problems):
    print(('PASS ' if not problems else 'FAIL ') + label)
    for p in problems:
      print('       ' + p)
    if problems:
      self.failed.append(label)


def main():
  cd = load_check_design()
  with tempfile.TemporaryDirectory(prefix='designcheck-config-') as cfg:
    patterns = os.path.join(cfg, 'TechPatterns.tsv')
    with open(patterns, 'w', encoding='utf-8') as fh:
      fh.write('pattern\tflags\tgate\tclass\tnote\nFrobni(?:cate)\t\thard\ttest-pattern\tsynthetic hard pattern\n')
    denylist = os.path.join(cfg, 'IpDenylist.sha256')
    with open(denylist, 'w', encoding='utf-8') as fh:
      fh.write(cd.digest(cd.normalise_phrase('zorbulon')) + '\n')
    strings = os.path.join(cfg, 'OriginalStrings.sha256')
    with open(strings, 'w', encoding='utf-8') as fh:
      fh.write('# no entries\n')
    t = Runner(cd, (patterns, denylist, strings))
    cases(t, cd)
  print(f'SelfTestDesignPhase: {t.n - len(t.failed)} of {t.n} checks pass: ' + ('PASS' if not t.failed else 'FAIL'))
  return 1 if t.failed else 0


def cases(t, cd):
  # -------------------------------------------------- the option itself
  c = t.run({}, design_phase=False)
  t.record('without --design-phase only gates 1 to 11 run',
           [] if [g for g, *_ in c.table()] == list(range(1, 12)) and not c.notes else ['gates 12 and up ran'])

  # -------------------------------------------------- Phase 0: nothing written yet
  c = t.run({})
  t.record('with --design-phase gates 1 to 19 run', [] if [g for g, *_ in c.table()] == list(range(1, 20)) else
           ['the gate list is not 1 to 19'])
  t.expect('no ledger folder yet: pass', c, 12, info='no gdd/adoption_ledger/ yet; 7 R- ids (5 sections, 2 reference '
           'files)')
  t.expect('design-phase decisions are reported', c, 13, notes=['DEC-300 First question', 'DEC-1000 Fourth'],
           info='3 Open DECs, 2 with Decide by: design phase (reported); playtest 1')
  t.expect('no gdd/ folder yet: pass', c, 14, info='nothing to check yet: no gdd/ folder')
  t.expect('no rationale yet: pass', c, 15, info='nothing to check yet')
  t.expect('no owner decision cited: pass', c, 16, info='3 owner decisions')
  t.expect('no Species tuning table yet: pass', c, 17, info='nothing to check yet')
  t.expect('model-required areas without a rationale are reported', c, 18,
           notes=[f'area {a} has no rationale' for a in cd.MODEL_AREAS], info='nothing to check yet')
  t.expect('clean repository: pass', c, 19, info='0 hard-fail hits')

  c = t.run({}, final=True)
  t.expect('--final: a missing ledger fails', c, 12, fails=['gdd/adoption_ledger/ does not exist'])
  t.expect('--final: design-phase decisions fail', c, 13, fails=['DEC-300', 'DEC-1000'])
  t.expect('--final: model-required areas without a rationale fail', c, 18,
           fails=[f'area {a} has no rationale' for a in cd.MODEL_AREAS])

  # -------------------------------------------------- gate 12: specificity, pending, dropped citations
  passing = dict(GOOD_SPECIES)
  passing['Design/gdd/adoption_ledger/README.md'] = ('# Adoption ledger\n\n| Document | adopt |\n|---|---|\n'
                                                     '| units | 3 |\n')
  passing['Design/gdd/adoption_ledger/02_units_ledger.md'] = ledger([
    ('`02_units/worker.md`', 'adopt', '', 'the Worker as written'),
    ('`02_units/worker.md §2.2`', 'adapt', 'WLD-001', 'the fight range moves to the world rules'),
    ('`02_units/worker.md §2.2.1`', '`adopt-skirmish`', '', 'grenades in the skirmish only'),
    ('`R-UNIT-005`', 'drop', 'WLD-002', 'the fuse is replaced by WLD-002'),
  ])
  passing['Design/gdd/adoption_ledger/05_population_flow_ledger.md'] = ledger([
    ('`05_population_flow.md §2`', 'pending', '', ''),
    ('`R-ECON-001`', 'adopt', '', ''),
  ])
  c = t.run(passing)
  want = {'R-UNIT-001': 'adopt', 'R-UNIT-002': 'adopt', 'R-UNIT-003': 'adapt', 'R-UNIT-004': 'adopt-skirmish',
          'R-UNIT-005': 'drop', 'R-ECON-001': 'adopt', 'R-ECON-002': 'pending'}
  got = getattr(c, 'ledger_verdicts', {})
  t.record('gate 12 the most specific row wins (id, deepest heading, file; unnumbered heading under its parent)',
           [] if got == want else [f'effective verdicts {got}'])
  t.expect('a pending row is reported; a drop that names a replacement is not', c, 12,
           notes=['"05_population_flow.md §2" is pending (1 R- ids)'],
           info='7 R- ids (5 sections, 2 reference files), 6 rows in 2 files: adopt 3, adopt-skirmish 1, adapt 1, '
                'drop 1, pending 1, no verdict 0')
  c = t.run(passing, final=True)
  t.expect('--final: a pending row fails', c, 12, fails=['"05_population_flow.md §2" is pending'])

  dropped = dict(passing)
  dropped['Design/gdd/adoption_ledger/02_units_ledger.md'] = ledger([
    ('`02_units/worker.md`', 'adopt', '', 'the Worker as written'),
    ('`02_units/worker.md §2.2`', 'adapt', 'WLD-001', 'the fight range moves to the world rules'),
    ('`02_units/worker.md §2.2.1`', 'adopt-skirmish', '', 'grenades in the skirmish only'),
    ('`R-UNIT-005`', 'drop', '', 'there is no fuse in Species'),
  ])
  c = t.run(dropped)
  t.expect('a dropped id cited by adopt and adopt-skirmish requirements is reported', c, 12,
           notes=['"05_population_flow.md §2" is pending',
                  'R-UNIT-002 (adopt) cites R-UNIT-005, which gdd/adoption_ledger/02_units_ledger.md:8 drops',
                  'R-UNIT-004 (adopt-skirmish) cites R-UNIT-005'])

  # -------------------------------------------------- gate 12: every failure
  failing = dict(GOOD_SPECIES)
  failing['Design/gdd/adoption_ledger/02_units_ledger.md'] = ledger([
    ('`02_units/worker.md §2.1`', 'adopt', '', ''),
    ('`02_units/worker.md §2.1`', 'adopt', '', 'a second row for the same heading'),
    ('`R-UNIT-003`', 'adopt', '', ''),
    ('`R-UNIT-003`', 'drop', '', ''),
    ('`02_units/worker.md §2.2.1`', 'keep', '', ''),
    ('`R-UNIT-004`', 'adapt', '', 'the grenade moves to the battle rules'),
    ('`R-UNIT-005`', 'adapt', 'WLD-009', 'the fuse moves'),
    ('`02_units/missing.md`', 'adopt', '', ''),
    ('`02_units/worker.md §7`', 'adopt', '', ''),
    ('`02_units/worker.md §3`', 'adopt', '', ''),
    ('`02_units/README.md`', 'adopt', '', ''),
    ('`R-UNIT-099`', 'adopt', '', ''),
    ('all of the worker', 'adopt', '', ''),
  ])
  failing['Design/gdd/adoption_ledger/05_population_flow_ledger.md'] = ledger([
    ('`05_population_flow.md §2.1`', 'adopt', '', ''),
  ])
  failing['Design/gdd/adoption_ledger/notes_ledger.md'] = '# Notes\n\nThis file has no ledger table.\n'
  c = t.run(failing)
  t.expect('conflicts, unmatched scopes, bad verdicts and ids without a verdict fail', c, 12, fails=[
    'R-UNIT-001, R-UNIT-002: 2 rows of the same specificity',
    'R-UNIT-003: 2 rows of the same specificity',
    '"R-UNIT-003": a drop row gives no reason',
    'unknown verdict "keep"',
    '"R-UNIT-004": an adapt row names no Species requirement',
    'adapt names WLD-009, which is not defined',
    'there is no reference/02_units/missing.md',
    'reference/02_units/worker.md has no heading 7',
    '"02_units/worker.md §3" matches no R- id',
    '"02_units/README.md" matches no R- id',
    '"R-UNIT-099" matches no R- id',
    '"all of the worker" is not a reference file',
    'reference/05_population_flow.md §2: 1 R- ids have no verdict: R-ECON-002',
    'notes_ledger.md: no ledger table',
  ])

  # -------------------------------------------------- gates 14 to 18 on a passing Species set
  c = t.run(GOOD_SPECIES)
  t.expect('GDD headings, status line and Scale path present; baseline component not checked', c, 14,
           info='1 GDD files, 1 new component files checked')
  t.expect('rationale files exist; Critical and High findings resolved', c, 15,
           info='2 GDD and new component files name a rationale; 2 rationale files, 2 Critical or High findings')
  t.expect('final and provisional rows; reference and baseline tables not checked', c, 17,
           info='2 tuning rows in Species documents: 1 final, 1 provisional')
  t.expect('a model-required rationale names an existing model', c, 18,
           notes=[f'area {a} has no rationale' for a in cd.MODEL_AREAS if a != 'A1'],
           info='1 of 6 model-required areas')

  final_missing = dict(GOOD_SPECIES)
  final_missing['Design/gdd/01_world_and_regions.md'] = gdd(
    'World and regions', status='> Status: Final · Prefix: WLD · Rationale: ../rationale/A9.md')
  c = t.run(final_missing)
  t.expect('a Final document must have the rationale it names', c, 15,
           fails=['gdd/01_world_and_regions.md: the rationale it names, rationale/A9.md, does not exist yet'])

  # -------------------------------------------------- gates 14, 15 and 17: failures
  bad = {
    'Design/gdd/02_settlements_and_population.md': gdd('Settlements', status='> Status: Draft · Prefix: SET',
                                                       rules='**SET-001** (MUST) A settlement has inhabitants.',
                                                       tuning='No tuning keys.',
                                                       omit=('7 Adoption from the reference',)),
    'Design/gdd/03_resources_and_economy.md': gdd('Resources', status=None, rules='', tuning='No tuning keys.'),
    'Design/gdd/04_building_and_construction.md': gdd('Building', status='> Status: Draft · Rationale: '
                                                      '[B3](../rationale/B3.md)', rules='', tuning='''\
| Key | Value | Unit | Status | Requirement | DEC | Playtest note |
|---|---|---|---|---|---|---|
| `con.queue.length` | 5 | orders | provisional | | | |
| `con.queue.slots` | 2 | slots | draft | | | |
| `con.queue.delay` | 3 | s | | | | |

| Key | Value | Unit | Requirement |
|---|---|---|---|
| `con.build.rate` | 1 | per s | |'''),
    'Design/rationale/B3.md': '''\
# B3 rationale

## Findings and resolutions

| Id | Severity | Category | Finding | Resolution |
|---|---|---|---|---|
| B3-F01 | High | economy and balance | Queues stall. | — |
| B3-F02 | Critical | griefing or exploit | Walls block sites. | |
| B3-F03 | Low | player experience | The queue icon is small. | |
''',
    'Design/architecture/components/replication.md': component('Replication', '../../rationale/D3.md',
                                                               scale_path=False, extra='''
## 10 Tuning table

| Key | Value | Unit | Status | Requirement | DEC | Playtest note |
|---|---|---|---|---|---|---|
| `rep.snapshot.rate` | 10 | per s | provisional | | | — |
'''),
    'Design/architecture/components/persistence/README.md': '# Persistence\n',
    'Design/architecture/components/persistence/storage.md': component('Storage', '../../../CONVENTIONS.md',
                                                                       scale_path=False),
  }
  c = t.run(bad)
  t.expect('missing GDD heading, status line, Rationale and Scale path fail', c, 14, fails=[
    '02_settlements_and_population.md: GDD template heading "## 7 Adoption from the reference" missing',
    '02_settlements_and_population.md: the status line names no "Rationale:"',
    '03_resources_and_economy.md: no "> Status:" line',
    'replication.md: new component without "## 9 Scale path"',
    'persistence/storage.md: new component without "## 9 Scale path"',
  ])
  t.expect('missing or misplaced rationale and unresolved findings fail; a Draft may name one not yet written', c, 15, fails=[
    'gdd/02_settlements_and_population.md: names no rationale file',
    'gdd/03_resources_and_economy.md: names no rationale file',
    'persistence/storage.md: the rationale it names is not a file under rationale/',
    'rationale/B3.md:7 B3-F01 (high) has no resolution',
    'rationale/B3.md:8 B3-F02 (critical) has no resolution',
  ], notes=['replication.md: the rationale it names, rationale/D3.md, does not exist yet'])
  t.expect('bad status cells, missing playtest notes and a table without status fail', c, 17, fails=[
    'con.queue.length: a provisional value without a playtest note',
    'con.queue.slots: status "draft" is neither final nor provisional',
    'con.queue.delay: status "" is neither final nor provisional',
    'tuning table without a status column',
    'rep.snapshot.rate: a provisional value without a playtest note',
  ])

  # -------------------------------------------------- gate 16
  cites = {'Design/notes.md': '''\
# Notes

O1, O2 and O31 apply, and so does the range O1–O2. A sort costs O(n log n) and a lookup O(1).
`O7` is code, PO8 and O9x are not citations.

```
O10 is inside a code block.
```
'''}
  c = t.run(cites)
  t.expect('cited owner decisions with entries (table rows and headings) pass; code and big-O forms are not cited',
           c, 16, info='3 owner decisions in decisions/owner_decisions.md; 8 citations of 3 of them, 0 with no entry')
  c = t.run({'Design/notes.md': '# Notes\n\nO3 is not an entry, and the range O30–O31 includes O30.\n'})
  t.expect('an owner decision with no entry fails, a range end included', c, 16,
           fails=['O3 is cited at notes.md:3', 'O30 is cited at notes.md:3 and 1 more places'])
  c = t.run({'Design/notes.md': '# Notes\n\nO1 applies.\n', 'Design/decisions/owner_decisions.md': None})
  t.expect('citations without owner_decisions.md fail', c, 16, fails=['decisions/owner_decisions.md does not exist'])

  # -------------------------------------------------- gate 18: failures
  models = dict(GOOD_SPECIES)
  models.update({
    'Design/rationale/A2.md': '# A2 rationale\n\n## 5 Model results\n\n`Tools/DesignModels/Missing.py` shows it.\n',
    'Design/rationale/B2_economy.md': '# B2 rationale\n\n## Weakest point\n\nSnowballing.\n',
    'Design/rationale/B4.md': '# B4 rationale\n\n## Model results\n\nThe battle model shows 400 bodies.\n\n'
                              '## Owner decisions taken\n\n`Tools/DesignModels/WorldDensity.py` is not here.\n',
    'Design/rationale/C1.md': '# C1 rationale\n\n## Weakest point\n\nNo model is required here.\n',
  })
  c = t.run(models)
  t.expect('a missing script, a missing section and a section naming no script fail', c, 18, fails=[
    'A2.md: "## Model results" names Tools/DesignModels/Missing.py, which does not exist',
    'B2_economy.md: model-required area B2 has no "## Model results" section',
    'B4.md: "## Model results" names no script',
  ], notes=['area B5 has no rationale', 'area B6 has no rationale'])

  # -------------------------------------------------- gate 19
  allowed = 'build/Frobnicate.tmp'
  scan = {
    '.gitignore': f'# build output\n{allowed}\n',
    'crlf.txt': 'first line\r\nFrobnicate with a CRLF ending\r\n',
    'Design/notes.md': '# Notes\n\nFrobnicate in the design.\n',
    '.git/config': 'Frobnicate the zorbulon\n',
    'node_modules/a.txt': 'Frobnicate\n',
    'sub/node_modules/b.txt': 'Frobnicate\n',
    'Tools/DesignCheck/package-lock.json': '{"integrity": "Frobnicate zorbulon"}\n',
    'blob.bin': b'\0\0\0Frobnicate\0',
  }
  scan.update(scan_files([
    ('.gitignore', cd.line_digest(allowed), 'a stock line'),
    ('crlf.txt', cd.line_digest('Frobnicate with a CRLF ending'), 'a line with a CRLF ending'),
    ('Design/notes.md', cd.line_digest('Frobnicate in the design.'), 'a line of the design root'),
  ]))
  c = t.run(scan)
  t.expect('allowed lines (CRLF, design root) and excluded paths pass', c, 19,
           info='3 hard-fail hits (1 in Design/, from gate 1), 3 allowed by ScanAllow.tsv, 0 not allowed')
  scan = {
    '.gitignore': f'{allowed}\nFrobnicate again\n',
    'other.txt': f'{allowed}\n',
    'notes/readme.txt': 'The Zorbulon device.\n',
    'Design/notes.md': '# Notes\n\nFrobnicate in the design.\n',
  }
  scan.update(scan_files([
    ('.gitignore', cd.line_digest(allowed), 'a stock line'),
    ('.gitignore', cd.line_digest('a line that is gone'), 'a stale row'),
    ('missing.txt', cd.line_digest(allowed), 'a row for a missing file'),
    ('other.txt', 'abc', 'not a digest'),
    ('other.txt', cd.line_digest(allowed), ''),
  ]))
  c = t.run(scan)
  t.expect('hits not allowed for their path, stale rows and bad rows fail', c, 19, fails=[
    '.gitignore:2 [test-pattern] Frobnicate',
    'other.txt:1 [test-pattern] Frobnicate',
    'notes/readme.txt:1 [ip-denylist] zorbulon',
    'Design/notes.md:3 [test-pattern] Frobnicate',
    'ScanAllow.tsv:3 .gitignore: allows no hit',
    'ScanAllow.tsv:4 missing.txt: allows no hit',
    'ScanAllow.tsv:5 sha256 must be 64 lower-case hex digits',
    'ScanAllow.tsv:6 other.txt: the reason is empty',
  ])

  # -------------------------------------------------- the real allow-list holds digests only
  allow, problems = cd.load_scan_allow(os.path.join(HERE, 'ScanAllow.tsv'))
  t.record('Tools/DesignCheck/ScanAllow.tsv parses and holds digests only',
           problems + ([] if allow else ['ScanAllow.tsv holds no row']))


if __name__ == '__main__':
  sys.exit(main())
