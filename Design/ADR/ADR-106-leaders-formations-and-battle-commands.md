---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T7)
---

# Leaders, formations and battle commands

## Context and Problem Statement

Task T7 of the [M1 plan](../implementation/m1_plan.md) gives the battle its Leaders and its commands (MVP-040, MVP-094, MVP-101). It covers:
- promotion as a command, because the MVP has no Powers (O70);
- joining a Leader: the Worker's Leader search and FollowingLeader (R-UNIT-096, R-UNIT-074);
- the formation block and its slots (R-UNIT-177 to R-UNIT-184);
- the Leader's orders, its order markers and their trail marks (R-UNIT-163 to R-UNIT-176, R-UNIT-185 to R-UNIT-187);
- the battle verbs of the plan's §6 as commands through ADR-087's checks, each with its outcome message (ADR-095);
- the AI's own actions as command types that only an AI sender may send (DEC-122, O70).

The reference runs every participant's commands as one batch and lets its bots apply their operations directly (R-SIM-444, R-BOT-015). These decisions are open:
- how the commands are encoded and which owner's queue takes them;
- who may command a team, and how that survives a restore;
- how the outcome reaches the client;
- where a formation's membership lives;
- the readings that the reference's prose leaves to the implementation.

## Decision Drivers

* Every command takes the one validated path, checked on arrival and on execution (MVP-094, ADR-087 items 1 to 3).
* A command names its figures by stable id, never by pool slot, and carries no in-band sentinel (R-SIM-449, DEC-202).
* A region reads and writes no other owner's state (ADR-085 item 2), and a restored world gives the hashes of a world never stopped (ADR-099).
* The crowd and Leader rules stay as the reference states them (MVP-040), and every number is a tuning value (MVP-099).

## Considered Options

* 1 — one message type per reference command, queued by the region; the region's state holds each team's commander, which the arrival check reads; the AI's actions are their own types, permitted to an AI commander only; a formation's slots live in its Leader
* 2 — one battle-order message with an operation code, its fields optional by operation
* 3 — the commands queued by the world owner, which then writes the region's figures

## Decision Outcome

Chosen option: "1".
- Option 2 makes each field optional by operation and moves the field checks into the handler, against ADR-088 item 5's schema per type.
- Option 3 has one owner write another's state (ADR-085 item 2).

* **Messages.** `CommandMessages` (GameShared) holds the messages, written by hand in the form the generated codecs will take (ADR-062, ADR-093). `GameMessageType` gives them the ids after `WorldTerrain`:
  - `CommandReport`, the outcome of one command;
  - the reference commands `Select`, `PromoteWorker`, `DemoteLeader`, `OrderUnit`, `ToggleUnit`, `SelectWorkers`, `OrderWorkers` and `ControlState` (R-SIM-450);
  - the AI's `AiOrderWorker`, `AiOrderLeader` and `AiLeaderFields`.
  - Each names its team. A figure is a 64-bit stable id; an optional one has a presence bit. A point is the x and z of a ground point as binary64; the server takes its height from its terrain and clamps it into the region's square.
  - A codec fails its reader for a field out of its bounds: a real that is not finite, an id of 0, a negative radius, an unknown operation. The queue then drops the command as malformed. `BitReader::Fail` is the codec's way to do so (ADR-093 amendment).
* **Routing.** The world server reads a reliable message's header. The queue of the MVP's one region takes it when it handles the type (`CommandQueue::Handles`, ADR-095 amendment), and the world owner's queue takes everything else and refuses what it does not know.
* **Commanders.** The region holds, by team index, the sender that commands the team and whether that sender is an AI.
  - The arrival check permits a command when its sender commands the team it names. An AI command needs an AI commander. So a steward commands only its own side (PIL-006), and only the AI sends the AI's actions (DEC-122's option C, as O70 states for M1).
  - The table is region state, written before the queue, so the queued commands of a snapshot pass their arrival check again on restore (ADR-095).
  - It is set between ticks, as a connection is made. T9's sandbox binds the steward's side and the AI's. Until then the server binds no steward, so it refuses a steward's battle command as not permitted.
* **Outcomes.** After each tick the world server takes the outcomes of the world's queue and of each region's. Each goes to the steward that sent the command, as a `CommandReport` on the reliable channel: the command's sequence number as the server counted it (ADR-095), its tick, and its result. The result is applied, no effect, or one of the six refusals.
  - A report the backlog refuses waits, with those after it, for the next tick.
  - An AI's outcomes are not sent anywhere: the AI of M1 reads none.
* **Execution checks** (R-SIM-453, ADR-087 item 3). A named figure qualifies while it is in its team, of the command's kind, alive and enabled, and neither destroyed, promoted nor demoted. Otherwise the command has no effect.
  - A Burning Worker is not promoted (R-UNIT-153).
  - A Burning or Drowning Worker takes no order: both states end only in death (R-UNIT-079, R-UNIT-080).
* **Selection** (R-SIM-401, R-SIM-405).
  - Each team holds its selected Leader by stable id and slot, which resolves only while that Leader holds the slot, and its pointer position from its latest control state (R-SIM-442).
  - A selection change takes every Worker of the team out of the crowd selection (R-UNIT-137).
  - The selected-Worker centre of R-SIM-402 is not held: only presentation reads it. The crowd order takes the mean of the Workers under control as it executes (R-SIM-455).
  - The crowd-order interval is the input layer's (R-SIM-403).
* **The verbs.**
  - Promotion spawns the Leader by the spawn step at the Worker's position with its velocity, and selects it.
  - A toggle selects the Leader in both its branches, so that one `ToggleUnit` makes one selection change (R-SIM-454).
  - The control state carries the pointer alone, the one field the MVP reads. Each replaces the last, so the latest of a tick holds (R-SIM-443).
  - The AI's waypoint order to a Worker has no route (R-BOT-086). Its Leader orders are `AssignOrders`, `AssignWaypoint` and `FormUp`, and its Leader fields are the no-formations flag and the bot target front (R-BOT-015). It promotes with `PromoteWorker`, because the MVP has no promotion Power (O70).
* **The Leader** holds its order, its demoted and no-formations flags, its bot target front, the tick of its last order marker, its formation flag, its formation angle and its slots. Its order position and order route are the figure's order position and route, so route freeing keeps an order route while its Leader holds it (O71).
  - Its advance follows R-UNIT-163: the removal of a demoted Leader; PrepareWaypoint for None; its behaviour, the GivingOrders of R-UNIT-167 included; the facing of the pointer; the order markers; the formation upkeep; the bot target front; and the rebuilt route. The MVP has no teleport for step 7.
* **The formation.** A Leader's slots name their members by stable id.
  - A Worker that follows a Leader holds that Leader's id and slot. It is in the formation while it holds a slot of that Leader and the Leader is in formation mode. The reference's "occupies a slot" takes the Leader's mode with it, because the slots freeze while the formation flag is clear. Nothing else records membership, so it cannot go stale, and the figure's own formation flag of state versions 2 to 6 is dropped.
  - The explosive factor of R-UNIT-116 reads this membership through the region's damage operation.
  - The threat search's centre moves along the Leader's front from the member's own position, as R-UNIT-098 step 7 states.
  - The volley rhythm reads the region's tick, in integer arithmetic.
  - The row-size table is an object of its parts in the record: each tier's largest array and its row size.
* **Order markers and trail marks** are objects of the region's effect pool (R-UNIT-187).
  - A marker holds its route while it lives, and route freeing counts it as a holder.
  - A trail mark's life counts from its creation tick, so it lasts its ten ticks whether or not its slot advances in the tick it was made (DEC-455).
  - Rendering them is presentation (T17).
  - The Leader and selection sounds are effect events: a promotion, a Leader selected, a waypoint order, a formation order, Workers selected, a Worker that took a Leader's order, and a crowd order.
* **Readings of the reference's prose**, kept as rules here:
  - The Leader searches measure in 3-D and take Leaders strictly within their ranges. The team's slot order stands for the order of its special units. A dead or demoted Leader is no candidate.
  - "Walkable, cliffs included" is the path test with evaluate-cliffs set.
  - The follow and signpost waypoints pass the Worker's push with kill clear, as an order's does (R-UNIT-115).
  - "One unit length" towards a click is 1 wu.
  - A Leader is walking while its speed is at least `ref.ctrl.leader.face_pointer_speed`, for step 8 as for R-UNIT-168.
  - Facing the pointer turns at the factor of facing an order target.
  - `FormUp` sets the front towards its target at once. A formation that has turned far enough sets its angle and walks in the same tick.
  - A Worker forgets its Leader when it stops following (R-UNIT-074), and keeps it through a fight (R-UNIT-101).
  - Steps 5, 6, 8 and 9 run for a dead Leader too, as the pipeline lists them.
* **Not built in the MVP:**
  - teleports and Beam Dishes, which R-UNIT-163 step 7, R-UNIT-169 and R-UNIT-171 test for;
  - the tutorial cases, and Leader mode cycling, which has no effect in a match (R-UNIT-174);
  - scrapping (`ScrapActor`), which no verb of the plan's §6 uses;
  - the control state's other fields;
  - the presentation of R-UNIT-190 to R-UNIT-193 and of the order route's preview, which arrive with T16 and T17.
  - The Leader's bot hook (R-UNIT-188) arrives with T8. The rate limits of ADR-087 item 6 arrive with remote clients.
* **Values in data.** The record holds the `ref.unit.leader.*`, `ref.unit.worker.*`, `ref.ctrl.leader.*`, `ref.sim.command.*` and `ref.sim.team.*` keys these rules read (ADR-098 amendment).
* **State.** `WORLD_STATE_VERSION` becomes 7.
  - Each region writes its commanders before its queue, and each team its selection and pointer after its kills.
  - Each figure writes its promoted and under-control flags, its Leader, its follow timer and the Leader's fields, and no longer its formation flag.
  - Each effect writes a marker's route, route point and arrival, and a trail mark's creation tick.
  - A region of an older version reads with no commander, and its figures with no Leader and no order.
  - The golden run of DBG-147 changes by the empty commander table, and the independent model computes the same values.

### Consequences

* Good, because every verb of the steward and of the AI takes the one path. Each is refused with its reason or executed with its effect, and its sender learns which.
* Good, because a formation's membership is read from its Leader and cannot go stale.
* Bad, because a command finds its figure by a scan of its team's slots, and the Leader search scans the team's figures at each Idle Worker's search. T10 measures both.
* Bad, because no steward commands a team until T9 binds the sandbox's sides, and the rate limits are not enforced yet.
* It forecloses: commands that name pool slots; membership stored in the Workers; an AI that writes the simulation outside the command path.

### Confirmation

* `GameLogicTests` checks:
  - the slot positions, row sizes, slot timeout, gap rule, full formation and volley rhythm of AC-UNIT-L03 to AC-UNIT-L06, AC-UNIT-L09 and AC-UNIT-L13;
  - promotion and demotion through the queue (AC-UNIT-L01, AC-UNIT-L02);
  - the toggle and waypoint clicks (AC-UNIT-L07);
  - the signpost (AC-UNIT-L08);
  - Workers that gather into a formation's slots and leave it when the Leader's orders are cancelled;
  - the order markers and trail marks (AC-UNIT-L10);
  - the crowd selection and crowd order;
  - the pointer a selected Leader faces;
  - the AI's Leader operations and fields;
  - a formation that holds its slots in a fight, fires and throws no grenade, and takes explosive damage × 0.6;
  - every refusal reason, counted, with the tick it would have executed in, and every executed outcome with its tick;
  - a region with commanders, formations, markers, a selection and a queued command reading back to the same state and stepping on alike;
  - the golden run.
* `GameLogicTests` sends a client's command through the transport. The client receives its outcome with its sequence number and tick.
* `NeuronCoreTests` checks a codec's own field bound. `NeuronServerTests` checks the types a queue handles.

## Pros and Cons of the Options

### 1 — a type per command, commanders in the region

* Good, because each type's schema states its fields and their bounds.
* Bad, because every new verb is a new type.

### 2 — one battle-order message

* Good, because there is one type.
* Bad, because its fields are optional by operation, and their checks move into the handler.

### 3 — commands through the world owner

* Good, because there is one queue.
* Bad, because one owner writes another's state.

## More Information

* Related: [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-088](ADR-088-transport-messages-identity-and-diagnostics.md), [ADR-093](ADR-093-transport-interface-and-message-layout.md), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md), [ADR-096](ADR-096-game-skeleton-integrated-server-and-tick-report.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md), [ADR-098](ADR-098-tuning-in-data-and-the-schema-generator.md), [ADR-099](ADR-099-world-snapshots.md), [ADR-104](ADR-104-navigation-nodes-land-route-graph-and-routes.md), [ADR-105](ADR-105-combat-weapons-and-essence-orbs.md).
* The rules: R-UNIT-074, R-UNIT-096, R-UNIT-098 to R-UNIT-104, R-UNIT-116 and R-UNIT-135 to R-UNIT-140 ([Worker](../reference/02_units/worker.md)), R-UNIT-152 to R-UNIT-189 ([Leader and formations](../reference/02_units/leader_and_formations.md)), R-SIM-401 to R-SIM-405 and R-SIM-438 to R-SIM-456 ([teams and player commands](../reference/12_simulation_model/06_teams_and_player_commands.md)), R-BOT-014 to R-BOT-016 ([bots](../reference/07_bots/README.md)).
* The code is in NeuronCore (`BitReader::Fail`), NeuronServer (`CommandQueue::Handles`), GameShared (`CommandMessages`, `GameMessages`, `EffectEvent`) and GameLogic (`Formation`, `LeaderOrders`, `OrderMarkers`, `RegionCommands`, `FigureAdvance`, `Combat`, `RegionSimulation`, `WorldServer`).
