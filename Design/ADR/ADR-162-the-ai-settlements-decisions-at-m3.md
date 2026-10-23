---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M3 task T7, owner decisions O80, O98)
---

# The AI settlements' decisions at M3

## Context and Problem Statement

M3's T7 makes the AI settlements decide:
- **The attack rule** (MVP-055): an AI settlement attacks only a settlement against which its Cladan's anger reaches its temperament's threshold, or for an area it has claimed (PIL-025). It does so only when it can field at least `mvp.ai.attack_min_share` (0.6) of the target's Workers it sees, and it chooses the raid or the claim battle by its temperament.
- **The temperaments** (MVP-057): Bold claims the central resource site early and prefers raids. Territorial claims every neighbour's half of its border areas, and Wary claims only after being attacked; both prefer claim battles. The tribute rates are [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md)'s.
- **The expedition** of up to three Leaders, formed as the steward's is (MVP-041, O98).
- **The battle core** of M1's reference bot ([ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md)) commanding their sides in battle (MVP-049, MVP-058). They act only through the command path, every `mvp.ai.decision_ticks` (50), within ADR-080 item 2's budget.

Four things stand in the way:
- **One commander a team.** An AI settlement's team is commanded by its scripted steward ([ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md)), and a command is permitted only from its team's commander (ADR-087 item 2). An AI of M1 is a commander of its own, with its own sender id and its own sequence of commands.
- **The battle core's goal.** M1's core seeks the bot nodes an enemy holds. A battle ends by its objective held (MVP-045), which the core never seeks.
- **The core at peace.** The core's dispatch, stray recall and formation Leaders would pull a settlement's Workers off their tasks.
- **The scope leaves a design gap.** It does not say how an AI gathers its expedition, how large it makes it, or what Wary claims once attacked. These are [DEC-1006](../decisions/species_design.md#dec-1006-how-the-ai-settlements-claim-and-raise-their-expeditions), Open, whose stated behaviour this record builds.

## Decision Drivers

* An AI settlement and the steward act on the same rules and the same commands (MVP-056, MVP-094).
* Its choice reads only what its settlement may see and its Cladan's relations, never whether a steward is present (MVP-055, PIL-086).
* Its decisions are region state, saved and replayed with the world, and a world of an earlier version continues (O98, MVP-095).
* The core of M1 is reused, not rewritten (R-BOT-001 to R-BOT-099 as ADR-107 builds them).

## Considered Options

1. **The decisions run by the scripted steward**: its sender id and its sequence of commands, with two battle cores of M1 it steps while its sides fight.
2. **A second AI commander** for the settlement's team in battle, handing the team between the steward and the AI.
3. **One AI of M1 for each AI settlement at all times**, its core running in peace too.

## Decision Outcome

Chosen option: 1.
- **Option 2** needs a team to change commanders mid-battle, and a command sent before the hand-over is refused after it.
- **Option 3** runs the dispatch on a settlement at peace, ordering its idle Workers off their tasks.

* **The record** (`GameLogic/SettlementAi`): each AI settlement's decisions (`SettlementAi`) are region state, one for each scripted steward whose settlement has a temperament. They hold:
  - **its decision countdown**;
  - **the muster** of its next attack: target, claim, size, fewest it may send, start, Leaders wanted and those it has;
  - **two battle cores** (`AiController`): one for its settlement's team, one for its expedition's. Their sender id is the steward's.

  The steward shares its sequence of commands with them, and the decisions step after the steward's own look in the region's tick.
* **The battle cores** (`StepBattleCore`, [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md) amended): M1's core takes the sequence it numbers its commands from, and an optional objective.
  - **The expedition's core.** With an objective, a Leader that stands beyond `mvp.battle.objective_radius` of it walks to it. That replaces holding a node and seeking an enemy's node; facing and stopping for many enemies stays.
  - **When they run.** The settlement's core steps every tick while the settlement defends a battle, and the expedition's while its battle is in progress. Each starts fresh with its battle, its first main cycle drawn as an AI's is.
* **A decision**, every `mvp.ai.decision_ticks`:
  1. **A claim** by its temperament, one at most, as DEC-1006 states: Bold the central resource site; Territorial each half its neighbour holds; Wary, once attacked, the halves its attackers hold. Each is sent only when `CheckClaim` would not refuse it.
  2. **No muster** while its expedition is out or it defends a battle; a muster under way ends then.
  3. **The muster's next step**:
     - **Leaders.** It adopts its team's undemoted Leaders, up to those it wants, and forms each up where it stands, facing the target, so that idle Workers on guard duty join it (R-UNIT-074). It promotes the Workers still to promote, nearest its Wellspring first.
     - **The declaration.** It declares once its Leaders' Workers reach the size, or once `mvp.ai.muster_minutes` (2) has passed with at least the fewest it may send. It names its Leaders in the order adopted while their Workers stay within `mvp.expedition.size_max`.
     - **Giving up.** It gives the attack up when the attack is no longer allowed, or when the time has passed with too few Workers.
  4. **Otherwise a new muster** against the settlement its Cladan holds most anger against, among those it may attack. It may attack a target when its anger reaches its threshold or it has an Open or Contested claim on what the target holds, the target is under no respite, and it can field enough:
     - **Enough** is `attack_min_share` of the target's Workers it sees, at least one (`SeenWorkersOf`). It can field its Workers beyond `mvp.protection.worker_floor`, at most `size_max` (`FieldableWorkers`), and the muster's size is all of them.
     - **The kind.** Bold musters a raid when its anger allows; the others the claim battle when they hold a claim.
     - **The stores.** A muster waits for stores that hold the attack's cost.
     - **Leaders.** It wants one for each `ref.sim.team.formation_size_upgrade` (90) Workers, at most three.
  5. **Demotion.** Outside a defense, every Leader of its team that no muster holds, nor a declaration just sent names, is demoted, so its Workers go back to their tasks.
* **What it reads.** Only its own settlement, its Cladan's grievances, the claims, the respites, the battles and results, and the target's Workers within its seen layer ([ADR-158](ADR-158-the-seen-layer-and-its-readers-at-m3.md)). Nothing tells it whether a steward is present.
* **The saved state.** `WORLD_STATE_VERSION` is 14: each region's AI settlements' decisions follow its respites. A region of version 13 or earlier makes them afresh for each AI settlement's steward, its first decision at its first step. The empty world's golden hash of DBG-147 moves with the new count, checked to be the only change.
* **Tuning.** `mvp.ai.muster_minutes` (2, minutes of world time at the tick rate) joins the record and table 3 of the MVP scope, for MVP-055 and DEC-1006.
* **What waits.**
  - The steward's settlement under the scripted steward with a stated conflict policy, the valley run of Playtest 1's lines, and the AI's measured cost are T9's.
  - The quiet minutes before a run's end are T8's.
  - An AI settlement never withdraws an attack or a claim, nor sends tribute.

### Consequences

* Good, because an AI settlement's every action is a command of its own steward's, through the same path as the steward's.
* Good, because M1's core fights both sides, and walks an expedition to its objective, without a second team commander.
* Good, because a settlement at peace keeps its Workers on their tasks: the cores run only in battle, and the Leaders are demoted after it.
* Bad, because a muster draws only the Workers that are idle on guard duty near its Wellspring, so an expedition is as large as its guard and its growth in the muster's minutes allow.
* Bad, because the first decision after a battle may start a new muster against the same settlement while the anger stands. The respite is what stops a second attack on a beaten defender.
* It forecloses: an AI settlement attacking with no anger and no claim; an AI settlement that reads whether a steward is present.

### Confirmation

GameLogicTests `SettlementAiTests`:
- **Claims on a corpus of three world keys.** The Bold settlement claims the central site at once, and the others nothing. Once the territories reach, the Territorial settlement claims each of its neighbours' halves, Open against them, and the Wary and the Bold none. Once the Wary settlement is attacked, it claims its attacker's half.
- **The attack rule.** Below the threshold, no muster. At it, the Bold settlement musters a raid of the 40 Workers it can field with one Leader, promotes and adopts its Leader, and declares when the muster's time is out, provoked. It musters nothing against a target under a respite, or one with more seen Workers than it can field against.
- **Raid or claim battle.** With anger and a claim, the Bold settlement raids and the Territorial one fights the claim battle. With a claim alone, both fight the claim battle.
- **The battle cores.** The expedition's core commands its Leaders in its battle and stops with it, and its Leaders home are demoted. The settlement's core defends the steward's raid, and its Leaders are demoted after it, but a new muster's.
- **Saving.** A muster under way is saved and restored, and both worlds decide alike to the declaration. A region of version 13 makes the decisions afresh.

`StateHashTests` holds DBG-147's new golden values. `RelationTests` clears the AI settlements' decisions where it tests the relations alone. The whole GameLogic and GameShared suites pass.

## Pros and Cons of the Options

### 1 — the decisions run by the scripted steward

* Good, because one commander sends everything its settlement does.
* Bad, because the steward's record and the decisions share a sequence of commands.

### 2 — a second AI commander in battle

* Good, because M1's AI runs unchanged.
* Bad, because the team changes hands at each battle's edges.

### 3 — an AI of M1 at all times

* Good, because nothing new is written.
* Bad, because a settlement at peace is dispatched as an army is.

## More Information

* Amends:
  - [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md): the battle core with a shared sequence and an objective;
  - [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md): state version 14;
  - [ADR-138](ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md): the scripted steward runs an AI settlement's decisions;
  - [ADR-156](ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md): a connecting steward stops them;
  - [ADR-161](ADR-161-expeditions-battles-the-loss-cap-and-the-respite-at-m3.md): the AI settlements' sides commanded.
* Related: [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md) (the AI settlements), [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md) (anger), [ADR-160](ADR-160-claims-moved-areas-and-tribute-at-m3.md) (claims), DEC-1006.
* Owner decisions O80, O98; MVP-041, MVP-049, MVP-055 to MVP-058, MVP-094, MVP-095; PIL-025, PIL-086; R-BOT-009, R-BOT-074; DBG-147.
* The code:
  - `GameLogic/SettlementAi`;
  - `AiController` (`StepBattleCore`), `RegionSimulation` (the decisions, their state and the step), `WorldSnapshot`;
  - `GameShared/TuningMvp` (`mvp.ai.muster_minutes`);
  - `GameLogicTests/SettlementAiTests`, `StateHashTests`, `RelationTests`, `TestSupport` (`ValleyWorld`'s tuning, `RegionStateOf`).
* **Amendment 2026-10-22 (M3 task T8, [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md)).** An AI settlement starts no muster in the run's quiet minutes, and gives up one under way.
* **Amendment 2026-10-23 (M3 task T9, [ADR-164](ADR-164-the-command-paths-confirmation-the-valley-run-and-the-valley-at-war-at-m3.md)).** An AI settlement defends a claim battle from its warning. The valley run found every claim battle won by the claimant in 30 seconds: its objective is the claimed half's resource site, away from the holder's home, and the holder's battle core stepped only once the battle had started, with no objective. Now, from the declaration of a claim battle on it to the battle's end, its battle core steps with the claimed site as its objective (`ClaimDefenseOf`), and it starts no muster and demotes no Leader meanwhile. A raid's defense is unchanged, and the state does not change.
