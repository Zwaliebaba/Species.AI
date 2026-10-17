# Species — vision brief

> Status: owner-approved input to the design phase · Date: 2026-10-17 · Owner: product and technology lead
> This brief states intent and the decisions already taken. The design phase (`prompts/species-design-phase.md`) turns it into the Species game design and architecture. It challenges every part of the brief and recommends improvements, and owner gates decide the changes.

## 1 Pitch

Species is a persistent, shared world of settlements. Each settlement belongs to a race and is run either by a live player or by AI. Settlements live: their crowds of inhabitants gather resources, build up their part of the world and grow. When a race is angered it attacks its neighbours; when it is attacked it defends. A player controls one settlement and gives it orders. When the player is away from the keyboard (AFK), a **deputy AI** acts on the player's standing orders, and the world keeps running on the server. Players check in periodically to see what happened, adjust orders and take direct control when they want to.

Battles are real-time crowd battles: hundreds of small autonomous figures, leaders and formations, rendered in the flat-shaded, glowing visual language of the reference library.

## 2 Pillars (proposed; the design phase confirms or changes them at its first owner gate)

1. **Living crowds.** Inhabitants act on their own. The player steers through leaders, orders and priorities, never by micromanaging every figure.
2. **A world that keeps running.** The server owns the world. Being AFK is a normal way to play, and it is safe because the deputy defends and protection rules apply.
3. **Conflict with a cause.** Races grow angry for reasons the player can see, such as territory pressure, competition for resources and past attacks. Attacks feel motivated, never random.
4. **Delegation you can trust.** The deputy follows standing orders that the player understands and can predict.
5. **The reference look and feel.** The visual and audio language of the reference library stays as it is.

## 3 Decisions taken (owner, 2026-10-17; O12–O13 added at the conversion gate G1, O14–O20 during the conversion Phase 3, O21–O24 when the code standards (`AGENTS.md`) were adopted)

| # | Topic | Decision | Consequence the design must carry |
|---|---|---|---|
| O1 | Scale | Start with **2–16 human players** per world plus AI settlements, still playable solo. If Species succeeds, grow to **50–200+ per world**. | The architecture must allow growth without a rewrite: a region-partitioned world, interest-managed replication from day one, and world state in a database rather than a single save file. |
| O2 | AFK attacks | A settlement **can be attacked while its owner is AFK**. The deputy defends with the same simulation as a live battle. | Protection rules are mandatory: a cap on what a settlement can lose while AFK, a shield period after an attack, a cost to attacking, and a defender's advantage. |
| O3 | Simulation fidelity | **Battles always run as the full crowd simulation.** The day-to-day economy of settlements nobody is watching runs on an **abstract model**. | The abstract model needs a stated accuracy and is calibrated against the full simulation. The switch between the two must not create exploits. |
| O4 | Downtime | **The world freezes** while its server is not running: the host has closed the client (integrated stage) or a dedicated server is down for maintenance. | No catch-up simulation. World time is server uptime. |
| O5 | Hosting | **Community-hosted dedicated servers first; official hosting later**, for the 50–200+ tier only, once Species has proven itself. | A headless server build that community operators can run; official hosting is a later stage, not a launch dependency. |
| O6 | Check-in | **The game client only in v1.** The server exposes an authenticated interface for settlement status and deputy orders, so a web or mobile companion can be added later without server changes. | Settlement status and orders are a server API, not client-only logic. |
| O7 | Races | **Symmetric** in v1: one rule set; races differ in looks, colour and AI personality. Asymmetric traits come later. | The data model must allow per-race modifiers later. |
| O8 | Skirmish | **Keep a skirmish mode**: a short, match-based crowd battle on a small map, separate from the persistent world. One mode at first. | It is the first playable and the combat balance test bed, and it runs on the same netcode as the world. |
| O9 | Look and feel | The look and feel of the reference library **does not change**. | Keep the style: flat-shaded low-poly, additive glow on black, the darkening layer, crowds of small figures. All art is newly authored, with new character silhouettes, textures, UI layout and logo, so Species is not a copy. A strategic zoom level for the big world is designed in the same style. |
| O10 | Server path | The server starts **integrated in the client** and moves to **dedicated servers** over time. | One server codebase. The integrated server runs in-process behind the same transport interface as a dedicated one, and the client never touches server state directly. |
| O11 | Research and training | Designed **later**. | The stat and modifier system, and the unit-specialisation hooks, are designed now, so research and training plug in without rework. |
| O12 | Vocabulary | Name collisions resolved at the reference-library gate G1 (2026-10-17): **Command Console, Convert Turret, Transit Gate, Essence Grove, energy grid, Beam Dish, Solar Array, Relic Spawn Zone**. | The Species vocabulary in `design/CONVENTIONS.md` §8 is authoritative. |
| O13 | Reference tuning values | The reference tuning values **may be adopted as Species starting values** (G1, 2026-10-17). | A legal check is required before commercial release. The design phase retunes freely; adopted values need no "provisional" flag for reasons of origin. |
| O14 | Population cap and player slots | Every open slot is played: empty slots are filled with bots by default, leavers are taken over by bots, and the host may close slots; the per-team cap is the map cap ÷ open slots (DEC-079). | Skirmish lobbies need slot open/close and bot fill. For the world game, the design phase decides how this maps to settlements. |
| O15 | Collector orbs | Collectors gather any team's Essence orbs; delivery respects the population cap (DEC-080). | — |
| O16 | Catch-up bonus | Proportional catch-up curve, with Rogue and Visitor factions excluded (DEC-081). | — |
| O17 | Mode second | 11 ticks for the first playable; A/B playtest of 10 vs 11 ticks before launch (DEC-003). | — |
| O18 | Hostile crate outcomes | Hazards team-blind; every outcome respects the map's exclusion list; Brood ownership decided in playtest (DEC-107, DEC-121). | — |
| O19 | Map values | Absent = lobby default, 0 = none, unlimited is explicit (DEC-120). | Typed map schema. |
| O20 | First playable | Spawn Control, 2 players, core Supply crate pool. | Matches the GameAssets.md priorities. |
| O21 | Build rules and dependencies | **`AGENTS.md` governs how code is written and built**: MSBuild solution (`.slnx`), toolset v145, `/std:c++latest`, Debug and Release only, x64 only, no CMake, no package manager beyond NuGet restore, and no third-party code except a **Microsoft allow-list: the DirectX Agility SDK, the GameInput redistributable and WinPixEventRuntime**. Shaders are compiled by `FXCompile` with the Windows SDK's compiler into byte arrays inside the executable. | The engine ADRs and components follow it: no vcpkg, no CMake presets, no build flavours beyond Debug and Release, no third-party libraries in the executable (compression, codecs, test framework, texture tools); the dedicated server is a Windows build until a later ADR decides another host. |
| O22 | ADR home | Architecture decision records live in **`Design/ADR/` as `ADR-NNN-<slug>.md`** (three-digit ids, MADR 4 content). The design library becomes **`Design/`** at the cutover. | The baseline ADRs keep their numbers (ADR-0050 becomes ADR-050); new ADRs continue from the highest id. |
| O23 | Code naming in the design | The C++ sketches, type, file and macro names in the design follow the `AGENTS.md` naming rules now: PascalCase types, functions, namespaces and files, `_camelCase` parameters, `m_` members, `UPPER_CASE` constants, American spelling in identifiers, namespace `Engine` for engine code and `Species` for game code. Prose stays British English. | Every component sketch is rewritten; CONVENTIONS §8 follows `AGENTS.md`. |
| O24 | AGENTS.md scope | `AGENTS.md` states the repository's current status and which document decides what. **R18 onwards stays reserved** for conformance rules that the Species design phase derives from the Species design. | The design phase proposes R18+ rules (candidates: presentation never writes simulation state; the determinism rules of the simulation core; tuning values in data, never in code). |

## 4 What carries over from the reference library

The reference library (`design/reference/` and `design/architecture/components/`, produced by the conversion run) is the authority for mechanics, behaviour and look and feel. The design phase **adopts, adapts or drops** each reference requirement by ID. Strong candidates:

- the crowd simulation: Workers, Leaders and formations, autonomous combat, creatures and hazards;
- the population-flow idea: deaths yield Essence orbs, and essence drives growth, which here becomes settlement growth;
- buildings and defences, power effects and weapons; supply crates may become world events;
- the bot architecture, as the starting point for settlement AI and the deputy (it must work under fog of war);
- the renderer, terrain, audio, UI framework, asset pipeline and platform layer.

Match-only content (the six match modes, per-match population caps, arena maps, per-mode bot strategies, the lobby and match flow) stays in the reference for the skirmish mode, and is not the world game.

## 5 Constraints

- Windows client; C++23; Direct3D 12; the technology baseline in the conversion run's ADRs.
- The server is **authoritative**. Clients send commands and receive the state they are allowed to see. Lockstep is not the world netcode.
- The server builds headless from day one. The same code runs in-process (integrated) and stand-alone (dedicated).
- The world is partitioned into regions from day one, so regions can later move to separate server processes.
- AI settlements and the deputy play under the same information rules as human players (fog of war).
- Every asset is newly authored. Nothing is traced, re-coloured or edited from the reference game's art, audio or text.

## 6 Challenges the design phase must resolve (seed list)

1. Griefing and AFK safety: protection rules that keep AFK play safe without making the world static.
2. Snowballing: stopping large settlements from becoming unassailable (diminishing returns, upkeep, defender's advantage, optional seasons).
3. Abstract-model drift: how the abstract economy model stays close to the full simulation, and how the hand-over avoids exploits.
4. Server cost per settlement and per battle at 16 and at 200 players, with explicit population and battle-size limits.
5. Replication of crowds: bandwidth for hundreds of units in view, and compression and interest management.
6. Big-world technology: coordinate precision far from the origin, terrain streaming, pathfinding at world scale.
7. Persistence across patches: world-save schema migration, backups and rollback.
8. Deputy trust: an order language players understand; what the deputy may never do; reporting what it did.
9. Anger legibility: a relations model players can read and influence.
10. Community hosting: server requirements, operating system (a Linux dedicated-server build is recommended, because most community hosts run Linux), configuration, moderation and identity.
11. The look-and-feel line: the same style, a distinct game.
12. The path to 50–200+: what must be true at 16 players so that 200 does not need a rewrite.

## 7 Non-goals for v1

A web or mobile companion app; asymmetric races; research and training content; official hosting; a client on any platform other than Windows; modding support (recommended for later, because community servers will ask for it).
