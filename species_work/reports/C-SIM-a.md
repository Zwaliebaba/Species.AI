# C-SIM-a report: object model and match lifecycle

## Files written

| File | Size (bytes) | Requirements | Tuning keys defined | Mermaid diagrams | Numbered tables | Acceptance criteria |
|---|---|---|---|---|---|---|
| `design_next/reference/12_simulation_model/README.md` | 9 259 | 0 | 0 | 1 | 2 (document map, decisions) | — |
| `design_next/reference/12_simulation_model/02_match_lifecycle_and_state_reset.md` | 41 557 | 31 (R-SIM-001 to 031) | 16 | 3 | 7 | 16 |
| `design_next/reference/12_simulation_model/03_entity_model_and_identity.md` | 56 631 | 45 (R-SIM-101 to 145) | 4 | 9 | 7 | 15 |
| `design_next/reference/12_simulation_model/04_containers_and_update_order.md` | 53 578 | 40 (R-SIM-201 to 240) | 42 | 3 | 6 | 16 |
| `species_work/scripts/C-SIM-a_check.py` | helper | | | | | |

Totals: 116 requirements, 62 tuning keys, 16 Mermaid diagrams, 20 numbered enumeration tables plus 4 tuning tables, 47 acceptance criteria. No file is over 120 KB, so no split was needed. Ids are numbered in three blocks (001, 101, 201), all inside R-SIM 001–299, and none is used twice.

Checks run: the forbidden scan gives 0 hits; the soft scan gives 0 hits (one "port of" was reworded). All 16 Mermaid blocks parse with mermaid 11.4.1 in headless Chromium, using a scratch copy of the checker. `C-SIM-a_check.py` confirms that every id is in range and unique, every cited DEC exists, every `decs_to_cite` entry is cited, there are no truncation words, no tuning key is defined twice in `design_next/`, every key is cited by a requirement, and every link points at a file on disk or a package target. One cited key is still undefined: `ref.match.grace_period`, which G01 owns (see the cross-document requests).

## Decision on the open conflict: does the stale Source Node id have any effect? (DEC-011)

**Finding: the stale id has no observable effect. X-DEC-5a is right, and X-DEC-4a's "production requests in that window are dropped" is not supported.** I decided this from ORIG 20 §5.3 and 22 §14.12, both in my ranges:

- The registry id is rewritten at the start of **every** Source Node advance.
- Its only reader is a Spawn Node's Essence-orb request. That request first fires at least 2 game seconds into a match, because the first production timer runs 20–40 ticks, and a Spawn Surge cannot be active at match start. Every Source Node advances in the first tick, so by the first read the id has been rewritten on every participant.
- On a map without a Source Node, a stale id from an earlier match fails the kind check and resolves to none.
- So no production request ever falls inside the stale window.

**How it is specified.** 02 R-SIM-024 makes the registry match state. It starts as "none" (DEC-011), each Source Node writes it at the start of each of its advances, and it resolves through a look-up plus a kind check. The rule states the ordering argument above without a number. Edge case §5 and AC-3/AC-4 cover it.

**Suggested DEC-011 amendment (coordinator).** Replace "stays stale until the new map's Source Node first advances" with: "stays stale until the new map's Source Node first advances, which always happens before any Spawn Node reads it, so the stale value has no effect".

## Open DECs cited (each by at least one requirement)

| DEC | Requirement(s) |
|---|---|
| DEC-106 | R-SIM-144 |
| DEC-201 | R-SIM-144 |
| DEC-206 | R-SIM-027 |
| DEC-221 | R-SIM-001; teardown step 3 |
| DEC-223 | R-SIM-002, R-SIM-022, R-SIM-027, R-SIM-201; handle-range keys |
| DEC-224 | R-SIM-219 |
| DEC-227 | R-SIM-219 |
| DEC-240 | R-SIM-107 |
| DEC-241 | R-SIM-107, R-SIM-145 |
| DEC-250 | R-SIM-031; teardown step 1 |
| DEC-251 | R-SIM-031 |
| DEC-312 | R-SIM-224 |
| DEC-455 | R-SIM-211, R-SIM-215, R-SIM-225 |

Also cited:

- Open DECs outside my list: DEC-046, DEC-066, DEC-243, DEC-304 and DEC-651.
- DEC-306 (Dropped): cited as superseded by DEC-455 in 04 §8.
- Resolved DECs stated as fixed rules: DEC-002, 004, 005, 006, 007, 008, 010, 011, 012, 013, 014, 015, 016, 024, 028, 039, 084, 087, 095, 103, 117, 118, 650, 652, 653 and 654.

## New DEC requests

1. **Multi-round Siege rebuild at a tick boundary (02 R-SIM-030).** ORIG 22 §14.14 recommends applying the round reset as a sequenced event at a fixed tick. DEC-066 (Open) only records that the reference rebuild point depends on each participant's pacing. I specified the tick boundary as the rule and cited DEC-066; I kept "the object serial counter is not reset" as the reference default (R-SIM-029). **Request:** a Resolved entry, or an amendment to DEC-066, that fixes the tick-boundary rebuild. The serial-counter reset stays open in DEC-066.
2. **Generation of laser-bolt and Essence-orb handles (03 R-SIM-127).** DEC-028 requires generational handles for every kind, orbs and bolts included. Taking their generation from the shared object serial counter would shift every later actor serial, and serials feed rules. I specified a per-slot generation that is independent of the serial counter. **Request:** add this to DEC-028, or confirm it.
3. **Team selection as a generational handle (03 R-SIM-142).** The reference stores the selected actor as a bare slot, so (inferred in ORIG) a selection outlives the actor's death and designates the slot's next occupant. I applied DEC-028, so a stale selection resolves to none. **Request:** DEC-028 should name the selection, or a new Resolved entry should.
4. **DEC-016 versus DEC-455 wording.** DEC-016 (Resolved) says containers iterate "in stable-id order, never in slot-reuse or hash order". DEC-455's default reproduces slot order with last-freed-first reuse. I specified the slot order that the documented allocation rules produce as the stable key (04 R-SIM-211). **Request:** reconcile DEC-016's sentence, for example "in the slot order that the documented allocation rules give".
5. **Placement of the actor-group offset table step.** DEC-652 says "a fixed step of match initialisation" without naming it. I made it the last initialisation step (02 table 2.4-A step 11). Please confirm.

## ADR conflicts

None. These are reference documents and cite no ADR.

## Missing assets

None. The documents use no asset ids.

## Cross-document requests

- **C-SIM-c (01, 07):**
  - 07 R-SIM-612 lists the initialisation draws as unused-slot colours first, then start positions. ORIG 21 §5.9, the exact initialisation order, puts start positions at step 3 (02 table 2.4-A) and the unused-slot colours at step 9, after team initialisation. Please reconcile, or tell the coordinator which source fixes the other order.
  - 07 R-SIM-730 says the Control Zone counter is "read by the Flag Chain rules and reset only by the start of a Zone Control match". My range (22 §14.12) gives:
    - readers: the Transit Gate reward choice (gun turret against airstrike, when zones held ≥ (count + 1) ÷ 2), the Zone Control win test and a score term;
    - recomputed only by Zone Control's per-second loop;
    - Flag Chain initialisation **adds** to it.

    Please align (02 R-SIM-019 and §5).
  - Time constants of 22 §14.9.1 and "one simulation thread at a time" (§14.9.5) are in my range. 01 R-SIM-541 to 557 and 07 R-SIM-732 already state them, so I wrote nothing new. I cite your keys `ref.sim.speed.stopped`, `ref.sim.speed.normal`, `ref.sim.slice.*` and `ref.sim.update_slices`, and I removed my own duplicates. 07 already cites my `ref.sim.match.presentation_seed`.
- **C-SIM-b (05, 06):**
  - 05 R-SIM-368 should add the attempt distribution (a distance `NextUnit(spread)`, then an angle `NextUnit(2π)`) and the acceptance test (on the map, with terrain height above `ref.sim.physics.sea_level`). 04 R-SIM-226 step 3 states both.
  - My range (21 §12) holds constants whose rules belong in 05, and I did not find them there:
    - the cliff push (slope above 1.0; spiral search from 0 to 50 wu in steps of 0.2 wu);
    - the separation push between actors (radius 2.0 wu, force 0.1 ÷ distance);
    - the route-generation performance warning (10 ms, diagnostic);
    - the route-trail fade (0.25 per second, presentation);
    - the actor-grid cache container (20 entries, growing by 20);
    - the neighbour-result growth (+100 or the requested count; DEC-028 forbids a fixed limit).

    Please confirm they are covered, or add them.
  - 06 R-SIM-422 (unused-slot colours): my range gives the draw as (draw mod 8) div 2, giving 0 to 3, colour only. Please add that formula with `ref.sim.team.player_colour_count` and `ref.sim.team.coop_colour_divisor`; 02 step 9 cites R-SIM-422.
  - 06 should hold the team selection as a generational handle (03 R-SIM-142, DEC-028).
  - I removed my duplicate `ref.sim.team.eliminate_damage_factor` (06 R-SIM-407 owns it) and my offset-table size key; I cite `ref.sim.group.offset_table_size`.
- **G01 (reference/01):**
  - Define `ref.match.grace_period` (30 ticks). 02 R-SIM-012 and R-SIM-029 cite it.
  - The Control Zone counter's readers and DEC-011 (02 R-SIM-019).
  - The multi-round Siege rebuild (02 R-SIM-029 and 030; DEC-066).
- **G02a1, G02a2, G02b (reference/02_units):**
  - The corpse fade: start value 100; 4 per tick on the ground (25 ticks) and 2 in the air (50 ticks, also 05 `ref.sim.physics.dead_fade_per_tick`), fixed at every game speed by DEC-039.
  - Which actor kinds run the base advance first, last or never (03 R-SIM-111 defers this to units).
  - transport.md should cite `ref.sim.state.transport_cycle_debounce` (3 ticks, per Transport; 02 R-SIM-021), not redefine it.
  - The serial-dependent Worker rules: Barrier Emitter clearance (serial mod 10), vortex lift height (5 × (serial mod 100)), airborne tumble ((serial mod 10) × 0.1) and render variation. 03 R-SIM-138 names them without numbers.
- **G03a and G03b (reference/03):**
  - Cite 02 R-SIM-024 for the Source Node registry decision and R-SIM-020 for the Spawn Node count reset, slot 5 included.
  - `ref.sim.structure.default_radius` (13 wu) is defined in 03.
  - The order register of 04 table 2.9-A lists the port occupant, ownership ties, the Population Limiter association and the Beam Dish receiver; please state the same keys.
- **G04:** the Spawn Surge and Frenzy timers count down in slice 8 (04 R-SIM-218 step 9). Each shot helper also creates a muzzle flash.
- **G06 and C-CAM:** I define the local picking constants (`ref.sim.query.*`, 04 table 2.10-A) and the match-entry camera bounds and fallback camera (`ref.sim.match_entry.*`, 02). Please cite them, or the coordinator moves them to their owner.
- **G09a:** the front-end camera keys `ref.sim.lifetime.front_end_camera_*` are in 02. Bot fill of empty slots is 02 R-SIM-006 and 06 R-SIM-433.
- **G10:** the kill-milestone presentation event (04 R-SIM-221). A team-change sound is bound only for Control Zones and Relics (03 R-SIM-122).
- **G07:** the DEC-312 default is stated in 04 R-SIM-224. The DEC-087 order (obstacle grid before the bot navigation links) is applied in 02 table 2.4-A step 7.

## Names needed that are not in the ledger (proposals)

- **Airstrike bomb**, the grenade kind that a Bomber drops. Used only as a lower-case description in 03 table 2.3-B. Proposal: `ThrownWeaponKind::AirstrikeBomb`.
- **Handle group markers**: "ungrouped", "structure pool", "Essence-orb pool", "laser-bolt pool", "effect pool". Written as prose only. Proposal: `SimObjectId::Group` with `Ungrouped`, `StructurePool`, `EssenceOrbPool`, `LaserBoltPool`, `EffectPool`.
- **Dust object** for one `DustVortex` effect. The ledger term "dust vortex" also names the shared vortex position. Proposal: keep `DustVortex` and use "dust object" in prose, or rename the type `DustMote`.
- **Member names in diagram 2.2-A**: `advance`, `onSpawn`, `applyHealthChange`, `pushOutOfObstacles`, `advanceAlongRoute`, `createActor`, `createStructure`, `initialiseFromRecord`, `assignTeam`, `sphereHits`, `meshHits`, `rayHits`, `resolve`, `spawnActors`, `isValid`. They were chosen so as not to repeat reference identifiers. Proposal: register them in CONVENTIONS §8.12, or treat them as illustrative.
- **On-screen control hints**, a process-lifetime service, written as prose only. Proposal: `ControlHints`.
- **Kill-milestone message string id**: none assigned. Proposal: `match.kill_milestone`, owned by G01 or G09.

## Interpretations for review

- **Group-slot reuse (04 R-SIM-208).** ORIG states two facts as confirmed: group creation places the group at an explicit slot, and explicit-slot insertion rebuilds the free list in ascending order. I derived the rule from these two facts.
- **Worker spawn front (04 R-SIM-226 step 6).** Read as: a Worker always takes a random horizontal front; every other kind takes the normalised velocity, or (1, 0, 0) when the velocity is zero.
- **Map-effect draws (04 R-SIM-238).** Draws that decide simulation outcomes use the simulation generator, and snowflake draws use the presentation generator (DEC-118). 07 owns the per-site list. The vortex and lightning land searches are bounded under DEC-014.
- **Kill milestone.** The event is emitted for every team and shown only for the local team (DEC-007).
- **Ownership change.** An actor's team change destroys it and respawns it in the new team (DEC-028); a structure changes team in place and keeps its serial.

## Dropped as legacy (one line per group)

- **Campaign-only types and their state**: blueprint, receiver, mine and dynamic-hub structures, the factory, the hatchery, the clone lab, the control tower and the other campaign structures, unprocessed orbs, Builder research ids, refinery population and the receiver id cache. Campaign or unreachable content (ledger drop).
- **Debug-only and unreachable kinds**: the Rift, the Rift tentacle, the Vehicle Duel vehicle, the Summoner, the Tri-walker, the drop ship, the lander, the laser trooper, chess pieces, the bridge, the missile, the land mine, the controller grenade, the box kite, the single-player order effect and the turret-target effect. Debug or unreachable (ledger drop, DEC-062).
- **The soul-rain map-effect spawner and its timer.** Its effect kind is dropped, and no reference map sets the effect.
- **Mission-objective evaluation in the world step, the campaign essence transfer and the campaign event evaluation at teardown.** Campaign.
- **The per-tick sync value, the sync-log fingerprint of every object and the world-state dump.** Superseded networking; DEC-206 decides the Species check.
- **Match-entry map permission check, the single-player server start and the editor-mode structure look-up.** Demo, campaign and editor builds only.
- **Container-library internals**: the sorted tree, the hash tables, the directory, the owning vector, bounded and presized arrays, the slice-pool constructor quirk, release-assert accessors, cursor-moving list reads and zero reads at invalid indices. Superseded containers; the ordering semantics they impose are kept (04 §2.3).
- **Superseded render and device detail**: the pixel-effect hook, per-actor draw calls in the crowd passes, graphics-state rebuild on burn patches, the alpha-sort buffer growth step, matrix-format scratch, and device and network statics. Superseded technology.
- **Load-thread mechanics**: which thread runs initialisation and the work-queue hand-over. Deferred (DEC-224, DEC-226). Replay recording and loading are kept as hooks.
- **Lobby messaging around match start**: option and team-request messages, keep-alive cadence, and the leave-message burst with its 100 ms wait. Deferred networking (DEC-243, DEC-221 item 7).
- **The 64 wu obstacle-grid cell outside matches and the single-player difficulty paths.** Single-player only.
- **Original numeric values**: the pseudo-unit codes, the effect-type enumeration and the fixed match id. Original numeric ids, replaced by Species enumerations.

## Publishing

Nothing was published, and no git write command was run.
