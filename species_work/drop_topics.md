# Drop topics and reason classes (coordinator draft, Phase 0)

F2 routing agents use these reason-class codes from the start. F1 refines the **descriptions and examples** from `ORIG/40` §2–§3 and `ORIG/00` §2.5, and may **add** classes. F1 never renames or removes a class, because the routing CSVs already use them.

Sources: prompt §0 "Superseded verdicts", §2 table, build-tag rule, `ORIG/40` §1.2 D10.

## Action `DROP-LEGACY`: reason classes

| Code | What | Examples (in-corpus terms) |
|---|---|---|
| `DL-CAMPAIGN` | Single-player campaign content compiled in but unreachable in a match (`[B:LEGACY]`), blocked old objects, campaign buildings and saves | campaign levels `map_*` and `mission_*`, `BLOCK_OLD_DARWINIA_OBJECTS` objects, Pattern Buffer, Control Tower, Clone Lab, research items, campaign save obfuscation |
| `DL-DEAD` | Not compiled in any PC configuration (`[B:DEAD]`) | attract mode, benchmark, loaders, AVI grabber, FTP benchmark upload |
| `DL-D3D9` | The Direct3D 9 build and its OpenGL-emulation layer (`[B:D3D]`), including its shaders and configuration | `opengl_directx*`, FixedFunc shaders, `ReleaseDirect3D` |
| `DL-DEMO` | Demo and full-game gating, authentication keys, demo limits (`[B:DEMO]`) | `IS_DEMO`, `authkey`, demo map lists |
| `DL-DEBUG` | Debug-only content (`[B:DEBUG]`) **without** an `ORIG/40` keep verdict | Tank Battle mode and its maps, cheat menus not kept by 40 |
| `DL-LEGACY-TECH` | The original's implementation of an in-scope behaviour in superseded technology. The **behaviour** is KEEP and the mechanism is dropped. | GL immediate mode, display lists, GL state tables, WGL, DirectSound, waveOut and the software mixer, x87 and `/fp` flags, VS2005/2008 builds, RAR archives, BMP decoding specifics, bitmap font atlases, positional text-table grammars, Inno Setup/NSIS, XCrashReport, Win32 window-manager specifics, XInput 1.3 |
| `DL-PLATFORM` | Non-Windows ports and their code paths | Linux, macOS, SDL paths |
| `DL-SERVICES` | The original's online services and their client code | metaserver, matchmaker, NAT punch-through, WAN registration, authentication-key validation |
| `DL-CUT` | Features `ORIG/40` D10 drops without a redesign | the cut-scene script interpreter as a gameplay system, the narrator, the loader mini-programs, the gesture recogniser, the start sequence, the credits loader |
| `DL-TESTMAPS` | Hidden, test, stress, benchmark, sandbox and cut maps | `mp_*_stress`, `mp_benchmark_1`, `mp_sandbox*`, `mp_tankbattle_*`, cut maps |

## Action `DEFER-DESIGN`: topics the design phase decides

The content is dropped. Its key constraints and lessons go into one DEC per topic (status Open, "Decide by: design phase").

| Code | Topic | What to capture in the DEC |
|---|---|---|
| `DD-NET` | Networking model: lockstep transport, sync checks, wire encoding, input delay, desync handling | the failure modes the networking model must prevent; bandwidth and population figures |
| `DD-LOBBY` | Lobby, discovery and session protocol | lobby authority rules, what the match setup needs |
| `DD-NUMERIC` | Simulation numeric model (floating-point environment, transcendental rounding, fixed point) | determinism hazards a new numeric model must avoid |
| `DD-THREADS` | Simulation threading and process architecture | integrity rules that the threading model must respect |
| `DD-SERVICES` | Sessions, identity and platform services | what entitlement, identity and discovery must provide |
| `DD-TERRAIN-RUNTIME` | Terrain runtime model (generated at load or baked) | the generator's determinism constraints and content-identity needs. The **algorithm** is KEEP. |
| `DD-PERSIST` | Saves and persistence formats | none beyond the list of what is saved |

## Action `KEEP-HOOKS`: features 40 D10 says must be designed fresh

Record what the corpus describes of their hooks and 40's verdict. Do not design them.

| Code | Feature |
|---|---|
| `KH-EDITOR` | map editor |
| `KH-REPLAY` | replays |
| `KH-ACHIEVE` | achievements and leaderboards |
| `KH-RESULTS` | post-match results screen |
| `KH-MUSIC` | music |

## Action `DROP-PROVENANCE`: reason classes

| Code | What |
|---|---|
| `DP-CLOSING` | Open questions and unknowns, docs-vs-code discrepancies, IP items, source map, appendix of dead/legacy code (each document's closing sections). Open questions and unknowns are **also** routed `TO-DEC` when they concern in-scope content. |
| `DP-METHOD` | method, statistics, "how this was built or checked", review and coverage material |
| `DP-INDEX` | contents lists, cross-reference indexes, the coverage matrix |
