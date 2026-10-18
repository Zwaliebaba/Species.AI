# A0 proposal notes: vision, pillars and core loops

> Proposer: A0-P · Date: 2026-10-18 · Documents: `Design/gdd/00_vision_pillars_and_core_loops.md` (status Draft), `Design/gdd/adoption_ledger/00_reference_overview_ledger.md` · Prompt §3 step 1

These notes are for the coordinator and, at Finalise, for the rationale file `Design/rationale/A0.md`. The challenger does not read them.

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
| R3 | A "keep the world running" mode of the integrated client: the host's client keeps its in-process server running with presentation suspended while the host is away from the game | Pillar 2 is delivered in Stage 1, before community servers exist; solo players get real absences | A low-power client mode for D1 and A2 (the server is headless already; the client must suspend rendering and audio and show the host that the world is running); power use on the host's computer | Recommended to D1 and A2; not written into the GDD |
| R4 | Call the shield period a respite | No collision with the Shield outcome | None | Table 3 |
| R5 | Call the settlement's player the steward and the server's runner the world host | No collision with the product owner, the owning team or the port operator | None | Table 3, PIL-091 |
| R6 | Presence never lowers protection (PIL-086) | Removes the exploit of staying away to keep the protection of an absence | B6 cannot give absent settlements more protection than present ones | PIL-086 |
| R7 | A band for the deputy's yield (PIL-023) | The deputy is neither ruinous nor better than the steward | B2's model must simulate the deputy's yield | PIL-023 |
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

## 4 Owner questions (for gate A)

**Q1 — Add a sixth pillar, Always a way back?** (PIL-015)
- **A (recommended): add it.** No settlement becomes unassailable, late joiners find a place, and a lost battle never ends a settlement. Trade-off: it rules out conquest by absorption and permanent knock-outs, which some players want; every later area carries one more constraint.
- B: keep five pillars and write anti-snowball and late-joiner rules as ordinary requirements of B2, B5 and B6. Trade-off: a leaner pillar set, but local trade-offs can erode the rules one at a time.
- C: keep five and rely on world-lifetime resets (A1) as the main lever. Trade-off: simplest; resets erase investment and weaken the persistent-world promise.

**Q2 — What is a people of Species called?** (PIL-091, Table 3)
- **A (recommended): clade.** A word of biology that suits the title Species, carries no real-world human meaning, and is a free identifier (`Clade`). Trade-off: an unfamiliar word that onboarding must teach in one line.
- B: race, as the vision brief writes it. Trade-off: the familiar genre word; but with peoples that differ in looks and colour and grow angry at each other, it invites a real-world reading in reviews, and it shares a word with the mode Launch Race.
- C: kin. Trade-off: short and warm; awkward in the singular ("a kin") and reads as family.

**Q3 — May a steward attack any settlement?** (PIL-063)
- **A (recommended): yes, any settlement not under protection.** Every attack pays its attack cost and records a grievance of the defender's clade against the attacker. Trade-off: the most emergent conflict between friends; the protection rules (B6) carry the griefing risk.
- B: only a settlement against which the attacker's clade holds a grievance (a cause is required). Trade-off: every war has a cause on record, for stewards too (pillar 3); it restricts friends who simply want to fight, and adds a rule to learn.
- C: no attacks between stewards in v1; stewards fight AI settlements and world events. Trade-off: removes griefing and the hardest balance problem; loses the conflict between players that the vision's seed challenges assume.

**Q4 — Can a settlement ever be removed from a world?** (PIL-080)
- **A (recommended): never by a battle.** The Wellspring cannot be destroyed or captured; a lost battle costs at most the loss cap's floor, then a respite follows. Trade-off: lower ultimate stakes; weak settlements persist, which respite and upkeep keep harmless.
- B: only while its steward is present. Trade-off: full stakes when present; but staying away becomes the safe choice, the exploit that PIL-086 forbids, and play becomes stressful.
- C: whenever it is not under protection; its steward then founds anew at a free site. Trade-off: classic conquest stakes; losers and late joiners churn, and the eight-hour sleeper can still wake to a refounding.

## 5 Adoption (reference/00, ledger owned by A0)

Effective verdicts: adopt 2, adopt-skirmish 93, adapt 17, drop 0, pending 0 (112 requirements, 18 sections, 11 exception rows).

| Scope | Verdict | Species requirements | Reason |
|---|---|---|---|
| §2.1 (R-VIS-001 to R-VIS-009) | adopt-skirmish | PIL-002, PIL-004 | The reference loop is the skirmish loop; the world replaces it (DEC-001 B) |
| R-VIS-003 | adapt | PIL-010 | Crowd autonomy becomes pillar 1 for both forms, extended to gathering and building |
| R-VIS-007 | adopt | — | Reference values and quirks rules apply to every adopted requirement (O13) |
| R-VIS-008 | adapt | PIL-014, PIL-027 | The look becomes pillar 5; 8 team colours bind skirmish only, the world's colour is the clade's |
| R-VIS-009 | adopt | — | The audio binding model serves both forms as written |
| §2.2 (R-VIS-010 to R-VIS-019) | adapt | PIL-003 | One skirmish mode in v1 (O8, O20) |
| R-VIS-011, R-VIS-017 | adopt-skirmish | — | Spawn Control and the match-clock minute as written |
| §2.3 (R-VIS-020 to R-VIS-027) | adapt | PIL-003 | The 49 reference maps do not carry; B9 sets the map set; every map new (DEC-063) |
| R-VIS-022, R-VIS-026, R-VIS-027 | adopt-skirmish | — | Map shape, unlisted development maps, start-position remapping as written |
| §2.4 to §2.17 | adopt-skirmish | — | Restatements of the owning documents; the world uses the owners' verdicts |
| §2.18 (R-VIS-170 to R-VIS-181) | adopt-skirmish | — | The skirmish match flow |
| R-VIS-170 | adapt | PIL-005 | The main page also leads to the world game |
| R-VIS-174 | adapt | PIL-003 | The reference tutorials are on Zone Control maps; B9 (DEC-123) and C5 decide |

No dropped requirement, so no adopted requirement cites a dropped one.

## 6 Decision DEC-001 (the triage's only A0 decision)

**Proposed resolution: Resolved, option B, 2026-10-18.** The skirmish keeps the reference core loop as reference/00 states it, with no build queue and no currency (PIL-002, PIL-003; ledger: §2.1, §2.6 to §2.18 adopt-skirmish). The world game adds settlements that gather resources, store them, build and maintain structures and grow, on top of essence-driven growth in which deaths yield Essence orbs and Essence becomes new Workers (PIL-004, PIL-050 to PIL-056); a resource layer exists. B1 specifies growth through Essence (including a source that needs no death, PIL-056), B2 the resource layer, B3 construction; their ledgers of reference/05 and reference/03 carry the R-ECON and R-BLDG verdicts. Not escalated: it changes no pillar, owner decision or stage plan, and the register's own recommendation is B. The register entry is edited at Finalise, not now.

## 7 Cross-area requests

| To | Request |
|---|---|
| Coordinator | Register the Table 3 terms in CONVENTIONS §8 and the glossary after gate A, with the new string-id roots in table 8-A (`battle`, `clade`, `deputy`, `expedition`, `presence`, `protection`, `region`, `relation`, `report`, `settlement`, `skirmish`, `view`, `world`); update the ledger README row of reference/00 (adopt 2, adopt-skirmish 93, adapt 17, drop 0, pending 0); resolve the gate 15 conflict (§8 below) |
| A1 | Free settlement sites (PIL-068); the default world lifetime (PIL-067); never "season" for a world's span; travel times that make the 10-minute warning of PIL-025 possible |
| A2 | The battle budget against `pil.battle.bodies_floor` at gate B; the tick rate for both forms (the R-VIS §2.6 row follows A2's reference/12 verdicts) |
| B1 | A source of Essence that needs no death (PIL-056); whether the Wellspring is built on the Spawn Node; the name of the settlement's population limit; reference/05 verdicts consistent with the R-VIS §2.7 and §2.9 rows |
| B2 | The models of PIL-022, PIL-023, PIL-028 and PIL-029 and the growth curve of PIL-055; the strength measure |
| B4 | Battle lengths (PIL-047), the battle size (PIL-021), the triggers (PIL-041), withdrawal (PIL-048), how an expedition is simulated while it travels |
| B5 | The causes of grievance and at least one easing that is not an attack (PIL-064); temperament; the relations model for PIL-065; truce and tribute (reserved terms); Q3 |
| B6 | The idle threshold; the loss cap and its floor; the respite, including whether attacking ends the attacker's own respite; newcomer protection; the default orders (PIL-079); PIL-086; the scripted situations of PIL-026; the long absence (PIL-078) |
| B7 | AI that meets PIL-025 and PIL-084 |
| B9 | R-MODE verdicts consistent with R-VIS-011 and R-VIS-017 (adopt-skirmish) and the §2.2 adapt row; DEC-123 |
| C1 | The world verbs (PIL-052, PIL-062); scrapping as a single-unit command (PIL-020); verb parity with skirmish (PIL-032) |
| C2 | The main page (PIL-005, R-VIS-170 adapt); the frozen integrated world (PIL-077); the long absence (PIL-078); a row change here if DEC-047 or DEC-048 changes a hook |
| C3 | The away report readable in 2 minutes (PIL-073); warnings; failure causes; R10, the weekly world digest |
| C4 | A row change here if DEC-049 adds music to skirmish |
| C5 | The tutorials (R-VIS-174 adapt) |
| D1 | R3, the keep-running mode of the integrated client |
| D9 | What a steward sees of a world host's action on the settlement |
| D10 | Consent for the playtest telemetry of the guards |
| E2 | The number of clade colours; provenance records for PIL-027 |

## 8 Gate result and the one conflict

All gates pass with `--design-phase`, with and without `--skip-mermaid`, except **gate 15**, which fails with one message: the GDD names `../rationale/A0.md`, which is not a file yet. The brief asks for the path in backticks because the rationale is written at Finalise, and creating it now is outside the files assigned to A0. The coordinator can create `Design/rationale/README.md` and a stub `A0.md`, or make gate 15 report rather than fail for a document whose status is Draft.

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
