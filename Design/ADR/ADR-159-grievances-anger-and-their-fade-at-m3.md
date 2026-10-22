---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M3 task T4, owner decisions O80, O98)
---

# Grievances, anger and their fade at M3

## Context and Problem Statement

M3's T4 builds the relations between settlements that stand in for B5 (MVP-050 to MVP-052):
- **Grievances and anger.** Each Cladan holds grievances against settlements. Its anger against a settlement is the sum of their weights (MVP-050, PIL-064).
- **Three causes** (MVP-051, PIL-124):
  - territory pressure, `mvp.grievance.pressure_per_hour` (30) of weight an hour each way while two settlements are in contact (MVP-027);
  - a claim on an area the settlement holds, `mvp.grievance.claim_weight` (30);
  - an attack declared on it, `mvp.grievance.attack_weight` (40), `mvp.grievance.unprovoked_factor` (2) times as much unprovoked.
- **The fade.** Every grievance loses `mvp.grievance.fade_share_per_hour` (10 %) of its weight an hour, `pil.conflict.settled_fade_factor` (4) times faster once its cause is removed (MVP-052, PIL-122).
- **Battle relief.** A battle result removes `mvp.grievance.battle_relief_won` (50 %) or `mvp.grievance.battle_relief_lost` (25 %) of the attacker's grievances against the target (MVP-052).

The claims are T5's, and the attacks and battles T6's. T4 gives them the grievances they raise, remove and relieve.

Five questions are open:
- **The numbers.** The simulation's quantities are integers where they can be (R16), as the stores are in thousandths of a resource unit ([ADR-130](ADR-130-settlements-structures-and-resource-sites-at-m2.md)). A tenth of a weight unit an hour is about 3 millionths a tick.
- **What "10 % an hour" means.** It can mean 10 % of the weight left each hour, or 10 % of the first weight each hour.
- **How a grievance ends.** PIL-122 names the faded grievance as one of a conflict's end states, but a proportional fade never reaches zero.
- **Contact.** The region records a contact event once for each pair (WLD-066, [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md)), and a territory never shrinks in M2. Territory pressure needs to know whether the pair is in contact now, since T5's claims move border areas.
- **The saved world.** O98 has every state raise from M3's first migrate version 10, so the owner's M2 world continues.

## Decision Drivers

* A grievance shows its cause, its weight and how it fades (PIL-064), so its state names all three.
* The relations replay exactly and cost nothing that ADR-080's budget notices.
* Every conflict reaches an end state (PIL-122).
* A world of version 10 continues (O98, MVP-095).

## Considered Options

1. **Region-level grievances in integer millionths, stepped once a second**, with the fade as a share of the weight left.
2. **Grievances stepped every tick.**
3. **Grievances as binary64 weights.**
4. **Grievances inside each settlement's record.**

## Decision Outcome

Chosen option: 1.
- **Option 2** rounds each tick's fade of a few millionths to the nearest, an error of up to a sixth of the fade at a small weight.
- **Option 3** breaks R16's rule for a quantity that integers hold well.
- **Option 4** changes the settlement record's layout, which every later raise would have to read in two forms.

* **The grievance** (`GameLogic/Relations`, `Grievance`) names:
  - the site of the settlement whose Cladan holds it, and the site of the settlement it is held against;
  - its cause, and what the cause names: 0 for territory pressure, else the claim or the attack, never 0;
  - whether it was an unprovoked attack (PIL-063);
  - whether its cause is removed;
  - its weight in integer millionths of a weight unit.
* **Where they are held.** The region holds its grievances in the order of holder, against, cause and subject, with one for each. Anger is the sum over a holder and a target (`AngerMicro`).
* **Contact** (`InContact`) is MVP-027's test now: the two are valley neighbours (WLD-062), and both their territories reach their pair's halfway line. It is not the recorded event.
* **The relations' step** (`AdvanceRelations`) runs in the settlements' step after the contact events, at each tick that starts a second of world time (every `round(1 / ref.sim.tick_period)` ticks).
  - **Pressure.** Each pair of valley neighbours in contact adds a second of `mvp.grievance.pressure_per_hour` to each one's pressure grievance against the other, and puts its cause back. A pair no longer in contact removes its cause.
  - **The fade.** Every grievance then loses its share of the weight it has: 1 − (1 − `fade_share_per_hour` / 100)^(seconds / 3,600), to the nearest millionth. The share is computed once a step, in parts per billion.
  - **A removed cause** fades `pil.conflict.settled_fade_factor` times as fast: the exponent is multiplied by the factor, so an hour leaves 0.9⁴ of the weight.
  - **The end.** A grievance whose cause is removed ends once it weighs less than half a unit, which the relations panel (T11) shows as none.
  - **What it gives.** With its cause in place, territory pressure tends to `pressure_per_hour` ÷ −ln(0.9), about 285, and reaches the Bold threshold of 40 (MVP-057) after about 1.4 hours of contact. The scope's note says about 1.5 hours.
* **The other causes' entry points**, for T5 and T6:
  - `AddGrievance` gives a claim's or an attack's weight. The same cause naming the same subject adds to its grievance.
  - `RemoveCause` removes the cause of every grievance of a cause that names a subject.
  - `RelieveAfterBattle` removes the won or lost share of the attacker's grievances against the target.
* **Tuning.** `pil.conflict.settled_fade_factor` joins the tuning record in a `pil` section. So do `pil.conflict.unprovoked_cost_factor` and `pil.battle.min_respite_minutes`, which T6 reads (the GDD's table of the pillar keys).
* **The saved state.** The grievances follow the scripted stewards in each region's state.
  - `WORLD_STATE_VERSION` is 11. A region of version 10 reads with no grievance.
  - The count takes 16 bits, so the empty world's golden hash of DBG-147 changes. The 3 bits of M2's scripted stewards' count fit in the last byte's padding, so that hash never changed for them.

### Consequences

* Good, because each grievance carries what the panel shows: its cause, its weight and whether it fades at the settled rate.
* Good, because the relations are a few integer operations a second for at most a dozen grievances.
* Good, because a conflict whose cause is removed ends.
* Good, because the owner's M2 world continues with no grievance.
* Bad, because the weights move once a second, not every tick.
* Bad, because a grievance whose cause remains never ends: a claim held open keeps a grievance, faded but present.
* Bad, because the half-unit end is the panel's rounding, not a design value. A design that wants another end makes it a tuning key.
* It forecloses: grievances that arise from a Worker's own choice (PIL-124); a fade measured from the first weight.

### Confirmation

GameLogicTests `RelationTests`:
- **Territory pressure.** It grows each way while sites 2 and 3 are in contact, an hour giving the closed form of the per-second step within 0.005 of a unit. Sites 1 and 2, which are not valley neighbours, are never in contact. Once the contact ends, an hour leaves 0.9⁴ of the weight, and the grievance ends below half a unit. Two seconds of the world's step are two of the relations' steps.
- **Claims and attacks.** A claim and an attack give their weights, and an unprovoked attack twice the attack's. The anger is the sum of each Cladan's own grievances, and the same subject adds to its grievance.
- **The fade.** An hour leaves 0.9 of a claim's weight with its cause, and 0.9⁴ once it is removed. Removing one claim's cause leaves another's.
- **Battle relief.** A won battle removes half of the attacker's grievances against the target and a lost one a quarter. Neither touches the target's grievances or the attacker's against another settlement.
- **Saving.** The snapshot holds the grievances, and the restored world steps alike. A region of version 10's layout reads whole with its settlements and no grievance.

`StateHashTests` holds DBG-147's new golden values. The whole GameLogic and GameShared suites pass.

## Pros and Cons of the Options

### 1 — region-level integers, once a second

* Good, because the rounding is a millionth a second.
* Bad, because the region's state grows a section that every later raise reads by version.

### 2 — every tick

* Good, because the weights move with the battle's ticks.
* Bad, because the fade of a tick rounds coarsely at small weights.

### 3 — binary64 weights

* Good, because the fade is exact.
* Bad, because it goes against R16's rule for the simulation's quantities.

### 4 — in the settlement's record

* Good, because a Cladan holds its own grievances in the state as in the design.
* Bad, because the settlement record's layout would change and be read in two forms ever after.

## More Information

* Amends:
  - [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md): contact as territory pressure's cause;
  - [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md): state version 11.
* Related: [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md) (the four settlements), [ADR-099](ADR-099-world-snapshots.md) (the snapshot's versions).
* Owner decisions O80, O98; MVP-027, MVP-050 to MVP-052, MVP-057, MVP-095; PIL-063, PIL-064, PIL-122, PIL-124; WLD-062, WLD-066; DBG-147.
* The code:
  - `GameLogic/Relations`;
  - `RegionSimulation` (the grievances, their state and the step);
  - `WorldSnapshot` (`WORLD_STATE_VERSION`);
  - `GameShared/TuningMvp` (the `pil` keys);
  - `GameLogicTests/RelationTests`, `StateHashTests`, `TestSupport` (`ValleyWorld`).
* **Amendment 2026-10-22 (M3 task T5, [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md)).** Contact is two valley neighbours whose territories meet, a cell of each sharing a side or a corner within the lens of their sites' discs, so that it holds across a half a claim has moved; before any move this is the halfway line.
* **Amendment 2026-10-22 (M3 task T6, [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md)).** The attack's grievance is given by the declaration (`DeclareAttack`), its subject the expedition's stable id, and its cause is removed when the expedition ends; every battle's end relieves the attacker's grievances against the defender.
