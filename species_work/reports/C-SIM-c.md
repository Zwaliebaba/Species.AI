# C-SIM-c report — tick timing, randomness and integrity

Package: `species_work/packages/C-SIM-c.json` · Prefix range R-SIM-500 to R-SIM-799 · Phase 3.

## 1 Files written

| File | Size | Requirements | Tuning keys defined | Mermaid diagrams | Tables | Acceptance criteria |
|---|---|---|---|---|---|---|
| `design_next/reference/12_simulation_model/01_tick_timing_and_main_loop.md` | 54.9 KB | 61 (R-SIM-500 to R-SIM-584) | 41 | 5 | 9 | 14 (A1–A14) |
| `design_next/reference/12_simulation_model/07_randomness_and_integrity.md` | 70.0 KB | 83 (R-SIM-600 to R-SIM-743) | 4 | 3 | 7 | 18 (A1–A18) |
| total | 124.9 KB | 144, all unique, all in range, ascending | 45, no duplicates in the set | 8 | 16 | 32 |

Helper scripts (run with `python3 -I`): `species_work/scripts/C-SIM-c_dec.py` (DEC extraction), `C-SIM-c_speed.py` (re-check of the speed-request durations), `C-SIM-c_check.py` (structural checks).

**Single definition of the reference tick rate**: `ref.sim.tick_period` = 0.1 s (10 ticks per second), R-SIM-500 in 01. Sibling files 02–06 already cite it.

Enumerations carried over in full: the slice assignment table; the steady-state pacing table and the 60 fps worked example; the tick-clock fields; every speed request with its parameters; the speed-request duration vectors (11 rows, re-computed and matching); the nine reference clocks; the in-match frame order (set-up, 10 steps plus 16 client steps, teardown); the cosmetic draws moved to presentation generators (12 rows); the unbounded rejection loops (14); the initialisation draws (R-SIM-626); the per-family draw census (Table 1, 53 rows); the state classification (Table 2, 31 rows); the presentation-draw sites (Table 3, 18 rows); the render paths that write simulation fields (Table 4, 10 rows); the local-state crossings (R-SIM-660 to R-SIM-669); the layering rules and integrity violations (R-SIM-670 to R-SIM-675); the six object-model rules; the bot integrity rules; the per-subsystem notes; the nine non-finite guards; the replay hook.

## 2 Checks

- `scan_patterns.py` with `forbidden_patterns.tsv` on both files: no hit.
- `scan_patterns.py` with `soft_patterns.tsv` on both files: no hit (the word "lockstep" is not used).
- Every requirement ID is in R-SIM-500 to R-SIM-799 and unique; every tuning key is unique across `design_next/` and cited by a requirement in §2 of its file.
- Every cited DEC (22 in 01, 42 in 07) and ADR (ADR-0012, ADR-0022, ADR-0036, ADR-0074) exists; no asset id is cited (`assets_for_you` is empty).
- Every `decs_to_cite` entry, Open and Resolved, is cited inside at least one requirement.
- All eight template headings of §6.3 are present in both files; no truncation word in prose.
- Mermaid blocks were checked by eye only (no Mermaid package in this environment): flowchart, stateDiagram-v2 and sequenceDiagram with quoted labels, no semicolons or hashes.

## 3 Open DECs cited

DEC-003, DEC-046, DEC-101, DEC-200, DEC-203, DEC-206, DEC-220, DEC-222, DEC-223, DEC-224, DEC-225, DEC-227, DEC-240, DEC-242 (all of the package's Open entries), plus DEC-241, DEC-246, DEC-318 and DEC-455.

Resolved DECs stated as fixed rules: DEC-002, DEC-004, DEC-005, DEC-006, DEC-007, DEC-008, DEC-009, DEC-010, DEC-011, DEC-012, DEC-013, DEC-014, DEC-015, DEC-016, DEC-024, DEC-036, DEC-038, DEC-039, DEC-087, DEC-090, DEC-092, DEC-093, DEC-095, DEC-103, DEC-117, DEC-118, DEC-119, DEC-456, DEC-465, DEC-555, DEC-652, DEC-653, DEC-654 (DEC-114 is cited for the drop position).

## 4 New DEC requests

1. **Tick scheduling of game speed (01 R-SIM-548, R-SIM-549).** DEC-039 makes game speed tick scheduling and DEC-092 makes the speed-hold counters scale with speed, but neither says what keeps running at speed 0: the speed blend, the pause and resume commands, and the grace countdown all run in the reference while the world is held. The documents state, as the conservative default, that command frames, the speed blend and the held-world match-rules update keep the real-time cadence of one per `ref.sim.tick_period`, the hold countdown counts world ticks, and the fractional part of speed × ticks is carried. The exact scheduling and rounding rule is pointed at DEC-225; a dedicated Open entry (or an explicit item in DEC-225) is recommended.
2. **Real duration of the speed-crate holds under DEC-092.** With the hold counted in world ticks, a Time Dilation at 0.2× holds for 150 ticks, that is 75 s of real time instead of the reference 15 s, and a Time Rush at 2.0× for 7.5 s. DEC-039's "for 15 s" reads as real time. The owner should confirm which is intended (01 R-SIM-557).
3. **Presentation frame delta while paused (01 R-SIM-571).** The reference keeps the last frame delta while the world is held, so camera and UI keep moving at the pace of the last frame. ADR-0036's presentation clock stops when paused, and DEC-024 asks for one presentation clock for camera motion; which clock the camera and UI use during a pause is not decided.
4. **Decorative objects in simulated pools (07 Table 2, §5).** Snowflakes, Wisps, muzzle flashes and shell casings live in the simulated effect pool and take simulation handles, so they shift later serials, which rules read (03 R-SIM-138). DEC-222 moves only their draws. Whether they leave the simulation containers is open.
5. **Essence orb hover draws (07 Table 1).** The units document of the corpus calls the orb wobble cosmetic, but orb positions are simulation state (collection) and DEC-222 item 7 does not move these draws; they are kept as simulation draws. Confirm.
6. **DEC-116 versus the start hold.** DEC-116 states that Supply crate drops continue during the grace period, but the reference holds the world (game speed 0) until the grace countdown ends, and the Supply crate drops run inside the world step in slice 8 (01 R-SIM-550 to R-SIM-552; 04 R-SIM-218). The two statements disagree; the coordinator should check DEC-116.

## 5 ADR conflicts

None. R-SIM-573 cites ADR-0012 (frame delta clamp kept), ADR-0036 (presentation clock for effects) and ADR-0074 (injectable platform clock); item 3 above is a gap, not a contradiction.

## 6 Missing assets

None.

## 7 Cross-document requests

Values from my input ranges that belong to other documents. Rows marked "present" were found already written in a sibling file; the others need their owner to carry them (or the coordinator to confirm they are covered from the owner's own ranges).

| Value (from my ranges) | Owner | Status |
|---|---|---|
| mode second = 11 ticks; 01 R-SIM-503 refers to "the mode-second constant of reference/01" without a key name | G01 (`reference/01_skirmish_rules_and_modes/README.md`) | please define the key and give its name; I removed my `ref.sim.mode_second` to avoid a duplicate |
| grace period 3.0 s = 30 ticks, cited as `ref.match.grace_period` | G01 | to define |
| Time Dilation 0.2 + [0, 0.2), Time Rush 1.3 + [0, 0.7), blend rate 0.05, hold 150 ticks; meteor slow-motion 0.4, rate 0.1, hold 40 ticks, trigger 200 wu | defined in 01 as `ref.sim.speed.*` | G04 (crates and powers) and G02b (meteor) should cite these keys instead of defining their own |
| team slots 6 (`ref.sim.max_teams`), Rogue and Visitor slots 4 and 5, colour ids 8 and 9 | 06 | present |
| Rogue colour (10, 10, 10), Visitor colour (200, 200, 200); 8 team colours; Siege attacker and defender colour ids 1 and 0; match-option parameter capacity 64 | 06 or G01 / G09 | check |
| global gravity constant 10 wu/s² ("entity physics"); 05 uses 15 wu/s² (actors) and 98 wu/s² (thrown) | 05 | check which rules, if any, use the value 10 |
| camera border 200 wu; fallback camera (world size x, 1000, world size x) looking (−1, −0.7, −1); front-end camera (−900, 3000, 3970) looking (0, 0.5, −1) | 02 | present |
| actor-grid and obstacle-grid cells 32 wu (obstacle grid 64 wu outside matches); walkability (step 20, wet limit 40, gradient 2.4, normal y 0.8, wall sphere 13.2, Barrier Emitter sphere 10); walking cost (step 40, wet limit 40, uphill 1.5, obstacle sphere 20); spawn search 10 tries and 20 wu clamp; route point reach 10 wu | 05 | present |
| visibility tolerance 20 wu | 05 | check |
| container growth steps (lights 1, structures 10, Essence orbs 100 with initial size 100, laser bolts 100, effects 100, ungrouped actors 100, actor groups 5, group members 100); Essence orb spawn offset +1 wu up | 04 | check (orb lift present) |
| forced match power and research levels | 02 | present |
| kill milestones 500, 1000, 2000, 3000, 4000, 5000 | 04 | present |
| map-effect spawners: shared timer start −99.9 and burst threshold below −90; snow 150 initial, one per 0.1 s at height 700–1000; orb rain one per 1.0 s at height 700–1000; dust: a vortex every 15–30 s with a 1-in-10 chance, 300 initial, two per 0.1 s, size + 0–20, capture radius 50, land test height above 1.0, vortex height 0–500; lightning every 0–15 s at height 1200 | 04 §2.8 (R-SIM-233) / G04 / G08a | check |
| Supply crate cap ⌊world size x / 500⌋; interval = drop-rate option × (0.5, 1.5]; placement on land of height ≥ 20, weighted ±100 around a structure, 1000 / 100 tries, separation = scan range × 10, start height + 0–200 | G04 | check |
| picking factors (actor group sphere ×1.5, group member ×1.5, actor ×1.0, structure ×0.75, Transport range 0.001); mouse fallback sphere world size × 40 | G06 / C-INP | check |
| thrown-weapon force √distance × 8 capped by the research maximum; turret round life by research 0.5 / 0.5 / 1.0 / 1.5 / 2.0 s, 0.6 s in matches; laser life by research 0.2 to 1.0 s, ×2 under Convert; muzzle flashes (thrown and rocket 20 and 3; turret 60 and 2 at + half a tick; laser 25 × life and 1.5); starting Power timer 30 s | G04 / G03b | check |
| blast cores r·d·0.01 (+0–100 %), debris max(2, r·d·0.01) up to 30, shockwave r/20; promotion flashes 5 + 0–4 particles with velocities ±2.5, 0–15, ±2.5 | C-VFX (05 has the damage reach and gain) | check |
| fog start = end / 3, end = max(4000, world size x + 1000, world size z + 1000), linear, density 1.0; seasonal light (1.3, 1.2, 1.2); special light direction (0, 1, 0.5), colour (2.0, 1.5, 0.75), shininess 10; Crawler texture 160 × 512, 32 copies, glow 128; render detail distances (Crawler 300 / 500 / 600 / 1000, Worker 500 / 1000 / 99999.9), alpha references 0.02 / 0.04 / 0.01, near-plane factors 1.05 and 1.2 | C-RND | check |
| bot link cull ratio 0.8 | G07 | check |
| centre message 3.0 s, size 40, at y 370 for a 1280-pixel width, growing 15·√(2 − t) in its last 2 s; chat font 14 × scale, fade from 55 to 60 s, wrap at 40 % of the width, box 25 % of the width; front-end fade-in 0.25 s; publisher-logo fade 1.0 s in and out; seasonal date window 24 to 26 December; font spacing 0.91 and 1.03, horizontal scale 0.6 (Latin) and 1.0 (CJK); large-menus value 2; lobby backdrop detail switch (smoothing factor frame delta × 0.5, low detail below 20 fps, high at 60 fps) | G09 / C-UI | check |
| crowd-order Transport search radius 30, particle colour (255, 100, 255), 1000 iterations; order throttle 1.0 s; selection circle growth 100 wu per real second up to 100 wu; Breach Charge direct route at 25 % of the selection; selection sound throttle 0.5 s; turret selection fire timer 0.5 s and cursor boost 2.0; team command state camera acceleration 6/256 when the squared offset exceeds 175, target length cap 125 | G06 / 06 | check |
| Leader formation cap 90; formation spacing 4.0; airstrike row 4 with multipliers 6 (x, z) and 3 (y) | G02a / 03 | check |
| the string-table swap delay (a deferred job that runs on its sixth poll) | C-UI | lesson in DEC-221 item 8 |

Other requests:

- **04 (C-SIM-a)** already cites my slice keys and R-SIM-510, R-SIM-511 and R-SIM-514; R-SIM-516 in 01 refers to 04 for the slot-append block sizes.
- **All reference documents** that state game-speed effects should cite `ref.sim.speed.normal` and `ref.sim.speed.stopped` rather than literal speeds.

## 8 Names needed that are not in the ledger

| Needed for | Proposal | Where used |
|---|---|---|
| the gamepad control-hint system that the in-match loop shuts down and advances | "control-hint overlay", `ControlHintOverlay` | 01 R-SIM-581 |
| the map effect that rains Essence orbs and that no reference map enables | "orb-rain map effect", `MapEffect::OrbRain` | 07 Table 2 |
| the host-side tick source | "command sequencer" (prose only; no identifier, because the networking model is deferred) | 01 §2.3 |
| one sequenced batch of commands, one tick in the reference | "command frame" (prose; already used by DEC-200) | 01, 07 |
| tick-clock presentation fields | "tick-take time", "extrapolation time", "smooth presentation time", "presentation frame time", "presentation frame delta" (prose field names of `TickClock` and the frame loop) | 01 §2.5 to §2.8 |
| the in-match menu window and the controller-disconnected dialog | prose only | 01 R-SIM-581 |

## 9 Dropped, one line per group

- Operating-system clock specifics and the fake-clock capture paths; the capture frame-rate value of the recording hook (the hook itself is kept without its number).
- Graphics-API present and vertical-sync specifics and the mechanics of the worker-queued graphics jobs (the frame step is kept generically).
- The disabled parallel-loop annotation of the world step and the project settings around it (only the serial sub-step order is kept).
- Networking: non-update sequenced messages, the per-frame divergence byte and its timing, latency statistics, the loading keep-alive interval, the leave burst, ports, online-service threads and their periods, authentication retries, the test-mode seed override, late-join re-simulation (DEC-200, DEC-206, DEC-244 hold the lessons).
- Generator internals: algorithm name, seeding constants, the presentation generator's arithmetic constants, the session-seed hash, an orphan duplicate generator and an unused clamped-normal helper (DEC-242).
- Numeric notes on precision and trigonometric wrappers, the tolerance constants, the floating-point control row (DEC-240, DEC-241).
- Thread affinity masks (DEC-224).
- Content outside the library: blocked structure types (the fence switch, research item, swarm building, scripted trigger, construction yard, blueprint relay, dynamic node and orb receiver), the tri-walker and the duel vehicle, the Leader's legacy shield and absorb, sacrifice by the dropped summoner, the box kite, the campaign world, cut-scene scripts, the summoner interface, the debug campaign-event pass, the advisor skip, and the focus-loss pause of campaign modes.
- Provenance tables: call-site counts per source file and the local-state reference census.
- Original numeric ids: the fixed match-world id and the no-team sentinel value (§6.2).
- The difficulty-scaling coefficients (inert: the level is 0 in every match, DEC-010).
- The debug camera mode selected when the in-match menu opens (DEC-024 drops debug modes).
- Two unused frame-loop constants (a minimum render period and a frame-rate increment) and the dead time-based branch of the pacing controller.
