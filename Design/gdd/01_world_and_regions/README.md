# World and regions

> Status: Draft · Prefix: WLD · Area: A1 · Rationale: `../../rationale/A1.md` · Related: [GDD index](../README.md) · [adoption ledger 08](../adoption_ledger/08_maps_and_level_design_ledger.md) · [vision, pillars and core loops](../00_vision_pillars_and_core_loops.md)

The Species world game's world: its shape and size, its regions and settlement sites, its terrain, its time and lifetime, its two views and the scale figures it hands to the server. The design is one document in three files, which share the prefix `WLD`, one tuning namespace (`wld.*`) and one numbering of acceptance criteria; every requirement id is defined in exactly one file.

| File | What it specifies | Requirements | Acceptance criteria |
|---|---|---|---|
| [world_and_sites.md](world_and_sites.md) | The world's extent, size and capacity, its world constants and its smallest form per stage; regions, sea channels and crossings; settlement sites; territory, border areas and contact; resource distribution; travel and the attack gate; native and stand-in AI settlements; founding, returns and late joiners | WLD-001 to WLD-010, WLD-020 to WLD-024, WLD-040 to WLD-044, WLD-060 to WLD-066, WLD-080 to WLD-086, WLD-100 to WLD-104, WLD-120 to WLD-127, WLD-140 to WLD-144 | AC-01 to AC-17 |
| [terrain_time_and_views.md](terrain_time_and_views.md) | The hybrid world generator and its validation; biomes, lighting and weather; the world calendar; the world lifetime; the strategic view and the settlement view; the scale figures, the battle load and the battle caps | WLD-160 to WLD-167, WLD-180 to WLD-185, WLD-200 to WLD-203, WLD-220 to WLD-224, WLD-240 to WLD-246, WLD-280 to WLD-285 | AC-18 to AC-36 |
| [skirmish_maps_and_map_tools.md](skirmish_maps_and_map_tools.md) | The skirmish map rules that A1's decisions change; the map tools; the generator variants, among them the feathered flatten type; the adoption of reference/08; A1's eight Open decisions | WLD-260 to WLD-270 | AC-37 to AC-46 |

*Table 1 — The files of world and regions.*

**The model.** Every figure of the three files comes from `Tools/DesignModels/A1WorldDensity.py`; `python3 -I Tools/DesignModels/A1WorldDensity.py check` reproduces each of them and every table of figures (WLD-283). Its modes `report`, `lifetime`, `battles`, `sweep`, `founding`, `tables` and `layout` print the figures by topic.

**Reading order.** [World and sites](world_and_sites.md) first: the other two files build on its geometry. A reader who sizes the server reads §2.6 of [terrain, time and views](terrain_time_and_views.md); a reader of skirmish maps reads [skirmish maps and map tools](skirmish_maps_and_map_tools.md) alone.
