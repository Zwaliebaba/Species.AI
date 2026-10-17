#!/usr/bin/env python3
"""F1: negative self-tests for the pattern lists.

Usage: python3 -I F1_selftest.py <WORK dir> <f1 dir> <scratch dir>

1 Fixtures that must give no hard-fail hit: glTF, d3dx12.h, glow, global, glyph (in several cases).
2 Ordinary-English sweep: every word that ORIG prose uses in lower case at least 3 times is scanned
  in lower case and in sentence-initial capitalised form; hits are listed for review.
3 A clean Species sample paragraph (ledger terms and the target technology) must give no hard-fail hit.
Writes <f1>/selftest_negative.tsv and prints a summary.
"""
import csv
import os
import subprocess
import sys

FIXTURES = [
    'The mesh pipeline imports glTF 2.0 files; glTF binary (.glb) is accepted.',
    'Helpers come from d3dx12.h, and the D3DX12 helper structs are used for barriers.',
    'The glow pass adds an additive glow; Glow intensity is tuned per team colour.',
    'A global constant buffer holds global state; Global illumination is not used.',
    'The glyph cache rasterises each glyph once; Glyph metrics come from DirectWrite.',
    'GLOW and GLOBAL and GLYPH in capitals are not identifiers either.',
]

SPECIES_SAMPLE = '''# Spawn Control rules (sample)
Workers occupy the five ports of a Spawn Node. When a team holds a Spawn Node, the Source Node sends
Essence orbs along the Relays of the spawn network, and the Spawn Node turns each orb into a Worker.
A Leader can be set as a Waypoint beacon. A Strike team of three Troopers, a Transport, a Collector and
a Builder come from Supply crates. Powers sit in the Power slots; the Comeback Bonus and the Catch-up bonus
help a losing team. Crate outcomes include Shield, Convert, Speed Boost, Fury, Reinforcements, Spawn Surge,
Rocket Troops, Supply Storm, Frenzy, Time Dilation, Time Rush, Essence Grove, Blight Grove, Hive, Brood, Swarm,
Allegiance Scrambler, Sickness, Abductor, Spitter Plants, Firebomb Run, Missile Barrage and meteor shower.
Creatures: Crawler, Reaper Serpent, Brood Drifter, Seed Pod, ant, centipede, spider, Crawler egg, Wisp.
Objects: Transit Gate, Core Device, Command Console, Launch Vessel, fuel depot, fuel pump, fuel line,
Solar Array, Beam Dish, Barrier Emitter, Breach Charge, Relic, Relic Base, Relic Spawn Zone, Control Zone,
Flag post, Home post, turret, Convert Turret, wall, tree, jump pad, exclusion zone, energy grid, grid pylon.
Modes: Spawn Control, Zone Control, Relic Run, Siege, Launch Race, Flag Chain. Rogue faction, Visitor faction, Bot.
Achievements: Total Control, Stockpiler, Crowned, Clean Launch, Iron Defence, Giant Slayer, Completionist,
Tourist, Massacre, Flag Sweep, Friendly Fire, Annihilator.

## Engine baseline (sample)
The renderer targets Direct3D 12 through the Agility SDK, feature level 12_1 and HLSL Shader Model 6.6,
compiled with DXC. The DXGI flip-model swap chain presents each frame. Textures are DDS with BC7, BC5 and BC4,
built with DirectXTex. Meshes are glTF 2.0; the d3dx12.h helpers build root signatures. The glyph cache uses
DirectWrite; the glow pass and global constant buffers follow the frame graph. Audio uses XAudio2 2.9 and
X3DAudio; input uses GameInput; the Win32 platform layer owns the window. The build uses MSVC, CMake presets
and vcpkg manifest mode on Windows 11. PIX and WinPixEventRuntime capture frames. The UI follows the Xbox
Accessibility Guidelines. Types: `SimObject`, `Actor`, `Structure`, `ActorGroup`, `MatchWorld`, `MatchTeam`,
`TickClock`, `ActorGrid`, `ObstacleGrid`, `RouteGraph`, `SupplyCrate`, `CrateOutcome::Shield`, `TurretType::Convert`,
`MatchMode::SpawnControl`, `SimRng::NextUint`, `FxRng::NextUnit`, `PowerSlots`, `Power`, `EssenceOrb`.
Tuning keys: `ref.unit.worker.move_speed`, `ref.mode.spawn_control.match_length`. Strings: `ui.lobby.ready`,
`mode.flag_chain.objective`. Map files follow `map_spawn_control_2p_01`.
'''


def run_scan(work, patterns, paths, counts=None):
    cmd = [sys.executable, '-I', os.path.join(work, 'scripts', 'scan_patterns.py')]
    if counts:
        cmd += ['--counts', counts]
    cmd += [os.path.join(work, patterns)] + paths
    res = subprocess.run(cmd, capture_output=True, text=True)
    return res.returncode, [ln for ln in res.stdout.splitlines() if ln]


def main():
    work, f1, scratch = sys.argv[1:4]
    os.makedirs(scratch, exist_ok=True)
    fx = os.path.join(scratch, 'fixtures.md')
    with open(fx, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join(FIXTURES) + '\n')
    sp = os.path.join(scratch, 'species_sample.md')
    with open(sp, 'w', encoding='utf-8') as fh:
        fh.write(SPECIES_SAMPLE)
        # every Species term and identifier of the ledger (rows that are not dropped)
        with open(os.path.join(work, 'rename_map.tsv'), encoding='utf-8', newline='') as lf:
            for r in csv.DictReader(lf, delimiter='\t'):
                if r['rule'] == 'drop':
                    continue
                if r['species_term']:
                    fh.write('The term %s is used here.\n' % r['species_term'])
                if r['species_identifier']:
                    fh.write('The identifier `%s` is used here.\n' % r['species_identifier'])
                if r['string_id_prefix']:
                    fh.write('The string id `%s.title` is used here.\n' % r['string_id_prefix'])
    vocab = []
    with open(os.path.join(f1, 'prose_vocab.tsv'), encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh, delimiter='\t'):
            if int(r['lower_count']) >= 1:
                vocab.append(r['word'])
    lw = os.path.join(scratch, 'vocab_lower.md')
    with open(lw, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join('and then %s is said.' % w for w in vocab) + '\n')
    cw = os.path.join(scratch, 'vocab_capitalised.md')
    with open(cw, 'w', encoding='utf-8') as fh:
        fh.write('\n'.join('%s is said first.' % (w[:1].upper() + w[1:]) for w in vocab) + '\n')

    rows = []
    rc, hits = run_scan(work, 'forbidden_patterns.tsv', [fx])
    rows += [('fixtures', h) for h in hits]
    fixtures_ok = not hits
    rc, hits = run_scan(work, 'forbidden_patterns.tsv', [sp])
    rows += [('species-sample-hard', h) for h in hits]
    sample_ok = not hits
    rc, shits = run_scan(work, 'soft_patterns.tsv', [sp])
    rows += [('species-sample-soft', h) for h in shits]
    rc, lhits = run_scan(work, 'forbidden_patterns.tsv', [lw])
    rows += [('vocab-lower', h) for h in lhits]
    rc, chits = run_scan(work, 'forbidden_patterns.tsv', [cw])
    rows += [('vocab-capitalised', h) for h in chits]
    with open(os.path.join(f1, 'selftest_negative.tsv'), 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['test', 'hit'])
        wr.writerows(rows)
    print('fixtures (glTF, d3dx12.h, glow, global, glyph): %s' % ('PASS, 0 hard hits' if fixtures_ok else 'FAIL'))
    print('clean Species sample: %s; soft hits %d' % ('PASS, 0 hard hits' if sample_ok else 'FAIL', len(shits)))
    print('ordinary-English sweep over %d words: %d lower-case hits, %d capitalised hits (listed in selftest_negative.tsv)'
          % (len(vocab), len(lhits), len(chits)))
    for h in lhits[:80]:
        print('  lower:', h.split(':', 2)[2])
    for h in chits[:200]:
        print('  cap:', h.split(':', 2)[2])
    if not (fixtures_ok and sample_ok):
        for t, h in rows:
            if t in ('fixtures', 'species-sample-hard'):
                print('  FAIL', t, h)
        sys.exit(1)


if __name__ == '__main__':
    main()
