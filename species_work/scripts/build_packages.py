"""Build the Phase 2 and Phase 3 work packages from routing.csv (gate G1).

Usage: python3 -I build_packages.py <WORK dir>

Writes WORK/packages/<id>.json and WORK/packages/INDEX.md. Every split decided
at G1 is encoded here, so the packages can be rebuilt after routing changes.
"""
import csv
import json
import os
import re
import sys
from collections import OrderedDict

BUDGET_KB = 150
WORK = sys.argv[1]
ROWS = list(csv.DictReader(open(os.path.join(WORK, 'routing.csv'), encoding='utf-8')))
for r in ROWS:
    r['s'], r['e'], r['b'] = int(r['start_line']), int(r['end_line']), int(r['bytes'])
    r['doc'] = r['orig_file'][:2]

CONTENT = ('KEEP', 'MERGE', 'KEEP-HOOKS')
DEC_ACTS = ('TO-DEC', 'KEEP-HOOKS')
COMMON = ['species_work/rename_map.tsv', 'design_next/CONVENTIONS.md',
          'design_next/decisions/README.md', 'species_work/dec_map.csv',
          'design_next/adr/README.md', 'design_next/assets/asset_manifest.csv']
STD = 'species_work/standards_baseline.md'
PHASE2_READS = ['species_work/rename_map.tsv', 'design_next/CONVENTIONS.md', 'species_work/owner_answers.md',
                'species_work/drop_topics.md', 'vision/species-vision-brief.md']
NO_PATH_DIRS = ('Never name Species directories `data/`, `code/`, `docs/`, `contrib/` or `targets/`: '
                'the provenance gate rejects them. Use `content/`, `src/`, `documentation/`, `tools/` instead.')


def dd(r):
    m = re.match(r'(DD-[A-Z-]+)', r['note'])
    return m.group(1) if m else ''


def sec(r):
    return r['target_section']


def in_lines(r, ranges):
    return any(a <= r['s'] <= b for a, b in ranges)


P = OrderedDict()


def pkg(pid, phase, wave, brief, select, targets=None, ids=None, reads=None, notes='', exempt=False,
        extra_inputs=None):
    P[pid] = dict(id=pid, phase=phase, wave=wave, brief=brief, select=select, targets=targets,
                  id_ranges=ids or {}, shared_reads=reads, notes=notes, budget_exempt=exempt,
                  extra_inputs=extra_inputs or [])


# ---------------- Phase 2, wave 2a: canonical decisions and engine ADRs ----------------
pkg('X-DEC-1', 2, '2a', '§9.2 X-DEC',
    lambda r: r['owner_package'] == 'X-DEC' and r['action'] in DEC_ACTS and
    (r['doc'] == '40' or (r['doc'] == '41' and r['s'] < 167)),
    ['decisions/verdicts_and_fixes.md'], {'DEC': '001-199'}, PHASE2_READS,
    'Canonical register for the 40 verdicts (D1-D10), the assessment matrix, the keep-list and the '
    '59-row fix-list (F1-F59), and the 41 top-15 and risk register. Wave 2b agents deduplicate against you.')
pkg('X-DEC-2a', 2, '2a', '§9.2 X-DEC (DEFER-DESIGN topics)',
    lambda r: r['action'] == 'DEFER-DESIGN' and dd(r) == 'DD-NET',
    ['decisions/deferred/networking.md'], {'DEC': '200-219'}, PHASE2_READS,
    'One DEC per DD topic (Open, "Decide by: design phase"); here DD-NET. Read for constraints, failure '
    'modes, bandwidth and population figures, not for rewriting: the content itself is dropped. Split the '
    'topic into a few DECs only where the design phase must answer separate questions.', exempt=True)
pkg('X-DEC-2b', 2, '2a', '§9.2 X-DEC (DEFER-DESIGN topics)',
    lambda r: r['action'] == 'DEFER-DESIGN' and dd(r) in ('DD-THREADS', 'DD-TICK', 'DD-ROADMAP'),
    ['decisions/deferred/process_tick_and_roadmap.md'], {'DEC': '220-239'}, PHASE2_READS,
    'Topics DD-THREADS, DD-TICK, DD-ROADMAP. Capture the lessons and integrity rules the process and '
    'threading model must respect, the tick-rate inputs, and the build-order inputs for the roadmap.')
pkg('X-DEC-2c', 2, '2a', '§9.2 X-DEC (DEFER-DESIGN topics)',
    lambda r: r['action'] == 'DEFER-DESIGN' and dd(r) in ('DD-NUMERIC', 'DD-LOBBY', 'DD-TERRAIN-RUNTIME',
                                                          'DD-SERVICES', 'DD-PERSIST'),
    ['decisions/deferred/numerics_lobby_terrain_services.md'], {'DEC': '240-259'}, PHASE2_READS,
    'Topics DD-NUMERIC, DD-LOBBY, DD-TERRAIN-RUNTIME, DD-SERVICES, DD-PERSIST.')
pkg('X-ADR-a', 2, '2a', '§9.2 X-ADR',
    lambda r: r['action'] == 'TO-ADR' and r['doc'] in ('06', '24', '25', '26', '27', '28', '40'),
    ['adr/'], {'ADR': '0001-0049'}, PHASE2_READS + [STD],
    'Engine runtime ADRs: renderer architecture, feature level and Shader Model, texture pipeline, mesh '
    'pipeline, audio engine, input, UI framework, animation and VFX runtime. Write `adr/README.md` (index '
    'for 0001-0099 plus the "Technology baseline" and "Deferred to the design phase" sections); X-ADR-b '
    'leaves a marked placeholder row "<!-- X-ADR-b rows -->" where the coordinator inserts the 0050-0099 rows at G2.')
pkg('X-ADR-b', 2, '2a', '§9.2 X-ADR',
    lambda r: r['action'] == 'TO-ADR' and r['doc'] in ('08', '29', '31', '32', '40', '41'),
    ['adr/'], {'ADR': '0050-0099'}, PHASE2_READS + [STD],
    'Toolchain, content and operations ADRs: platform and toolchain baseline, language standard and '
    'modules, build and dependency management, pack-file format and the no-loose-override rule, data '
    'schema format and versioning, localisation, accessibility, crash reporting and telemetry with consent, '
    'logging, profiling, test framework and CI, client packaging and distribution. Never edit `adr/README.md`: '
    'write your index rows (number, title, status, date) to `species_work/adr_index_X-ADR-b.md`; the coordinator merges them at G2.')

# ---------------- Phase 2, wave 2b: per-document decisions, deduplicated ----------------
W2B = ('Deduplicate: read the wave-2a register files first and reference an existing DEC instead of '
       'creating a duplicate; add only new decisions.')
pkg('X-DEC-3', 2, '2b', '§9.2 X-DEC',
    lambda r: r['owner_package'] == 'X-DEC' and r['action'] in DEC_ACTS and r['doc'] == '41' and r['s'] >= 167,
    ['decisions/open_questions.md'], {'DEC': '300-449'}, PHASE2_READS, '41 §3 open-question register. ' + W2B)
pkg('X-DEC-4a', 2, '2b', '§9.2 X-DEC',
    lambda r: r['owner_package'] == 'X-DEC' and r['action'] in DEC_ACTS and '00' <= r['doc'] <= '05',
    ['decisions/units_buildings_crates_economy.md'], {'DEC': '450-549'}, PHASE2_READS, W2B)
pkg('X-DEC-4b', 2, '2b', '§9.2 X-DEC',
    lambda r: r['owner_package'] == 'X-DEC' and r['action'] in DEC_ACTS and '06' <= r['doc'] <= '10',
    ['decisions/controls_bots_maps_meta_audio.md'], {'DEC': '550-649'}, PHASE2_READS, W2B)
pkg('X-DEC-5a', 2, '2b', '§9.2 X-DEC',
    lambda r: r['owner_package'] == 'X-DEC' and r['action'] in DEC_ACTS and '20' <= r['doc'] <= '25',
    ['decisions/simulation_terrain_render_assets.md'], {'DEC': '650-749'}, PHASE2_READS, W2B)
pkg('X-DEC-5b', 2, '2b', '§9.2 X-DEC',
    lambda r: r['owner_package'] == 'X-DEC' and r['action'] in DEC_ACTS and '26' <= r['doc'] <= '28',
    ['decisions/vfx_audio_ui.md'], {'DEC': '750-799'}, PHASE2_READS, W2B)
pkg('X-DEC-5c', 2, '2b', '§9.2 X-DEC',
    lambda r: r['owner_package'] == 'X-DEC' and r['action'] in DEC_ACTS and '29' <= r['doc'] <= '32',
    ['decisions/data_sequences_platform_debug.md'], {'DEC': '800-849'}, PHASE2_READS, W2B)


# ---------------- Phase 3: rewrite packages ----------------
def own(p, extra=None):
    return lambda r: r['owner_package'] == p and r['action'] in CONTENT and (extra is None or extra(r))


G02A1_T = ('reference/02_units/README.md', 'reference/02_units/worker.md',
           'reference/02_units/leader_and_formations.md', 'reference/02_units/')
pkg('G00', 3, None, '§9.3', own('G00'), ids={'R-VIS': '001-299'})
pkg('G01', 3, None, '§9.3', own('G01'), ids={'R-MODE': '001-599'},
    notes='Mode file names: use the Species mode names from the ledger (CONVENTIONS §8).')
pkg('G02a1', 3, None, '§9.3', own('G02a', lambda r: r['target_file'] in G02A1_T),
    ['reference/02_units/README.md', 'reference/02_units/worker.md',
     'reference/02_units/leader_and_formations.md'], {'R-UNIT': '001-299'},
    notes='The worker state machine needs the largest ID block. Facts MERGEd from 41 §1.4 are stated as rules.')
pkg('G02a2', 3, None, '§9.3', own('G02a', lambda r: r['target_file'] not in G02A1_T),
    ids={'R-UNIT': '300-499'})
pkg('G02b', 3, None, '§9.3', own('G02b'), ['reference/02_units/creatures.md', 'reference/02_units/hazards_and_projectiles.md'],
    ids={'R-UNIT': '500-899'}, notes='Output split at Phase 3 (over 120 KB): creatures.md and hazards_and_projectiles.md.')
pkg('G03a', 3, None, '§9.3', own('G03a'), ids={'R-BLDG': '001-399'})
pkg('G03b', 3, None, '§9.3', own('G03b'), ['reference/03_buildings_and_world_objects/defences_and_props.md',
    'reference/03_buildings_and_world_objects/trees_and_crate_structures.md'], ids={'R-BLDG': '400-799'},
    notes='Output split at Phase 3 (over 120 KB).')
pkg('G04', 3, None, '§9.3', own('G04'),
    ['reference/04_supply_crates_powers_and_weapons/README.md',
     'reference/04_supply_crates_powers_and_weapons/crates_and_powers.md',
     'reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md'], {'R-CRATE': '001-599'},
    notes='Output is a folder (the input is 140 KB): crates and powers from 04 §1-§5, weapons and projectiles from 04 §6.')
pkg('G05', 3, None, '§9.3', own('G05'), ids={'R-ECON': '001-399'})
pkg('G06', 3, None, '§9.3', own('G06'), ['reference/06_controls_camera_and_commands/' + f for f in ('README.md',
    'actions_and_bindings.md', 'selection_and_power_slots.md', 'cursor_and_markers.md', 'camera.md')], ids={'R-CTRL': '001-599'},
    notes='If the output passes 120 KB, make `reference/06_controls_camera_and_commands/` a folder (README plus files).')
pkg('G07', 3, None, '§9.3', own('G07'), ['reference/07_bots/' + f for f in ('README.md', '01_navigation_graph.md',
    '02_priority_model.md', '03_decision_loop_and_leaders.md', '04_powers_and_unit_policies.md',
    '05_mode_strategies_and_tutorials.md')], ids={'R-BOT': '001-399'},
    notes='State plainly where the bots use perfect information.')
pkg('G08a', 3, None, '§9.3', own('G08', lambda r: 'Map-editor hooks' not in sec(r)),
    ['reference/08_maps_and_level_design/README.md', 'reference/08_maps_and_level_design/rules_and_conventions.md'],
    {'R-MAP': '001-299'},
    notes='No map names, no map list. Per-map tables are restated as anonymous distributions or ranges.')
pkg('G08b', 3, None, '§9.3', own('G08', lambda r: 'Map-editor hooks' in sec(r)),
    ['reference/08_maps_and_level_design/map_editor_hooks.md'], {'R-MAP': '300-499'},
    notes='Hooks only (KH-EDITOR): record what the corpus describes and the verdict; do not design the editor.')
pkg('G09a', 3, None, '§9.3', own('G09a'),
    ['reference/09_front_end_and_match_flow/README.md',
     'reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md'], {'R-META': '001-399'},
    notes='You also write the folder README (document map for both files).')
pkg('G09b', 3, None, '§9.3', own('G09b'), ids={'R-META': '400-699'})
pkg('G10', 3, None, '§9.3', own('G10'), ids={'R-AUD': '001-499'},
    reads=COMMON + ['design/_work/drafts/X2b_sound_events.csv'],
    notes='The event catalogue is TO-ASSETS; read X2b_sound_events.csv for the event list and define Species source and event ids.')
pkg('G11', 3, None, '§9.3', own('G11'), ids={'R-TUT': '001-299'}, notes='New text only.')

SIM = 'reference/12_simulation_model/'
SIM_A = ('§1 ', '§2.2', '§2.3', '§2.4', '§2.5', 'Entity type hierarchy', 'Building type hierarchy',
         'Object handle')
SIM_B = ('§2.6', '§2.7', '§2.8', '§2.9', '§2.11', '§ Player command catalogue')


def sim_group(r):
    s = sec(r)
    if s.startswith('§4 Tuning'):
        return 'a' if r['doc'] == '21' else 'c'
    if s.startswith(SIM_A):
        return 'a'
    if s.startswith(SIM_B):
        return 'b'
    return 'c'


pkg('C-SIM-a', 3, None, '§9.3', own('C-SIM', lambda r: sim_group(r) == 'a'),
    [SIM + 'README.md', SIM + '02_match_lifecycle_and_state_reset.md', SIM + '03_entity_model_and_identity.md',
     SIM + '04_containers_and_update_order.md'], {'R-SIM': '001-299'},
    notes='Folder README holds purpose, the model at a glance and the document map for all seven files.')
pkg('C-SIM-b', 3, None, '§9.3', own('C-SIM', lambda r: sim_group(r) == 'b'),
    [SIM + '05_space_routing_and_physics.md', SIM + '06_teams_and_player_commands.md'], {'R-SIM': '300-499'})
pkg('C-SIM-c', 3, None, '§9.3', own('C-SIM', lambda r: sim_group(r) == 'c'),
    [SIM + '01_tick_timing_and_main_loop.md', SIM + '07_randomness_and_integrity.md'], {'R-SIM': '500-799'},
    notes='01 defines the reference tick-rate constant used by every other document.')
pkg('C-TER', 3, None, '§9.3', own('C-TER'), ids={'TER': '001-399'}, reads=COMMON + [STD],
    notes='The test vectors from 23 §16 name a reference map: restate their inputs without the name.')
RND_A = ('README.md', 'frame_graph_and_passes.md', 'world_renderers.md', 'view_lighting_and_fog.md')
pkg('C-RND-a', 3, None, '§9.3', own('C-RND', lambda r: r['target_file'].endswith(RND_A)),
    ['architecture/components/renderer/' + f for f in RND_A + ('shaders.md',)], {'RND': '001-299'},
    reads=COMMON + [STD], notes='shaders.md lists the HLSL shaders derived from the pass table.')
pkg('C-RND-b', 3, None, '§9.3', own('C-RND', lambda r: not r['target_file'].endswith(RND_A)),
    ids={'RND': '300-499'}, reads=COMMON + [STD],
    notes='visual_language.md states the look and feel to keep precisely: palettes, blend modes, glow, '
          'darkening layer, crowd rendering, the camera sense of scale.')
pkg('C-AST', 3, None, '§9.3', own('C-AST'), ids={'AST': '001-299'}, reads=COMMON + [STD], notes=NO_PATH_DIRS)
pkg('C-VFX', 3, None, '§9.3', own('C-VFX'), ids={'VFX': '001-399'}, reads=COMMON + [STD])
pkg('C-AEN', 3, None, '§9.3', own('C-AEN'), ids={'AEN': '001-399'}, reads=COMMON + [STD])
UI = 'architecture/components/ui_framework/'
UI_B = [(732, 1001), (1247, 1329), (2373, 2414)]
pkg('C-UI-a', 3, None, '§9.3', own('C-UI', lambda r: not (r['doc'] == '28' and in_lines(r, UI_B))),
    [UI + 'README.md', UI + 'core_and_widgets.md', UI + 'layout_rendering_input_and_localisation.md'],
    {'UI': '001-299'}, reads=COMMON + [STD])
pkg('C-UI-b', 3, None, '§9.3', own('C-UI', lambda r: r['doc'] == '28' and in_lines(r, UI_B)),
    [UI + 'pages_and_screen_recipes.md'], {'UI': '300-499'}, reads=COMMON + [STD],
    notes='Page graph and state machine, how a screen is built, and the framework defects as acceptance tests.')
pkg('C-INP', 3, None, '§9.3', own('C-INP'), ids={'INP': '001-299'}, reads=COMMON + [STD])
pkg('C-CAM', 3, None, '§9.3', own('C-CAM'), ids={'CAM': '001-199'}, reads=COMMON + [STD])
pkg('C-DAT', 3, None, '§9.3', own('C-DAT'), ids={'DAT': '001-399'}, reads=COMMON + [STD],
    notes='Plan a `data_schemas/` folder. ' + NO_PATH_DIRS)
pkg('C-PLT', 3, None, '§9.3', own('C-PLT'), ids={'PLT': '001-299'}, reads=COMMON + [STD])
pkg('C-BLD', 3, None, '§9.3', own('C-BLD'), ids={'BLD': '001-299'}, reads=COMMON + [STD], notes=NO_PATH_DIRS)
pkg('C-DBG', 3, None, '§9.3', own('C-DBG'), ids={'DBG': '001-299'}, reads=COMMON + [STD])
GLO_X = [dict(file='90_glossary.md', start_line=42, end_line=63, action='KEEP', heading='entry columns and kinds'),
         dict(file='90_glossary.md', start_line=1102, end_line=1111, action='KEEP', heading='vocabulary by build tag (exclusion list)')]
pkg('X-GLO-1', 3, None, '§9.3', lambda r: r['owner_package'] == 'X-GLO' and r['s'] <= 475,
    ['glossary/a_to_l.md'], reads=COMMON, notes='Terms A-L (90 lines 96-475). In-scope Species terms only.',
    extra_inputs=GLO_X)
pkg('X-GLO-2', 3, None, '§9.3', lambda r: r['owner_package'] == 'X-GLO' and r['s'] > 475,
    ['glossary/m_to_z.md'], reads=COMMON, notes='Terms M-Z (90 lines 476-844). In-scope Species terms only.',
    extra_inputs=GLO_X)
pkg('X-AST', 3, '3.1', '§9.3 X-AST', lambda r: r['owner_package'] == 'X-AST',
    ['assets/GameAssets.md', 'assets/README.md', 'assets/asset_manifest.csv', 'assets/art_and_audio_briefs.md'],
    reads=['species_work/rename_map.tsv', 'design_next/CONVENTIONS.md', 'species_work/standards_baseline.md',
           'species_work/owner_answers.md'], exempt=True,
    notes='Script-driven from the CSVs (brief); TO-ASSETS prose only where the brief allows. Pass 1 started early (during Phase 2) at the owner\'s request, with GameAssets.md as its main deliverable; pass 2 in Phase 4.')


def main():
    os.makedirs(os.path.join(WORK, 'packages'), exist_ok=True)
    claimed = {}
    index = ['| Package | Phase/wave | Rows | Input KB | Over budget | Targets | ID ranges |', '|---|---|---|---|---|---|---|']
    for pid, p in P.items():
        sel = [r for r in ROWS if p['select'](r)]
        for r in sel:
            if r['action'] in CONTENT:
                key = (r['orig_file'], r['s'], r['e'], r['action'])
                if key in claimed and claimed[key] != pid:
                    print(f'WARN {pid}: row {key} also claimed by {claimed[key]}')
                claimed[key] = pid
        inputs = [dict(file=r['orig_file'], start_line=r['s'], end_line=r['e'], anchor=r['anchor'],
                       action=r['action'], target_file=r['target_file'], target_section=r['target_section'],
                       note=r['note']) for r in sel] + p['extra_inputs']
        kb = round(sum(r['b'] for r in sel) / 1024, 1)
        targets = p['targets'] or sorted({r['target_file'] for r in sel if r['action'] in CONTENT} or
                                         {r['target_file'] for r in sel})
        out = dict(id=pid, phase=p['phase'], wave=p['wave'], brief=p['brief'], targets=targets,
                   id_ranges=p['id_ranges'], shared_reads=p['shared_reads'] or COMMON, notes=p['notes'],
                   budget_kb=kb, budget_exempt=p['budget_exempt'], inputs=inputs)
        with open(os.path.join(WORK, 'packages', f'{pid}.json'), 'w', encoding='utf-8') as f:
            json.dump(out, f, indent=1, ensure_ascii=False)
        over = 'yes' if kb > BUDGET_KB and not p['budget_exempt'] else ('exempt' if p['budget_exempt'] else '')
        index.append(f"| {pid} | {p['phase']}/{p['wave'] or '-'} | {len(sel)} | {kb} | {over} | "
                     f"{'<br>'.join(targets)} | {', '.join(f'{k} {v}' for k, v in p['id_ranges'].items())} |")
    # every content row must be claimed exactly once
    missing = [r for r in ROWS if r['action'] in CONTENT and
               (r['orig_file'], r['s'], r['e'], r['action']) not in claimed]
    for r in missing:
        print(f"UNCLAIMED {r['orig_file']}:{r['s']}-{r['e']} {r['action']} -> {r['owner_package']}")
    with open(os.path.join(WORK, 'packages', 'INDEX.md'), 'w', encoding='utf-8') as f:
        f.write('# Work packages (built at G1 by scripts/build_packages.py)\n\n' + '\n'.join(index) + '\n')
    print(f'{len(P)} packages; unclaimed content rows: {len(missing)}')
    print('\n'.join(index))


if __name__ == '__main__':
    main()
