# M3 plan

> Status: Draft (O80); the owner answered its questions and its headless phase A starts before M2's exit (O98); it is made Final on M2's exit evidence · Related: [MVP scope](mvp_scope.md) §3 (M3) and §6.1 (Playtest 1) · [M2 plan](m2_plan.md) · [M1 plan](m1_plan.md) · [MVP assets](mvp_assets.md) · [owner decisions](../decisions/owner_decisions.md) (O53, O55, O70, O74, O80, O84, O92, O98) · [ADR index and technology baseline](../ADR/README.md)

## 1 Purpose

This plan sets out milestone M3 of the [MVP scope](mvp_scope.md), the neighbours and the run. It was drafted while M2 was built, so that M2 could be built with M3 in view, and it now carries the owner's answers to its questions (O98) and what M2 built. It covers:
- what M3 owes beyond the scope's own row, most of it the confirmations of the first playable (§2);
- what M2 has settled and what still waits on its evidence (§3);
- the order of the work (§4);
- the tasks of M3 (§5);
- Playtest 1 and its stop rule (§6);
- the owner's answers (§7);
- the risks this plan sees (§8).

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
| [ADR-014](../ADR/ADR-014-hdr-output.md), [ADR-015](../ADR/ADR-015-scene-anti-aliasing.md) | the SDR path covered by every test, with HDR off, until the options pages are final; the scene's GPU time at 4x multisampling in a battle with `mvp.client.figures_max` figures on screen at 1080p, measured on the reference test machine for the MVP (O98) | T16 |
| [ADR-025](../ADR/ADR-025-audio-formats-codecs-and-residency.md) | the first sound set of MVP-087 played from its runtime formats | T15 |
| [ADR-028](../ADR/ADR-028-input-device-back-ends.md) | keyboard and mouse (MVP-100); the gamepad tests wait for a build with gamepad input | T16 |
| [ADR-034](../ADR/ADR-034-text-rendering-and-glyph-cache.md) | the front end and the HUD drawn by DirectWrite in Bahnschrift (MVP-102, MVP-103; O92, [ADR-147](../ADR/ADR-147-the-directwrite-text-system-at-m2.md)) | T16 |
| [ADR-062](../ADR/ADR-062-authoring-syntax-and-schema-language.md) | the MVP's records loaded by all three hosts from code that the generator wrote (MVP-091, MVP-099; ADR-098) | T16 |
| [ADR-064](../ADR/ADR-064-accessibility-baseline-xag.md) | the conformance, contrast, text-size and flash tests | T16 |
| [ADR-077](../ADR/ADR-077-client-http-stack.md) | no telemetry in the MVP, so its condition is unchanged and recorded as such | T16 |
| [ADR-079](../ADR/ADR-079-simulation-tick-rate.md) | an order's latency on the M3 build at most 275 ms at the 95th percentile, and crowd orders rated responsive in the Playtest 1 log | T16, T17 |
| [ADR-082](../ADR/ADR-082-counter-based-simulation-random-draws.md) | a test client given everything the steward's client receives cannot compute a later draw | T3 |
| [ADR-084](../ADR/ADR-084-client-boundary-through-the-transport.md), [ADR-086](../ADR/ADR-086-replication-and-interest-management.md) | the replica, or a test client, holds no figure, structure or store outside its settlement's seen layer (MVP-097) | T3 |
| [ADR-087](../ADR/ADR-087-command-path-validation-and-timing.md) | a forged command (another settlement's order, a missing field, a pool slot as an id) refused with its reason and counted; an order for an emptied group doing nothing; an action contested between the steward and an AI won by each within 45 % to 55 % over 1,000 ticks | T9 |

*Table 1 — What the ADRs put at M3, and the task that produces each.*

M1's and M2's records leave these to M3:

| Left by | What M3 builds | Task |
|---|---|---|
| [ADR-107](../ADR/ADR-107-simple-ai-bot-navigation-and-node-census.md) (O70) | the AI reads the seen layer in place of the whole battle (MVP-058) | T3 |
| [ADR-111](../ADR/ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-139](../ADR/ADR-139-replicating-the-settlement-at-m2.md) | the replication narrowed to the seen layer (MVP-097), so a client no longer receives the whole region | T3 |
| [ADR-130](../ADR/ADR-130-settlements-structures-and-resource-sites-at-m2.md) | three more founded settlements, each with its temperament (MVP-057); an attacking expedition is a team that is no settlement | T2, T6 |
| [ADR-138](../ADR/ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md) | the scripted steward grown into the AI settlements' economy (O98) | T2 |
| [ADR-125](../ADR/ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md) | the world markers' other types: attacks, claims, and an expedition's Leaders over the fog (O98) | T12 |
| [ADR-142](../ADR/ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md), [ADR-144](../ADR/ADR-144-black-fog-outside-the-stewards-areas-at-m2.md) | the battlefield of every battle the steward's settlement is a side of joins the steward's areas for its duration (WLD-244), widening the camera's bound and the fog's mask | T12 |
| [ADR-143](../ADR/ADR-143-territory-lines-and-border-areas-drawn-at-m2.md) | the border areas' holders and claims marked (MVP-053) | T11 |
| [ADR-141](../ADR/ADR-141-the-turrets-shots-drawn-at-m2.md) | the turret's shots seen in battle, and `TurretTargetSighted` heard | T13, T15 |
| [ADR-155](../ADR/ADR-155-the-front-end-and-saved-worlds-at-m2.md) | the front end's run and epilogue entries, and the Leaving state back to the front end (ADR-083 item 3) | T14 |
| [ADR-156](../ADR/ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md) | a battle at a settlement drawn among the rendered scenes, and the territory's halfway line recorded so that a saved world's contact hour can be judged | T9 |

*Table 2 — What M1's and M2's records leave to M3.*

## 3 What M2 has settled, and what still waits

The parts of M3 that cost most rest on what M2 measures. M2's code is written, and its exit waits on the owner's runs (`playtests/M2.md`):
- **When neighbours meet.** On T10's tuning a lone settlement's territory reaches a halfway line at 2.65 to 2.67 hours of play on seeds 1 to 8, inside the scope's hour 2 to 3, and two neighbours growing towards each other meet no later. The owner's growth run confirms the pace.
- **The AI settlements' economy** is M2's scripted steward (O98). Under it a settlement grows with a step at least every 15 minutes and reaches its plateau of 300 Workers at 4.27 to 4.32 hours on seeds 1 to 8 ([ADR-138](../ADR/ADR-138-the-scripted-steward-and-the-growth-scene-at-m2.md)). The temperaments are new parameters of it (T2).
- **The settlement at its plateau** is `mvp.settlement.population_max` (300, O84). An expedition of `mvp.expedition.size_max` (200) is two thirds of a plateau settlement, and the loss cap's 30 % of a defender at its plateau is 90 Workers (MVP-046).
- **The cost of four settlements and two battles.** The region of four scripted settlements is a headless scene of the performance tool; its figures on the reference test machine come with the owner's M2 performance run. M3 adds the two battles of MVP-015 and the AI settlements' decisions within ADR-080 item 2's budget, which O74 set at 60 µs.
- **The HUD's form.** The relations panel, the warning and the results are built on M2's UI core ([ADR-149](../ADR/ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md)), HUD ([ADR-152](../ADR/ADR-152-the-settlement-hud-at-m2.md)) and verbs ([ADR-153](../ADR/ADR-153-the-settlement-verbs-at-m2.md)), whose layout and palette are provisional until the owner's T16 run (DEC-1005).
- **The rendered frame.** The settlement drawn at its plateau is judged at M2's exit ([ADR-156](../ADR/ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md)); M3's battles are drawn on top of it.

## 4 The order of M3

**The valley is built and run headless before it is played.** Playtest 1 costs the owner two runs of 8 hours of play over two to three weeks (the scope's §6.1), so its measurable lines are first predicted without the owner: phase A builds the neighbours, the conflicts and the run as a headless simulation and ends with a valley run in which every settlement is driven by a policy, stepped faster than real time on a corpus of seeds (T9). Phase B makes it playable and audible, and the first-playable confirmations and Playtest 1 close it.

- **Phase A, T2 to T9**, headless and tested in CI, starts before M2's exit (O98), on a pull request of its own, as M2's phase A did before M1's exit (O86). The tuning of T9 to M2's evidence waits for the owner's M2 runs.
- **Phase B, T10 to T17**, starts at M2's exit, each task with its run checklist (M0 plan §2).

> **Amendment 2026-10-23 (O101).** Phase B starts before M2's exit, on phase A's pull request, each task still with its run checklist; the owner runs them when M2's exit allows. The tuning to M2's evidence still waits for the owner's M2 runs, and T17 still needs Playtest 1.

**Saved worlds across M3's state changes.** The owner's M2 growth run saves worlds of `WORLD_STATE_VERSION` 10 across sessions. ADR-137 lets a raise refuse its predecessor until M2's exit build, but phase A raises the version while that run may still be going on. So from M3's first raise every raise migrates version 10 and later (MVP-095): the owner's M2 world continues on any later build.

## 5 Tasks

| Task | Builds | Done when | Ids |
|---|---|---|---|
| **T1 Plan** | this plan with the owner's answers (O98); made Final on M2's exit evidence | the design checks pass | O80, O98, MVP-002 |
| **T2 Four settlements** | the three AI settlements founded at the other sites when a world starts, each with its Cladan's colour (the site's index, ADR-130) and one of the three temperaments, no two alike (MVP-013, MVP-057); each run by the scripted steward of ADR-138 with its temperament's parameters (O98), through the command path (MVP-056) | the four settlements grow side by side headless with the same hashes at 1, 2 and 8 worker threads; a world of version 10 continues with its settlement alone | MVP-013, MVP-056, MVP-057, MVP-094, MVP-095 |
| **T3 The seen layer and its readers** | the seen layer that M2 computes once per update (MVP-016, MVP-097, O81), now for four settlements; the AI's navigation graph and priority field fed only from it (MVP-058, R-BOT-025 to R-BOT-071); the replication narrowed to it (ADR-111, ADR-139) | table 1's ADR-082 and ADR-084 rows pass | MVP-016, MVP-058, MVP-097, ADR-082, ADR-084, ADR-086 |
| **T4 Relations** | grievances and anger (MVP-050) from the three causes (MVP-051), their fade and the battle relief (MVP-052) | unit tests of each cause, fade and relief | MVP-027, MVP-050 to MVP-052 |
| **T5 Claims and tribute** | claims on a held half of a border area and on the central resource site, held uncontested or settled by a claim battle, and withdrawn (MVP-053); tribute and the easing of a grievance (MVP-054) | unit tests of every claim's path to its end | MVP-053, MVP-054, WLD-086 |
| **T6 Expeditions and battles** | the expedition of up to three Leaders (MVP-041 as amended by O98): Workers promoted to Leaders and Workers drawn from guard duty to them, ordered and declared as one; declaring an attack, its cost and the warning hold (MVP-042); the battle's start, battlefield, defender's advantage and end (MVP-043 to MVP-045); the loss cap and the respite (MVP-046, MVP-047); the battle result (MVP-048); the cap of two battles in progress (MVP-015) | headless battles at a settlement and at a claimed area end by each of MVP-045's four ends, with the loss cap's bound on every transfer; an expedition of three Leaders moves and fights as one | MVP-015, MVP-041 to MVP-049, PIL-045, PIL-087 |
| **T7 The AI settlements' decisions** | the attack rule (MVP-055), the temperaments' claims, preferred attack and response to tribute (MVP-057), expeditions of several Leaders formed as the steward's are, and the reference bot's battle core of M1 commanding their sides (MVP-049, MVP-058), every `mvp.ai.decision_ticks` within ADR-080 item 2's budget | the AI settlements of three temperaments behave apart on a corpus of seeds; the AI's cost is within its budget | MVP-049, MVP-055, MVP-057, MVP-058, ADR-080 |
| **T8 The run and renown** | the run's length, the time left and the quiet minutes (MVP-060); renown and its sources (MVP-061); the standing (MVP-062); the end of the run, the read-only world and the epilogue's data (MVP-063); the rest of the server's statistics (MVP-064), with the tick at which each territory first reaches a halfway line | a run ends on time with its standing and history, and a world that ended is read-only after a restart | MVP-060 to MVP-064 |
| **T9 Command-path tests, the valley run and the tuning** | table 1's ADR-087 row; the valley run of §4, with the steward's settlement under the scripted steward and a stated conflict policy, reporting Playtest 1's measurable lines (figures in battles, battle lengths, conflicts settled, the strongest settlement against the median, battles per run); the performance scene of four settlements with two battles, headless and drawn; the tuning of the relation, battle and run values of the scope's table 3 by stated levers, after M2's evidence (O98) | the valley run meets Playtest 1's measurable pass lines on the corpus of seeds, or the values it needs go to the owner; the owner's performance run records every figure within its budget | MVP-003, MVP-064, ADR-080, ADR-087 |
| **T10 Replication of relations, battles and the run** | relations, claims, warnings, expeditions, battles, results, renown and the run in the replication and the guaranteed events, on M2's replication ([ADR-139](../ADR/ADR-139-replicating-the-settlement-at-m2.md)) | ADR-084's test holds with four settlements and a battle | MVP-090, MVP-097, MVP-098, ADR-111 |
| **T11 Relations panel and notifications** | MVP-102's M3 part on M2's HUD: the relations panel with each Cladan's anger, every grievance with its cause, weight and fade, and every claim; the border areas' holders and claims marked (ADR-143); the notifications of contact, claims, the warning with its countdown and battles starting and ending; the respite; the M3 icons | the owner runs it: writes an attack's cause before opening the panel, and checks it | MVP-050, MVP-102, PIL-024 |
| **T12 Conflict verbs and markers** | forming an expedition of up to three Leaders, declaring an attack with its cost and whether it is unprovoked shown before it is confirmed, withdrawing it, making and withdrawing a claim, and sending tribute (MVP-101), on M2's verbs; the edge compass for attacks and claims and their world markers; the expedition's Leaders marked over the fog and at the screen's edge while it is outside a battle (O98); the battlefield joining the steward's areas, the camera's bound and the fog's mask for a battle's duration (WLD-244) | the owner runs it: raids an AI settlement, follows the expedition to it, claims an area and eases a grievance | MVP-041, MVP-042, MVP-053, MVP-054, MVP-101, WLD-244 |
| **T13 Battle results** | the battle result of every battle the steward's settlement is a side of, and the public line of every other (MVP-048), on the result card (`icon.hud.result_card`) | the owner runs it: a result agrees with the server's statistics | MVP-048 |
| **T14 Renown, the run and the epilogue** | the renown standing by source, the time left in the run, the epilogue page, the front end's run and epilogue entries on M2's front end (O80, [ADR-155](../ADR/ADR-155-the-front-end-and-saved-worlds-at-m2.md)), and the Leaving state back to the front end (ADR-083 item 3) | the owner runs it: ends a run, reads its epilogue, returns to the front end and reopens the epilogue from it | MVP-060 to MVP-063, MVP-103 |
| **T15 Audio and the first sound set** | the audio engine at the MVP's size (ADR-022 to ADR-027) and the sound set of MVP-087, every sound synthesized, the Worker death group included (O98); the master volume that M2 stores, heard | the owner runs it: each sound plays on its event, at its bus's volume | MVP-087, ADR-022 to ADR-027 |
| **T16 The first-playable confirmations** | table 1's rows not produced by an earlier task, on the M3 build | each ADR is confirmed or amended with its evidence | the ADRs of table 1 |
| **T17 Exit** | Playtest 1 (§6); the likeness review of the HUD's additions (PIL-027); the M3 record | the exit criteria of the scope's §3 hold, with Playtest 1's record in `playtests/` | MVP-002, PIL-027 |

*Table 3 — The tasks of M3.*

## 6 Playtest 1 and the stop rule

Playtest 1 is the scope's §6.1: two full runs at the session pace over two to three weeks, with the session log's three lines after every session and the server's statistics for the measured lines. The valley run of T9 has predicted the measured lines before it starts, so the owner's runs test what a script cannot: whether an hour of play is fun, whether a run has an arc after the plateau, and whether the owner wants another run.

The outcome is recorded in the M3 record. A no to the first question stops the MVP at M3 for a redesign; the evidence then resumes the paused design areas (O51), starting with B1, B2, B5 and B6 (the scope's §6.3).

## 7 The owner's answers

The owner took this plan's recommendation on each question (O98, 2026-10-22); the decision records the options that were offered.

1. **How an expedition of several Leaders forms.** An expedition is up to three Leaders, promoted by the steward and ordered and declared against a target as one. Each keeps the reference formation of `ref.sim.team.formation_size_upgrade` (90), so three of them reach `mvp.expedition.size_max` (200), and the AI settlements field theirs the same way (MVP-041 as amended; the [M1 plan](m1_plan.md) §7).
2. **The AI settlements' economy** is M2's scripted steward, with the temperaments as its parameters (T2).
3. **The camera in a battle away from home.** WLD-244 already adds a battle's battlefield to the steward's areas for its duration. Outside a battle the expedition is marked over the fog, its Leaders shown as markers there and at the screen's edge, with no figure drawn and the settlement view not widened (T12; the MVP scope's §5.2 narrows WLD-244, whose strategic view the MVP does not build).
4. **The sound set's sources.** Every sound is synthesized, the Worker death group included; a recorded group may replace it later with no change of code.
5. **ADR-015's GPU.** Its confirmation is measured on the reference test machine for the MVP, and again on the minimum-specification tier before the first external build ([ADR-015](../ADR/ADR-015-scene-anti-aliasing.md) as amended).
6. **The scope's open items** (§7 of the scope): neither a difficulty setting beyond the temperaments (DEC-112) nor the strategic view's second layer for battles between AI settlements is built before Playtest 1, which decides them.
7. **When M3 starts.** Its headless phase A starts before M2's exit, on a pull request of its own; phase B starts at M2's exit (§4).

## 8 Risks

- **M2's evidence may move M3's values.** Phase A starts before the owner's M2 runs, so it builds the rules and their tests at the scope's values, and T9 tunes them once M2's evidence is in. A miss in the owner's growth run moves `mvp.*` economy values that the valley run then reads.
- **The AI settlements' economy has no reference.** The reference bot fights; it does not build or gather. M2's scripted steward is the mitigation, measured on the growth scene, and T2 is the first task for that reason; the temperaments are new to it.
- **An expedition of several Leaders is new state.** The formation code of M1 is unchanged (R-UNIT-162), but the expedition that groups up to three Leaders, its orders and its declaration are new, for the steward and the AI alike (T6, T7).
- **Saved worlds across M3's raises.** Every raise from M3's first migrates version 10 and later (§4), so phase A carries a migration with each state change.
- **ADR-015 is measured on the reference test machine** (O98), whose GPU is not the minimum-capability GPU the ADR names. The minimum-specification tier is decided before the first external build (O58, O69), and the measurement is repeated then.
- **Fourteen confirmations at one milestone.** Each ADR's evidence is gathered as the task that touches its area lands, not at the end, so that T16 confirms what is in hand rather than starting fourteen pieces of work.
- **Playtest 1's cost.** Two runs of 8 hours are the owner's weeks; a valley run that misses its measurable lines is retuned before the owner starts, not during.
