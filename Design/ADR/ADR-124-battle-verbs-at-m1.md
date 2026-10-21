---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T17), owner (O75, O78)
---

# The battle verbs at M1

## Context and Problem Statement

M1 task T17 gives the steward the battle verbs (M1 plan table 2 row T17; MVP-101). The server has executed T7's commands since ADR-106. T17 lands in two steps:
- T17a, this record: the client's side of the verbs, the selection circle and the outcomes;
- T17b: the markers, the route preview, the flag pointer, the world markers and the cursor badges, in its own record, [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md).

Six things in the design do not hold as written at M1:
1. **No command path.** The client has no way to send a battle command, and it drops every `CommandReport` the server sends.
2. **Selection latency.** The client sees its own commands' effects about two ticks late: the command executes at the next tick, and the replica is drawn one tick behind. A right-click just after a selection circle would read "no Workers selected" and promote instead of ordering the crowd.
3. **The circle is not additive.** The server's `SelectWorkers` releases the whole selection before it selects (ADR-106's reading of R-UNIT-137). R-CTRL-156 and R-CTRL-158 make it additive, with only a `Select` releasing the group (R-CTRL-161).
4. **The control state's cadence.** R-SIM-438 sends the control state every tick. At M1 each one is journaled (MVP-076) and answered with a report, while only a selected Leader reads it (R-UNIT-168).
5. **No promotion cue.** Nothing shows which Worker a right-click will promote (R-CTRL-356's ring is not built at M1, ADR-119).
6. **Server gaps.** The server does not check every rule of the input layer: the crowd-order interval (R-CTRL-160), toggling a Leader while another is selected (R-CTRL-050 step 2), and a target off the region, which it clamps.

## Decision Drivers

* The reference verbs as written, wherever M1 reaches them (MVP-101), with each deviation named.
* The input layer never writes simulation state: what the client computes is local (R-CTRL-151, R-SIM-403).
* One press, one meaning, one command (R-CTRL-048, R-CTRL-050).
* Every value in data, in the client's own record (ADR-116).

## Considered Options

1. **A pump of the verbs in the settlement view, with a local prediction.** The client resolves each frame's input against the replica's latest selection and the effect of each command it has sent but not yet seen.
2. **Resolve against the replica alone.** The steward waits for each command's effect before the next press means what it should.
3. **Predict the whole simulation.** The client runs the commands on a copy of the region.

## Decision Outcome

Chosen option: 1, because it makes a press resolve as the steward's previous press left the selection, with no simulation on the client. Option 2 turns a quick circle-and-order into a promotion. Option 3 is ADR-084's prediction of a unit, which the MVP does not build.

* **The command path** (`ClientSession`):
  - **Sending.** `SendCommand` sends a command on the reliable channel and returns its sequence number. The server numbers every reliable message of the connection in order, the sandbox restart's included (ADR-095, ADR-109), so the client counts each send the same way.
  - **Outcomes.** The session decodes each `CommandReport` and keeps it until taken. A refused command is logged as a warning.
* **The bindings.** Table 3.6-A rows 19 to 24 and 40 to 45, compiled in (O75, ADR-119): `action.select.pick`, `pick_idle`, `circle_begin`, `circle_grow` and `circle_end` on the primary button, `action.select.clear` on Space, `action.order.crowd`, `promote` and `special` on the secondary button, `action.order.straight_route` on Ctrl held, `action.order.move_modifier` on Shift held, and `action.order.demote` on C.
* **The pump** (`GameClient/BattleVerbs`, run by the settlement view after the camera, R-CTRL-020 step 8):
  - **The primary press** (rows S1 to S6):
    - an own Leader under the pointer is selected, unless it is selected already (R-CTRL-062);
    - on empty ground with nothing selected, one `Select` of nothing is sent and, when no Workers are selected, the circle begins (R-CTRL-048, edge case 1);
    - with a Leader selected, a press on the ground is row S2, which has only a sound.
  - **The circle** grows by `ref.ctrl.select.circle_growth` (100 wu a real second) up to `ref.ctrl.select.circle_max_radius` (100 wu) while held. It sends `SelectWorkers` at most once a tick (R-CTRL-156). S4's conditions are read once, when it begins. Its release resets the crowd-order time (R-CTRL-157).
  - **Space** sends a `Select` of nothing (row S3).
  - **The secondary press** resolves in R-CTRL-050's order:
    1. **Toggle.** An own Leader under the pointer, while no Workers are selected and nothing or that Leader is selected, is toggled (row O3).
    2. **The selected Leader's order** goes to the order target (R-CTRL-057): the position of an own Leader under the pointer, or the ground point. Shift gives a move-only order and Ctrl a straight route (rows O6, O7).
    3. **The crowd order** needs Workers selected, no circle and a target on the region, at least `ref.ctrl.order.crowd_interval_ticks` (10) ticks after the last crowd order or circle send (row O1, R-CTRL-160).
    4. **A promotion** of the nearest own Worker that is not dead or Burning within `ref.ctrl.promote_radius` (10 wu) of the ground point (row O2).
  - **C** demotes the selected Leader (row T5, R-CTRL-044).
  - **A dead selected Leader** is released once with a `Select` of nothing (R-CTRL-109).
  - **The match time** is the replica's latest tick. **The positions** are the frame's drawn figures.
  - **No team.** Nothing is sent while the client commands no team, as with `--watch` (O77), or before it holds a state.
* **The prediction.** Each command sent records the selection it leaves:
  - a `Select` leaves its Leader or nothing;
  - `SelectWorkers` leaves Workers;
  - a promotion leaves an unnamed new Leader;
  - a toggle leaves its Leader;
  - a crowd order and a demotion leave nothing;
  - an order to the selected Leader may keep or clear it (R-UNIT-171), so it predicts nothing.

  The latest prediction whose report's tick the replica does not yet hold decides the selection. A report of anything but `Applied` drops its prediction.
* **The control state** (R-SIM-438, ADR-087 item 5). The pointer goes to the server while a Leader is selected, the only reader at M1 (R-UNIT-168), when it moved, at most once each `ref.sim.command.control_state_period` (1 tick). It is not sent every tick.
* **Off the region.** No order or promotion is sent while the unavailable cursor shows (O76). An own Leader under the pointer is still a target.
* **The promotion cue** (O78). The highlight cursor shows over the Worker a secondary press would promote, as it shows over the client's own Leaders (ADR-119 amendment).
* **The circle's flashes** (R-CTRL-370, VFX-220 row 1). At each step of `vfx.emit.step_rate` on the unscaled clock, the circle emits `ref.ctrl.cursor.circle_particles` (30) control flashes of its radius's size. They are evenly spaced on it, turn at `ref.ctrl.cursor.circle_spin` (0.5 rad a real second), and lie `ref.ctrl.cursor.circle_lift` (10 wu) above the terrain under each. A step due within the rounding of the frames' sum is taken, so the rate does not depend on the frame rate.
* **The server's circle** (ADR-106 amendment). `SelectWorkers` adds the team's living, unpromoted Workers within the radius that are not Burning and not already controlled, and releases nothing (R-CTRL-158). Only a `Select` releases the group (R-CTRL-161).

### Consequences

* Good, because the steward can promote Leaders, toggle their formations, order them and steer a crowd with the reference's buttons.
* Good, because a quick sequence of presses resolves as the steward meant, without simulating anything on the client.
* Good, because the pump is CPU code that CI tests with synthetic replicas.
* Bad, because a prediction can be wrong for an order to the selected Leader, which predicts nothing: the next press reads the replica, two ticks late.
* Bad, because a promotion's new Leader is selected before its id is known, so C cannot demote it until the replica shows it.
* Neutral, because the control state goes only while a Leader is selected, so the journal holds fewer commands than the reference sends.

### Confirmation

* GameClientTests (`BattleVerbsTests`):
  - the resolver's order (AC-CTRL-AB-01);
  - move-only and straight by input mode (AC-CTRL-AB-02);
  - the demotion (AC-CTRL-AB-05);
  - one press, one command (edge case 1, input T21);
  - the circle's growth and its once-a-tick sends (AC-CTRL-SP-09);
  - the crowd-order interval (AC-CTRL-SP-10);
  - the promotable Worker;
  - the prediction;
  - the release of a dead selection (R-CTRL-109);
  - the control state's cadence;
  - a client of no team.
* GameClientTests: the session's numbering and reports (`ClientSessionTests`), and the circle's flash rate at 30 and 144 frames a second (`EffectsLookTests`, VFX T10).
* GameLogicTests: the additive circle and the release by `Select` (`CommandTests`).
* The owner's T17 run, with its checklist at T17b.

## Pros and Cons of the Options

### 1 — a pump with a local prediction

* Good, because it is the reference's input layer, with the latency of a client–server game hidden where it changes a press's meaning.
* Bad, because the prediction is a second account of the selection, which the replica's must overrule.

### 2 — the replica alone

* Good, because the client holds no account of its own.
* Bad, because a press within two ticks of another resolves wrongly.

### 3 — predict the whole simulation

* Good, because every effect shows at once.
* Bad, because it runs the region on the client, which ADR-084 builds for one unit only and the MVP not at all.

## More Information

* Related: [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-087](ADR-087-command-path-validation-and-timing.md), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md), [ADR-106](ADR-106-leaders-formations-and-battle-commands.md), [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md).
* Owner decisions O75, O77 and O78; MVP-101; R-CTRL-041 to R-CTRL-044, R-CTRL-048, R-CTRL-050, R-CTRL-055, R-CTRL-057, R-CTRL-062, R-CTRL-073, R-CTRL-109, R-CTRL-155 to R-CTRL-161, R-CTRL-370; R-SIM-438; VFX-220.
* The code: `GameClient/BattleVerbs`, `ClientSession`, `SettlementView`, `InputBindings` and `EffectsLook`; `GameLogic/RegionCommands`.
