#!/usr/bin/env python3
"""Render design/README.md from _work/script_inputs/README_template.md, filling {{DOCMAP}} (title, part, size, contents
summary per document) and {{STATUS}} (coverage result and evidence-tag counts from _work/coverage_report.json).
Usage: python3 -I finalize_readme.py <DESIGN_DIR>"""
import glob, json, os, re, sys
design = sys.argv[1]; work = os.path.join(design, '_work')
tpl = open(os.path.join(work, 'script_inputs', 'README_template.md'), encoding='utf-8').read()
PART = lambda n: 'A — Functional' if n < 20 else ('B — Technical' if n < 40 else 'C — Basis for the new game')
rows = ['| # | Document | Part | Size | Scope (from §5) |', '|---|---|---|---|---|']
SCOPE = {
 0: 'Concept, core loop, distinctive mechanics, player glossary', 1: 'Every mode: objectives, win/lose/draw, scoring, timers, teams, options',
 2: 'Every entity type: stats, state machines, targeting, control, death', 3: 'Every building/world object: function, capture, I/O, level parameters',
 4: 'Crate outcomes, probabilities, exclusions, weapons and projectiles', 5: 'Spawn-point flow, population cap, spirits, reinforcements, throughput',
 6: 'Every action and binding, selection, task manager, officers, gestures, camera, input graph', 7: 'CPU player architecture, decision loop, difficulty, per-mode strategy',
 8: 'Shipped maps, level-design capabilities, per-level options, map editor', 9: 'Screen flow, lobby, options, gating, achievements, help, loading, localisation',
 10: 'Sound events and binding, music, positional rules, voices, DSP, mix', 20: 'Layers, modules, singletons, lifetimes, threads, main loop and timing',
 21: 'Class hierarchy (all 159 classes), identity, containers, update order, spatial grids, routing', 22: 'Topology, messages, lockstep, sync/desync, lobby protocol, RNG and float determinism',
 23: 'Landscape pipeline precise enough to reproduce terrain from a level file', 24: 'GL and D3D9-emulation backends, ordered pass table with GL state, every renderer, shaders',
 25: 'Per-file catalogue of every texture, sprite, shape, icon, thumbnail, help image, shader, sound; loading pipeline', 26: '.anim playback, procedural animation, particles, explosions, gesture rendering',
 27: 'Sound engine architecture, back-ends, voices, cache, decoder, DSP, tooling', 28: 'Eclipse/Darwinia window framework, widgets, focus, navigation, building a screen',
 29: 'Formal grammar or byte layout for every text, binary, runtime and network format', 30: 'Script language, tutorials, camera animation, start/end sequences, Sepulveda',
 31: 'Platform layer, window management, build configs, data pipeline, installers, every tool', 32: 'Debug menus, cheats, testbed, benchmark, profiler, logging, hotspots',
 40: 'Keep/adapt/replace/drop per subsystem; legacy tech → Windows/C++23/D3D12/multiplayer', 41: 'Every [U], design-critical [I], docs-vs-code discrepancy and IP item',
 90: 'Glossary and canonical neutral placeholder names', 91: 'Concept/class/format → documents → source files', 99: 'Every manifest entry → citing documents, read depth, or justification'}
for p in sorted(glob.glob(os.path.join(design, '[0-9][0-9]_*.md'))):
    name = os.path.basename(p); n = int(name[:2])
    title = open(p, encoding='utf-8').readline().strip('# \n')
    kb = os.path.getsize(p) / 1024
    rows.append(f'| {n:02d} | [{title}]({name}) | {PART(n)} | {kb:,.0f} KB | {SCOPE.get(n, "")} |')
docmap = '\n'.join(rows)
st = ''
jp = os.path.join(work, 'coverage_report.json')
if os.path.exists(jp):
    d = json.load(open(jp))
    tags = d.get('tags', {})
    st = ('## Status\n\n'
          f'- **Documents:** {len(rows) - 2} numbered documents plus this README.\n'
          f'- **Coverage (Phase 4, `_work/scripts/coverage_check.py`):** (a) uncited live source files **{len(d["a"])}**, (b) uncited data files **{len(d["b"])}**, '
          f'(c) formats without grammar **{len(d["c"])}**, (d) world-object classes without spec/diagram **{len(d["d"])}**, (e) assets without catalogue entry **{len(d["e"])}**; '
          f'cited paths not in the tree: **{len(d["bad_citations"])}**. Remaining justified items are listed in [99](99_coverage_matrix.md).\n'
          f'- **Evidence tags across documents 00–41:** [C] {tags.get("[C]", 0):,} · [I] {tags.get("[I]", 0):,} · [U] {tags.get("[U]", 0):,} (tag occurrences, not unique claims; the de-duplicated register is in [41](41_risks_and_open_questions.md)).\n')
out = tpl.replace('{{DOCMAP}}', docmap).replace('{{STATUS}}', st)
open(os.path.join(design, 'README.md'), 'w', encoding='utf-8').write(out)
print('README written', len(out))
