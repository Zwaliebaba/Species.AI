# P6-ENG2 report

## Summary

- **Findings:** 57 in the packet. 53 fixed, 3 fixed differently (R2b-226, R3b-10, R1b-5), 0 rejected, 1 request (R3b-9: my half is done, the checker table is in BLD).
- **Requests from other packets applied:** P6-RD requests 1, 2 and 3; P6-DAT request 4; P6-RE request 6; P6-ADR requests 3 and 4. They are listed in the table after the findings.
- **Files edited**, all in `design_next/architecture/components/`:
  - `platform_layer.md`;
  - `diagnostics_test_and_performance/` (`README.md`, `developer_tools.md`, `logging_checks_and_profiling.md`, `performance_baseline.md` and `test_harness_soak_and_telemetry.md`).

  I edited the DBG §2.1 sketch only around P6-DAT's finished soak declarations. I added `SoakHost` and declared `SoakReport` (R3b-27), and the telemetry record now cites the generated one (R4a-11). P6-DAT's `SoakEntry` and the record citations are unchanged. There was no conflict.
- **New requirement ids:**
  - PLT-132: the one Windows header, `WindowsHeaders.h`.
  - PLT-133: the platform's log sink, which breaks the cycle between the platform layer and diagnostics.
- **Removed requirements.** Only the generated index still cites them, so each stays as a "(removed: …)" stub: PLT-006, PLT-113 and DBG-005.
- **Removed tuning keys:** the 13 `plt.display.reference.*` keys.
- **Tests.** Removed: PLT T16. New: PLT T35 (Windows header), T36 (log sink) and T37 (mapped file).
- **Sketches.** Both §2.1 sketches are clang-format 18.1.3 clean. I checked them with the read-only `species_work/scripts/P6-DAT_format.py`, using my scratchpad as its scratch directory. No line is over 140 columns.
- **Gate.** I ran `python3 -I Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid` after my last edit. Gates 1, 2 and 6 to 11 pass. Gates 3 and 5 fail only in files other packets are editing now, and none of the failures is in or caused by my files:
  - gate 3: 18 missing anchors in `decisions/README.md` and `decisions/open_questions.md` (P6-DEC's DEC retitling);
  - gate 5: `ADR/ADR-078-gpu-resource-factory.md` cites RND-009 and RND-035, which `renderer/` does not define yet (P6-RND).

  Before those packets started, the gate passed with my changes in place, apart from P6-DEC's first anchor failures. I changed no Mermaid diagram, only two captions (PLT diagram 6-A, DBG diagram 4.9-A), so I did not run the Mermaid gate.
- **Further consistency edits.** These were needed by the rulings or by finished packets but are not packet rows:
  - PLT-050: the reference clock narrative becomes the Species rule, citing DEC-041 (ruling 3; DBG-241 has the same subject).
  - PLT-056: now agrees with the new R-SIM-575 (P6-RE).
  - DBG-201: the tick row is written through `ref.sim.tick_period` (ruling 8).
  - DBG-203 H14: the audio update now runs on the audio thread. This follows P6-RE retiring `ref.sim.frame.low_fps_audio_threshold`.
  - DBG-090: "decode task" becomes "stream task", AEN-322's name.
  - DBG-040: the logger's lifetime, for R3b-15.
  - PLT-114 and T15: a screen-options apply now runs the fallback chain in place (P6-RD request 3).

## Findings

| Id | Status | Note |
|---|---|---|
| R4a-13 | fixed | P6-DAT made the sketch half (`SoakEntry::automatedParticipants`, schema key `automated_participants`). DBG-161 now states the mapping: for a test map, its `automated_participants`; for an edition entry, the map's player count, so every slot is played by a bot team (DEC-079). The `Species` server supplies the bot teams through `SoakHost`. |
| R4a-11 | fixed | DBG §2.1: `struct TelemetrySessionRecord; …` becomes the citation `// TelemetrySessionRecord: generated from telemetry.session: data_schemas/test_and_telemetry_records.md §2`. DBG-026 names the generated record and DAT-379/DAT-380. DAT already uses the one name (P6-DAT). |
| R2b-217 | fixed | DBG-202 "minimum hardware" row deleted (ruling 4). The machine is DBG-209. |
| R2b-199 | fixed | PLT-113 becomes a stub "(removed: the client has one creation chain, PLT-112)". Table 6.1-A, the 13 `plt.display.reference.*` keys, their introducing sentence in §4.16 and T16 are deleted. PLT-112 is kept. |
| R2b-222 | fixed | DBG-005's table is deleted; DBG-005 is a stub pointing to DBG-004 and DBG-006. |
| R3b-27 | fixed | New `Engine::SoakHost`, implemented by the game's headless server: `StartSession`, `RequestEntry`, `MapConfirmed`, `Spectate`, `ConnectedParticipants`, `StartMatch` (returns the seed), `AdvanceTick`, `MatchRunning`, `CurrentTick`, `StateHash`, `LivePopulation` and `EndEntry`, one per step of DBG-163. `SoakReport` is declared (the generated `SoakMatchRow` rows and the `SoakPassSummary`). The constructor is `SoakRunner(const SoakConfigurationRecord&, SoakHost&)` and the run is `Run(const std::filesystem::path& _reportFolder)` (DAT-375). DBG-025, DBG-031 (host called from the runner's thread only) and DBG-163 (which call serves which step) follow. |
| R2b-237 | fixed | DBG-130: lines are a line list, one pixel wide; a line wider than 1 px is drawn as a screen-facing quad of that width. The cube draws its twelve edges at `dbg.draw.cube_line_width`, and the arrow width is in pixels. |
| R2b-227 | fixed | DBG-045: "covers every Species engine subsystem, frame-graph pass and loading stage (ADR-068)"; the column-heading mapping is gone. DBG-046: the 186-call-site sentence is deleted. |
| R2b-226 | fixed-differently | The audio row is `audio.update`, with one marker per update step of AEN-123, `audio.stream` (the stream task) and `audio.bank_load` (AEN-322); no retired-effect or instance markers remain. I wrote `audio.stream` instead of the proposed "decode" because AEN-322 names a stream task, not a decode step (DBG-090 follows). There is no voice-start marker because AEN-322 brackets none. |
| R2b-218 | fixed | DBG-203's "Evidence" column becomes "Measured by", naming the §3.4 markers, or the soak report and overlay outputs, for each of H1 to H16. Every reference-notes citation is gone. |
| R2b-221 | fixed | DBG-205: "The Species starting guidance (DEC-228) is a hard budget … on the reference test machine (DBG-209)". The key row reads "Species; design-phase input". |
| R3b-10 | fixed-differently | Ruling 10 instead of a provisional named class. DBG-209 states the interim rule: the first machine that meets the ADR-050 and ADR-002 minima and runs the performance job is pinned and recorded; until then, every performance test reports with the build identity and the agent's hardware and does not fail. DBG-209 also defines "on the minimum CPU", "on minimum-specification hardware" and "on the minimum-specification machine" as this machine, so AST-125, RND-454, UI-240, INP-200, CAM-120, AEN-303, DAT-050, BLD table 5-A and TER T26 all resolve to it without edits. DBG-205, DBG-207, DBG-208, §6, T29 and README §8.1 and §8.3 follow. DEC-971's update is Request 2a. |
| R3b-28 | fixed | The baseline soak is single-process: the headless server runs every entry with `clients` = 0 through `SoakHost` (DBG-160, DBG-162 with default 0, DBG-006 item 3). DBG-163 step 8 waits only for its delay in a single-process pass. DBG-164 and DBG-165 item 2 apply only once the networking model exists (DEC-200; new README §8.1 row). DBG-166 and T26/T27 follow. The DAT `clients` row is Request 3. |
| R3b-15 | fixed | The reviewer's second option. The platform layer declares `PlatformLogSink` (`Write`, `RecordFatal`), `PlatformLogLevel` and `SetPlatformLogSink`; the diagnostics component implements the sink with its logger and black box. PLT-133 states the direction and the start-up buffering (records held until step 7, Debug debugger output, `FailFast` when no sink is set). PLT-038 step 7, PLT-039, PLT-043, PLT-118, PLT-013, table 1-A row 6 and T36 follow; so do DBG-001 item 1, DBG-073 (last paragraph) and DBG-040. `FatalExit`'s use of `MessageArgument` is a downward edge and stays. |
| R3b-9 | request | My half: PLT-003, T6 (renamed "source rules"), PLT-132/T35 and DBG-140/T20 name `Build\CheckProjectFiles.py` and BLD table 4.1-B (P6-ADR's new BLD-123, "exactly the rules of table 4.1-B") instead of a "lint rule". I used the existing checker, not a new `Build\CheckSources.py`, so `AGENTS.md` §3 and the guards of ADR-070 need no change. The table rows, BLD-076, DEC-222 item 3 and the other components' tests are Requests 1, 2b, 5, 6 and 7. |
| R2b-200 | fixed | PLT-006 and table 1-C are removed; PLT-006 is a stub. |
| R2b-201 | fixed | PLT-021 and table 3.2-A now list only the Species runtime files of ADR-071 (13 rows). "The client writes no other file" replaces the "not specified" rows. |
| R2b-202 | fixed | With P6-RD request 1: PLT-029 is replaced, table 3.4-A is deleted and the keys are gone. |
| R3b-14 | fixed | PLT-038: the client's composition root, the client project's entry code (DEC-221), runs the sequence. The platform layer provides steps 1 to 6 and 8 to 10 as §2.1 functions and objects (new `InstallFatalHandlers` and `CheckOperatingSystem`) and never creates or calls a higher component. The order is a requirement on the composition root (T32). PLT-043 and table 1-A row 19 follow. Ruling 2 is applied: at step 11 the asset cache mounts and starts its CPU loads without GPU resources; at step 13 the renderer creates its device, the `GpuResourceFactory` and the swap chain, then `AssetCache::AttachGpu`. ADR-078 had landed by then, so it is cited in step 13, in the status line and in a new §8.1 row (Proposed). PLT-043 now detaches the asset cache from the factory before the renderer is destroyed, as ADR-078 "Lifetime" states. |
| R2b-203 | fixed | PLT-070: monitors are matched to DXGI outputs, and the modes come from `IDXGIOutput1::GetDisplayModeList1` in `DXGI_FORMAT_R8G8B8A8_UNORM` (ADR-012), progressive modes only, at least 640 × 480, merged by size, in reported order. No colour-depth filter and no refresh list remain. `ModeSize` is replaced by `SizePx` and `DesktopMode` loses `bitsPerPixel`. PLT-027, PLT-071 (current mode: width, height, refresh), PLT-073 and T13 follow. |
| R2b-223 | fixed | DBG-022: "No debug binding exists other than those of DBG-020". The three non-campaign negatives are kept as clarifications; the campaign items and the reference sentence are gone. |
| R2b-224 | fixed | DBG-042 is restated as a Species rule; the campaign switch and the terrain storage-mode mirror are gone. |
| R2b-225 | fixed | The DBG-200 clause is deleted. |
| R2b-238 | fixed | The DBG-095 clause is deleted. |
| R2b-239 | fixed | The DBG-099 sentence is deleted. |
| R2b-240 | fixed | DBG-110: the reference sentence becomes the Species rule: each rate is updated at the end of every window, and the round trip is measured once the transport provides it (DEC-208 item 2). |
| R2b-241 | fixed | DBG-120: "The window offers only the commands of DBG-117"; the sound-statistics content stays on the audio page (AEN-323). |
| R2b-242 | fixed | DBG-131: the reference clause becomes "so a prefix search matches only the items in the queue". |
| R2b-243 | fixed | DBG-221: the campaign-cheat and narrator rows are deleted. |
| R2b-230 | fixed | The DBG-044 sentence is deleted. |
| R2b-228 | fixed | The DBG-046 clause is deleted. |
| R2b-229 | fixed | DBG-048 keeps only the Species rule: identity, time and message in the black box, the stack from the dump, the symbol store, no frame-pointer walk. |
| R2b-231 | fixed | The DBG-066 sentence is deleted. |
| R2b-232 | fixed | DBG-070: the column is deleted. The abort row says what an abort is ("a release check that always fails"). |
| R2b-233 | fixed | The DBG-073 paragraph is deleted, and with it the DEC-341 rows of the DBG README and its §8 line. A one-sentence Species statement takes its place: the PLT-118 handlers reach steps 1 and 2 through `PlatformLogSink::RecordFatal` (R3b-15). |
| R2b-234 | fixed | DBG-081: both reference clauses become the Species rule (the trees exist whether or not the view is open; every node is timed). |
| R2b-235 | fixed | The DBG-082 clause is deleted. |
| R2b-236 | fixed | The DBG-087 sentence is deleted. |
| R2b-219 | fixed | DBG-202: "the population cap of each map (map rules and conventions)". |
| R4b-22 | fixed | "(301 to 500 ticks, 30.1 to 50 s)" and "(901 ticks, 90.1 s)", matching reference/05 table rows. |
| R2b-220 | fixed | H16: "no bandwidth limiting or adaptation (DEC-207)". |
| R2b-244 | fixed | The DBG-141 fuzz row takes P6-ADR request 4's text: harnesses are test methods of `<Project>Tests`; a coverage-guided project needs an owner decision and an `AGENTS.md` change (O27, ADR-069). |
| R2b-245 | fixed | The DBG-160 sentence is deleted; the runner drives the session through `SoakHost` with no UI. |
| R2b-246 | fixed | The DBG-162 sentence is deleted (DEC-804 states the rule). |
| R2b-247 | fixed | The DBG-166 sentence is deleted, and the single-process cells (divergence count 0, empty first-divergence cell) are stated. |
| R2b-248 | fixed | DBG-241: the reference sentences become the Species rule. Every platform-clock reader follows the fixed step, the real-time schedules of R-SIM-740 included (PLT-050, DEC-041); video uses external tools (ADR-076); no other recording exists (DEC-060). |
| R3b-60 | fixed | New PLT-132: `WindowsHeaders.h` in the platform layer's project owns the macro family and is the only includer of `<windows.h>`; every other source includes it, checked by `Build\CheckProjectFiles.py` (T35). The name follows BLD-020's reading of R7 for a header without a type (`BuildIdentityFields.h`). Registry and checker rows are Requests 4 and 1. |
| R2b-208 | fixed | The explicit `<a id="415-build-identity-products-and-flavours">` is deleted; the heading's own slug `415-build-identity-products-and-configurations` is already what the index links to, and no document linked the old anchor. |
| R2b-209 | fixed | The PLT-020 sentence is deleted, and with it the DEC-333 row of PLT §8.1. |
| R2b-210 | fixed | The PLT-101 sentence is deleted. |
| R3b-47 | fixed | PLT-110: "rare: the values are copied on the main thread and written by the file writer (PLT-081)". |
| R1b-5 | fixed-differently | Table 6.1-A, whose vertical-sync column the finding corrects, is deleted under ruling 4 (R2b-199). P6-RD has already removed the attempt-2 clause from R-META-423, so nothing is left to correct. |
| R2b-204 | fixed | PLT-115 keeps only the Species rule: one compiled-in table (DEC-100); an invalid entry takes its own default (DEC-800). |
| R2b-205 | fixed | The PLT-117 clause is deleted. |
| R2b-207 | fixed | Table 4.5-A has a single "Species rule" column. Its full-screen row reads "borderless full screen with render-size scaling; the display mode is never changed". This also drops the R-SIM-575 citation (P6-RE request 6). |
| R2b-206 | fixed | Table 6.3-A is now "Item / Source" with Species wording. The external-program row is dropped (PLT-092 covers it), and the "in addition" row becomes four rows. |
| R3b-48 | fixed | `MappedFile` is declared: move-only (deleted copy), destructor unmaps, `Bytes()` returns a `std::span<const std::byte>` and `SizeBytes()` the length. PLT-090 states the moved-from and empty-file views; new T37. |

## Requests from other packets applied

| Request | File and place | Change |
|---|---|---|
| P6-RD 1 | `platform_layer.md` PLT-029, table 3.4-A | PLT-029 now names the render size and window mode (RND-442; R-META-414, R-META-424), with PLT-028's placement settings and their defaults, and "no other screen setting (RND-443)". Table 3.4-A is deleted. The dead citations of R-META-418, R-META-419 and the `ref.meta.options.screen.*` defaults are gone. |
| P6-RD 2 | `platform_layer.md` PLT-073 | "the size list of the screen-options page (R-META-415)". |
| P6-RD 3 | `platform_layer.md` PLT-113, table 6.1-A, §4.16 keys and intro sentence, PLT-112 | All deleted; PLT-113 is a stub. PLT-112 now runs at start-up and when the screen options apply a new render size or window mode (R-META-423). An apply makes the attempts in place on the existing window and swap chain; PLT-114, the caption of diagram 6-A and T15 follow. |
| P6-DAT 4 | DBG `README.md` §2.1, DBG-161; `test_harness_soak_and_telemetry.md` | The telemetry citation comment replaces the declaration (R4a-11); DBG-161 states the automated participants (R4a-13); `SoakHost` sits beside the runner, which now takes `const SoakConfigurationRecord&` and `SoakHost&` (R3b-27). |
| P6-RE 6 | `performance_baseline.md` DBG-201; `platform_layer.md` table 4.5-A | The "slow frames" row with the retired key is deleted. The vertical-sync row's "(R-SIM-575)" went with the Reference column (R2b-207). I also made PLT-056 agree with the new R-SIM-575 and DBG-203 H14 agree with the audio thread. |
| P6-ADR 3 | `platform_layer.md` PLT-104, §8.1 | `BuildIdentity.h` becomes `BuildIdentityFields.h`; ADR-076 has its own row, "Accepted, screenshot encoding with the in-box WIC PNG encoder; video by external tools", and the Proposed row lists ADR-065, ADR-028 and ADR-077. |
| P6-ADR 4 | `test_harness_soak_and_telemetry.md` DBG-141 fuzz row | The text exactly as requested (it also fixes R2b-244). |

## Requests

1. **Coordinator (P6-ADR has finished): `architecture/components/build_tooling_and_distribution.md` (R3b-9, R3b-60).** The file is 121,305 bytes; these edits add about 700 bytes, under the 122,880-byte limit.
   - a. Table 4.1-B, row 13: replace the row with "| 13 | a source file includes a header of a project that its project does not reference, which is every library's include allow-list until DEC-222 decides otherwise | BLD-041, DEC-222 |".
   - b. Table 4.1-B, add after row 13: "| 14 | a source file breaks a source rule of a component: a platform service of PLT-003 used outside the platform layer's project; `<windows.h>` included, or the Windows macro family defined, outside `WindowsHeaders.h` (PLT-132); a map hash embedded (BLD-076); a mesh socket looked up by name text (DAT-277); an allocator, lock, logger or voice-creation call in an audio callback unit, or an XAudio2 or X3DAudio include outside the voice back-end (AEN-025, AEN-003); a test method whose `Requirement` names no requirement id of the design (DBG-140) | PLT-003, PLT-132, BLD-076, DAT-277, AEN-025, DBG-140 |".
   - c. BLD-076: replace "and a lint step rejects any source file that embeds a map hash (ADR-060)" with "and `Build\CheckProjectFiles.py` rejects any source file that embeds a map hash (table 4.1-B row 14, ADR-060)".
   - d. §6 failure table, the row "a source file that embeds a map hash | lint step | …": "lint step" becomes "`Build\CheckProjectFiles.py`".
   - e. §7, T24: kind "lint fixture" becomes "checker fixture".
2. **P6-DEC, `decisions/`.**
   - a. **DEC-971** (ruling 10). Replace "The exact hardware is not named." in Specified behaviour with: "The exact hardware is not named. Until the design phase names it, the first machine that meets the minima of ADR-050 and ADR-002 and runs the performance job is pinned and recorded as the reference test machine; until that machine is pinned, every performance test runs and reports its result with the build identity and the hardware of the agent that ran it, and does not fail (DBG-208, DBG-209). A budget stated on the minimum CPU, on minimum-specification hardware or on the minimum-specification machine is stated on the reference test machine." Option A becomes "as stated: the interim rule, then the pinned machine". Add `audio_engine/` (AEN-303), `input/` (INP-200), `ui_framework/` (UI-240), `camera_and_sequences.md` (CAM-120), `data_schemas/` (DAT-050) and `terrain/` (T26) to Affects, and repoint the DBG path to the folder `architecture/components/diagnostics_test_and_performance/` (R4b-77).
   - b. **DEC-222** item 3 (R3b-9). Add to Specified behaviour: "Until it is decided, each library's include allow-list is the headers of the projects it references (BLD-041), checked by `Build\CheckProjectFiles.py` (build tooling table 4.1-B row 13), so the boundary tests of the components run from the first package."
   - c. **DEC-221** (R3b-14). Add to Specified behaviour: "Until it is decided, the client's composition root is the entry code of the client project; it runs the start-up and shutdown sequences of the platform layer (PLT-038, PLT-043), and the platform layer provides only its own steps, the log sink and the lifecycle hooks (PLT-133, PLT-045)."
   - d. **DEC-229** item 6, Foundations, exit check: "with the forbidden-include and static checks active" becomes "with the include allow-list and the source rules of `Build\CheckProjectFiles.py` active (build tooling table 4.1-B rows 13 and 14)".
   - e. **DEC-200**: add `architecture/components/diagnostics_test_and_performance/` (DBG-160, DBG-164: the soak runs in one process with no client until the networking model exists) to Affects.
   - f. Optional. **DEC-333** and **DEC-341**: no PLT requirement cites them any more, and no DBG requirement cites DEC-341, so remove `platform_layer.md (PLT)` from both Affects lines and the DBG path from DEC-341's.
3. **Coordinator (P6-DAT has finished): `architecture/components/data_schemas/test_and_telemetry_records.md` (R3b-28).**
   - Table 3.1-A, row `clients`: Type "count, at least 1" becomes "count, 0 or more". Presence and default becomes "defaulted: 0, the single-process pass of DBG-160". Rules becomes "the client participants the runner starts (DBG-160, DBG-163); a pass with clients exists only once the networking model exists (DEC-200) and then needs at least the number that, with the host, reaches `dbg.soak.min_participants`".
   - T70: add "a configuration without `clients` validates as a single-process pass".
4. **P6-GLO, `CONVENTIONS.md`.**
   - a. Table 8.12-B, PLT "clock and window": remove `ModeSize` (replaced by `SizePx`; R2b-203).
   - b. Table 8.12-B, PLT "files, lifecycle, threads and services": add `PlatformLogSink` and `PlatformLogLevel` (`Debug`, `Info`, `Warning`, `Error`), and add the free functions `SetPlatformLogSink`, `InstallFatalHandlers` and `CheckOperatingSystem` to the PLT free-functions group of R4a-39.
   - c. Table 8.12-B, PLT "data names", or a files group: add the header `WindowsHeaders.h` (PLT-132).
   - d. Table 8.12-B, DBG "types and macros": add `SoakHost`. `SoakReport` is now declared and stays (P6-DAT's request 1 already removes `SoakConfig`, `SoakProgression` and `TelemetrySessionRecord` from this row).
   - e. §8.14, PLT terms: add "composition root, log sink". DBG terms: add "single-process pass, soak host".
5. **P6-RND (R3b-9, ruling 2).**
   - a. `renderer/README.md` T1: kind "build check" becomes "checker fixture". Checks: "a simulation-library source that includes a renderer header fails `Build\CheckProjectFiles.py` (build tooling table 4.1-B row 13); the renderer has only const access to the snapshot; the dedicated-server link map has no Direct3D symbol, because the server project references and links none (row 11, BLD-045)".
   - b. `animation_and_vfx/README.md` T1: "no source of this component includes the simulation generator's interface (`Build\CheckProjectFiles.py`, build tooling table 4.1-B row 13) or takes non-const access to the snapshot; the dedicated-server link map holds none of its symbols (row 11, BLD-045)".
   - c. Check that RND-020's order and the two-phase `AssetCache` of `asset_pipeline_and_resources.md` match PLT-038 steps 11 and 13. Step 11: mount and CPU loads, no GPU resource. Step 13: device, `GpuResourceFactory` and swap chain, then `AssetCache::AttachGpu`, then the window is shown.
   - d. Optional (R3b-10). Write "on the reference test machine (DBG-209)" where `renderer/options_and_budgets.md` RND-454, T7 and §8.2 item 1, `terrain/README.md` T26 and `asset_pipeline_and_resources.md` design input 7 say "minimum-specification" or "minimum CPU". DBG-209 now defines those phrases, so this is wording only.
6. **P6-ENG1 (R3b-9).**
   - a. `audio_engine/README.md` T2: kind "static analysis" becomes "checker fixture"; append "(`Build\CheckProjectFiles.py`, build tooling table 4.1-B row 14)".
   - b. `audio_engine/README.md` T3: after "audio code includes no simulation-generator or writable simulation header" add "(build tooling table 4.1-B row 13)".
   - c. `ui_framework/README.md` R1: "a UI build that includes a simulation write interface fails to compile" becomes "a UI source that includes a simulation write interface fails `Build\CheckProjectFiles.py` (build tooling table 4.1-B row 13)".
   - d. `input/README.md` T20: append to "the simulation library has no dependency on the input library" the text "(`Build\CheckProjectFiles.py`, build tooling table 4.1-B rows 11 and 13)".
   - e. Optional (R3b-10): AEN-303, T27, INP-200, T29, UI-240, `ui.budget.cpu_ms` and pages T33 may write "on the reference test machine (DBG-209)" for "minimum CPU" or "minimum-specification".
   - f. For information: the DBG marker catalogue now names the audio markers `audio.update` (with one marker per AEN-123 step), `audio.stream` and `audio.bank_load`, as AEN-322 states. No AEN change is needed.
7. **Coordinator (P6-DAT has finished): `data_schemas/entity_audio_geometry_and_panel_schemas.md` T57 (R3b-9).** "a lint rule rejects socket look-ups by name text in code" becomes "`Build\CheckProjectFiles.py` (build tooling table 4.1-B row 14) rejects socket look-ups by name text in code".
8. **P6-RND (ruling 2), for information.** ADR-078 landed while I worked. The platform layer now cites it in PLT-038 step 13, in PLT-043 (the detachment before the renderer) and in §8.1 and its status line. ADR-078 names no attach method, so I kept ruling 2's name `AssetCache::AttachGpu`. If AST names it differently, tell the coordinator and PLT-038 step 13 follows.
9. **Coordinator, requirements index.** Regenerate `REQUIREMENTS_INDEX.md` and `requirements_index/`:
   - new: PLT-132 and PLT-133;
   - stubs: PLT-006, PLT-113 and DBG-005;
   - text changed: PLT-003, PLT-021, PLT-027, PLT-029, PLT-038, PLT-070, PLT-071, PLT-112, PLT-114, PLT-115, DBG-022, DBG-042, DBG-048, DBG-070, DBG-120, DBG-160, DBG-161 and DBG-209, among others.

   After regeneration, the three stubs may be deleted if nothing but the index cites them.

## Noted, not changed (outside my packet's findings)

- DBG-121 and the DBG-221 column "With two or more participants in the reference" describe how reference cheats act. They are the basis of the classification (DEC-202 item 8), so I kept them.
- DBG-240's first sentences restate R-SIM-740 and R-SIM-741, and DBG-065 says "at least the content that the reference debug log carries". Neither describes the reference's code; both state observable content.
- PLT-025 and PLT-044 can raise a fatal exit (`FatalExit`, DBG-074) at step 5, before the logger exists at step 7. PLT-133 now covers this case: records before step 7 are held, and with no sink the fatal path goes straight to the platform. DBG-074 might say that before step 7 only the platform's `EndWithMessage` part runs; it is a one-line clarification for a later pass.
