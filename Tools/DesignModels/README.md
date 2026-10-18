# DesignModels

Quantitative models of the design. They are development tools: nothing here ships or links into the executable
(`AGENTS.md` R14). Each model is written from the design documents alone and states which document it implements.

## TerrainGenerator.py

An independent implementation of terrain height-field generation, written from the terrain specification
(`Design/architecture/components/terrain/`) alone. It reproduces the test values of that specification: the vector
sets of `height_field_generation.md` §7.2, which tests T1 to T3 and T7 to T13 of the terrain `README.md` §7.1 use. It
is the conformance generator that the specification names: the expected values of a new vector set are produced with
it, never taken from a map.

```
python3 Tools/DesignModels/TerrainGenerator.py            # check: every test value, exact equality
python3 Tools/DesignModels/TerrainGenerator.py traceA     # set A: terrain values, tile 2 events, derived values
python3 Tools/DesignModels/TerrainGenerator.py traceM1    # set A, tile 0 (method 1): guide pins and first calls
python3 Tools/DesignModels/TerrainGenerator.py traceB     # set B (method 2): first calls, highest sample
python3 Tools/DesignModels/TerrainGenerator.py smooth     # set C: one smooth evaluation (its inputs come from set A)
python3 Tools/DesignModels/TerrainGenerator.py flatten    # set D: the five flatten areas in order
python3 Tools/DesignModels/TerrainGenerator.py full       # traceA, then normals and bilinear reads
python3 Tools/DesignModels/TerrainGenerator.py log2       # ceilLog2 for every integer from 1 to 69 999
```

`check` prints one line per test and exits with status 1 when any value differs. Values are printed with `repr()`,
which gives the shortest text that parses back to the same binary64 value. Python 3 with the standard library only;
no input files.

## A1WorldDensity.py

The world-density model of area A1, written from `Design/gdd/01_world_and_regions/` (prefix WLD, three files). For a
world of a given steward capacity it computes the regions, settlement sites, native AI settlements and site margin
(WLD-003, WLD-004), generates a layout from a seed (regions as a compact blob at the centre of the reserved extent,
four sites per island valley offset and rotated as a whole, crossings, the travel network), and reports settlements per
region, travel times between neighbours, across a region and across the world over seeds 1 to 10 (WLD-100 to
WLD-104), the contact rate (the world day on which territories meet, and the territory contacts of every settlement in
a fresh world at 25, 50 and 100 % steward occupancy, WLD-064, WLD-065), the life of a world over a year of world days
(arrivals, active lifetimes, 21 days on the map under the deputy, dormancy, returns, the return queue and the stand-in
AI settlements, five seeds per scenario, WLD-124 to WLD-127), and the scale figures that area A2 sizes the server with
(bodies per region, battles in progress at the evening peak and at the session pace in a base, a planning and a stress
case with the 99th percentile and the weekly maximum, the battle caps, expeditions in progress, the union of watched
settlements, terrain samples; WLD-280 to WLD-285). Every figure is given for 16 and 200 stewards.

```
python3 -I Tools/DesignModels/A1WorldDensity.py              # check: every figure and table stated
python3 -I Tools/DesignModels/A1WorldDensity.py report       # world size, geometry, travel, contact and scale
python3 -I Tools/DesignModels/A1WorldDensity.py lifetime     # a year of world days, keeper on and off, 5 seeds each
python3 -I Tools/DesignModels/A1WorldDensity.py battles      # battles in progress: 3 cases, p99, weekly max
python3 -I Tools/DesignModels/A1WorldDensity.py sweep        # contact days and lens over spacing and growth
python3 -I Tools/DesignModels/A1WorldDensity.py founding     # founding rules compared
python3 -I Tools/DesignModels/A1WorldDensity.py tables       # every table of figures (python: STATED_TABLES)
python3 -I Tools/DesignModels/A1WorldDensity.py layout 16 1  # one generated layout (capacity, seed) as text
```

Inputs are the WLD tuning values (class `Params`, each field naming its key), placeholders for values that later
areas set (the territory growth curve and plateau population, B1; gathering trips, B2; the share of attacks that
present stewards declare, B5; the evening window of present play, B6), each marked in the source, the battle cases
(`BATTLE_CASES`) and the lifetime scenarios (class `Scenario`, placeholders of B6 and C2). Every random value is a
counter-based SplitMix64 hash of the seed and its keys. `check` prints one line per figure (140 at present) and exits
with status 1 when any differs; it runs in about 5 seconds. Python 3 with the standard library only; no input files.

## A2ServerBudget.py

The server-budget model of area A2, written from `Design/architecture/04_solution_strategy.md` and the records
ADR-079 to ADR-088 of `Design/ADR/`. From per-figure CPU cost budgets (ADR-080), the replication encoding (ADR-086),
the persistence constraints (chapter §4.3.1) and area A1's world figures, which it reads from `A1WorldDensity.py` in
this folder by its path (settlements and regions per capacity, stewards present and settlements watched at the
evening peak, the battles in progress of the planning and the stress case, the placeholder plateau population), it
computes at 16 and at 200 stewards: server CPU per settlement (full and abstract fidelity), per battle, per region and
per world, with the sensitivity to costs at 2 and 4 times the budgets; bandwidth per client by view (home, battle,
valley war, strategic) at 10 and at 20 ticks per second, upstream, the integrated host's upload and hosted egress;
storage of a world over its lifetime and its growth per world week, checkpoint writes and server memory; the battle
sizes of PIL-021 against each stage's budget; the two forms of a battle replay; command-to-display latency; and the
share of the evening peak at the world's battle cap for candidate caps.

```
python3 -I Tools/DesignModels/A2ServerBudget.py              # check: every figure and table the chapter states
python3 -I Tools/DesignModels/A2ServerBudget.py report       # every section below
python3 -I Tools/DesignModels/A2ServerBudget.py cpu          # CPU per settlement, battle, region and world
python3 -I Tools/DesignModels/A2ServerBudget.py bandwidth    # per client by view, upload, egress
python3 -I Tools/DesignModels/A2ServerBudget.py storage      # storage, growth, checkpoints, memory, replays
python3 -I Tools/DesignModels/A2ServerBudget.py latency      # command-to-display latency at 10 and 20 Hz
python3 -I Tools/DesignModels/A2ServerBudget.py caps         # the world battle cap: share of the peak at the cap
python3 -I Tools/DesignModels/A2ServerBudget.py tables       # the chapter's tables (python: STATED_TABLES)
```

Inputs are class `Params`, each field naming the record or document it comes from; values that later areas set are
marked "placeholder" with that area (the abstract step's and the AI's costs, B2, B7 and D7; expedition sizes, B4;
away-report entries, replay share and dormant records, B6 and C2). The CPU costs are budgets the implementation must
meet, not measurements. `check` prints one line per figure (37 figures and 10 tables at present) and exits with status
1 when any differs; it runs in under a second. Python 3 with the standard library only; no input files besides A1's
model.

## Adding a model

The design phase adds its quantitative models here: economy and pacing curves, population and spawn models,
balance tables and any other model whose numbers a design document states. Each model is one Python file named in the
`AGENTS.md` style (`PacingModel.py`), LF line endings, 2-space indent and 120 columns (`.editorconfig`), with a
docstring naming the design document it implements and a `check` mode that reproduces the numbers that document
states. Add it to this file.
