# A0 resolution 1

> Proposer: A0-P · Round 1 · Date: 2026-10-18 · Answers `challenge_1.md` (23 findings, 6 "make it better" items) · Documents changed: `Design/gdd/00_vision_pillars_and_core_loops.md`, `Design/gdd/adoption_ledger/00_reference_overview_ledger.md`

**Totals.** Findings: 22 Accepted (5 with one part changed or left out, each stated), 0 Rejected, 1 Escalated (A0-F01, Critical). Make it better: 6 Accepted (3 in part). Open Critical or High: A0-F01, until the owner answers Q5 at gate A; the design is written for the recommended option and works for every option (the two paces of PIL-100 hold under all three).

I checked the challenger's model (`species_design_work/scripts/A0C_SessionPacing.py`, run with `python3 -I`): its inputs are the draft's tuning values, and its arithmetic holds. Its numbers are quoted below where a resolution uses them.

## Findings

### A0-F01 World-time targets do not survive stage 1 (Critical) — Escalate

**The finding stands.** At the session patterns of Table C, a solo steward in an integrated world meets one battle every 16.8 to 84 hours of play and reaches the plateau after 336 hours of play, and the away loop never runs (script, section 5). Every B area would tune against an always-on world that stage 1 does not have.

**What changed without changing an owner decision (Accepted parts).**
- Two **world paces** (new §2.11, PIL-100 to PIL-104): the **always-on pace**, for a world whose server keeps running, and the **session pace**, for a world that runs only while its world host plays. At the session pace, the economy, relations and protection timers advance `pil.pace.session_factor` (12, provisional) times faster per hour of world time; battles, travel and Worker movement run at the tick rate unchanged (PIL-101). World time is still server uptime and nothing is caught up, so O4 holds as written. The factor of 12 makes a host who plays 2 hours a day reach the 14-day plateau in 14 real days (the script's 168 days divided by 12).
- Every target of §4 is stated for the always-on pace, with the session-pace conversion rule (PIL-103); each guard names its pace (PIL-022, PIL-023, PIL-028, PIL-029, PIL-065, PIL-123).
- Tables B and C carry a stage and pace column; the edge case of the solo steward is rewritten for both paces.
- The pace is fixed when the world is created and is shown on the join page (PIL-104, PIL-130).

**What the owner decides (Q5).** Whether, and how, a stage-1 world can keep running while its host is not playing. That is how O4 ("the host has closed the client") and O10 (the server integrated in the client) work in stage 1, so it is an escalation, not an edit.
- **Proposer (recommended, option A):** keep O4 and O10 as written. A world host may create an always-on integrated world: when the host leaves the game, the client keeps running with its presentation suspended (no rendering, no audio, an icon in the notification area), the in-process server keeps the world running, and closing the client still freezes it. Session-paced worlds remain the default for a host who does not want to leave the game running.
- **Challenger (option B):** the same two paces, but the always-on integrated world keeps running after the game closes, as the headless server build in a background process on the host's machine. More convenient and more robust (a client crash does not stop the world), but it amends O4's example and adds a local dedicated form to stage 1 beside the in-process one (O10), with the hand-over of a world between two processes that D1 otherwise builds for stage 2.
- **Option C:** stage 1 runs at the session pace only; pillar 2's away loop arrives with community dedicated servers in stage 2. Cheapest, but stage 1 ships without the vision's absent play, and pillar 2 is first tested in stage 2.
- **Why A over B:** A gives every player the same experience as B (close the window, the world keeps running) at the cost of a suspended presentation mode in one process, and needs no change to an owner decision; B's robustness matters most for always-on worlds, which stage 2's dedicated servers serve anyway. If the owner prefers B, the GDD changes only PIL-077 and PIL-102's wording.

### A0-F02 The integrated host owns world time and the save (High) — Accepted

Changed: new §2.12, "The world host of an integrated world":
- PIL-130: the join page and the world show who the world host is, the world's pace, and the hours it ran in each of the last `pil.host.running_history_days` (7) days, so a steward in another time zone sees when the world is alive before joining.
- PIL-131: every freeze is recorded with its cause and its world time, and appears in the away report of every steward of the world.
- PIL-132: closing the client while a battle runs in the world, or while an expedition is inside its warning time of the host's own settlement, asks the host for confirmation, and the freeze is recorded as a freeze during a battle (or during a warning) in every steward's away report.
- PIL-133: D4 and D9 own the save's integrity from stage 1: a world save carries a record that makes a restored or edited save visible to the other stewards when they next join.
- Part (d), attacks between stewards off by default in integrated worlds (A0-M1), is part of owner question Q3 (PIL-063), with the challenger's view as the recommended option.
- Ben's case (only online when the world is frozen) is answered by the pace (PIL-100) and the join page (PIL-130): a session-paced world suits stewards who play at the same hours, and the join page says so.

### A0-F03 The away-yield guard makes active play nearly worthless (High) — Accepted

The script shows it: at a 50 to 80 % away yield, 120 minutes a day out-grows 10 minutes a day by only 7.6 to 1.9 %, and a never-present settlement grows at 92 to 98 % of a heavy player's. Changed:
- PIL-023 is restated against real patterns and stated orders: under the default orders and a construction queue of `pil.guard.queue_hours` (8 hours), a steward who plays the typical pattern grows at least `pil.guard.presence_gain_share` (25 %) more per day of world time than one who checks in once a day, and the check-in steward grows at least `pil.guard.check_in_floor_share` (70 %) as much as the typical one (always-on pace, B2's model).
- The typical pattern is defined once (PIL-102): `pil.session.typical_active_minutes` (60) minutes of active play and `pil.session.typical_check_ins` (2) check-ins per day.
- PIL-023 says where presence pays: the decisions only a present steward takes (placing and queueing construction, expansion, claims, founding, declaring attacks), battles fought in person, and the aide. The deputy's own rate per hour is an input of B2's model, not a guard.
- The wrong sentence about "above the band" is gone; `pil.guard.away_yield_share` is removed.

### A0-F04 Attacking the away is always better, and PIL-086 forbids the counter (High) — Accepted

The challenger is right that PIL-086 forbade the natural counter, while PIL-023 already charges an absence for its safety. Changed: PIL-086 now prices the trade instead of forbidding it. An absence may carry stronger protection than presence only so far as the growth the absence costs outweighs the protection it gains, and in B6's protection model an attacker's expected net gain from an attack on an away settlement is at most `pil.guard.away_target_premium` (1.2) times its gain from the same attack with the steward present. B6 chooses the levers (an away-only defence bonus, a smaller loss cap while away, the defence windows of A0-M3); the edge case "a steward stays away to keep the protection of an absence" is rewritten as a priced choice.

### A0-F05 Collusion: Essence feeding and respite gifting (High) — Accepted, with one rule changed

Changed:
- PIL-045 no longer decides who collects Essence (that is B1's and B4's); it states the constraint: the net transfer of a battle to either side (stores, Workers and Essence together) is bounded by the loss cap of the side that loses it, attacker or defender, and Essence beyond the bound is lost to both.
- PIL-087: a respite starts only after a battle that cost the defender at least the minimum loss that B6 sets, never after a battle that ended within `pil.battle.min_respite_minutes` (2 minutes) of its start, and never after an attack withdrawn before its battle (Figure 5).
- PIL-028's model includes battle transfer (A0-F12), so the winner's snowball is measured.
- §5 holds the three scenarios (feeding, respite gifting, the winner's snowball) with B1, B4 and B6 as owners; D5 is asked for the account rules that make a second account visible (PIL-088 needs the same identity).
- **Changed from the recommendation:** the challenger's third rule ("Essence from a battle between two steward-run settlements that hold no grievance against each other is not collectable") is not used, because every declared attack records a grievance at declaration (PIL-063), so the condition would never hold in the feeding case. The transfer bound covers feeding whatever the grievances.

### A0-F06 The world loop has nothing to win and no way to settle a conflict (High) — Accepted

Changed (with A0-M4 and A0-M6):
- **Claims** (PIL-120, PIL-121): territory pressure and competition for resources create a claim on a named contested area, a resource site or a border area of territory outside a settlement's core. A battle won by the claimant moves the claimed area, within the loser's loss cap; a truce or tribute (if B5 defines them) or a withdrawal settles it. A settlement's core and its Wellspring are never claimable (pillar 6 holds).
- PIL-122: a grievance whose cause has been removed fades `pil.conflict.settled_fade_factor` (4) times faster, and B5 gives every conflict an end state (a settled claim, a truce or a faded grievance).
- PIL-123, a guard: in B5's model at the always-on pace, at least `pil.guard.conflict_resolution_share` (60 %) of the conflicts between two neighbours end within `pil.guard.conflict_resolution_days` (7 days) of world time.
- PIL-124: grievances arise only from settlement-level actions (expansion, a claim, an expedition, an attack), never from a Worker's own task choice; that keeps pillar 1's autonomy from starting wars nobody chose.
- PIL-125 (SHOULD, from stage 2): a world standing by renown, earned by defences held, grievances eased, claims settled and structures raised, never by stores taken, and an epilogue when a world's lifetime ends (A1, B5, C2).
- New terms: claim, core, renown.

### A0-F07 Pillar 3 binds only the AI (Medium) — Accepted (as the recommended option of Q3)

Changed: PIL-012 now binds clades and stewards. PIL-063 states the recommended option of Q3: a steward may attack any settlement outside protection, but an attack with no cause on record (no grievance of the attacker's clade against the target and no claim) is an unprovoked attack: it costs `pil.conflict.unprovoked_cost_factor` (2) times the attack cost, records a heavier grievance (B5 sets the weight), and the target's away report and the world's record name it as unprovoked. The owner confirms or changes it with Q3.

### A0-F08 Hostile world events are the random aggression pillar 3 rules out (Medium) — Accepted

Changed: pillar 3 (PIL-012, Table 1) is scoped to clades and stewards; world events are hazards. PIL-049: a world event is announced at least `pil.conflict.event_warning_minutes` (5 minutes) of world time before it reaches a territory, counts against the loss cap, starts no respite, and its creatures belong to the Rogue faction, which in the world game holds no grievance and never wins (B4, B6). Table 4 gains a Rogue faction row; the R-VIS-030 row stays `adopt-skirmish`, with the world's use stated in PIL-049.

### A0-F09 Battles between settlements of one clade are unreadable (Medium) — Accepted

Changed: PIL-014 says what must be true: the two sides of every battle are distinguishable to every viewer at every zoom; in the strategic view a clade's colour is its identity, and in a battle the sides use a friend-and-foe palette that E2 and B5 choose. New provisional key `pil.world.clade_count` (6), which B5 confirms with E2.

### A0-F10 "Watched" is undefined (Medium) — Accepted

Changed: "watched" is a term of Table 3 (a settlement is watched while any client's settlement view covers part of its territory; the strategic view never makes it watched), and PIL-057 adds the guard: over a day of world time, under the same orders, the abstract model's yield is within `pil.guard.fidelity_yield_gap` (3 %) of the full simulation's (B2, D2), so that no steward gains by choosing whether a settlement is watched. PIL-023 names the full simulation as the model it measures, with the abstract model held to PIL-057.

### A0-F11 The guards mix world time and active-play time (Medium) — Accepted

Changed: every guard is written in world time at a stated session pattern. PIL-022: regain within `pil.guard.regain_days` (2 days) of world time at the typical pattern (replaces the 2 hours of active play). PIL-029: a week-4 founder who plays the typical pattern reaches 50 % of the median within `pil.guard.late_joiner_days` (3 days) of world time (replaces 6 hours of active play). PIL-055: the growth-step window is `pil.settlement.early_days` (10 days) of world time at the typical pattern, and the step rate is measured inside sessions of active play. The re-founding case is in §5 and PIL-088: newcomer protection and any catch-up apply once per person per world, with D5 supplying the identity.

### A0-F12 The leader guard cannot fail in the model that runs it (Medium) — Accepted

Changed: strength is defined once (PIL-058: Workers, plus stores at their construction value, plus structures at their construction cost, with B2 owning the weights), and PIL-028 runs in B2's economy model extended with B4's battle transfer, B5's choice of targets and A1's site richness, with the strongest settlement played by an always-attacking strategy and every other settlement by the typical pattern.

### A0-F13 Abandoned settlements accumulate (Medium) — Accepted (as the recommended option of Q4)

Changed: PIL-078 adds dormancy. A settlement whose steward has not been present for `pil.session.dormancy_days` (21 days) of world time becomes dormant: it leaves the map with its stores and structures kept, its site and territory become free, its deputy stops and it costs the server nothing but storage; a returning steward places it again at a free settlement site, with newcomer protection. No battle ever removes a settlement (PIL-080). Q4 now asks about removal by battle and by abandonment together, with the challenger's split as the recommended option.

### A0-F14 The ledger's restatement verdicts contradict PIL-032 (Medium) — Accepted

Changed in the ledger: §2.8 `adapt` (PIL-032, PIL-040) with R-VIS-072 `adopt-skirmish`; §2.13 `adapt` (PIL-010, PIL-032); §2.17 `adapt` (PIL-032, PIL-085) with R-VIS-162 and R-VIS-163 `adopt-skirmish`; R-VIS-090 `adapt` (PIL-010); R-VIS-050 `adapt` (PIL-085, which now states the tick and game speed of both forms). Where no PIL requirement commits the world game, the restatement stays `adopt-skirmish`. The second part (a ledger convention that a restating id takes its owner's verdict, computed by the gate) is a change to `Tools/DesignCheck/`, which is the coordinator's; it goes to the coordinator as a cross-area request.

### A0-F15 adopt-skirmish rows bind six-mode content (Medium) — Accepted

Changed: PIL-003 gains the clause rule (a reference requirement adapted to it keeps binding the skirmish in its clauses for Spawn Control or for every mode; its clauses for other modes wait until that mode is scheduled). R-VIS-005, R-VIS-034, R-VIS-054, R-VIS-066, R-VIS-067, R-VIS-081, R-VIS-130, R-VIS-131, R-VIS-142 and R-VIS-173 are `adapt` with PIL-003 (exception rows, and §2.15 as a whole). B9 confirms each when it writes the skirmish.

### A0-F16 A battle floor of 100 figures is below "hundreds" (Medium) — Accepted

Changed: PIL-021 has two values: a typical battle shows `pil.battle.bodies_typical` (400) figures, and below the at-risk floor `pil.battle.bodies_floor` (now 200) the conflict goes to the owner. A2's and B4's models report both at 16 and at 200 stewards.

### A0-F17 No smallest world game per stage (Medium) — Accepted, with one part changed

Changed: PIL-110 and Table 10, "The smallest world game per stage", with the pillars and guards each stage meets. **Changed from the recommendation:** the abstract model stays in stage 1, because O3 makes it an owner decision rather than an option; deferring it would change when an owner decision applies. A2 and E3 may propose a deferral at gate B with its cost. The first world-game milestone is tested with 1 and 4 stewards, without capping the 2 to 16 of O1.

### A0-F18 The pillar 5 guard checks provenance, not distinctness (Medium) — Accepted

Changed: PIL-027 adds a look-alike review by people allowed to see the reference game (the owner or counsel, outside the clean room) at each art milestone of E2, against a list of features that must differ (Worker and Leader silhouettes, structure silhouettes, the HUD layout, the logo), with the result recorded in E2's asset manifest; it cites O13's legal check.

### A0-F19 R-VIS-007 adopted with a qualifier (Low) — Accepted

Changed: R-VIS-007 is `adapt` with the new PIL-092: the reference library is the specification of record (DEC-005), the reference values are the starting baseline of every `adopt` and `adopt-skirmish` requirement (O13, DEC-004), an adapted requirement starts from them only where its area says so, and quirks stay explicit rules (DEC-006).

### A0-F20 Which attacks warn the target is stated three ways (Low) — Accepted

Changed: PIL-025 states one rule for every attack, by an AI settlement or a steward: the target is warned at least `pil.conflict.warning_minutes` (10 minutes) before the expedition reaches its territory. Figure 5 cites PIL-025, and AC-15 tests both kinds of attacker.

### A0-F21 Vocabulary overloads (Low) — Accepted

Changed in Table 3 and Table 4: "frozen" stays the world's term, and Table 4 says that the skirmish's grace-period hold is "held" in player-facing text; the declared world attack's code name is `AttackDeclaration` (prose keeps "attack"); the aide has its own root `aide`; the check-in's string id is `presence.check_in`; abstract fidelity is `Fidelity::Modeled`.

### A0-F22 "Race" survives in file names; the product name needs a screen (Low) — Accepted

Changed: the trademark paragraph of §2.10 adds the product name to the pre-release registry search, with the note that a creature-evolution simulation released on PC in 2018 carries the same word in its title. Cross-area request to the coordinator: if the owner confirms "clade", rename `gdd/06_races_relations_and_conflict.md` and B5's area title (or record why not).

### A0-F23 PIL-020 and AC-05/AC-06 disagree (Low) — Accepted

Changed: PIL-020 and AC-06 list the same exceptions (promoting a Worker, scrapping a unit, direct control of special units; demoting a Leader addresses a Leader, not a Worker, and is allowed); AC-05 says the steward gives input other than orders (camera movement) throughout, so the steward stays present.

## Make it better

| Item | Answer | What changed |
|---|---|---|
| A0-M1 Co-op first in integrated worlds | Accepted in part | Attacks between stewards are a world setting, off by default in integrated worlds and on by default in dedicated worlds, shown on the join page: part of Q3's recommended option (PIL-063). Defending each other by expedition already exists (PIL-062). Trade between stewards is not added: whether stores move between settlements is B2's (prompt §5.2) |
| A0-M2 Watch the battles your deputy fought | Accepted (SHOULD) | PIL-140: the away report SHOULD offer a replay of each battle at up to `pil.report.replay_speed_max` (8) times speed, with the deputy's decisions on the timeline, each tied to its standing order. A2 (reproducibility, DEC-240, DEC-241), D3 and D4 decide feasibility and cost; the challenger's 50 to 200 KB per battle minute is an estimate for them to measure |
| A0-M3 Defence windows | Accepted in part | Named in PIL-086 as one lever B6 may choose; not mandated, because it is one of three ways to meet the new guard and B6's protection model should choose between them |
| A0-M4 Claims | Accepted | PIL-120, PIL-121, PIL-122 (see A0-F06); the term "claim" |
| A0-M5 A check-in fast path | Accepted | PIL-141: a check-in entry of the client that opens the away report and the standing-order editor without loading the settlement view, through the same server API calls (C2, C3, D6) |
| A0-M6 Standing and an epilogue | Accepted in part | PIL-125 (SHOULD, from stage 2) with the term "renown"; achievements and leaderboards stay C2's decision (DEC-047) |

## Owner questions after round 1 (merged list for gate A)

| # | Question | Recommended | Other options | Proposer and challenger |
|---|---|---|---|---|
| Q1 | Add a sixth pillar, Always a way back (PIL-015) | Add it, with claims (PIL-120) so that conflict can still move something. Trade-off: rules out conquest by absorption and permanent knock-outs | B keep five and write the rules into B2, B5 and B6 (leaner, but local trade-offs erode them); C rely on world-lifetime resets (simplest, but erases investment) | Agree |
| Q2 | What is a people of Species called (PIL-091) | Clade: suits the title, carries no real-world weight in "an angered people attacks its neighbours", free as a game term; onboarding teaches the word | B race (familiar, heavier, shares a word with Launch Race); C kin (plain, reads as family) | Agree |
| Q3 | May a steward attack any settlement (PIL-063) | Yes, outside protection; an unprovoked attack costs 2 times as much and is named; between stewards, a world setting off by default in integrated worlds and on in dedicated worlds. Trade-off: conflict between friends stays possible while stage-1 worlds default to co-op | B only with a cause on record (cleanest pillar 3, but steward play depends on AI grievances); C no attacks between stewards in v1 (no griefing; loses player conflict); D any settlement, no surcharge, on everywhere (the first draft) | Agree (the recommendation is the challenger's, A0-F07 and A0-M1) |
| Q4 | Can a settlement be removed from a world (PIL-078, PIL-080) | Never by battle; by abandonment, through dormancy after 21 days (it leaves the map with its stores and structures and returns at a free site). Trade-off: no knock-outs and no ghost settlements; a returning steward may lose the old site | B never at all (no one loses a place; worlds fill with deputy-run ghosts); C by battle when unprotected (classic stakes; the eight-hour sleeper can wake to a refounding) | Agree (the split is the challenger's, A0-F13) |
| Q5 | How does a stage-1 world keep running while its host is not playing (A0-F01; O4, O10) | A: O4 and O10 as written; an always-on integrated world keeps running while the client stays open with its presentation suspended; session-paced worlds otherwise. Trade-off: no owner decision changes; the host must leave the game running | B: the world keeps running after the game closes, as the headless build in a background process (most convenient; amends O4's example, adds a local dedicated form to stage 1); C: session pace only in stage 1, the away loop arrives in stage 2 (cheapest; stage 1 has no absent play) | **Disagree:** the challenger recommends B; the proposer recommends A |

For information, not a question: the product name "Species" is added to the pre-release registry search (A0-F22).
