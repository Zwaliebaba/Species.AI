# Maps and level design

> Status: Reference · Prefix: R-MAP · Related: [01 Skirmish rules and modes](../01_skirmish_rules_and_modes/README.md) · [03 Buildings and world objects](../03_buildings_and_world_objects/README.md) · [04 Supply crates, powers and weapons](../04_supply_crates_powers_and_weapons/README.md) · [05 Population flow](../05_population_flow.md) · [07 Bots](../07_bots/README.md) · [09 Front end and match flow](../09_front_end_and_match_flow/README.md) · [12 Simulation model](../12_simulation_model/README.md)

This folder specifies what a map is in the reference game: how the game finds its maps and lists them, how a map loads into a match, what a map definition can express, the per-map match options, the conventions that each mode's maps follow, and the distribution of the reference map set over modes and player counts. It also records the hooks of the map editor. Every Species map is authored new: this folder carries no reference map name, layout or terrain (DEC-063), and the per-map data of the reference maps appears only as anonymous counts, ranges and distributions per mode and player count.

## Files

| File | Content | Requirement ids |
|---|---|---|
| [README.md](README.md) | this index; the reference map set at a glance; reading guide | none |
| [rules_and_conventions.md](rules_and_conventions.md) | map identity and the map catalogue; the map definition and the level load; start positions; the match options of the map header (the map options and map effects); level-design capabilities (terrain, lights, structure placement, routes, camera views, starting forces); fixed research levels; what a map cannot express; per-mode map conventions; the reference map distribution and the per-map parameters as anonymous distributions | R-MAP ids 001 to 299 |
| [map_editor_hooks.md](map_editor_hooks.md) | the map editor's hooks: what an editor edits, its entry and exit flow, custom-map handling and the writer rules (DEC-045) | R-MAP ids 300 to 499 |

## The reference map set at a glance

The reference map set has 49 match maps. Each is listed under exactly one mode and declares 2, 3 or 4 player positions. One map definition serves its player count; at load each start position is remapped to a match team (DEC-025, DEC-560), and the host may close slots before the match (DEC-079).

| Mode | 2 players | 3 players | 4 players | Maps |
|---|---|---|---|---|
| Spawn Control | 5 | 2 | 2 | 9 |
| Zone Control | 2 | 4 | 3 | 9 |
| Relic Run | 4 | 2 | 3 | 9 |
| Siege | 4 | 2 | 2 | 8 |
| Launch Race | 3 | 3 | 1 | 7 |
| Flag Chain | 4 | 2 | 1 | 7 |
| **All modes** | **22** | **15** | **12** | **49** |

The Species map slots that recreate this distribution are named `map_<mode>_<players>p_<nn>` (CONVENTIONS §8.11) and are listed in the asset manifest as `mapfile.spawn_control`, `mapfile.zone_control`, `mapfile.relic_run`, `mapfile.siege`, `mapfile.launch_race` and `mapfile.flag_chain`, each with its thumbnail `thumb.<mode>`. The content build renders every thumbnail with the engine and generates the map index, the list of map ids that the achievement hooks count (DEC-563). The rules that create this list and order it, including the tie order of DEC-314, are in [rules_and_conventions.md](rules_and_conventions.md) §2.2.

Maps that declare no mode, such as development, test and benchmark maps, are never listed in match setup (DEC-802); the reference's own maps of that kind are not specified (DEC-334).

## Reading guide

- **Map authors** start with [rules_and_conventions.md](rules_and_conventions.md) §2.4 (the map definition), §2.8 to §2.14 (what a map can express) and §2.16 to §2.21 (the conventions and the validation rules of each mode).
- **Mode and economy designers** read §2.6 and §2.7 (the match options that a map sets, and how map values meet lobby defaults), then the mode documents of [reference/01](../01_skirmish_rules_and_modes/README.md) and the population rules of [reference/05](../05_population_flow.md).
- **Tool builders** read [map_editor_hooks.md](map_editor_hooks.md) and the schema family of the map definition in [data schemas](../../architecture/components/data_schemas/README.md).
- **Terrain and rendering** use §2.8 to §2.10 for the authored inputs; the generator algorithm is [terrain](../../architecture/components/terrain.md) (DEC-025, DEC-246, DEC-247) and the lights and sea are [renderer](../../architecture/components/renderer/README.md).

## Decisions that shape this folder

| DEC | Status | Effect on this folder |
|---|---|---|
| DEC-025 | Resolved | one map definition serves 2 to 4 players; the capability set and the terrain-generator look are kept |
| DEC-045 | Open | the map editor is recorded as hooks only ([map_editor_hooks.md](map_editor_hooks.md)) |
| DEC-063 | Dropped | no reference map shape, terrain or identity is reproduced |
| DEC-314 | Open | the order of tied entries in the map lists |
| DEC-334 | Dropped | the reference's development, test and benchmark maps |
| DEC-563 | Resolved | thumbnails and the map index are content-build outputs |

The full list of decisions that the rules cite is in [rules_and_conventions.md](rules_and_conventions.md) §8.
