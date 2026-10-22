---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T8)
---

# The simple AI, its navigation graph and the node census

## Context and Problem Statement

Task T8 of the [M1 plan](../implementation/m1_plan.md) gives the sandbox its opponent: the reference bot's formation-Leader core, its dispatch with random target selection and its stray recall, with the node reads they need (O70). It acts only through the command path (MVP-058 at M1, MVP-094).

The reference bot is an immortal actor of its team that applies its actions at once inside the step (R-BOT-006, R-BOT-014). It runs on a graph of map-placed navigation nodes, whose recounts every bot rule reads and every Worker's threat search reads too (R-BOT-045). These decisions are open:
- what the MVP's bot nodes and links are, and which of them are state;
- where the AI runs in the tick, and how its actions reach the simulation;
- which difficulty the AI is;
- the readings that the reference's prose leaves to the implementation.

## Decision Drivers

* The AI acts only through the one command path, executed at the next tick (MVP-058, MVP-094, ADR-087).
* The bot rules stay as the reference states them, at the reference cadences, and read the whole battle until M3 (O70, R-BOT-017).
* A region reads no other owner's state (ADR-085 item 2), and a restored world gives the hashes of a world never stopped (ADR-099).
* Every draw is keyed by a purpose and a subject (ADR-082, ADR-097), and every number is a tuning value (MVP-099).

## Considered Options

* 1 — the AI as region state that runs after the tick's slices and submits commands to its region's queue; the bot graph derived from the MVP-017 nodes and the terrain; the nodes' counts as region state
* 2 — the AI as an actor in its team, as the reference's bot controller is, applying its operations directly
* 3 — the AI outside the region, in the world owner, reading the region's state between ticks

## Decision Outcome

Chosen option: "1".
- Option 2 bypasses the command path, against MVP-058 and DEC-122's option C (O70).
- Option 3 reads another owner's state (ADR-085 item 2).

* **The graph** (`BotGraph`). The bot's nodes are the navigation nodes of MVP-017, in their order, which stands for the reference's structure order (O70).
  - Links are built once by R-BOT-030 and culled by R-BOT-034, when the region's terrain attaches.
  - The MVP has no manual link, no wall, no Barrier Emitter, no zone and no run-time node, so a link records no emitter and the graph never changes.
  - The graph is derived from the terrain, like the land route graph (ADR-104), so neither the hash nor a snapshot holds it.
* **The census** (`BotNode`, R-BOT-041 to R-BOT-047). Each node's countdown, its friend, enemy and idle counts per team slot, and its owner are region state, in a pool sliced like the others.
  - The nodes advance in each slice after the effects, in the place of the structures of R-SIM-218 step 4, which arrive with M2.
  - A node draws its first countdown once, when the region makes the nodes: on the terrain's attachment, or on a restore of a region saved before this record. A node has no stable id, so its draw's subject is its index + 1.
  - Each recount writes the nearest node of every Worker it finds (R-BOT-043), a Worker field.
  - Every Worker's threat search reads the counts: R-UNIT-098 step 6, deferred by ADR-105, is built.
* **The AI** (`AiController`). `RegionSimulation::AddAi` makes an AI for a team between ticks.
  - It issues the AI's sender id from the region's counter, draws its main-cycle phase, and makes the AI the team's AI commander (ADR-106).
  - At the end of each step, after the slices, each AI runs R-BOT-074 on that tick's state: the stray recall, its main-cycle countdown, the Leader policy of each Leader of its team (R-UNIT-188), and the main cycle when the countdown is out.
  - Each action is a command of its region's queue with the AI's id and sequence number, so it executes at the start of the next tick. A waypoint order is `AiOrderWorker` with no route; a Leader's orders and fields are `AiOrderLeader` and `AiLeaderFields`; a promotion is `PromoteWorker` (ADR-106).
  - Its state is region state: its id, team, main-cycle countdown, next sequence number and each live Leader's policy countdown.
* **The easy bot.** The M1 AI is the reference's easy bot (R-BOT-022, R-BOT-075): random target selection is the easy bot's at every selection (R-BOT-087, O70), with its main cycle of 200 ticks, the easy bot's calm rule and no garrison.
  - Its main cycle runs the formation Leaders of R-BOT-091, then the well-defended list and dispatch of R-BOT-078 to R-BOT-086.
  - The MVP has no Spawn Node, Power, Beam Dish, Transport, Strike team, mode or tutorial, so none of their rules runs.
* **Readings of the reference's prose**, kept as rules here:
  - Because the AI's actions execute at the next tick, a step treats the Workers it has ordered as no longer idle. It also treats the Workers it has promoted in this cycle as non-waypoint Leaders where they stand. The reference's immediate actions would leave both so.
  - The AI is not an actor: it has no body for a recount to count, and no position (R-BOT-008, R-BOT-010).
  - No node is important until M2's structures exist (R-BOT-049). So every node the team owns is a corridor that releases all its idle Workers, and a formation Leader's item 3 never walks to an important node.
  - A recount counts every actor in range, a corpse included, and only live Idle Workers as idle.
  - A node's range and the census shortcut use the recount's circle in x and z; the other distances of the bot rules are 3-D.
  - In R-UNIT-188 item 3, a defended node is held facing from it towards its first linked node of a hostile owner. Otherwise the Leader faces from where it stands towards the node's first link.
  - The one draw context of a purpose and subject in a tick continues across the AI's uses of it (ADR-103): the stray recall's picks and the selections are the AI's, and the send draw and the order offset are the Worker's or the Leader's.
* **Not built:** the conveyor Leaders, Beam Dish aiming and the Power handlers (no Spawn Node, Beam Dish or Power exists), the priority model and the best selection (only random selection is used), Relic Run, Siege, Launch Race and tutorials, and the wall-hit reset of the stray recall (no wall exists).
* **Values in data.** The record holds the `ref.bot.*` and `ref.unit.leader.bot_*` keys these rules read.
* **State.** `WORLD_STATE_VERSION` becomes 8.
  - Each region writes its AIs after its commanders, and its nodes' state after its orbs.
  - Each Worker writes its nearest node.
  - A region of an older version reads with no AI, and makes its nodes as a new region does.
  - The golden run of DBG-147, a world with no terrain, changes by the empty AI list and node pool, and the independent model computes the same values.

### Consequences

* Good, because the AI takes the steward's path: every action is a command, checked, executed at the next tick and reported.
* Good, because the census shortcut makes a Worker's threat search cheap where a node has counted no enemy.
* Bad, because two AIs with a fixed 200 a side do not take a battle to its end. Measured with a probe program over `RegionSimulation` on seed 1's valley, built with g++ 13 at -O2 in a 4-core 2.1 GHz Xeon container:
  - Each AI promotes a Leader at each node of 70 friends with no formation Leader within 300 wu. On the 320 wu lattice of MVP-017, neighbouring nodes each get one, so a side of 200 forms two to five formations, and most of them never reach 90.
  - A formation advances only while it is full (R-UNIT-188 item 4). Once it has lost members, or if it never filled, it holds and faces.
  - Formation volleys come from the edge of a search circle that reaches about 150 wu ahead, beyond the laser's reach of about 90 wu (DEC-111), so most of their bolts miss.
  - Under one simulation key, after 40,000 ticks 102 and 127 figures were left of 200 a side. Under another, 198 and 197 were left after 20,000 ticks. With the test's set-up, the formations came to rest about 540 wu apart with no shot fired.
  - 40,000 ticks took about 9 s there.
  - So T8's "to a battle's end" is not met. What should end a battle, or refill a formation, at M1 is a question for the owner (DEC-112, MVP-045).
* Bad, because a recount, a stray recall and the census shortcut each scan nodes linearly. T10 measures them.
* It forecloses: an AI that writes the simulation outside the command path; bot nodes as state that a snapshot must agree with the terrain on.

### Confirmation

* `GameLogicTests` checks:
  - the links within range and the cull of a detour (AC-BOT-08, AC-BOT-10), and the nearest walkable node;
  - a recount's counts, owner, idle count and a Worker's nearest node, and one recount every 20 ticks (AC-BOT-11, AC-BOT-12);
  - the census shortcut, before and after the recount that finds an enemy;
  - the easy bot's main cycle every 200 ticks (AC-BOT-20);
  - a crowded node's formation Leader, promoted by the AI's command at the next tick, formed up by its policy and joined by Workers;
  - dispatch along a link with no route, and the stray recall;
  - two AIs on seed 1's valley: both send Workers, promote and form up, and their every command is permitted;
  - the same hashes of three such regions at 1, 2 and 8 workers as serially;
  - an AI battle restored and stepping on alike;
  - the golden run.
* T10 measures the costs of the recounts, the census and the AI on the reference test machine.

## Pros and Cons of the Options

### 1 — region AIs through the queue

* Good, because the AI reads its region only, and acts as a steward does.
* Bad, because the AI must account for its own pending actions.

### 2 — the reference's bot controller

* Good, because its actions are immediate, as the reference's.
* Bad, because it bypasses the command path.

### 3 — the AI in the world owner

* Good, because one AI could read several regions.
* Bad, because it reads another owner's state.

## More Information

* Related: [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md), [ADR-099](ADR-099-world-snapshots.md), [ADR-104](ADR-104-navigation-nodes-land-route-graph-and-routes.md), [ADR-105](ADR-105-combat-weapons-and-essence-orbs.md), [ADR-106](ADR-106-leaders-formations-and-battle-commands.md).
* The rules: R-BOT-025 to R-BOT-055 ([navigation graph](../reference/07_bots/01_navigation_graph.md)), R-BOT-072 to R-BOT-095 ([decision loop and Leaders](../reference/07_bots/03_decision_loop_and_leaders.md)), R-BOT-006 to R-BOT-024 ([bots](../reference/07_bots/README.md)), R-UNIT-188 ([Leader and formations](../reference/02_units/leader_and_formations.md)), R-UNIT-098 ([Worker](../reference/02_units/worker.md)).
* The code is in GameLogic (`BotNavigation`, `AiController`, `Combat`, `RegionSimulation`).
* **Amendment 2026-10-20 (the formation measurement, after M1 task T10).** Two measurements tested the stall that the Consequences describe, and the rules stay as they are. Both ran a probe program over `RegionSimulation` on seed 1's valley, built with g++ 13 on Linux. The outcomes are deterministic on that build, and another build may differ in detail.
  - **The formation-Leader radius.** The sandbox battle, with the AI on both sides, ran for 30,000 ticks under two simulation keys, at five values of `ref.bot.leader.formation_leader_radius`. Losses of both sides together, under the two keys:
    - 300 wu, the reference's: 28 and 21;
    - 460 wu, above the 453 wu diagonal of MVP-017's 320 wu lattice: 46 and 35;
    - 640 wu: 135 and 141;
    - 800 wu: 177 and 155;
    - 960 wu: 177 and 178.

    At 460 wu each side still held 3 to 6 Leaders. A Leader with a waypoint order does not count for R-BOT-091, so a node whose Leader has walked away promotes another. No battle ended at any radius.
  - **A full formation against a crowd.** A Leader and 90 Workers were formed up and filled by the steward's commands, then ordered at an idle crowd of 90 Workers 400 wu away. The run was repeated under 12 keys. The formation closed in every time.
    - It won clearly in the 4 runs in which the crowd threw one grenade: 45 to 50 of its 91 were left, and the crowd was cut to 10 to 17.
    - It was destroyed in the 7 runs in which the crowd threw 5 to 9 grenades: 0 to 7 were left.
    - In one run, with 6 grenades, it came out about even: 18 against 22.
  - **Why the grenades decide it.** A grenade's shockwave ring passes over everything 29 to 59 wu from it on 2 or 3 ticks, at about 40 damage each time (R-CRATE-471, R-CRATE-472). A member's explosive damage × 0.6 (R-UNIT-184) still kills a Worker of 50 health, and a packed formation holds dozens of members in that band. Two overlapping explosions killed 30 members in one tick.
  - **The members' threat search.** Centring it nearer the Leader made the formation weaker: with `ref.unit.worker.formation_search_offset` at 0.33 or 0 instead of 0.66, it killed 19 and 18 instead of 28, under the first key.
  - **A formation that the AI forms.** It took over 1,000 ticks to fill, and did not engage the crowd in 3,000 ticks.
  - **So the radius stays the reference's 300 wu.** A larger radius gives the AI fuller formations, which lose to a crowd that throws grenades more often than they win, and it does not make a battle end. How formations fare against grenades is DEC-111's and DEC-112's question, decided by playtest.
* **Amendment 2026-10-21 (M2 task T5, [ADR-131](ADR-131-structure-step-collision-shapes-and-ports-at-m2.md)).** A node is important when a structure with at least one port stands strictly within `ref.bot.node.important_radius` of it (R-BOT-049). The structures advance in step 4 before the nodes.
* **Amendment 2026-10-22 (M3 task T3, [ADR-158](ADR-158-the-seen-layer-and-its-readers-at-m3.md)).** A node's enemies of a team are the teams the region makes its enemies, so settlements at peace do not count each other (ADR-157), and a node held by an enemy is one whose owner is such a team. An AI reads each node through its settlement's seen layer: a node whose census disc the layer does not hold reads as one no team owns and nothing is counted at, and a Leader's policy counts only the enemies it sees; the sandbox's teams read every node. The AIs now decide after the settlements step and their seen layers are made.
