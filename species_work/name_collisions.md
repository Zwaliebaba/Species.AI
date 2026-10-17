# Name collision screen (F1)

Every Species name in `WORK/rename_map.tsv` was checked against three lists:

1. the studio's other titles: *Uplink*, *Darwinia*, *DEFCON*, *Multiwinia*, *Subversion*, *Prison Architect*, *Scanner Sombre*;
2. well-known game and franchise trademarks and coined terms;
3. the other Species names (one name must name one thing, and must not reuse an original term).

The ledger already uses the **proposed** replacement for every hit below, marked "pending owner batch 1". If the owner keeps a 90 §3 name, the coordinator changes the ledger row back (and the matching row of `NEXT/CONVENTIONS.md` §8).

Severity: **High** = must be resolved before the rewrite (named in the brief, or a studio title). **Medium** = should be resolved: a distinctive franchise term, or an internal collision that will confuse documents. **Low** = information only; no change recommended, or the content is dropped.

## High

| # | 90 §3 name | Collides with | Proposed replacement | Reason (one line) |
|---|---|---|---|---|
| H1 | **Uplink Console** (control station) | the studio's game *Uplink* | **Command Console** (`CommandConsole`, `building.command_console`); alternative *Arming Console* | A capturable console that commands a linked object; "Command" carries no title echo. Its capitalised form keeps it apart from "player command" in reference/12. |
| H2 | **Subversion** turret type (GunTurret type 4) | the studio's game *Subversion*, and the original outcome name already renamed to *Convert* | **Convert Turret** (`TurretType::Convert`, `building.turret.convert`) | The turret fires the converting laser of the Convert power, so it follows the Convert rename. |

## Medium

| # | 90 §3 name | Collides with | Proposed replacement | Reason (one line) |
|---|---|---|---|---|
| M1 | **Warp Gate** (trunk port) | *StarCraft II*'s "Warp Gate", a distinctive Blizzard term for a unit-delivery building | **Transit Gate** (`TransitGate`, `building.transit_gate`) | Reinforcement waves pass through it; "transit" says that without a franchise echo. |
| M2 | **Relay Dish** (radar dish) | the Species **Relay** (spawn link): one word for two unrelated objects | **Beam Dish** (`BeamDish`, `building.beam_dish`) | It carries units along a beam to a receiving dish; "Relay" stays with the spawn network. |
| M3 | **Energy Collector** (solar panel) | the Species **Collector** (harvester unit) | **Solar Array** (`SolarArray`, `building.solar_array`) | Keeps "Collector" for the unit; the object is a capturable solar power source. |
| M4 | **Power** (task, power-up) and the **power network** (surges, pylons, splitter) | each other: "power" names both the supply-crate ability and the surge network of Launch Race and the Barrier Emitters | rename the network to the **energy grid**: energy surge, grid pylon (`GridPylon`), grid terminal (`GridTerminal`), grid splitter (`GridSplitter`); keep **Power** for the crate ability | "Power slot" and "power surge" would otherwise be ambiguous in the same documents. |
| M5 | **Spirit Grove** (Magical Forest) | the original's own word "spirit", which the ledger replaces with *Essence orb* | **Essence Grove** (`CrateOutcome::EssenceGrove`, `crate.essence_grove`) | Its trees drop essence orbs, so the name follows the Essence orb rename. |
| M6 | **Relic Base** for both the "drop zone" and the "statue spawn zone" (90 §2 row) | itself: two different zones of Relic Run under one name | keep **Relic Base** for the delivery zone; add **Relic Spawn Zone** (`RelicSpawnZone`, `building.relic_spawn_zone`) | One name must name one thing; the relic appears in one zone and is delivered to another. |

## Low (information; no owner question needed)

| # | Species name | Collides with | Proposal | Reason (one line) |
|---|---|---|---|---|
| L1 | **Armour Duel** (Tank Battle, debug-only, dropped) | the original unit name *Armour*, renamed to Transport | **Vehicle Duel** (already in the ledger) | Avoids reintroducing a renamed original term in Dropped DEC entries. |
| L2 | **Lightning War** (achievement for the Flag Chain mode) | a literal English rendering of the original mode name *Blitzkrieg* | **Flag Sweep** (already in the ledger) | Ties the achievement to the Species mode name instead of the original one. |
| L3 | **Relic Run** (mode) | *Lara Croft: Relic Run* (2015 mobile game title) | keep | The distinctive part of that title is "Lara Croft"; "relic run" as a mode name is descriptive. Alternative if zero overlap is wanted: *Relic Carry*. |
| L4 | **Hatchery** (incubator) | *StarCraft*'s Zerg "Hatchery" | keep (content dropped) | The incubator is placed only by campaign and developer maps, so the name appears only in Dropped entries. |
| L5 | **Rift** / **Rift Horror** (portal, debug-only) | the MMO *RIFT* and the *Oculus Rift* headset | keep (content dropped) | Debug-only outcome; the name appears only in Dropped entries. |
| L6 | **Spitter Plant** | the *Left 4 Dead* special infected "the Spitter" | keep | A compound of two generic words; no franchise identity is copied. |
| L7 | **Reaper Serpent** | "Reaper" in several franchises (*Mass Effect*, *Diablo III: Reaper of Souls*, *Overwatch*) | keep | A generic word in a new compound. |
| L8 | **Brood** (Eggs outcome) and **Brood Drifter** | *StarCraft: Brood War* uses "Brood" in a title | keep | Generic word; both Species uses are about eggs that hatch Crawlers, so they are consistent. |
| L9 | **Breach Charge** | a generic military term also used as a gadget name in *Rainbow Six Siege* | keep | Generic military term. |
| L10 | **Zone Control** (mode) and **Control Zone** (object) | each other | keep | Intentional pairing: the Zone Control mode is played on Control Zones. |
| L11 | **Fury** (Rage) and **Frenzy** (Mega Rage) | each other | keep | Intentional pair of related outcomes. |
| L12 | **Transport**, **Power**, **Shield** | ordinary technical words ("network transport", "GPU power") | keep | Written capitalised as Species terms; networking is deferred to the design phase. |
| L13 | Game title **Species** (run parameter) | the *Species* film franchise (1995) and the game *Species: Artificial Life, Real Evolution* (2018) | none: owner parameter | Recorded for the owner's information only; this run does not change the game name. |

No Species name matches *Darwinia*, *Multiwinia*, *DEFCON*, *Prison Architect* or *Scanner Sombre*.

## Owner questions (batch 1, at most four)

Recommended options first. The coordinator may merge them as it sees fit.

1. **Control station name** (H1): Command Console (recommended) · Arming Console · keep Uplink Console.
2. **Convert Turret** (H2): Convert Turret (recommended) · Converter Turret.
3. **Medium renames M1–M3, M5** (one multi-select question): accept Transit Gate, Beam Dish, Solar Array, Essence Grove (recommended) · keep the 90 names.
4. **Energy grid and Relic Spawn Zone** (M4, M6): accept both (recommended) · keep "power network" and one shared Relic Base.

## Names needed that 90 §3 did not supply

The ledger adds these, with the same selection rules (role-based, generic where possible). They need no owner decision unless the coordinator wants one:

- turret types: gun turret, rocket turret, mortar turret, flame turret, Convert Turret (`TurretType::*`);
- fuel pump (fuel generator), fuel line (fuel pipe), exclusion zone (restriction zone), grid pylon, grid terminal, grid splitter;
- Relic Spawn Zone; Home post (split out of the "Flag post / Home post" row);
- Species identifiers for every in-scope class of the reference entity model (for example `SimObject`, `Actor`, `Structure`, `ActorGroup`, `MatchWorld`, `MatchTeam`, `TickClock`), because no Species identifier may equal an original identifier;
- kind-suffixed identifiers for kept generic words (`TreeObject`, `WallObject`, `JumpPadObject`, `SpiderCreature`, `CentipedeCreature`, `AntCreature`).
