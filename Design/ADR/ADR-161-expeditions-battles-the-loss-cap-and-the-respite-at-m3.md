---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M3 task T6, owner decisions O80, O98)
---

# Expeditions, battles, the loss cap and the respite at M3

## Context and Problem Statement

M3's T6 builds the attacks between settlements:
- **The expedition** (MVP-041 as O98 amends it): up to three of a settlement's Leaders and the Workers following them, at most `mvp.expedition.size_max` (200) Workers, ordered and declared as one.
- **The declaration** (MVP-042): it pays `mvp.conflict.attack_cost_ru` (300 ru), twice that for an unprovoked attack (PIL-063), warns the target at once, and holds the expedition at the edge of the target's territory for `mvp.conflict.warning_minutes` (5). No attack is declared on a settlement under a respite.
- **The battle** (MVP-043 to MVP-045): it starts when the released expedition enters the target's territory, or the claimed area. The defender takes 0.75 of the damage inside its own territory. The battle ends at the first of four ends: the objective held, the loss cap reached, the rout (or a withdrawal) and the time.
- **The loss cap and the respite** (MVP-046, MVP-047): a defender loses at most 30 % of its Workers and 25 % of its stores in one battle, and never falls below 20 Workers. Every transfer is bounded by the losing side's cap, Essence beyond it ascending (PIL-045). A loss of 10 % of its strength in a battle of at least 2 minutes starts a respite of 20 minutes.
- **The result** (MVP-048) and **the cap of two battles** in progress in a region (MVP-015).

Four things stand in the way:
- **The peace.** [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md) makes every settlement's team a non-enemy of every other's. A battle must make its two sides enemies, and nothing else: the attacker's gatherers at home and the defender's at its far border stay at peace with everyone.
- **Who the expedition is.** Its figures leave their settlement's tasks and follow its orders, yet they stay the settlement's population, its eyes for the seen layer ([ADR-158](ADR-158-the-seen-layer-and-its-readers-at-m3.md)) and its commander's to order.
- **The plunder.** A won raid moves stores that M2 holds only as a settlement's integer stores and a Worker's load ([ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)).
- **A world of an earlier version continues** (O98, MVP-095).

## Decision Drivers

* A battle changes who fights whom only between its two sides, and only while it lasts (PIL-041).
* The threat search, its cache and the AI read hostility by team (R-SIM-311, R-SIM-316, [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md)), and stay as they are.
* Every end of an attack, before or in its battle, returns its figures and removes its grievance's cause (PIL-122).
* Every transfer a battle makes is bounded by the loss cap (MVP-046).

## Considered Options

1. **A team for each settlement's expedition**, the team slot after the four sites' (`ExpeditionTeamOf`): the figures move to it at the declaration and back at the end, and a battle makes its two teams enemies.
2. **An expedition flag on the figures** of the settlement's own team, with hostility decided figure by figure.
3. **The two settlements enemies for the battle's length**, all their figures with them.

## Decision Outcome

Chosen option: 1.
- **Option 2** breaks the threat search's team masks: every query, the cache and the AI's census would test figures one by one.
- **Option 3** puts the attacker's gatherers at home and the defender's at its far border at war, and lets a battle spread beyond its battlefield.

* **The expedition's team** (`GameLogic/Battles`): the expedition of the settlement at site *s* is team 4 + *s* (`EXPEDITION_TEAM_FIRST`).
  - **A side.** A settlement's team and its expedition's are both settlement sides (`IsSettlementSide`, `SettlementOfSide`).
  - **The peace.** `EnemiesOf` makes a settlement side a non-enemy of every settlement and expedition. A battle in progress then makes its expedition's team and its defender's team enemies of each other, and no other pair. The actor grid's cache forgets its queries when a battle starts or ends (`ForgetCachedEnemies`).
  - **What stays the settlement's.** Its expedition's figures count in its population (`Population`) and its seen layer ([ADR-158](ADR-158-the-seen-layer-and-its-readers-at-m3.md) amended). `Sight::OfTeam` reads the expedition's team as its settlement's.
  - **Its commander.** The settlement's commander commands the expedition's team while it is out, and a steward who connects takes both ([ADR-156](ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md) amended).
* **The declaration** (`DeclareAttack`) is refused, in this order (`AttackRefusal`):
  - by a team that is no settlement's;
  - while the settlement has an expedition out;
  - on no settlement or itself, or for a claim that is not its own Open or Contested claim;
  - on a target under a respite;
  - with no live Leader of its own among those named;
  - with more Workers following the named Leaders than `size_max`, counted across them;
  - with stores short of the cost.
* **A declaration that stands:**
  - **The target.** A raid's is the named settlement. A claim battle's is the claimed area's holder, or the rival claimant of a contested central resource site ([ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md)).
  - **The cost.** It is unprovoked when the attacker's Cladan holds no anger against the target and the attacker claims nothing the target holds; a claim battle is never unprovoked. It then pays `attack_cost_ru` times `pil.conflict.unprovoked_cost_factor` (2).
  - **The grievance.** The target's Cladan gains the attack's grievance, its subject the expedition's stable id ([ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md)). The weight it added is recorded for the result.
  - **The figures.** The Leaders and the Workers following them move to the expedition's team (`RegionSimulation::MoveFigure`). Each Worker's Leader slot is remapped, and each leaves its task.
  - **The walk.** Each Leader walks in formation (R-UNIT-171's waypoint and formation) to the staging point. For a raid that is `mvp.battle.objective_radius` beyond the target's territory radius from its Wellspring, towards the attacker's Wellspring. For a claim battle it is the resource site's clearance and twice the objective radius from the site.
* **The attacks' step** (`AdvanceBattles`) runs in the settlements' step after the claims. It takes, in order:
  - **An emptied expedition.** One with no living figure before its battle ends, as a withdrawal does.
  - **The release.** A held expedition is released `warning_minutes` after its declaration, and each Leader walks to the objective.
  - **The start.** A released expedition starts its battle when one of its figures stands in the target's territory, the claimed half, or the central site's clearance, while fewer than `wld.scale.battles_per_region_max` (2) are in progress. Expeditions are taken in the order declared, so a waiting one starts first when a battle ends (MVP-015).
  - **The battle's record.** It keeps the objective, the battlefield and the defender at the start: its population, stores and strength. The battlefield is a disc: the target's territory radius about its Wellspring, or half of `wld.territory.max_radius` about the claimed site, and `wld.view.settlement_margin` (800 wu) beyond (MVP-043).
  - **The ends.** Each battle counts the ticks in which the attacker's Workers are within the objective radius of the objective and no defender figure is. It ends at the first of:
    1. the defender's losses at a loss cap above 0;
    2. the hold reaching `mvp.battle.objective_hold_s` (30 s);
    3. the attacker's living Workers below `mvp.battle.rout_share` (0.25) of those it sent, or none of its figures alive;
    4. `pil.battle.max_minutes` (20) since its start.
  - **The winner.** The first two ends are the attacker's wins, the other two the defender's.
* **The loss cap** (`LossCapOf`): the lesser of `loss_cap_worker_share` of the defender's figures at the start and those above `worker_floor`.
  - **Damage** (`BattleDamage`, in `RegionSimulation::Damage`): a defender figure takes none once its battle's losses reach the cap. Before that it takes `mvp.defense.damage_taken_factor` inside its own territory.
  - **Deaths** (`BattleDeath`): every death counts for its side. An expedition figure that dies inside the defender's territory leaves its orb, which drifts to the defender's Wellspring (MVP-023), while the battle's Essence moved is below `loss_cap_worker_share` of the Workers the expedition sent. Beyond that it leaves none: the Essence ascends.
  - **A settlement at its floor.** One of 20 Workers has a cap of 0 and loses nothing in a battle; the loss cap is not an end for it.
* **The end of a battle** (`EndBattle`):
  - **A won raid** takes `loss_cap_store_share` of the defender's stores at the start, no more than it still has. The plunder is shared among the expedition's living Workers that still follow its Leaders, the first taking the remainder. Each carries its share to the nearest drop-off point (`StartCarrying`, MVP-031).
  - **A claim battle** settles its claim, moving the area on a win ([ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md)), and moves no stores.
  - **The relief.** The attacker's grievances against the defender are relieved ([ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md)).
  - **The respite.** The defender starts one of `mvp.protection.respite_minutes` (20) when its strength fell by `respite_min_loss_share` (0.1) of its strength at the start, unless the battle lasted less than `pil.battle.min_respite_minutes` (2). A new respite replaces the defender's earlier one.
  - **The result** (`BattleResult`) records the sides, the end and the winner, the ticks, each side's losses, the stores and Essence moved, whether the area moved, the attack's grievance and the respite. Its renown is T8's.
  - **Home.** The expedition ends: its grievance's cause is removed, every figure of its team returns to the settlement's team, and its Leaders walk to the Wellspring with the Workers following them.
* **A withdrawal** (`WithdrawAttack`) before the battle ends the attack with no battle, no result and no respite (PIL-087). In the battle it ends the battle as a loss, recorded as `BattleEnd::Withdrawn`, which MVP-045 counts as a rout.
* **The commands** (`DeclareAttackCommand`, `WithdrawAttackCommand`, message types 0x011B and 0x011C) are registered with the settlement's ([ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md)). Each is permitted for the team's commander, and its refusal is its outcome's reason.
* **The saved state.** `WORLD_STATE_VERSION` is 13.
  - After its moved areas, each region holds its expeditions in the order declared, its battles in the order started, its battle results and its respites.
  - A region of version 12 or earlier reads with none. The empty world's golden hash of DBG-147 moves with the four counts, checked to be the only change.
* **Tuning.** `wld.scale.battles_per_region_max` (2) and `pil.battle.max_minutes` (20) join the record, at the scope's values.
* **What waits.** The AI's attacks and its command of its side (MVP-049, T7), the renown (T8), the steward's verbs for an expedition (T12), the warning, the result and the battlefield on the HUD and in the camera's bound (T11, T12), and the battle's sound (T15) are later tasks.

### Consequences

* Good, because the threat search, its cache and the AI read the battle's hostility through their team masks, unchanged.
* Good, because every way an attack ends returns its figures and removes its grievance's cause.
* Good, because the loss cap bounds the defender's losses, the stores taken and the Essence moved in one place each.
* Bad, because moving a figure between teams moves its slot, so whatever named the old slot must be remapped. Today that is the followers' Leader slot and the team's selection.
* Bad, because a Worker that stopped following its Leader in the battle is no carrier of the plunder, so fewer Workers carry it.
* Bad, because the battlefield is a disc about the target, not its territory's shape; nothing at M3 reads it but the result and the steward's areas of T12.
* It forecloses: more than one expedition of a settlement at a time; an attack by a team that is no settlement's; a battle with more than two sides.

### Confirmation

GameLogicTests `BattleTests`:
- **The declaration.** Each refusal comes in its order. One that stands pays twice the cost when unprovoked and once with a cause on record. It gives the target the attack's grievance, its weight recorded. It moves the Leader and its six Workers to the expedition's team, still following it, under the settlement's commander, and walks them in formation to the territory's edge. The two stay at peace there, and the expedition is an enemy of a team that is no settlement's.
- **Three Leaders as one.** The expedition is held, with no battle, until the warning ends. Then each Leader walks to the objective. Its battle starts as it enters, with its battlefield and its defender at the start, and only its two sides become enemies. It ends in a result within the loss cap's bounds, its survivors home.
- **The ends.** The loss cap comes before the objective held. The capped stores are carried home by the expedition's Workers. A defender at the objective stops the hold. The rout comes before the time. A withdrawal in the battle is the defender's win, one before it leaves no result and no respite, and each refusal of a withdrawal comes.
- **The loss cap.** It is a share of the Workers above the floor. The defender takes 0.75 inside its territory, full damage outside, and none at the cap. The expedition's Essence is bounded by its own share.
- **The respite.** It follows a loss in a long enough battle and refuses an attack until it ends. A shorter battle or a smaller loss starts none.
- **The battle cap.** With one battle allowed, an expedition that enters waits at peace and starts its battle when the first ends.
- **A claim battle.** It is refused for a waiting claim. For an open one it targets the holder at the provoked cost, starts round the claimed half's resource site, and a win moves the half, no stores, and settles the claim.
- **The commands** carry their refusals as their outcomes' reasons, and the steward commands its expedition while it is out.
- **Saving.** The snapshot holds the expedition, its battle, a result and a respite, and the restored world steps as the saved one. A region of version 12's layout reads with none.

GameSharedTests `RelationMessageTests`: the attack commands round-trip at their bounds, and a declaration cut short is malformed. `StateHashTests` holds DBG-147's new golden values, and `RelationTests`, `ClaimTests`, `ValleyTests` and `ReplicationTests` read the earlier versions through `RegionStateOf`. The whole GameLogic and GameShared suites pass.

## Pros and Cons of the Options

### 1 — a team for each expedition

* Good, because hostility stays a property of teams.
* Bad, because figures move between teams, and their slots with them.

### 2 — a flag on the figures

* Good, because no figure moves.
* Bad, because every hostility test becomes a figure's.

### 3 — the settlements at war for the battle

* Good, because nothing moves and nothing new is tested.
* Bad, because the war is everywhere, not on the battlefield.

## More Information

* Amends:
  - [ADR-130](ADR-130-settlements-structures-and-resource-sites-at-m2.md): a settlement's population includes its expedition;
  - [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md): the seen layer about the expedition's figures;
  - [ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md): the attack commands;
  - [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md): state version 13;
  - [ADR-156](ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md): a connecting steward commands its expedition;
  - [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md): the peace broken by a battle's two sides;
  - [ADR-158](ADR-158-the-seen-layer-and-its-readers-at-m3.md): an expedition's sight is its settlement's;
  - [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md): the attack's grievance added and its cause removed;
  - [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md): claim battles settle claims.
* Related: [ADR-106](ADR-106-leaders-formations-and-battle-commands.md) (formations), [ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md) (carrying).
* Owner decisions O80, O98; MVP-015, MVP-023, MVP-031, MVP-041 to MVP-048, MVP-051, MVP-053, MVP-095, MVP-101; PIL-041, PIL-045, PIL-063, PIL-080, PIL-087, PIL-122; WLD-102; DBG-147.
* The code:
  - `GameShared/RelationMessages`, `GameMessages`, `TuningMvp`;
  - `GameLogic/Battles`;
  - `RegionSimulation` (`EnemiesOf`, `MoveFigure`, `Damage`, the state and the step), `ActorGridCache`, `Construction` (`Population`), `SeenLayer`, `SettlementTasks` (`StartCarrying`), `SettlementCommands`, `CommandJournal` (`ConnectSteward`), `WorldSnapshot`;
  - `GameLogicTests/BattleTests`, `StateHashTests`, `TestSupport` (`ValleyWorld`, `RegionStateOf`); `GameSharedTests/RelationMessageTests`.
* **Amendment 2026-10-22 (M3 task T7, [ADR-162](ADR-162-the-ai-settlements-decisions-at-m3.md)).** An AI settlement's side of a battle is commanded by M1's battle core, which its scripted steward steps while the settlement defends and while its expedition fights, the expedition's walking to the battle's objective (MVP-049).
* **Amendment 2026-10-22 (M3 task T8, [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md)).** No attack is declared in the run's quiet minutes (`AttackRefusal::Quiet`, after `NoSettlement`); at the run's end an expedition whose battle has not started ends with no battle, and battles in progress finish. Each battle's end gives the defender who held it or the claimant who won it its renown and is recorded in both sides' statistics.
