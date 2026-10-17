# P15_network — Networking, metaserver, netlib
Scope: `code/network/*` (all 30 files incl. the uncompiled `network_defines_debug.h`, `networkupdate.inc`), `contrib/systemiv/lib/metaserver/*` (12 files; 6 compiled), `contrib/systemiv/lib/netlib/*` (20 files; 16 compiled), `contrib/systemiv/lib/network_stream*` (3 files). Supporting evidence read: `docs/Network.txt`, `docs/NetworkThreads.txt`, `docs/network overview.txt`, `docs/integrating virtual players into network system.txt`. Out-of-scope files read to resolve questions are listed in the coverage CSV (Directory wire format, GameTimer, TeamControls packing, main loop, App network entry points, lobby/browser UI call sites). Build baseline: see build_matrix.md (`Release|Win32`, `TARGET_PC_FULLGAME`, `LAN_PLAY_ENABLED`, `WAN_PLAY_ENABLED`, `NETWORK_STATS_ENABLED`, `AUTHENTICATION_LEVEL 1`; `TARGET_DEBUG`, `TARGET_BETATEST_GROUP`, `TESTBED_ENABLED`, `TRACK_SYNC_RAND`, `LOCATION_EDITOR` all **off**).

Path convention: all refs relative to `SRC=multiwinia/`. Tags: [C] confirmed by reading, [I] inferred (reason given), [U] unknown.

---

## [→ 22_determinism_and_networking] 1. Network model at a glance

| Question | Answer | Evidence |
|---|---|---|
| Topology | **Star, client/server over UDP.** One process hosts a `Server`; every participant (including the host) runs a `ClientToServer`. Clients never talk to each other. | [C] `code/app.cpp:App::StartNetwork`, `code/network/server.cpp:Server::Initialise`, `code/network/clienttoserver.cpp:ClientToServer::ClientJoin` |
| Is the host also a client? | **Yes.** `App::StartNetwork(true,…)` creates `m_server` and then calls `m_clientToServer->ClientJoin()` on the same process. Host-client ↔ server traffic bypasses sockets: the host client hands letters straight to `Server::ReceiveLetter` (tagged with the host's LAN IP + client listen port), and the server hands letters for `m_receiverId == m_clientToServer->m_clientId` straight to `ClientToServer::ReceiveLetter`. The host client nevertheless goes through exactly the same sequencing/ack pipeline as remote clients. | [C] `code/network/clienttoserver.cpp:ClientToServer::AdvanceSender` (`if( g_app->m_server ) g_app->m_server->ReceiveLetter(…)`), `code/network/server.cpp:Server::AdvanceSender` |
| Dedicated server? | **No.** No headless mode exists; the server is always embedded in a game client. | [C] only construction site of `Server` is `App::StartNetwork` (`code/app.cpp`) |
| Single player | Uses the identical pipeline: `App::StartSinglePlayerServer` → `StartNetwork(true, NULL, -1)` → server IP `"127.0.0.1"`, server port **-1**. `ClientToServer::GetServerPort() == -1` is the game-wide "single player" flag (no advertising, rejects other joiners, no kicking on auth failure). | [C] `code/app.cpp:App::StartSinglePlayerServer`, `App::StartNetwork`; `code/network/server.cpp:Server::RegisterNewTeam`, `Server::Advertise`, `Server::AuthenticateClients` |
| Multiplayer host | `App::StartMultiPlayerServer` → `StartNetwork(true, NULL, NULL)`; port `NULL`(=0) ≠ -1 so the host client joins its own LAN IP (`GetLocalHostIP`) at the server listener's port. | [C] `code/app.cpp:App::StartMultiPlayerServer`, `App::StartNetwork` |
| Lockstep vs state sync | **Deterministic lockstep with a server-sequenced command stream ("relay lockstep").** The server never simulates authoritatively and never sends world state. Every 0.1 s it packages all client commands received since the previous tick into one *UPDATE letter* with a monotonically increasing sequence id, appends it to an unbounded history, and sends it to every client. Each client executes the letters strictly in sequence-id order; one UPDATE letter = one simulation frame of 0.1 s × game speed, split into 10 slices. Only a 1-byte checksum per frame flows back for desync detection. | [C] `code/network/server.cpp:Server::Advance`, `Server::SendLetter`; `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`, `ClientToServer::GetNextLetter` |
| Input delay | Not a fixed number of frames. Input becomes effective at the start of the first UPDATE frame generated after the server receives it: delay = (time to next server tick, 0–100 ms) + one-way latency to server + one-way latency back + client-side buffering (see §2.4). The local host experiences zero network latency but the same tick quantisation and buffering. | [C] pipeline; [I] composition of delays |
| Lobby | The lobby runs **inside the same lockstep channel**: game options, team assignments, names, colours, chat, ready flags and the START_GAME command are all sequenced commands in the history. A late joiner reconstructs lobby state by replaying the full history from sequence id 0. | [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessServerUpdates` (SETGAMEOPTION*, START_GAME, REQUEST_TEAMNAME…); `code/UI/WaitingButton.h` (connect progress = `g_lastProcessedSequenceId / m_serverSequenceId`) |
| Transport reliability | Custom over UDP: sequence ids + cumulative acks (every client letter carries `LASTSEQID` = highest contiguous id received) + server-side resend window of 3 letters per tick + one piggy-backed copy of the oldest unacknowledged letter + run-length encoding of empty frames. Client→server messages are **not** acknowledged or resent (they are re-sent at the application level where it matters: ALIVE 10 Hz, team requests, joins). | [C] `code/network/server.cpp:Server::UpdateClients`; `code/network/clienttoserver.cpp:ClientToServer::ReceiveLetter` |

### 1.1 Peer roles

| Role | Object | Count | Responsibilities |
|---|---|---|---|
| Server | `Server` (`g_app->m_server`) | 0 or 1 per process | Accept/authenticate clients, assign client ids and team slots, sequence and broadcast commands, keep full history, resend lost letters, compare sync bytes, relay IFrame files, advertise to metaserver/LAN, kick |
| Client | `ClientToServer` (`g_app->m_clientToServer`) | exactly 1 per process (created at app start) | Connect/handshake, send commands, receive/order letters, execute simulation slices, generate sync bytes, display desync/lag |
| Per-client server record | `ServerToClient` | one per connected (and per disconnected) client | IP/port, socket session, auth key/key id, password, client id, spectator flag, last known/sent seq id, caught-up flag, last-message time, sync byte array, saved last ALIVE/SELECTUNIT/REQUEST_COLOUR, FTP manager |
| Team slot (server side) | `ServerTeam {m_clientId, m_teamType}` in `Server::m_teams` (DArray, index = team id 0..5) | ≤ `NUM_TEAMS` = 6 | Which client owns which team; a client may own several (e.g. host owns its own + all CPU teams) |

[C] `code/network/server.h`, `code/network/servertoclient.h`, `code/globals.h:NUM_TEAMS`. This two-level client/team split is exactly what `docs/integrating virtual players into network system.txt` proposed ("DArray of Teams … DArray of Clients") [C].

---

## [→ 22_determinism_and_networking] 2. Timing model

### 2.1 Server tick
- `Server::Initialise`: `m_nextServerAdvanceTime = GetHighResTime() + 0.1`. [C]
- `Server::AdvanceIfNecessary(now)`: if `now > m_nextServerAdvanceTime` → `Advance()` once, then `m_nextServerAdvanceTime += 0.1`. At most **one** tick per call; if the main loop stalls, ticks are produced back-to-back on subsequent calls until the schedule catches up (no tick skipping). `Server::SkipToTime` exists to jump the schedule ("waking from sleep") but has no caller. [C] `code/network/server.cpp:Server::AdvanceIfNecessary`, `Server::SkipToTime`
- Callers: every main loop — `code/main.cpp:LocationGameLoop`, `GlobalWorldGameLoop` (and the front-end menu loop at `code/main.cpp` ~L1465), and the loading screen loop under `m_networkMutex.TryLock()` (`code/loading_screen.cpp:LoadingScreen::Render`). So the server ticks at **10 Hz real time** regardless of game speed or pause. [C]

### 2.2 Client frame and slices
- One UPDATE letter ⇒ one *frame*. On taking a letter (`g_sliceNum == -1`) the client applies its commands (`ProcessServerUpdates`), sets `g_lastProcessedSequenceId = seqId`, `g_sliceNum = 0`, calls `g_gameTimer.Tick()`, then `GenerateSyncValue()` + `SendSyncronisation()`. Subsequent slice steps call `Location::Advance(g_sliceNum)` and `ParticleSystem::Advance(g_sliceNum)` (skipped while `App::GamePaused()`), and **at slice 8** `Multiwinia::Advance()` (always, even when paused, if `App::Multiplayer()`); after slice 9 `g_sliceNum = -1`. `NUM_SLICES_PER_FRAME = 10` (`code/globals.h`). [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`
- Non-UPDATE sequenced letters (CLIENTHELLO, CLIENTGOODBYE, TEAMASSIGN, NETSYNCERROR, …) consume a sequence id but do **not** start a frame, tick the timer, or produce a sync byte. Unsequenced letters (`SEQID == -1`) are processed as soon as they reach the inbox head. [C] same function.
- `GameTimer::Tick` adds `GetServerAdvancePeriod() = 0.1 × m_gameSpeed` to `m_gameTime` (network time) unless paused, and moves `m_gameSpeed` toward a requested speed by `diff × speedOfChange` per tick (snap when within 0.01). `GameTimer::Reset` sets speed **0** in multiplayer (game starts frozen until the grace countdown finishes), 1 otherwise. [C] `code/gametimer.cpp:GameTimer::Tick`, `GameTimer::Reset`

### 2.3 Slices per render frame (client pacing controller)
`ClientToServer::GetNumSlicesToAdvance(catchUp)` [C]:
```
if catchUp: return 20
pending = (m_lastValidSequenceIdFromServer - g_lastProcessedSequenceId) * 10
if g_sliceNum != -1: pending += 10 - g_sliceNum
n = floor(pending * 0.3)
// (a branch for pending <= 10 computes a time-based value into a shadowed local and has no effect)
return clamp(n, 0, 20)
```
`ClientToServer::AdvanceAndCatchUp` [C]:
```
if loadingLocation: KeepAlive(); return
start = now
loop:
   catchUp = g_lastProcessedSequenceId < m_lastValidSequenceIdFromServer - 20  and  m_clientId != -1
   Advance()                       // connection logic, FTP, send outbox
   ProcessAllServerLetters(catchUp)
   if loadingLocation: return      // a START_GAME was just processed
while catchUp and (now - start) <= 0.3 s
```
So a client more than 20 frames behind runs 20 slices (2 frames) per iteration for up to 0.3 s per render frame; otherwise it drains 30 % of its pending slices per render frame, at most 20.

### 2.4 Effective buffering / input latency [I]
Derived from §2.3: the server injects 100 slices/s; at render rate R fps the client consumes ⌊0.3·P⌋ slices per frame, so the steady-state backlog is P ≈ 100/(0.3·R) slices: ≈ 5.6 slices (56 ms) at 60 fps, ≈ 11 slices (110 ms) at 30 fps; and because ⌊0.3·P⌋ = 0 for P ≤ 3, a backlog of up to 3 slices is never drained until the next letter arrives. Total command latency ≈ U(0,100 ms) tick wait + RTT + backlog. A replacement design should use an explicit, fixed input-delay/jitter buffer instead.

### 2.5 Estimated latency (stats only)
`GetEstimatedServerSeqId = max( floor((now - g_gameTimer.m_estimatedServerStartTime)/0.1), m_serverSequenceId )`; `m_estimatedServerStartTime` = min over received letters of `now - seqId×0.1`. `GetEstimatedLatency = max(0, est - g_lastProcessedSequenceId) × 0.1` s. Used only by the network stats window and benchmark upload. `m_latency` (PING round trip) is computed from relayed PING commands, but `SendPing` has no caller. [C] `code/network/clienttoserver.cpp:GetEstimatedServerSeqId`, `GetEstimatedLatency`, `ReceiveLetter`; `code/interface/network_window.cpp`; `code/benchmark.cpp`

---

## [→ 22_determinism_and_networking] 3. Transport layer — NetLib (contrib/systemiv/lib/netlib)

NetLib is "a very thin portable UDP network library" [C] `contrib/systemiv/lib/netlib/net_lib.h`.

| Class / function | Behaviour | Ref |
|---|---|---|
| `NetLib::Initialise` | `WSAStartup(2.2)`; fails if 2.2 unavailable. Called once from `MetaServer_Initialise`. | [C] `net_lib.cpp:NetLib::Initialise`, `metaserver.cpp:MetaServer_Initialise` |
| `NetSocketListener(port, name, iface="")` | Owns one UDP socket. `Bind()`: `socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)`, bind to `INADDR_ANY:port` (or resolved interface via `getaddrinfo`), up to 6 attempts with sleep `attempts×100 ms` while the address is in use; port 0 ⇒ OS-assigned, read back with `getsockname`. Mutex-guarded, idempotent. | [C] `net_socket_listener.cpp:NetSocketListener::Bind` |
| `NetSocketListener::StartListening(cb)` | **Blocking loop** on the calling thread: `recvfrom` into a fresh `NetUdpPacket` (buffer `MAX_PACKET_SIZE` = **1500** bytes); on success calls `cb(packet)` (callee owns/deletes it); errors (`length <= 0`) are logged (except `WSAEINTR`, `WSAECONNRESET` = ICMP port unreachable) and the packet is reused. Runs until `StopListening` (sets flag, `shutdown`+`closesocket`, `Sleep(250)`). | [C] `NetSocketListener::StartListening`, `StopListening` |
| `NetSocketListener::EnableBroadcast` | `setsockopt(SO_BROADCAST)` | [C] |
| `NetSocketSession(listener, host, port)` | A *send endpoint* that reuses the listener's bound socket (so replies come back to the same port — essential for NAT hole punching). Resolves `host` with `getaddrinfo` (AF_INET). `WriteData` = one `sendto` (no fragmentation, no retry). | [C] `net_socket_session.cpp` |
| `NetSocket` | A separately-created "connected" UDP socket with a select-based write loop (timeout 10 000 ms, poll 100 ms). Only used by `GetLocalHostIP` to discover the default-route local address by `connect("80.175.29.66", 5000)` + `getsockname` (no packet sent); fallback `gethostbyname(gethostname())`, then `"127.0.0.1"`. | [C] `net_socket.cpp`, `net_lib.cpp:GetLocalHostIP` |
| `NetUdpPacket` | `{int m_length; sockaddr_in m_clientAddress; char m_data[1500];}`; `GetPort()` = `ntohs(sin_port)`. | [C] `net_udp_packet.h/.cpp` |
| `NetMutex` | Win32 `CRITICAL_SECTION` (recursive); `NetLockMutex` RAII guard; `NetEvent` (auto-reset Win32 event, `Wait(ms)`, -1 = infinite); `NetSemaphore`. | [C] `net_mutex.h`, `net_mutex_win32.cpp` |
| `NetStartThread(fn, arg)` | `CreateThread`, never joined. | [C] `net_thread_win32.cpp` |
| `GetLocalHost`, `StrToIpAddress`, `IpAddressToStr` | host-name resolution helpers; `IpToString` (`code/network/generic.cpp`) is the game-side equivalent. | [C] |

Hazard [I]: Windows `recvfrom` with a 1500-byte buffer fails with `WSAEMSGSIZE` for larger datagrams, which `StartListening` treats as an error and drops. Any letter whose serialised size exceeds 1500 bytes (e.g. a busy UPDATE plus its piggy-backed PREVUPDATE copy) can therefore never be delivered, and the same letter is resent forever → that client stalls. The server only logs "Largest datagram sent". The new design must enforce an MTU budget or fragment.

---

## [→ 22_determinism_and_networking] 4. Encoding

### 4.1 Primitive wire encoding — `contrib/systemiv/lib/network_stream*`
All multi-byte values are written as their in-memory bytes; on big-endian hosts they are byte-reversed, so **the wire format is little-endian**. [C] `contrib/systemiv/lib/network_stream_inlines.cpp:ReadNetworkValue/WriteNetworkValue` (`#ifndef __BIG_ENDIAN__`).

| Primitive | Encoding | Bytes | Ref |
|---|---|---|---|
| `int`, `unsigned` | LE 32-bit | 4 | [C] `WriteNetworkValue<T>` |
| `float` | IEEE-754 binary32 LE | 4 | [C] |
| `double` | IEEE-754 binary64 LE | 8 | [C] |
| `unsigned long long` | LE 64-bit | 8 | [C] |
| `short` / `unsigned short` | LE 16-bit | 2 | [C] (used by `TeamControls::Pack`) |
| `char`, `bool` | 1 byte (bool written as 0/1) | 1 | [C] |
| `Vector3` | x, y, z as **double** | 24 | [C] `network_stream.cpp:WriteNetworkValue(Vector3)`; `code/lib/vector3.h` and `contrib/systemiv/lib/math/vector3.h` both declare `double x, y, z` under the same include guard `VECTOR3_H` |
| Packed int | value 0..254 → 1 byte; otherwise byte `0xFF` + 4-byte int (so −1 = 5 bytes) | 1 or 5 | [C] `network_stream.cpp:WritePackedInt` |
| Dynamic string | packed-int length (−1 = NULL) + raw bytes, no terminator; on read a length < 0 (≠ −1) or > max ⇒ substitute safe string | 1/5 + n | [C] `WriteDynamicString`, `ReadDynamicString` |
| Unicode string | packed-int length in UTF-16 code units (−1 = NULL) + 2 bytes per unit (Win32 `wchar_t`); Linux/Mac convert to/from UTF-16LE | 1/5 + 2n | [C] `WriteUnicodeString`, `ReadUnicodeString` |
| Void (blob) | 4-byte int length + bytes | 4 + n | [C] `WriteVoidData` |
| Number array | packed-int count + each element via `WriteNetworkValue` | 1/5 + 4n for ints | [C] `WriteNumbersData` |

### 4.2 Message container — `Directory` (tosser; read from `contrib/systemiv/lib/tosser/directory.cpp`, outside this partition)
Every network message in the game, the metaserver protocol and the matchmaker is a `Directory`: a named node with a list of typed named fields and a list of child Directories. Wire layout [C] `Directory::Write`, `DirectoryData::Write`:

```
Directory     := '<' DynString(name) PackedInt(numFields) Field* PackedInt(numSubdirs) Directory* '>'
Field         := DynString(fieldName) u8(type) Value(type)
Value(1 INT)      := i32
Value(2 FLOAT)    := f32
Value(3 CHAR)     := u8          (unsigned char fields are stored as CHAR)
Value(4 STRING)   := DynString
Value(5 BOOL)     := u8 (0/1)
Value(6 VOID)     := i32 length, bytes
Value(7 FIXED)    := 8 bytes     (only with USE_FIXED — dead in baseline)
Value(8 INTS)     := PackedInt count, i32*
Value(9 ULLONG)   := u64
Value(10 UNICODE) := UnicodeString
```
Limits on read: ≤ 1024 fields per node (`DIRECTORY_MAXDIRSIZE`; the subdirectory count check mistakenly re-tests the field count), strings ≤ 10240 chars (`DIRECTORY_MAXSTRINGLENGTH`). Start/end markers `'<'`/`'>'` must match or the whole datagram is rejected ("bogus letter"). `CreateData` replaces an existing field of the same name. Missing-field reads return safe values: int −1, float −1.0, char `'?'` (63), string `"[SAFESTRING]"`, bool false, ullong `0xFFFF…`. [C] `contrib/systemiv/lib/tosser/directory.h` (`DIRECTORY_SAFE*`), `directory.cpp:Directory::Read`.

Consequence used by the server filter (§7): `GetDataUChar(NET_DARWINIA_TEAMID)` on a message without a TEAMID field returns 63, not 255. [C]

Field names are deliberately 1–4 character codes ("optimised network symbols"); an uncompiled alternative header `code/network/network_defines_debug.h` maps them to readable names ("much higher bandwidth usage") [C]. `TRACK_SYNC_RAND` builds contain a decoder table `DecodeCmd` (`code/network/clienttoserver.cpp`) that lists every code → name pair [C] [B:DEBUG].

### 4.3 Size budget of common messages [I computed from §4.1/§4.2]
| Message | Size (bytes, excl. 28-byte IP+UDP header; code accounts `UDP_HEADER_SIZE` = 32) |
|---|---|
| Node overhead ('<', name "ba", counts, '>') | 7 |
| COMMAND field (e.g. "cd") | 7 |
| char field | 5; int field 8; Vector3 blob field 32 |
| Client ALIVE letter | 61 (flags + mouse only) … 69 (with both direct vectors) |
| Client SYNCHRONISE letter | 35 |
| UPDATE header as stored in history (COMMAND + SEQID) | 22; as sent (+LASTSEQID) 30 |
| ALIVE as relayed inside UPDATE (FROMIP/FROMPORT/LASTSEQID stripped) | 53–61 |
| 4-player in-game UPDATE with only ALIVEs | 30 + 4×53 = 242; with piggy-backed PREVUPDATE copy ≈ 476 |
| RLE "N empty updates" letter | 38 |
⇒ ≈ 5 KB/s per client downstream and ≈ 1 KB/s upstream in a quiet 4-player game.

### 4.4 `NetworkValue` family — UI ↔ network option binding (`code/network/networkvalue.*`)
Despite the name these classes are not a wire encoding; they adapt lobby UI input fields (`InputField::Type` order: Nowt, Char, Bool, Int, Float, String, UnicodeString) to network requests. [C] `code/network/networkvalue.h`.

| Class | Get() reads | Set() does | Ref |
|---|---|---|---|
| `NetworkValue` (abstract) | — | `ToLatinString`/`FromLatinString` via Unicode; abstract `To/FromUnicodeString` | [C] |
| `BoundedNetworkValue` | — | optional `[min,max]` clamp `Bound()` (the constructor initialises `m_max` with itself — uninitialised value) | [C] `networkvalue.cpp:BoundedNetworkValue` |
| `NetworkInt`, `NetworkBool`, `NetworkChar` | `%d` text | parse `%d` | [C] |
| `NetworkFloat`, `NetworkDouble` | `%.2f` text | parse `%lf`, then `iv_trunc_low_nibble` (determinism truncation, see 22) | [C] `NetworkFloat::FromUnicodeString` |
| `NetworkString`, `NetworkUnicodeString`, `LocalString` (bounded copy), `LocalUnicodeString`, `LocalType<…>` (`LocalInt/Float/Double/Char/Bool/IntAsBool`), `NetworkIntAsBool`, `NetworkIntAsString` | local variable | local variable (no network) | [C] |
| `NetworkGameOptionInt(option, int&)` | bound int | `RequestSetGameOptionInt(option, Bound(v))` | [C] |
| `NetworkGameOptionBool(option, bool&)` | bound bool | `RequestSetGameOptionInt(option, Bound(v))` | [C] |
| `NetworkGameOptionString(option, const char*&)` | bound string | `RequestSetGameOptionString(option, v)` | [C] |
| `NetworkGameOptionMapId` | index of `g_app->m_requestedMap` in `m_gameMenu->m_maps[gameType]` (case-insensitive file name match), −1 if none | `RequestSetGameOptionString(GAMEOPTION_MAPNAME, maps[v]->m_fileName)` | [C] `GetMapId`, `SetMapId` |
| `NetworkGameParam(index)` | `m_multiwinia->m_params[index]` | `RequestSetGameOptionArrayElementInt(GAMEOPTION_GAME_PARAM, index, v)` | [C] |
| `NetworkGameResearchLevel(index)` | `m_globalWorld->m_research->m_researchLevel[index]` | `RequestSetGameOptionArrayElementInt(GAMEOPTION_GAME_RESEARCH_LEVEL, index, v)` | [C] |
| `NetworkIntArray(size, factory)` | array of the above | — | [C] |

Lobby bindings instantiated by `GameMenuWindow`: `m_gameType`(GAMEOPTION_GAME_TYPE), `m_requestedMapId`(map), `m_aiMode`(GAMEOPTION_AIMODE), `m_coopMode`(GAMEOPTION_COOPMODE, bool), `m_singleAssaultRound`(GAMEOPTION_SINGLEASSAULT, bool), `m_basicCrates`(GAMEOPTION_BASICCRATES, bool), `m_params[64]`, `m_researchLevel[NumResearchItems]`. [C] `code/UI/GameMenuWindow.h`, `code/UI/GameMenuWindow.cpp:GameMenuWindow::GameMenuWindow`

## [→ 22_determinism_and_networking] 5. Message catalogue — common fields and identifiers

All game messages: Directory name `NET_DARWINIA_MESSAGE` = `"ba"`, command field `NET_DARWINIA_COMMAND` = `"bb"` (string). [C] `code/network/network_defines.h`. The server adds `FROMIP "ga"` (string) and `FROMPORT "gb"` (int) to every received letter (`Server::ReceiveLetter`) and strips them, together with `LASTSEQID`, before relaying (`ServerToClientLetter::AddUpdate`). [C] `code/network/server.cpp`.

### 5.1 Data-field codes (complete list from `code/network/network_defines.h`)
| Code | Symbol | Wire type as used | Meaning |
|---|---|---|---|
| da | TEAMTYPE | int | requested/assigned team type (`TeamTypeUnused=-1, LocalPlayer=0, RemotePlayer=1, CPU=2, Spectator=3`, `code/team.h`) |
| db | DESIREDTEAMID | int | always −1; server ignores (code commented out) |
| dc | LASTPROCESSEDSEQID | int | seq id the sync byte refers to |
| dd | SYNCVALUE | char | 1-byte sync value |
| de | TEAMID | char (int in TEAMASSIGN) | team slot 0..5, 255 = none |
| df | POSITION | void (24-byte Vector3) | world position |
| dg | ALIVE_DATA | void | packed `TeamControls` (§5.4) |
| dl | UNITID | int | |
| dm | ENTITYID | int | entity index (or unique id for SHAMANSUMMON) |
| dn | BUILDINGID | int | building unique id |
| do | ENTITYTYPE | char | shaman summon type |
| dp | RADIUS | float | selection radius |
| dq | PROGRAM | char | task-manager program/task id; 255 = "entity" in TERMINATE |
| dr | RANDSEED | int | sync RNG seed |
| ds | TEAMNAME | unicode | |
| dt | COLOURID | int | colour 0..7, −1 = none |
| dw | UNICODEMESSAGE | unicode | chat text |
| dx | ENCRYPTION_PADDING_CTS | — | **unused** |
| dy | SERVER_PASSWORD | unicode | join password |
| dz | CHRISTMASMODE | (used as a *command* code) | |
| ea | DISCONNECT | int (reason) / also a command code | |
| ed | CLIENTID | int (char in chat/spectator msgs) | |
| ei | SYNCERRORID | string | `"YYYY-MM-DD H.MM.S <seq>"` local time |
| ej | SYNCERRORSEQID | int | |
| el | VERSION | string | `APP_VERSION` |
| em | SYSTEMTYPE | string | `APP_SYSTEM` = `"PC"` |
| en | PREVUPDATE | (subdirectory name) | piggy-backed earlier letter |
| eo | NUMEMPTYUPDATES | int | run-length of empty frames |
| ep | SEQID | int | sequence id; −1 = unsequenced/direct |
| eq | LASTSEQID | int | client→server: highest contiguous id received (ack); server→client: server's next seq id at send time |
| er | XUID | — | **unused** (Xbox Live) |
| et | CLIENTISDEMO | bool | client uses a demo key |
| eu | ENCRYPTION_PADDING_STC | — | **unused** |
| ev | SERVER_FULL | (command code) | handled by client, never sent |
| ew | MAKE_SPECTATOR | int | 0/1 in CLIENTID reply |
| ey | SET_PLAYERACHIEVEMENTS | (command code) | |
| ez | PLAYERACHIEVEMENTS | char | "viral achievement" bit mask |
| fa | FILESEND | (command code) | FTP block/ack |
| fb | FILENAME | string | |
| fc | FILESIZE | int | |
| fd | BLOCKNUMBER | int (data) / ints (ack list) | |
| fe | BLOCKDATA | void (≤ 1024 bytes) | |
| ga | FROMIP | string | added by server on receipt |
| gb | FROMPORT | int | added by server on receipt |
| gc | FROMPLAYER | — | **unused** |
| he | GAMEOPTION_NAME | int | option id (§5.3) |
| hf | GAMEOPTION_INDEX | int | array index (or element count for ARRAYINT) |
| hg | GAMEOPTION_VALUE | int / string / ints | |
| hi | GAMEOPTION_VERSION | int | per-option version counter |
| hj | GAMEOPTION_PDLC_BITMASK | — | **unused** (downloadable content, console remnant) |
| hk | JOIN_VIA_INVITE | — | **unused** |
| c3 | TEAM_SCORES | ints[6] (also the command code) | |
| cob | DIRECTROUTE | char (bool in armour orders) | |
| cod | FORCEONLYMOVE | char | |
| coe | SECONDARYFUNCTION | char | |
| (metaserver) dq AUTHKEY, dr AUTHKEYID, di GAMEVERSION, dB RANDOM, dC STARTTIME | see §15 | | also embedded in game messages |

Preference names defined here: `NetworkClientPort` (default 4001), `NetworkServerPort` (default 4000), `NetworkUsePortForwarding` (default 0), `NetworkTrackSynchronisation` (unused in code). [C] `code/network/network_defines.h`, `ClientToServer::OpenConnections`, `Server::Initialise`.

### 5.2 Team controls payload (`ALIVE_DATA`) — `code/team.cpp:TeamControls::Pack` (outside partition, read for layout)
| Offset | Field | Bytes | Present when |
|---|---|---|---|
| 0 | flags (u16): 0x0001 unitMove, 0x0002 directUnitMove, 0x0004 primaryFireTarget, 0x0008 secondaryFireTarget, 0x0010 targetDirected, 0x0020 secondaryFireDirected, 0x0040 cameraEntityTracking, 0x0080 unitSecondaryMode, 0x0100 endSetTarget, 0x0200 consoleController, 0x0400 leftButtonPressed | 2 | always |
| 2 | directUnitMoveDx, Dy (i16 each) | 4 | flag 0x0002 |
| +0 | directUnitTargetDx, Dy (i16 each) | 4 | flag 0x0010 or 0x0020 |
| +0 | mousePos (Vector3, 3×double) | 24 | always |
[C] `code/team.cpp:TeamControls::GetFlags`, `Pack`, `Unpack`. Sent every `IAMALIVE_PERIOD` = 0.1 s from `code/main.cpp:SendTeamControlsToNetwork` while in a location (flags cleared when a window/task manager/chat log is open). Applied on every client by `Location::UpdateTeam(teamId, controls)` (P03/P09).

### 5.3 Lobby game-option ids (`GAMEOPTION_*`, `code/network/network_defines.h`)
| Id | Symbol | Message used | Client effect on receipt (`ProcessServerUpdates`) |
|---|---|---|---|
| 0 | GAME_TYPE | SETGAMEOPTIONINT | if 0 ≤ v < `Multiwinia::NumGameTypes` (7): zero all 64 params and all research levels, `SetGameResearch(zeros)`, `SetGameOptions(v, zeros)` |
| 1 | GAME_PARAM | SETGAMEOPTIONARRAYELEMENTINT (index 0..63) | if `ValidGameOption(index)`: `SetGameOption(index, v)` |
| 2 | GAME_RESEARCH_LEVEL | SETGAMEOPTIONARRAYELEMENTINT (index < `GlobalResearch::NumResearchItems`) | `m_researchLevel[index] = v` |
| 3 | MAPNAME | SETGAMEOPTIONSTRING | `strcpy(g_app->m_requestedMap, v)`; on the host: `m_server->m_noAdvertise = (singlePlayer or map "null"/empty)` |
| 4 | AIMODE | SETGAMEOPTIONINT | if 0 ≤ v ≤ `NumAITypes` (3; note off-by-one allows 3): `m_aiType = v` (`AITypeEasy=0, Standard=1, Hard=2`) |
| 5 | COOPMODE | SETGAMEOPTIONINT | `m_coopMode = v`; when enabled clears every team colour to −1 and the local requested colour; restores the menu's previously requested colour when disabled |
| 6 | SINGLEASSAULT | SETGAMEOPTIONINT | `m_assaultSingleRound = v` |
| 7 | BASICCRATES | SETGAMEOPTIONINT | `m_basicCratesOnly = v` |
| 8..13 | TEAMSSTATUS_LBOUND..UBOUND (8 + NUM_TEAMS−1) | SETGAMEOPTIONINT | `m_teamsStatus[id-8] = v` (no sender found in baseline) |
| 14 | NUMGAMEOPTIONS | — | array size |
Versioning [C] `ClientToServer::RequestSetGameOption*`, `GameOptionVersion`, `SetGameOptionVersion`: every option (and every array element of options 1 and 2, arrays of 64) carries a version; a request is sent with `localVersion+1`; a receiver applies an update only if `version > localVersion`, then stores the version. Because all clients apply the same sequenced stream, conflicting simultaneous edits resolve deterministically (the first one in the stream wins; later ones with an equal version are discarded). Versions reset to 0 on handshake (`ResetGameOptionVersions` on CLIENTID). `SETGAMEOPTIONARRAYINT` ignores versions and has no caller ("TODO: Delete this method").

