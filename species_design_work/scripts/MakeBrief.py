"""Write the proposer and challenger briefs of a design area from the design-phase prompt.

Usage: python3 -I MakeBrief.py <REPO> <AREA> [<AREA> ...]

For each area, reads its row of prompts/species-design-phase.md §5.2, the ledger files it owns (§5.3), its rows
of species_design_work/dec_triage.md, the GDD or component template (§6), the challenge step (§3) and the common
rules (§4), and writes species_design_work/areas/<AREA>/brief.md and challenge_brief.md. Upstream areas are
listed with their documents; the proposer reads the Final ones.
"""
import os
import re
import sys

REPO = os.path.abspath(sys.argv[1])
P = open(os.path.join(REPO, 'prompts', 'species-design-phase.md'), encoding='utf-8').read()
TRIAGE = open(os.path.join(REPO, 'species_design_work', 'dec_triage.md'), encoding='utf-8').read()
CELL = re.compile(r'(?<!\\)\|')


def cells(line):
    return [c.strip() for c in CELL.split(line.strip())][1:-1]


def between(a, b):
    return P[P.index(a):P.index(b)].strip().rstrip('-').strip()


AREAS = {}
for line in between('### 5.2 Areas', '### 5.3 Adoption ledger').split('\n'):
    c = cells(line) if line.startswith('| ') else []
    if len(c) == 6 and re.match(r'^[A-E]\d+$', c[0]):
        AREAS[c[0]] = dict(wave=c[1], output=c[2], scope=c[3], depends=c[4], model=c[5])

LEDGER = {}
for line in between('**Ownership.**', 'The engine component requirements').split('\n'):
    c = cells(line) if line.startswith('| ') else []
    if len(c) == 2 and c[0].startswith('`'):
        owner = re.match(r'\s*([A-E]\d+)', c[1]).group(1)
        part = re.search(r'\(([A-E]\d+) for ([^)]*)\)', c[1])
        for doc in re.findall(r'`([^`]+)`', c[0]):
            d = doc.rstrip('/').removesuffix('.md')
            LEDGER.setdefault(owner, []).append((d, f'owner; {part.group(1)} sends the rows for {part.group(2)}' if part else 'owner'))
            if part:
                LEDGER.setdefault(part.group(1), []).append((d, f'{part.group(2)} only; {owner} owns the file'))

ARC42 = {'01': '01_introduction_and_goals.md', '02': '02_architecture_constraints.md', '03': '03_context_and_scope.md',
         '04': '04_solution_strategy.md', '05': '05_building_block_view.md', '06': '06_runtime_view.md',
         '07': '07_deployment_view.md', '08': '08_crosscutting_concepts.md', '09': '09_architecture_decisions.md',
         '10': '10_quality_requirements.md', '11': '11_risks_and_technical_debt.md', '12': '12_glossary.md'}
RULES = between('## 4 Common rules for every agent (non-negotiable)', '## 5 Design areas, outputs and waves')
GDD_T = between('**GDD template**', '**Component template.**')
COMP_T = between('**Component template.**', '**ADRs** follow MADR 4')
STEP2 = between('**Step 2 — Challenge.**', '**Step 3 — Revise.**')
SEEDS = between('### 5.6 Seed challenges', '## 6 Writing standard')
PREFIX = {}
for line in open(os.path.join(REPO, 'Design', 'CONVENTIONS.md'), encoding='utf-8'):
    c = cells(line) if line.startswith('| `') else []
    if len(c) == 3:
        PREFIX[c[2].strip('`')] = c[0].strip('`')

EXTRA = {
    'A2': ('`Design/architecture/04_solution_strategy.md` is arc42 chapter 4, with these sections: '
           '4.1 Technology decisions (one line each, citing its ADR); 4.2 Top-level decomposition (the processes, the '
           'server and client libraries and the transport between them, integrated and dedicated, O10); 4.3 Approaches '
           'to the quality goals (scale, cost, cheating, persistence, operability); 4.4 Scale path (what is true at 16 '
           'players so that 200 or more needs no rewrite, O1); 4.5 Model results (the server CPU, bandwidth and storage '
           'budgets at 16 and at 200 players, from your model). Status line as in the GDD template, without a prefix. '
           'Each topic deferred to the design phase (`Design/ADR/README.md` §"Deferred to the design phase") that this '
           'area decides gets an ADR in MADR 4 form, `Design/ADR/ADR-NNN-<slug>.md` numbered from ADR-079 '
           '(`AGENTS.md` §6), status Proposed until the owner gate, which resolves or re-dates its deferred DEC entries. '
           'Add each new ADR to the index of `Design/ADR/README.md`.'),
}


for area in sys.argv[2:]:
    a = AREAS[area]
    outs = re.findall(r'`([^`]+)`', a['output'])
    files = []
    for o in outs:
        m = re.fullmatch(r'gdd/(\d\d)', o)
        a42 = re.fullmatch(r'architecture/(\d\d)', o)
        if m:
            files += [f'Design/{k}' for k in PREFIX if k.startswith(f'gdd/{m.group(1)}_')]
        elif a42:
            files.append(f'Design/architecture/{ARC42[a42.group(1)]}')
        elif o.startswith('components/'):
            files.append(f'Design/architecture/{o}')
        else:
            files.append(f'Design/{o}')
    prefix = next((PREFIX[f[len('Design/'):]] for f in files if f[len('Design/'):] in PREFIX), None)
    dec_rows = [l for l in TRIAGE.split('\n') if l.startswith('| DEC-') and cells(l)[3] == area]
    ledger = LEDGER.get(area, [])
    is_comp = any('/components/' in f for f in files)
    is_gdd = any('/gdd/' in f for f in files)
    out = [f'# {area} brief', '',
           f'Area **{area}** of the Species design phase (wave {a["wave"]}). Read `prompts/species-design-phase.md` §1–§6 '
           'in full first: this brief only adds what is specific to the area.', '',
           '## Scope (prompt §5.2)', '', a['scope'], '',
           f'**Depends on:** {a["depends"]} (read the Final documents of those areas and their rationale files). '
           f'**Model required:** {a["model"]}.', '',
           '## What you write', '']
    out.append(f'1. {", ".join("`" + f + "`" for f in files)}' + (f', prefix `{prefix}`' if prefix else '') +
               ', status `Draft`, in the ' + ('component template (with §9 Scale path)' if is_comp else 'GDD template') +
               ' below. Write the rationale path in the status line as text in backticks '
               f'(`../rationale/{area}.md` from `gdd/`, `../../rationale/{area}.md` from `architecture/components/`): '
               'the rationale is written at Finalise.')
    n = 2
    if ledger:
        out.append(f'{n}. The verdicts of the ledger files you own (prompt §5.3): ' +
                   '; '.join(f'`Design/gdd/adoption_ledger/{d}_ledger.md` ({x})' for d, x in ledger) +
                   '. Fill every `pending` row of your scope; add exception rows where one requirement differs from its section. '
                   'Where another area owns the file, write your rows directly while that area is not running (the coordinator '
                   'keeps one writer per file at a time); if it is running, send them to it as a cross-area request.')
        n += 1
    if dec_rows:
        out.append(f'{n}. A proposed resolution for each Open decision the triage gives you (prompt §5.4): ' +
                   ', '.join(cells(l)[0] for l in dec_rows) + '. Propose in your notes; the register is edited at Finalise.')
        n += 1
    if a['model'] not in ('—', ''):
        out.append(f'{n}. The quantitative model: {a["model"]}. A Python script `Tools/DesignModels/<Area><Model>.py` '
                   '(PascalCase, documented inputs and outputs, `python3 -I`, standard library only), with its README entry; '
                   'its results go into the document and, at Finalise, into the rationale\'s "Model results".')
        n += 1
    out.append(f'{n}. `species_design_work/areas/{area}/proposal_notes.md` (prompt §3 step 1) and '
               f'`species_design_work/reports/{area}-P.md` (common rule 11).')
    out += ['', 'Before you finish, run `python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase`, '
            'and once without `--skip-mermaid` if you wrote diagrams. Every gate must pass; pending ledger rows and undecided '
            'decisions of other areas are expected.', '']
    if dec_rows:
        out += ['## Your decisions (from `species_design_work/dec_triage.md`)', '',
                '| DEC | Title | Decide by | Area | Also affects | Assigned by | Register file |', '|---|---|---|---|---|---|---|']
        out += dec_rows + ['']
    if not is_comp and not is_gdd:
        template = EXTRA.get(area, 'Follow the arc42 template for this chapter; give every statement a requirement id of the '
                                   'registered prefix where one exists, and cite the ADRs it rests on.')
    else:
        template = COMP_T if is_comp else GDD_T
    out += ['## Template', '', template, '', RULES, '']
    os.makedirs(os.path.join(REPO, 'species_design_work', 'areas', area), exist_ok=True)
    open(os.path.join(REPO, 'species_design_work', 'areas', area, 'brief.md'), 'w', encoding='utf-8').write('\n'.join(out))
    ch = [f'# {area} challenge brief', '',
          f'You are **{area}-C**, the challenger of area {area}. You have a fresh context on purpose: attack the area\'s '
          'documents, weakest point first, then say how to make them better.', '',
          f'**Read:** {", ".join("`" + f + "`" for f in files)}' + (' and the ledger rows of the area' if ledger else '') +
          '; `vision/species-vision-brief.md`; `Design/decisions/owner_decisions.md`; `prompts/species-design-phase.md` '
          '§1–§6; the Final documents and rationale files of the areas it depends on '
          f'({a["depends"]}); the reference and component documents it adopts or adapts. **Do not read** '
          f'`species_design_work/areas/{area}/proposal_notes.md` or `species_design_work/reports/{area}-P.md`.', '',
          f'**Write:** `species_design_work/areas/{area}/challenge_<n>.md` (n = the round), and, for a quick check or the '
          'quantitative model, scripts under `species_design_work/scripts/` or `Tools/DesignModels/`. You never edit the '
          'area documents.', '', STEP2, '',
          f'**Model required:** {a["model"]}.', '', SEEDS, '', RULES, '',
          'Your final reply is at most 200 words: the weakest point in one sentence, the finding counts by severity, and '
          'the number of "make it better" items.', '']
    open(os.path.join(REPO, 'species_design_work', 'areas', area, 'challenge_brief.md'), 'w', encoding='utf-8').write('\n'.join(ch))
    print(f'{area}: {files} prefix {prefix}; ledger {ledger}; {len(dec_rows)} DECs; model {a["model"][:40]}')
