"""FX-08: split architecture/components/terrain.md into the folder terrain/.

Usage: python3 -I FX-08_split_terrain.py <terrain.md> <out_dir>

Mechanical split, no content change:
  README.md                           everything except the sections below
  height_field_generation.md          §4.2 to §4.9 and §7.2
  edges_normals_colours_and_sea.md    §4.10 to §4.16
Every relative link is rewritten for the new folder depth. Heading texts, requirement ids
and tuning keys are kept byte for byte; the script checks that every TER id and every
backticked key of the source is still present exactly as often in the output.
"""
import os
import re
import sys
from collections import Counter

SRC, OUT = sys.argv[1], sys.argv[2]
src_dir = os.path.dirname(os.path.abspath(SRC))
out_dir = os.path.abspath(OUT)
lines = open(SRC, encoding='utf-8').read().split('\n')

LINK = re.compile(r'(?<!!)(\[(?:[^\]\[]|\[[^\]]*\])*\]\()([^)\s]+)((?:\s+"[^"]*")?\))')
FENCE = re.compile(r'^[ \t]*(```|~~~)')


def relink(text):
    out, inside = [], False
    for line in text.split('\n'):
        if FENCE.match(line):
            inside = not inside
            out.append(line)
            continue
        if inside or '](' not in line:
            out.append(line)
            continue

        def fix(m):
            t = m.group(2)
            if re.match(r'^[a-z][a-z0-9+.-]*:', t, re.I) or t.startswith('#'):
                return m.group(0)
            path, sep, anchor = t.partition('#')
            dest = os.path.normpath(os.path.join(src_dir, path))
            new = os.path.relpath(dest, out_dir).replace(os.sep, '/')
            return m.group(1) + new + (sep + anchor if sep else '') + m.group(3)

        out.append(LINK.sub(fix, line))
    return '\n'.join(out)


def idx(prefix):
    for i, l in enumerate(lines):
        if l.startswith(prefix):
            return i
    raise SystemExit(f'heading not found: {prefix}')


i42, i410, i417 = idx('### 4.2 '), idx('### 4.10 '), idx('### 4.17 ')
i5, i72, i73 = idx('## 5 '), idx('### 7.2 '), idx('### 7.3 ')

readme_part = lines[:i42] + lines[i417:i72] + lines[i73:]
gen_part = lines[i42:i410]
vec_part = lines[i72:i73]
sea_part = lines[i410:i417]


def strip_trailing_blank(block):
    while block and block[-1] == '':
        block = block[:-1]
    return block


GEN_HEAD = '''# Height-field generation

> Status: Engine baseline · Prefix: TER · ADRs: [ADR-052](../../../ADR/ADR-052-cpp23-language-standard.md) · [ADR-061](../../../ADR/ADR-061-data-schemas-and-versioning.md) · [ADR-069](../../../ADR/ADR-069-test-framework.md) · Related: [Terrain](README.md) · [Edges, normals, colours and sea](edges_normals_colours_and_sea.md) · [Maps: rules and conventions](../../../reference/08_maps_and_level_design/rules_and_conventions.md) · [Randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md)

This document specifies how the terrain component generates the height field from the terrain records of a map: the numeric primitives and grid reads, the private tile generator, the generation of one tile, the generation methods, the noise, the tile edge cases, the composition of tiles and the flatten areas; and the vector sets that every implementation reproduces. The pipeline that runs these steps in order is [README](README.md) §4.1, and the notation is that of the README. Section numbers are shared by the files of this folder.

## 1 Responsibilities and boundaries

The responsibilities and boundaries of the terrain component are [README](README.md) §1 (TER-001 to TER-009). This document specifies steps 1 to 3 of the pipeline of TER-060 and the grid primitives that the queries of README §2.2 use.

## 2 Interfaces

The types and functions specified here are those of the interface sketch of [README](README.md) §2.1: `SurfaceGrid`, `Grid2D`, `TerrainTile`, `TerrainFlattenArea` and `TerrainField::Generate`. The private tile generator (§4.3) has no public interface.

## 3 Data model

The records that generation reads are [README](README.md) §3: the terrain definition (table 3.1-A), the terrain tiles (table 3.2-A), the flatten areas (table 3.3-A) and the guide grid (table 3.4-A).

## 4 Behaviour and algorithms
'''

GEN_TAIL = '''
## 5 Concurrency and performance budget

The cost of generation and its place outside every tick and frame budget are [README](README.md) §5 (TER-240 to TER-242).

## 6 Failure modes, diagnostics and telemetry

The content-build rejections of TER-112 and TER-124 are part of the validation of [README](README.md) TER-250; the generation log and the per-tile trace are TER-253.

## 7 Acceptance tests

The tests are [README](README.md) §7.1 (T1 to T16 and T22 to T26 use the vector sets below), and the conformance procedure is README §7.3.
'''

SEA_HEAD = '''# Edges, normals, colours and sea

> Status: Engine baseline · Prefix: TER · ADRs: [ADR-011](../../../ADR/ADR-011-world-surface-rendering.md) · [ADR-060](../../../ADR/ADR-060-content-identity-by-hash.md) · [ADR-061](../../../ADR/ADR-061-data-schemas-and-versioning.md) · Related: [Terrain](README.md) · [Height-field generation](height_field_generation.md) · [World renderers](../renderer/world_renderers.md) · [Options and budgets](../renderer/options_and_budgets.md) · [Space, routing and physics](../../../reference/12_simulation_model/05_space_routing_and_physics.md)

This document specifies the world grid of the generated height field and its edge semantics, the simulation normal grid, the per-triangle colour-ramp look-up and its presentation generators, the sea derivations (shore mask, depth map, flat-water tile mask, wave meshes and coastline waves), the terrain lifecycle, and the conversions that the output depends on. Section numbers are shared by the files of this folder.

## 1 Responsibilities and boundaries

The responsibilities and boundaries of the terrain component are [README](README.md) §1 (TER-001 to TER-009). The world extent, the edge semantics and the normal grid are simulation data; the colour look-up and the sea derivations are presentation data that never feed the simulation (TER-007).

## 2 Interfaces

The functions specified here are those of the interface sketch of [README](README.md) §2.1: `TerrainField::Height`, `TerrainField::Normal` and the presentation derivations `FacetColor`, `BuildSeaMasks` and `SelectWaveVertexPairs`.

## 3 Data model

The rendering inputs and data products are [README](README.md) §3.6 and §3.7 (table 3.7-A).

## 4 Behaviour and algorithms
'''

SEA_TAIL = '''
## 5 Concurrency and performance budget

The memory of the grids and the sea masks and the cost of the queries are [README](README.md) §5 (TER-241, TER-243).

## 6 Failure modes, diagnostics and telemetry

A missing ramp, sea tile or gradient falls back to the map default ([README](README.md) TER-251); the grid digests are TER-252.

## 7 Acceptance tests

The tests of this document are [README](README.md) §7.1: T5, T15, T17 to T21 and T24.

## 8 Open decisions

The decisions are [README](README.md) §8.1; the design-phase inputs on off-map heights, cosmetic noise and the sea at world scale are README §8.3 items 7, 8 and 10.
'''

GEN_END = '''
## 8 Open decisions

The decisions are [README](README.md) §8.1; the generator variants and the numeric model are README §8.3 items 3 and 9.
'''

os.makedirs(out_dir, exist_ok=True)
gen = GEN_HEAD + '\n' + relink('\n'.join(strip_trailing_blank(gen_part))) + '\n' + GEN_TAIL + '\n' + \
    relink('\n'.join(strip_trailing_blank(vec_part))) + '\n' + GEN_END
sea = SEA_HEAD + '\n' + relink('\n'.join(strip_trailing_blank(sea_part))) + '\n' + SEA_TAIL
readme = relink('\n'.join(readme_part))

files = {'README.md': readme, 'height_field_generation.md': gen, 'edges_normals_colours_and_sea.md': sea}
for name, text in files.items():
    if not text.endswith('\n'):
        text += '\n'
    open(os.path.join(out_dir, name), 'w', encoding='utf-8').write(text)

# checks: every id and key occurrence survives, every heading survives exactly once in the moved body
src_text = '\n'.join(lines)
all_out = '\n'.join(files.values())
for pat in (r'\*\*TER-\d{3}\*\*', r'`ter\.[a-z0-9_.]+`', r'^#{2,3} \d.*$'):
    a = Counter(re.findall(pat, src_text, re.M))
    b = Counter(re.findall(pat, all_out, re.M))
    missing = {k: v for k, v in a.items() if b[k] < v}
    print(pat, 'source', sum(a.values()), 'output', sum(b.values()), 'missing', missing or 'none')
for name, text in files.items():
    print(name, len(text.encode('utf-8')), 'bytes')
