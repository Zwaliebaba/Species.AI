# M4 plan

> Status: Draft (O108); the owner answered its questions (O109); it is made Final on Playtest 1's evidence · Related: [MVP scope](mvp_scope.md) §2.8, §3 (M4) and §6.2 (Playtest 2) · [M3 plan](m3_plan.md) · [M2 plan](m2_plan.md) · [MVP assets](mvp_assets.md) · [owner decisions](../decisions/owner_decisions.md) (O4, O6, O35, O44, O54, O108, O109) · [ADR index and technology baseline](../ADR/README.md)

## 1 Purpose

This plan sets out milestone M4 of the [MVP scope](mvp_scope.md), away play. The owner had it built at once, before Playtest 1 and before M1, M2 and M3 meet their exit criteria (O108), and answered its four questions (O109). It covers:
- what M4 owes beyond the scope's own row, most of it the transport's confirmations (§2);
- what M4 rests on that is not settled yet (§3);
- the order of the work (§4);
- the tasks of M4 (§5);
- Playtest 2 (§6);
- the owner's answers (§7);
- the risks this plan sees (§8).

M4 ends when the exit criterion of the MVP scope's §3 holds: Playtest 2 (§6.2 of the scope) is run and recorded.

The scope's stop rule still holds for the MVP as a whole: a no from Playtest 1 to "is an hour of play fun?" stops the MVP for a redesign, which then keeps or reworks what M4 built (O108).

## 2 What M4 owes

M4 is the milestone at which the transport records are confirmed (the scope's §5.3), and three records put their tests here:

| Record | What M4 shows | Task |
|---|---|---|
| [ADR-088](../ADR/ADR-088-transport-messages-identity-and-diagnostics.md) | the headless server and the client connect over `QuicTransport` on the loopback interface; through the link simulator at 10 % loss and a 150 ms round trip for 30 minutes no figure freezes for more than 2 ticks after a single lost datagram; the overhead per state datagram, from MsQuic's statistics, within 5 bytes of 69 (IPv4) or 89 (IPv6); a client of another build or with an edited content file refused with its reason; the same session through `InProcessTransport` with identical per-tick hashes; every message parser fuzzed for 10⁷ inputs | T7, T8, T11 |
| [ADR-089](../ADR/ADR-089-msquic-transport-package.md) | the pinned `msquic.dll` beside each executable; `MsQuicOpenVersion` with `QUIC_API_VERSION_2` succeeding on the reference test machine, and the peer's datagram support reported enabled; the pin re-checked on nuget.org | T7, T11 |
| [ADR-083](../ADR/ADR-083-server-process-ownership-and-threading.md) | the world server's states in a process of their own, and every freeze recorded with its cause (item 7, PIL-131) | T6, T9 |
| [ADR-069](../ADR/ADR-069-test-framework.md) | the fuzzer of every message parser, run in CI | T11 |
| [ADR-071](../ADR/ADR-071-install-layout-runtime-files-and-user-data.md) | `msquic.dll` and its licence notice, the fourth allow-listed package, in the install's list (ADR-089 names the change) | T11 |
| PLT-100 | the platform layer stays socket-free: MsQuic owns the UDP socket, and only the transport's implementation links it | T7 |

*Table 1 — What the records put at M4, and the task that produces each.*

M1's, M2's and M3's records leave these to M4:

| Left by | What M4 builds | Task |
|---|---|---|
| [ADR-100](../ADR/ADR-100-headless-server-program.md) | the headless server's transport listener, and the world it saves and continues from a folder | T9 |
| [ADR-137](../ADR/ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md) | the freezes of MVP-071: a stopped server and a sleeping computer | T6 |
| [ADR-155](../ADR/ADR-155-the-front-end-and-saved-worlds-at-m2.md) | the front end's choice of pace (O80) | T10 |
| [ADR-156](../ADR/ADR-156-the-settlement-drawn-at-its-plateau-and-the-owners-growth-run.md) | the settlement of a steward who is away run by the deputy, in place of nobody | T4 |

*Table 2 — What the earlier records leave to M4.*

## 3 What M4 rests on that is not settled yet

- **The battle and relations values.** The valley run misses four of Playtest 1's six measurable lines (DEC-1008, O102), and the battle values wait for the owner's M2 evidence. The deputy and the away report are built on the rules as they stand; a change of those rules changes what the deputy meets, not how it decides.
- **The economy at the always-on pace.** Every value of the scope's table 3 is stated at the session pace, and its pace column converts it. Nothing has run at the always-on pace yet, so T2 shows the conversion on the growth scene before anything else reads it.
- **The transport has never run.** `QuicTransport` is new code that only CI's Windows runner and the owner's PC compile and run, so its tests run in CI on the loopback interface from the task that writes it (T7).

## 4 The order of M4

**The world that runs without the steward is built and run headless before it is connected.** The pace, presence, the deputy, the away report and the freeze records are simulation and server code, tested in CI on the in-process transport (phase A, T2 to T6). The transport, the handshake and the server process come next (phase B, T7 to T10), then the confirmations, a headless run of Playtest 2's measurable lines and the exit (phase C, T11 to T13). Each task is pushed with its run checklist (M0 plan §2) on one pull request of M4's own.

**Saved worlds across M4's state changes.** Every raise of `WORLD_STATE_VERSION` migrates version 10 and later (MVP-095), as M3's did, so the owner's M2 and M3 worlds continue on M4's build at the session pace.

## 5 Tasks

| Task | Builds | Done when | Ids |
|---|---|---|---|
| **T1 Plan** | this plan with the owner's answers (O109); the scope's amendments of O108 and O109 | the design checks pass | O108, O109, MVP-002 |
| **T2 The always-on pace** | a world's pace in its start, fixed for its life (PIL-104); the values the scope's table 3 marks `scaled` converted by `mvp.pace.session_factor` when a world of the always-on pace starts (PIL-101, PIL-103) and nowhere else | a lone settlement at the always-on pace reaches its Workers at nearly the factor times the session pace's ticks, below it by the walks and the gathering that take the same ticks at both paces (measured in [ADR-180](../ADR/ADR-180-the-always-on-pace.md)); hashes equal at 1, 2 and 8 worker threads; a world of version 17 or older continues at the session pace | MVP-070, MVP-071, PIL-100, PIL-101, PIL-104 |
| **T3 Presence** | the steward's presence per settlement (PIL-070): present while connected with input given within `mvp.presence.idle_minutes`, away otherwise, decided by the server from the connection and the client's input reports, entering the simulation between ticks and journaled (MVP-076); no away state at the session pace | unit tests of every transition; a journal with presence changes replays to the same hashes | MVP-072, PIL-070 |
| **T4 The deputy** | the deputy of a settlement whose steward is away (MVP-073, PIL-071, PIL-079, PIL-083): its commands through the command path as the settlement's commander; in every battle its settlement is a side of (O109) each Leader's formation sent to the objective; the turrets manned by the guards' share as the steward left it; nothing it never does; every action recorded with the standing order that permitted it (PIL-026) | Playtest 2's five scripted situations, run headless, give the deputy actions this plan's §6 predicts, each recorded with its order | MVP-073, PIL-043, PIL-071, PIL-079, PIL-083, PIL-026 |
| **T5 The away report** | each absence's record in the region (from the tick the steward went away: the Workers and stores then, the deputy's actions, the structures completed); the away report as a server message built from it, the statistics and the world record (MVP-074, PIL-073, PIL-075): battles and results, Workers and stores lost and gained, construction finished, grievances recorded, claims moved, freezes, every deputy action with its order; sent on a return whose absence has news (O109) and on request | unit tests: each item of the summary agrees with the server's statistics over an absence with battles, a claim and a freeze | MVP-074, MVP-098, PIL-073, PIL-075, PIL-131 |
| **T6 Freeze records** | the freezes of MVP-071 (PIL-131): a server that was not running between a clean stop and its next start, and a computer that slept, each with its cause, world time and length, beside the server stall and the unclean stop of ADR-137 | tests of each cause from a snapshot folder and a simulated suspend | MVP-071, MVP-075, PIL-076, PIL-131 |
| **T7 QuicTransport** | `QuicTransport` and its listener on MsQuic (ADR-088 items 1 to 4, ADR-089): one connection per client, the reliable channel on one bidirectional stream, the state channel on QUIC datagrams with their fates, the keep-alive and idle timeouts, TLS 1.3 with the server's self-signed certificate pinned by the client, ALPN `species-play`; MsQuic linked only by the transport | NeuronCore's tests open a loopback connection in CI: order, fates, close and statistics as `InProcessTransport`'s tests state them; a changed certificate refused | ADR-088, ADR-089, PLT-100 |
| **T8 The handshake** | the client's build, protocol, simulation and content identity at its connection (ADR-088 item 6, ADR-057, ADR-060), on both transports, and the refusal with its reason, which the client shows | a client of another build or with an edited content file is refused with its reason | ADR-088, ADR-057, ADR-060 |
| **T9 The headless server of an always-on world** | `SpeciesServer.exe` running one always-on world from its folder in a console window of its own, which stops it when closed (O109), listening on the loopback interface and naming its port and certificate in the folder; the client starting it when it is not running and connecting over `QuicTransport`, leaving it running when the client leaves or closes; one always-on world at a time (O109) | the owner runs it: starts an always-on world, closes the client, reopens it and continues the running world, then closes the server's console and continues the world again, with the freeze recorded | MVP-071, O54, O109, ADR-083, ADR-100 |
| **T10 The pace, the presence and the away report in the client** | the New world page's pace (O80); the world panel's pace and presence; the away report's page, which opens first on a return with news, its summary on one page and the deputy's actions one step away, and its entry on the HUD (MVP-102); `icon.hud.deputy` with its stand-in until the owner draws it | the owner runs it: is away while an AI settlement attacks, comes back to the report, and reads its summary within 2 minutes | MVP-072, MVP-074, MVP-102, MVP-103, PIL-073 |
| **T11 The transport's confirmations** | the link simulator under `Tools/` (ADR-088 item 7); the fuzzer of every message parser (ADR-069); the overhead per state datagram from MsQuic's statistics; the 30-minute run at 10 % loss and a 150 ms round trip; `msquic.dll` and its licence notice in the install's list (ADR-071) | table 1's ADR-088, ADR-089, ADR-069 and ADR-071 rows are confirmed or amended with their evidence in `playtests/M4.md` | ADR-069, ADR-071, ADR-088, ADR-089 |
| **T12 The away run** | a headless run of an always-on world in which the steward's settlement is played by the scripted steward while present and left to the deputy while away, by the typical and the check-in patterns of PIL-102, reporting Playtest 2's measurable lines (the strength regained after an absence with attacks; growth per world day present against away) | the away run meets Playtest 2's measurable pass lines on a corpus of seeds, or the values it needs go to the owner | PIL-022, PIL-023, MVP-003 |
| **T13 Exit** | Playtest 2 (§6); the M4 record | the exit criterion of the scope's §3 holds, with Playtest 2's record in `playtests/` | MVP-002 |

*Table 3 — The tasks of M4.*

## 6 Playtest 2

Playtest 2 is the scope's §6.2: one always-on run of 8 world days, which at the always-on pace is 8 real days with the computer left running, at the typical pattern of PIL-102, with the session log's three lines after every session. It follows Playtest 1, because the scope plays Playtest 1's runs at the session pace on M3's build and M4 changes no rule of them.

The scripted situations of its second line, and the deputy's actions this plan expects in each (MVP-073 as amended by O109):

| Situation | The deputy's action |
|---|---|
| An attack declared on the settlement | none during the warning; when the battle starts, each Leader's formation to the objective (defend the settlement), and the guards' share at the turrets as the steward left it |
| A claim battle at a border area the settlement holds | each Leader's formation to the objective (defend the settlement) |
| The stores below the next queued cost | none: the construction waits for the stores (finish the queued construction) |
| The population at the limit with a dwelling queued | none: the dwelling is built by the Workers whose priority is construction (finish the queued construction) |
| A respite ending while a neighbor's anger is above its threshold | none: the deputy never attacks |

*Table 4 — Playtest 2's scripted situations and the deputy's expected actions.*

The headless away run of T12 predicts the measurable lines before the owner starts, so that the owner's eight days test what a script cannot: whether the report is quick to read, whether the deputy is predictable, and whether coming back is fun.

## 7 The owner's answers

The owner answered this plan's questions on 2026-10-24 (O108, O109); the decisions record the options that were offered.

1. **When M4 is built.** All of it now, before Playtest 1 and the exits of M1 to M3 (O108).
2. **How an always-on world stops.** Its server runs in a console window of its own, and closing that window stops it: the world freezes with its final snapshot, and the freeze is recorded. The client leaving the world or closing keeps it running.
3. **The battles the deputy commands.** Every battle its settlement is a side of: the defence of its territory, the defence of a border half it holds, and the steward's own expedition when one is out. It sends each Leader's formation to the objective and leaves the turrets to the guards; it never declares, withdraws, claims, promotes or demotes.
4. **When the away report opens.** By itself on a return whose absence has news (a battle, a deputy action, a freeze, a grievance, a claim moved or a structure finished); an absence with none opens nothing, and the latest report is one press away on the HUD.
5. **How many always-on worlds run.** One at a time: starting or continuing another stops the one that runs first, with a clean freeze recorded.

## 8 Risks

- **Rework after Playtest 1.** M4 is built before the first playtest answers whether the game is fun (O108). A redesign of the settlement, the relations or the battles changes what the deputy meets and what the away report counts; the deputy's rule and the report's form are kept small for that reason.
- **The transport is compiled only in CI.** The cloud session writes it and cannot run it, so its tests run on CI's Windows runner on the loopback interface, and its first run on the owner's PC is T9's run checklist.
- **A background process on the owner's PC.** An always-on world keeps a process running for 8 days. Its log, its snapshots and its console are the only view of it, so a fault there is found only when the steward returns; the freeze records and the away report are the evidence.
- **The certificate store.** The server's self-signed certificate lives in the user's certificate store and its key in the user's key store, created at the first start; a failure there stops the server with its reason, before the world opens.
- **Playtest 2's cost.** Eight real days of the owner's computer and attention. The away run of T12 is retuned before the owner starts, not during.
