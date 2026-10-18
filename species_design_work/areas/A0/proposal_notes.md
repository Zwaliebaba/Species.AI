# A0 proposal notes: vision, pillars and core loops

> Proposer: A0-P · Date: 2026-10-18 · Documents: `Design/gdd/00_vision_pillars_and_core_loops.md` (status Draft), `Design/gdd/adoption_ledger/00_reference_overview_ledger.md` · Prompt §3 step 1

**Finalise (2026-10-18):** owner gate A decided O31 to O37. O32 replaced the proposed name of the peoples with **Cladan** (plural: the Cladans; identifier `Cladan`, string-id prefix `cladan.`); every use of the earlier proposed word in this file is history, and the design uses Cladan only.


These notes are for the coordinator and, at Finalise, for the rationale file `Design/rationale/A0.md`. The challenger does not read them. **Round 1 revision (2026-10-18):** the answers to `challenge_1.md` are in `resolution_1.md`; this file is updated to match (owner questions §4, adoption §5, cross-area requests §7, gate result §8).

## 1 Options considered

| Choice | Options | Chosen | Why |
|---|---|---|---|
| The pillar set | (a) confirm the vision's five as written; (b) revise the wording of pillars 2 and 5, sharpen 1, 3 and 4, and add a sixth on fairness over time; (c) merge pillars 2 and 4 into one "safe absence" pillar and add a "fair world" pillar | (b) | (a) leaves two literal contradictions (pillar 2 against O4, pillar 5 against O9) and no pillar for runaway leaders, late joiners or ruinous losses. (c) loses the separate test of the deputy's trustworthiness, which is the vision's most distinctive promise |
| Pillar 2 wording | "A world that keeps running"; "A world that runs while you are away"; "Safe to be away" | "A world that runs while you are away" | It keeps the vision's image, is exact under O4 (the world freezes when its server stops) and in Stage 1 (it runs while the host's client runs), and names absence as the normal case. "Safe to be away" drops the persistent world from the name |
| Pillar 5 wording | "The reference look and feel"; "The reference style, a distinct game"; "One look, read from afar" | "The reference style, a distinct game" | It carries both halves of O9 (same style; all art new) and the seed challenge of the look-and-feel line; the third option drops the newness requirement |
| Pillar 6 | none; "No permanent losers"; "Room to rise"; "Always a way back" | "Always a way back" (owner question Q1) | It names the promise to the player who lost, joined late or slept, in plain words; "No permanent losers" is a negative and reads as a guarantee of winning |
| Loop structure | one loop diagram for the whole game; three nested loops with explicit couplings; one loop per actor (steward, deputy, AI) | three nested loops with couplings | The prompt asks for three timescales; later areas need to find their place by timescale, and the couplings (growth presses on neighbours, grievances bring battles, battle results feed both outer loops) are where exploits and balance problems live |
| Unit of the targets | ticks; minutes and hours of world time | world time, with "of active play" where it counts only presence | They are experience targets, not timers; the tick period is A2's (DEC-227), and each owner converts a target into its own tick-based key |
| Name of the protection period (the vision's "shield") | Shield; Ward; Sanctuary; Aegis; Respite | Respite | Shield is already the Supply crate outcome that protects a crowd (CONVENTIONS §8.7); Ward and Aegis are item names in well-known competitive games; Sanctuary is a place name in a well-known action role-playing series. Respite is a plain word that says "a pause in attacks" |
| Name of the settlement's indestructible centre | Hearth; Nexus; Core; reuse the Spawn Node; Wellspring | Wellspring (B1 may build it on the Spawn Node) | Hearth evokes a well-known card game's trademark; Nexus is the base structure of well-known competitive arena games; Core collides with the Core Device. Wellspring fits Essence ("the spring of new Workers") |
| Name of the settlement's player | owner; ruler; founder; keeper; warden; steward | steward | "owner" already means the product owner in every design document and the owning team of a structure in a match; founder is wrong after a hand-over; keeper and warden are well-known game titles or character classes; steward fits pillar 1 (one tends a crowd, one does not command each figure) |
| Name of the peoples (the vision's "race") | race; clade; kin; species; tribe; clan; lineage; folk; strain | clade (owner question Q2) | See Q2. Species collides with the title and the `Species` namespace; Tribes and Lineage are well-known game titles; clan is the common word for a player guild in online games; strain reads as disease |
| Name of a force sent out | warband; host; column; raid; expedition | expedition | Warband is part of a well-known game title; host collides with the server host; column is a table word in every document; raid covers only attacks, while an expedition can also reinforce or gather |
| Name of the person who runs a server | operator; admin; world host | world host | operator is already the Worker in a structure's port (glossary); admin fits only a dedicated server; world host extends the reference's "host" to both stages |
| Name of the AI that helps a present steward | Advisor; assistant; aide | aide | Advisor is listed among the names of Dropped decision entries (CONVENTIONS §8.13); "assistant" is confusable with tooling; aide is short and free |
| Identifier of presence | `Presence`; `StewardPresence` | `StewardPresence` | `Presence` is an engine type of the data schemas (CONVENTIONS §8.12-B) |
| Ledger treatment of the overview's restatements | adopt for both forms; drop as a summary; adopt-skirmish with the world's verdict left to the owning document's ledger | adopt-skirmish | Adopt would bind the world game to rules whose owners (B1, B4, C1) have not yet decided their world verdicts; drop would remove the skirmish summary that tests cite. CONVENTIONS §3 gives a restatement its owner's verdict, so the owners send A0 a row change if they differ |
| Skirmish mode set | six modes; one mode | one (Spawn Control) | O8 ("one mode at first") and O20 decide it |
| Typical battle length | 2 to 5 minutes; 4 to 12 minutes; 15 to 30 minutes | 4 to 12, at most 20 | It fits inside a check-in's upper end and leaves several battles per active session; the reference matches last 5 to 15 match-clock minutes for a whole match, and a world battle is one fight, not a match |
| Away yield | 100 % (the deputy as good as the steward); 50 to 80 %; below 30 % | 50 to 80 % | Above the band an absence is the better strategy; below it the check-in pattern is pointless and pillar 2 fails |

## 2 Recommendations (improvements beyond the vision)

| # | Recommendation | Benefit | Cost | Where |
|---|---|---|---|---|
| R1 | Add pillar 6, Always a way back | The three most common failure modes of persistent strategy worlds (runaway leaders, late joiners, ruinous losses) get a pillar-level owner | One more constraint for every area; it rules out conquest by absorption | PIL-015, Q1 |
| R2 | Give every pillar a measurable guard with a provisional target | Pillars become testable; models and playtests know what to measure | Instrumentation in B2's, B5's and B6's models and in playtests; consent for playtest telemetry (D10) | PIL-020 to PIL-029 |
| R3 | A "keep the world running" mode of the integrated client: the host's client keeps its in-process server running with presentation suspended while the host is away from the game | Pillar 2 is delivered in Stage 1, before community servers exist; solo players get real absences | A low-power client mode for D1 and A2 (the server is headless already; the client must suspend rendering and audio and show the host that the world is running); power use on the host's computer | Round 1: written into PIL-077 as the recommended option of owner question Q5, beside the two world paces (PIL-100 to PIL-104) |
| R4 | Call the shield period a respite | No collision with the Shield outcome | None | Table 3 |
| R5 | Call the settlement's player the steward and the server's runner the world host | No collision with the product owner, the owning team or the port operator | None | Table 3, PIL-091 |
| R6 | Presence never lowers protection (PIL-086) | Removes the exploit of staying away to keep the protection of an absence | B6 cannot give absent settlements more protection than present ones | Withdrawn in round 1 (A0-F04): it forbade the counter to attacks on the away; PIL-086 now prices the trade |
| R7 | A band for the deputy's yield (PIL-023) | The deputy is neither ruinous nor better than the steward | B2's model must simulate the deputy's yield | Replaced in round 1 (A0-F03) by a guard on real session patterns: presence gain 25 %, check-in floor 70 % |
| R8 | AI attacks only while a grievance is on record, with a warning of 10 minutes | Pillar 3 is enforceable; players can name a cause for every AI attack | AI is less opportunistic and may look passive; temperament (B5) must keep it lively | PIL-025 |
| R9 | A source of Essence that needs no death in battle | Growth never forces war; away yield exists | B1 adds and tunes the source against the Essence that battles give | PIL-056 |
| R10 | A weekly world digest for the days-and-weeks loop: the week's battles, grievances and shifts in strength, shown at check-in | The slowest loop becomes readable | A page for C3 and an event log on the server, which the away report needs anyway | Recommended to C3 and B5; not written into the GDD |
| R11 | A trademark registry search of the Table 3 terms in the target markets before release | Removes residual risk of the knowledge-based screen | A search service fee and a few hours | Table 3 note |
| R12 | Skirmish as the battle trainer: every skirmish verb works in a world battle | One set of battle skills; skirmish feeds the world game | C1 keeps the verbs at parity | PIL-032 |

## 3 Assumptions

1. The first audience is groups of 2 to 16 who know each other or share a community, on an integrated or a community server (O1, O5); this sets the session patterns and the protection design cases.
2. The reference unit name Worker serves as the world's inhabitant: one unit gathers, carries, builds and fights. B1 and B4 decide its world behaviour.
3. A steward chooses the clade at founding (B5 and C2 decide).
4. World time is server uptime (O4), so every experience target is in world time; on a dedicated server running all day, world time equals real time.
5. Stewards can attack each other (the vision's seed challenges 1 and 2 imply it); Q3 asks the owner.
6. One AI architecture serves AI settlements, the deputy and the aide (prompt B7).
7. A battle stays within one region or its neighbours (A2 decides the partitioning).
8. In the integrated stage, the other stewards of a world can play only while the world host's client runs it.
9. Strength is a composite measure that B2 defines for the models; the guards compare it.
10. Playtest surveys and telemetry for the guards are possible with consent (D10).
11. Species will be sold commercially, so the trademark screen matters.
12. Clades are collective holders of grievance, as the vision's "when a race is angered it attacks its neighbours" says; B5 may refine the granularity.

## 4 Owner questions (for gate A), merged after round 1

Five questions, each the owner's: Q1 changes the pillar set, Q2 the vocabulary of the vision, Q3 and Q4 the stakes that O2's protection rules frame, and Q5 how O4 and O10 work in stage 1. Recommended option first; where the proposer and the challenger disagree, both views are given.

**Q1 — Add a sixth pillar, Always a way back?** (PIL-015, PIL-120) Proposer and challenger agree.
- **A (recommended): add it, with claims**, so that a conflict can still move something (an area outside a settlement's core). Trade-off: it rules out conquest by absorption and permanent knock-outs, which some players want.
- B: keep five pillars and write the rules into B2, B5 and B6. Trade-off: a leaner pillar set; local trade-offs can erode the rules one at a time.
- C: keep five and rely on world-lifetime resets. Trade-off: simplest; resets erase investment.

**Q2 — What is a people of Species called?** (PIL-091) Agree.
- **A (recommended): clade.** Suits the title, carries no real-world weight in "an angered people attacks its neighbours", and is free as a game term. Trade-off: onboarding must teach the word; B5's file name follows if confirmed.
- B: race. Trade-off: familiar; heavier, and shares a word with Launch Race.
- C: kin. Trade-off: plain; reads as family.

**Q3 — May a steward attack any settlement?** (PIL-063) Agree; the recommendation is the challenger's (A0-F07, A0-M1).
- **A (recommended): yes, outside protection; an unprovoked attack (no grievance or claim on record) costs twice the attack cost and is named; attacks between stewards are a world setting, off by default in integrated worlds and on by default on dedicated servers.** Trade-off: conflict between friends stays possible while stage-1 worlds default to co-op against the AI clades.
- B: only with a cause on record. Trade-off: the cleanest pillar 3; steward play then depends on AI grievances.
- C: no attacks between stewards in v1. Trade-off: no griefing; loses conflict between players.
- D: any settlement, no surcharge, on everywhere (the first draft). Trade-off: simplest; a steward's attack can be random, and the integrated host's advantages are unbounded.

**Q4 — Can a settlement be removed from a world?** (PIL-078, PIL-080) Agree; the split is the challenger's (A0-F13).
- **A (recommended): never by a battle; by abandonment, through dormancy after 21 days without presence**: the settlement leaves the map with its stores and structures and returns at a free site. Trade-off: no knock-outs and no ghost settlements; a returning steward may lose the old site.
- B: never at all. Trade-off: no one loses a place; worlds fill with deputy-run settlements that block sites (24 beside 16 active after 3 months at the challenger's assumed churn).
- C: by battle when unprotected. Trade-off: classic stakes; the eight-hour sleeper can wake to a refounding.

**Q5 — How does a stage-1 world keep running while its host is not playing?** (A0-F01; PIL-077, PIL-100; O4, O10) **Disagree.**
- **A (proposer's recommendation): O4 and O10 as written.** An always-on integrated world keeps running while the host's client stays open with its presentation suspended; closing the client freezes it; otherwise the world runs at the session pace. Trade-off: no owner decision changes; the host must leave the game running.
- B (challenger's recommendation): the world keeps running after the game closes, as the headless server build in a background process on the host's machine. Trade-off: the most convenient and robust always-on stage-1 world; it amends O4's example and adds a local dedicated form to stage 1 beside the in-process one (O10).
- C: the session pace only in stage 1; the away loop arrives with dedicated servers in stage 2. Trade-off: cheapest; stage 1 ships without absent play.
- Both paces (PIL-100 to PIL-104) hold under every option; the GDD is written for A and changes only PIL-077 and PIL-100's wording under B.

## 5 Adoption (reference/00, ledger owned by A0), after round 1

Effective verdicts: adopt 1, adopt-skirmish 60, adapt 51, drop 0, pending 0 (112 requirements, 18 section rows, 24 exception rows).

| Scope | Verdict | Species requirements | Reason |
|---|---|---|---|
| §2.1 | adopt-skirmish | PIL-002, PIL-004 | The reference loop is the skirmish loop; the world replaces it (DEC-001 B) |
| R-VIS-003, R-VIS-090 | adapt | PIL-010 | Crowd autonomy becomes pillar 1 for both forms |
| R-VIS-005, R-VIS-034, R-VIS-054, R-VIS-066, R-VIS-067, R-VIS-081, R-VIS-142, R-VIS-173, §2.15 | adapt | PIL-003 (R-VIS-034 also PIL-014; R-VIS-142 also PIL-084) | Six-mode content: Spawn Control clauses bind, other modes wait for scheduling (A0-F15) |
| R-VIS-007 | adapt | PIL-092 | The baseline rule narrowed to adopted requirements (A0-F19) |
| R-VIS-008 | adapt | PIL-014, PIL-027 | Pillar 5; friend-and-foe palette in battles |
| R-VIS-009 | adopt | — | The audio binding model, as written |
| §2.2, §2.3 | adapt | PIL-003 | One skirmish mode; B9's map set |
| R-VIS-011, R-VIS-017, R-VIS-022, R-VIS-026, R-VIS-027 | adopt-skirmish | — | As written |
| R-VIS-050 | adapt | PIL-085 | One tick for both forms; no game-speed change in the world |
| §2.8 (except R-VIS-072), §2.13 | adapt | PIL-010, PIL-032, PIL-040 | World battles use the same crowd rules and verbs (A0-F14) |
| §2.17 (except R-VIS-162, R-VIS-163) | adapt | PIL-032, PIL-085 | The same verbs in world battles (A0-F14) |
| §2.4 to §2.7, §2.9 to §2.12, §2.14, §2.16, §2.18 (other ids) | adopt-skirmish | — | Restatements binding the skirmish summary; the world takes the owners' verdicts |
| R-VIS-170 | adapt | PIL-005 | The main page also leads to the world game |
| R-VIS-174 | adapt | PIL-003 | Tutorials: B9 (DEC-123) and C5 |

No dropped requirement, so no adopted requirement cites a dropped one.

## 6 Decision DEC-001 (the triage's only A0 decision)

**Proposed resolution: Resolved, option B, 2026-10-18.** The skirmish keeps the reference core loop as reference/00 states it, with no build queue and no currency (PIL-002, PIL-003; ledger: §2.1, §2.6 to §2.18 adopt-skirmish). The world game adds settlements that gather resources, store them, build and maintain structures and grow, on top of essence-driven growth in which deaths yield Essence orbs and Essence becomes new Workers (PIL-004, PIL-050 to PIL-056); a resource layer exists. B1 specifies growth through Essence (including a source that needs no death, PIL-056), B2 the resource layer, B3 construction; their ledgers of reference/05 and reference/03 carry the R-ECON and R-BLDG verdicts. Not escalated: it changes no pillar, owner decision or stage plan, and the register's own recommendation is B. The register entry is edited at Finalise, not now.

## 7 Cross-area requests (after round 1)

| To | Request |
|---|---|
| Coordinator | Register the Table 3 terms in CONVENTIONS §8 and the glossary after gate A, with the new string-id roots (`aide`, `battle`, `clade`, `deputy`, `expedition`, `presence`, `protection`, `region`, `relation`, `report`, `settlement`, `skirmish`, `view`, `world`); update the ledger README row of reference/00 (adopt 1, adopt-skirmish 60, adapt 51, drop 0, pending 0); consider a ledger convention, computed by the gate, that a restating id takes its owner's verdict (A0-F14); if the owner confirms "clade", rename `gdd/06_races_relations_and_conflict.md` and B5's title (A0-F22); note that gdd/00 is 116.6 KB of the 120 KB limit, so a later growth splits it into a folder |
| A1 | Free sites (PIL-068); the core's size and claims on areas outside it (PIL-120, PIL-121); travel times that meet PIL-025 and PIL-049 at both paces; the default lifetime (PIL-067); never "season" |
| A2 | The battle budgets against 400 and 200 figures (PIL-021); one tick for both forms (PIL-085); reproducibility for replays (PIL-140) |
| B1 | A peaceful Essence source (PIL-056); the Wellspring and the core; who collects Essence within the transfer bound (PIL-045) |
| B2 | The models of PIL-022, PIL-023, PIL-028 (joint with B4, B5, A1), PIL-029 and PIL-057, at both paces (PIL-103); strength's weights (PIL-058); upkeep on claimed areas (PIL-121) |
| B4 | Battle lengths and sizes (PIL-021, PIL-047); the transfer bound (PIL-045); world events and the Rogue faction (PIL-049); no game-speed outcome in a world (PIL-085) |
| B5 | Claims and end states (PIL-120 to PIL-124); the unprovoked surcharge and heavier grievance (PIL-063); renown (PIL-125); the clade count with E2 (PIL-014); the relations models of PIL-065 and PIL-123 |
| B6 | The levers and model of PIL-086; the minimum loss of PIL-087; dormancy (PIL-078); the idle threshold; the default orders; newcomer protection once per person (PIL-088) |
| B7 | AI that meets PIL-025 (grievance or claim) and PIL-084 |
| B9 | Confirm the R-VIS rows adapted to PIL-003 and the clause rule; DEC-123 |
| C1 | World verbs including claims (PIL-062); single-unit commands (PIL-020); verb parity (PIL-032) |
| C2 | The join page (PIL-130); the check-in entry (PIL-141); closing an integrated world (PIL-132); dormancy and return (PIL-078); renown and the epilogue (PIL-125); a row change if DEC-047 or DEC-048 changes a hook |
| C3 | The away report with freezes (PIL-131, PIL-132), warnings, claims and replays (PIL-140); the weekly digest (R10) |
| C4, C5 | A row change if DEC-049 adds music; the tutorials (R-VIS-174) |
| D1 | Keeping an integrated world running (PIL-077, Q5) |
| D4, D9 | The save record (PIL-133); dormant settlements in storage (PIL-078); replay storage (PIL-140); the trust model of community servers |
| D5 | Identity that makes a person and a second account recognisable (PIL-088) |
| D10 | Consent for the playtest telemetry of the guards |
| E2 | The likeness review at each art milestone (PIL-027); clade colours and the friend-and-foe palette (PIL-014) |

## 8 Gate result

After round 1: every gate passes with `--design-phase`, with and without `--skip-mermaid` (gate 4: 274 blocks, 0 failed). Gate 15 now reports, rather than fails, the rationale file that is written at Finalise. Gates 12 and 13 report the other areas' pending rows and design-phase decisions, as expected; the A0 ledger has no pending row.

## 9 Comparables behind the lessons of Table D (titles for the rationale file)

| Genre lesson | Titles |
|---|---|
| Protection after attacks, beginner protection, attack costs | Travian, OGame, Clash of Clans |
| Raids on absent players drive players away; predictable protection | Rust, ARK: Survival Evolved (offline raid protection) |
| Priorities beat per-inhabitant orders; show why an inhabitant idles | RimWorld, Dwarf Fortress, Against the Storm |
| Opinion modifiers with cause, size and decay; justified wars | Crusader Kings, Europa Universalis, Stellaris |
| Opaque or over-capable delegates | Stellaris sector governors, X4: Foundations station managers |
| Resets cure runaway leaders but erase investment | Travian server rounds, seasonal leagues of online action role-playing games |
| Crowds read at a distance; bounded battle lengths | Total War, Ultimate Epic Battle Simulator |
