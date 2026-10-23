---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T15)
---

# The first sound set played at M3

## Context and Problem Statement

M3 task T15's third step plays the sounds of [ADR-171](ADR-171-the-first-sound-set-synthesized-and-cooked-at-m3.md) through the engine of [ADR-172](ADR-172-the-audio-engine-at-the-mvps-size.md). MVP-087 names seven events: laser shots, deaths, an orb becoming a Worker, a structure completed, the attack warning, and a battle starting and ending. The M3 plan also asks for the master volume that M2 stores to be heard.

These decisions are open:
- where the client learns of each event. Six have a presentation event or a notice, but nothing tells the client that an orb became a Worker;
- when a sound plays relative to its draw;
- the listener;
- where the engine's tuning and the events' levels live;
- how the master volume reaches the engine.

## Decision Drivers

* AEN-003: the audio engine never reads simulation or presentation objects. The game tells it what to play.
* ADR-084 item 5 and ADR-123: an event of tick t is drawn as the display time reaches t − 1. A sound heard before or after its flash would read as lag.
* AEN-209 and DEC-566: the listener is the camera.
* MVP-087 names the set's groups but not their levels, buses or distances (DEC-1009).
* The seen layer narrows a steward's events to what its settlements see (ADR-158), and a sound must not tell more than the picture.

## Considered Options

For the orb becoming a Worker:
1. **A presentation event**, `OrbBecameWorker`, emitted where the orb's Worker appears.
2. **The client infers it** from the replica: an orb gone near its Wellspring and a new Worker at the door.

For when an event's sound plays:
3. **With its draw**, on the display clock.
4. **On arrival**, the frame the event is received.

## Decision Outcome

Chosen options: 1 and 3.
- Option 2 guesses from two replicated changes that need not arrive in the same tick, and a Wellspring that grows a Worker from its stores looks the same.
- Option 4 plays a sound up to a tick and the interpolation before its flash.

What is built:

* **The event.** `EffectEventKind::OrbBecameWorker` (24) is emitted when an orb becomes a Worker at its Wellspring's door, with the Worker as its subject and position (MVP-025). The encoding's kind field keeps its five bits, so `PresentationEvents` stays version 1. The event is public and narrowed by the seen layer like every other (ADR-158). It changes no state, so the state hash and the journal are unchanged.
* **The bindings** (`GameClient/WorldSounds`). `GameSound` lists the seven events in the engine's table order. `GameSoundEvents` reads each group's members from the mounted content: the resident sounds keyed by the group's stem alone, or by the stem, an underscore and a number, in key order. A group with no member is silent (AEN-182). Each event gets:
  - its bus and whether it is positioned, as DEC-1009 states;
  - its level, `mvp.sound.<event>.level_db`;
  - the minimum distance, `mvp.sound.min_distance`;
  - its cap of real instances, `aen.voice.event_cap.world_max` for the laser shot and the death, `world_default` for the other world sounds, and the announcer bus's cap for the rest (AEN-143).
* **What plays when.**
  - `LaserFired`, `FigureDied`, `OrbBecameWorker` and `StructureCompleted` play at the event's position. Each waits in `SoundSchedule` until the display time reaches its tick less one, as its draw does.
  - The notices `AttackDeclared`, `BattleStarted` and `BattleEnded` play two-dimensional as the HUD takes them. `SettlementHud::Arrived` keeps the notices of its latest update. Only the stewards they concern receive them (ADR-165).
  - A structure's completion plays from its presentation event, never from its notice, so it plays once, and for every structure the steward sees.
  - Leaving a world drops the sounds still waiting.
* **The listener.** Each frame with a world, `ListenerOf` sets it from the camera's pose before the camera shake: its position, its view direction, and the up vector that turns with the pitch about the view's right.
* **The tuning.** The presentation record holds the engine's `aen.*` keys and the audio design's `ref.aud.*` keys that ADR-172 reads, `ref.sim.frame.audio_update_period`, and the provisional `mvp.sound.*` (DEC-1009). `AudioSettingsOf` and `AudioDeviceDescOf` turn them into the engine's settings and the device's pools.
* **The composition root.**
  - `Species.exe` makes the engine after the records load. It runs on its own `CounterTimeSource` (AEN-006) and on XAudio2's default device, or silently with none.
  - The engine starts at `audio.master_volume` and follows the setting through a subscription to the settings store, so a change on the options page is heard at once (AEN-076, O96).
  - The frame bench makes the same engine at the full master volume, so its scenes pay for the sounds as the game does.

### Consequences

* Good, because each of MVP-087's sounds plays with what the player sees. A sound off screen within the seen layer is heard at its distance and pan, and nothing outside it is heard.
* Good, because the mix is data: the owner's run retunes keys, not code.
* Bad, because `TurretTargetSighted`, which ADR-141 left for M3's audio, plays nothing. No group for it is in the set (DEC-1009).
* Bad, because a sound follows its notice's arrival on the reliable channel. A battle's start can be heard a little before its first shots are seen.
* It forecloses nothing: an eighth event is a row in `BINDINGS`, a key and a group.

### Confirmation

* `GameClientTests/WorldSoundsTests`:
  - each event's members, read in key order from a mounted content folder, with another group's key excluded and a group with none silent;
  - each event's bus, position, level, distance and cap;
  - the effect events and notices that play, and those that do not;
  - a sound due with its draw, and none left after leaving the world;
  - the listener's front, up and right, also looking down at an angle;
  - the engine's settings and the device's pools from the committed presentation record.
* The GameLogic and GameShared suites pass with the new event emitted.
* CI's render check starts `Species.exe` with the engine on whatever output device the runner has.
* The owner's T15 run: each sound on its event, at its bus's volume, and the master volume heard (the M3 plan's T15 row).

## Pros and Cons of the Options

### 1 — a presentation event

* Good, because the rule that makes the Worker says so, at the tick and place it happens.
* Bad, because it adds a kind to the presentation events.

### 2 — inferred by the client

* Good, because the simulation is unchanged.
* Bad, because it guesses, and mistakes a Wellspring's own growth for an orb.

### 3 — with the draw

* Good, because sound and picture agree.
* Bad, because a sound waits up to a tick and the interpolation.

### 4 — on arrival

* Good, because it is the earliest.
* Bad, because it plays before what it belongs to is seen.

## More Information

* Amends [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md) (a presentation event's kind added within its five bits), [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md) (an orb that becomes a Worker raises `OrbBecameWorker`) and [ADR-141](ADR-141-the-turrets-shots-drawn-at-m2.md) (`TurretTargetSighted` is not heard at M3, DEC-1009).
* Owner decisions O96 and O98; MVP-025, MVP-042, MVP-043, MVP-045, MVP-087; AEN-003, AEN-006, AEN-064, AEN-076, AEN-143, AEN-182, AEN-209; DEC-566, DEC-1009.
* The code:
  - `GameShared/EffectEvent`, `GameLogic/EssenceOrb`;
  - `GameClient/WorldSounds`, `SettlementHud`, `WorldPresentation`;
  - `Species/Species.cpp`, `Tools/SpeciesFrameBench`;
  - `GameData/Records/tuning/presentation.json` and its schema;
  - `GameClientTests/WorldSoundsTests`.
