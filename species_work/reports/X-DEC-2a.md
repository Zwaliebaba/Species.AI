# X-DEC-2a report: networking (DD-NET) deferred decisions

## Files written

| File | What |
|---|---|
| `design_next/decisions/deferred/networking.md` | 9 Open DECs (DEC-200 to DEC-208), about 50.5 KB. It has a header, a scope paragraph and an index table, and the DECs are grouped under four `##` sections. Each entry uses the §6.5 fields plus **Reference behaviour** and **Constraints and lessons**. The field order matches the sibling file `process_tick_and_roadmap.md`. |
| `species_work/dec_map_X-DEC-2a.csv` | 9 rows with the columns `dec_id, title, status, affects_prefix, affects_files, orig_refs`. It is written with the `csv` module and `lineterminator='\n'`. |
| `species_work/scripts/X-DEC-2a_dec_map.py` | Builds the CSV and the range table below from the package JSON. It checks that every package input index is either mapped or listed as unused. |
| `species_work/reports/X-DEC-2a.md` | This report. |

**Unintended publication (needs the coordinator's or owner's attention).** I applied five small wording edits through the Artifact tool's publish-with-edits call instead of the Edit tool. That call also published `networking.md` as a **private** claude.ai artifact: https://claude.ai/artifact/PDkJgi6qCyVwQkiTkbtoEv. Only the owner can open it. The file on disk is correct and is the source of truth. I did not delete the artifact, because the tool allows deletion only when the owner asks. Delete it if it is not wanted.

## Counts

- DECs: 9, all with the status Open and "Decide by: design phase". IDs used: DEC-200 to DEC-208. IDs DEC-209 to DEC-219 are free.
- Constraints and lessons: 88 numbered items (200: 10, 201: 13, 202: 10, 203: 8, 204: 8, 205: 10, 206: 11, 207: 11, 208: 7).
- Requirements, tuning keys and diagrams: none (this is a register file).
- Package inputs: 105 ranges. 102 fed at least one DEC; 3 fed none (see "Judged irrelevant").

## The DECs

| DEC | Title | Recommendation (short) |
|---|---|---|
| DEC-200 | Replication and authority model | A: one server-authoritative replication model for the world and skirmish. B (lockstep for skirmish only) is recorded as an option that departs from O8. |
| DEC-201 | Replication bandwidth and population budget | A, budget first, with C (aggregated crowds) as the main technique to evaluate |
| DEC-202 | Command validation and authority | B: validate on arrival and again on execution, with one path shared by the client, the deputy, settlement AI and the O6 API |
| DEC-203 | Input timing, coalescing and latency | B: a fixed delay and jitter buffer, plus local-feedback prediction only. Adopt the continuous-versus-one-shot coalescing rule under any model. |
| DEC-204 | Content and version identity | A: a content manifest with hashes and server-side version gating, designed so that B (content download) can follow |
| DEC-205 | Join, reconnect, leave and session continuity | C: snapshot join and a session token. When the integrated host leaves, the world freezes (O4); there is no host migration. |
| DEC-206 | Divergence detection and integrity checking | A: a 64-bit canonical state hash every tick in every build, wherever the same inputs are simulated twice |
| DEC-207 | Transport, message format and robustness | Decide together with DEC-200 and the services decision. The listed constraints apply whichever transport is chosen. |
| DEC-208 | Network diagnostics and test tooling | B: one metric set for developers and community-server operators |

The brief's rule "lockstep-only defects become one DEFER-DESIGN entry: the networking model must prevent these failure modes" is met as follows. The scope paragraph states, in bold, that the chosen model must prevent every failure mode listed under Constraints and lessons in DEC-200 to DEC-208. The failure modes themselves are grouped by the question they constrain, as the package note and the coordinator's brief ask (4 to 10 DECs, one per question).

## Figures captured (all now in the DECs)

- **Population (DEC-201):** 6 team slots per match. Caps are 1400–2000 bodies per map: 1×1400, 9×1500, 2×1600, 15×1800 and 20×2000 of the 49 standard arenas, and 2 cooperative arenas have no cap. A team gets 350–1000 bodies. Each Essence orb is collectable for 30–50 s and then ascends for 90 s. Up to about 670 laser bolts are created per second with 2000 Workers. There are about 1000–2000 full searches per second. Peak simulated objects are about 2000–6000.
- **Bandwidth (DEC-201):** commands only cost about 5 KB/s down and about 1 KB/s up per client in a quiet 4-player match. That doubles at 20 Hz and triples at 30 Hz. State replication of a reference-size battle is estimated at hundreds of KB/s raw, or tens of KB/s with quantisation, deltas and interest management. The reference has no bandwidth limiting.
- **Message sizes (DEC-201):**
  - control state: 26/30/34 bytes;
  - control message: 61–69 bytes sent, 53–61 bytes forwarded;
  - 4-player frame: 242 bytes, or about 476 bytes with a piggy-backed copy;
  - frame header: 22 bytes stored, 30 bytes sent;
  - option command: 46 bytes;
  - run-length message: 38 bytes;
  - field overheads: node 7, integer field 8, one-byte field 5, vector field 32 bytes;
  - positions: 3 × 64-bit floats (24 bytes).
- **Datagram limits (DEC-207, DEC-202):** the receive buffer is 1500 bytes. A frame overflows at about 30 option commands or about 25 control-sized commands per 0.1 s tick. The proposed payload budget is about 1200 bytes, with fragmentation above it. Container limits are 1024 fields per node and 10 240-character strings; child counts and blob lengths are unbounded.
- **Latency and buffering (DEC-203):**
  - command delay = U(0–100 ms) + round trip + pacing delay;
  - pacing executes ⌊0.3·P⌋ slices per render frame;
  - mean backlog: 7.0, 12.7, 17.0 and 23.3 slices at 60, 30, 20 and 15 fps;
  - arrival-to-last-slice delay: 100 ms (60–120 fps), 133, 150 and 217 ms;
  - a backlog of 3 slices or fewer is never drained;
  - a host below 10 fps slows the session;
  - catch-up: 3 frames per tick, a gain of 20 frames/s;
  - lockstep comparison delays: 100, 67 and 200 ms plus half the round trip at 20, 30 and 10 Hz;
  - one tick of display interpolation delay.
- **Timeouts (DEC-205, DEC-207):**
  - the server drops a client after 20.0 s of silence;
  - the client shows a lost-connection notice after 5.0 s;
  - the statistics view marks a client lost after 2.0 s;
  - connect retry: 1.0 s, with no attempt limit;
  - team and colour requests repeat every 0.1 s, the team name every 1.0 s;
  - a loading keep-alive goes out every 1.0 s;
  - a graceful leave sends 3 messages, then waits 100 ms;
  - resend window: 3 frames per tick; catch-up starts at more than 10 frames behind and ends below 6;
  - run-length coalescing applies to more than 5 empty frames when the client was heard within 5.0 s.
- **Join (DEC-205):** joining by replay catches up at +20 frames/s, which is about 10 minutes for a 20-minute match. The proposed rejoin target is under 10 s at minute 20.
- **Divergence detection (DEC-206):** the per-frame check is one byte. Two processes whose random-draw counts differ still report the same byte with probability 1/255 per frame, so detection is missed for k frames with probability 255^−k (about 0.4 % and 1.5 × 10^−5). The draw call log holds 100 000 entries.
- **Blob transfer (DEC-207):**
  - 1024-byte blocks;
  - an initial window of 10 blocks, shrinking by 1 to a minimum of 1 and growing by 1;
  - an initial round-trip estimate of 0.1 s, smoothed 0.9/0.1;
  - a block timeout of 3 × the round trip;
  - each acknowledgement sent 3 times, with at most 256 block numbers per acknowledgement.
- **Diagnostics (DEC-208):**
  - rates are 5 s averages;
  - the server counts a 32-byte header per datagram, against an actual 28 bytes;
  - the statistics view has at most 6 client rows and a 10-frame queue strip;
  - the link-simulator profile is 200 ms, 25 % loss and 8000 B/s;
  - the proposed soak target is 10 % loss with reordering;
  - the default replay rate is 25 frames per second.
- **Content (DEC-204):** the largest arena file is 46 085 bytes; the map read buffer is a fixed 100 000 bytes.

## Gate results

- `scan_patterns.py forbidden_patterns.tsv networking.md`: **0 hits**.
- `scan_patterns.py soft_patterns.tsv networking.md`: **17 hits, all `\blockstep\b`**. They are on lines 31–203, all inside DEC-200 to DEC-208. The scope paragraph and the index say "the reference networking model" instead. All 17 are allowed under prompt §7.2, because each one is in a DEC entry that records what the design phase must decide. They are of two kinds: (a) the closing sentence of every Reference behaviour paragraph ("Lockstep is the reference model; Species does not use it for the world (O10)"), which the brief requires; (b) option B of DEC-200 and the comparison figures in DEC-203 and DEC-206. Suggested entry for `CONVENTIONS.md` §9: *lockstep, in `decisions/deferred/networking.md` (DEC-200 to DEC-208): it names the reference networking model that the design phase decides on.* One other soft hit ("Recovery") was fixed.
- No truncation words. No evidence or build tags. No source paths. No original identifiers, message ids, port numbers or wire layouts. No ORIG item ids (X1, L20, P8, F59, R07 and similar); those appear only in this report and in the CSV.
- Links: all 9 index anchors resolve. The only link to a file is the one to `../../../vision/species-vision-brief.md`. Neighbouring register files and the reference documents are named in code spans, not links, so that the link gate does not depend on other agents' file names.

## Cross-document requests

1. **X-DEC-1** (`verdicts_and_fixes.md`) should point these rows at the DECs here instead of creating duplicates:

   | Rows | DEC |
   |---|---|
   | D2, R17, 40 §4 | DEC-200 |
   | T21 | DEC-207 |
   | K25, C10, F59 | DEC-203 |
   | F16 | DEC-202 |
   | F30 | DEC-206 |
   | F31, networking part | DEC-204 and DEC-206 (numeric part: X-DEC-2c) |
   | G28, slot reclaim | DEC-205 |
   | G30 | DEC-205 |
   | G34, keep-alive | DEC-205 |
   | G56 | DEC-205 |
   | G58, G59 | DEC-206 |
   | G60 | DEC-208 |
   | G67 | DEC-204 |
   | 40-U1, network part | DEC-201 |
   | 40-U6, 40-U7 | DEC-205 |

   It should do the same for these 41 rows:

   | Rows | DEC |
   |---|---|
   | top-15 rank 11 | DEC-203 |
   | top-15 rank 13 | DEC-207 |
   | R05 | DEC-206 |
   | R07 | DEC-202 |
   | R08 | DEC-207 |
   | R09 | DEC-205 |
   | R11 | DEC-204 |
   | R29 | DEC-203 |

2. **X-DEC-3** (`open_questions.md`) should reference, not duplicate, these open questions:

   | Open questions | DEC |
   |---|---|
   | 20-U1, 20-U2, 20-U3, 22-U4, 22-U5 | DEC-203 |
   | 22-U3 / 29-U12 | DEC-207 |
   | 22-U6, 22-U18, 22-U19, 22-U21 / 30-U7 | DEC-205 |
   | 22-U8 / 06-U7 / 21-U5 | DEC-200, constraint 4 |
   | 22-U20 / 29-U15 / 30-U3 | DEC-206, constraint 9 |
   | 29-U14 / 23-I2 / 31-I2 | DEC-204 |
   | 32-I1 | DEC-202, constraint 8 |

3. **X-DEC-2b** (`process_tick_and_roadmap.md`): my entries cite its DEC-220 (headless build), DEC-222 (simulation and presentation boundary), DEC-223 (state ownership and reset), DEC-225 (pacing), DEC-227 (tick rate) and DEC-228 (CPU budget and stress populations) by number. I took the numbers from its index as it stood during this run. If they are renumbered, update DEC-200 (constraints 4, 7 and 9), DEC-201 (Specified behaviour, constraint 13) and DEC-203 (Specified behaviour, constraint 4).
4. **X-DEC-2c** (`numerics_lobby_terrain_services.md`): my entries name that file, without a DEC number, as the owner of the numeric model, lobby and match-setup authority, identity, entitlement, encryption keys, discovery and persistence. The coordinator may replace these mentions with DEC numbers at G2.
5. **Gate 7.3 item 6 (every Open DEC is cited by at least one requirement).** These Phase 3 packages should cite the DECs in their "Open decisions" sections:

   | Package | Document | DECs |
   |---|---|---|
   | C-SIM-b | `06_teams_and_player_commands.md` | DEC-200, DEC-202 |
   | C-SIM-c | `01_tick_timing_and_main_loop.md` | DEC-203 |
   | C-SIM-c | `07_randomness_and_integrity.md` | DEC-206 |
   | G05 | population caps | DEC-201 |
   | G01 | README, players leaving | DEC-205 |
   | G07 | bots | DEC-200 |
   | G08a | map identity | DEC-204 |
   | C-PLT | | DEC-207 |
   | C-DAT | message container | DEC-207 |
   | C-DBG | | DEC-206, DEC-208 |
   | C-AST and C-BLD | content manifest, build identity | DEC-204 |

   The same list is in `dec_map_X-DEC-2a.csv` (`affects_files`).

## DEC and ADR requests

- None beyond the 9 entries. DEC-209 to DEC-219 remain free for the coordinator.
- ADR: none, because networking is a superseded-verdict topic. DEC-204 constraint 6 relies on the pack-file **no-loose-override** rule that X-ADR writes. DEC-208 relies on the telemetry-with-consent ADR.

## Names

- All names used are in the ledger: Worker, Leader, Builder, Collector, Transport (the unit, written "Transport units" where the networking word "transport" is nearby), Strike team, Essence orb, Crawler, Power, Power slots, Bot, bot team, bot controller, Rogue faction, Visitor faction, Control Zone, Flag Chain, Zone Control, Relic, Barrier Emitter, Beam Dish, Spawn Node, dust vortex, laser bolt, supply crate, actor grid cache, `TeamCommandState`.
- I avoided the word "relay" (in Species, Relay is the spawn-link world object) and used "sequencer" and "forward" instead.
- No new names were needed.
- "Arena" (a match map) follows the wording of the vision brief.

## Judged irrelevant or owned elsewhere

| Range or item | Reason |
|---|---|
| `40_carry_forward_assessment.md:467` (derived spatial-cache results) | Simulation determinism, owned by reference/12 and the simulation DECs; not a networking question |
| `40_carry_forward_assessment.md:468` (single-threaded simulation, parallel-pragma hazard) | Threading model (X-DEC-2b) |
| `40_carry_forward_assessment.md:469` | Blank line |
| Inside fed ranges: 06 §13 X12–X30 | Camera, input-driver, UI and asset defects with no networking aspect (X-DEC-4b and C-INP own them). X23 and X24 were used in DEC-202. |
| Inside fed ranges: 22 §5.1, the field-code list | Wire codes and the type enumeration are not reproduced (§6.2). Only two lessons were kept: codes reused across namespaces, and missing-field defaults. |
| Inside fed ranges: 22 §3 port numbers, host-discovery address trick; 20 §8.6 discovery and registration threads | Legacy transport and services detail (DL-SERVICES); not reproduced |
| Inside fed ranges: 22 §5.5 and §5.6 enumerator values, log strings, end-of-life message types | Message ids and enumerator values are not reproduced. The reason set was kept in Species terms (DEC-205 constraint 8). |
| Inside fed ranges: 22 §7.2 snapshot layout; 29 §11.4 and 32 §10.4 dump line grammars; 32 §5.1 window geometry and colours | Wire and file layouts are not reproduced. Their coverage gaps and lessons were kept (DEC-206, DEC-208). |
| Inside fed ranges: 40 §4.4 P2 integer timers, P3 numeric model, P4 RNG discipline, P14 bounded algorithms | Named only in DEC-200 constraint 10 as lockstep preconditions; owned by DD-TICK, DD-NUMERIC, reference/12 and the simulation DECs |
| Inside fed ranges: 40 §4.6, the Siege round reset applied outside the tick | A simulation-ordering rule (reference/12 and DD-TICK); not a networking question |
| Inside fed ranges: 40 §4.8 rows on lobby, discovery and identity (lines 489–491) | Not in this package (DD-LOBBY and DD-SERVICES), so not read for content |
| 40-U6 and 22-U8, frequency of the live divergences | Irrelevant to Species. Under server authority the paths are removed by construction. The lesson was kept as DEC-200 constraint 4. |
| 22-U4, intent of the dead pacing branch | Irrelevant to a new design |
| Platform and product names in 40 T21 and §4.8 (transport and relay products, schema libraries) | Not named (brief rule: describe roles and properties). The transport choice is left to DEC-207. |

## Which ranges fed which DEC

Generated by `species_work/scripts/X-DEC-2a_dec_map.py`. Indices are positions in the package's `inputs` list.

| # | Range | Anchor | DECs fed |
|---|---|---|---|
| 0 | `00_overview.md:293-295` | 7-design-critical-quirks-the-new-team-must-decid | DEC-200, DEC-204, DEC-205, DEC-206 |
| 1 | `06_player_control_and_input.md:2287-2323` | 13-known-defects-and-desync-paths-in-the-input-l | DEC-200, DEC-202 |
| 2 | `07_ai.md:1221-1236` | 15-determinism-lockstep-and-the-network-model | DEC-200, DEC-205 |
| 3 | `08_levels_and_level_design.md:97-109` | 23-map-identity-mapcrc | DEC-204 |
| 4 | `20_architecture.md:1040-1055` | 86-network-objects | DEC-207 |
| 5 | `22_determinism_and_networking.md:35-79` | 1-model-overview | DEC-200, DEC-203, DEC-205, DEC-206, DEC-207 |
| 6 | `22_determinism_and_networking.md:124-127` | 24-effective-buffering-and-input-latency-i | DEC-203 |
| 7 | `22_determinism_and_networking.md:128-131` | 25-estimated-latency-statistics-only | DEC-208 |
| 8 | `22_determinism_and_networking.md:142-166` | 3-transport--netlib-over-udp | DEC-207, DEC-202 |
| 9 | `22_determinism_and_networking.md:167-168` | 4-wire-encoding (intro) | DEC-207 |
| 10 | `22_determinism_and_networking.md:169-187` | 41-primitive-encoding--contribsystemivlibnetwork | DEC-201, DEC-207 |
| 11 | `22_determinism_and_networking.md:188-226` | 42-message-container--directory-tosser | DEC-207, DEC-202, DEC-201 |
| 12 | `22_determinism_and_networking.md:227-243` | 43-size-budget-of-common-messages-i-computed-fro | DEC-201, DEC-207, DEC-202 |
| 13 | `22_determinism_and_networking.md:267-270` | 5-message-catalogue (intro) | DEC-207 |
| 14 | `22_determinism_and_networking.md:271-335` | 51-data-field-codes-complete-list-from-codenetwo | DEC-207, DEC-202 |
| 15 | `22_determinism_and_networking.md:417-441` | 55-server--client-letters-servertoclientletter-e | DEC-205, DEC-206, DEC-207 |
| 16 | `22_determinism_and_networking.md:442-460` | 56-disconnect-reasons | DEC-205 |
| 17 | `22_determinism_and_networking.md:461-462` | 6-server-sequencing-and-client-ordering (intro) | DEC-203 |
| 18 | `22_determinism_and_networking.md:503-529` | 62-serverupdateclients--per-client-resendack-win | DEC-207, DEC-203 |
| 19 | `22_determinism_and_networking.md:530-533` | 63-serveradvancesender-c | DEC-207, DEC-208 |
| 20 | `22_determinism_and_networking.md:534-542` | 64-client-connection-states-c-codenetworkclientt | DEC-205 |
| 21 | `22_determinism_and_networking.md:543-558` | 65-clienttoserverreceiveletter-client-listener-t | DEC-207 |
| 22 | `22_determinism_and_networking.md:559-562` | 66-getnextletter-c | DEC-207 |
| 23 | `22_determinism_and_networking.md:563-568` | 67-outgoing-c-clienttoserveradvancesender | DEC-207, DEC-208 |
| 24 | `22_determinism_and_networking.md:569-609` | 7-iframes | DEC-206 |
| 25 | `22_determinism_and_networking.md:721-742` | 86-one-frame-of-input-flow | DEC-203 |
| 26 | `22_determinism_and_networking.md:743-752` | 87-pause-leaving-timeouts-and-lag-c | DEC-205 |
| 27 | `22_determinism_and_networking.md:763-833` | 9-sync-checking-and-desync-handling | DEC-206, DEC-208 |
| 28 | `22_determinism_and_networking.md:845-846` | 11-bandwidth-network-quality-file-transfer-demo- | DEC-201 |
| 29 | `22_determinism_and_networking.md:847-850` | 111-bandwidthcounter-codenetworkbandwidth-c | DEC-201, DEC-208 |
| 30 | `22_determinism_and_networking.md:851-854` | 112-network-statistics-window-bfull-network_stat | DEC-208 |
| 31 | `22_determinism_and_networking.md:855-858` | 113-networkquality-codenetworknetwork_quality--a | DEC-208 |
| 32 | `22_determinism_and_networking.md:859-870` | 114-ftp-manager-codenetworkftp_manager--reliable | DEC-207 |
| 33 | `22_determinism_and_networking.md:1742-1758` | 1413-inputs-that-must-match-but-are-never-verifi | DEC-202, DEC-204, DEC-206, DEC-207 |
| 34 | `22_determinism_and_networking.md:1847-1868` | 16-constants (lines 1847-1868) | DEC-205, DEC-206, DEC-207, DEC-208 |
| 35 | `29_data_formats_reference.md:331-388` | 35-directory-serialisation-network-wire-containe | DEC-207 |
| 36 | `29_data_formats_reference.md:2683-2726` | 114-desync-dump-files | DEC-206 |
| 37 | `29_data_formats_reference.md:2786-2807` | 117-packed-team-controls-alive_data-payload | DEC-201, DEC-203 |
| 38 | `30_scripting_tutorial_and_cutscenes.md:1105-1108` | 72-late-join-replay-from-sequence-0-bfull | DEC-205 |
| 39 | `31_platform_build_and_tools.md:1180-1197` | 103-runtime-use-c | DEC-204 |
| 40 | `32_debug_test_and_performance.md:276-309` | 51-network-stats-window--ships-in-full-c-codeint | DEC-208 |
| 41 | `32_debug_test_and_performance.md:743-764` | 104-sync-logs-c-codenetworkclienttoservercppclie | DEC-206 |
| 42 | `32_debug_test_and_performance.md:891-894` | 13-desync-debugging-tooling | DEC-206 |
| 43 | `32_debug_test_and_performance.md:895-905` | 131-detection-in-each-build-c-codenetworkiframec | DEC-206 |
| 44 | `32_debug_test_and_performance.md:906-913` | 132-forensics-produced-on-desync-c-codenetworkcl | DEC-206 |
| 45 | `32_debug_test_and_performance.md:914-923` | 133-sync-rng-call-tracking-track_sync_rand-bdebu | DEC-206 |
| 46 | `32_debug_test_and_performance.md:924-933` | 134-reproduction-aids-c | DEC-206, DEC-208 |
| 47 | `40_carry_forward_assessment.md:44-44` | 12-the-ten-most-consequential-decisions | DEC-200 |
| 48 | `40_carry_forward_assessment.md:84-84` | 15-consistency-notes-for-this-synthesis | DEC-203 |
| 49 | `40_carry_forward_assessment.md:120-120` | 22-technical-design-part-b-subsystems | DEC-200, DEC-203, DEC-207 |
| 50 | `40_carry_forward_assessment.md:168-168` | 23-finer-grained-verdicts-individual-mechanics-a | DEC-205 |
| 51 | `40_carry_forward_assessment.md:170-170` | 23-finer-grained-verdicts-individual-mechanics-a | DEC-205 |
| 52 | `40_carry_forward_assessment.md:174-174` | 23-finer-grained-verdicts-individual-mechanics-a | DEC-205 |
| 53 | `40_carry_forward_assessment.md:196-196` | 23-finer-grained-verdicts-individual-mechanics-a | DEC-205 |
| 54 | `40_carry_forward_assessment.md:198-199` | 23-finer-grained-verdicts-individual-mechanics-a | DEC-206 |
| 55 | `40_carry_forward_assessment.md:207-207` | 23-finer-grained-verdicts-individual-mechanics-a | DEC-204 |
| 56 | `40_carry_forward_assessment.md:243-243` | 31-mapping-table | DEC-207, DEC-203 |
| 57 | `40_carry_forward_assessment.md:326-327` | 4-determinism-and-multiplayer-architecture-recom | DEC-200 |
| 58 | `40_carry_forward_assessment.md:328-345` | 41-inputs-to-the-decision | DEC-200, DEC-201 |
| 59 | `40_carry_forward_assessment.md:346-346` | 41-inputs-to-the-decision | DEC-200 |
| 60 | `40_carry_forward_assessment.md:347-347` | 41-inputs-to-the-decision | DEC-200 |
| 61 | `40_carry_forward_assessment.md:348-364` | 42-options-compared | DEC-200, DEC-201, DEC-205 |
| 62 | `40_carry_forward_assessment.md:365-370` | 43-recommendation | DEC-200, DEC-201 |
| 63 | `40_carry_forward_assessment.md:371-375` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-200, DEC-203 |
| 64 | `40_carry_forward_assessment.md:376-376` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-200 |
| 65 | `40_carry_forward_assessment.md:377-378` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-200 |
| 66 | `40_carry_forward_assessment.md:379-380` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-200 |
| 67 | `40_carry_forward_assessment.md:381-383` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-202, DEC-206 |
| 68 | `40_carry_forward_assessment.md:384-384` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-204 |
| 69 | `40_carry_forward_assessment.md:385-387` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-204, DEC-207, DEC-205 |
| 70 | `40_carry_forward_assessment.md:388-388` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-200 |
| 71 | `40_carry_forward_assessment.md:389-389` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-206 |
| 72 | `40_carry_forward_assessment.md:390-390` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-200 |
| 73 | `40_carry_forward_assessment.md:391-392` | 44-preconditions-all-mandatory-unless-marked-opt | DEC-200 |
| 74 | `40_carry_forward_assessment.md:393-443` | 45-target-architecture | DEC-200, DEC-203 |
| 75 | `40_carry_forward_assessment.md:444-452` | 46-how-each-original-weakness-is-closed | DEC-206, DEC-202, DEC-204 |
| 76 | `40_carry_forward_assessment.md:453-453` | 46-how-each-original-weakness-is-closed | DEC-200 |
| 77 | `40_carry_forward_assessment.md:454-457` | 46-how-each-original-weakness-is-closed | DEC-200, DEC-204 |
| 78 | `40_carry_forward_assessment.md:458-459` | 46-how-each-original-weakness-is-closed | DEC-203, DEC-202 |
| 79 | `40_carry_forward_assessment.md:460-460` | 46-how-each-original-weakness-is-closed | DEC-200 |
| 80 | `40_carry_forward_assessment.md:461-466` | 46-how-each-original-weakness-is-closed | DEC-205, DEC-207, DEC-203, DEC-206 |
| 81 | `40_carry_forward_assessment.md:467-467` | 46-how-each-original-weakness-is-closed | none: derived spatial-cache results: simulation determinism owned by reference/12, not a networking question |
| 82 | `40_carry_forward_assessment.md:468-468` | 46-how-each-original-weakness-is-closed | none: single-threaded simulation and parallel-pragma hazard: threading model (process_tick_and_roadmap.md) |
| 83 | `40_carry_forward_assessment.md:469-469` | 46-how-each-original-weakness-is-closed | none: blank line closing the table |
| 84 | `40_carry_forward_assessment.md:470-482` | 47-tick-rate-input-delay-and-budgets | DEC-201, DEC-203 |
| 85 | `40_carry_forward_assessment.md:483-488` | 48-session-lobby-and-platform-layer | DEC-200, DEC-207 |
| 86 | `40_carry_forward_assessment.md:492-496` | 48-session-lobby-and-platform-layer | DEC-200, DEC-205 |
| 87 | `40_carry_forward_assessment.md:529-529` | 51-keep-list--mechanics-worth-keeping-as-is | DEC-203 |
| 88 | `40_carry_forward_assessment.md:552-552` | 52-fix-list--shipped-bugs-and-quirks-to-decide-o | DEC-202 |
| 89 | `40_carry_forward_assessment.md:566-567` | 52-fix-list--shipped-bugs-and-quirks-to-decide-o | DEC-206, DEC-204 |
| 90 | `40_carry_forward_assessment.md:595-595` | 52-fix-list--shipped-bugs-and-quirks-to-decide-o | DEC-203 |
| 91 | `40_carry_forward_assessment.md:682-682` | open-questions-and-unknowns | DEC-201 |
| 92 | `40_carry_forward_assessment.md:687-688` | open-questions-and-unknowns | DEC-205 |
| 93 | `41_risks_and_open_questions.md:83-83` | 13-the-top-15-design-critical-uncertainties | DEC-203 |
| 94 | `41_risks_and_open_questions.md:85-85` | 13-the-top-15-design-critical-uncertainties | DEC-207 |
| 95 | `41_risks_and_open_questions.md:141-141` | 2-risk-register-for-the-new-game | DEC-206 |
| 96 | `41_risks_and_open_questions.md:143-145` | 2-risk-register-for-the-new-game | DEC-202, DEC-207, DEC-205 |
| 97 | `41_risks_and_open_questions.md:147-147` | 2-risk-register-for-the-new-game | DEC-204 |
| 98 | `41_risks_and_open_questions.md:165-165` | 2-risk-register-for-the-new-game | DEC-203 |
| 99 | `41_risks_and_open_questions.md:389-395` | 312-20--architecture | DEC-203 |
| 100 | `41_risks_and_open_questions.md:430-433` | 314-22--determinism-and-networking | DEC-207, DEC-203, DEC-205 |
| 101 | `41_risks_and_open_questions.md:435-435` | 314-22--determinism-and-networking | DEC-200 |
| 102 | `41_risks_and_open_questions.md:442-448` | 314-22--determinism-and-networking | DEC-205, DEC-206 |
| 103 | `41_risks_and_open_questions.md:558-558` | 321-29--data-formats-reference | DEC-204 |
| 104 | `41_risks_and_open_questions.md:600-600` | 324-32--debug-test-and-performance | DEC-202 |
