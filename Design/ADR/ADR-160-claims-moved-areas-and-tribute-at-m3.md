---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M3 task T5, owner decisions O80, O98)
---

# Claims, moved areas and tribute at M3

## Context and Problem Statement

M3's T5 builds claims and tribute (MVP-053, MVP-054, WLD-086).

**Claims** (MVP-053):
- **What may be claimed.** A settlement may claim the half of a valley border area that its neighbour holds, or the valley's central resource site. Only the settlement on the other side of a border may claim a border area (WLD-086).
- **A claim on a held area** is settled by a claim battle, which moves the area if the claimant wins, or by the claimant's withdrawal.
- **The central site while nobody holds it.** A claim on it is held after `mvp.claim.uncontested_minutes` (30) unless another settlement claims it in that time. Then it is contested and settled by a claim battle at the site.
- **What is never claimable:** a settlement's core and home cell (WLD-061).

**Tribute** (MVP-054): a steward eases a grievance held against its settlement by sending stores. Each `mvp.relation.tribute_ru_per_weight` (10 ru) reduces its weight by one. The recipient's temperament sets the rate: Bold standard, Territorial half, Wary twice (MVP-057).

T6 builds the claim battles. T5 gives them a claim to settle and the area to move.

Five things stand in the way:
- **Territory bounds.** M2's territory stops at the halfway line of every valley border pair ([ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md)). WLD-060 bounds it by the core, the home cell, its own border areas and those won by claims.
- **Holding a half.** WLD-063 says a half is held by its own side's settlement from the tick its territory enters it, and by nobody while the site is free. M2 has no notion of holding at all.
- **The central resource site.** It lies outside every territory (WLD-083), and M2's gatherers work only at the resource sites inside their territory ([ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)).
- **Contact after a move.** [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md) reads contact as both territories reaching the halfway line. A moved half shifts the border off that line.
- **The territory event.** WLD-066 also records a claim's move of a border area, which M2's territory events, contact alone, cannot hold.

## Decision Drivers

* Every claim reaches an end: held, won, lost or withdrawn (PIL-122).
* A claim and a tribute are commands of the one path, the steward's and an AI's alike (MVP-056, MVP-094).
* A won half becomes its winner's territory, and its resource sites with it (WLD-086).
* A world of version 10 or 11 continues (O98, MVP-095).

## Considered Options

1. **Claims and moved areas as region state**, with a half's holder derived from the territory and the moved halves, and territory bounds that read the moved halves.
2. **Holders stored for every half**, written whenever a territory changes.
3. **Claims as settlement state.**

## Decision Outcome

Chosen option: 1.
- **Option 2** stores what the territory already says, and must be kept in step with every growth.
- **Option 3** splits a claim that two settlements share, the claimant's and the holder's, across two records.

* **The areas** (`GameShared/RelationMessages`):
  - A half of a valley border area is its pair's index in `VALLEY_PAIRS` times two plus its side. That is eight in a region.
  - The central resource site is area 8 (`CLAIM_AREA_CENTRAL`).
  - A half is the part of its pair's lens that is no nearer the other site than its own, inside the coast bands (WLD-062, `InBorderHalf`).
* **Holding** (`GameLogic/Claims`):
  - The region holds the areas claims have moved (`AreaHolding`), in area order.
  - A half's holder (`HolderOf`) is the settlement a claim moved it to. Otherwise it is its own side's settlement once that settlement's territory holds a cell in it.
  - The central resource site is held only by a claim.
  - A half won back by its own side's settlement has no holding.
* **Territory bounds** (`WithinTerritoryBounds`): a half that a claim moved to a settlement is within its bounds, and is no longer within its own side's. The territories of both sites are made again when a half moves.
* **Claims** (`Claim`, in the order made) have three states:
  - **Open**: on a held area. The holder's Cladan gains `mvp.grievance.claim_weight` against the claimant (MVP-051, [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md)), with the claim's stable id as the grievance's subject.
  - **Waiting**: on the central resource site while nobody holds it. It holds the site `mvp.claim.uncontested_minutes` after it was made (`AdvanceClaims`, in the settlements' step after the relations).
  - **Contested**: a second claim on that site while nobody holds it contests every claim on it, and no wait holds it.
* **Refusals.** A claim is refused, in this order:
  - by a team that is no settlement's;
  - on a half of a pair the claimant is not in, or an area beyond the region's;
  - on a half its neighbour does not hold;
  - on an area the claimant holds;
  - on an area the claimant already claims.
* **The end of a claim** always removes its grievance's cause (PIL-122).
  - **A withdrawal** by its claimant ends it.
  - **A claim battle** (T6, `SettleClaim`) ends it. A won battle moves the half and records the move as a territory event (`ContactEventKind::AreaMoved`, with the half). A won battle on the central site ends every claim on it.
  - **A lone rival.** A contested claim left alone by a withdrawal or a lost battle waits again, from when it was made.
* **Contact** ([ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md) amended) is now two valley neighbours whose territories meet: a cell of one and a cell of the other share a side or a corner (`TerritoriesMeet`). Only the lens of their sites' discs is searched. Before any move this is the halfway line. After one it is the moved border.
* **The central resource site's holder** gathers there: its gatherers' port search takes the site's ports, and it counts among the settlement's held resource sites (MVP-064).
* **Tribute** (`SendTribute`):
  - **The grievance.** It names a grievance held against the sender by its holder, cause and subject. It is refused for a team that is no settlement's, for no such grievance, for an amount of nothing, and for an amount beyond the stores.
  - **The rate.** Each `tribute_ru_per_weight` eases a unit at the recipient's rate (MVP-057). The steward's settlement, which has no temperament, eases at the standard rate.
  - **What is paid.** No more than eases the grievance whole. The recipient keeps what its stores have room for.
  - **What is not built.** Tribute is not withdrawn from a grievance whose cause remains, and truce and alliance are not built (MVP-054).
* **The commands** (`MakeClaimCommand`, `WithdrawClaimCommand`, `SendTributeCommand`, message types 0x0118 to 0x011A) are registered with the settlement's ([ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md)). Each is permitted for the team's commander, and its refusal is its outcome's reason (`ClaimRefusal`, `TributeRefusal`).
* **The saved state.** `WORLD_STATE_VERSION` is 12.
  - Each region's claims and moved areas follow its grievances.
  - A territory event's kind takes a bit, and a move names its half.
  - A region of version 11 or 10 reads with no claim and no moved area, its events contacts. The empty world's golden hash of DBG-147 moves with the two counts, checked to be the only change.
* **What waits.** The steward's verbs (T12), the panel and the markers (T11), and the AI's claims (T7) are later tasks. A border area that returns to its side when its settlement leaves the map is B5's: no MVP settlement leaves.

### Consequences

* Good, because a claim's every path ends, and removes its grievance's cause when it does.
* Good, because the territory follows a moved half with no state beyond the move itself.
* Good, because the claims and tribute are the same commands for the steward and the AI.
* Bad, because a half's holder is computed from the territory raster on each check, a scan of its held cells. That is affordable for commands and the claims' step, but not for every tick.
* Bad, because contact now searches the lens's cells once a second for each of the four pairs, where the halfway test walked one line.
* Bad, because a contested claim's wait restarts from its making, so a claim contested for longer than the wait is held at once when its rival withdraws.
* It forecloses: claims on a home cell or a core; a half claimed by a settlement of another pair.

### Confirmation

GameLogicTests `ClaimTests`:
- **Who may claim a half.** It is not held, and not claimable, at the founding radius. Once its territory enters it, it is held, and only its neighbour may claim it: not site 0, not its holder, not an area beyond the region's. The claim is Open, gives the holder the claim's weight against the claimant, and is made once. Another settlement cannot withdraw it; its claimant can, which removes the cause.
- **Claim battles over a half.** A lost battle moves nothing. A won one moves the half: the winner's territory grows into it, the loser's leaves it, the move is the territory event, and the two stay in contact. The loser claims it back, and winning returns it to its own side.
- **The central site uncontested.** A claim waits with no grievance and holds the site after its wait, not before. The holder counts one more resource site, and another's claim on the site is Open against it.
- **The central site contested.** Two claims are contested, and no wait holds the site. A withdrawal leaves the other waiting, and it then holds the site. A lost battle ends its claim; a won one ends every claim on the site.
- **Tribute.** It eases at Bold's, Territorial's and Wary's rates, the recipient keeping it. No more is paid than eases a grievance whole. Each refusal comes in its order.
- **The commands** carry their refusals as their outcomes' reasons. Another settlement's claim is not the steward's to make.
- **Saving.** The snapshot holds the claims, the moved half and its event, and the restored territory keeps the half. A region of version 11's layout reads with none.

GameSharedTests `RelationMessageTests`: the commands round-trip at their bounds, and an area beyond the region's, claim 0 and a negative tribute are malformed. `StateHashTests` holds DBG-147's new golden values. The whole GameLogic and GameShared suites pass.

## Pros and Cons of the Options

### 1 — region claims and moved areas, holders derived

* Good, because the state is the decisions alone: who claimed what, and what moved.
* Bad, because a holder is computed when asked.

### 2 — every half's holder stored

* Good, because a holder is read, not computed.
* Bad, because every territory change must write it.

### 3 — claims in the settlement's record

* Good, because the claimant's record holds its claims.
* Bad, because the holder's side of a claim lives elsewhere, and the settlement record's layout changes.

## More Information

* Amends:
  - [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md): the territory's bounds with moved halves, and the territory event of a move;
  - [ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md): the relations' commands;
  - [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md): state version 12;
  - [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md): contact as territories meeting.
* Related: [ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md) (gathering).
* Owner decisions O80, O98; MVP-051, MVP-053, MVP-054, MVP-056, MVP-057, MVP-064, MVP-094, MVP-095, MVP-101; PIL-064, PIL-121, PIL-122; WLD-060 to WLD-063, WLD-066, WLD-083, WLD-086; DBG-147.
* The code:
  - `GameShared/RelationMessages`, `GameMessages`;
  - `GameLogic/Claims`;
  - `Territory` (`InBorderHalf`, `TerritoriesMeet`, the bounds and the events);
  - `Relations` (`InContact`), `SettlementCommands`, `SettlementTasks`, `Statistics`, `RegionSimulation`, `WorldSnapshot`;
  - `GameLogicTests/ClaimTests`, `RelationTests`, `StateHashTests`; `GameSharedTests/RelationMessageTests`.
* **Amendment 2026-10-22 (M3 task T6, [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md)).** A claim battle's end settles its claim (`SettleClaim`); an attack is declared for an Open or a Contested claim, never a Waiting one, on the area's holder or the rival claimant.
* **Amendment 2026-10-22 (M3 task T8, [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md)).** Each claim made, withdrawn, lost or won (a central site held after its wait is won) is recorded in its claimant's statistics. The weight a tribute removes counts towards its sender's renown, as does a withdrawn claim's grievance once the claim has stood `mvp.claim.uncontested_minutes` (DEC-1007).
* **Amendment 2026-10-23 (O102, [DEC-1008](../decisions/species_design.md#dec-1008-the-valley-runs-misses-of-playtest-1s-lines) option A).** Claims are spaced on each area. A claim's end, by a withdrawal, a claim battle or the central site's wait, records its area's tick (`ClaimEnd`, one per area in the order of their areas), and `CheckClaim` refuses a claim on that area until `mvp.claim.cooldown_minutes` have passed (`CooldownEndOf`), after every other check, with the reason `ClaimRefusal::CoolingDown` (7). The ticks are region state after the renown: `WORLD_STATE_VERSION` is 16, and a region of version 15 or earlier reads with none, so no area cools down. The value is 120 minutes, from the valley run's sweep (ADR-164's amendment); the tests of claims alone run on a tuning with no cooldown.
