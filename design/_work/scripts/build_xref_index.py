#!/usr/bin/env python3
"""Generate 91_cross_reference_index.md: concept -> documents (sections) -> source files.

Concepts = curated game/engine concepts (regex), every code/worldobject class (from _work/class_assignment.csv), every data
format (from _work/formats_registry.csv). For each concept: the sections whose heading names it (primary), the other
documents that mention it (secondary, with mention counts), and the source/data files cited inside the primary sections
(ranked by citation frequency).
Usage: python3 -I build_xref_index.py <DESIGN_DIR>"""
import collections, csv, glob, importlib.util, os, re, sys

design = sys.argv[1]
work = os.path.join(design, '_work')
here = os.path.dirname(os.path.abspath(__file__))

EXCLUDE = {'91_cross_reference_index.md', '99_coverage_matrix.md', 'README.md'}
docs = {os.path.basename(p): open(p, encoding='utf-8', errors='replace').read()
        for p in sorted(glob.glob(os.path.join(design, '[0-9][0-9]_*.md'))) if os.path.basename(p) not in EXCLUDE}

PATH_RE = re.compile(r'(?<![\w/])((?:code|data|contrib|tools|targets|docs)/[\w\-./+]+\.[A-Za-z0-9]+)')
manifest = {r['path'].lower(): r['path'] for r in csv.DictReader(open(os.path.join(work, 'manifest.csv')))}

def slug(h):
    s = h.strip().lower()
    s = re.sub(r'[^\w\- ]', '', s)
    return s.replace(' ', '-')

# split into sections (## / ### / ####)
sections = []  # (doc, level, heading, anchor, body)
for name, text in docs.items():
    parts = re.split(r'(?m)^(?=#{2,4} )', text)
    for part in parts:
        m = re.match(r'(#{2,4}) (.*)', part)
        if not m: continue
        head = m.group(2).strip()
        sections.append((name, len(m.group(1)), head, slug(head), part))

def cited_files(body):
    c = collections.Counter()
    for p in PATH_RE.findall(body):
        low = p.lower().rstrip('.,;)')
        if low in manifest: c[manifest[low]] += 1
    return c

CONCEPTS = [
 # (group, concept, regex)
 ('Game concepts', 'Game modes (all)', r'game ?modes?|game ?types?'),
 ('Game concepts', 'Domination', r'domination|skirmish'),
 ('Game concepts', 'King of the Hill', r'king of the hill|\bkoth\b'),
 ('Game concepts', 'Capture the Statue', r'capture the statue|\bcts\b'),
 ('Game concepts', 'Assault', r'\bassault\b'),
 ('Game concepts', 'Rocket Riot', r'rocket ?riot'),
 ('Game concepts', 'Blitzkrieg', r'blitzkr(ie|ei)g'),
 ('Game concepts', 'Tank Battle', r'tank ?battle'),
 ('Game concepts', 'Retribution', r'retribution'),
 ('Game concepts', 'Handicap', r'handicap'),
 ('Game concepts', 'Grace period / ready / countdown', r'grace|countdown'),
 ('Game concepts', 'Scoring and victory', r'scor(e|ing)|victory|win(ning)? condition|end of game|game over'),
 ('Game concepts', 'Population cap', r'population cap|pop(ulation)? ?cap'),
 ('Game concepts', 'Spawn-point economy', r'spawn[- ]?point|spawn economy|spawn link'),
 ('Game concepts', 'Spirits and souls', r'spirit|soul'),
 ('Game concepts', 'Reinforcements (trunk ports)', r'reinforcement|trunk ?port'),
 ('Game concepts', 'Crates and power-ups', r'crate|power-?up|reward'),
 ('Game concepts', 'Officers, orders and formations', r'officer|formation|orders?\b'),
 ('Game concepts', 'Task manager and slots', r'task ?manager|task types?|\btasks?\b'),
 ('Game concepts', 'Selection model', r'selection'),
 ('Game concepts', 'Gestures', r'gesture'),
 ('Game concepts', 'Camera', r'camera'),
 ('Game concepts', 'CPU AI', r'\bai\b|cpu player|aitarget|aiobjective'),
 ('Game concepts', 'Difficulty', r'difficulty'),
 ('Game concepts', 'Tutorials', r'tutorial'),
 ('Game concepts', 'Help system', r'help'),
 ('Game concepts', 'Achievements', r'achievement'),
 ('Game concepts', 'Lobby and game setup', r'lobby|game setup|host|join'),
 ('Game concepts', 'Front end / screen flow', r'front[- ]end|screen flow|main menu|gamemenuwindow|page'),
 ('Game concepts', 'Options and preferences', r'option|preference|prefs'),
 ('Game concepts', 'Demo / full-game gating and authentication', r'demo|auth|key'),
 ('Game concepts', 'Levels and maps', r'level|maps?\b'),
 ('Game concepts', 'Map editor', r'editor'),
 ('Game concepts', 'Loading screens and loaders', r'loading|loader'),
 ('Game concepts', 'Start / end sequences, attract mode, credits', r'start ?sequence|end ?sequence|attract|credits'),
 ('Game concepts', 'Localisation', r'locali[sz]ation|language|phrase'),
 ('Engine subsystems', 'App, globals and lifetime', r'\bapp\b|global|singleton|lifetime|startup|shutdown'),
 ('Engine subsystems', 'Main loop and frame timing', r'main loop|frame|tick|timestep|gametimer|game timer|slice'),
 ('Engine subsystems', 'Threading', r'thread'),
 ('Engine subsystems', 'Location and world containers', r'location|container'),
 ('Engine subsystems', 'Entity model and object identity', r'entity|worldobject|object id|handle'),
 ('Engine subsystems', 'Spatial partitioning', r'entity grid|grid cache|obstruction|spatial'),
 ('Engine subsystems', 'Routing and pathfinding', r'rout(e|ing)|path'),
 ('Engine subsystems', 'Collision and physics', r'collision|physics|obstacle'),
 ('Engine subsystems', 'Teams and ownership', r'\bteams?\b|ownership|colour'),
 ('Engine subsystems', 'Determinism and RNG', r'determinism|rng|random|syncrand|float'),
 ('Engine subsystems', 'Networking: messages and protocol', r'network|message|letter|iframe|protocol|command'),
 ('Engine subsystems', 'Sync checking and desync', r'sync|desync'),
 ('Engine subsystems', 'Metaserver and matchmaking', r'metaserver|matchmak|nat|server browser|discovery'),
 ('Engine subsystems', 'Terrain generation', r'landscape|terrain|tile|flatten|height'),
 ('Engine subsystems', 'Water', r'water|wave|shore'),
 ('Engine subsystems', 'Rendering backend (GL / D3D9 emulation)', r'backend|opengl|direct3d|d3d|emulation'),
 ('Engine subsystems', 'Render passes and GL state', r'render pass|pass table|frame composition|gl state'),
 ('Engine subsystems', 'Shaders', r'shader'),
 ('Engine subsystems', 'Shapes (.shp meshes)', r'shape|\.shp|fragment|marker'),
 ('Engine subsystems', 'Textures and bitmaps', r'texture|bitmap|bmp'),
 ('Engine subsystems', 'Resource manager and archives', r'resource|archive|\.dat'),
 ('Engine subsystems', 'Particles and VFX', r'particle|explosion|vfx|effect'),
 ('Engine subsystems', 'Animation', r'animat|\.anim|leg'),
 ('Engine subsystems', 'Text and fonts', r'text render|font'),
 ('Engine subsystems', 'Audio engine', r'sound system|audio engine|voice|channel|sample|dsp|mixer|soundinstance'),
 ('Engine subsystems', 'Audio design and events', r'sound event|sounds\.txt|music|audio design'),
 ('Engine subsystems', 'Input drivers and bindings', r'input|binding|driver|xinput|control type'),
 ('Engine subsystems', 'UI framework (Eclipse / Darwinia windows)', r'eclipse|window|button|widget|ui framework'),
 ('Engine subsystems', 'Scripting', r'script|opcode'),
 ('Engine subsystems', 'Platform, build and tools', r'platform|build|tool|installer|vcproj'),
 ('Engine subsystems', 'Crash handling and logging', r'crash|log|black box|assert'),
 ('Engine subsystems', 'Debug, cheats, profiler, benchmark', r'debug|cheat|profil|benchmark|testbed|harness'),
 ('Part C', 'Carry-forward verdicts', r'carry|verdict|keep|replace|adapt'),
 ('Part C', 'Risks and open questions', r'risk|open question|unknown'),
 ('Part C', 'IP items', r'\bip\b|intellectual property'),
]

classes = list(csv.DictReader(open(os.path.join(work, 'class_assignment.csv'))))
formats = list(csv.DictReader(open(os.path.join(work, 'formats_registry.csv'))))

def concept_rows(concept, rx, head_only_rx=None):
    hr = re.compile(head_only_rx or rx, re.I)
    br = re.compile(rx, re.I)
    prim = [(d, h, a) for d, lvl, h, a, b in sections if hr.search(h)]
    files = collections.Counter()
    for d, lvl, h, a, b in sections:
        if hr.search(h): files.update(cited_files(b))
    mentions = collections.Counter()
    for name, text in docs.items():
        n = len(br.findall(text))
        if n: mentions[name] += n
    return prim, mentions, files

def fmt_prim(prim, limit=8):
    seen, out = set(), []
    for d, h, a in prim:
        key = (d, a)
        if key in seen: continue
        seen.add(key)
        out.append(f'[{d[:2]} {h[:60]}]({d}#{a})')
        if len(out) >= limit: break
    more = len({(d, a) for d, h, a in prim}) - len(out)
    return '<br>'.join(out) + (f'<br>… +{more} more' if more > 0 else '') if out else '—'

def fmt_mentions(m, prim_docs, limit=10):
    items = [(d, n) for d, n in m.most_common() if d not in prim_docs][:limit]
    return ', '.join(f'[{d[:2]}]({d}) ×{n}' for d, n in items) or '—'

def fmt_files(f, limit=8):
    return '<br>'.join(f'`{p}`' for p, _ in f.most_common(limit)) or '—'

out = ['# 91 — Cross-reference index\n',
       '> Part C · Generated by `_work/scripts/build_xref_index.py` from documents 00–41 — do not edit by hand. '
       'For each concept: the sections whose **heading** names it (primary), the other documents that **mention** it '
       '(with mention counts), and the source/data files most cited inside the primary sections. Paths are relative to the '
       '`multiwinia/` source root. Evidence and build tags are explained in [README](README.md) and [90](90_glossary.md).\n',
       '## Contents\n', '- [1 Game concepts](#1-game-concepts)', '- [2 Engine subsystems](#2-engine-subsystems)',
       '- [3 Part C](#3-part-c)', '- [4 World-object classes](#4-world-object-classes)', '- [5 Data formats](#5-data-formats)\n']
groups = collections.OrderedDict()
for g, c, rx in CONCEPTS: groups.setdefault(g, []).append((c, rx))
for i, (g, items) in enumerate(groups.items(), 1):
    out.append(f'## {i} {g}\n\n| Concept | Primary sections | Also mentioned in | Key source files |\n|---|---|---|---|')
    for c, rx in items:
        prim, m, f = concept_rows(c, rx)
        out.append(f'| **{c}** | {fmt_prim(prim)} | {fmt_mentions(m, {d for d, _, _ in prim})} | {fmt_files(f)} |')
    out.append('')
out.append('## 4 World-object classes\n\nEvery class declared in `code/worldobject/*.h`, the document that carries its behaviour specification, '
           'and the sections that name it. Class diagrams: [21](21_simulation_and_entity_model.md).\n\n'
           '| Class | Base | Header | Spec doc | Sections naming it | Mentioned in |\n|---|---|---|---|---|---|')
docname = {d[:2]: d for d in docs}
for r in sorted(classes, key=lambda r: r['class'].lower()):
    rx = r'\b' + re.escape(r['class']) + r'\b'
    prim, m, f = concept_rows(r['class'], rx)
    spec = docname.get(r['doc'], '')
    out.append(f"| `{r['class']}` | {r['base'] or '—'} | `{r['header']}` | [{r['doc']}]({spec}) | {fmt_prim(prim, 4)} | "
               f"{fmt_mentions(m, {d for d, _, _ in prim}, 6)} |")
out.append('\n## 5 Data formats\n\nEvery format in `_work/formats_registry.csv`, its grammar anchor in [29](29_data_formats_reference.md), '
           'and where else it is discussed.\n\n| Format | Grammar | Files (pattern) | Mentioned in |\n|---|---|---|---|')
for r in formats:
    name = r['name']
    words = [w for w in re.findall(r'[A-Za-z][A-Za-z.\-]{3,}', name)][:2]
    rx = '|'.join(re.escape(w) for w in words) if words else re.escape(r['format_id'])
    _, m, _ = concept_rows(name, rx)
    pats = ', '.join(f'`{p}`' for p in r['patterns'].split(';') if p) or 'runtime / network'
    out.append(f"| {name} | [fmt-{r['format_id']}](29_data_formats_reference.md#fmt-{r['format_id']}) | {pats} | {fmt_mentions(m, set(), 6)} |")
open(os.path.join(design, '91_cross_reference_index.md'), 'w', encoding='utf-8').write('\n'.join(out) + '\n')
print('concepts', len(CONCEPTS), 'classes', len(classes), 'formats', len(formats), 'sections scanned', len(sections))
