"""F2c routing plan for ORIG 40, 41 and 90.

Usage: python3 -I F2c_routing.py <orig_dir> <headings.csv> <out.csv>

Writes the routing CSV incrementally (one file at a time), then checks
coverage: every line from the H1 heading to the H1 end line of headings.csv
is covered, and a line covered by several rows has at most one non-shared row.
Prints counts per action, per owner_package and per DEFER-DESIGN topic.
"""
import csv
import os
import re
import sys
from collections import defaultdict

F40 = "40_carry_forward_assessment.md"
F41 = "41_risks_and_open_questions.md"
F90 = "90_glossary.md"

DR = "decisions/decision_register.md"
ADRF = "adr/"
G00F = "reference/00_reference_overview.md"
GLO = "glossary.md"

DD_TOPIC = {
    "DD-NET": "networking model",
    "DD-LOBBY": "lobby, discovery and session protocol",
    "DD-NUMERIC": "simulation numeric model",
    "DD-THREADS": "simulation threading and process architecture",
    "DD-SERVICES": "sessions, identity and platform services",
    "DD-TERRAIN-RUNTIME": "terrain runtime model",
    "DD-PERSIST": "saves and persistence formats",
}
KH_FEATURE = {
    "KH-EDITOR": "map editor",
    "KH-REPLAY": "replays",
    "KH-ACHIEVE": "achievements and leaderboards",
    "KH-RESULTS": "post-match results screen",
    "KH-MUSIC": "music",
}

ROWS = []  # (file, start, end, action, target_file, target_section, owner, note, shared, anchor_override)


def R(f, s, e, action, tfile="", tsec="", owner="", note="", shared=False, anchor=None):
    ROWS.append((f, s, e, action, tfile, tsec, owner, note, shared, anchor))


def drop(f, s, e, code, text, shared=False, anchor=None, legacy=False):
    R(f, s, e, "DROP-LEGACY" if legacy else "DROP-PROVENANCE", "", "", "",
      f"{code} — {text}", shared, anchor)


def defer(f, s, e, code, text, shared=False, anchor=None):
    R(f, s, e, "DEFER-DESIGN", DR,
      f"DEC (Open, Decide by: design phase) — {DD_TOPIC[code]}", "X-DEC",
      f"{code} — {text}", shared, anchor)


def hooks(f, s, e, codes, text, shared=True):
    feats = "; ".join(KH_FEATURE[c] for c in codes)
    R(f, s, e, "KEEP-HOOKS", DR,
      f"DEC (40 verdict Drop (redesign); designed by the design phase) — {feats}", "X-DEC",
      "; ".join(codes) + " — " + text, shared)


def dec(f, s, e, tsec, note, shared=False, anchor=None):
    R(f, s, e, "TO-DEC", DR, tsec, "X-DEC", note, shared, anchor)


def adr(f, s, e, tsec, note, shared=False):
    R(f, s, e, "TO-ADR", ADRF, tsec, "X-ADR", note, shared)


# --------------------------------------------------------------------------
# 40 — carry-forward assessment
# --------------------------------------------------------------------------
drop(F40, 1, 8, "DP-INDEX", "H1 title block: provenance header, evidence legend, sources merged, target and licence statement (superseded by prompt §0 and the vision brief)")
drop(F40, 9, 25, "DP-INDEX", "contents list")
drop(F40, 26, 27, "DP-INDEX", "§1 heading only; its H3 children are routed separately")
drop(F40, 28, 38, "DP-METHOD", "§1.1 verdict legend (Keep, Adapt, Replace, Drop, Drop (redesign)); give it to X-DEC and X-ADR as read-only context for the verdict columns of §1.2, §2 and §5 (report: cross-document requests)")
dec(F40, 39, 53, "DEC entries for the ten headline verdicts D1–D10",
    "§1.2 D-table, one H3 routed once; X-DEC and X-ADR read it in full. Split: D1 → KEEP G00 (reference core loop) + DEC (the vision overrides no-build-queue/no-currency for the world game only); D2 → DEFER DD-NET; D3 → DEFER DD-NUMERIC; D4 → DEFER DD-THREADS; D5 → TO-ADR renderer (the look is a C-RND visual-language requirement); D6 → DEC Resolved: every timer an integer tick count, mode second kept at 11 ticks as an explicit constant, re-tune only after playtest; D7 → DEFER DD-SERVICES (+ DD-LOBBY); D8 → TO-ADR content pipeline + DEFER DD-TERRAIN-RUNTIME (baked heightfields); D9 → TO-ADR audio engine (binding model KEEP in G10 and C-AEN); D10 → KEEP-HOOKS + DROP-LEGACY DL-CUT. Shared single-line rows follow for each split")
R(F40, 43, 43, "KEEP", G00F, "§1 Purpose and player experience: the reference core loop and what makes it distinctive", "G00",
  "D1 row (shared with the §1.2 TO-DEC row): crowd-and-territory core; no build queue and no currency; armies produced by held spawn points (≥ 3 of 5 ports, 30 s activation, burst/pause production, per-team cap); autonomous soldiers; officers as signposts and formation leaders; crates 90 % good / 10 % hostile feeding six task slots; six modes on one rules skeleton; node-graph CPU opponents with perfect information and no other cheats", True)
defer(F40, 44, 44, "DD-NET", "D2 row (shared): lockstep over a server-sequenced command stream and its rebuilt preconditions (integer ticks, per-match state, RNG discipline, state hashing, command authority, content hashes, version gating, snapshot join); capture as failure modes to prevent, plus the 1400–2000 bodies per map scale figure", True)
defer(F40, 45, 45, "DD-NUMERIC", "D3 row (shared): replace the numeric environment; capture the hazards (extended-precision FPU state, compiler float flags, transcendental rounding, FMA contraction, cross-architecture play and the fixed-point reserve)", True)
defer(F40, 46, 46, "DD-THREADS", "D4 row (shared): per-match world object, pure step function, hard simulation/presentation boundary, deterministic job system; the integrity rules themselves are KEEP in reference/12 (F2b routing of 22)", True)
adr(F40, 47, 47, "renderer architecture: frame graph, PSO catalogue derived from the ordered pass table, instancing of units and sprites, GPU particles, flip-model swap chain and presentation",
    "D5 row (shared): replace the backend, keep the look; flat-shaded, additive-glow-on-black and darkening-layer style is a C-RND visual-language requirement, not an ADR decision", True)
defer(F40, 49, 49, "DD-SERVICES", "D7 row (shared): session, lobby and platform services behind a transport abstraction; also feeds DD-LOBBY (host or relay validates every lobby and game command; client-side lobby authority, clear-text password and advertised host key are the failure modes)", True)
adr(F40, 50, 50, "data schema format and versioning; texture pipeline (BCn-compressed DDS); pack-file format with hashing; build-time validators",
    "D8 row, content-pipeline part (shared): typed versioned schemas replace positional text tables and sentinel-valued options; hashed pack files replace solid archives with loose-file override", True)
defer(F40, 50, 50, "DD-TERRAIN-RUNTIME", "D8 row, terrain-runtime part (shared): baked heightfields with a content hash per map versus generation at load; the generator algorithm stays KEEP in C-TER", True)
adr(F40, 51, 51, "audio engine: XAudio2 voices, virtual voices, authored priorities, buses",
    "D9 row (shared): replace the engine and the mix; the (source, event) binding model with friend/foe variants, sample groups and generative loops stays KEEP in G10 and C-AEN; music is KEEP-HOOKS", True)
hooks(F40, 52, 52, ["KH-EDITOR", "KH-REPLAY", "KH-ACHIEVE", "KH-RESULTS", "KH-MUSIC"],
      "D10 row (shared): achievements, leaderboards, music, map editor, replays and post-match results exist only as stubs or nothing and are designed from scratch; record the verdict; the hooks themselves come from 08 §8, 09 §7, 10 §6.4 and §14, 01 §2.11 (F2a routing)")
drop(F40, 52, 52, "DL-CUT", "D10 row (shared): cut-scene script interpreter as a gameplay system, narrator, loader mini-programs, gesture recogniser; the same row drops the campaign (DL-CAMPAIGN), the D3D9 emulator (DL-D3D9) and the legacy UI toolkit (DL-LEGACY-TECH)", shared=True, legacy=True)
drop(F40, 54, 64, "DP-METHOD", "§1.3 verdict tally (counts per verdict over §2.1–§2.2)")
drop(F40, 65, 68, "DP-METHOD", "§1.4 one-paragraph recovery summary; restated by §1.2 and §4")
dec(F40, 69, 90, "fact corrections behind DEC entries (consistency notes C1–C12)",
    "C1 synced generator seeded per match (integrity; reference/12 keeps seed per match); C6 nuke sea search terminates on reference data but must be bounded (F33); C7 loose-file override in development builds only (→ TO-ADR, shared row); C8 three statics survive between matches (integrity, Resolved via F4 and F29); C9 difficulty preference forced to 0 (no live path); C10 latest-only coalescing loses one-shot input (→ DD-NET, shared row); C11 spawn-link transit 10h + 1 ticks (G05 owns it from 05 §3.4); C12 build status of capture, tools menu and profiler (drop classes for C-DBG); C2–C5 are not live defects (lessons: every interaction through commands; validate references at build time); line 88 spot-check list is DP-METHOD")
adr(F40, 81, 81, "pack-file format and the no-loose-override rule for shipping",
    "C7 row (shared): loose data files override archives only in development builds (G72)", True)
defer(F40, 84, 84, "DD-NET", "C10 row (shared): latest-only coalescing of the continuous control packet discards an earlier packet's one-shot flags; coalesce continuous state only and deliver one-shot events exactly once in tick order", True)
drop(F40, 91, 94, "DP-METHOD", "§2 heading and matrix reading notes")
dec(F40, 95, 111, "scope verdicts for the reference mechanics (Part A subsystems R1–R12)",
    "R1 modes Keep; R2 population and spawn economy Keep with four designer decisions; R3 units Adapt; R4 buildings Adapt; R5 crates Keep; R6 AI Keep and fix 24 bugs; R7 control Adapt; R8 camera Adapt; R9 levels and terrain generation Adapt; R10 level format and editor Replace; R11 meta Adapt / Drop (redesign); R12 audio design Adapt. Inform scope for G01–G11. Shared rows: R7, R10 → TO-ADR; R9 → DD-TERRAIN-RUNTIME; R10 → KH-EDITOR; R11 → KH-ACHIEVE + DD-LOBBY; R12 → KH-MUSIC")
adr(F40, 105, 105, "input (GameInput)", "R7 row (shared): replace the window-message and legacy pad drivers; the command model and control scheme stay KEEP in G06 and C-INP", True)
defer(F40, 107, 107, "DD-TERRAIN-RUNTIME", "R9 row (shared): terrain generator as an editor-side tool, baked heightfields with a content hash instead of regeneration at load", True)
adr(F40, 108, 108, "data schema format and versioning (versioned, schema-validated map asset with a separately readable header)",
    "R10 row (shared): replaces the positional text level format and its container", True)
hooks(F40, 108, 108, ["KH-EDITOR"], "R10 row (shared): new editor with undo, lossless round-trip, stable-id selection, link tools and in-editor play-test of the real simulation")
hooks(F40, 109, 109, ["KH-ACHIEVE"], "R11 row (shared): achievements (12 designed conditions, stubbed), leaderboards (no back end) and quick match redesigned on platform services")
defer(F40, 109, 109, "DD-LOBBY", "R11 row (shared): lobby state as a sequenced command stream, host or server validation of every lobby command, one atomic versioned lobby-state update; quick match also feeds DD-SERVICES", True)
hooks(F40, 110, 110, ["KH-MUSIC"], "R12 row (shared): music and match-end stings that never existed are designed new; the binding model, priorities and buses stay in G10 and the audio ADR")
# §2.2, contiguous split: one action per subsystem row
adr(F40, 112, 116, "audio engine", "R13 audio engine Replace (H3 heading and table header included; columns per line 114)")
defer(F40, 117, 117, "DD-THREADS", "R14 architecture and main loop Replace: god object, raw-pointer services, statics reset by hand, flag-driven loops; main-loop timing (tick rate, render interpolation) stays KEEP in reference/12 (F2b routing of 20)")
dec(F40, 118, 118, "scope verdict: simulation, entity model, spatial grids and routing",
    "R15 Adapt: three object families, advance/render split, per-team grids, routing; informs C-SIM scope and the F43 and F44 decisions")
defer(F40, 119, 119, "DD-NUMERIC", "R16 determinism: Replace the mechanism / Keep the discipline; the discipline (RNG streams, draw order, integrity rules) stays KEEP in reference/12 (F2b routing of 22)")
defer(F40, 120, 120, "DD-NET", "R17 networking (transport, lobby, matchmaking): relay-lockstep topology, coalescing of continuous control state with one-shot events merged, lobby as commands; also feeds DD-LOBBY")
adr(F40, 121, 127, "renderer architecture; shaders (DXC, HLSL); texture pipeline and pack files; animation and VFX (GPU particles, instancing); UI framework; data schema format",
    "R18 rendering backend, R19 render passes and visual style (style KEEP as C-RND visual-language requirements), R20 shaders, R21 textures, assets and archives, R22 animation, VFX and particles, R23 UI framework (front-end page patterns Adapt), R24 data formats")
dec(F40, 128, 128, "scope verdict: scripting, tutorials and cut-scenes",
    "R25: script interpreter and content Drop (DL-CUT); tutorials and presentation timelines Drop (redesign) as step graphs (G11 owns the tutorial reference)")
adr(F40, 129, 132, "platform and toolchain baseline; build and CI; test framework, profiling and logging; localisation; crash reporting and telemetry with consent",
    "R26 platform, build and tools; R27 debug, test and profiling; R28 localisation; R29 crash reporting")
defer(F40, 133, 134, "DD-SERVICES", "R30 authentication and DRM: drop the key system; entitlement through platform services (the legacy key gating is DL-DEMO)")
# §2.3 finer-grained verdicts
dec(F40, 135, 214, "DEC entries for the finer-grained verdicts G1–G72",
    "G1–G72: Keep and Adapt rows → Resolved DECs or confirm-DECs for G01–G11; Drop rows → Dropped DECs (G10, G11 DL-LEGACY-TECH; G21, G26 DL-DEBUG; G35 DL-CUT and DL-DEAD; G45 DL-D3D9; G64, G65 DL-DEAD; G68, G69 DL-CAMPAIGN); decide rows G25 (multi-round Assault), G43 (seasonal mode), G44 (negative renderer). Shared rows: KEEP-HOOKS G36, G40, G54, G57, G70; DEFER G28, G30, G34, G37, G38, G39, G56, G58–G59, G67; TO-ADR G14, G46–G52, G55, G61–G66, G67, G71–G72")
adr(F40, 154, 154, "input (GameInput) binding back-ends", "G14 composable input binding graph Adapt (shared): reject unknown condition words; the graph design is KEEP in C-INP", True)
defer(F40, 168, 168, "DD-NET", "G28 (shared): leaver replaced by a CPU stays a reference rule (G01); reconnection that reclaims the slot is the networking model's job", True)
defer(F40, 170, 170, "DD-NET", "G30 spectator team (shared): spectating via snapshot and stream", True)
defer(F40, 174, 174, "DD-NET", "G34 loading screen (shared): network keep-alive during load; progress display and async streaming are engine scope (asset pipeline ADR)", True)
hooks(F40, 176, 176, ["KH-ACHIEVE"], "G36 achievements (12 conditions), statistics, leaderboards (shared)")
defer(F40, 177, 177, "DD-SERVICES", "G37 quick match (shared): skill, region or ping matchmaking on the platform, or none", True)
defer(F40, 178, 178, "DD-LOBBY", "G38 LAN/WAN server browser with password and filters (shared)", True)
defer(F40, 179, 179, "DD-SERVICES", "G39 demo and full gating (shared): platform trial instead; the legacy gating is DL-DEMO", True)
hooks(F40, 180, 180, ["KH-RESULTS"], "G40 post-match results screen (shared): absent in the reference, which freezes the match under an overlay")
adr(F40, 186, 192, "renderer architecture (GPU terrain with chunked culling, wire overlay as an edge term, GPU water, instanced crowd cards and shadows, instanced shard buffer); animation (client-side gait planner and two-bone IK); UI glyph cache with shaping",
    "G46–G52 (shared): terrain strip mesh, dynamic water, unit cards blending to billboards, blob and sprite shadows, shape shattering, legged locomotion, bitmap font atlases; the look of G48–G49 stays KEEP in C-RND", True)
hooks(F40, 194, 194, ["KH-MUSIC"], "G54 music system (shared): state-driven layered score, new composition")
adr(F40, 195, 195, "audio engine: per-bus effects", "G55 per-event DSP effects (inactive) replaced by per-bus effects (shared)", True)
defer(F40, 196, 196, "DD-NET", "G56 late join by replaying the whole command history (shared): snapshot plus tail", True)
hooks(F40, 197, 197, ["KH-REPLAY"], "G57 command-history record and replay (shared): formal replay file with seed, hashes and snapshots")
defer(F40, 198, 199, "DD-NET", "G58 snapshot and sync hash in debug builds only, G59 desync world dump (shared): hash canonical state in every build; structured state diff at the failing tick", True)
adr(F40, 201, 206, "diagnostics: sequenced debug commands, profiling (PIX, ETW, GPU timestamps), headless CI soak, consented telemetry, screenshot readback and encoding",
    "G61 cheat window, G62 profiler, G63 testbed (headless CI soak), G64 benchmark and upload (DL-DEAD; telemetry with consent), G65 frame capture (DL-DEAD; external tools), G66 screenshots and poster maker (shared)", True)
adr(F40, 207, 207, "content build: content hash per map", "G67 map identity (shared): content hash produced by the content build", True)
defer(F40, 207, 207, "DD-NET", "G67 map identity (shared): exchanging and checking content hashes before a match", True)
hooks(F40, 210, 210, ["KH-EDITOR"], "G70 map editor and custom maps (shared): new editor with play-test (R10)")
adr(F40, 211, 212, "platform layer clock with injectable time; pack-file no-loose-override rule for shipping",
    "G71 process-relative clock with injectable fake time Keep; G72 loose data override dropped for shipping, development builds only (shared)", True)
# §3 obsolete-technology mapping
drop(F40, 215, 216, "DP-INDEX", "§3 heading only; its H3 children are routed separately")
adr(F40, 217, 231, "renderer architecture: PSOs, root signature and bindless policy, barrier model, instancing, swap chain and presentation; minimum feature level and Shader Model",
    "T1–T9 (H3 heading, intro and table header lines 217–222 included): fixed-function pipeline, immediate mode, display lists, the D3D9 emulation layer, device-loss handling, extensions and hardware blacklists, matrix stacks, back-buffer copies and read-back, window, swap and display mode")
defer(F40, 232, 233, "DD-NUMERIC", "T10 extended-precision FPU state and float flags, T11 global synced generator and global LCG; the RNG streams and draw discipline stay KEEP in reference/12 (F2b routing of 22); the generator algorithm choice belongs to the design phase")
adr(F40, 234, 242, "audio engine and codec; texture pipeline (BCn, DDS, colour space, alpha modes); UI glyph cache; pack-file format; platform window layer; input (GameInput)",
    "T12–T20: 3D sound API, software mixer and wave-out device, Vorbis decode and PCM cache, BMP-only textures and colour-key alpha, bitmap font atlases, solid archives, window manager, window-message input driver, legacy pad API")
defer(F40, 243, 243, "DD-NET", "T21 UDP net library: listener threads per socket, critical sections, fire-and-forget sends")
defer(F40, 244, 245, "DD-SERVICES", "T22 metaserver, matchmaker and NAT punch-through (also DD-LOBBY for discovery); T23 authentication keys and demo limits (the legacy gating is DL-DEMO)")
adr(F40, 246, 247, "UI framework (retained, batched, focus graph, DPI); data schema format and versioning",
    "T24 legacy UI toolkit, T25 positional text formats")
defer(F40, 248, 249, "DD-THREADS", "T26 single-threaded simulation in 10 slices, T27 loader worker thread and main-thread job hand-off; client async loading is engine scope (the asset-pipeline ADR may cite it without a threading-model decision)")
adr(F40, 250, 254, "platform and toolchain baseline; build and dependency management; content build; client packaging and distribution; crash reporting; platform clock",
    "T28 legacy toolchain, T29 data build (batch and Perl scripts, map identity pre-build step), T30 installers, T31 crash reporter, T32 high-resolution clock with injectable fake time")
defer(F40, 255, 255, "DD-THREADS", "T33 parallel-loop pragma (inert, secondary build only); lesson: any parallel loop in the simulation must stay deterministic")
adr(F40, 256, 261, "mesh pipeline (glTF import, socket schema, flat shading, team tint); telemetry with consent; profiling (PIX); logging (ETW and TraceLogging); screenshot encoding",
    "T34 mesh exporter and text mesh format, T35 benchmark upload (DL-DEAD; consented telemetry instead), T36 profiler, T37 debug log file, T38 PNG and zlib libraries")
dec(F40, 262, 281, "DEC entries: artefacts to preserve and at what fidelity (B1–B13)",
    "§3.2 preservation table: B1–B3 tick-count timers and transit (D6, F1, F2); B4 explosion falloff (F42); B5 byte-sized stats (F39); B6 per-tick probabilities (F41); B7 grid cache staleness (F43); B8 formation volley timing (F57); B9 volume authoring scale (audio, only if old data were imported: no); B10 flat-shading provoking vertex (bake per-triangle colour; C-RND); B11 iteration-order tie-breaks (KEEP as ordered rules in reference/12); B12 terrain heights and map identities (no: IP); B13 synced RNG stream (no)")
adr(F40, 282, 325, "context for the technology-baseline ADRs (migration overview)",
    "§3.3 migration-map flowchart: N1 renderer, N5 audio, N6 input, N8 UI and glyphs, N9 textures, pack files and schemas, N10 toolchain, packaging and crash reporting are ADR topics; N3 numeric model, N4 simulation architecture and job system, N7 transport and lobbies are superseded (DD-NUMERIC, DD-THREADS, DD-NET, DD-SERVICES) and must not become ADRs; legacy nodes O1–O10 are never named in an ADR")
# §4 determinism and multiplayer recommendation (all DEFER-DESIGN; H3 rows so each topic is visible)
defer(F40, 326, 327, "DD-NET", "§4 heading")
defer(F40, 328, 347, "DD-NET", "§4.1 inputs: scale of the simulation (6 team slots; population cap 1400–2000 per map, 350–1000 per team; spirits in thousands; ≈ 670 lasers per second; 1000–2000 task searches per second; 2000–6000 simulated objects at peak; ≈ 5 KB/s down and 1 KB/s up command traffic; 10 Hz tick) and the weaknesses of the model; these are the bandwidth and population figures the DD-NET DEC records")
defer(F40, 346, 346, "DD-NUMERIC", "§4.1 target-platform note (shared): remaining nondeterminism sources on x64 (FMA contraction, reassociation, auto-vectorisation, CPU-dispatched CRT transcendentals, MXCSR state)", True)
defer(F40, 348, 364, "DD-NET", "§4.2 options compared (lockstep, state synchronisation, rollback, authoritative referee): bandwidth, client and server CPU, latency, cheat resistance, join and reconnect, determinism burden, replays, development cost")
defer(F40, 365, 370, "DD-NET", "§4.3 recommendation (lockstep; superseded by the vision: authoritative server, interest-managed replication); keep the rationale as a constraint (command bandwidth is independent of unit count at 2000+ bodies)")
defer(F40, 371, 392, "DD-NET", "§4.4 preconditions P1–P17 as failure modes the networking model must prevent: P1 fixed tick, P7 stable ids and ordering, P8 state hashing and desync handling, P9 command authority, P10 content hash, P11 version gating, P12 transport hygiene, P13 join, reconnect and host migration, P17 referee; shared rows split out P2, P3–P4, P5–P6, P10, P14, P15, P16")
dec(F40, 376, 376, "DEC (Resolved): integer tick timers", "P2 integer timers (shared): every rule timer, cooldown, lifetime and probability in ticks; the mode second explicit (D6, F1)", True)
defer(F40, 377, 378, "DD-NUMERIC", "P3 numeric model, P4 synced RNG discipline (shared); the RNG discipline itself is KEEP in reference/12", True)
defer(F40, 379, 380, "DD-THREADS", "P5 simulation isolation, P6 per-match state (shared); the integrity rules are KEEP in reference/12", True)
adr(F40, 384, 384, "content build: manifest with hashes of every simulation-relevant asset", "P10 content hash (shared): hash generation is content-build and pack-file scope; its exchange before a match is DD-NET", True)
dec(F40, 388, 388, "DEC (Resolved): bounded algorithms", "P14 bounded algorithms (shared): no unbounded rejection loops or searches; capped attempts with a deterministic fallback (F33)", True)
hooks(F40, 389, 389, ["KH-REPLAY"], "P15 replays (shared): replay file holds build identity, content hashes, seed, team layout, command stream and periodic snapshots; seek through snapshots")
defer(F40, 390, 390, "DD-THREADS", "P16 headless simulation build (shared): the simulation runs without window, GPU or audio device; the vision requires a headless server from day one", True)
defer(F40, 393, 443, "DD-NET", "§4.5 target architecture: client and sequencer flowchart and one-tick input sequence (lockstep-specific; the simulation/presentation split also informs DD-THREADS)")
defer(F40, 444, 469, "DD-NET", "§4.6 how each weakness is closed: the failure modes the networking model must prevent (1-byte sync value, permanent desync freeze, client-side lobby authority and no ownership check, no map identity exchange, version check in the browser only, latest-only control packet, slot-reuse ordering and 1-byte task ids, history-replay late join, no reconnect, unacknowledged commands and datagram stalls, render-rate-coupled latency, Assault reset outside the tick, replay without seed, unattributable desyncs); shared rows split out the non-network items")
defer(F40, 453, 453, "DD-NUMERIC", "§4.6 (shared): extended-precision and float-flag fragility", True)
dec(F40, 454, 457, "DEC (Resolved): simulation-integrity fixes", "§4.6 (shared): 1.1-second timers (D6); statics surviving between matches; UI and render code writing simulation state; render-derived tree hit spheres drawn from a shared generator", True)
dec(F40, 460, 460, "DEC (Resolved): bounded algorithms", "§4.6 (shared): unbounded rejection loops (F33)", True)
dec(F40, 467, 467, "DEC (Open): exact versus approximate grid queries", "§4.6 (shared): derived caches changing results (F43)", True)
defer(F40, 468, 468, "DD-THREADS", "§4.6 (shared): single-threaded simulation; a parallel-loop pragma would break determinism", True)
defer(F40, 470, 482, "DD-NET", "§4.7 tick rate, input delay and budgets: per-tick CPU budget at the population cap (≤ 8 ms at 20 Hz on minimum spec), bandwidth multiples per tick rate, the 2× stress map; the reference 10 Hz tick stays KEEP in reference/12")
defer(F40, 483, 488, "DD-NET", "§4.8 transport abstraction and sequencer placement")
defer(F40, 489, 490, "DD-LOBBY", "§4.8 lobby (game lobby state owned by the sequencer, changed only by validated commands) and discovery (LAN beacon with version and content hash)")
defer(F40, 491, 491, "DD-SERVICES", "§4.8 identity and entitlement (platform account and auth ticket validated by the sequencer)")
defer(F40, 492, 496, "DD-NET", "§4.8 CPU players as simulation entities on every peer, leavers (CPU takeover is a KEEP rule in G01; slot reclaim is DD-NET), spectators")
# §5 keep-list and fix-list
drop(F40, 497, 498, "DP-INDEX", "§5 heading only; its H3 children are routed separately")
R(F40, 499, 530, "KEEP", G00F, "§1 Purpose and player experience: what makes the reference distinctive (keep-list mechanics)", "G00",
  "K1–K25 keep-list: the core-loop summary for G00; each mechanic is specified in full by its owning package (K1–K4, K13–K14 G05; K5 G02a; K6–K8, K12, K21 G06; K9–K11 G04; K15–K17 G01; K18 G07; K19–K20 G08 and G09; K22 C-RND; K23 G10; K24 G11; K25 DD-NET)")
dec(F40, 499, 530, "DEC entries: keep-list confirmations K1–K25",
    "K1–K25 (shared with the KEEP row): Resolved keep-as-specified decisions; the vision overrides K1 (no build queue, no currency) for the world game only", True)
defer(F40, 529, 529, "DD-NET", "K25 bounded coalescing of continuous control state (shared)", True)
dec(F40, 531, 597, "DEC entries: fix-list F1–F59",
    "§5.2 fix-list, 59 rows F1–F59 (the brief says 61): Keep for feel → Resolved as a rule (F1, F42); Fix → Resolved; Decide → Open with the stated default (F5, F14, F25, F26, F27, F34, F40, F43, F53); crash and simulation-integrity fixes always Resolved (F4, F6, F24, F29, F33, F39, F45, F46); lockstep-only defects go to the single DD-NET entry (F16, F30, F31, F59); hook redesigns F7, F23, F49, F55; engine fixes F50, F51, F52, F54")
hooks(F40, 543, 543, ["KH-ACHIEVE"], "F7 achievements evaluated but never awarded (shared): platform achievements; resolve threshold mismatches")
defer(F40, 552, 552, "DD-NET", "F16 server relays option, start and ready commands from any client (shared): command authority", True)
hooks(F40, 559, 559, ["KH-RESULTS"], "F23 no results screen (shared): post-match results screen with rematch")
defer(F40, 566, 567, "DD-NET", "F30 sync value and permanent desync freeze; F31 implicit bit-exactness preconditions (also DD-NUMERIC) (shared)", True)
hooks(F40, 585, 585, ["KH-MUSIC"], "F49 music never heard (shared): state-driven layered score")
adr(F40, 586, 588, "texture pipeline (cache keyed by asset and variant, import manifest); localisation (per-key fallback); data schema (one defaults table)",
    "F50 texture cache keyed by name, F51 missing phrases and partial languages, F52 defaults read before archives mount (shared)", True)
adr(F40, 590, 590, "UI framework", "F54 UI framework defects fixed by construction (shared)", True)
hooks(F40, 591, 591, ["KH-EDITOR"], "F55 editor defects fixed by construction in a new editor (shared)")
defer(F40, 595, 595, "DD-NET", "F59 latest-only control packet drops one-shot flags (shared)", True)
drop(F40, 598, 615, "DP-CLOSING", "§6 IP replacement summary; consumed by the F1 ledger")
R(F40, 609, 610, "TO-ASSETS", "assets/README.md", "counts by category (cross-check only)", "X-AST",
  "§6 Art and Audio rows (shared): category counts (textures, font atlases, meshes, panel animations, shaders; samples, playable samples, music tracks); every asset new; the audio event list kept by function; cross-check for the manifest only, never the original names", True)
defer(F40, 616, 675, "DD-NET",
      "also DD-NUMERIC; DD-THREADS; DD-SERVICES; DD-TERRAIN-RUNTIME — §7 build order and work breakdown: the roadmap is the design phase's job and no drop_topics code covers it (DD-ROADMAP proposed in the report). Only per-topic lessons feed DECs: determinism first with cross-CPU replay hashing and a headless runner (DD-NUMERIC, DD-THREADS); networking exit criterion 4 clients, lossy link, 30 minutes, no desync (DD-NET); platform services in P7 (DD-SERVICES); baked-heightfield loader and baker (DD-TERRAIN-RUNTIME). Principle 3 repeats the §5.2 Decide items; principle 4 matches the X-AST placeholder rule")
drop(F40, 676, 699, "DP-CLOSING", "open questions and unknowns 40-U1…40-U17; in-scope items also routed TO-DEC (shared row)")
dec(F40, 676, 699, "Open DEC entries from the 40 open questions",
    "shared with the DP-CLOSING row: U2 11-tick second (D6, F1); U3 handicap step and cap divisor (F25, F27); U4 any-team harvesting (F26); U5 special teams in win checks (F28); U10 approximate grid queries (F43); U14 whether reference tuning values may be used verbatim (owner question, see report); U1 performance envelope unmeasured (also DD-NET); netcode, numeric and services questions split out as shared DEFER rows; U13 concerns IP-register completeness only", True)
defer(F40, 682, 682, "DD-NET", "40-U1 (shared): simulation and frame cost at the population cap never measured (tick rate, per-tick budget, caps)", True)
defer(F40, 687, 688, "DD-NET", "40-U6 real-world desync frequency, 40-U7 host-quit behaviour (shared)", True)
defer(F40, 689, 689, "DD-SERVICES", "40-U8 server side of the metaserver and matchmaker (shared)", True)
defer(F40, 690, 690, "DD-NUMERIC", "40-U9 cross-build float lockstep evidence (shared)", True)
defer(F40, 692, 692, "DD-TERRAIN-RUNTIME", "40-U11 bit-equality of the terrain port with the reference generator (shared)", True)
hooks(F40, 693, 693, ["KH-ACHIEVE"], "40-U12 whether a retail achievement layer existed (shared): reference for achievement design only")
defer(F40, 696, 697, "DD-NUMERIC", "40-U15 ARM64 support, 40-U16 non-Windows platforms and cross-play (shared)", True)
defer(F40, 698, 698, "DD-SERVICES", "40-U17 launch platform service set (shared)", True)
drop(F40, 700, 712, "DP-CLOSING", "docs-vs-code discrepancies")
drop(F40, 713, 726, "DP-CLOSING", "IP items; consumed by the F1 ledger")
drop(F40, 727, 785, "DP-CLOSING", "source map")
drop(F40, 786, 806, "DP-CLOSING", "appendix of dead and legacy code; its scope matches drop_topics.md (F1)")

# --------------------------------------------------------------------------
# 41 — risks and open questions
# --------------------------------------------------------------------------
drop(F41, 1, 6, "DP-INDEX", "H1 title block: provenance header, evidence legend, sources merged")
drop(F41, 7, 23, "DP-INDEX", "contents list")
drop(F41, 24, 25, "DP-INDEX", "§1 heading only; its H3 children are routed separately")
drop(F41, 26, 34, "DP-METHOD", "§1.1 how the register was built")
drop(F41, 35, 66, "DP-METHOD", "§1.2 per-document statistics (322 source rows, 235 distinct entries)")
dec(F41, 67, 90, "Open confirm-DECs for the top 15 design-critical uncertainties",
    "§1.3: ranks 1 (11-tick second), 5 (approximate grid queries), 6 (per-team cap divisor and uncapped co-op maps), 7 (special teams in win checks), 8 (hostile crate ownership), 9 (rocket survival under siege), 10 (handicap), 12 (AI officer-creation stall), 14 (crowd-sound cull radius), 15 (tutorial 2 start condition) → Open DECs with the reference default; rank 4 (no performance measurement) → Open, decide by prototype; ranks 2–3 (build provenance, retail patch archives) are evidence-base questions with no Species consequence (DP-METHOD); ranks 11 and 13 → DD-NET (shared rows); line 89 lists confirmed integrity facts (Resolved DECs)")
defer(F41, 83, 83, "DD-NET", "rank 11 (shared): steady-state backlog and input latency; the tick rate is bounded by the host's frame rate", True)
defer(F41, 85, 85, "DD-NET", "rank 13 (shared): oversize update message stall; MTU and fragmentation requirement", True)
dec(F41, 91, 130, "resolved facts behind confirm-DECs (no Open DEC needed)",
    "§1.4 items resolved: lines 97–109 settled in this register (02-U5, 02-U6, 02-U7, 02-U12, 03-U14, 04-U2, 04-U3, 04-U10, 05-U1, 03-U7, 07-U6, 07-U11, the font-atlas identity), lines 115–127 closed by the owning documents, line 129 narrowed items, line 111 is method; X-DEC records them as Resolved or omits them; shared MERGE rows carry the in-scope facts to their reference targets")
R(F41, 97, 97, "MERGE", "reference/02_units/", "creatures and hazards: abductor craft wander-target rule", "G02b",
  "02-U5 (shared): wander targets are accepted only in the low-x, low-z quarter of the map; state it as the rule (the 02 open question is resolved)", True)
R(F41, 98, 98, "MERGE", "reference/02_units/", "player units: strike team weapon", "G02a",
  "02-U6 / 06-U8 (shared): strike teams always throw grenades in a match; no reachable weapon switch", True)
R(F41, 99, 99, "MERGE", "reference/02_units/", "player units: fixed research levels in a match", "G02a",
  "02-U7 / 03-U4 (shared): the match research table overwrites the lobby research option (fixed levels); G09 treats the lobby option as ineffective", True)
R(F41, 100, 100, "MERGE", "reference/02_units/", "creatures and hazards: abductor search loops", "G02b",
  "02-U12 (shared): the abductor search loops terminate on every reference map (worst-case acceptance 0.76 % and 6.3 %); they are bounded per F33", True)
R(F41, 101, 101, "MERGE", "reference/10_audio_design.md", "event model: building team-change events", "G10",
  "03-U14 (shared): the per-tick team-change event of empty solar panels is silent (the event is bound only for zones and relics)", True)
R(F41, 102, 104, "MERGE", "reference/04_supply_crates_powers_and_weapons.md", "crate drop placement and timing", "G04",
  "04-U2, 04-U3, 04-U10 (shared): weighted drops always accept their first candidate, so they may land in water or off the map; the height at the (0, 0) corner is negative on every map; crates drop during the grace period", True)
dec(F41, 131, 166, "DEC entries from the risk register R01–R29",
    "§2: R02–R04, R06 simulation-integrity risks → Resolved fixes; R12–R17 gameplay-fidelity risks → Open DECs (balance tuned under quirks, flat-terrain estimates, unknown intent, AI bugs, unknown performance envelope); R22–R25 legal and IP risks → consumed by the F1 ledger and X-TOOLS (no DEC); R26–R28 schedule → roadmap (design phase); shared rows split out R01, R05, R07–R11, R18–R21, R29")
defer(F41, 137, 137, "DD-NUMERIC", "R01 floating-point results differ across x64 CPUs, compilers and flags (shared)", True)
defer(F41, 141, 141, "DD-NET", "R05 desyncs undetected and unrecoverable (shared)", True)
defer(F41, 143, 145, "DD-NET", "R07 any client commands any team, R08 oversize datagrams and remote crashes, R09 no reconnection, host migration or practical late join (shared)", True)
defer(F41, 146, 146, "DD-SERVICES", "R10 clear-text password, broadcast host key, absent service back end (shared)", True)
defer(F41, 147, 147, "DD-NET", "R11 mixed builds or edited data play together and desync silently (content and version identity) (shared)", True)
adr(F41, 154, 157, "data schema format and validation; separate collision assets; localisation fallback and IME; content-build validators",
    "R18–R21 data and asset pipeline risks (shared)", True)
defer(F41, 165, 165, "DD-NET", "R29 latest-only coalescing of the continuous control packet (shared)", True)
drop(F41, 167, 170, "DP-METHOD", "§3 heading and register column notes")
drop(F41, 171, 208, "DP-METHOD", "§3.0 recovery-phase questions resolved during synthesis; every resolution lives in its owning document (Where column), which F2a and F2b route")
dec(F41, 209, 213, "Open DEC entries: 00 overview", "§3.1: merge pointers only; the entries live in §3.2, §3.6, §3.10 and §3.11")
dec(F41, 214, 230, "Open DEC entries: game modes (G01)",
    "§3.2: 01-U1 lobby time limit versus map play time (F8), 01-U2 11-tick second (D6), 01-U5 out-of-range roster read (F11), 01-U6 detonation flip (F19), 01-U7 rocket damageable in flight (F20), 01-U8 recount periods (Resolved), 01-U9 co-op loop bound (F13), 01-U10 special teams in win checks (F28), 01-U11 multi-round Assault (F5), 01-U12 sudden-death recovery")
dec(F41, 231, 245, "Open DEC entries: units (G02a, G02b)",
    "§3.3: 02-U1 byte speed stat (F39), 02-U3 wall-clock facing (Resolved, no live path), 02-U5, 02-U6, 02-U7, 02-U12 (Resolved in §1.4; MERGE rows), 02-U15 transport cannot unload over water, 02-U16 nuke arrival times")
dec(F41, 246, 265, "Open DEC entries: buildings and world objects (G03a, G03b)",
    "§3.4: 03-U1, 03-U2, 03-U5, 03-U6, 03-U7 Resolved; 03-U3 development maps reachable (DL-TESTMAPS → Dropped); 03-U8 global buildings inert (DL-CAMPAIGN → Dropped); 03-U9 invisible obstacles on routes; 03-U12 unrotated tree crown sphere; 03-U13 rocket damage per attack; 03-U14 (MERGE row); 03-U16 Assault solar-panel knock-out intent; 03-U17 global master spawn points")
dec(F41, 266, 282, "Open DEC entries: crates, powers and weapons (G04)",
    "§3.5: 04-U1, 04-U2, 04-U6, 04-U12 Resolved; 04-U3 corner edge cases, 04-U4 hostile outcome intent, 04-U5 randomiser team assignment, 04-U7 weapon timing on real terrain, 04-U10 crates during the grace period (MERGE row), 04-U11 effective weapon ranges from flat-terrain arithmetic")
dec(F41, 283, 299, "Open DEC entries: population flow (G05)",
    "§3.6: 05-U1 Resolved; 05-U2 spawn-network latency per map, 05-U3 fate of harvested spirits, 05-U5 handicap intent (F27), 05-U6 cap divisor intent (F25), 05-U7 doubled reinforcement rate (F3), 05-U9 pulling an operator off a port, 05-U10 hazard population counter reset, 05-U11 spawn-mania request count, 05-U12 harvester throughput estimate")
dec(F41, 300, 315, "Open DEC entries: controls and input (G06, C-INP)",
    "§3.7: 06-U1, 06-U2, 06-U3, 06-U6 legacy input-stack details (DL-LEGACY-TECH → Dropped); 06-U5 camera state read by simulation code (integrity); 06-U9 order with zero controlled units; 06-U10 stale orbit centre; 06-U11 documentation classification (DP-METHOD); 06-U12 closed")
dec(F41, 316, 332, "Open DEC entries: bots (G07)",
    "§3.8: 07-U1 officer-creation stall, 07-U2 stale task id fires a power at own units, 07-U5 fence-blind initial graph, 07-U6 Resolved, 07-U7 AI entity as a target, 07-U8 NaN priorities, 07-U9 retribution switch in the lobby, 07-U10 unbound defence markers, 07-U11 Resolved, 07-U12 AI slice ordering (F37 covers the bug fixes)")
dec(F41, 333, 351, "Open DEC entries: maps and level design (G08)",
    "§3.9: 08-U4 menu tie order, 08-U12 design intent of concentric hill zones, duplicate relic zones, pre-owned zones and light pairs (confirm); 08-U8, 08-U9, 08-U10 editor round-trip and container robustness (KH-EDITOR lessons, F55); 08-U7 thumbnail tool (X-AST context); 08-U1, 08-U3, 08-U5, 08-U6, 08-U11 packaging, demo and downloadable-map questions (DL-DEMO, DL-TESTMAPS → Dropped); 08-U13 identity buffer overflow (DL-LEGACY-TECH)")
dec(F41, 352, 370, "Open DEC entries: front end and match flow (G09)",
    "§3.10: 09-U1 achievement layer (KH-ACHIEVE, shared row); 09-U2 key file and 09-U17 demo predicates (DL-DEMO → Dropped); 09-U4 loose default preferences (F52); 09-U5, 09-U7 service listing fields (DL-SERVICES → Dropped); 09-U6 menu transition look; 09-U8 unused art; 09-U9 credits fade (DL-CUT); 09-U10 building detail branches (C-RND); 09-U13 manual server entries (DD-LOBBY); 09-U16 uninitialised loading flag")
hooks(F41, 356, 356, ["KH-ACHIEVE"], "09-U1 (shared): whether a retail achievement layer existed")
dec(F41, 371, 388, "Open DEC entries: audio design (G10)",
    "§3.11: 10-U1 intelligibility of voice samples (X-AST: new voice set), 10-U2 retail archive match, 10-U3 sample provenance (IP, F1), 10-U4 stationary loops never stopped (F48), 10-U8 victory jumps on maps without objectives, 10-U9 capture-tone name bug (moot), 10-U10 neutral team passing friend/foe filters, 10-U12 lobby music (KH-MUSIC, shared row), 10-U14 render-rate-dependent flashes, 10-U15 early menu-sound load, 10-U16 game-over pitch drop")
hooks(F41, 382, 382, ["KH-MUSIC"], "10-U12 (shared): whether lobby music was planned")
defer(F41, 389, 395, "DD-NET", "§3.12 (H3 heading and table header included): 20-U1 host frame rate and vsync bound the tick, 20-U2 host below 10 fps slows the session, 20-U3 pacing model assumptions")
dec(F41, 396, 397, "DEC (Resolved): match-state integrity", "§3.12: 20-U4 multi-round reset applied outside the tick (F5), 20-U6 game-timer residue across matches (F46)")
defer(F41, 398, 401, "DD-THREADS", "§3.12: 20-U8 listener running during shutdown, 20-U10 worker calling convention, 20-U11 OS audio threads touching game memory, 20-U12 file-system object never instantiated (DL-DEAD)")
dec(F41, 402, 402, "DEC (Resolved): per-match state reset", "§3.12: 20-U13 teleport map has no per-match clear (integrity, Resolved)")
defer(F41, 403, 406, "DD-THREADS", "§3.12: 20-U14 unlocked texture look-ups outside loading screens; merge pointers")
dec(F41, 407, 423, "Open DEC entries: simulation model (C-SIM)",
    "§3.13: 21-U1 tree generator race (F45, integrity), 21-U2 grid cache staleness (F43), 21-U3 edge building sphere, 21-U4 route ids differ between peers (lockstep-only, DD-NET lesson), 21-U6 and 21-U7 slot handles without generation check (stable ids), 21-U8 unbounded push loop (F33), 21-U9 thresholds tuned against approximate queries, 21-U10 pool pointer held across growth, 21-U11 dead airborne entities falling through terrain")
defer(F41, 424, 429, "DD-SERVICES", "§3.14 (H3 heading and table header included): 22-U1 service back-end listing fields, 22-U2 key algorithm (DL-DEMO)")
defer(F41, 430, 433, "DD-NET", "§3.14: 22-U3 oversize update message, 22-U4 pacing branch intent, 22-U5 measured backlog and latency, 22-U6 join in progress")
defer(F41, 434, 434, "DD-NUMERIC", "§3.14: 22-U7 CRT transcendental equality across CPUs")
defer(F41, 435, 435, "DD-NET", "§3.14: 22-U8 frequency of the squad free-look desync L1; the fix itself is F29 (Resolved)")
defer(F41, 436, 440, "DD-NUMERIC", "§3.14: 22-U10 compiler defaults, 22-U11 literal parsing accuracy, 22-U12 cross-CRT pow, exp and log for terrain (also DD-TERRAIN-RUNTIME), 22-U13 cross-platform peers, 22-U15 secondary build in multiplayer")
dec(F41, 441, 441, "DEC (Resolved): per-match state reset", "§3.14: 22-U17 team-slot-5 spawn counter survives between matches (integrity, Resolved)")
defer(F41, 442, 448, "DD-NET", "§3.14: 22-U18 rejoin into a full match, 22-U19 host quit, 22-U20 replays without a stored seed (KH-REPLAY lesson), 22-U21 command-stream size and late join; merge pointers")
defer(F41, 449, 453, "DD-TERRAIN-RUNTIME", "§3.15 (H3 heading and table header included): 23-U1 bit-exactness of the terrain port; it matters only for regenerating reference maps, and Species maps are new")
dec(F41, 454, 455, "Open DEC entries: terrain (C-TER)", "§3.15: 23-U3 load-time generator draws interleaving with the colour chain (shared generator, integrity); 23-U4 legacy float-to-integer conversion (Dropped)")
defer(F41, 456, 456, "DD-NUMERIC", "§3.15: 23-I1 extended-precision evaluation equals IEEE double without FMA")
dec(F41, 457, 462, "Open confirm-DECs: terrain generator inferences (C-TER)", "§3.15: 23-I3 roughness exponent and gain normalisation, 23-I4 method-2 directional bias and smooth-flatten divisor, 23-I5 world-edge wrap artefacts only under water; merge pointers")
dec(F41, 463, 482, "Open DEC entries: renderer (C-RND)",
    "§3.16: 24-U1–24-U5, 24-U12, 24-U13 secondary-build questions (DL-D3D9 → Dropped); 24-U9 non-Windows cursor (DL-PLATFORM); 24-U11 atlas range (DL-LEGACY-TECH); 24-U8 texture flags by first requester (F50, texture-pipeline ADR); 24-U10 flat-shading provoking vertex (B10, visual language); 24-U14 one-frame light lag; 24-U15 grid-outline texture intent (visual-language confirm)")
dec(F41, 483, 495, "Open DEC entries: textures and assets (X-AST, C-AST)",
    "§3.17: 25-U1 retail patch archives (evidence base, DP-METHOD); 25-U2 per-SKU language lists (DL-LEGACY-TECH); 25-U3 run-time instantiated assets (TO-ASSETS, shared row); 25-U4 driver clamp semantics (DL-LEGACY-TECH); 25-U5 shape hit-test overrun (separate collision assets, R20); 25-U6 processing conflict (F50)")
R(F41, 489, 489, "TO-ASSETS", "assets/asset_manifest.csv", "reach of assets whose consumers have no placement in live maps", "X-AST",
  "25-U3 (shared): which of the 51 full-build assets without a placement are instantiated at run time (crate or mode spawning); decides their priority in the manifest", True)
dec(F41, 496, 512, "Open DEC entries: animation and VFX (C-VFX)",
    "§3.18: 26-U1 what each panel animation depicts (X-AST context), 26-U2 seasonal mode per peer (F53), 26-U5 foot positions read by the simulation (integrity confirm), 26-U6 secondary-build look (DL-D3D9), 26-U7 render prediction time, 26-U8 saturated team colours intent (visual-language confirm), 26-U9 DSP enabled (Dropped), 26-I1 tree hit volume from render data (F45), 26-I2 synced generator consumed for cosmetic particles and 26-I3 unsynchronised generators in simulation code (integrity, Resolved)")
dec(F41, 513, 531, "Open DEC entries: audio engine (C-AEN)",
    "§3.19: 27-U1, 27-U3, 27-U4, 27-U6, 27-U7, 27-U9, 27-U11, 27-U12, 27-U13 legacy driver and mixer details (DL-LEGACY-TECH → Dropped); 27-U2 Resolved (music unreachable); 27-U5 attached sounds hard-cut, 27-U8 nearest-emitter cull radius, 27-U10 smoothed parameters start value, 27-U14 loop keep-alive polling → behaviour confirm-DECs (F48)")
dec(F41, 532, 545, "Open DEC entries: UI framework (C-UI)",
    "§3.20: 28-U1, 28-U2 text and transition look (confirm); 28-U3 partial languages (F51); 28-U4 IME input (localisation ADR); 28-U5 lobby authority (DD-LOBBY); 28-U6, 28-U7, 28-U8 legacy toolkit details (Dropped); 28-U9 demo-key hosting (DL-DEMO)")
dec(F41, 546, 561, "Open DEC entries: data schemas (C-DAT)",
    "§3.21: 29-U4 data-only marker name look-up, 29-U13 marker sign convention (asset import confirm); 29-U5, 29-U6, 29-U7, 29-U8, 29-U10, 29-U11 legacy formats and tools (DL-LEGACY-TECH → Dropped); 29-U14 map identity (DD-NET, shared row)")
defer(F41, 558, 558, "DD-NET", "29-U14 (shared): a locally modified map desyncs instead of being rejected because no map identity is exchanged", True)
dec(F41, 562, 572, "Open DEC entries: tutorial and help (G11)",
    "§3.22: 30-U1, 30-U2 tutorial 2 start conditions (confirm; tutorials are redesigned as step graphs), 30-U5 local player hard-coded to team 0 (F56), 30-U6 Resolved")
dec(F41, 573, 587, "Open DEC entries: platform, build and tools (C-PLT, C-BLD)",
    "§3.23: 31-U6, 31-U9, 31-U10, 31-U12 absent or legacy tools (DL-LEGACY-TECH → Dropped); 31-U11 Resolved; 31-U13 clock monotonic across core affinity sets (platform ADR); 31-U14 build provenance (evidence base, DP-METHOD); 31-I3 worker-thread crashes bypass the crash reporter (crash-reporting ADR)")
dec(F41, 588, 603, "Open DEC entries: diagnostics, test and performance (C-DBG)",
    "§3.24: 32-U2 per-system cost unmeasured (Open, decide by prototype); 32-U1, 32-U4, 32-U5, 32-U6 legacy CRT and tooling details (Dropped); 32-U3, 32-U7 dead builds (DL-DEAD); 32-U8 beta note; 32-I1 cheat network safety (DD-NET, shared row); 32-I2 catch-up spiral (DD-THREADS, shared row); 32-I3 research cheat no-op")
defer(F41, 600, 600, "DD-NET", "32-I1 (shared): network-safety classification of the cheats (arbitrary placement desyncs)", True)
defer(F41, 601, 601, "DD-THREADS", "32-I2 (shared): catch-up spiral of a slow client and the zero-slice dead band", True)
drop(F41, 604, 849, "DP-CLOSING", "§4 docs-vs-code discrepancy register")
drop(F41, 850, 855, "DP-CLOSING", "§5 IP replacement register intro; consumed by the F1 ledger")
drop(F41, 856, 910, "DP-CLOSING", "§5.1 replacement categories IP01–IP50; consumed by the F1 ledger")
R(F41, 907, 907, "TO-ASSETS", "assets/art_and_audio_briefs.md", "distinctive design expressions that need a new presentation", "X-AST",
  "IP48 (shared): the mechanic may stay but its presentation is new (submarine nuke and ground symbol, abduction event, seasonal styling, gesture symbol designs, intro homages, menu backdrop); see also §5.3 rule 5", True)
drop(F41, 911, 974, "DP-CLOSING", "§5.2 map and level names; consumed by the F1 ledger")
R(F41, 911, 974, "TO-ASSETS", "assets/asset_manifest.csv", "map thumbnails, one per map: counts only", "X-AST",
  "§5.2 map counts only (shared with the DP-CLOSING row): 57 level files, 53 with a menu entry; per mode and player count from the file-name pattern; never the display names; X4_levels.csv stays the primary source", True)
drop(F41, 975, 982, "DP-CLOSING", "§5.3 rules for the new game; consumed by the F1 ledger (already stated in prompt §0 and §6.2 and vision §5)")
drop(F41, 983, 986, "DP-CLOSING", "open questions and unknowns: none beyond §3")
drop(F41, 987, 990, "DP-CLOSING", "docs-vs-code discrepancies")
drop(F41, 991, 998, "DP-CLOSING", "IP items; consumed by the F1 ledger")
drop(F41, 999, 1038, "DP-CLOSING", "source map")
drop(F41, 1039, 1042, "DP-CLOSING", "appendix of dead and legacy code")

# --------------------------------------------------------------------------
# 90 — glossary
# --------------------------------------------------------------------------
drop(F90, 1, 6, "DP-INDEX", "H1 title block: provenance header, evidence legend, sources merged")
drop(F90, 7, 31, "DP-INDEX", "contents list")
drop(F90, 32, 95, "DP-METHOD", "§1 conventions (scope, entry columns, citations, evidence and build tags, IP column, how the list was built); X-GLO may read lines 42–63 (entry columns and kinds) as read-only context")
R(F90, 96, 475, "KEEP", GLO, "Species terms A–L", "X-GLO",
  "§2 A–Z part 1 of 2 (H2 intro and letters A–L): in-scope terms only, renamed through the ledger, each defined and linked to its owning Species document; drop terms whose definition is tagged LEGACY, DEAD, DEBUG, D3D or EDITOR without a 40 keep verdict (exclusion list: lines 1102–1111), evidence and build tags, and pure code identifiers")
R(F90, 476, 844, "KEEP", GLO, "Species terms M–Z", "X-GLO",
  "§2 A–Z part 2 of 2 (letters M–Z): same rules as part 1", anchor="m")
drop(F90, 845, 1042, "DP-CLOSING", "§3 canonical placeholder names; consumed by the F1 ledger")
drop(F90, 1043, 1046, "DP-CLOSING", "open questions and unknowns: none; the two collision resolutions it mentions are ledger matters (F1)")
drop(F90, 1047, 1050, "DP-CLOSING", "docs-vs-code discrepancies")
drop(F90, 1051, 1073, "DP-CLOSING", "IP items; consumed by the F1 ledger")
drop(F90, 1074, 1101, "DP-CLOSING", "source map")
drop(F90, 1102, 1111, "DP-CLOSING", "appendix of build-restricted vocabulary by tag; X-GLO should use it as its exclusion list (report: cross-document requests)")


FIELDS = ["orig_file", "anchor", "start_line", "end_line", "bytes", "action",
          "target_file", "target_section", "owner_package", "note"]
ACTIONS = {"KEEP", "MERGE", "DROP-LEGACY", "DROP-PROVENANCE", "DEFER-DESIGN",
           "TO-DEC", "TO-ADR", "TO-ASSETS", "KEEP-HOOKS"}
CODE_RE = re.compile(r"^(DL|DD|DP|KH)-[A-Z-]+")


def load_headings(path):
    heads = defaultdict(list)
    with open(path, encoding="utf-8", newline="") as f:
        for row in csv.DictReader(f):
            heads[row["file"]].append(
                (int(row["level"]), row["anchor"], int(row["start_line"]), int(row["end_line"])))
    return heads


def anchor_for(heads, f, s, e):
    best = None
    for level, anchor, hs, he in heads[f]:
        if hs <= s and e <= he and (best is None or level > best[0]):
            best = (level, anchor)
    return best[1]


def split_rows(rows):
    """Cut every row at the boundaries of the rows it contains, so that any two
    rows either have identical ranges or do not overlap (gate G1 rule)."""
    bounds = set()
    for r in rows:
        bounds.add(r[1])
        bounds.add(r[2] + 1)
    out = []
    for r in rows:
        f, s, e, action, tfile, tsec, owner, note, shared, anchor = r
        cuts = sorted(b for b in bounds if s < b <= e)
        starts = [s] + cuts
        ends = [c - 1 for c in cuts] + [e]
        n = len(starts)
        if n == 1:
            out.append(r)
            continue
        m = CODE_RE.match(note)
        lead = (m.group(0) + " — ") if m and (action.startswith(("DROP", "DEFER")) or action == "KEEP-HOOKS") else ""
        for k, (ps, pe) in enumerate(zip(starts, ends), start=1):
            if k == 1:
                pnote = f"{note} [part 1/{n} of lines {s}–{e}]"
            else:
                pnote = f"{lead}part {k}/{n} of lines {s}–{e}; full note on part 1"
            out.append((f, ps, pe, action, tfile, tsec, owner, pnote, shared, anchor))
    return out


def main():
    orig, headings_csv, out = sys.argv[1], sys.argv[2], sys.argv[3]
    heads = load_headings(headings_csv)
    errors = []
    stats_rows = []
    with open(out, "w", encoding="utf-8", newline="") as fo:
        w = csv.writer(fo, lineterminator="\n")
        w.writerow(FIELDS)
        for f in (F40, F41, F90):
            with open(os.path.join(orig, f), encoding="utf-8") as fi:
                lines = fi.read().split("\n")
            h1 = [h for h in heads[f] if h[0] == 1][0]
            h1_start, h1_end = h1[2], h1[3]
            if h1_end != len(lines):
                errors.append(f"{f}: H1 end {h1_end} != {len(lines)} split lines")
            rows = split_rows([r for r in ROWS if r[0] == f])
            rows.sort(key=lambda r: (r[1], r[8], -r[2]))
            spans = sorted({(r[1], r[2]) for r in rows})
            cursor = h1_start
            for s, e in spans:
                if s != cursor:
                    errors.append(f"{f}: span {s}-{e} does not start at {cursor} (gap or partial overlap)")
                cursor = max(cursor, e + 1)
            if cursor != h1_end + 1:
                errors.append(f"{f}: spans end at {cursor - 1}, H1 ends at {h1_end}")
            cover = defaultdict(list)
            for r in rows:
                _, s, e, action, tfile, tsec, owner, note, shared, anchor = r
                if action not in ACTIONS:
                    errors.append(f"{f}:{s}-{e}: bad action {action}")
                if action.startswith(("DROP", "DEFER")) or action == "KEEP-HOOKS":
                    if not CODE_RE.match(note):
                        errors.append(f"{f}:{s}-{e}: note lacks reason code")
                if action.startswith("DROP") and (owner or tfile):
                    errors.append(f"{f}:{s}-{e}: DROP row with owner or target")
                if not (h1_start <= s <= e <= h1_end):
                    errors.append(f"{f}:{s}-{e}: out of range")
                size = sum(len(l.encode("utf-8")) + 1 for l in lines[s - 1:e])
                anc = anchor or anchor_for(heads, f, s, e)
                w.writerow([f, anc, s, e, size, action, tfile, tsec, owner, note])
                stats_rows.append((f, s, e, size, action, owner, note, shared))
                for n in range(s, e + 1):
                    cover[n].append(shared)
            fo.flush()
            missing = [n for n in range(h1_start, h1_end + 1) if n not in cover]
            if missing:
                errors.append(f"{f}: uncovered lines {missing[:20]} (+{max(0, len(missing) - 20)})")
            clash = [n for n, v in cover.items() if v.count(False) > 1]
            if clash:
                errors.append(f"{f}: lines with >1 non-shared row: {sorted(clash)[:20]}")
            nonshared_bytes = sum(r[3] for r in stats_rows if r[0] == f and not r[7])
            total = sum(len(l.encode("utf-8")) + 1 for l in lines[h1_start - 1:h1_end])
            print(f"{f}: rows={len(rows)} lines={h1_end} primary_bytes={nonshared_bytes} h1_bytes={total}")
    # statistics
    per_action = defaultdict(lambda: [0, 0, 0])
    per_owner = defaultdict(lambda: [0, 0, 0])
    for f, s, e, size, action, owner, note, shared in stats_rows:
        for d, k in ((per_action, action), (per_owner, owner or "(none)")):
            d[k][0] += 1
            d[k][1] += size
            if not shared:
                d[k][2] += size
    print("\naction rows bytes primary_bytes")
    for k in sorted(per_action):
        print(k, *per_action[k])
    print("\nowner rows bytes primary_bytes")
    for k in sorted(per_owner):
        print(k, *per_owner[k])
    print("\nDEFER-DESIGN topics")
    topics = defaultdict(list)
    for f, s, e, size, action, owner, note, shared in stats_rows:
        if action == "DEFER-DESIGN":
            codes = re.findall(r"DD-[A-Z]+(?:-[A-Z]+)*", note)
            for c in dict.fromkeys(codes):
                if c in DD_TOPIC:
                    topics[c].append(f"{f[:2]}:{s}-{e}" + ("" if c == codes[0] else "*"))
    for c in DD_TOPIC:
        print(c, len(topics[c]), " ".join(topics[c]))
    print("\nKEEP-HOOKS features")
    feats = defaultdict(list)
    for f, s, e, size, action, owner, note, shared in stats_rows:
        if action == "KEEP-HOOKS":
            for c in re.findall(r"KH-[A-Z]+", note.split(" — ")[0]):
                feats[c].append(f"{f[:2]}:{s}-{e}")
    for c in KH_FEATURE:
        print(c, " ".join(feats[c]))
    print("\nERRORS" if errors else "\nOK: no errors")
    for e in errors:
        print(" ", e)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
