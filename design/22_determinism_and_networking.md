# 22 — Determinism and networking

> Part B · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: P15_network (network model, protocol, sessions, metaserver), X3_determinism_sync (RNG, floating point, ordering, sync checking, leaks, risk register), P13_gfx_backend_math (numeric environment, math library), P14_platform_lib (RNG algorithms, hashing helpers), P01_app_core, P02_game_modes, P03_entity_core, P04_entities_military, P05_entities_creatures, P06_buildings_core, P07_buildings_misc, P08_crates_ai, P09_control_taskmanager, P10_input_camera, P11_terrain_world, P16_sound (determinism notes), routed `_meta_*` notes. Generated tables: `_work/scripts/S_22_rng_tables.py` over `_work/drafts/X3_syncrand_sites.csv`, `X3_unsynced_rng_sites.csv`, `X3_local_state_refs.csv`.

This document specifies how Multiwinia keeps several machines running one game: the relay-lockstep network model, the complete wire protocol, session lifecycle, sync checking, and the determinism discipline (random numbers, floating point, ordering, state classification) on which the model depends. The lobby **user interface** and server-browser screens are specified in [09 §5](09_meta_systems.md#5-multiplayer-front-end-versus-cpu-quick-match-lanwan-browser-hostjoin-lobby); this document specifies the **protocol** underneath them. Entity/container internals are in [21](21_simulation_and_entity_model.md); the main loop and threads in [20 §9–§11](20_architecture.md#9-main-loops).

## Contents

1. [Model overview](#1-model-overview)
2. [Timing and pacing](#2-timing-and-pacing)
3. [Transport — NetLib over UDP](#3-transport--netlib-over-udp)
4. [Wire encoding](#4-wire-encoding)
5. [Message catalogue](#5-message-catalogue)
6. [Server sequencing and client ordering](#6-server-sequencing-and-client-ordering)
7. [IFrames](#7-iframes)
8. [Session lifecycle](#8-session-lifecycle)
9. [Sync checking and desync handling](#9-sync-checking-and-desync-handling)
10. [Virtual (CPU) players in the network model](#10-virtual-cpu-players-in-the-network-model)
11. [Bandwidth, network quality, file transfer, demo limits, replay](#11-bandwidth-network-quality-file-transfer-demo-limits-replay)
12. [Lobby and server discovery protocol](#12-lobby-and-server-discovery-protocol)
13. [Threads and locking](#13-threads-and-locking)
14. [Determinism model](#14-determinism-model)
15. [Risk register for the new game](#15-risk-register-for-the-new-game)
16. [Constants](#16-constants)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

---

## 1 Model overview

### 1.1 At a glance

| Question | Answer | Evidence |
|---|---|---|
| Topology | **Star, client/server over UDP.** One process hosts a `Server`; every participant (including the host) runs a `ClientToServer`. Clients never talk to each other. | [C] `code/app.cpp:App::StartNetwork`, `code/network/server.cpp:Server::Initialise`, `code/network/clienttoserver.cpp:ClientToServer::ClientJoin` |
| Is the host also a client? | **Yes.** `App::StartNetwork(true,…)` creates `m_server` and then calls `m_clientToServer->ClientJoin()` in the same process. Host-client ↔ server traffic bypasses sockets: the host client hands letters straight to `Server::ReceiveLetter` (tagged with the host's LAN IP and client listen port), and the server hands letters whose `m_receiverId == m_clientToServer->m_clientId` straight to `ClientToServer::ReceiveLetter`. The host client nevertheless goes through exactly the same sequencing/ack pipeline as remote clients. | [C] `code/network/clienttoserver.cpp:ClientToServer::AdvanceSender`, `code/network/server.cpp:Server::AdvanceSender` |
| Dedicated server? | **No.** There is no headless mode; the server is always embedded in a game client. | [C] the only construction site of `Server` is `App::StartNetwork` (`code/app.cpp`) |
| Single player | Uses the identical pipeline: `App::StartSinglePlayerServer` → `StartNetwork(true, NULL, -1)` → server IP `"127.0.0.1"`, server port **−1**. `ClientToServer::GetServerPort() == -1` is the game-wide "single player" flag (no advertising, other joiners rejected, no kicking on authentication failure). | [C] `code/app.cpp:App::StartSinglePlayerServer`, `App::StartNetwork`; `code/network/server.cpp:Server::RegisterNewTeam`, `Server::Advertise`, `Server::AuthenticateClients` |
| Multiplayer host | `App::StartMultiPlayerServer` → `StartNetwork(true, NULL, NULL)`; port `NULL` (= 0) ≠ −1, so the host client joins its own LAN IP (`GetLocalHostIP`) at the server listener's port. | [C] `code/app.cpp:App::StartMultiPlayerServer` |
| Lockstep vs state sync | **Deterministic lockstep over a server-sequenced command stream ("relay lockstep").** The server never simulates authoritatively and never sends world state. Every 0.1 s of real time it packages all client commands received since the previous tick into one *UPDATE letter* with a monotonically increasing sequence id, appends it to an unbounded history and sends it to every client. Each client executes the letters strictly in sequence-id order; one UPDATE letter = one simulation *frame* of `0.1 × gameSpeed` game seconds, executed as 10 *slices*. Only a 1-byte checksum per frame flows back for desync detection. | [C] `code/network/server.cpp:Server::Advance`, `Server::SendLetter`; `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`, `ClientToServer::GetNextLetter` |
| Input delay | Not a fixed number of frames. A command takes effect at the start of the first UPDATE frame generated after the server receives it: delay = wait for the next server tick (0–100 ms) + one-way latency to the server + one-way latency back + client-side slice backlog (§2.4). The host experiences zero network latency but the same tick quantisation and buffering. | [C] pipeline; [I] composition of delays |
| Lobby | Runs **inside the same lockstep channel**: game options, team assignments, names, colours, chat, ready flags and START_GAME are all sequenced commands in the history. A late joiner reconstructs lobby state by replaying the full history from sequence id 0. | [C] `ClientToServer::ProcessServerUpdates`; `code/UI/WaitingButton.h` (connect progress = `g_lastProcessedSequenceId / m_serverSequenceId`) |
| Transport reliability | Custom over UDP: sequence ids + cumulative acks (every client letter carries `LASTSEQID` = highest contiguous id received) + a server-side resend window of 3 letters per client per tick + one piggy-backed copy of the oldest unacknowledged letter + run-length encoding of empty frames. Client→server messages are **not** acknowledged or resent by the transport; where it matters they are repeated at application level (ALIVE 10 Hz, team requests, joins, sync bytes). | [C] `code/network/server.cpp:Server::UpdateClients`; `ClientToServer::ReceiveLetter`, `ClientToServer::Advance` |
| What must be bit-identical on every peer | (1) the ordered command stream; (2) the synced RNG state (MT19937, one seed per server session); (3) every simulated field touched by `Location::Advance` and `Multiwinia::Advance` (classification §14.10); (4) static data — level file, `data/stats.txt`, shapes used for hit tests, terrain seeds; (5) per-process statics that simulation code reads (§14.12). | [C]/[I] |
| Numeric environment relied on | Win32 **x87** code (no `/arch:SSE2`), x87 precision control forced to a **53-bit significand** per thread, `/fp:precise` in the shipped configuration, and every transcendental and `sqrt` result rounded to **IEEE binary32** by `iv_trunc_low_nibble`. | §14.8 |
| Ordering relied on | Strictly serial, single-threaded, index-ordered iteration of slot arrays (`FastDArray`/`SliceDArray` with LIFO free lists) under a fixed slice schedule; no hash-map or pointer ordering anywhere in the simulation. | §14.9 |
| How divergence is detected | One byte per frame sent to the server. **Baseline build: the byte is the next synced-RNG draw** (`255 × syncfrand(1)`), so only divergence of the RNG stream position is detected; debug/beta builds hash a world snapshot (SHA-1). | [C] `code/network/iframe.cpp:IFrame::Generate`; §9 |
| What happens on desync | The server broadcasts NETSYNCERROR (sequenced); every client pauses the simulation permanently (`App::GamePaused`), shows a red banner and writes `syncerror-*.txt`. No resync, no recovery. | [C] §9 |

### 1.2 Peer roles

| Role | Object | Count | Responsibilities |
|---|---|---|---|
| Server | `Server` (`g_app->m_server`) | 0 or 1 per process | Accept/authenticate clients, assign client ids and team slots, sequence and broadcast commands, keep the full history, resend lost letters, compare sync bytes, relay IFrame files, advertise to the metaserver/LAN, kick |
| Client | `ClientToServer` (`g_app->m_clientToServer`) | exactly 1 per process (created at application start-up) | Connect/handshake, send commands, receive/order letters, execute simulation slices, generate sync bytes, display desync/lag |
| Per-client server record | `ServerToClient` | one per connected (and per disconnected) client | IP/port, socket session, auth key / key id, password, client id, spectator flag, last known/sent sequence id, caught-up flag, last-message time, sync byte array `m_sync`, saved last ALIVE / SELECTUNIT / REQUEST_COLOUR, FTP manager, `m_syncErrorSeqId`, `m_disconnected` reason, `m_basicAuthCheck` |
| Team slot (server side) | `ServerTeam {m_clientId, m_teamType}` in `Server::m_teams` (DArray, index = team id 0..5) | ≤ `NUM_TEAMS` = 6 | Which client owns which team; a client may own several (the host owns its own team plus every CPU team it requested) |

[C] `code/network/server.h`, `code/network/servertoclient.h`, `code/globals.h:NUM_TEAMS`. This two-level client/team split is exactly what `docs/integrating virtual players into network system.txt` proposed ("DArray of Teams … DArray of Clients") [C].

### 1.3 Why the model works (and its preconditions)

The design relies on four properties, each of which the code maintains only by discipline, not by construction [I from the whole analysis]:

1. **Total order of inputs.** All player intent reaches the simulation only as relayed commands inside UPDATE letters, executed in sequence order on every client (§6). Local input is converted into world-space command data before sending (`TeamControls`, positions as raw doubles), so no client re-interprets another's input with local state (with one confirmed exception, §14.11 L1).
2. **Identical initial state.** Every client loads the same level by file name and seeds the synced RNG with the server's seed before replaying the history (§8.1, §14.3). Nothing verifies that the level files or game data are identical (§14.13).
3. **Bit-identical arithmetic.** One executable, one compiler configuration, the same x87 precision on every simulation thread, and transcendental results rounded to binary32 (§14.8).
4. **Strict separation of randomness.** Gameplay draws only from the synced generator; cosmetics from separate generators whose call counts may differ per peer (§14.2–§14.6).

The model's costs: input latency of at least one server tick plus a round trip; a single slow client slows nobody else but cannot catch up faster than 20 frames/s; late joiners must re-simulate the entire session; and any determinism bug is unrecoverable (§9.4).

---

## 2 Timing and pacing

### 2.1 Server tick

- `Server::Initialise`: `m_startTime = GetHighResTime()`, `m_nextServerAdvanceTime = m_startTime + 0.1` [C] `code/network/server.cpp:Server::Initialise`.
- `Server::AdvanceIfNecessary(now)`: if `now > m_nextServerAdvanceTime` → `Advance()` once, then `m_nextServerAdvanceTime += 0.1f` (a **float** literal added to a double: each step is 0.100000001490116 s) [C]. At most **one** tick per call; if the main loop stalls, ticks are produced back-to-back on subsequent calls until the schedule catches up (no tick skipping). `Server::SkipToTime` exists to jump the schedule ("waking from sleep") but has no caller [C] `Server::AdvanceIfNecessary`, `Server::SkipToTime`. Under `TESTBED_ENABLED` [B:DEBUG] the same `+= 0.1f` is applied in every branch.
- Callers: every main loop — `code/main.cpp:LocationGameLoop`, `GlobalWorldGameLoop` and `MainMenuLoop` (front end/lobby) — and the loading-screen loop under `m_networkMutex.TryLock()` (`code/loading_screen.cpp:LoadingScreen::Render`). The server therefore ticks at **10 Hz real time** regardless of game speed or pause [C].

### 2.2 Client frame and slices

- One UPDATE letter ⇒ one *frame*. When the client takes a letter (only while `g_sliceNum == -1`) it applies its commands (`ProcessServerUpdates`), sets `g_lastProcessedSequenceId = seqId`, `g_sliceNum = 0`, calls `g_gameTimer.Tick()`, then `GenerateSyncValue()` and `SendSyncronisation()`. Each subsequent slice step calls `Location::Advance(g_sliceNum)` and `ParticleSystem::Advance(g_sliceNum)` (both skipped while `App::GamePaused()`), and **at slice 8** `Multiwinia::Advance()` (always, even when paused, if `App::Multiplayer()`); after slice 9 `g_sliceNum = -1` [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`. `NUM_SLICES_PER_FRAME = 10` (`code/globals.h`).
- Non-UPDATE sequenced letters (CLIENTHELLO, CLIENTGOODBYE, TEAMASSIGN, NETSYNCERROR, NETSYNCFIXED, SPECTATORASSIGN) consume a sequence id but do **not** start a frame, tick the timer or produce a sync byte. Unsequenced letters (`SEQID == -1`) are processed as soon as they reach the inbox head [C] same function.
- `App::Multiplayer()` is `m_gameMode == GameModeMultiwinia`, i.e. true in every Multiwinia game including versus-CPU and tutorials [C] `code/app.cpp:App::Multiplayer`.
- `GameTimer::Tick` stamps `m_lastClientAdvance = GetHighResTime()`, adds `GetServerAdvancePeriod() = 0.1 × m_gameSpeed` to `m_gameTime` unless `App::GamePaused()`, and moves `m_gameSpeed` toward a requested speed by `diff × speedOfChange` per tick (snap when within 0.01, using `iv_abs`) [C] `code/gametimer.cpp:GameTimer::Tick`. `GameTimer::Reset` sets speed **0** in multiplayer (the match starts frozen until the grace countdown finishes), 1 otherwise; `m_gameTime = 0` [C] `GameTimer::Reset`. `GetNetworkTime` is a macro for `g_gameTimer.GetGameTime` and `SERVER_ADVANCE_PERIOD` for `g_gameTimer.GetServerAdvancePeriod()` — one synced clock (Resolved: P06 asked whether `GetNetworkTime()` and `GetGameTime()` were different clocks; `code/gametimer.h` shows they are the same) [C].

### 2.3 Slices per render frame (client pacing controller)

`ClientToServer::GetNumSlicesToAdvance(catchUp)` [C]:

```
if catchUp: return 20
pending = (m_lastValidSequenceIdFromServer - g_lastProcessedSequenceId) * 10
if g_sliceNum != -1: pending += 10 - g_sliceNum
n = int(pending * 0.3)
// a branch for pending <= 10 computes a time-based value (timeSinceStartOfServerAdvance * 100)
// into a shadowing local and has no effect
return clamp(n, 0, 20)
```

`ClientToServer::AdvanceAndCatchUp` (called once per render frame) [C]:

```
if loadingLocation: KeepAlive(); return
start = now
do:
   catchUp = g_lastProcessedSequenceId < m_lastValidSequenceIdFromServer - 20  and  m_clientId != -1
   Advance()                       // connection logic, FTP, identity upkeep, send outbox
   ProcessAllServerLetters(catchUp)
   if loadingLocation: return      // a START_GAME was just processed
while catchUp and (now - start) <= 0.3 s
```

So a client more than 20 frames behind runs 20 slices (2 frames) per iteration for up to 0.3 s per render frame; otherwise it drains 30 % of its pending slices per render frame, at most 20.

### 2.4 Effective buffering and input latency [I]

Derived from §2.3: the server injects 100 slices/s; at render rate R fps the client consumes ⌊0.3·P⌋ slices per frame, so the steady-state backlog is P ≈ 100/(0.3·R) slices: ≈ 5.6 slices (56 ms) at 60 fps, ≈ 11 slices (110 ms) at 30 fps. Because ⌊0.3·P⌋ = 0 for P ≤ 3, a backlog of up to 3 slices is never drained until the next letter arrives. Total command latency ≈ U(0, 100 ms) tick wait + RTT + backlog. A replacement design should use an explicit, fixed input-delay/jitter buffer instead. [U-intent] whether the dead time-based branch was meant to pace by time (22-U4).

### 2.5 Estimated latency (statistics only)

`GetEstimatedServerSeqId = max( floor((now − g_gameTimer.m_estimatedServerStartTime)/0.1), m_serverSequenceId )`; `m_estimatedServerStartTime` = minimum over received letters of `now − seqId × 0.1`. `GetEstimatedLatency = max(0, estimated − g_lastProcessedSequenceId) × 0.1` s. Used only by the network statistics window (and the benchmark report, which is compiled out: `code/benchmark.cpp` is wholly inside `#ifdef BENCHMARK_AND_FTP` [B:DEAD]). `m_latency` (PING round trip) is computed from relayed PING commands, but `SendPing` has no caller [C] `code/network/clienttoserver.cpp:ClientToServer::GetEstimatedServerSeqId`, `GetEstimatedLatency`, `ReceiveLetter`; `code/interface/network_window.cpp`.

### 2.6 Pause and game speed

- `App::ToggleGamePaused` allows a pause only when no team is `TeamTypeRemotePlayer` (effectively versus-CPU/single player), not in the grace period and not game over; it sends PAUSE [C] `code/app.cpp:App::ToggleGamePaused`.
- Every client executing PAUSE toggles `m_userRequestsPause` and requests speed 0 (or 1) with rate 0.2 per frame: speed after n frames = 0.8ⁿ (stopping) or 1 − 0.8ⁿ (resuming); snapped when within 0.01, i.e. **21 frames ≈ 2.1 s** of slow-down/speed-up [C] `GameTimer::Tick`; [I] arithmetic.
- `App::GamePaused()` = location exists and (`GameTimer::IsPaused()` (speed < 0.01) or `m_outOfSyncClients.Size() > 0` or (lost focus and single player)); the `m_userRequestsPause` term is commented out [C] `code/app.cpp:App::GamePaused`. While paused the server keeps producing frames; clients consume them (sequence ids advance) but skip `Location::Advance`; `GameTimer::Tick` still blends speed, so a pause can end.
- Other deterministic speed changes come from the simulation: slow-down crate `0.2 + syncfrand(0.2)` for 15 s, speed-up crate `1.3 + syncfrand(0.7)` for 15 s (rate 0.05), meteor slow-motion 0.4 for 4 s (rate 0.1), victory (speed 0, then locked) [C] `code/worldobject/crate.cpp:Crate::RunSlowDownTime/RunSpeedUpTime`, `code/worldobject/meteor.cpp:Meteor::Advance`, `code/multiwinia.cpp:Multiwinia::DeclareWinner`. The grace countdown `Multiwinia::AdvanceGracePeriod` decrements `GRACE_TIME` (3.0 s) by a literal 0.1 per frame while every Local/Remote/Spectator team is ready, then requests speed 1.0 immediately (rate 1.0) [C] `code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod`, `code/lib/universal_include.h:GRACE_TIME`.
- A requested duration counts down 0.1 per frame regardless of speed, after which the target becomes 1.0; `LockSpeedChange` blocks further requests [C] `GameTimer::Tick`, `GameTimer::RequestSpeedChange`.

---

## 3 Transport — NetLib over UDP

NetLib is "a very thin portable UDP network library" [C] `contrib/systemiv/lib/netlib/net_lib.h`.

| Class / function | Behaviour | Ref |
|---|---|---|
| `NetLib::Initialise` | `WSAStartup(2.2)`; fails if 2.2 is unavailable. Called once from `MetaServer_Initialise`. | [C] `contrib/systemiv/lib/netlib/net_lib.cpp:NetLib::Initialise`, `contrib/systemiv/lib/metaserver/metaserver.cpp:MetaServer_Initialise` |
| `NetSocketListener(port, name, iface="")` | Owns one UDP socket. `Bind()`: `socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)`, bind to `INADDR_ANY:port` (or a resolved interface via `getaddrinfo`), up to 6 attempts with a sleep of `attempts × 100 ms` while the address is in use; port 0 ⇒ OS-assigned, read back with `getsockname`. Mutex-guarded, idempotent. | [C] `contrib/systemiv/lib/netlib/net_socket_listener.cpp:NetSocketListener::Bind` |
| `NetSocketListener::StartListening(cb)` | **Blocking loop** on the calling thread: `recvfrom` into a fresh `NetUdpPacket` (buffer `MAX_PACKET_SIZE` = **1500** bytes); on success calls `cb(packet)` (the callee owns and deletes it); a result `<= 0` is logged via `AppDebugOutNetSocketError` and the packet is reused. Runs until `StopListening` (sets the flag, `shutdown` + `closesocket`, `Sleep(250)`). | [C] `NetSocketListener::StartListening`, `StopListening` |
| `NetSocketListener::EnableBroadcast` | `setsockopt(SO_BROADCAST)` | [C] |
| `NetSocketSession(listener, host, port)` | A *send endpoint* that reuses the listener's bound socket (so replies come back to the same port — essential for NAT hole punching). Resolves `host` with `getaddrinfo` (AF_INET). `WriteData` = one `sendto` (no fragmentation, no retry). | [C] `contrib/systemiv/lib/netlib/net_socket_session.cpp` |
| `NetSocket` | A separately created "connected" UDP socket with a select-based write loop (timeout 10 000 ms, poll 100 ms). Only used by `GetLocalHostIP` to discover the default-route local address by `connect("80.175.29.66", 5000)` + `getsockname` (no packet sent); fallback `gethostbyname(gethostname())`, then `"127.0.0.1"`. | [C] `contrib/systemiv/lib/netlib/net_socket.cpp`, `net_lib.cpp:GetLocalHostIP` |
| `NetUdpPacket` | `{int m_length; sockaddr_in m_clientAddress; char m_data[1500];}`; `GetPort()` = `ntohs(sin_port)`. | [C] `contrib/systemiv/lib/netlib/net_udp_packet.h` |
| `NetMutex` | Win32 `CRITICAL_SECTION` (recursive); `NetLockMutex` RAII guard; `NetEvent` (auto-reset Win32 event, `Wait(ms)`, −1 = infinite); `NetSemaphore`. | [C] `contrib/systemiv/lib/netlib/net_mutex.h`, `net_mutex_win32.cpp` |
| `NetStartThread(fn, arg)` | `CreateThread`, never joined. | [C] `contrib/systemiv/lib/netlib/net_thread_win32.cpp` |
| `GetLocalHost`, `StrToIpAddress`, `IpAddressToStr` | Host-name resolution helpers; `IpToString(in_addr, char*)` (`code/network/generic.cpp`) is the game-side equivalent used by both listen callbacks. | [C] |

**Ports** [C] `code/network/network_defines.h`, `Server::Initialise`, `ClientToServer::OpenConnections`: game server UDP **4000** (pref `NetworkServerPort`), game client UDP **4001** (pref `NetworkClientPort`; falls back to an OS-chosen port), metaserver/matchmaker **5113**, metaserver client listener / LAN broadcast **5009** (§12). If the server port cannot be bound, `Server::Initialise` ignores the failure ("TODO PORT FORWARDING") [C].

**Reliability model** [C]: server→client letters are reliable through the resend window (§6.2); client→server letters are fire-and-forget, with application-level repetition (ALIVE every 0.1 s, SYNCHRONISE per frame, JOIN every 1 s, team/name/colour requests until echoed, CLIENT_LEAVE sent 3×). Every received datagram is parsed as one `Directory` (§4.2); a malformed datagram is discarded ("bogus letter").

**The 1500-byte limit.** `recvfrom` uses a 1500-byte buffer [C] `contrib/systemiv/lib/netlib/net_lib.h:MAX_PACKET_SIZE`, `net_socket_listener.cpp:StartListening`; senders write any size in one `sendto` (the client into a 16 384-byte buffer, the server whatever `Directory::Write` produces) [C] `ClientToServer::AdvanceSender`, `Server::AdvanceSender`. On Windows a larger datagram makes `recvfrom` fail with `WSAEMSGSIZE`, which the loop treats as an error and drops [I: documented WinSock behaviour]; on BSD sockets it is truncated, then `Directory::Read` fails on the missing end marker and it is dropped [I]. Consequences [I computed from §4]: (a) a single history letter larger than ≈ 1500 bytes can never be delivered — the server resends it (and piggy-backs it) forever, the client cannot pass that sequence id, shows "lost connection" after 5 s, and never recovers; (b) a normal-size letter that carries a large piggy-backed copy (§6.2) is lost, but a later letter's piggy-back usually repairs the gap. The server only logs "Largest datagram sent" [C] `Server::AdvanceSender`. A relayed SETGAMEOPTIONARRAYELEMENTINT command is 46 bytes and an ALIVE 53–61 bytes (§4.3), so a single letter crosses the limit at ≈ 30 option commands or ≈ 25 ALIVE-sized commands in one 0.1-s tick. The new design must enforce an MTU budget or fragment.

**Unused transport helpers** [C]: `code/network/listener.*` (`Listener`, `EventListener`, `ListenerList` observer classes) — no users; `code/network/bytestream.h` and `contrib/systemiv/lib/netlib/bytestream.h` (raw-pointer `READ_`/`WRITE_` macros) — no users.

