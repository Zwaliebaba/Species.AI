# Multiwinia — functional and technical design recovery

This directory is a design recovery of **Multiwinia** (Introversion Software, 2008), built from the released source code. It describes what the game does and how its engine works, in enough detail for a new team to build a new game on it without reading the original source. The target platform for that new game is **Windows, C++23, DirectX 12, multiplayer**.

- **Source analysed:** `multiwinia/` in `gene9/Darwinia-and-Multiwinia-Source-Code`, commit `f909630267a320c8f8de5e30bbb2dbf5f3a68acf`. The tree has 6,189 files, of which 776 are live source files (about 235k lines) and 2,332 are data files.
- **Scope:** `MULTIWINIA_ONLY`. The single-player Darwinia campaign code that is compiled into the same binary is documented only as `[B:LEGACY]`.
- **Licence posture:** `INTERNAL_STUDY`. The documents may quote short verbatim excerpts of 15 lines or fewer, each marked with a `Verbatim excerpt` line. Introversion's licence (`multiwinia/EULA.txt`) prohibits redistributing verbatim source. Every name, character, text, art and audio asset that is Introversion IP is listed for replacement in [41](41_risks_and_open_questions.md); the canonical neutral placeholder names are in [90](90_glossary.md).

{{STATUS}}

## Executive summary

**What Multiwinia is.** Multiwinia is a 2–4 player real-time strategy game built around population flow. Players don't build units from a queue. Territory produces units: captured spawn points emit a steady stream of Darwinians, and the player directs them through officers, formations and a small set of power-up "tasks". Dead units release spirits that feed the economy back. Crates fall from the sky and are the strategic wildcard: 31 reward types ship, drawn from 90 %/10 % good/bad pools. Six modes ship: Domination, King of the Hill, Capture the Statue, Assault, Rocket Riot and Blitzkrieg. They are played on 49 retail maps, against human or CPU opponents. See [00](00_overview.md) and [01](01_game_modes.md)–[10](10_audio_design.md).

**How it is built.** It is a single-threaded C++ simulation advancing at a fixed **10 Hz** (0.1 s slices), with render-time extrapolation in between. Multiplayer uses **relay lockstep**: the host's server sequences every peer's commands into 10 Hz frames, and every peer, including the host's own client, simulates the whole world. There is no dedicated server. The shipped renderer is the **OpenGL** build; the Direct3D 9 path (an OpenGL-API emulation layer) has four independent compile breakages at this commit. Terrain is generated procedurally from per-tile LCG seeds stored in each level file. See [20](20_architecture.md)–[32](32_debug_test_and_performance.md).

**What a new team most needs to know.** These are the headline findings, each [C] unless marked; details and links are in [40](40_carry_forward_assessment.md) and [41](41_risks_and_open_questions.md).

1. **Multiplayer security and robustness are weak.**
   - The per-frame sync check hashes only the synced random-number stream, not world state, and a detected desync freezes every peer permanently.
   - The server never checks that a command's sender owns the team it targets, and lobby/host authority exists only in the UI.
   - Peers start a match by map **file name** only; no map CRC is exchanged.
   - Live desync paths exist: the insertion-squad camera look-around flag is read inside the simulation, and some statics survive between matches. See [22](22_determinism_and_networking.md).
2. **Determinism rests on fragile numerics.** Correctness depends on x87 precision, `/fp:precise`, the MSVC 2005 CRT's `pow`/`exp`/`log` rounding and a float-truncation wrapper (`iv_trunc_low_nibble`). The D3D configuration's `/fp:fast` plus missing `D3DCREATE_FPU_PRESERVE` would likely desync it against the GL build. See [22](22_determinism_and_networking.md) and [23](23_terrain_and_landscape.md).
3. **"Seconds" are not seconds.** Rule timers that subtract 0.1 per tick finish one tick late, so a "1-second" rule fires every 11 ticks (1.1 s). AI periods are 11/101/200 ticks. Several creature timers count per frame. A new implementation should use integer tick counters and decide the intended values. See [01](01_game_modes.md), [04](04_crates_powerups_and_weapons.md) and [07](07_ai.md).
4. **Economy edge cases shape play.**
   - The population cap per team is the map's cap divided by the map's player count, not by the number of teams actually playing. Two co-op maps have no spawn cap.
   - Harvesters collect any team's souls and bypass both the cap and the 30 s activation delay.
   - Retribution is effectively on by default, because an absent option reads as −1.
   See [05](05_economy_and_population.md).
5. **Much of the code is not the shipped game.** These are compiled in but unreachable in a Multiwinia match: the Darwinia campaign, Tank Battle (debug only), the map editor (no target defines `LOCATION_EDITOR`), achievements (stubbed in this PC source), leaderboards, profiles, key rebinding, attract mode, the benchmark, the start sequence, scripts (all 42 are legacy or dead), Sepulveda, and gestures. **Music is never heard**: every trigger is compiled out or Darwinia-only, and the 11 tracks are excluded from the shipped archives. See [09](09_meta_systems.md), [10](10_audio_design.md) and [30](30_scripting_tutorial_and_cutscenes.md).
6. **Data binding is brittle.**
   - `stats.txt` and `effects.txt` bind by **row position**.
   - A blank line inside a `sounds.txt` block crashes the loader.
   - The texture cache ignores load flags, so whichever caller loads a texture first fixes its mipmap and mask settings.
   - One shipped texture uses RLE8 compression and cannot be decoded.
   See [25](25_textures_and_assets.md) and [29](29_data_formats_reference.md).
7. **UI/render code writes simulation state in places.** Examples are a cursor `=`/`==` typo, local-only task bookkeeping, and tree collision spheres generated during display-list build from an unsynced LCG. A new engine should separate simulation and presentation strictly. See [06](06_player_control_and_input.md), [03](03_buildings_and_world_objects.md) and [21](21_simulation_and_entity_model.md).

**Recommended direction.** The full assessment is in [40](40_carry_forward_assessment.md). Keep the game design: the population-flow loop, the spawn economy, officers and formations, the crate pools and the six modes' rules, with the quirks above decided case by case. Replace the technology stack entirely: D3D12, XAudio2, a modern networking layer with server-side command authority, map/version hashing and state hashing, strict-IEEE or fixed-point simulation maths with integer tick timers, BCn/DDS assets in hashed archives, and a modern UI framework. All IP must be replaced (see [41](41_risks_and_open_questions.md) and [90](90_glossary.md)).

## Reading guide

| If you are… | Start with | Then |
|---|---|---|
| a game designer | [00](00_overview.md), [01](01_game_modes.md), [05](05_economy_and_population.md) | [02](02_units_and_entities.md)–[04](04_crates_powerups_and_weapons.md), [07](07_ai.md), [08](08_levels_and_level_design.md) |
| an engine architect | [20](20_architecture.md), [21](21_simulation_and_entity_model.md), [22](22_determinism_and_networking.md) | [40](40_carry_forward_assessment.md), [24](24_rendering.md), [31](31_platform_build_and_tools.md) |
| a network/multiplayer engineer | [22](22_determinism_and_networking.md) | [20](20_architecture.md) §timing, [09](09_meta_systems.md) lobby, [41](41_risks_and_open_questions.md) |
| a graphics programmer | [24](24_rendering.md) | [23](23_terrain_and_landscape.md), [25](25_textures_and_assets.md), [26](26_animation_and_vfx.md) |
| an audio programmer/designer | [10](10_audio_design.md) | [27](27_audio_engine.md) |
| a tools/pipeline engineer | [29](29_data_formats_reference.md), [31](31_platform_build_and_tools.md) | [25](25_textures_and_assets.md), [08](08_levels_and_level_design.md) |
| a producer / tech lead | this page, [40](40_carry_forward_assessment.md), [41](41_risks_and_open_questions.md) | [00](00_overview.md) |

Each document has the same closing sections: Open questions and unknowns, Docs-vs-code discrepancies, IP items, a **Source map** (every source/data file behind that document) and an appendix on dead/legacy code. Use [91](91_cross_reference_index.md) to go from a concept or class to every document and source file, and [99](99_coverage_matrix.md) to go from any file in the tree to the documents that cite it.

## Document map

{{DOCMAP}}

## How to read the evidence

Every non-trivial claim carries a source reference: `path:Symbol` for code (paths relative to `multiwinia/`, e.g. `code/worldobject/darwinian.cpp:Darwinian::Advance`), or a `data/...` path for data. Each claim also carries one evidence tag:

| Tag | Meaning |
|---|---|
| **[C] Confirmed** | Read directly from code or data, and spot-verified during synthesis. |
| **[I] Inferred** | Deduced from structure, naming, partial reading or a re-implementation (for example the Python terrain port). The reasoning is stated. A design-critical [I] is listed in 41. |
| **[U] Unknown** | Could not be determined from the tree; the entry says what would resolve it. All [U] items are collected in 41. |

**Build tags** say where a feature exists. The full matrix is in `_work/build_matrix.md`.

| Tag | Meaning |
|---|---|
| **[B:FULL]** | The baseline: `Release|Win32` (OpenGL) with `TARGET_PC_FULLGAME`, shipped as `multiwinia.exe` by every Multiwinia packaging script. |
| **[B:D3D]** | Only under `USE_DIRECT3D` (`ReleaseDirect3D`; probably does not compile at this commit). |
| **[B:DEBUG]** | Only under `TARGET_DEBUG`. |
| **[B:EDITOR]** | Only under `LOCATION_EDITOR`, which no shipped target defines. |
| **[B:DEMO]** | The runtime demo gate (`IS_DEMO`) or `TARGET_MULTIWINIA_DEMOONLY`. |
| **[B:LEGACY]** | Compiled into FULL but unreachable in Multiwinia (Darwinia campaign, `BLOCK_OLD_DARWINIA_OBJECTS`). |
| **[B:DEAD]** | Not compiled in any PC configuration. |

**Docs vs code.** The `docs/` folder in the source tree is supporting evidence only. Where it disagrees with the code, the documents describe the code and log the discrepancy.

## Method and working files (`_work/`)

The set was produced in five phases, following the brief's method.

1. **Inventory.** `manifest.csv` lists every file in the tree. `build_matrix.md` records the live build. `partition_plan.md` and `partitions.json` split the 776 live source files into 21 subsystem partitions.
2. **Subsystem analysis.** 21 partition agents and 6 cross-cutting agents (two data-format agents, two asset-catalogue agents, determinism, level catalogue) wrote `drafts/*.md`, `drafts/*.csv` and per-file `coverage/*.csv`.
3. **Synthesis.** `scripts/route_sections.py` routed draft sections into `routed/`. One synthesis agent per document merged the drafts and resolved contradictions by re-reading code. Coordinator corrections are logged in `phase2_log.md`.
4. **Coverage verification.** `scripts/coverage_check.py` joins the manifest, the coverage fragments and every citation in the documents. It produces `coverage_report.md`/`.json` and [99](99_coverage_matrix.md). `scripts/mermaid_check.js` validates every Mermaid diagram with mermaid 11.4.1 in headless Chromium.
5. **Independent review.** A fresh agent sampled [C] claims against the code and reported gaps; its report and the fixes are in `_work/review/`.

The generator and checker scripts are reproducible. Run them with `python3 -I`, passing paths as arguments; the source root is the `multiwinia/` directory of the commit above.
