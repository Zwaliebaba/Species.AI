---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T15)
---

# The first sound set, synthesized and cooked, at M3

## Context and Problem Statement

M3 task T15 brings the audio engine and the first sound set of MVP-087: laser shots, deaths, an orb becoming a Worker, a structure completed, the attack warning, and a battle starting and ending. These are seven sample groups and 25 members in the asset manifest. The owner chose to synthesize the whole set, the Worker death group included, and a recorded group may replace it later with no change of code (O98). No audio file exists under `GameData/`, and the content build has no audio cooker. Every one of the 25 rows names `synthesised tone` as its placeholder.

This first step of T15 makes the sounds and cooks them. The engine that plays them is the second step, and the game's bindings the third.

These decisions are open:
- where the synthesized sounds come from: files in the repository, or the content build;
- what a synthesized sound is;
- the runtime layout of a resident sound (`ContentType::ResidentSound`);
- how the MVP's sounds are banked, since its persistent world has no match to load a match bank with (AEN-074).

## Decision Drivers

* AST-082 already says what stands in for a row with no authored file: its placeholder, at the row's key and in its runtime format, replaced by an authored file with no other change. AST-083 says a placeholder passes every check an authored file must, and AST-084 says it is a pure function of its row.
* AST-079 and AST-080 fix the sources and the resident format: WAV at 48 kHz, 24-bit integer or 32-bit float PCM, mono for a positioned sound; cooked to 16-bit PCM at 48 kHz, rounded with no dither.
* The texture stand-ins of the MVP are made by the content build (ADR-140), and the mesh kit's committed output is checked by CI (ADR-128).
* Table 4.7-A's synthesised tone is "a sine or noise burst of the row's duration": a test signal, not a laser or a war horn.

## Considered Options

For where the sounds come from:
1. **The content build synthesizes each stand-in**, by the row's placeholder rule, and nothing is committed.
2. **A sound kit tool** writes WAV sources into `GameData/Audio/`, committed, and CI checks that it writes them again, as the mesh kit does.

For what a synthesized sound is:
3. **A recipe per group's role**: oscillators, frequency sweeps, bell partials, brass harmonics and filtered noise.
4. **Table 4.7-A's sine or noise burst**.

For the banks:
5. **Every resident member in the interface bank**, loaded at start-up for the process's lifetime.
6. **The match bank** loaded with each world.

## Decision Outcome

Chosen options: 1, 3 and 5.
- Option 2 puts about 4 MB of WAV files in the repository and a tool project, a CI step and an ADR-090 amendment around them, for sounds that are placeholders until a group is recorded. AST-082 already covers that case.
- Option 4 would make the attack warning and a laser shot the same beep, which a playtest cannot judge.
- Option 6 needs a match the persistent world does not have (§8.3 item 2 of the audio engine). The whole MVP set is 2.79 MiB, a third of the interface bank's 8 MiB.

* **The stand-ins** (`Tools/SpeciesContentBuild/AudioCooker`, `MakePlaceholderSound`). Each sound row of the MVP's priority with no authored file and the placeholder `synthesised tone` gets a stand-in at its key. It is synthesized at 48 kHz mono in double precision, peak-normalized to −1 dBFS, faded over its last 4 ms, and cooked as an authored member is. It is flagged as a placeholder, as AST-083 asks. Its noise comes from SplitMix64, seeded by FNV-1a of the asset id mixed with the member's number, so each member is a pure function of its row (AST-084). The recipes:
  - `sfx.weapon.laser_bolt.spawn`, 10 members of 0.24 to 0.56 s: a frequency-modulated zap falling from 2.6 kHz toward about 0.4 kHz, over a click of differenced noise;
  - `sfx.unit.worker.die`, 8 members of 0.45 to 1.15 s: a warbling squeal falling from about 0.8 kHz to 0.1 kHz, over a two-pole crackle;
  - `sfx.unit.essence_orb.collected`, 2.0 s: a rising four-note arpeggio of glassy bells (C6, E6, G6, C7) with a shimmer;
  - `sfx.building.dwelling.complete`, 2.0 s: a two-note chime a fifth apart (G5, D6) over a soft thump;
  - `sfx.unit.worker.attack_horn`, 3 members of 3.0, 3.7 and 4.4 s: a brass call on G2, A2 or F2 rising a fourth, with vibrato and breath;
  - `sfx.ui.battle.start`, 2.5 s: three drum hits, then a brass stab on D;
  - `sfx.ui.battle.end`, 2.5 s: two brass chords resolving from G to C, over a faint cymbal wash.

  A sound row with no recipe gets table 4.7-A's 440 Hz sine burst of its shortest target duration. Since every member peaks at the same level, each event's mix level is the engine's to set (the second step).
* **Authored sources** (`CookSound`, `ReadWav`). A WAV source is read plain or `WAVE_FORMAT_EXTENSIBLE`. The build fails, naming the file, for:
  - another rate (it resamples nothing);
  - another encoding than 24-bit integer or 32-bit float PCM;
  - no frames (AEN-179);
  - more than one channel for a positioned row, or more than two for any row;
  - a row whose target format is not resident;
  - a member longer than `aen.residency.stream_threshold` (20 s).

  The MVP cooks no stream: a streamed group is a build error until a milestone needs one.
* **The cooked layout** (`NeuronCore/SoundAsset`, `ContentType::ResidentSound`, version 1). It holds the version, the rate, the channel count and the frame count, each little-endian, then the frames as interleaved 16-bit samples. The reader refuses:
  - another version;
  - a rate other than 48 kHz;
  - zero or more than two channels;
  - no frames;
  - a length its frame count does not give.
* **The bank** (`CheckSoundBank`). Every resident member of the MVP is in the interface bank. The build fails when their bytes pass `aen.memory.interface_bank_budget` (8 MiB), naming the largest members (AST-081). The 25 members hold 2,925,950 bytes, counted on the cooked output of this build.

### Consequences

* Good, because the game always has its sound set, and a recorded group replaces its stand-in when its files appear, with no change of code (O98, AST-082).
* Good, because no binary audio is committed, and two builds write the same bytes.
* Good, because each group sounds like its role, so Playtest 1 can judge the mix.
* Bad, because the sounds can be heard only through the game, as no tool writes them as files.
* Bad, because changing a sound is a change to the content build's code.
* Bad, because the MVP's sounds are placeholders by AST-083's flag, so a release edition would refuse them (AST-085); a release is not planned before the design resumes.
* It forecloses: committed synthesized WAV files; a match bank at the MVP; streamed sounds at the MVP.

### Confirmation

* `Build\CheckContentBuild.py`:
  - two builds write the same bytes;
  - each of the 25 sound rows of the MVP without a file has its mono 48 kHz stand-in at its key;
  - an authored WAV source replaces its stand-in;
  - a source at 44.1 kHz, a 16-bit source, a stereo source of a positioned row and a source with no frames are refused with their file and rule.
* `NeuronCoreTests/SoundAssetTests`: a resident sound reads back as written, and one the engine cannot play does not read.
* Analysed here on the Linux build's output, by a script reading the cooked entries:
  - every member peaks at −1 dBFS and ends at silence;
  - the DC offset is at most 0.0014 of full scale;
  - no member of the Worker death or battle groups has power above 4 kHz.
* The owner's T15 run: each sound is heard on its event.

## Pros and Cons of the Options

### 1 — the content build synthesizes the stand-ins

* Good, because AST-082 already describes it, and nothing is committed.
* Bad, because the sounds exist only as cooked entries.

### 2 — a sound kit's committed WAV files

* Good, because the sounds are files to listen to and replace.
* Bad, because of the repository's size, a tool project and a CI step for placeholders.

### 3 — a recipe per role

* Good, because each sound says what happened.
* Bad, because the recipes are code to maintain.

### 4 — a sine or noise burst

* Good, because it is the simplest stand-in.
* Bad, because every event sounds alike.

### 5 — the interface bank

* Good, because a persistent world has no match load to wait for, and the set is small.
* Bad, because the sounds stay resident for the whole process.

### 6 — the match bank

* Good, because memory follows the world.
* Bad, because the world game has no match to load it with.

## More Information

* Amends [ADR-025](ADR-025-audio-formats-codecs-and-residency.md) (its resident path built at the MVP's size; the stream path not built) and [ADR-140](ADR-140-structures-drawn-and-the-stand-ins-at-m2.md) (the stand-ins now include the sound rows); at the MVP, table 4.7-A's synthesised tone of the [asset pipeline](../architecture/components/asset_pipeline_and_resources.md) is read with this ADR's recipes.
* Owner decision O98; MVP-087; AST-079 to AST-084; AEN-072, AEN-074, AEN-179.
* The code:
  - `Tools/SpeciesContentBuild/AudioCooker` (`CookSound`, `MakePlaceholderSound`, `CheckSoundBank`), `WavFile` (`ReadWav`), `ContentBuild`;
  - `NeuronCore/SoundAsset`;
  - `Build/CheckContentBuild.py`;
  - `NeuronCoreTests/SoundAssetTests`.
