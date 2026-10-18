---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Virtual voices, priorities and voice stealing

## Context and Problem Statement

A Species battle triggers hundreds of sounds at once: every Worker's shot and death, every impact, every building's loop, the ambience beds and the UI. Only a bounded number of source voices should be mixed, and the ones that play must be the ones the player most needs to hear: nearby, loud, gameplay-relevant sounds, and never a flood of identical copies. Loops that fall out of the mix and come back must resume in time, not restart, and a loop near the boundary must not flicker in and out. A one-shot that cannot play at once may still be worth playing a moment later. Starting a sound must not be delayed by a fixed per-update quota.

The problem is how sound instances are mapped onto a bounded set of real voices, and how they are ranked and replaced.

## Decision Drivers

- A bounded real-voice count, with no limit on how many sounds the game may trigger.
- Ranking that combines authored importance with what the player actually hears.
- Fewer copies of the same event, louder copies first.
- Stable behaviour near the boundary: no ping-pong between real and virtual.
- No audible clicks when a voice is taken over.
- Start latency independent of a per-update start quota.

## Considered Options

1. **Virtual voices**: every instance is virtual until it ranks within the real-voice budget; ranking by authored priority times audibility with a per-event copy decay; stealing only with a margin.
2. A **hard cap on real voices**, rejecting new sounds when it is reached.
3. A hard cap on real voices, **stealing the oldest** voice when it is reached.

## Decision Outcome

Chosen option: **1, virtual voices with ranked, margin-protected stealing**, because it is the only option that keeps the most important sounds audible under any load while staying stable at the boundary.

- **Instances.** Every trigger creates a sound instance. An instance is **virtual** (tracked, not mixed) until it ranks within the real-voice budget, and then becomes **real** (bound to a source voice). A virtual instance keeps advancing its play position, so a loop that becomes real again resumes in time.
- **Budgets.** The audio engine component sets the total real-voice budget, a cap per event and a cap per bus. Source voices are pooled by format (channel count, sample rate, encoding), because a source voice's format is fixed when it is created.
- **Ranking.** Each update, every instance gets a score: its event's **authored priority** times its **audibility** (its volume after distance attenuation, ADR-027, and bus gain, ADR-024). Copies of the same event are ranked by loudness, and each further copy's score is multiplied by the event's **copy decay** factor (one multiplication per louder copy). The copy decay is an authored design value of each event; its default belongs to the audio design (0.75 per louder copy). The audio engine owns the ranking arithmetic that applies it.
- **Stealing.** When a virtual instance outranks the lowest-ranked real instance, it takes that instance's voice only if its score exceeds the victim's by the **steal margin**. The victim fades out over a short ramp before its voice is reused. Real instances whose audibility falls to zero become virtual at once.
- **One-shots.** A one-shot that cannot become real waits up to its event's **patience window**, an authored design value of each event whose default belongs to the audio design; if it is still virtual at the end of the window, or its sound has ended, it is retired.
- **Starts.** Triggers carry their trigger time and start at the next audio update with an offset that preserves the trigger order and spacing. There is no fixed per-update start quota; an optional cap on starts per update exists as a tuning value, off by default.
- **Gain changes.** Every volume and pitch change on a real voice is ramped over the update interval, never stepped.

### Consequences

- Good, because the game can trigger as many sounds as it likes; the budget only decides what is mixed.
- Good, because important and nearby sounds win, and floods of one event thin out naturally.
- Good, because loops survive leaving and re-entering the mix in time, and the margin stops flicker.
- Bad, because ranking every instance each update costs CPU time proportional to the number of live instances (bounded by per-event caps and the patience window).
- Bad, because the authored priorities, decays, margins and windows are more tuning values to balance.

### Confirmation

- A unit test drives the ranking with recorded battle trigger streams and checks: the real set never exceeds the budget or any cap; no instance changes between real and virtual more than once within the margin's hold time; a loop that returns resumes at the expected position.
- A unit test checks the copy decay order (louder copies rank first) and the retirement of one-shots at the end of their patience window.
- An audio capture test checks that stolen voices fade out without discontinuities.

## Pros and Cons of the Options

### 1. Virtual voices with ranked, margin-protected stealing

- Good, because audibility and authored importance both count.
- Good, because it is stable under load.
- Bad, because it adds ranking work and tuning values.

### 2. Hard cap, reject when full

- Good, because it is simple and cheap.
- Bad, because a nearby, important sound is lost whenever distant, unimportant sounds filled the cap first.

### 3. Hard cap, steal the oldest

- Good, because new sounds always play.
- Bad, because long, important loops are the oldest and are cut first, and floods of one event push everything else out.

## More Information

- Related: ADR-022 (audio engine), ADR-024 (bus gains), ADR-025 (formats and voice pools), ADR-026 (update and command timing), ADR-027 (distance attenuation).
- The audio engine component (`architecture/components/audio_engine/README.md`) owns the budget, the caps, the steal margin and the ranking arithmetic as tuning values; the audio design (`reference/10_audio_design/`) owns the authored priorities, copy decays and patience windows per event, with their defaults.
- **Amendment 2026-10-17.** The copy decay and the patience window are design values per event with defaults owned by the audio design (`ref.aud.rank.copy_decay`, `ref.aud.layer.one_shot_patience_updates`); the engine defines no copy-decay key of its own (AEN-063).
