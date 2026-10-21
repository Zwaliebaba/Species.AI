# M3 plan

> Status: Draft (O79); its phases and tasks are set, and it is finished on M2's exit evidence, when the owner answers the questions of §7 · Related: [MVP scope](mvp_scope.md) §3 (M3) and §6.1 (Playtest 1) · [M2 plan](m2_plan.md) · [M1 plan](m1_plan.md) · [MVP assets](mvp_assets.md) · [owner decisions](../decisions/owner_decisions.md) (O53, O55, O70, O74, O79) · [ADR index and technology baseline](../ADR/README.md)

## 1 Purpose

This draft sets out milestone M3 of the [MVP scope](mvp_scope.md), the neighbours and the run, far enough that M2 can be built with M3 in view, and says what M2 has to settle before it is finished (§3). It covers:
- what M3 owes beyond the scope's own row, most of it the confirmations of the first playable (§2);
- what waits on M2's evidence, and why this plan is a draft (§3);
- the order of the work (§4);
- the tasks of M3 as they stand (§5);
- Playtest 1 and its stop rule (§6);
- the questions for the owner at M2's exit (§7);
- the risks this draft sees (§8).

M3 ends when the exit criteria of the MVP scope's §3 hold:
- Playtest 1 (§6.1 of the scope) is run and recorded;
- the Proposed ADRs whose "Confirmed when" names the first playable are confirmed or amended on this milestone's build (the scope's §5.3).

M3 is also where the MVP's first question is answered. A no to "is an hour of play fun?" stops the MVP at M3 for a redesign, and M4 is built only on a yes (the scope's §1.2).

## 2 What M3 owes

Since O55 the first playable is M3's build, and fourteen Proposed ADRs name it as their confirmation point (the scope's §5.3). Each is confirmed or amended on its own "Confirmed when" evidence, and the other records below put their tests at M3:

| Record | What M3 shows | Task |
|---|---|---|
| [ADR-002](../ADR/ADR-002-minimum-gpu-capability.md), [ADR-013](../ADR/ADR-013-working-colour-space-and-blending.md) | the capability probe's unit test, and the golden-image tests of the reference test scenes at exactly feature level 12_1 | T16 |
| [ADR-004](../ADR/ADR-004-frame-graph-from-the-ordered-pass-table.md), [ADR-007](../ADR/ADR-007-barrier-model.md) | the world frame compiled from the pass table (RND-067), and the reference frame drawn on both barrier paths, with the enhanced executor added before the M3 build ([ADR-113](../ADR/ADR-113-the-frame-at-m1.md)) | T16 |
| [ADR-006](../ADR/ADR-006-root-signature-and-bindless-resource-access.md), [ADR-078](../ADR/ADR-078-gpu-resource-factory.md) | GPU-based validation over a full session, from creating the world through at least one battle to a save (MVP-075); the first textured draw, and the forced device removal of ADR-008 during a session | T16 |
| [ADR-014](../ADR/ADR-014-hdr-output.md), [ADR-015](../ADR/ADR-015-scene-anti-aliasing.md) | the SDR path covered by every test, with HDR off, until the options pages are final; the scene's GPU time at 4x multisampling in a battle with `mvp.client.figures_max` figures on screen at 1080p, which the ADR asks for on the minimum-capability GPU (§8) | T16 |
| [ADR-025](../ADR/ADR-025-audio-formats-codecs-and-residency.md) | the first sound set of MVP-087 played from its runtime formats; without a sound set the confirmation waits | T15 |
| [ADR-028](../ADR/ADR-028-input-device-back-ends.md) | keyboard and mouse (MVP-100); the gamepad tests wait for a build with gamepad input | T16 |
| [ADR-034](../ADR/ADR-034-text-rendering-and-glyph-cache.md) | the front end and the HUD drawn in GameFont (MVP-102, MVP-103) | T16 |
| [ADR-062](../ADR/ADR-062-authoring-syntax-and-schema-language.md) | the MVP's records loaded by all three hosts from code that the generator wrote (MVP-091, MVP-099; ADR-098) | T16 |
| [ADR-064](../ADR/ADR-064-accessibility-baseline-xag.md) | the conformance, contrast, text-size and flash tests | T16 |
| [ADR-077](../ADR/ADR-077-client-http-stack.md) | no telemetry in the MVP, so its condition is unchanged and recorded as such | T16 |
| [ADR-079](../ADR/ADR-079-simulation-tick-rate.md) | an order's latency on the M3 build at most 275 ms at the 95th percentile, and crowd orders rated responsive in the Playtest 1 log | T16, T17 |
| [ADR-082](../ADR/ADR-082-counter-based-simulation-random-draws.md) | a test client given everything the steward's client receives cannot compute a later draw | T3 |
| [ADR-084](../ADR/ADR-084-client-boundary-through-the-transport.md), [ADR-086](../ADR/ADR-086-replication-and-interest-management.md) | the replica, or a test client, holds no figure, structure or store outside its settlement's seen layer (MVP-097) | T3 |
| [ADR-087](../ADR/ADR-087-command-path-validation-and-timing.md) | a forged command (another settlement's order, a missing field, a pool slot as an id) refused with its reason and counted; an order for an emptied group doing nothing; an action contested between the steward and an AI won by each within 45 % to 55 % over 1,000 ticks | T9 |

*Table 1 — What the ADRs put at M3, and the task that produces each.*

Two items of M1 end at M3: the AI's reading of the whole battle, which MVP-058 replaces with the seen layer (O70), and the replication of the whole battle, which MVP-097 narrows (ADR-111). M1 also left the expedition of several Leaders to M3 (the [M1 plan](m1_plan.md) §7; §7 below).

## 3 What waits on M2's evidence

This plan is a draft because the parts of M3 that cost most rest on what M2 measures:
- **When neighbours meet.** First contact is timed by the territory's growth, which the [M2 plan](m2_plan.md) §3 expects T10 to retune. The scope's §6.1 asks for 4 to 10 battles involving the steward in a run, with first contact in hour 2 to 3; with the first guesses, the M2 plan's estimate reaches the 150 Workers of that contact only after five to six and a half hours of play, which would leave too little run for that many battles.
- **The AI settlements' economy.** MVP-056 has them run the settlement rules through the same command path. The reference bot has no economy of this kind, so M2's scripted steward (the M2 plan §5) is its first form, and how well it plays a settlement is M2's evidence.
- **The settlement at its plateau.** Its size sets an expedition's share of a settlement (MVP-041, MVP-055), the loss cap's bite (MVP-046) and the figures in a battle that Playtest 1 counts.
- **The cost of four settlements and two battles.** M2 measures a region of four scripted settlements at their plateau (the M2 plan's table 1); M3 adds the two battles of MVP-015 and the AI settlements' decisions within ADR-080 item 2's budget, which O74 set at 60 µs.
- **The HUD's form.** The relations panel, the warning and the results are built on M2's UI framework and HUD.

## 4 The order of M3

**The valley is built and run headless before it is played.** Playtest 1 costs the owner two runs of 8 hours of play over two to three weeks (the scope's §6.1), so its measurable lines are first predicted without the owner: phase A builds the neighbours, the conflicts and the run as a headless simulation and ends with a valley run in which every settlement is driven by a policy, stepped faster than real time on a corpus of seeds (T9). Phase B makes it playable and audible, and the first-playable confirmations and Playtest 1 close it.

- **Phase A, T2 to T9**, headless and tested in CI.
- **Phase B, T10 to T17**, each task with its run checklist (M0 plan §2).

## 5 Tasks

The tasks as they stand; T1 finishes them on M2's evidence.

| Task | Builds | Done when | Ids |
|---|---|---|---|
| **T1 Plan** | this plan made Final; the answers of §7 | the design checks pass | O79, MVP-002 |
| **T2 Four settlements** | the three AI settlements at the other sites, each with its Cladan's colour and one of the three temperaments, no two alike (MVP-013, MVP-057); each running the settlement rules through the command path (MVP-056), with the M2 scripted steward grown into an AI settlement's economy | the four settlements grow side by side headless with the same hashes at 1, 2 and 8 worker threads | MVP-013, MVP-056, MVP-057, MVP-094 |
| **T3 The seen layer and its readers** | each settlement's seen layer computed once per update (MVP-016, MVP-097), if M2 did not build it; the AI's navigation graph and priority field fed only from it (MVP-058, R-BOT-025 to R-BOT-071); the replication narrowed to it (ADR-111) | table 1's ADR-082 and ADR-084 rows pass | MVP-016, MVP-058, MVP-097, ADR-082, ADR-084, ADR-086 |
| **T4 Relations** | grievances and anger (MVP-050) from the three causes (MVP-051), their fade and the battle relief (MVP-052) | unit tests of each cause, fade and relief | MVP-027, MVP-050 to MVP-052 |
| **T5 Claims and tribute** | claims on a held half of a border area and on the central resource site, held uncontested or settled by a claim battle, and withdrawn (MVP-053); tribute and the easing of a grievance (MVP-054) | unit tests of every claim's path to its end | MVP-053, MVP-054, WLD-086 |
| **T6 Expeditions and battles** | the expedition (MVP-041) and how one of several Leaders forms (§7, question 1); declaring an attack, its cost and the warning hold (MVP-042); the battle's start, battlefield, defender's advantage and end (MVP-043 to MVP-045); the loss cap and the respite (MVP-046, MVP-047); the battle result (MVP-048); the cap of two battles in progress (MVP-015) | headless battles at a settlement and at a claimed area end by each of MVP-045's four ends, with the loss cap's bound on every transfer | MVP-015, MVP-041 to MVP-049, PIL-045, PIL-087 |
| **T7 The AI settlements' decisions** | the attack rule (MVP-055), the temperaments' claims, preferred attack and response to tribute (MVP-057), and the reference bot's battle core of M1 commanding their sides (MVP-049, MVP-058), every `mvp.ai.decision_ticks` within ADR-080 item 2's budget | the AI settlements of three temperaments behave apart on a corpus of seeds; the AI's cost is within its budget | MVP-049, MVP-055, MVP-057, MVP-058, ADR-080 |
| **T8 The run and renown** | the run's length, the time left and the quiet minutes (MVP-060); renown and its sources (MVP-061); the standing (MVP-062); the end of the run, the read-only world and the epilogue's data (MVP-063); the rest of the server's statistics (MVP-064) | a run ends on time with its standing and history, and a world that ended is read-only after a restart | MVP-060 to MVP-064 |
| **T9 Command-path tests, the valley run and the tuning** | table 1's ADR-087 row; the valley run of §4, with the steward's settlement under the scripted steward and a stated conflict policy, reporting Playtest 1's measurable lines (figures in battles, battle lengths, conflicts settled, the strongest settlement against the median, battles per run); the performance scene of four settlements with two battles; the tuning of the relation, battle and run values of the scope's table 3 by stated levers | the valley run meets Playtest 1's measurable pass lines on the corpus of seeds, or the values it needs go to the owner; the owner's performance run records every figure within its budget | MVP-003, MVP-064, ADR-080, ADR-087 |
| **T10 Replication of relations, battles and the run** | relations, claims, warnings, battles, results, renown and the run in the replication and the guaranteed events | ADR-084's test holds with four settlements and a battle | MVP-090, MVP-097, MVP-098, ADR-111 |
| **T11 Relations panel and notifications** | MVP-102's M3 part: the relations panel with each Cladan's anger, every grievance with its cause, weight and fade, and every claim; the notifications of contact, claims, the warning with its countdown and battles starting and ending; the respite; the M3 icons | the owner runs it: writes an attack's cause before opening the panel, and checks it | MVP-050, MVP-102, PIL-024 |
| **T12 Conflict verbs** | forming an expedition, declaring an attack with its cost and whether it is unprovoked shown before it is confirmed, withdrawing it, making and withdrawing a claim, and sending tribute (MVP-101); the edge compass for attacks and claims and their world markers | the owner runs it: raids an AI settlement, claims an area and eases a grievance | MVP-041, MVP-042, MVP-053, MVP-054, MVP-101 |
| **T13 Battle results** | the battle result of every battle the steward's settlement is a side of, and the public line of every other (MVP-048), on the result card (`icon.hud.result_card`) | the owner runs it: a result agrees with the server's statistics | MVP-048 |
| **T14 Renown, the run and the epilogue** | the renown standing by source, the time left in the run, the epilogue page, and the front end's run and epilogue entries (O79) | the owner runs it: ends a run, reads its epilogue, and reopens it from the front end | MVP-060 to MVP-063, MVP-103 |
| **T15 Audio and the first sound set** | the audio engine at the MVP's size (ADR-022 to ADR-027) and the sound set of MVP-087, from the synthesiser and the recorded or synthesised death group; the volume option heard | the owner runs it: each sound plays on its event, at its bus's volume | MVP-087, ADR-022 to ADR-027 |
| **T16 The first-playable confirmations** | table 1's rows not produced by an earlier task, on the M3 build | each ADR is confirmed or amended with its evidence | the ADRs of table 1 |
| **T17 Exit** | Playtest 1 (§6); the likeness review of the HUD's additions (PIL-027); the M3 record | the exit criteria of the scope's §3 hold, with Playtest 1's record in `playtests/` | MVP-002, PIL-027 |

*Table 2 — The tasks of M3, as they stand in this draft.*

## 6 Playtest 1 and the stop rule

Playtest 1 is the scope's §6.1: two full runs at the session pace over two to three weeks, with the session log's three lines after every session and the server's statistics for the measured lines. The valley run of T9 has predicted the measured lines before it starts, so the owner's runs test what a script cannot: whether an hour of play is fun, whether a run has an arc after the plateau, and whether the owner wants another run.

The outcome is recorded in the M3 record. A no to the first question stops the MVP at M3 for a redesign; the evidence then resumes the paused design areas (O51), starting with B1, B2, B5 and B6 (the scope's §6.3).

## 7 Questions for the owner at M2's exit

1. **How an expedition of several Leaders forms.** A formation holds `ref.sim.team.formation_size_upgrade` (90) and an expedition up to `mvp.expedition.size_max` (200), so an expedition needs at least three Leaders (MVP-041; the [M1 plan](m1_plan.md) §7).
2. **The AI settlements' economy.** Whether M2's scripted steward, as M2 leaves it, is the AI settlements' economy with the temperaments added, or needs its own task.
3. **The camera in a battle away from home.** The M2 camera is bounded by the steward's areas; a raid's battlefield is the target's territory and its margin (MVP-043).
4. **The sound set's sources.** The recorded Worker death group, or the synthesised stand-in the MVP assets allow.
5. **ADR-015's GPU.** Whether its confirmation waits for the minimum-specification tier, or is amended to the reference test machine for the MVP (§8).
6. **The scope's open items** (§7 of the scope), which Playtest 1 decides: a difficulty setting beyond the temperaments (DEC-112), and the strategic view's second layer for battles between AI settlements.

## 8 Risks

- **The AI settlements' economy has no reference.** The reference bot fights; it does not build or gather. M2's scripted steward is the mitigation, and T2 is the first task for that reason.
- **ADR-015 names a GPU the MVP does not measure on.** Its 4x multisampling time is to be measured on the minimum-capability GPU, but the MVP's reference test machine is the owner's ARM64 PC, which does not meet ADR-050's x64 minimum, and the minimum-specification tier is decided only before the first external build (O58, O69). At M3 the measurement runs on the reference test machine and the confirmation either waits for that tier or is amended to it; T1 puts the choice to the owner.
- **Fourteen confirmations at one milestone.** Each ADR's evidence is gathered as the task that touches its area lands, not at the end, so that T16 confirms what is in hand rather than starting fourteen pieces of work.
- **Playtest 1's cost.** Two runs of 8 hours are the owner's weeks; a valley run that misses its measurable lines is retuned before the owner starts, not during.
