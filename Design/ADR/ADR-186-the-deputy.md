---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T4; owner decisions O2, O109, O110)
---

# The deputy

## Context and Problem Statement

While the steward is away (ADR-185) the deputy runs the settlement on the default orders of PIL-079 and the priorities the steward last set: defend the settlement, keep gathering, finish the queued construction, never attack (MVP-073). O110 reads "a battle at its settlement" as every battle the settlement is a side of (PIL-043):
- the defence of its territory;
- the defence of a border half it holds;
- the steward's own expedition when one is out.

In each, the deputy sends each Leader's formation to the objective and leaves the turrets to the guards; Workers without a Leader fight by the threat search. It never declares, withdraws, claims, pays tribute, demolishes, queues construction, promotes, demotes or changes the priorities (PIL-083). It does nothing while the steward is present (PIL-071). Every action it takes appears in the away report with the standing order that permitted it (PIL-026).

The settlement's economy needs no deputy:
- gathering, construction of the queue and the guards at the turret ports are the Workers' own tasks under the priorities (ADR-135);
- placement pays for a structure at once (MVP-032).

What the deputy does is command Leaders in battle. Who is the deputy in the command path, when does it look, and what does it record?

## Decision Drivers

* The deputy's commands go through the one command path, as an AI's do (ADR-087, ADR-106), and are made again by a replay, as an AI's are (ADR-109).
* The steward stays the settlement's commander, so that the steward's first input after an absence is obeyed at once.
* Each deputy action is recorded with its standing order, in the absence it belongs to (ADR-185).
* Its rule is small enough for the steward to predict in Playtest 2's five scripted situations (the scope's §6.2, the M4 plan's table 4).

## Considered Options

1. **A region-owned deputy, permitted as an AI while its steward is away.** A sender of its own for each stewarded settlement. The region permits its AI commands for the settlement's team and its expedition's only while the settlement has an open absence. It looks every tick of an absence.
2. **The commander swapped.** The settlement's commander becomes the deputy while away and the steward again on return.
3. **The AI settlements' battle core.** The scripted steward's battle core of ADR-162 (`StepBattleCore`) commands the steward's sides while away.

## Decision Outcome

Chosen option: 1.
- Option 2 needs the steward's sender at every return and makes an idle steward's next command race the swap back.
- Option 3 promotes and holds Leaders by the reference bot's rules, which MVP-073 and PIL-083 forbid, and which the steward could not predict.

What is built:

* **The deputy.** `Deputy` (`GameLogic/Deputy`): its team, its sender id and its commands' sequence numbers. `RegionSimulation::EnsureDeputy` makes it, with a new stable id of the region, the first time its steward goes away (`SetStewardPresence`, ADR-185), and keeps it.
* **The permission.** `RegionSimulation::MayCommand` permits a deputy's command, AI commands included, for its settlement's team and its expedition's (`ExpeditionTeamOf`) while that settlement has an open absence (`DeputyMayCommand`). It permits nothing for another team, and nothing while the steward is present. The steward stays the commander throughout.
* **The rule.** `StepDeputy`, at the end of each region tick after the AIs and scripted stewards, while its settlement has an open absence. For each side its settlement has in a battle in progress, it takes each live, undemoted Leader in slot order:
  - the settlement's team where it is the defender, its territory's battle or a claim battle at a half it holds, under `DefendTheSettlement`;
  - its expedition's team once its battle has started, under `TheStewardsAttack`, the steward's own declared attack (O110).

  A Leader that is not in formation mode with its waypoint within 1 wu of the battle's objective is sent two AI Leader operations (R-UNIT-188): `FormUp` and then `AssignWaypoint`, both at the objective. The Workers that follow it go with it. Each such Leader is one `DeputyAction` (`LeaderToObjective`, the Leader's id, the other side's site) in the open absence (`RecordDeputyAction`). A Leader on its way is not ordered again. Its commands execute at the next tick.
* **Every tick.** The deputy looks every tick of an absence, not every `mvp.ai.decision_ticks`, so that a battle's Leaders are ordered at its first tick. A short battle could otherwise end before the deputy's first look. The look reads only the region's battles while it has none.
* **The standing orders.** `StandingOrder` (`GameLogic/Presence`) holds the four default orders of PIL-079 and `TheStewardsAttack` (4), a saved value never renumbered, written in 4 bits, so that the orders a later stage adds fit without a layout change.
* **The state.** The region writes its deputies as a new last section. `WORLD_STATE_VERSION` is 20 (`STATE_VERSION_WITH_DEPUTIES`). A region of version 19 or older reads with none, and makes each at its steward's next absence.

What the deputy leaves alone, and why it needs no rule:
- The guards at the turret ports, the gatherers and the builders of the queue are the Workers' tasks under the steward's priorities and hold. The deputy never sends `SetPriorities`, `HoldGrowth` or a placement.
- An expedition held by its warning, or walking to its target, is left as the steward sent it; no withdrawal is ever sent.

### Consequences

* Good, because the deputy's rule is one sentence that the steward can predict, and the away report names each action and its order.
* Good, because a replay makes the deputy's commands again from the journal's presence entries, as it makes an AI's.
* Bad, because a Leader that loses its formation in a battle is ordered again, a second action of the absence.
* Bad, because the deputy does not promote: a settlement without Leaders defends with its Workers' threat search alone, as MVP-073 says. Playtest 2 judges whether that is safe enough (PIL-022).
* It forecloses a deputy that weighs a battle: no retreat and no reinforcement. Standing orders beyond the defaults wait for B6 (the scope's §5.1).

### Confirmation

* `GameLogicTests/DeputyTests`, on the tests' valley with the steward away:
  - situation 1: an attack declared on the settlement draws no action during its warning, and at its battle the settlement's Leader is sent to the Wellspring, in formation, once, under "defend the settlement" against the attacker;
  - situation 2: a claim battle at the half the settlement holds sends its Leader to the half's resource site;
  - the steward's own raid's Leader, turned aside, is sent back to the target's Wellspring under "the steward's attack", and the expedition is never withdrawn;
  - situations 3 and 4: at the population limit, with the stores empty and a dwelling queued, the deputy takes no action, the dwelling is built, and the priorities and the hold stay;
  - situation 5: a neighbour's respite ends while the steward's Cladan is angry with it, and nothing is declared;
  - the deputy commands its settlement's and its expedition's teams only while away, no other team, the same deputy at the next absence; it saves and restores, and a version-19 region reads with none.
* Playtest 2: the steward predicts the deputy in at least 4 of the 5 scripted situations, and every deputy action is in the report with its order (the scope's table 6).

## Pros and Cons of the Options

### 1 — a region-owned deputy

* Good, because the steward's command is obeyed at once, and the deputy acts only while away by construction.
* Bad, because the permission gains a second rule beside the commander's.

### 2 — the commander swapped

* Good, because the permission is unchanged.
* Bad, because the steward's return races the swap, and the region would keep the steward's sender for it.

### 3 — the AI settlements' battle core

* Good, because it is built and tested.
* Bad, because it promotes and holds by rules the deputy must not follow.

## More Information

* Owner decisions O2, O109, O110; MVP-073, PIL-026, PIL-043, PIL-071, PIL-079, PIL-083; the [M4 plan](../implementation/m4_plan.md), task T4 and table 4.
* Amends [ADR-106](ADR-106-leaders-formations-and-battle-commands.md) (the command path's permission) and [ADR-099](ADR-099-world-snapshots.md) (the state version).
* The code: `GameLogic/Deputy`, `GameLogic/Presence`, `GameLogic/RegionSimulation`, `GameLogic/WorldSnapshot`.
