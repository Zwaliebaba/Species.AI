---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M3 task T2, owner decisions O80, O98)
---

# The four settlements and their peace at M3

## Context and Problem Statement

M3's T2 founds the three AI settlements beside the steward's (MVP-013, MVP-056).
- **Temperaments.** Each AI settlement's Cladan carries one of MVP-057's three temperaments, and no two AI settlements of a world share one.
- **Their economy** is M2's scripted steward ([ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md)), with the temperaments as its parameters (O98).

Three things stand in the way:
- **The world start founds one settlement.** `WorldStart` founds the steward's settlement alone ([ADR-130](ADR-130-settlements-structures-and-resource-sites-at-m2.md)).
- **Every other team is an enemy.** The regions' teams treat every other team as one (`RegionSimulation::EnemiesOf`, R-SIM-311). Four settlements' Workers would therefore fight wherever they met, and their turrets would fire at each other's gatherers, with no battle declared. PIL-041 says a battle starts only when hostile forces meet: an expedition entering a territory, or two hostile expeditions meeting.
- **M2's runs must keep one settlement.** The owner's M2 growth run plays the game's new world with no neighbours (MVP-002), while M3's phase A starts before M2's exit (O98).

## Decision Drivers

* A world's AI settlements and their temperaments follow from its start and key, and replay from its journal.
* Settlements never fight outside a battle, and the sandbox's teams still fight each other.
* The game's own new world keeps one settlement until M3's phase B.

## Considered Options

1. **A count of AI settlements in the world start**, founded by `StartWorld`, with the temperaments dealt by a keyed draw. Settlement teams are at peace with each other.
2. **AI settlements always founded with the steward's.** The growth scene and M2's runs would gain neighbours.
3. **Temperaments fixed by site**: each site always deals the same temperament.
4. **Peace by a figure-level filter**, in place of the teams' enemy masks.

## Decision Outcome

Chosen option: 1.
- **Option 2** changes M2's evidence: the growth scene measures a settlement with no neighbours.
- **Option 3** would make a world's temperaments predictable from its layout alone. MVP-013 asks only that they differ.
- **Option 4** costs a test inside every grid query, where a team mask costs nothing. The battles of T6 make their sides hostile as whole teams.

* **The world start** (`GameLogic/CommandJournal`):
  - **The count.** `WorldStart::aiSettlements` gives how many AI settlements are founded, at most three.
  - **Founding.** `StartWorld` founds them after the steward's settlement, at the free sites in site order.
  - **The deal.** The three temperaments are dealt among them by a Fisher–Yates shuffle, on one keyed draw (`DrawPurpose::TemperamentDeal`, subject 1) of the region's first tick.
  - **Their economy.** Each AI settlement is run by a scripted steward as its AI commander. `RegionSimulation::FoundSettlement` takes the temperament.
* **The journal** is format 6, which holds the count. A journal of format 5 reads as one with no AI settlement (MVP-095).
* **The servers.** `WorldServer::Desc` and `IntegratedServer::Desc` carry the count. Its default, 0, keeps the client's new world one settlement until phase B turns it on.
  - The growth scene and the frame bench's settlement keep one settlement.
  - M3's valley run (T9) founds three AI settlements.
* **Peace** (`RegionSimulation::EnemiesOf`, now a member):
  - **Between settlements.** A settlement's team has every team for an enemy except its own and every other settlement's.
  - **Wild teams.** A team that is no settlement's, as the sandbox's are, has every other team for one, and is every team's.
  - **Battles.** T6 adds a battle's sides.
  - **The turret.** Its ownership rule reads the same enemies, so it needs the region (`OwnershipOf`).
* **No state version raise.** The Cladan's temperament has been region state since version 9, so T2 adds none. A world saved by M2 continues with its one settlement.

### Consequences

* Good, because a world's four settlements and their temperaments follow from its start alone, and replay.
* Good, because the settlements' Workers and turrets fight only in a battle, as PIL-041 has it.
* Good, because M2's evidence keeps its one settlement.
* Bad, because the client's world has no neighbours until phase B sets the count.
* Bad, because the enemy test now walks the region's settlements, at most four, wherever the cached enemy query misses.
* It forecloses: settlements that fight without a battle; temperaments that two AI settlements share.

### Confirmation

GameLogicTests (`ValleyTests`):
- a world founds three AI settlements, each with its own temperament and its scripted steward, and the deal varies with the key;
- settlements are at peace: none is another's enemy, their Workers standing together do each other no harm, and a wild team among them is fought;
- the four settlements work side by side with the same hashes at 0, 1, 2 and 8 worker threads, and each gathers;
- a journal of format 6 keeps its AI settlements and replays them, and a world of the M2 form restores with its one settlement.

The whole GameLogic suite passes unchanged, the turret tests against a wild team among them.

## Pros and Cons of the Options

### 1 — a count in the start, a keyed deal, peace between settlements

* Good, because each world's deal differs and replays.
* Bad, because the start's format changes again.

### 2 — AI settlements always

* Good, because there is no count to carry.
* Bad, because M2's growth scene loses its lone settlement.

### 3 — temperaments by site

* Good, because there is no draw.
* Bad, because every world with its steward at one site deals alike.

### 4 — a figure-level filter

* Good, because one rule covers expeditions too.
* Bad, because every enemy query pays for it.

## More Information

* Amends [ADR-130](ADR-130-settlements-structures-and-resource-sites-at-m2.md) (the AI settlements founded with the world), [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md) (journal format 6) and [ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md) (the scripted steward runs the AI settlements).
* Owner decisions O80, O98; MVP-013, MVP-056, MVP-057, MVP-095; PIL-041; R-SIM-311.
* The code:
  - `GameLogic/CommandJournal` (`WorldStart::aiSettlements`, `FoundAiSettlements`, format 6);
  - `RegionSimulation` (`FoundSettlement`'s temperament, `EnemiesOf`);
  - `DrawPurpose` (`TemperamentDeal`);
  - `Turret` (`OwnershipOf`), `Combat`, `Weapons`, `AiController`;
  - `WorldServer`, `IntegratedServer`;
  - `GameLogicTests/ValleyTests`.
* **Amendment 2026-10-22 (M3 task T6, [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md)).** A settlement's expedition is a team of its own, at peace as its settlement is, and a battle in progress makes its expedition's team and its defender's team enemies of each other and of no other team. The actor grid's cache forgets its queries when a battle starts or ends.
