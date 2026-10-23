---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (owner decision O106)
---

# The steward's growth hold

## Context and Problem Statement

MVP-024's growth ([ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md)) has the Wellspring pay `mvp.essence.orb_cost_ru` (27 ru) for an orb every `mvp.essence.wellspring_interval_s` (12 s), while the settlement is below its population limit and its stores hold that cost. In the opening, every 27 ru gathered goes to the next orb. Nothing on the construction bar (200 to 400 ru) can be paid for until the population reaches its first limit of 40.

The owner's M3 session found nothing to do in that time ([DEC-1010](../decisions/species_design.md#dec-1010-the-openings-choice-between-growing-and-building)). The owner chose a growth hold (O106): a switch on the settlement panel that the steward sets, under which the Wellspring grows nothing and the stores fill.

## Decision Drivers

* The hold is the steward's choice, made through the one command path as every steward's command is (ADR-087, ADR-136), and it is simulation state that saves, replays and hashes.
* A steward who never holds meets the economy that M2's growth scene tuned, unchanged.
* The HUD shows the hold at once, as it shows the priorities, until a status confirms it (ADR-153).
* The owner's M2 world, saved at version 16, continues (O98).

## Considered Options

1. **A settlement flag set by a command.** A hold command, the flag in the region's state, and the status's growth state.
2. **A priority of growth.** A fourth share of the priorities that throttles the Wellspring. It changes MVP-036's three shares, their command and the tuned task search.
3. **A client-side reserve.** The client withholds placements until the stores reach a cost. The server would still spend the stores on growth, so nothing would be held.

## Decision Outcome

Chosen option: 1.
- Option 2 widens a tuned mechanic for a yes-or-no choice.
- Option 3 cannot hold what the server spends.

What is built:

* **The state.** `Settlement::growthHeld` (`GameLogic/Settlement`) is false at founding. `Grow` (`GameLogic/Growth`) returns before the timer's countdown while it is set, so the timer waits where it stands and no orb is paid for.
  - The region writes each settlement's hold as one bit, in the settlements' order, after the claims' ends: a new last section of its state.
  - `WORLD_STATE_VERSION` is 17 (`STATE_VERSION_WITH_GROWTH_HOLD`), and a region of version 16 or older reads with no hold.
* **The command.** `HoldGrowthCommand` (`GameShared/SettlementMessages`, the message type `HoldGrowth`, appended) names the team and whether it holds, in 5 bytes.
  - It is registered with the settlement's commands and permitted to the team's commander, steward or AI.
  - It is applied from the tick it executes in. A hold the settlement already has has no effect.
  - The scripted steward and the AI settlements never send it. `RegionSimulation::AddScriptedSteward` lets go a hold on the settlement it takes, so a scripted settlement always grows.
* **The status.** `GrowthState::Held` (3) is the growth's state before any other cause. It fits the field's two bits, and `SettlementStatus` is version 2.
* **The client.**
  - The settlement verbs send the hold from a press on `HudZone::GrowthHold`, toggling the hold last sent or the status's.
  - The HUD shows the hold sent until a status of its tick holds it, as it shows the priorities. A refused or ineffective hold shows no more.
  - The growth row keeps room at its end for a button at the icon size, drawn on the accent plate while held. Its glyph is `icon.hud.respite`'s pause, until the owner draws a hold icon (provisional under DEC-1005).
  - The row reads "Growth held: the stores fill for building" (`ui.hud.growth`'s `held` case).

### Consequences

* Good, because the opening has a choice from its first minute, and its cost, Workers not grown, shows in the panel.
* Good, because a steward who never holds meets M2's tuned economy unchanged, so the growth run's evidence stands.
* Bad, because a held settlement can stall its own growth for a whole run; the panel says so, and the steward lets it go.
* Bad, because a version raise again changes the state the owner's world holds; it migrates, as every raise since version 10 has (O98).
* It forecloses nothing: a reserve or a growth share can be added beside it later.

### Confirmation

* `GameSharedTests/SettlementMessageTests`: the hold round-trips at its bound, and a status's held growth reads back.
* `GameLogicTests/EssenceTests`:
  - a held settlement with full stores grows no Worker and pays for no orb for three intervals, its timer waiting;
  - let go, it grows when its timer runs out;
  - the hold is saved, and a version-16 region reads with none.
* `GameLogicTests/SettlementCommandTests`:
  - the hold applies, and its status says held before stores short;
  - a second hold has no effect;
  - a scripted steward lets it go.
* `GameClientTests/SettlementVerbsTests` and `SettlementHudTests`:
  - the button sends the hold and then the let-go;
  - the hold shows until a status of its tick, and a refused hold shows no more;
  - the button is a zone over the panel, and is on the accent plate while held.
* The owner's run: the opening has a choice, and the HUD shows it.

## Pros and Cons of the Options

### 1 — a flag by command

* Good, because it is small, saved and deterministic.
* Bad, because it raises the state version.

### 2 — a growth share

* Good, because growth could be throttled, not only stopped.
* Bad, because it reworks MVP-036 and the tuned task search.

### 3 — a client-side reserve

* Good, because nothing in the simulation changes.
* Bad, because it holds nothing.

## More Information

* Owner decisions O98, O106; DEC-1010; MVP-024, MVP-098, MVP-101, MVP-102.
* Amends [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md) (growth), [ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md) (the settlement's commands and status) and [ADR-153](ADR-153-the-settlement-verbs-at-m2.md) (the HUD's presses).
* The code: `GameLogic/Settlement`, `GameLogic/Growth`, `GameLogic/SettlementCommands`, `GameLogic/RegionSimulation`, `GameLogic/WorldSnapshot`, `GameShared/SettlementMessages`, `GameShared/GameMessages`, `GameClient/SettlementVerbs`, `GameClient/SettlementHud`, `GameData/Strings/en_us.json`.
