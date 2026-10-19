# Deferred decisions

> Status: Decision register, deferred to the design phase · Related: [Decision register](../README.md) · [Vision brief](../../../vision/species-vision-brief.md)

These decisions belong to the Species world game rather than to the reference mechanics, so the Species design phase takes them. Every entry was written Open; design area A2 resolved DEC-200 to DEC-208, DEC-221 to DEC-228 and DEC-240 to DEC-242 on 2026-10-19, after owner gate B, and the others stay Open for the areas that the decision triage names. An entry's specified behaviour is what the library stated when it was written, and each entry lists the reference behaviour, the constraints and lessons any Species answer must respect, and the figures that size the design.

| File | Ids | Topics |
|---|---|---|
| [networking.md](networking.md) | DEC-200 to DEC-219 | replication and authority, bandwidth and population budget, command validation, input timing and latency, content and version identity, join, reconnect and session continuity, divergence detection and integrity checking, transport and message format, network diagnostics and test tooling |
| [process_tick_and_roadmap.md](process_tick_and_roadmap.md) | DEC-220 to DEC-239 | process roles and the headless server build, the composition root and application state machine, the simulation and presentation boundary, state ownership and reset, threading, pacing and catch-up, loading and streaming, the tick rate, the per-tick CPU budget and population caps, build order and milestones |
| [numerics_lobby_terrain_services.md](numerics_lobby_terrain_services.md) | DEC-240 to DEC-259 | the numeric model and random generator, lobby authority and match setup, session join, discovery and services, the terrain runtime and large-world terrain, player identity and entitlement, persisted data and world-state persistence |
