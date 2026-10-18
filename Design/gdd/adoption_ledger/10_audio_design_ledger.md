# Adoption ledger: 10 audio design

> Status: Draft · Owner: area C4 · Reference: [10_audio_design](../../reference/10_audio_design/README.md) · Rules: `prompts/species-design-phase.md` §5.3

41 sections, 165 requirements. A section row covers every requirement under its heading; an exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the Species requirements that replace it), `drop` (gives the reason).

| Scope | Verdict | Species requirements | Note |
|---|---|---|---|
| `10_audio_design/README.md §2.1` | pending | | Conventions and units: 6 requirements (R-AUD-001 to R-AUD-006) |
| `10_audio_design/event_catalogue.md §2.1` | pending | | Sound sources: 6 requirements (R-AUD-120 to R-AUD-125) |
| `10_audio_design/event_catalogue.md §2.2` | pending | | Units: 4 requirements (R-AUD-126 to R-AUD-129) |
| `10_audio_design/event_catalogue.md §2.3` | pending | | Creatures: 1 requirement (R-AUD-130) |
| `10_audio_design/event_catalogue.md §2.4` | pending | | Weapons: 1 requirement (R-AUD-131) |
| `10_audio_design/event_catalogue.md §2.5` | pending | | Structures: 6 requirements (R-AUD-132 to R-AUD-137) |
| `10_audio_design/event_catalogue.md §2.6` | pending | | Supply crates and outcomes: 2 requirements (R-AUD-138 to R-AUD-139) |
| `10_audio_design/event_catalogue.md §2.7` | pending | | Interface and ambience: 1 requirement (R-AUD-140) |
| `10_audio_design/event_catalogue.md §2.8` | pending | | Events with no binding: 4 requirements (R-AUD-141 to R-AUD-144) |
| `10_audio_design/event_catalogue.md §2.9` | pending | | Kinds with no sound definitions: 2 requirements (R-AUD-145 to R-AUD-146) |
| `10_audio_design/event_model_and_sound_data.md §2.1` | pending | | Pipeline and trigger entry points: 2 requirements (R-AUD-020 to R-AUD-021) |
| `10_audio_design/event_model_and_sound_data.md §2.2` | pending | | Matching and layering: 8 requirements (R-AUD-022 to R-AUD-029) |
| `10_audio_design/event_model_and_sound_data.md §2.3` | pending | | Event ids built at run time: 6 requirements (R-AUD-030 to R-AUD-035) |
| `10_audio_design/event_model_and_sound_data.md §2.4` | pending | | Stopping, querying and loop control: 9 requirements (R-AUD-036 to R-AUD-044) |
| `10_audio_design/event_model_and_sound_data.md §2.5` | pending | | From a sample group to sound: 4 requirements (R-AUD-045 to R-AUD-048) |
| `10_audio_design/event_model_and_sound_data.md §2.6` | pending | | The sound data of a layer: 2 requirements (R-AUD-049 to R-AUD-050) |
| `10_audio_design/event_model_and_sound_data.md §2.7` | pending | | Modulatable values: 5 requirements (R-AUD-051 to R-AUD-055) |
| `10_audio_design/event_model_and_sound_data.md §2.8` | pending | | Envelopes and loops: 7 requirements (R-AUD-056 to R-AUD-062) |
| `10_audio_design/event_model_and_sound_data.md §2.9` | pending | | Sample groups: 6 requirements (R-AUD-063 to R-AUD-068) |
| `10_audio_design/event_model_and_sound_data.md §2.10` | pending | | Authoring conventions: 7 requirements (R-AUD-069 to R-AUD-075) |
| `10_audio_design/event_model_and_sound_data.md §2.11` | pending | | Filters and effects: 7 requirements (R-AUD-076 to R-AUD-082) |
| `10_audio_design/front_end_hud_voice_and_ambience.md §2.1` | pending | | Menu, HUD and interface sounds: 3 requirements (R-AUD-300 to R-AUD-302) |
| `10_audio_design/front_end_hud_voice_and_ambience.md §2.2` | pending | | Countdown, match clock and match end: 6 requirements (R-AUD-303 to R-AUD-308) |
| `10_audio_design/front_end_hud_voice_and_ambience.md §2.3` | pending | | Objective and outcome stings: 3 requirements (R-AUD-309 to R-AUD-311) |
| `10_audio_design/front_end_hud_voice_and_ambience.md §2.4` | pending | | Voice: 3 requirements (R-AUD-312 to R-AUD-314) |
| `10_audio_design/front_end_hud_voice_and_ambience.md §2.5` | pending | | Voice behaviour: 5 requirements (R-AUD-315 to R-AUD-319) |
| `10_audio_design/front_end_hud_voice_and_ambience.md §2.6` | pending | | Ambience beds: 7 requirements (R-AUD-320 to R-AUD-326) |
| `10_audio_design/music_hooks_and_sound_design_brief.md §2.1` | pending | | Music hooks: 7 requirements (R-AUD-400 to R-AUD-406) |
| `10_audio_design/music_hooks_and_sound_design_brief.md §2.2` | pending | | Music design brief (new): 4 requirements (R-AUD-407 to R-AUD-409) |
| `10_audio_design/music_hooks_and_sound_design_brief.md §2.3` | pending | | Sound-design brief: 4 requirements (R-AUD-410 to R-AUD-413) |
| `10_audio_design/positional_priorities_and_mix.md §2.1` | pending | | Listener: 2 requirements (R-AUD-200 to R-AUD-201) |
| `10_audio_design/positional_priorities_and_mix.md §2.2` | pending | | Distance attenuation: 3 requirements (R-AUD-202 to R-AUD-204) |
| `10_audio_design/positional_priorities_and_mix.md §2.3` | pending | | Panning: 2 requirements (R-AUD-205 to R-AUD-206) |
| `10_audio_design/positional_priorities_and_mix.md §2.4` | pending | | Position kinds and sound lifetime: 3 requirements (R-AUD-207 to R-AUD-209) |
| `10_audio_design/positional_priorities_and_mix.md §2.5` | pending | | Height and altitude: 1 requirement (R-AUD-210) |
| `10_audio_design/positional_priorities_and_mix.md §2.6` | pending | | Game-speed pitch bend: 2 requirements (R-AUD-211 to R-AUD-212) |
| `10_audio_design/positional_priorities_and_mix.md §2.7` | pending | | Crowd sounds: 2 requirements (R-AUD-213 to R-AUD-214) |
| `10_audio_design/positional_priorities_and_mix.md §2.8` | pending | | Priorities and voices: 5 requirements (R-AUD-215 to R-AUD-220) |
| `10_audio_design/positional_priorities_and_mix.md §2.9` | pending | | Buses and routing: 3 requirements (R-AUD-221 to R-AUD-223) |
| `10_audio_design/positional_priorities_and_mix.md §2.10` | pending | | Volume law and the master volume: 3 requirements (R-AUD-224 to R-AUD-226) |
| `10_audio_design/positional_priorities_and_mix.md §2.11` | pending | | The reference mix: 1 requirement (R-AUD-227) |
