# Reference mechanics

> Status: Reference · Index of the reference documents · Related: [CONVENTIONS](../CONVENTIONS.md) · [Requirements index](../REQUIREMENTS_INDEX.md) · [Decision register](../decisions/README.md) · [Engine components](../architecture/components/README.md)

This folder specifies the reference mechanics of a Species skirmish match: the rules, numbers, states and behaviour that the Species look and feel is built on. The documents are engine-agnostic. They say what a mechanic does, never how an engine implements it; the engine baseline lives in [architecture/components](../architecture/components/README.md).

These documents are a reference, not the Species game design. The Species design phase adopts, adapts or drops each requirement by id. Each document follows the reference template of [CONVENTIONS](../CONVENTIONS.md) §7: purpose and player experience, rules, states and flows, tuning table, edge cases, acceptance criteria, dependencies and open decisions.

## Documents

| Document | Prefix | Scope |
|---|---|---|
| [00 Reference overview](00_reference_overview.md) | `R-VIS` | the core loop, what makes the reference distinctive, the player-facing vocabulary |
| [01 Skirmish rules and modes](01_skirmish_rules_and_modes/README.md) | `R-MODE` | the shared match skeleton and the six modes |
| [02 Units](02_units/README.md) | `R-UNIT` | the Worker, Leader, Essence orb, Strike team, Transport, Collector, Builder, creatures, hazards and projectiles |
| [03 Buildings and world objects](03_buildings_and_world_objects/README.md) | `R-BLDG` | economy and spawn objects, mode objects, defences, props, trees and crate structures |
| [04 Supply crates, powers and weapons](04_supply_crates_powers_and_weapons/README.md) | `R-CRATE` | Supply crates, their outcomes, the Powers and the weapons |
| [05 Population flow](05_population_flow.md) | `R-ECON` | spawning, caps, Essence, reinforcements and throughput |
| [06 Controls, camera and commands](06_controls_camera_and_commands/README.md) | `R-CTRL` | actions and bindings, selection, power slots, cursor and markers, the player camera |
| [07 Bots](07_bots/README.md) | `R-BOT` | the bot architecture, navigation, decision loop, difficulty and per-mode strategies |
| [08 Maps and level design](08_maps_and_level_design/README.md) | `R-MAP` | map rules and conventions, the in-scope maps, the map editor hooks |
| [09 Front end and match flow](09_front_end_and_match_flow/README.md) | `R-META` | screen flow, match setup, front-end presentation, options and localisation |
| [10 Audio design](10_audio_design/README.md) | `R-AUD` | the sound event model, sound data, music and ambience |
| [11 Tutorial and help](11_tutorial_and_help.md) | `R-TUT` | the tutorials, hints and help pages |
| [12 Simulation model](12_simulation_model/README.md) | `R-SIM` | tick timing, match lifecycle, entity model, update order, space and routing, the command catalogue, randomness and integrity |

## Reading order

Start with [00](00_reference_overview.md) for the loop and the vocabulary, then [12](12_simulation_model/README.md) for the simulation contracts every other document relies on (tick, identity, update order, randomness). Read [01](01_skirmish_rules_and_modes/README.md), [02](02_units/README.md), [03](03_buildings_and_world_objects/README.md), [04](04_supply_crates_powers_and_weapons/README.md) and [05](05_population_flow.md) for the match itself, and [06](06_controls_camera_and_commands/README.md) to [11](11_tutorial_and_help.md) for control, bots, maps, front end, audio and onboarding.
