# 29 — Data formats reference

> Part B · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: X1a_text_formats, X1b_asset_formats, P14_platform_lib (29.T, 29.P, 29.L, 29.D, 29.B), P15_network (§4 encoding, §5.2 team controls), P11_terrain_world (level-file and `.mwm` notes), P16_sound (sound grammar cross-checks), P10_input_camera (binding grammar cross-checks), P19 (script opcodes), _meta_* notes, phase2_log.md. Spot-verified against the code for this document (every parser named in §2 was re-read; survey scripts re-run, §1.4).

## Contents

- [1 How to read this document](#1-how-to-read-this-document)
- [2 Format index](#2-format-index)
- [3 Shared layers: tokeniser, conventions, obfuscation, binary readers, Directory](#3-shared-layers-tokeniser-conventions-obfuscation-binary-readers-directory)
- [4 Level and game-definition files](#4-level-and-game-definition-files)
- [5 Configuration, input and credit lists](#5-configuration-input-and-credit-lists)
- [6 Localisation and scripts](#6-localisation-and-scripts)
- [7 Entity, gesture and audio tables](#7-entity-gesture-and-audio-tables)
- [8 Geometry, images and animation](#8-geometry-images-and-animation)
- [9 Shaders](#9-shaders)
- [10 Packaging: archives and pack lists](#10-packaging-archives-and-pack-lists)
- [11 Runtime-written, network and tool files](#11-runtime-written-network-and-tool-files)
- [12 Re-implementation guidance for the new engine](#12-re-implementation-guidance-for-the-new-engine)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

---

## 1 How to read this document

### 1.1 Scope

Every file format the Multiwinia code base reads or writes — shipped text definitions under `data/`, binary assets, packed archives, build-time lists, files written at run time into the profile directory, and the binary wire container used by the network layer — has its own section below, introduced by an HTML anchor `fmt-<id>` that matches a row of `_work/formats_registry.csv`. Semantics that belong to a subsystem (what a building column *does*, what a sound parameter *sounds like*) are specified in the subsystem's document; this document fixes **syntax, types, units, defaults, ranges, parser behaviour and failure modes** precisely enough to write a compatible reader and writer.

Every format section follows the same template: purpose · files · encoding · formal grammar (EBNF) or byte layout · field table (name, type, units, default, range, semantics, parser ref) · verification · minimal valid example (written for this document, not copied from shipped data) · discrepancies and edge cases.

### 1.2 Grammar notation

* EBNF: `a b` sequence, `a | b` alternative, `[ a ]` optional, `{ a }` zero or more, `1*a` one or more, `'lit'` literal. Literals are **ASCII case-insensitive** unless marked **(cs)** (case-sensitive), because almost every parser compares keywords with `stricmp`.
* `EOL` = end of one physical line. Text grammars are stated **after** the shared cleaning and tokenising pass of [§3.1](#fmt-textreader); "token" means a maximal run of non-separator characters on a cleaned line.
* `REST` = the remainder of the line returned by `GetRestOfLine`/`GetRestOfUnicodeLine` (leading separators skipped, everything else verbatim).
* Binary layouts are little-endian unless stated; `u8/i16/u16/i32/u32/u64/f32/f64` have their usual meaning.

### 1.3 Numeric value types used in field tables

| Type | Definition | Parser primitive |
|---|---|---|
| `INT` | whatever C `atoi` accepts: optional sign, decimal digits; parsing stops at the first non-digit; an unparsable token gives 0; no overflow detection | `atoi` |
| `FLOAT` | whatever C `atof` accepts (decimal point `.`, optional exponent); result kept as `double` | `atof` |
| `F32` | `FLOAT`, then rounded to IEEE-754 single precision and widened back to `double` — `iv_atof(s)` ≡ `iv_trunc_low_nibble(atof(s))`; used for fields that feed the deterministic simulation | `contrib/systemiv/lib/math/math_stubs.cpp:iv_atof` |
| `WORD` | one token, copied verbatim (`strcpy`/`strdup`), usually into a fixed buffer whose size is given | `TextReader::GetNextToken` |
| `BOOL` | `INT`, non-zero = true (`atoi(x) ? true : false` or `(bool)atoi(x)`) | `atoi` |

Determinism note [C]: F32 rounding is applied by `iv_atof`; any field marked F32 must be parsed with a platform-independent decimal parser and rounded to binary32 in the new engine, otherwise terrain and stats drift between builds ([22](22_determinism_and_networking.md)).

### 1.4 Failure vocabulary and verification method

* **MISSING ⇒ CRASH** — the parser calls `GetNextToken()` without first checking `TokenAvailable()`; a short line yields a `NULL` token that is passed to `atoi`/`atof`/`strcpy`/`stricmp`/`strdup` → access violation (or CRT invalid-parameter termination) in release [C code path; I exact failure mode — no `_set_invalid_parameter_handler` exists in `code/` or `contrib/systemiv/lib`].
* **fatal** — `AppReleaseAssert`, `AppAssert` or `AppAbort`: message box "Fatal Error", `blackbox.txt`, process ends ([31 §6.5](31_platform_build_and_tools.md)) [C] `contrib/systemiv/lib/debug_utils.h` (`AppAssert` is defined as `AppReleaseAssert`).
* **debug assert** — `AppDebugAssert`: does nothing in Release except showing the OS cursor; parsing continues [C] `contrib/systemiv/lib/debug_utils.h`.
* **Verification** for each format names (a) the parser function(s) re-read for this document and (b) the survey script and the number of shipped files it checked. Survey scripts live in `_work/scripts/` (X1a_*, X1b_*, P11_mapcrc.py, P10_bindings.py, P16_parse_sounds.py, S_31_archive_contents.py) and were re-run for this document with `python3 -I` on the source tree at commit `f909630`; results quoted below are from those runs (e.g. `X1a_level_survey.py` reproduced `_work/drafts/X1a_level_field_usage.csv` byte-for-byte).

---

## 2 Format index

Kind: **text** (line-oriented text parsed by the shared tokeniser unless noted), **binary**, **runtime** (written by the running game or a tool; not shipped), **network** (wire format), **convention** (naming rule over files of another format), **authoring** (source files never read by the game).

| Anchor | Name | Kind | Files | Parser / writer | Build |
|---|---|---|---|---|---|
| [fmt-textreader](#fmt-textreader) | TextReader line reader and tokeniser | text layer | every text format | `contrib/systemiv/lib/filesys/text_stream_readers.cpp:TextReader`, `contrib/systemiv/lib/unicode/unicode_text_stream_reader.cpp:UnicodeTextReader`, `code/lib/resource.cpp:Resource::GetTextReader` | [B:FULL] |
| [fmt-text-common](#fmt-text-common) | Common text conventions (numbers, blocks, blank-line rules, case) | text layer | every text format | per-format parsers | [B:FULL] |
| [fmt-redshirt](#fmt-redshirt) | "redshirt2" obfuscated text | text layer / runtime | campaign saves only | `TextReader::CleanLine`, `contrib/systemiv/lib/filesys/text_file_writer.cpp:TextFileWriter` | [B:LEGACY] use |
| [fmt-binary-common](#fmt-binary-common) | BinaryReader conventions | binary layer | every binary read | `contrib/systemiv/lib/filesys/binary_stream_readers.cpp` | [B:FULL] |
| [fmt-directory](#fmt-directory) | `Directory` serialisation (network letters, metaserver, `.mwm`, server history) | network / binary | UDP datagrams; inside `.mwm`, `serverHistory.dat` | `contrib/systemiv/lib/tosser/directory.cpp:Directory::Read/Write`, `contrib/systemiv/lib/network_stream.cpp` | [B:FULL] |
| [fmt-level-file](#fmt-level-file) | Level map / mission files | text | `data/levels/*.txt`, `data/levels/old/*.txt`, `data/purity/map_*.txt`, `data/purity/mission_*.txt` | `code/level_file.cpp:LevelFile` | mp: [B:FULL]; campaign: [B:LEGACY] |
| [fmt-level-enum](#fmt-level-enum) | Achievement level ids | text | `data/levels/level_enum.txt` | `code/achievement_tracker.cpp:AchievementTracker::AchievementTracker` | [B:FULL] |
| [fmt-multiwinia-txt](#fmt-multiwinia-txt) | Game types and lobby parameters | text | `data/multiwinia.txt`, `data/multiwinia-testbed.txt` | `code/multiwinia.cpp:Multiwinia::LoadBlueprints` | [B:FULL] / [B:DEAD] |
| [fmt-game-txt](#fmt-game-txt) | Campaign world definition | text | `data/game.txt`, `data/game_demo.txt`, `data/game_demo2.txt`, `data/game_unlockall.txt`, `data/purity/game.txt` | `code/global_world.cpp:GlobalWorld::LoadGame` | [B:LEGACY] / [B:DEAD] |
| [fmt-locations-txt](#fmt-locations-txt) | World-map location positions | text | `data/locations.txt` | `code/global_world.cpp:GlobalWorld::LoadLocations` | [B:LEGACY] |
| [fmt-map-crc](#fmt-map-crc) | Map identity CRC and `code/mapcrc.h` | binary algorithm + generated header | `code/mapcrc.h` | `contrib/systemiv/lib/math/filecrc.cpp:FileCRC`, `tools/MapCRC/main.cpp:GenerateMapCRCs` | [B:FULL] |
| [fmt-preferences](#fmt-preferences) | Preferences | text / runtime | `<P>preferences.txt`, `data/default_preferences.txt` | `code/lib/preferences.cpp:PrefsManager`, `PrefsItem` | [B:FULL] |
| [fmt-input-bindings](#fmt-input-bindings) | Input binding files | text | `data/input/input_preferences*.txt`, `<P>user_input_preferences.txt` | `code/lib/input/input.cpp:InputManager::parseInputPrefs`, `code/lib/input/inputdriver_*.cpp` | [B:FULL] |
| [fmt-keyboard-layout](#fmt-keyboard-layout) | Keyboard-layout overrides | text | `data/input/keyboards/*.txt` | as above (replace mode) | [B:FULL] |
| [fmt-locales](#fmt-locales) | LANGID → layout map | text | `data/input/locales.txt` | `code/lib/input/file_paths.cpp:readLocaleNames` | [B:FULL] |
| [fmt-betatesters](#fmt-betatesters) | Credit name lists | text | `data/betatesters.txt`, `data/multiwinia_betatesters.txt` | `code/UI/GameMenuWindow.cpp:ReadMultiwiniaBetaTesters`, `code/loaders/credits_loader.cpp:CreditsLoader::GenerateBetaTesterList` | [B:FULL] / [B:LEGACY] |
| [fmt-language](#fmt-language) | Language phrase tables and caption markup | text (UTF-16 or 8-bit) | `data/language/*.txt`, `strings_<lang>.txt` overlays | `code/lib/language_table.cpp:LangTable` | [B:FULL] |
| [fmt-script](#fmt-script) | Cut-scene scripts | text | `data/scripts/*.txt` | `code/script.cpp:Script` | [B:LEGACY] |
| [fmt-purity-misc](#fmt-purity-misc) | Purity-control script | text | `data/purity/buyme.txt` | `code/script.cpp:Script` (no caller) | [B:DEAD] |
| [fmt-purity](#fmt-purity) | Purity-control data set (directory) | convention | `data/purity/*` | none | [B:DEAD] |
| [fmt-stats-txt](#fmt-stats-txt) | Entity base stats | text | `data/stats.txt` | `code/worldobject/entity.cpp:EntityBlueprint::Initialise` | [B:FULL] |
| [fmt-gestures](#fmt-gestures) | Gesture classifier training set | text | `data/gestures.txt` | `code/gesture.cpp:Gesture::LoadTrainingData` | [B:FULL] (parsed; gestures unused in play) |
| [fmt-gesture-demo](#fmt-gesture-demo) | Recorded gesture strokes | text | `data/gesture_demos/*.txt` | `code/gesture_demo.cpp:GestureDemo` | [B:DEAD] |
| [fmt-sounds-txt](#fmt-sounds-txt) | Sound-event blueprints | text | `data/sounds.txt` | `code/sound/soundsystem.cpp:SoundSystem::LoadBlueprints`, `code/sound/sound_parameter.cpp:SoundParameter::Read` | [B:FULL] |
| [fmt-effects-txt](#fmt-effects-txt) | DSP filter blueprints | text | `data/effects.txt` | `code/sound/soundsystem.cpp:SoundSystem::LoadEffects` | [B:FULL] |
| [fmt-ogg](#fmt-ogg) | Ogg Vorbis samples (+ unreachable WAV) | binary | `data/sounds/*.ogg`, `data/mwsounds/*.ogg` | `code/sound/sound_stream_decoder.cpp:SoundStreamDecoder`, `code/sound/sample_cache.cpp:CachedSample` | [B:FULL] |
| [fmt-shp](#fmt-shp) | Shape meshes | text | `data/shapes/*.shp` | `code/lib/shape.cpp:Shape::Load`, `ShapeFragment`, `ShapeMarker` | [B:FULL] |
| [fmt-max](#fmt-max) | 3ds Max shape sources | authoring (binary) | `data/shapes/source/*.max` | `tools/ShapeExporter/ShapeExporter.cpp` (tool) | not read by game |
| [fmt-bmp](#fmt-bmp) | BMP bitmaps and textures | binary | `data/**/*.bmp` (506 files) | `code/lib/bitmap.cpp:BitmapRGBA::LoadBmp`, `code/lib/resource.cpp:Resource::GetTextureWithFlags` | [B:FULL] |
| [fmt-png-output](#fmt-png-output) | PNG screenshot output | runtime (binary) | `<P>screenshots/*.png` | `code/lib/bitmap.cpp:BitmapRGBA::SavePng`, `code/lib/poster_maker.cpp:PosterMaker::SavePoster` | [B:FULL] |
| [fmt-colour-ramp](#fmt-colour-ramp) | Terrain/water/wave colour ramps | binary (BMP) + convention | `data/terrain/*.bmp` | `code/landscape_renderer.cpp:LandscapeRenderer::GetLandscapeColour`, `code/water.cpp:Water` | [B:FULL] |
| [fmt-anim](#fmt-anim) | Animated help panel | text | `data/animations/*.anim` | `code/animatedpanel.cpp:AnimatedPanel::Read` | [B:FULL] |
| [fmt-icons](#fmt-icons) | Icon naming | convention | `data/icons/*.bmp` | `code/taskmanager_interface_icons.cpp`, `code/multiwiniahelp.cpp` | [B:FULL] |
| [fmt-mwhelp](#fmt-mwhelp) | Help-image naming | convention | `data/mwhelp/*.bmp` | `code/multiwiniahelp.cpp:MultiwiniaHelp` | [B:FULL] |
| [fmt-thumbnail](#fmt-thumbnail) | Map thumbnail naming | convention | `data/thumbnails/*.bmp` | `code/UI/MapImageButton.h:MapImageButton::Render` | [B:FULL] |
| [fmt-sepulveda](#fmt-sepulveda) | Mentor portraits | convention | `data/sepulveda/*.bmp` | `code/sepulveda.cpp:Sepulveda` | [B:DEAD] |
| [fmt-psp](#fmt-psp) | Paint Shop Pro image | authoring | `data/icons/banner_template.psp` | none | not read |
| [fmt-grd](#fmt-grd) | Photoshop gradient | authoring | `data/Untitled Gradients.grd` | none | not read |
| [fmt-shader](#fmt-shader) | HLSL shaders | text (HLSL) | `data/shaders/*` | `code/lib/shader.cpp:Shader` | [B:D3D] |
| [fmt-dat-archive](#fmt-dat-archive) | `.dat` RAR archives | binary | `init.dat`, `language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat`, `sounds.dat`, `sounds2.dat`; repository `animator.dat` | `code/lib/resource.cpp:Resource::ParseArchive`, `contrib/unrar/unrar.cpp:UncompressedArchive` | [B:FULL] |
| [fmt-pack-list](#fmt-pack-list) | Build-time inclusion/exclusion/sound lists | text (build) | `tools/*exclusions*.txt`, `tools/*inclusions*.txt`, `tools/sounds.txt`, `tools/menu_sounds.txt`, `tools/*.list` | `tools/listfiles.pl`, `tools/copyfiles.pl`, `tools/build_multiwinia_data.bat` | build only |
| [fmt-runtime-files](#fmt-runtime-files) | Index of runtime-written files | runtime | profile directory, working directory | various | various |
| [fmt-profile-save](#fmt-profile-save) | Campaign save files | runtime (text) | `<P>users/<profile>/*.txt` | `code/global_world.cpp:GlobalWorld::SaveGame`, `code/level_file.cpp:LevelFile::SaveMissionFile` | [B:LEGACY] |
| [fmt-authkey](#fmt-authkey) | Authentication key files | runtime (text) | `<P>authkey.dev`, `<P>authkey` | `contrib/systemiv/lib/metaserver/authentication.cpp` | [B:FULL] |
| [fmt-sync-dump](#fmt-sync-dump) | Desync dump files | runtime (text) | `<P>syncerror-*.txt`, `<P>syncdiff-*.txt` | `code/location.cpp:Location::DumpWorldToSyncLog`, `code/network/syncdiff.cpp:SyncDiff::Print` | [B:FULL] |
| [fmt-mwm](#fmt-mwm) | Custom map container | runtime (binary) | `<P>maps/*.mwm` | `code/mapfile.cpp:MapFile` | [B:EDITOR] |
| [fmt-server-history](#fmt-server-history) | Server message recording | runtime (binary) | `./serverHistory.dat` | `code/network/server.cpp:Server::SaveHistory/LoadHistory` | [B:FULL] (pref-gated) |
| [fmt-team-controls](#fmt-team-controls) | Packed team-controls payload | network (binary) | `ALIVE_DATA` field of client letters | `code/team.cpp:TeamControls::Pack/Unpack` | [B:FULL] |
| [fmt-blackbox](#fmt-blackbox) | Black-box report | runtime (text) | `<P>blackbox.txt` | `contrib/systemiv/lib/debug_utils.cpp:AppGenerateBlackBox` | [B:FULL] |
| [fmt-crash-report](#fmt-crash-report) | SEH crash log and minidump | runtime (text + binary) | `<P>ERRORLOG.TXT`, `<P>CRASH.DMP` | `code/lib/exception_handler.cpp:RecordExceptionInfo` | [B:FULL] |
| [fmt-debug-log](#fmt-debug-log) | Debug log | runtime (text) | `<P>debug.txt` | `contrib/systemiv/lib/debug_utils.cpp:AppDebugOut` | [B:FULL] |
| [fmt-origin-version](#fmt-origin-version) | `origin` / `version` files | runtime / packaging (text) | `./origin`, `./version` | `code/app.cpp:App::Initialise`, `code/main.cpp:AppMain` | [B:FULL] |
| [fmt-language-errors](#fmt-language-errors) | Translation QA report | runtime (text) | `./language_errors.txt` | `code/lib/language_table.cpp:LangTable::TestAgainstEnglish` | [B:FULL] (pref-gated) |
| [fmt-user-info](#fmt-user-info) | Registration system/user info texts | text (legacy) | none shipped | `code/keygen.cpp` | [B:LEGACY] |
| [fmt-support-ini](#fmt-support-ini) | Crash-report file list | text (INI) | `MultiwiniaSupport.ini` | `contrib/XCrashReport/Dlg.cpp:CDlg::GetFileDetails` | support tool |
| [fmt-testbedconfig](#fmt-testbedconfig) | Test-bed sequence | text | `testbedconfig.txt` | `code/UI/GameMenuWindow.cpp:GameMenuWindow::ReadTestBedConfig` | [B:DEAD] |

Format ids added by this document (appended to `_work/formats_registry.csv`): `binary-common`, `directory`, `map-crc`, `png-output`, `server-history`, `team-controls`, `blackbox`, `crash-report`, `debug-log`, `origin-version`, `language-errors`, `user-info`.

---

## 3 Shared layers: tokeniser, conventions, obfuscation, binary readers, Directory

<a id="fmt-textreader"></a>
### 3.1 TextReader line reader and tokeniser

**Purpose.** One reader family turns every shipped text file into cleaned lines and tokens; every text grammar in this document is defined on top of it. Compiled into the baseline [B:FULL] [C] `targets/vs2005/Darwinia.vcproj` (entries for `contrib\systemIV\lib\filesys\text_stream_readers.cpp` and `contrib\systemIV\lib\unicode\unicode_text_stream_reader.cpp`).

**Reader classes** [C] `contrib/systemiv/lib/filesys/text_stream_readers.h`, `contrib/systemiv/lib/unicode/unicode_text_stream_reader.cpp`:

| Class | Input | Encoding | Chosen when |
|---|---|---|---|
| `TextReader` (abstract) | – | – | base: cleaning, tokenising; `CleanLine`, `TokenAvailable`, `GetNextUnicodeToken`, `GetRestOfUnicodeLine`, `SetSeperatorChars` are virtual; `GetNextToken` and `GetRestOfLine` are **not** virtual (always operate on the narrow line `m_line`) |
| `TextFileReader` | loose disk file, `fopen(path,"r")` (CRT text mode: CRLF read as LF) | single byte, no transcoding | disk file without UTF-16 BOM |
| `TextDataReader` | memory block (archive entry, `.mwm` map text) | single byte, raw (CR kept until cleaning) | archive entry without BOM |
| `UnicodeTextFileReader` | loose disk file, `fopen(path,"rb")` | UTF-16 by BOM `FF FE` (LE) or `FE FF` (BE); one code unit per character, no surrogate handling | disk file whose first two bytes are a BOM |
| `UnicodeTextDataReader` | memory block | as above; BOM skipped (`m_offset = 2`) | archive entry beginning with a BOM |

**Selection** [C] `code/lib/resource.cpp:Resource::GetTextReader`: for resource name `N` (relative to `data/`): (1) [B:DEAD] `USE_DARWINIA_MOD_SYSTEM` mod paths; (2) loose `data/N` relative to the working directory — `UnicodeTextFileReader`, replaced by `TextFileReader` if `IsUnicode()` is false; (3) archive key `data/N` — `UnicodeTextDataReader`, else `TextDataReader`; (4) `NULL` and debug log `Failed to find text resource: data/N`. The full path buffer is `char[256]` (a name longer than 250 characters overflows) [C]. A few callers bypass `Resource` and open a `TextFileReader`/`UnicodeTextFileReader` directly on a profile or working-directory path (preferences, user bindings, campaign saves, `testbedconfig.txt`, `origin`); those are named in their sections. Both file readers call `FindCaseInsensitive(path)` first (`contrib/systemiv/lib/filesys/filesys_utils.cpp`), which matters only on case-sensitive file systems [C]. `contrib/systemiv/lib/filesys/file_system.cpp:FileSystem::GetTextReader` is compiled but `g_fileSystem` is never created by Multiwinia [C grep: no `new FileSystem` under `code/`].

**Physical lines and end of file** [C] `TextFileReader::ReadLine`, `TextDataReader::ReadLine`, `UnicodeTextFileReader::ReadLine`, `UnicodeTextFileReader::ReadUnicodeLine`, `UnicodeTextDataReader::ReadLine`, `TextReader::DoubleMaxLineLen`:

| Rule | `TextFileReader` | `TextDataReader` | `UnicodeTextFileReader` | `UnicodeTextDataReader` |
|---|---|---|---|---|
| Line terminator | LF (CRLF → LF by the CRT) | LF; the CR stays in the line and is removed by cleaning | LF code unit | LF code unit (the LF itself is skipped, `m_offset += 2`) |
| Unterminated last line | **processed** (`ReadLine` returns true for it, false on the next call) | loaded but `ReadLine` returns **false** → a `while (ReadLine())` loop never sees it | loaded but returns **false** (`ReadUnicodeLine` returns `NULL` at `feof`) | loaded but returns **false** |
| Maximum line length | buffer starts at 128 characters and doubles; reaching 65 536 is `AppReleaseAssert("Text file contains line with more than 65536 characters")` (fatal) | same | same | same |
| NUL code unit | n/a | n/a | skipped without being stored (the slot keeps stale data) | stored |

Resolved: drafts disagreed on which readers drop an unterminated last line (X1b listed only the archive reader; P14 implied the UTF-16 file reader keeps it). The code shows that **only the 8-bit disk reader** processes it; the three others drop it. Five shipped files lack a final newline (`data/levels/level_enum.txt`, `testbedconfig.txt`, `data/language/japanese.txt`, `data/language/korean.txt`, `data/language/portuguese.txt`); consequences are given in their sections [C] `X1a_text_survey.py` (re-run: `level_enum.txt` and `testbedconfig.txt` flagged NO-FINAL-LF).

**Cleaning** — `CleanLine`, applied once to every line, in this order [C] `TextReader::CleanLine`, `UnicodeTextReader::CleanLine`:

1. *De-obfuscation*: only on the first line of a file — see [fmt-redshirt](#fmt-redshirt).
2. *Comment removal*: scan left to right; the first `#` is replaced by NUL and the scan stops. No quoting, no escaping. During the scan, runs of `<`/`>` are counted and three adjacent angle brackets trigger `AppDebugAssert` (merge-conflict detector, debug builds only).
3. *Line-end removal*: the first `\n` and then the first `\r`, searched from the start of the (already comment-truncated) string, are replaced by NUL. A stray CR in mid-line therefore discards the rest of the line.

Order of conversion for UTF-16 [C]: `UnicodeTextFileReader::ReadLine` builds the narrow mirror (`UnicodeString::CopyUnicodeToCharArray`, every code unit > 255 becomes `?`, ≤ 255 keeps its low byte) **before** cleaning the wide line, so the narrow mirror still contains comments and CR/LF; `UnicodeTextDataReader::ReadLine` cleans first and converts afterwards.

**Tokenisation** [C] `TextReader::GetNextToken`, `TokenAvailable`, `GetRestOfLine`, `UnicodeTextReader::GetNextUnicodeToken`, `GetRestOfUnicodeLine`, `TokenAvailable`:

| Call | Operates on | Behaviour |
|---|---|---|
| `TokenAvailable()` | wide line for UTF-16 readers (virtual), narrow line otherwise | true iff a non-separator character exists at or after the cursor |
| `GetNextToken()` | **narrow** line always | `NULL` if the cursor is on NUL; skip separators; `NULL` if none remain; else scan to the next separator **or the string terminator** (`strchr(sep, '\0')` matches the terminator), write NUL there and move the cursor **one past it** |
| `GetRestOfLine()` | narrow line | `NULL` if the cursor is on NUL; else skip separators and return the remainder verbatim (internal separators and trailing blanks kept; an empty string if only separators remained) |
| `GetNextUnicodeToken()` | wide line (UTF-16 readers) | same algorithm on the wide line, returns a `UnicodeString` copy (empty when none) |
| `GetRestOfUnicodeLine()` | wide line (UTF-16 readers); narrow line converted with `UnicodeString(char const*)` otherwise | as `GetRestOfLine`; returns an empty `UnicodeString` instead of `NULL` |

Separator sets [C] `DEFAULT_SEPERATOR_CHARS` in both `.cpp` files: 8-bit readers `" \t\n\r:,"` (space, TAB, LF, CR, colon, comma); UTF-16 readers add form feed (`L" \f\t\n\r:,"`) for the wide calls. No parser in `code/` or `contrib/systemiv/lib` calls `SetSeperatorChars` [C grep], so every format uses the default set. (`UnicodeTextReader::SetSeperatorChars` would copy the new characters over the start of the wide set without terminating it — unused.)

**Formal summary**

```ebnf
file          = [ obfuscation_header ] , { physical_line } ;       (* header: first line only, see fmt-redshirt *)
physical_line = { char - LF } , ( LF | EOF ) ;
cleaned_line  = prefix of physical_line before the first '#', then before the first LF, then before the first CR ;
separator     = ' ' | TAB | LF | CR | ':' | ',' | FF ;                 (* FF only for the wide calls of UTF-16 readers *)
token         = 1*( char - separator - NUL ) ;
token_line    = { separator } , [ token , { 1*separator , token } ] , { separator } ;
rest_of_line  = text after the leading separators, verbatim ;
```

Consequences used by every grammar: `:` and `,` are interchangeable with blanks (`pos: 1, 2, 3` ≡ `pos 1 2 3`); no value can contain a blank, `:`, `,` or `#` unless the consumer reads `REST`; a `REST` value can never begin with blank, `:` or `,`; keyword case is decided by each parser.

**Encodings shipped** [C] `X1a_text_survey.py` → `_work/drafts/X1a_encoding_survey.csv` (190 text files in scope) plus the X1b `file(1)` survey of `.shp`/`.anim`/`sounds.txt`/`effects.txt`/`stats.txt`/`gestures.txt`:

| Group | Files | Encoding | Line ends |
|---|---|---|---|
| `data/levels/*.txt` | 87 | ASCII | CRLF; `level_enum.txt` has no final newline |
| `data/levels/old/*.txt` | 30 | ASCII | CRLF |
| `data/purity/*.txt` | 4 | ASCII | CRLF |
| `data/scripts/*.txt` | 42 | ASCII | CRLF |
| `data/input/*.txt`, `data/input/keyboards/*.txt` | 6 | ASCII | CRLF |
| `data/*.txt` | 15 | ASCII, except `data/betatesters.txt` and `data/multiwinia_betatesters.txt` (Latin-1 bytes in accented names) | CRLF |
| `data/language/*.txt` | 9 | `english.txt` single byte (2 Latin-1 bytes); French, German, Italian, Korean, Portuguese, Spanish **UTF-16BE + BOM**; Japanese, Russian **UTF-16LE + BOM** | CRLF; Portuguese mixes 217 LF-only lines; Japanese, Korean, Portuguese have no final newline |
| `data/shapes/*.shp` | 139 | ASCII | CRLF except `primaryupgradeport.shp` (LF only) |
| `data/animations/*.anim` | 6 | ASCII | CRLF |
| root `testbedconfig.txt`, `MultiwiniaSupport.ini` | 2 | ASCII | CRLF; `testbedconfig.txt` has no final newline |

No shipped file starts with `redshirt2`; no file has a UTF-8 BOM (a UTF-8 BOM would not be recognised and would become part of the first token). Single-byte text is widened byte-for-byte (ISO-8859-1) by `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::UnicodeString(char const*)` [C].

**Verification.** Parser functions re-read for this document: all of `text_stream_readers.cpp`, all of `unicode_text_stream_reader.cpp`, `Resource::GetTextReader`, `UnicodeString::StrOffsetCmp`. Survey: `X1a_text_survey.py` over 190 files (re-run on 7 representative files for this document; results identical to the CSV).

**Minimal valid example** (shows the separator and comment rules; any consumer sees the tokens `alpha`, `12`, `beta`, `3.5`):

```text
alpha: 12, beta 3.5   # trailing comment after a blank
```

**Edge cases and discrepancies** [C, derived from the code above]:

* *Comment glued to a token* (no blank before `#`), 8-bit readers: `key#note` is cleaned to `key` NUL `note` …; because `GetNextToken` moves one past the terminator, the next call returns `note` (and further comment words) as tokens. Through `UnicodeTextFileReader` the narrow mirror is uncleaned, so the first token is `key#note`. Always put a blank before `#`. (Resolved: P14 and X1a each described one half of this behaviour; both halves are correct.)
* *Token ending exactly at the string terminator* (no trailing separator and no CR/LF — only possible on an unterminated last line of a disk file, or after a glued comment): the following `GetNextToken` reads past the terminator into stale buffer contents [C].
* A line that is entirely a comment is correctly reported as empty by `TokenAvailable` for every reader [C].
* `GetRestOfLine` on a line whose last token ends at the terminator returns `NULL` (not `""`); several consumers pass that to `strcpy`/`strlen`/`UnicodeString(char const*)` (`strlen(NULL)`) — crash [C].

<a id="fmt-text-common"></a>
### 3.2 Common text-format conventions

**Purpose.** Conventions that the individual text grammars share, so they are stated once.

| Convention | Rule | Parser evidence |
|---|---|---|
| Block delimiters | Level, game and campaign files use `<Name>_StartDefinition` / `<Name>_EndDefinition` first tokens (case-insensitive); `sounds.txt`, `effects.txt`, `multiwinia.txt` use `<KIND> <name>` … `END`; `.anim` uses `BEGIN Object|Keyframe` … `END` | `code/level_file.cpp:LevelFile::ParseMapFile`, `code/global_world.cpp:GlobalWorld::LoadGame`, `code/sound/soundsystem.cpp:SoundSystem::LoadBlueprints`, `code/animatedpanel.cpp:AnimatedPanel::Read` |
| Blank / comment-only lines | Allowed wherever the loop begins with `if (!TokenAvailable()) continue;`; **crash** wherever the loop is `ReadLine(); tok = GetNextToken(); stricmp(tok, …)` (NULL token). Each grammar states which applies | per parser |
| Missing block end | the block parser consumes the rest of the file | per parser |
| Unknown first token at top level | mostly `AppDebugAssert(0)` + skip (release: ignored) | per parser |
| Unknown field inside a block | ignored (most parsers) or debug assert + skip | per parser |
| Repeated scalar keyword | last occurrence wins; list keywords accumulate | `LevelFile::ParseMultiwiniaOptions` |
| Duplicate keys in tables | preferences: first wins; language: last wins; `level_enum.txt`: fatal; sample groups: both kept, first found | §5.1, §6.1, §4.2, §7.4 |
| Case of names | keywords `stricmp`; **identity names** (game-type names, `Effects` words, input driver words) often `strcmp` — marked (cs) | per parser |
| Numbers | §1.3 (`INT`, `FLOAT`, `F32`); decimal point is always `.` (C locale; `setlocale(LC_CTYPE, …)` is the only locale call and does not affect `atof`) [C] `code/app.cpp:App::SetLanguage` | – |
| Positional binding | `stats.txt` rows and `effects.txt` blocks bind by **order**, not by name; `gestures.txt` symbols likewise | §7.1, §7.5, §7.2 |
| Always end files with a newline | see §3.1 | – |

**Verification.** Derived from the per-format parser re-reads listed in each section; the blank-line hazards were checked against all shipped files by `X1a_level_survey.py` (118 level files: 0 violations) and `X1b_sounds_grammar.py` (`sounds.txt`: 0 hazards).

**Minimal valid example** (a two-field block in the level-file style; blank lines allowed because that parser checks `TokenAvailable`):

```text
Lights_StartDefinition

    0.0 1.0 0.0   0.8 0.8 0.8
Lights_EndDefinition
```

**Discrepancies.** The same keyword set is parsed by two independent parsers in one place (the menu scanner and `LevelFile` read `MultiwiniaOptions` with different case rules, §4.1.4) — a new engine should derive both from one table.

<a id="fmt-redshirt"></a>
### 3.3 "redshirt2" obfuscated text

**Purpose.** Light obfuscation of text written into the user profile by non-debug builds: campaign `game.txt` and mission saves ([fmt-profile-save](#fmt-profile-save)). Detection is automatic in `TextReader::CleanLine`, so any 8-bit text resource *could* be obfuscated; none under `data/` is [C] grep for `redshirt2` in `data/` = 0 hits; `X1a_text_survey.py` column `redshirt2` = 0 for 190 files.

**Files.** Written by `contrib/systemiv/lib/filesys/text_file_writer.cpp:TextFileWriter::TextFileWriter(path, _encrypt = true)` from `code/global_world.cpp:GlobalWorld::SaveGame` and `code/level_file.cpp:LevelFile::SaveMissionFile` unless `TARGET_DEBUG`; a UTF-16 variant exists in `code/unicode/unicode_text_file_writer.cpp:UnicodeTextFileWriter` [C]. Preferences are written with `encrypt = false` [C] `code/lib/preferences.cpp:PrefsManager::Save`.

**Layout and algorithm** [C] `TextFileWriter::TextFileWriter`, `TextFileWriter::printf`, `TextReader::CleanLine`:

```ebnf
obfuscated_file = 'redshirt2' , encoded_text ;      (* 9 ASCII bytes, NO newline: the first content line follows on the same physical line *)
```

| Item | Value |
|---|---|
| Magic | `redshirt2`, written once by the constructor with `fprintf`; the reader compares the first 9 characters of the first line with `strnicmp` (case-insensitive) and removes them |
| Key table `s_offsets[16]` | 31, 7, 9, 1, 11, 2, 5, 5, 3, 17, 40, 12, 35, 22, 27, 2 |
| Counter | `k` = 0 per file; persists across lines and `printf` calls; **pre-incremented** (`k = (k+1) mod 16`) before each use, so the first offset used is `s_offsets[1]` = 7 |
| Encoded characters | `c > 32` as a signed `char` (i.e. 33…127); blanks, TAB, CR, LF and bytes ≥ 128 pass unchanged and do **not** advance `k` |
| Encode | `e = c + s_offsets[k]; if e ≥ 128 then e −= 95` |
| Decode | `p = e − s_offsets[k]; if p < 33 then p += 95` — a bijection on 33…127 |
| Order w.r.t. comments | decoding happens before comment stripping, so only a decoded `#` starts a comment |

UTF-16 reader [C] `UnicodeTextReader::CleanLine`: it calls `m_unicodeline.StrOffsetCmp(L"redshirt2", 9, true)`, which on MSVC is `wcsncmp(m_unicodestring + 9, L"redshirt2", 9)` — it compares characters 9…17 of the first line, case-sensitively, so a UTF-16 file beginning with `redshirt2` is **not** recognised. Resolved: X1b stated that the UTF-16 reader recognises the marker; P14 stated the offset bug; `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::StrOffsetCmp` confirms P14.

**Verification.** Writer and reader re-read (code above); the key table is identical in `text_stream_readers.cpp`, `unicode_text_stream_reader.cpp` and `text_file_writer.cpp` [C]. No shipped sample exists, so the algorithm was checked by hand on the example below.

**Minimal valid example** (worked by hand). Plaintext line `Kills 7`: `K`(75)+7=82 `R`; `i`(105)+9=114 `r`; `l`(108)+1=109 `m`; `l`(108)+11=119 `w`; `s`(115)+2=117 `u`; blank unchanged, `k` stays 5; `7`(55)+5=60 `<`. Stored file (no newline after the magic): `redshirt2Rrmwu <` followed by CRLF.

**Discrepancies / hazards.** The three-angle-bracket debug assert runs on decoded text only; the scheme gives no security; a new game should not keep it (use a versioned binary save).

<a id="fmt-binary-common"></a>
### 3.4 BinaryReader conventions

**Purpose.** All binary assets (BMP, Ogg, CRC input) are read through one reader interface with a disk and a memory implementation [C] `contrib/systemiv/lib/filesys/binary_stream_readers.cpp`, chosen by `code/lib/resource.cpp:Resource::GetBinaryReader` (loose `data/N` first, then archive key `data/N`; mod paths [B:DEAD]).

| Operation | `BinaryFileReader` (disk) | `BinaryDataReader` (archive / memory) |
|---|---|---|
| Byte order | little-endian composites: `ReadS16 = b1 | b2<<8`, `ReadS32 = b1 | b2<<8 | b3<<16 | b4<<24` | same |
| `ReadU8`/`ReadS8` past the end | returns `(char)EOF` (0xFF) and sets `m_eof` | same (`m_offset >= m_dataSize` → `m_eof = true`, return `EOF`) |
| `ReadS16`/`ReadS32` short | returns a partial composite, sets `m_eof` | returns 0, sets `m_eof` |
| `ReadBytes(n)` | `fread`; `m_eof` only if fewer than n bytes | sets `m_eof` when the last byte of the data is consumed even if all n were read; returns 0 when already at EOF |
| `Seek(SEEK_END, off)` | `fseek` | `offset = size − off` |
| `GetFileType()` | extension after the last `.` | same |

**Verification.** `binary_stream_readers.cpp` re-read for `BinaryDataReader::ReadU8`, `ReadS16`; other rows from P14 §29.B.1 [C]. Consequence [I]: a parser that tests `m_eof` after a final successful `ReadBytes` behaves differently for loose and archived files; no shipped parser in scope does so (the BMP loader debug-asserts `!m_eof` only before the pixel rows).

**Minimal valid example.** Bytes `34 12 78 56` read with `ReadS16` twice give 0x1234 and 0x5678; read with `ReadS32` once give 0x56781234.

**Discrepancies.** None found beyond the EOF asymmetry above.

<a id="fmt-directory"></a>
### 3.5 `Directory` serialisation (network wire container)

**Purpose.** Every client↔server letter, every metaserver/matchmaker message, the `.mwm` map container and `serverHistory.dat` are serialised `Directory` trees: a named node with typed named fields and child nodes [C] `contrib/systemiv/lib/tosser/directory.cpp:Directory::Read`, `Directory::Write`, `DirectoryData::Read`, `DirectoryData::Write`; primitives `contrib/systemiv/lib/network_stream.cpp:WritePackedInt`, `ReadPackedInt`, `WriteDynamicString`, `ReadDynamicString`, `WriteUnicodeString`, `ReadUnicodeString`, `ReadVoidData`, `contrib/systemiv/lib/network_stream_inlines.cpp:WriteNetworkValue`, `ReadNumbersData`. Message catalogue and field codes: [22](22_determinism_and_networking.md).

**Encoding.** Binary, little-endian (values are written as their in-memory bytes on x86 and byte-reversed on big-endian hosts by `WriteNetworkValue`) [C].

**Byte layout**

```ebnf
directory   = '<' , name:dynstring , numData:packedint , numData * field , numSubdirs:packedint , numSubdirs * directory , '>' ;
field       = name:dynstring , type:u8 , value(type) ;
value(1)    = i32 ;                         (* INT *)
value(2)    = f32 ;                         (* FLOAT, raw IEEE-754 bits *)
value(3)    = u8 ;                          (* CHAR; unsigned char fields are stored as CHAR *)
value(4)    = dynstring ;                   (* STRING *)
value(5)    = u8 ;                          (* BOOL, written 0/1 *)
value(6)    = len:i32 , len * byte ;        (* VOID blob *)
value(7)    = 8 * byte ;                    (* FIXED, only with USE_FIXED - not compiled *)
value(8)    = count:packedint , count * i32 ;   (* INTS *)
value(9)    = u64 ;                         (* ULLONG *)
value(10)   = count:packedint , count * u16 ;   (* UNICODE, UTF-16LE code units; Win32 wchar_t written directly *)
dynstring   = len:packedint , len * byte ;  (* no terminator; len = -1 means NULL *)
packedint   = u8 (0..254)  |  0xFF , i32 ;  (* any value < 0 or >= 255 takes the 5-byte form *)
```

**Field table**

| Element | Type | Default / safe value on read | Range / limit | Semantics | Parser ref |
|---|---|---|---|---|---|
| start / end marker | 1 byte `<` / `>` | – | must match, else `Read` returns false (the whole datagram is rejected as a "bogus letter") | framing | `Directory::Read` |
| node name | dynstring | `"[SAFESTRING]"` if length < −1 or > 10 240 | `DIRECTORY_MAXSTRINGLENGTH` = 10 240 | message / sub-node name (game messages: `"ba"`) | `ReadDynamicString` |
| `numData` | packedint | – | 0…1024 (`DIRECTORY_MAXDIRSIZE`), else fail | number of fields; indices are not preserved (writer emits used slots in order) | `Directory::Read`, `Directory::Write` |
| `numSubdirs` | packedint | – | **not validated** (the check re-tests `numData`) | number of children | `Directory::Read` |
| field type | u8 | unknown type → `DirectoryData::Read` returns false | 1–10 (7 only under `USE_FIXED`) | – | `DirectoryData::Read` |
| VOID length | i32 | – | **unbounded** (`new char[len]`) | blob | `ReadVoidData` |
| INTS count | packedint | – | `count × 4 < 10 240`, else `AppAssert` — **fatal** | int array | `ReadNumbersData` |
| UNICODE length | packedint | `L"[SAFESTRING"` (sic, missing `]`) when out of range | ≤ 10 240 units | – | `ReadUnicodeString` |
| typed getters on missing/mistyped data | – | INT −1, FLOAT −1.0, CHAR `'?'` (63), STRING `"[SAFESTRING]"`, BOOL false, ULLONG 0xFFFFFFFFFFFFFFFF, UNICODE `L"[SAFESTRING"` | – | safe defaults | `contrib/systemiv/lib/tosser/directory.h` (`DIRECTORY_SAFE*`) |

Other rules [C]: `CreateData(name, …)` replaces an existing field of the same name; name lookup is linear and case-sensitive (`strcmp`), sub-directory paths use `/`; `Directory::Write(int &length)` returns a heap buffer with a trailing NUL not counted in `length`. When a dynstring length is out of range the bytes are **not skipped**, so the rest of the stream is misparsed [C] `ReadDynamicString`. Datagrams are received into 1 500-byte buffers (`MAX_PACKET_SIZE`), so a serialised letter larger than 1 500 bytes is never delivered [I, P15].

**Verification.** `Directory::Read`, `DirectoryData::Read/Write`, `ReadPackedInt`, `WritePackedInt`, `ReadDynamicString`, `ReadUnicodeString`, `ReadVoidData`, `ReadNumbersData`, `WriteNetworkValue` re-read for this document. No shipped sample; sizes cross-checked against P15 §4.3 (node overhead 7 bytes for name `ba`).

**Minimal valid example** (written for this document): a node `ab` with one INT field `cd` = 5 and no children, 15 bytes:

```text
3C                    '<'
02 61 62              name "ab"
01                    numData = 1
02 63 64 01 05000000  field "cd", type 1 (INT), value 5
00                    numSubdirs = 0
3E                    '>'
```

**Discrepancies / hazards** (remote-input robustness): unvalidated sub-directory count and VOID length, fatal INTS assert, desynchronising safe-string substitution — all reachable from any datagram [C]. A new engine should use a length-prefixed, schema-checked message format with hard size limits.

---

## 4 Level and game-definition files

<a id="fmt-level-file"></a>
### 4.1 Level files — map and mission (`map_*`, `mp_*`, `mission_*`)

#### 4.1.1 Purpose, files, build reality

A level is **one map file** (terrain, lights, static buildings, Multiwinia options — and in Multiwinia also units and camera data) plus an optional **mission file** (dynamic buildings, units, objectives, camera data; Darwinia campaign only). `LevelFile(missionName, mapName, textFileLevel)` parses the mission file first (unless its name is `null`), then the map file, then `GenerateAutomaticObjectives` [C] `code/level_file.cpp:LevelFile::LevelFile`. `Location::LoadLevel` passes `textFileLevel = (extension of the map name == "txt")`; any other extension is loaded as a `.mwm` container ([fmt-mwm](#fmt-mwm)) [C] `code/location.cpp:Location::LoadLevel`. In Multiwinia the requested mission is always `"null"` (set in `App::Initialise`, again when a client receives the map name, and by the editor) so only the map file is read [C] `code/app.cpp:App::Initialise`, `code/network/clienttoserver.cpp:ClientToServer::ProcessServerLetters`, `code/interface/multiwinia_window.cpp`.

| File set | Count | Kind | Loaded by | Build tag |
|---|---|---|---|---|
| `data/levels/mp_*.txt` | 57 | map | lobby map list (`GameMenu::CreateMapList`, `ListResources("levels/","mp_*")`) and `LevelFile` | [B:FULL]; 8 (`mp_assault_stress`, `mp_benchmark_1`, `mp_sandbox`, `mp_sandbox_all`, `mp_tankbattle_2P`, `mp_tankbattle_3P`, `mp_tankbattle_4P`, `mp_cts_3p_3`) are excluded from the retail `main.dat` by `tools/multiwinia_exclusions.txt`, leaving **49 shipped maps** (= `VALID_ACHIEVEMENT_LEVELS` 49 in `code/achievement_tracker.cpp`) [C] |
| `data/levels/map_*.txt` | 12 | map | Darwinia campaign via `game.txt` | [B:LEGACY]; excluded from retail (`levels/map_*`) |
| `data/levels/mission_*.txt` | 17 | mission | Darwinia campaign | [B:LEGACY]; excluded (`levels/mission_*`) |
| `data/levels/old/*.txt` | 30 | map | nothing — `ListResources` matches only the exact directory `data/levels/` (`stricmp(dirPart, desiredDir)`) [C] `code/lib/resource.cpp:Resource::ListResources` | [B:DEAD] data; excluded (`levels/old/*`) |
| `data/purity/map_mine.txt`, `data/purity/mission_mine_enable.txt` | 2 | map / mission | nothing (no code opens `data/purity/`) | [B:DEAD]; excluded (`purity`) |

**Encoding.** Single-byte ASCII, CRLF, every level file newline-terminated [C] survey. Map files are read through `Resource::GetTextReader("levels/<name>")`; if that fails, `<profile>/maps/<name>` with a plain `TextFileReader`; the release assert `"Invalid map file specified"` fires if neither opens [C] `LevelFile::ParseMapFile(char const*)`. Mission files try the campaign save `<P>users/<profile>/<name>` first (unless editing), then `levels/<name>`; `"Invalid level specified"` (fatal) if neither [C] `LevelFile::ParseMissionFile`.

#### 4.1.2 Top-level grammar and block dispatch

```ebnf
level_file   = { blank_line | block | stray_line } ;
block        = start_line , block_body , end_line ;            (* body grammar per block, §4.1.4 - §4.1.15 *)
start_line   = BLOCKNAME '_StartDefinition' , { ignored_token } , EOL ;
end_line     = BLOCKNAME '_EndDefinition' , { ignored_token } , EOL ;   (* recognised only as the FIRST token *)
stray_line   = any other first token , EOL ;                   (* AppDebugAssert(0); ignored in release *)
```

The dispatcher reads the first token of every non-blank line and hands the reader to the block parser, which consumes lines up to and including its own end keyword [C] `LevelFile::ParseMapFile(TextReader*)`, `LevelFile::ParseMissionFile`. A block whose end keyword is missing swallows the rest of the file.

| Block (`X_StartDefinition`) | In a map file | In a mission file | Parser | Notes |
|---|---|---|---|---|
| `MultiwiniaOptions` | yes | rejected (debug assert, body lines re-read at top level) | `ParseMultiwiniaOptions` | Multiwinia only; must be first so the menu scanner can stop early (§4.1.4) |
| `Landscape` | yes | rejected | `ParseLandscapeData` | **no blank lines** inside |
| `LandscapeTiles` | yes | rejected | `ParseLandscapeTiles` | |
| `LandFlattenAreas` | yes | rejected | `ParseLandFlattenAreas` | |
| `Lights` | yes | rejected | `ParseLights` | first non-empty block wins |
| `Buildings` | yes → **static** (`m_dynamic = false`) | yes → **dynamic** | `ParseBuildings(in, dynamic)` | `ParseMapFile` has a second, unreachable `Buildings` branch with `dynamic = true`; every map-file block is static |
| `CameraMounts` | yes | yes | `ParseCameraMounts` | |
| `CameraAnimations` | yes | yes | `ParseCameraAnims` | |
| `InstantUnits` | yes | yes | `ParseInstantUnits` | |
| `Routes` | yes | yes | `ParseRoutes` | no shipped file uses it |
| `PrimaryObjectives` | yes | yes | `ParsePrimaryObjectives` | **no blank lines** inside |
| `RunningPrograms` | — | yes | `ParseRunningPrograms` | save game only |
| `Difficulty` | — | yes | `ParseDifficulty` | |
| `LeaderboardStats` | — | yes | `ParseLeaderboardStats` | save game only |

Block order written by `LevelFile::SaveMapFile`: `MultiwiniaOptions` (only when `g_app->m_multiwinia` exists), `Landscape`, `LandscapeTiles`, `LandFlattenAreas`, `Lights`, `Buildings`, then (Multiwinia) `CameraMounts`, `InstantUnits`. Mission files: `Difficulty`, `CameraMounts`, `CameraAnimations`, `Buildings`, `InstantUnits`, `Routes`, `PrimaryObjectives`, `RunningPrograms`, `LeaderboardStats` [C] `LevelFile::SaveMapFile`, `LevelFile::SaveMissionFile`. Shipped orders [C] `X1a_level_survey.py` (re-run):

| File set | Files | Block order |
|---|---|---|
| mp | 47 | MultiwiniaOptions > Landscape > LandscapeTiles > LandFlattenAreas > Lights > Buildings > CameraMounts > CameraAnimations > Buildings > InstantUnits > PrimaryObjectives |
| mp | 10 | MultiwiniaOptions > Landscape > LandscapeTiles > LandFlattenAreas > Lights > Buildings > CameraMounts > InstantUnits |
| map | 11 | Landscape > LandscapeTiles > LandFlattenAreas > Lights > Buildings |
| map | 1 | Landscape > LandscapeTiles > LandFlattenAreas > Lights > Buildings > CameraMounts > CameraAnimations > Buildings > InstantUnits > PrimaryObjectives |
| mission | 14 | CameraMounts > CameraAnimations > Buildings > InstantUnits > PrimaryObjectives |
| mission | 3 | Difficulty > CameraMounts > CameraAnimations > Buildings > InstantUnits > PrimaryObjectives |
| old | 28 | MultiwiniaOptions > … (same as the 47-file mp order) |
| old | 2 | Landscape > … > PrimaryObjectives (as the 1-file map order) |
| purity | 1 + 1 | mission order (14-file form); map order (11-file form) |

#### 4.1.3 Object defaults before parsing [C] `LevelFile::LevelFile`, `code/level_file.h:LandscapeDef`, `LandscapeFlattenArea`, `InstantUnit`, `CamAnimNode`

| Member | Default | Member | Default |
|---|---|---|---|
| `m_landscapeColourFilename` | `landscape_default.bmp` | `m_levelName` | `NewLevel` |
| `m_wavesColourFilename` | `waves_default.bmp` | `m_description` | `LevelDescription` |
| `m_waterColourFilename` | `water_default.bmp` | `m_author` | `Author` |
| `m_thumbnailFilename` | `default.bmp` (no keyword sets it; `.mwm` only) | `m_difficultyRating` | NULL |
| `m_levelDifficulty` | −1 | `m_gameTypes` | −1 |
| `m_populationCap`, `m_defenderPopulationCap`, `m_numPlayers`, `m_defaultPlayTime` | −1 | `m_coop`, `m_forceCoop`, `m_official` | false |
| `m_levelOptions[0..14]` | −1 (`memset`), then options 0, 1, 4, 9, 10 set to 0 | `m_effects[0..4]`, `m_invalidCrates[*]`, `m_attacking[0..5]` | false |
| `m_mapId` | −1 | `m_leaderboardLevelTime` / `m_lastLeaderboardLevelTime` | 0.0 / 9 999 999.0 |
| `LandscapeDef.m_cellSize` | 12.0 | `LandscapeDef.m_worldSizeX/Z` | 2000 / 2000 |
| `LandscapeDef.m_outsideHeight` | −10 | `LandscapeDef.m_maxHeight` | 0.0 |

Buffers [C] `code/level_file.h`: `m_levelName`, colour file names: `char[MAX_FILENAME_LEN]` = 512; `m_author` `char[64]`; `m_description` `char[512]`; all filled with unchecked `strcpy`.

#### 4.1.4 `MultiwiniaOptions` block [C] `LevelFile::ParseMultiwiniaOptions`

```ebnf
mw_options   = 'MultiwiniaOptions_StartDefinition' EOL , { mw_line } , 'MultiwiniaOptions_EndDefinition' EOL ;
mw_line      = blank
             | 'PopulationCap' INT | 'DefenderPopulationCap' INT | 'NumPlayers' INT | 'PlayTime' INT
             | 'GameTypes' GAMETYPE { GAMETYPE }                 (* LevelFile keeps only the first token *)
             | 'Effects' { EFFECT }
             | 'LevelOptions' { level_option }
             | 'InvalidCrates' { CRATENAME }
             | 'AttackingTeams' { INT }
             | 'CoopGroup1Positions' { INT } | 'CoopGroup2Positions' { INT }
             | 'CoopMode' | 'ForceCoop' | 'OfficialLevel'
             | 'Name' REST | 'Author' REST | 'Description' REST
             | 'Difficulty' WORD
             | OTHER_WORD { any } ;                              (* silently ignored *)
level_option = 'nohandicap' | 'instantspawnpointcapture' | 'nogunturrets' | 'forceradarteammatch' | 'noradarcontrol'
             | ( 'trunkportreinforcements' | 'trunkportarmour' | 'attackerreinforcements' | 'defenderreinforcements'
               | 'airstrikecapturezones' | 'futurewinianteam' | 'attackreinforcementoption' | 'defendreinforcementoption' ) INT
             | ( 'defenderreinforcementtype' | 'attackerreinforcementtype' ) RESEARCHNAME
             | OTHER_WORD ;                                      (* ignored, no argument consumed *)
GAMETYPE     = 'Domination' | 'KingOfTheHill' | 'CaptureTheStatue' | 'Assault' | 'RocketRiot' | 'Blitzkrieg' | 'TankBattle' ;  (* (cs) *)
EFFECT       = 'snow' | 'dustwind' | 'soulrain' | 'lightning' | 'SpecialLighting' ;                                              (* (cs) *)
```

Case and failure rules [C]: first tokens and `level_option` words are case-insensitive in `LevelFile` (`stricmp`) but **case-sensitive in the menu scanner** (`code/game_menu.cpp:GameMenu::AddMap`, `strcmp`); `GAMETYPE` must equal a blueprint name of [fmt-multiwinia-txt](#fmt-multiwinia-txt) by `strcmp` in both; `EFFECT` words are `strcmp`. Every scalar `INT`/`WORD`/`RESEARCHNAME`/`REST` argument is MISSING ⇒ CRASH; list keywords check `TokenAvailable`. A scalar keyword may repeat — the last wins (`mp_cts_4p_3.txt` has `PopulationCap -1` then `PopulationCap 2000` → 2000); lists accumulate.

| Keyword | Type | Units | Default | Range / valid values (shipped) | Semantics (consumer) | Parser ref |
|---|---|---|---|---|---|---|
| `PopulationCap` | INT | Darwinians | −1 (no cap) | ≥1 or −1; shipped −1…2500 | total cap; each team's cap = cap / `NumPlayers` [C] `code/worldobject/spawnpoint.cpp:SpawnPoint` ([05](05_economy_and_population.md)) | `ParseMultiwiniaOptions` |
| `DefenderPopulationCap` | INT | Darwinians | −1 | −1, 200, 250, 275, 350 | Assault defender cap [C] `code/worldobject/spawnpoint.cpp`, `code/worldobject/trunkport.cpp` | same |
| `NumPlayers` | INT | start positions | −1 | 2–4 (absent only in `mp_sandbox_all.txt`) | lobby slot count; Assault rotation [C] `code/multiwinia.cpp`, `GameMenu::AddMap` | same; menu scanner |
| `PlayTime` | INT | minutes | −1 | 5, 10, 15, 20 | `MapData::m_playTime`, suggested match length [C] `GameMenu::AddMap`; lobby use [I] ([09](09_meta_systems.md)) | same; menu scanner |
| `GameTypes` | GAMETYPE list | – | −1 | blueprint names | `LevelFile`: index of the **first** token's blueprint → `m_gameTypes`; menu: **every** matching token adds the map to that type's list (a token repeated twice adds the map twice); a map with no matching token is discarded ("didn't contain a game type") | same; `GameMenu::AddMap` |
| `Effects` | EFFECT list | – | none | `snow`→`LevelEffectSnow`, `dustwind`→`LevelEffectDustStorm`, `soulrain`→`LevelEffectSoulRain`, `lightning`→`LevelEffectLightning`, `SpecialLighting`→`LevelEffectSpecialLighting` | weather / lighting effects run by `Location::Advance` [C] `code/location.cpp` ([23](23_terrain_and_landscape.md), [24](24_rendering.md)) | same |
| `LevelOptions` | option list | – | §4.1.3 | §4.1.5 | per-level rule switches | same |
| `InvalidCrates` | crate names | – | none | names of `Crate::GetName(i)` (`stricmp`), 0 ≤ i < `NumCrateRewards` | reward excluded on this map [C] `code/worldobject/crate.cpp` ([04](04_crates_powerups_and_weapons.md)) | same |
| `AttackingTeams` | INT list | team position | none | 0…5; **no bounds check** — out-of-range writes past `m_attacking[6]` | Assault attacking positions [C] `code/multiwinia.cpp` | same |
| `CoopGroup1Positions`, `CoopGroup2Positions` | INT list | position | empty | ≥ 2 entries expected (`Location` reads [0] and [1]) | co-op pairing [C] `code/location.cpp` | same |
| `CoopMode` | flag | – | false | – | co-op allowed | same; menu scanner |
| `ForceCoop` | flag | – | false | – | co-op mandatory | same; menu scanner |
| `OfficialLevel` | flag | – | false | – | `m_official = true`, `m_mapId = 0`; counted by the menu for official-map achievements | same; menu scanner |
| `Name` | REST | – | `NewLevel` | ≤ 511 bytes | display name; the 10 shipped files that have it say `NewLevel` — real names come from the language table [I] | same; menu scanner |
| `Author` | REST | – | `Author` | ≤ 63 bytes (`char[64]`, unchecked) | unused by shipped maps; **the menu scanner stops scanning at `Author`** (`break`) | same; menu scanner |
| `Description` | REST | – | `LevelDescription` | ≤ 511 bytes | unused by shipped maps | same; menu scanner |
| `Difficulty` | WORD | – | NULL | `basic`, `intermediate`, `advanced`; menu sorts by substring `basic` < `inter` < `advan`, other = 0 | lobby sort and badge | same; menu scanner |

**Menu scanner (second, independent parser)** [C] `code/game_menu.cpp:GameMenu::AddMap`, `GameMenu::CreateMapList`: reads every `data/levels/mp_*` file (loose and archived; `.mwm` from `<P>maps/` only under `LOCATION_EDITOR`), compares the first token of each line **case-sensitively** with `Name`, `GameTypes`, `NumPlayers`, `Author`, `Description`, `PlayTime`, `Difficulty`, `CoopMode`, `ForceCoop`, `OfficialLevel`, and stops at the first `MultiwiniaOptions_EndDefinition` or at an `Author` line. It does not check that it is inside the options block (a file without the block is scanned to the end). It then computes the map CRC ([fmt-map-crc](#fmt-map-crc)) and sorts each game-type list by `NumPlayers`, then difficulty.

#### 4.1.5 `LevelOptions` words (`LevelFile::NumLevelOptions` = 15) [C] `code/level_file.h` enum, `ParseMultiwiniaOptions`

| # | Enum | Word | Argument | Default | Shipped values | Consumer / semantics |
|---|---|---|---|---|---|---|
| 0 | `LevelOptionNoHandicap` | `nohandicap` | – (sets 1) | 0 | 1 file (old/) | handicap forced off [C] `code/worldobject/spawnpoint.cpp` |
| 1 | `LevelOptionInstantSpawnPointCapture` | `instantspawnpointcapture` | – | 0 | 5 files | no 30 s capture timer [C] `spawnpoint.cpp` |
| 2 | `LevelOptionTrunkPortReinforcements` | `trunkportreinforcements` | INT Darwinians per wave | −1 | 60, 75, 100, 250 | trunk-port reinforcement waves [C] `code/multiwinia.cpp` |
| 3 | `LevelOptionTrunkPortArmour` | `trunkportarmour` | INT max armour | −1 | 1, 3 | trunk-port armour spawns [C] `code/multiwinia.cpp`, `code/worldobject/crate.cpp` |
| 4 | `LevelOptionNoGunTurrets` | `nogunturrets` | – | 0 | 2 files | no GunTurret reinforcement tasks [C] `code/multiwinia.cpp` |
| 5 | `LevelOptionAttackerReinforcements` | `attackerreinforcements` | INT | −1 | none | Assault attacker reinforcement count |
| 6 | `LevelOptionDefenderReinforcements` | `defenderreinforcements` | INT | −1 | none | Assault defender reinforcement count |
| 7 | `LevelOptionDefenderReinforcementType` | `defenderreinforcementtype` | research name → `GlobalResearch::GetType` (−1 unknown) | −1 | `AirStrike`, `Armour`, `DropShip` (old/) | defender reinforcement task type |
| 8 | `LevelOptionAttackerReinforcementType` | `attackerreinforcementtype` | research name | −1 | `HotFeet`, `AirStrike` (old/) | attacker reinforcement task type |
| 9 | `LevelOptionForceRadarTeamMatch` | `forceradarteammatch` | – | 0 | 1 file | radar dishes link only when same owner [C] `code/worldobject/radardish.cpp` |
| 10 | `LevelOptionNoRadarControl` | `noradarcontrol` | – | 0 | 1 file | radar dishes cannot be aimed [C] `code/location_input.cpp`, `code/worldobject/ai.cpp`, `code/gamecursor.cpp` |
| 11 | `LevelOptionAirstrikeCaptureZones` | `airstrikecapturezones` | INT zones | −1 | 3 | Blitzkrieg: zones before turrets become airstrikes [C] `code/multiwinia.cpp` |
| 12 | `LevelOptionFuturewinianTeam` | `futurewinianteam` | INT team position | −1 | 2 (old/) | that position fields Futurewinians [C] `code/team.cpp`, `code/worldobject/darwinian.cpp` |
| 13 | `LevelOptionAttackReinforcementMode` | `attackreinforcementoption` | INT | −1 | 1, 2 (old/) | `task->m_option` for attacker reinforcements |
| 14 | `LevelOptionDefendReinforcementMode` | `defendreinforcementoption` | INT | −1 | none | `task->m_option` for defender reinforcements |

Semantics and units of every option in play: [01](01_game_modes.md), [03](03_buildings_and_world_objects.md), [05](05_economy_and_population.md). Writer quirk [C] `LevelFile::WriteMultiwiniaOptions`: only options whose value is **> 0** are written (research type 0 = `Darwinian` can never be saved), and the `futurewinianteam` case falls through into `attackreinforcementoption` (missing `break`), so a saved map with a Futurewinian team gains a spurious `attackreinforcementoption <same value>`.

#### 4.1.6 `Landscape` block [C] `LevelFile::ParseLandscapeData`, `WriteLandscapeData`

```ebnf
landscape = 'Landscape_StartDefinition' EOL , { ls_line } , 'Landscape_EndDefinition' EOL ;
ls_line   = KEY , VALUE , { ignored } , EOL ;          (* KEY case-insensitive; unknown KEY ignored; NO blank/comment lines *)
```

The loop calls `GetNextToken()` with no `TokenAvailable()` check, so a blank or comment-only line passes `NULL` to `stricmp` (crash); a known key without value passes `NULL` to `atoi`/`iv_atof`/`strcpy` (crash). The end keyword is tested last, after the keys [C].

| Key | Type | Units | Default | Shipped range / values | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `worldSizeX` | INT | world units (wu) | 2000 | all 100 map files | landscape extent along x | `ParseLandscapeData` |
| `worldSizeZ` | INT | wu | 2000 | all | extent along z | same |
| `cellSize` | F32 | wu per height-map cell | 12.0 | all | height-map resolution | same |
| `outsideHeight` | F32 | wu | −10 | all | height outside the tiles (sea floor) | same |
| `maxHeight` | FLOAT (`atof`, not F32-rounded) | wu | 0.0 | 41 mp files | upper height clamp, 0 = none ([23](23_terrain_and_landscape.md)) | same |
| `landColourFile` | WORD ≤ 511 | file in `data/terrain/` | `landscape_default.bmp` | all | slope × height colour ramp ([fmt-colour-ramp](#fmt-colour-ramp)) | same |
| `wavesColourFile` | WORD ≤ 511 | file in `data/terrain/` | `waves_default.bmp` | all | wave colour table | same |
| `waterColourFile` | WORD ≤ 511 | file in `data/terrain/` | `water_default.bmp` | all | tiled sea texture | same |

Algorithmic meaning of each field: [23](23_terrain_and_landscape.md).

#### 4.1.7 `LandscapeTiles` block [C] `LevelFile::ParseLandscapeTiles`, `code/landscape.cpp:LandscapeTile::GuideGridSetPower`, `GuideGridFromString`, `GuideGridToString`

```ebnf
tiles     = 'LandscapeTiles_StartDefinition' EOL , { blank | tile_row } , 'LandscapeTiles_EndDefinition' EOL ;
tile_row  = posX posY posZ size fractalDim heightScale desiredHeight genMethod seed lowlandSmoothing guidePower [ guideGrid ] EOL ;
guideGrid = { LETTER LETTER } ;    (* exactly 2*(2^guidePower - 1)^2 letters; present iff guidePower > 0 *)
LETTER    = 'A'..'P' ;             (* value = c - 'A' (0..15); cell = 16*hi + lo (0..255) *)
```

| Col | Field | Type | Units | Default | Range (shipped) | Semantics | Parser ref |
|---|---|---|---|---|---|---|---|
| 1 | `m_posX` | INT (decimals truncated) | wu | – | – | tile origin x | `ParseLandscapeTiles` |
| 2 | `m_posY` | F32 | wu | – | – | vertical offset | same |
| 3 | `m_posZ` | INT | wu | – | – | tile origin z | same |
| 4 | `m_size` | INT | wu | – | – | tile edge length | same |
| 5 | `m_fractalDimension` | F32 | – | – | – | roughness | same |
| 6 | `m_heightScale` | F32 | – | – | – | height multiplier | same |
| 7 | `m_desiredHeight` | F32 | wu | – | – | target peak height | same |
| 8 | `m_generationMethod` | INT | enum | – | 0, 1 | generator ([23](23_terrain_and_landscape.md)) | same |
| 9 | `m_randomSeed` | INT | – | – | – | private per-tile generator seed | same |
| 10 | `m_lowlandSmoothingFactor` | F32 | – | – | – | lowland smoothing | same |
| 11 | guide power | INT | – | – | 0…5 | grid resolution `2^p − 1` cells per side; 0 = no grid | `GuideGridSetPower` |
| 12 | guide grid | WORD | – | – | – | row-major, z outer, x inner; **no length check** — a short string reads past the token | `GuideGridFromString` |

All 11 leading columns are MISSING ⇒ CRASH. Shipped tile rows [C] survey: 209 with p = 0 (11 tokens); 41 / 292 / 40 / 101 / 46 with p = 1 / 2 / 3 / 4 / 5 (12 tokens) — all grid lengths exact.

#### 4.1.8 `LandFlattenAreas` block [C] `LevelFile::ParseLandFlattenAreas`

```ebnf
flatten_row = centreX centreY centreZ size [ flattenType heightThreshold ] EOL ;    (* blank lines allowed *)
```

| Col | Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|---|
| 1–3 | `m_centre` | F32 ×3 | wu | – | – | area centre (y = target height) | `ParseLandFlattenAreas` |
| 4 | `m_size` | F32 | wu (radius) [I] | 40.0 (ctor) | – | extent | same |
| 5 | `m_flattenType` | INT | enum | 0 (`FlattenTypeAbsolute`) when absent | 0 Absolute, 1 Add, 2 Subtract, 3 Smooth, 4 Subtract2 | flatten mode ([23](23_terrain_and_landscape.md)) | same |
| 6 | `m_heightThreshold` | F32 | wu | 40.0 (ctor) | – | read only when column 5 is present, then MISSING ⇒ CRASH | same |

Shipped: 14 rows with 4 tokens (campaign-era maps), 54 rows with 6 tokens [C] survey.

#### 4.1.9 `Lights` block [C] `LevelFile::ParseLights`, `WriteLights`

```ebnf
light_row = frontX frontY frontZ red green blue EOL ;       (* six F32, MISSING => CRASH; blank lines allowed *)
```

| Col | Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|---|
| 1–3 | `m_front` | F32 ×3 | direction | – | any non-zero | `(x,y,z,0)` normalised — an infinitely distant directional light | `ParseLights` |
| 4–6 | `m_colour` | F32 ×3 | intensity multipliers | – | unclamped; shipped up to 2.00 | `(r,g,b,0)` | same |

Precedence [C]: if any light already exists when a `Lights` block starts, the **whole block is ignored** (`ignoreLights`). The code comment says this lets a mission override map lights, but `ParseMissionFile` rejects `Lights`, so only a second `Lights` block in one map file is affected. Every shipped map has one block with 1–2 lights (198 rows / 100 files) [C] survey. The writer dumps the *current* `Location` lights with `%6.2f`.

#### 4.1.10 `Buildings` block [C] `LevelFile::ParseBuildings`, `code/worldobject/building.cpp:Building::CreateBuilding(char*)`, `Building::Read`, `Building::BuildingIsBlocked`, each subclass `Read`

```ebnf
buildings = 'Buildings_StartDefinition' EOL , { blank | b_row } , 'Buildings_EndDefinition' EOL ;
b_row     = TYPENAME id x z team frontX frontZ isGlobal { extra } EOL ;
```

Load rules [C]:

1. `TYPENAME` is compared case-insensitively with `Building::GetTypeName(i)` for i = 0…74. A blocked type (`BLOCK_OLD_DARWINIA_OBJECTS`, 39 types in FULL), `Invalid`, or an unknown name makes `CreateBuilding` return NULL and the **whole line is ignored** (no error).
2. Base columns (all MISSING ⇒ CRASH):

| Col | Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|---|
| 2 | `id` | INT | – | – | ≥ 0; unique within this `LevelFile` (map + mission) else `AppReleaseAssert("%s UniqueId was not unique in %s")` — **fatal** | `WorldObjectId(team, UNIT_BUILDINGS = −100, −1, id)`; target of all link columns | `Building::Read`, `ParseBuildings` |
| 3 | `x` | F32 | wu | – | – | position x (y from the landscape [I]) | `Building::Read` |
| 4 | `z` | F32 | wu | – | – | position z | same |
| 5 | `team` | INT → `unsigned char` | team id | – | 0…5 (`NUM_TEAMS` = 6, `code/globals.h`) or 255 = none; shipped 0 ×747, 1 ×1899, 2 ×352, 3 ×119, 255 ×5325 rows | owner | same |
| 6 | `frontX` | F32 | – | – | – | facing `(frontX, 0, frontZ)` normalised | same |
| 7 | `frontZ` | F32 | – | – | – | | same |
| 8 | `isGlobal` | BOOL | – | – | 0/1; **must be 1** for `TrunkPort`, `ControlTower`, `RadarDish`, `Incubator`, `FenceSwitch`, else `AppReleaseAssert("Non-global %s found in %s")` — fatal | campaign-global flag | `ParseBuildings` |

3. Extra columns are read by the subclass (table below); surplus tokens are ignored.
4. Mission rows get `m_dynamic = true`, map rows `false`.
5. Difficulty scaling (Darwinia legacy) for team 1: `SpawnPopulationLock.maxPopulation ×= f`, `AntHill.numAntsInside ×= f`, `AISpawnPoint.period /= f`, `Triffid.reloadTime /= f` with `f = 1 + difficultyLevel/5` when the level has no `Difficulty` block; `App::UpdateDifficultyFromPreferences` hard-codes `m_difficultyLevel = 0`, so `f = 1.0` [C] `code/app.cpp:App::UpdateDifficultyFromPreferences`.
6. Every row constructs a template object and some constructors draw synchronised random numbers, so the set and order of rows must be identical on every peer [C] ([22](22_determinism_and_networking.md)). Parsed objects are templates; `Location` later creates live buildings with `CreateBuilding(type)` + `Initialise(template)` — behaviour of every column: [03](03_buildings_and_world_objects.md).

**Per-type extra columns** (75 type names = enum 0…74, `NumBuildingTypes` = 75). Legend: R required (MISSING ⇒ CRASH); O optional, read only if a token remains, nested (a later O is read only if the earlier O was present); `*` = list to end of line. Row counts are over all 118 level files [C] `X1a_level_survey.py` → `_work/drafts/X1a_level_field_usage.csv`.

| # | Type name | Class (`Read`) | Extra columns after `isGlobal` | Optional defaults | Blocked in FULL | Shipped rows (files) |
|---|---|---|---|---|---|---|
| 0 | `Invalid` | – | – (NULL) | – | n/a | 0 |
| 1 | `Factory` | Factory | R `initialCapacity` INT | – | yes | 0 |
| 2 | `Cave` | Cave | – | – | yes | 0 |
| 3 | `RadarDish` | RadarDish | O `forceConnection` INT, O `autoConnectAtStart` INT, `validLinkId`* INT | 0, 0, empty | no | 164 (22) |
| 4 | `LaserFence` | LaserFence | R `nextLaserFenceId` INT, O `scale` F32, O `mode` F32→int (0 Disabled, 1 Enabling, 2 Enabled, 3 Disabling, 4 NeverOn, 5 AlwaysOn) | 1.0, 0 | no | 764 (17) |
| 5 | `ControlTower` | ControlTower | R `controlBuildingId` INT | – | no | 150 (20) |
| 6 | `GunTurret` | GunTurret | O `state` INT (turret type 0 Standard, 1 Rocket, 2 Mortar, 3 Flame, 4 Subversion), O group {`targetX` F32, `targetZ` F32, `targetRadius` F32, `targetForce` F32} (all four R once `targetX` is present) | 0, (0,0,0,0) | no | 264 (23) |
| 7 | `Bridge` | Bridge | R `nextBridgeId` INT, R `bridgeType` INT | – | yes | 0 |
| 8 | `Powerstation` | Powerstation | R `linkedBuildingId` INT | – | yes | 0 |
| 9 | `Tree` | Tree | R `height` F32, R `budsize` F32, R `pushOut` F32, R `pushUp` F32, R `iterations` INT, R `seed` INT, R `branchColour` INT (packed RGBA, signed decimal), R `leafColour` INT (packed RGBA), O `leafDropRate` INT, O `spiritDropRate` INT, O `evil` INT (== 1 → true) | 0, 0, false | no | 1041 (69) |
| 10 | `Wall` | Wall | O `objectiveLink` INT | −1 | no | 374 (12) |
| 11 | `TrunkPort` | TrunkPort | O `targetLocationId` INT | −1 | no | 173 (49) |
| 12 | `ResearchItem` | ResearchItem | R `researchType` research name (`GlobalResearch::GetType`, −1 unknown), O `level` INT | 1 | yes | 8 (6) ignored |
| 13 | `Library` | Library | – | – | yes | 0 |
| 14 | `Generator` | PowerBuilding | R `powerLink` INT | – | yes | 1 (1) ignored |
| 15 | `Pylon` | PowerBuilding | R `powerLink` INT | – | no | 389 (22) |
| 16 | `PylonStart` | PylonStart | R `powerLink` INT, R `reqBuildingId` INT | – | yes | 1 (1) ignored |
| 17 | `PylonEnd` | PowerBuilding | R `powerLink` INT | – | no | 98 (21) |
| 18 | `SolarPanel` | PowerBuilding | R `powerLink` INT | – | no | 274 (21) |
| 19 | `PowerSplitter` | PowerSplitter | R `powerLink` INT, `linkId`* INT | empty | no | 27 (8) |
| 20 | `TrackLink` | MineBuilding | R `trackLink` INT | – | yes | 111 (2) ignored |
| 21 | `TrackJunction` | TrackJunction | R `trackLink` INT, `trackLinks`* INT | – | yes | 3 (1) ignored |
| 22 | `TrackStart` | TrackStart | R `trackLink` INT, R `reqBuildingId` INT | – | yes | 1 (1) ignored |
| 23 | `TrackEnd` | TrackEnd | R `trackLink` INT, R `reqBuildingId` INT | – | yes | 2 (2) ignored |
| 24 | `Refinery` | MineBuilding | R `trackLink` INT | – | yes | 1 (1) ignored |
| 25 | `Mine` | MineBuilding | R `trackLink` INT | – | yes | 4 (1) ignored |
| 26 | `Yard` | ConstructionYard | – | – | yes | 1 (1) ignored |
| 27 | `DisplayScreen` | DisplayScreen | – | – | yes | 0 |
| 28 | `UpgradePort` | UpgradePort | – | – | yes | 0 |
| 29 | `PrimaryUpgrade` | PrimaryUpgradePort | – | – | yes | 0 |
| 30 | `Incubator` | Incubator | O `numStartingSpirits` INT | 0 | no | 96 (30) |
| 31 | `AntHill` | AntHill | R `numAntsInside` INT (< 0 or > 10 000 logged and replaced by 0) | – | no | 13 (2) |
| 32 | `SafeArea` | SafeArea | R `size` F32, R `entitiesRequired` INT, R `entityTypeRequired` INT (entity enum §4.1.13) | – | yes | 4 (3) ignored |
| 33 | `Triffid` | Triffid | R `size` F32, R `pitch` F32, R `force` F32, R `variance` F32, R `reloadTime` F32, R `useTrigger` INT, R `triggerX` F32, R `triggerZ` F32, R `triggerRadius` F32, then up to 10 O spawn flags INT (== 1 → true) in the order Virii, Centipede, Spider, Spirits, Eggs, TriffidEggs, Ants, SporeGenerator, SoulDestroyer, Darwinians | missing flags false | no | 47 (13) |
| 34 | `SpiritReceiver` | ReceiverBuilding | R `spiritLink` INT | – | yes | 49 (1) ignored |
| 35 | `ReceiverLink` | ReceiverBuilding | R `spiritLink` INT | – | yes | 83 (2) ignored |
| 36 | `SpiritSpawner` | ReceiverBuilding | R `spiritLink` INT | – | yes | 1 (1) ignored |
| 37 | `SpiritProcessor` | ReceiverBuilding | R `spiritLink` INT | – | yes | 1 (1) ignored |
| 38 | `SpawnPoint` | SpawnBuilding | `link`* INT (`SetBuildingLink` each) | empty | no | 471 (55) |
| 39 | `SpawnPopulationLock` | SpawnPopulationLock | R `searchRadius` F32 (wu), R `maxPopulation` INT | – | no | 166 (39) |
| 40 | `SpawnPointMaster` | MasterSpawnPoint | `link`* INT | empty | no | 55 (55) |
| 41 | `SpawnLink` | SpawnBuilding | `link`* INT | empty | no | 464 (37) |
| 42 | `AITarget` | AITarget | O `linkId` INT, O `priorityModifier` FLOAT (`atof`, not F32), O `checkCliffs` INT | −1, 0.0, 1 | no | 1577 (81) |
| 43 | `AISpawnPoint` | AISpawnPoint | R `activatorId` INT, R `entityType` INT, R `count` INT, R `period` INT, O `spawnLimit` INT, O `routeId` INT | −1, −1 | yes | 19 (6) ignored |
| 44 | `BlueprintStore` | BlueprintBuilding | R `buildingLink` INT | – | yes | 1 (1) ignored |
| 45 | `BlueprintConsole` | BlueprintConsole | R `buildingLink` INT, R `segment` INT | – | yes | 4 (1) ignored |
| 46 | `BlueprintRelay` | BlueprintRelay | R `buildingLink` INT, R `altitude` F32 | – | yes | 12 (1) ignored |
| 47 | `ScriptTrigger` | ScriptTrigger | R `linkId` INT, R `range` F32, R `scriptFilename` WORD (≤ 255), R `entityType` WORD (`always`, `never`, `camenter`, `camview`, or an entity type name; case-insensitive) | – | yes | 25 (11) ignored |
| 48 | `Spam` | Spam | – | – | yes | 3 (1) ignored |
| 49 | `GodDish` | GodDish | – | – | yes | 2 (2) ignored |
| 50 | `StaticShape` | StaticShape | R `scale` F32, R `shapeName` WORD (file in `data/shapes/`) | – | no | 257 (30) |
| 51 | `FuelGenerator` | FuelBuilding | R `fuelLink` INT | – | no | 64 (9) |
| 52 | `FuelPipe` | FuelBuilding | R `fuelLink` INT | – | no | 217 (9) |
| 53 | `FuelStation` | FuelBuilding | R `fuelLink` INT | – | no | 64 (9) |
| 54 | `EscapeRocket` | EscapeRocket | R `fuelLink` INT, R `fuel` F32, R `passengers` INT, R `spawnBuildingId` INT, R `spawnCompleted` INT | – | no | 22 (9) |
| 55 | `FenceSwitch` | FenceSwitch | R `linkedBuildingId` INT, R `linkedBuildingId2` INT, R `switchValue` INT (−1 → copy of `linkedBuildingId`), R `lockable` INT, R `script` WORD, O `locked` INT (== 1) | false | yes | 0 |
| 56 | `DynamicHub` | DynamicHub | R `buildingLink` INT, R `shapeName` WORD, R `requiredScore` INT, R `currentScore` INT, R `minActiveLinks` INT | – | yes | 0 |
| 57 | `DynamicNode` | DynamicNode | R `buildingLink` INT, R `shapeName` WORD, R `scoreValue` INT, R `scoreSupplied` INT | – | yes | 0 |
| 58 | `FeedingTube` | FeedingTube | R `receiverId` INT | – | yes | 0 |
| 59 | `MultiwiniaZone` | MultiwiniaZone | R `size` FLOAT (`atof`; radius wu [I]), `blitzkriegLink`* INT | empty | no | 182 (29) |
| 60 | `ChessPiece` | ChessPiece | – | – | yes | 0 |
| 61 | `ChessBase` | ChessBase | R `width` INT, R `depth` INT | – | yes | 2 (1) ignored |
| 62 | `CloneLab` | CloneLab (Incubator) | O `numStartingSpirits` INT | 0 | no | 15 (4, old/ only) |
| 63 | `ControlStation` | ControlStation | R `controlBuildingId` INT | – | no | 54 (13) |
| 64 | `Portal` | Portal | – | – | yes | 91 (10, old/) ignored |
| 65 | `Crate` | Crate | – | – | yes (spawned at run time) | 0 |
| 66 | `Statue` | Statue | – | – | yes | 1 (1) ignored |
| 67 | `WallBuster` | WallBuster | – | – | no | 3 (1) |
| 68 | `PulseBomb` | PulseBomb | R `startTime` F32 (s), `link`* INT | empty | no | 10 (10) |
| 69 | `RestrictionZone` | RestrictionZone | R `size` F32 (wu), O `blockPowerups` INT | false | no | 35 (14) |
| 70 | `JumpPad` | JumpPad | R `force` F32, R `angle` F32 (radians, [03](03_buildings_and_world_objects.md)) | – | no | 27 (4) |
| 71 | `AIObjective` | AIObjective | R `nextObjective` INT | – | no | 98 (10) |
| 72 | `AIObjectiveMarker` | AIObjectiveMarker | R `scanRange` F32 (wu), R `objectiveId` INT, O `armourObjective` INT, O `pickupOnly` INT | 0, 0 | no | 345 (17) |
| 73 | `EruptionMarker` | EruptionMarker | – | – | no | 1 (1) |
| 74 | `SmokeMarker` | SmokeMarker | – | – | no | 9 (2) |

Forced-global types at construction (`RadarDish`, `ControlTower`, `TrunkPort`, `Incubator`, `DynamicHub`, `FenceSwitch`) are overwritten by column 8 anyway [C] `Building::CreateBuilding(int)`. Spot-verified for this document: `Tree::Read` (`code/worldobject/tree.cpp`), `GunTurret::Read` (`code/worldobject/gunturret.cpp`), `AITarget::Read` (`code/worldobject/ai.cpp`) match the table exactly [C].

#### 4.1.11 `CameraMounts` block [C] `LevelFile::ParseCameraMounts`, `WriteCameraMounts`

```ebnf
mount_row = NAME posX posY posZ frontX frontY frontZ upX upY upZ EOL ;    (* 10 tokens, all MISSING => CRASH *)
```

| Col | Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|---|
| 1 | `m_name` | WORD, `strncpy` 63 chars into `char[64]` | – | – | keep ≤ 62 characters (a 63+ character name is not NUL-terminated by `strncpy`) | mount name, looked up case-insensitively (`LevelFile::GetCameraMount`) | `ParseCameraMounts` |
| 2–4 | `m_pos` | F32 ×3 | wu | – | – | camera position | same |
| 5–7 | `m_front` | F32 ×3 | direction | – | non-zero | normalised after reading | same |
| 8–10 | `m_up` | F32 ×3 | direction | – | non-zero | normalised after reading | same |

Names consumed by code [C]: `player<N>` (N = the local team's position index) is the start camera in Multiwinia (`code/main.cpp`, `sprintf(camname, "player%d", …)`); `start` is the default start mount (`code/main.cpp`, `SetTarget("start")`); `CamPosBefore` is reserved (`MAGIC_MOUNT_NAME_START_POS`, `code/level_file.h`). Shipped `mp_*`: `player0` 51, `player1` 51, `player2` 31, `player3` 13, `start` 7 rows [C] survey.

#### 4.1.12 `CameraAnimations` block [C] `LevelFile::ParseCameraAnims`, `WriteCameraAnims`

```ebnf
cam_anims = 'CameraAnimations_StartDefinition' EOL , { blank | animation } , 'CameraAnimations_EndDefinition' EOL ;
animation = ANIMNAME EOL , { blank | node } , 'End' EOL ;          (* ANIMNAME strncpy 63 into char[64] *)
node      = ( 'Move' | 'Cut' ) MOUNTNAME DURATION EOL ;            (* all MISSING => CRASH *)
```

| Field | Type | Units | Default | Range | Failure | Parser ref |
|---|---|---|---|---|---|---|
| transition | word → `CamAnimNode::GetTransitModeId` (`Move` = `TransitionMove` 0, `Cut` = `TransitionCut` 1) | – | – | `Move`, `Cut` | unknown word → `AppReleaseAssert("Bad camera animation camera mode in level file %s")` — fatal | `ParseCameraAnims` |
| `m_mountName` | WORD (`strdup`) | – | – | a mount defined **earlier** (this file or the mission parsed before), or `CamPosBefore` | `"Bad camera animation mount name"` — fatal | same |
| `m_duration` | F32 | seconds | – | 0.0 ≤ d < 60.0 | `"Bad camera animation transition time"` — fatal | same |

Shipped: 96 blocks, all empty [C] survey; consumed only by the script command `CamAnim` ([fmt-script](#fmt-script)) [B:LEGACY].

#### 4.1.13 `InstantUnits` block [C] `LevelFile::ParseInstantUnits`, `code/level_file.h:InstantUnit`

```ebnf
iu_row = ENTITYTYPE team x z count inAUnit state spread [ waypointX waypointZ ] [ routeId [ routeWaypointId [ runAsTask ] ] ] EOL ;
```

| Col | Field | Type | Units | Default (ctor) | Range | Semantics / failure | Parser ref |
|---|---|---|---|---|---|---|---|
| 1 | `m_type` | entity type name, case-insensitive (`Entity::GetTypeId`) | – | −1 | names below | unknown, or `Entity::EntityIsBlocked` (FULL blocks `AI`, `ArmyAnt`, `DropShip`, `Egg`, `Lander`, `LaserTroop`, `Meteor`, `Nuke`, `Officer`, `Shaman`, `SpaceInvader`, `Tentacle`, `TriffidEgg`, `Tripod`, `InvalidType`, `SpaceShip`) → line skipped | `ParseInstantUnits` |
| 2 | `m_teamId` | INT | team id | −1 | 0…5 | R | same |
| 3–4 | `m_posX`, `m_posZ` | F32 | wu | 0.0 | – | group centre, R | same |
| 5 | `m_number` | INT | entities | 0 | – | R | same |
| 6 | `m_inAUnit` | BOOL | – | false | – | R; `Squadie` with 0 is skipped (legacy crash guard) | same |
| 7 | `m_state` | INT | entity state enum | −1 | −1 = default state | R | same |
| 8 | `m_spread` | F32 | wu (radius) | 200.0 | – | R | same |
| 9–10 | `m_waypointX/Z` | F32 | wu | 0.0, 0.0 | – | the pair is read when **one** token remains, so a single extra token ⇒ CRASH | same |
| 11 | `m_routeId` | INT | – | −1 | – | O | same |
| 12 | `m_routeWaypointId` | INT | – | −1 | – | O (nested) | same |
| 13 | `m_runAsTask` | BOOL | – | false | – | O (nested) | same |

Entity type names and indices (also used by the `entityType` columns of `AISpawnPoint` and `SafeArea` and by [fmt-stats-txt](#fmt-stats-txt) row order) [C] `code/worldobject/entity.cpp:Entity::GetTypeName`: 0 `InvalidType`, 1 `LaserTroop`, 2 `Engineer`, 3 `Virii`, 4 `Squadie` (= `TypeInsertionSquadie`), 5 `Egg`, 6 `SporeGenerator`, 7 `Lander`, 8 `Tripod`, 9 `Centipede`, 10 `SpaceInvader`, 11 `Spider`, 12 `Darwinian`, 13 `Officer`, 14 `ArmyAnt`, 15 `Armour`, 16 `SoulDestroyer`, 17 `TriffidEgg`, 18 `AI`, 19 `Shaman`, 20 `Harvester`, 21 `Nuke`, 22 `Tentacle`, 23 `Meteor`, 24 `Tank`, 25 `SpaceShip`, 26 `DropShip`.

Shipped rows [C] survey: `Darwinian` 390 rows in 64 files (39 mp), `Armour` 7 (4 mp), `Tank` 9 (3 mp, TankBattle maps), `Engineer` 3 (old/); campaign missions add `Virii`, `Centipede`, `Spider`, `SporeGenerator`, `SoulDestroyer`; blocked and skipped: `AI` 7, `ArmyAnt` 1, `Shaman` 9. Team-1 difficulty scaling (`number ×= f`, centipede splitting with non-synchronised `sfrand(60)`, `spread ×= 1.2^(difficulty/5)`) is inert because `f = 1` [C].

#### 4.1.14 `Routes` block [C] `LevelFile::ParseRoutes`, `LevelFile::ParseRoute`, `code/routing_system.h:WayPoint`

```ebnf
routes     = 'Routes_StartDefinition' EOL , { blank | route | other_line } , 'Routes_EndDefinition' EOL ;
route      = 'Route' ID EOL , { blank | waypoint } , 'End' EOL ;   (* ID INT, debug-asserted 0 <= ID < 10000 *)
waypoint   = '0' x y z EOL            (* WayPoint::Type3DPos, F32 x3 *)
           | '1' x z EOL              (* WayPoint::TypeGroundPos, F32 x2 *)
           | '2' buildingId EOL ;     (* WayPoint::TypeBuilding, INT *)
other_line = any line whose first token is not 'Route' ;          (* ignored *)
```

The type is `atoi(first token)` (debug assert that it starts with a digit); any other number creates a waypoint at (0,0,0) of that type [C]. No shipped file contains a `Routes` block [C] survey.

#### 4.1.15 `PrimaryObjectives` block [C] `LevelFile::ParsePrimaryObjectives`, `code/global_world.cpp:GlobalEventCondition`

```ebnf
objectives = 'PrimaryObjectives_StartDefinition' EOL , { objective } , 'PrimaryObjectives_EndDefinition' EOL ;   (* NO blank lines *)
objective  = condition [ STRINGID [ CUTSCENE ] ] EOL ;
condition  = ( 'BuildingOnline' | 'BuildingOffline' ) LOCATIONNAME BUILDINGID
           | 'ResearchOwned' RESEARCHNAME
           | 'DebugKey' INT
           | 'NeverTrue'
           | 'AlwaysTrue' | 'NotInLocation' ;           (* debug-asserted as invalid here *)
```

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| condition word | word → `GlobalEventCondition::GetType` (case-insensitive) | – | `AlwaysTrue` 0, `BuildingOnline` 1, `BuildingOffline` 2, `ResearchOwned` 3, `NotInLocation` 4, `DebugKey` 5, `NeverTrue` 6; unknown → −1 (debug assert; line still stored) | condition type | `ParsePrimaryObjectives` |
| `LOCATIONNAME` | WORD → `GlobalWorld::GetLocationId` | – | a name derived from `game.txt` (§4.4) | in Multiwinia the global world is empty → −1 [C] `code/app.cpp:App::Initialise` | same |
| `BUILDINGID` | INT | – | – | building id in that location | same |
| `STRINGID` | WORD | none | phrase id | objective text | same |
| `CUTSCENE` | WORD | none | script file | run on completion | same |

The loop has no `TokenAvailable` guard — a blank line inside crashes [C]. If the block is absent or empty, `GenerateAutomaticObjectives` adds one `NeverTrue` objective and derives secondary objectives from `ResearchItem`/`TrunkPort` buildings (blocked or campaign-only) [C]. Shipped [C] survey: 77 files `NeverTrue` (one `NeverTrue -1 -1`), 19 files `BuildingOnline` (16 missions, `mp_sandbox`/`mp_sandbox_all` with `:sandbox,2`, `map_sandbox`), 1 `ResearchOwned`. In Multiwinia objectives have no gameplay effect [I: only Darwinia's `Location::MissionComplete` evaluates them].

#### 4.1.16 Mission-only blocks [B:LEGACY] [C] `LevelFile::ParseDifficulty`, `ParseRunningPrograms`, `ParseLeaderboardStats`, `WriteRunningPrograms`

| Block | Grammar | Field semantics, units, defaults |
|---|---|---|
| `Difficulty` | `'CreatedAsDifficulty' INT` (other words ignored) | `m_levelDifficulty = n − 1`, clamped ≥ 0 (1-based in the file); present in 3 shipped missions, value 1 |
| `RunningPrograms` | `ENTITYTYPE count state data waypointX waypointZ { posX posZ health } × count` — all MISSING ⇒ CRASH; `count` ≤ 10 (fixed arrays `m_positionX[10]`, **no bounds check**) | save-game record of running Engineer/Squad tasks (`data` = engineer spirits or squad weapon type); the writer's squad branch uses `%8.2` with no conversion letter → malformed output |
| `LeaderboardStats` | `'Time' FLOAT`, `'Kills' INT`, `'Survivors' …` | `Time` parsed with `sscanf("%f")` into a `double` → garbage; `Kills` written straight into `m_location->m_teams[2]->m_teamKills` (requires a live location) |

#### 4.1.17 Writer notes [C] `LevelFile::Save`, `LevelFile::SaveMapFile`, `LevelFile::SaveMissionFile`, `Building::Write`, `LevelFile::WriteLandscapeTiles`

* Map save target: `TARGET_DEBUG` → `data/levels/<map>` via `Resource::GetTextFileWriter`; otherwise `<P>maps/<map>` (plain). Mission save: `<P>users/<profile>/<mission>` (obfuscated unless `TARGET_DEBUG`, [fmt-profile-save](#fmt-profile-save)).
* Building rows: `"\t%-20s"` type name then `%-8d %-8.2f %-8.2f %-8d %-8.2f %-8.2f %-8d` for the base columns, then the subclass `Write` — positions and fronts keep **2 decimals**, so a load/save round trip is lossy.
* `WriteLandscapeTiles` writes `desiredHeight` with `%6.0f` and concatenates `lowlandSmoothing %6.2f` and `guidePower %6d` without a separator — readable only because of the field widths.

#### 4.1.18 Verification

Parsers re-read for this document: `LevelFile::ParseMissionFile`, `ParseMapFile` (both overloads), `ParseMultiwiniaOptions`, `ParseBuildings`, `ParseInstantUnits`, `ParseLandscapeData`, `ParseLandscapeTiles`, `ParseLandFlattenAreas`, `ParseLights`, `ParseRoute`, `ParseRoutes`, `ParsePrimaryObjectives`, `ParseCameraMounts`, `ParseCameraAnims`, the two `LevelFile` constructors, `Building::Read`, `Tree::Read`, `GunTurret::Read`, `AITarget::Read`, `GameMenu::AddMap`. Survey: `X1a_level_survey.py` (re-run; output identical to `_work/drafts/X1a_level_field_usage.csv`) over **118 level files** (57 mp, 12 map, 17 mission, 30 old, 2 purity): every strict part of the grammar (no blank lines in `Landscape`/`PrimaryObjectives`, every required column present, every `isGlobal` constraint met, guide-grid lengths exact) is respected by shipped data; 10 anomalies listed below.

#### 4.1.19 Minimal valid examples (written for this document)

Two-player map (`mp_example_2p.txt`) with one tile, one flatten area, a spawn network and two cameras:

```text
MultiwiniaOptions_StartDefinition
    GameTypes       Domination
    NumPlayers      2
    PopulationCap   800
    PlayTime        10
    Difficulty      intermediate
    LevelOptions    instantspawnpointcapture
MultiwiniaOptions_EndDefinition

Landscape_StartDefinition
    worldSizeX      1600
    worldSizeZ      1000
    cellSize        12.0
    outsideHeight   -10.0
    landColourFile  landscape_default.bmp
    wavesColourFile waves_default.bmp
    waterColourFile water_default.bmp
Landscape_EndDefinition

LandscapeTiles_StartDefinition
    200  0.0  100  1200  1.10  1.00  90  0  4242  1.00  0
LandscapeTiles_EndDefinition

LandFlattenAreas_StartDefinition
    800.0 30.0 600.0 60.0 0 30.0
LandFlattenAreas_EndDefinition

Lights_StartDefinition
    0.30 0.80 0.50  1.00 0.95 0.90
Lights_EndDefinition

Buildings_StartDefinition
    SpawnPointMaster  10  800.0  600.0  255  0.0  1.0  0  11 12
    SpawnPoint        11  400.0  600.0  0    1.0  0.0  0
    SpawnPoint        12  1200.0 600.0  1   -1.0  0.0  0
    TrunkPort         13  800.0  300.0  255  0.0  1.0  1
Buildings_EndDefinition

CameraMounts_StartDefinition
    player0  250.0 250.0 600.0   0.60 -0.60  0.00   0.60 0.60 0.00
    player1  1350.0 250.0 600.0 -0.60 -0.60  0.00  -0.60 0.60 0.00
CameraMounts_EndDefinition

InstantUnits_StartDefinition
    Darwinian  0  400.0  600.0  15  0  -1  40.0
    Darwinian  1  1200.0 600.0  15  0  -1  40.0
InstantUnits_EndDefinition
```

Campaign mission ([B:LEGACY]) with a difficulty block and one objective:

```text
Difficulty_StartDefinition
    CreatedAsDifficulty 2
Difficulty_EndDefinition

Buildings_StartDefinition
    Incubator  20  300.0 450.0  1  0.0 1.0  1  5
Buildings_EndDefinition

PrimaryObjectives_StartDefinition
    BuildingOnline sample,20  sample_objective_text
PrimaryObjectives_EndDefinition
```

#### 4.1.20 Discrepancies between shipped files and the parsers [C] `X1a_level_survey.py`

| # | Finding | Files | Effect |
|---|---|---|---|
| 1 | `GameTypes none` | `mp_assault_stress`, `mp_benchmark_1`, `mp_sandbox`, `mp_sandbox_all` + 6 old/ | map dropped from the lobby list; reached only by other paths [I] |
| 2 | `GameTypes Shaman`, `GameTypes Skirmish` | 10 + 6 old/ maps | unknown names → ignored (old naming) |
| 3 | top-level `GameTypes` line before any block | `old/mp_chess1.txt` | debug assert; ignored |
| 4 | `CarryableBuilding` building type | `map_sandbox.txt` | not a type name → line ignored |
| 5 | `AIObjective` with 2 extra tokens (needs 1) | `mp_assault_stress.txt` (3 rows), `old/mp_assault_3P_1.txt` (3) | surplus ignored |
| 6 | `Triffid` with 20 extra tokens (max 19) | `old/mp_shaman_2P.txt` (2 rows) | surplus ignored |
| 7 | 39 blocked Darwinia types present (`Portal` 91 rows in old/, `ScriptTrigger` 25, `TrackLink` 111, `SpiritReceiver` 49, `ReceiverLink` 83, plus the other rows marked "ignored" in §4.1.10) | campaign maps, old/ | silently ignored in Multiwinia |
| 8 | blocked entity types in `InstantUnits` (`AI`, `ArmyAnt`, `Shaman`) | missions, old/ | skipped |
| 9 | `Name NewLevel` placeholder | 10 mp maps | menu shows language-table names [I] |
| 10 | `Author`, `Description`, `Routes`, `RunningPrograms`, `LeaderboardStats` parsed but never present; `maxHeight` only in 41 mp maps | – | editor / save features only |
| 11 | duplicate `PopulationCap` | `mp_cts_4p_3.txt` | last (2000) wins |
| 12 | second `Buildings` block in 47 mp maps (after `CameraAnimations`) | 47 mp | also static (dynamic branch unreachable) |

<a id="fmt-level-enum"></a>
### 4.2 `level_enum.txt` — achievement level ids

**Purpose.** Maps map file names to the small integer ids used by the achievement bitsets ("Master of Multiwinia", "Explorer") [C] `code/achievement_tracker.cpp:AchievementTracker::AchievementTracker`, `AddVictoryOnLevel`, `AddVisitedLevel`.

**Files.** `data/levels/level_enum.txt` (59 lines), read through `Resource::GetTextReader(ACHIEVEMENT_LEVEL_ENUM)`; a missing file only logs `"Failed to load level_enum.txt"` [C].

**Encoding.** ASCII, CRLF, **no final newline** (last line `mp_PDLC2_rocketriot_3P_1.txt 59`) [C] survey.

```ebnf
level_enum = { blank | MAPFILENAME LEVELID EOL } ;      (* LEVELID INT; a one-token line => atoi(NULL) crash *)
```

| Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `MAPFILENAME` | WORD | – | – | file name incl. `.txt`; unique case-insensitively (a duplicate key is `AppAbort("Error with HashTable")` — **fatal**) | key of `m_levelList` (`HashTable<int>`, case-insensitive) and value of `m_levelListReverse` (`std::map<int,std::string>`) | `AchievementTracker::AchievementTracker`, `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::GetInsertPos` |
| `LEVELID` | INT | – | – | 1…49 used; > `MAX_LEVEL_ID` (49) ignored by `AddVictoryOnLevel`/`AddVisitedLevel` | bit index into `m_won[8]`, `m_visited[8]`; achievements need `VALID_ACHIEVEMENT_LEVELS` = 49 (41 without `INCLUDEGAMEMODE_ASSAULT`) | same |

Both containers copy the token, so the reader buffer may be freed [C] `HashTable::PutData`.

**Verification.** Parser re-read for this document (loop with `TokenAvailable` guard, unguarded second `GetNextToken`). Data check (X1a script): ids 1…49 are exactly the 49 retail maps (every `mp_*` not excluded by `tools/multiwinia_exclusions.txt`), no gaps, no duplicates; ids 50…59 name `mp_PDLC1_*`/`mp_PDLC2_*` maps absent from the tree [C].

**Minimal valid example**

```text
mp_alpha_2p_1.txt   1
mp_beta_4p_2.txt    2
```

**Discrepancies.** (1) `tools/multiwinia_exclusions.txt` excludes `levels/level_enum.txt` from the retail `main.dat`, so in a packaged retail build the file is not found and the two achievements can never be awarded unless another archive supplies it [C] list; whether `patch.dat` did is **29-U1**. (2) Because the last line is unterminated, it would be dropped if read from an archive (§3.1) — harmless (id 59 > 49).

<a id="fmt-multiwinia-txt"></a>
### 4.3 `multiwinia.txt` — game types and lobby parameters

**Purpose.** Defines the Multiwinia game types in **file order** (that order *is* the game-type index used by maps, network, UI) and each type's lobby parameters [C] `code/multiwinia.cpp:Multiwinia::LoadBlueprints`, `MultiwiniaGameBlueprint::MultiwiniaGameBlueprint`, `MultiwiniaGameParameter::MultiwiniaGameParameter`.

**Files.** `data/multiwinia.txt` [B:FULL]; `data/multiwinia-testbed.txt` replaces it only under `TESTBED_ENABLED` [B:DEAD]. Missing file: `AppReleaseAssert("Couldn't load multiwinia.txt")` — fatal [C].

**Encoding.** ASCII, CRLF (71 lines) [C] survey.

```ebnf
mw_file  = { blank | gametype } ;
gametype = 'GAMETYPE' NAME EOL , { param } , 'END' EOL ;    (* NO blank or comment-only line between GAMETYPE and END *)
param    = PARAMNAME MIN MAX DEFAULT CHANGE EOL ;           (* WORD F32 F32 F32 INT, all MISSING => CRASH *)
```

The top-level keyword is only debug-asserted to be `GAMETYPE`; in release any first token starts a game type. Inside a block the parser does `ReadLine(); GetNextToken()` with no `TokenAvailable` check, so a blank line inside a block crashes (`stricmp(NULL, "END")`). `END` is case-insensitive [C].

| Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `NAME` | WORD (`char[256]`) | – | – | identity, case-sensitive | blueprint name; level `GameTypes` must match it with `strcmp`; phrase id `multiwinia_gametype_<lower(NAME)>`; index order must equal `Multiwinia::GameType*`: 0 `Domination` (`GameTypeSkirmish`), 1 `KingOfTheHill`, 2 `CaptureTheStatue`, 3 `Assault`, 4 `RocketRiot`, 5 `Blitzkrieg` (`GameTypeBlitzkreig`), 6 `TankBattle` [C] `code/multiwinia.h` | `LoadBlueprints` |
| `PARAMNAME` | WORD (`char[256]`) | – | – | – | looked up case-insensitively by `Multiwinia::GetGameOption(name)` (−1 if absent); phrase id `multiwinia_param_<lower(PARAMNAME)>` | same |
| `MIN`, `MAX`, `DEFAULT` | F32 (stored `double`, used as int by the lobby) | per parameter ([01](01_game_modes.md)) | – | – | slider/drop-down range and initial value | same |
| `CHANGE` | INT | – | – | −1 checkbox, 0 drop-down, > 0 slider step | UI widget type [C] `code/UI/GameMenuWindow.cpp` | same |

At most `MULTIWINIA_MAXPARAMS` = 64 parameters per type are storable in `Multiwinia::m_params[64]`; the loader does not check [I].

Shipped parameters (`name min max default change`) [C] `data/multiwinia.txt`:

| Game type (index) | Parameters |
|---|---|
| Domination (0) | ScoreMode 0 1 0 0; TimeLimit 0 60 10 1; Handicap 0 90 90 10; CrateDropRate 0 300 40 10; StartingPowerups 0 3 0 1; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; CrateDropMode 0 1 1 0 |
| KingOfTheHill (1) | ScoreMode 0 3 0 0; TimeLimit 1 60 5 1; Handicap 0 90 90 10; CrateDropRate 0 300 40 10; StartingPowerups 0 3 0 1; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; CrateDropMode 0 1 1 0 |
| CaptureTheStatue (2) | TimeLimit 1 60 10 1; CrateDropRate 0 300 40 10; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; CrateDropMode 0 1 1 0 |
| Assault (3) | Handicap 0 90 90 10; CrateDropRate 0 300 40 10 |
| RocketRiot (4) | CrateDropRate 0 300 40 10; ReinforcementTimer 10 300 60 10; ReinforcementCount 1 100 40 1; MaxArmour 1 10 2 1; TurretFrequency 0 10 7 1; MaxTurrets 0 5 3 1 |
| Blitzkrieg (5) | TimeLimit 0 60 15 1; CrateDropRate 0 300 40 10; ReinforcementTimer 10 300 60 10; ReinforcementCount 1 100 40 1; MaxArmour 1 10 2 1; TurretFrequency 0 10 2 1; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; MaxTurrets 0 5 2 1 |
| TankBattle (6) | NumTanks 3 10 3 1 (game type compiled out: `INCLUDEGAMEMODE_TANKBATTLE` undefined) |

`multiwinia-testbed.txt` differs only by an extra comment and `TimeLimit 0 60 10 1` appended to RocketRiot [C] diff.

**Verification.** `Multiwinia::LoadBlueprints` re-read (unguarded inner loop confirmed). Data: `data/multiwinia.txt` 7 blocks, 53 token lines, no blank line inside a block [C] `X1a_text_survey.py`.

**Minimal valid example**

```text
GAMETYPE Domination
    TimeLimit      1   30   12   1
    CrateDropMode  0    1    0   0
END
```

**Discrepancies.** The header comment of `data/multiwinia.txt` (`Change: 0 = DropDownMenu, -1 = CheckBox`) matches the code [C]. Adding, removing or reordering a `GAMETYPE` block silently renumbers every game type on maps, network and UI.

<a id="fmt-game-txt"></a>
### 4.4 `game.txt` family — campaign world definition [B:LEGACY]

**Purpose.** `GlobalWorld::LoadGame(name)` builds the Darwinia campaign world (locations, global buildings, research, scripted events). Reached only through `App::LoadProfile`/`App::SaveProfile`, whose callers are campaign/prologue paths; the FULL Multiwinia flow creates an **empty** `GlobalWorld` (the `LoadProfile()` call in `App::Initialise` is commented out) [C] `code/app.cpp:App::LoadProfile`, `App::Initialise`, `code/global_world.cpp:GlobalWorld::LoadGame`.

**Files**

| File | Selected when | Tag |
|---|---|---|
| `data/game.txt` | `GAMEDATAFILE` = `game.txt` (`DEMOBUILD`/`DEMO2` undefined) [C] `code/app.cpp` | [B:LEGACY] |
| `data/game_demo.txt` | `DEMOBUILD` only | [B:DEAD] |
| `data/game_demo2.txt` | `DEMO2` only | [B:DEAD] |
| `data/game_unlockall.txt` | `LoadProfile` with a full version and profile `AccessAllAreas` or `AttractMode` (every trunk port online, every location available) [C] `App::LoadProfile` | [B:LEGACY] |
| `data/purity/game.txt` | never (no code opens `purity/`) | [B:DEAD] |
| `<P>users/<name>/game.txt` | save game, tried first ([fmt-profile-save](#fmt-profile-save)) | [B:LEGACY] |

Load order [C] `GlobalWorld::LoadGame`: user save (`UnicodeTextFileReader` → `TextFileReader`) else `Resource::GetTextReader(name)`; then `locations.txt` (§4.5); then every location's map file and every `levels/mission_<location>*.txt` are parsed to register global buildings and completed missions; finally `EvaluateEvents()`.

**Encoding.** ASCII, CRLF [C] survey.

```ebnf
game_file   = { blank | section | other_line } ;                       (* other_line ignored *)
section     = locations | gbuildings | research | events | tutorial | leveltimes | squaddies ;
locations   = 'Locations_StartDefinition' EOL , { blank | loc_row } , 'Locations_EndDefinition' EOL ;
loc_row     = ID AVAILABLE MAPFILE MISSIONFILE EOL ;                    (* INT INT WORD WORD, MISSING => CRASH *)
gbuildings  = 'Buildings_StartDefinition' EOL , { blank | gb_row } , 'Buildings_EndDefinition' EOL ;
gb_row      = ID TEAMID LOCID TYPE LINK ONLINE EOL ;                    (* six INT, MISSING => CRASH *)
research    = 'Research_StartDefinition' EOL , { blank | r_line } , 'Research_EndDefinition' EOL ;
r_line      = 'Research' RESEARCHNAME PROGRESS LEVEL | 'CurrentResearch' RESEARCHNAME | 'CurrentPoints' INT ;
events      = 'Events_StartDefinition' EOL , { blank | event } , 'Events_EndDefinition' EOL ;
event       = 'Event' { condition } EOL , { blank | action_line } , 'End' EOL ;
condition   = 'AlwaysTrue' | 'NotInLocation' | 'NeverTrue'
            | ( 'BuildingOnline' | 'BuildingOffline' ) LOCATIONNAME BUILDINGID
            | 'ResearchOwned' RESEARCHNAME | 'DebugKey' INT ;
action_line = 'Action' ( 'SetMission' LOCATIONNAME MISSIONFILE | 'RunScript' SCRIPTNAME | 'MakeAvailable' LOCATIONNAME ) EOL ;
tutorial    = 'Tutorial_StartDefinition' EOL , { WORD WORD INT EOL } , 'Tutorial_EndDefinition' EOL ;   (* e.g. "CurrentChapter = 3" *)
leveltimes  = 'LevelTimes_StartDefinition' EOL , { INDEX VALUE EOL } , 'LevelTimes_EndDefinition' EOL ;
squaddies   = 'SquaddiesUsed_StartDefinition' EOL , { INDEX VALUE EOL } , 'SquaddiesUsed_EndDefinition' EOL ;
```

All keywords case-insensitive; `:` and `,` are separators, so conditions are conventionally written `BuildingOnline:garden,8` (three tokens) [C].

| Section / field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `loc_row.ID` | INT | – | – | 1…12 in Darwinia | location id (referenced by `gb_row.LOCID`, `TrunkPort.targetLocationId`, `locations.txt`) | `GlobalWorld::ParseLocations` |
| `loc_row.AVAILABLE` | BOOL | – | – | 0/1 | reachable on the world map | same |
| `loc_row.MAPFILE` | WORD ≤ 255 | – | – | `map_<name>.txt` | map file; **location name = MAPFILE minus its first 4 and last 4 characters** (`map_garden.txt` → `garden`); a name shorter than 8 characters underflows | same |
| `loc_row.MISSIONFILE` | WORD ≤ 255 | – | – | file or `null` | current mission | same |
| `gb_row` | 6 × INT | – | – | shipped type 0 | `GlobalBuilding{id, teamId, locationId, type, link, online}`, looked up by (id, locationId) | `GlobalWorld::ParseBuildings` |
| `Research NAME PROGRESS LEVEL` | research name, INT, INT | points, level | – | 29 names (below) | sets level, then progress; **an unknown name writes `m_researchLevel[-1]`** (out of bounds) | `GlobalResearch::Read` |
| `CurrentResearch NAME` | research name | – | – | – | current research (−1 unknown) | same |
| `CurrentPoints INT` | INT | points | – | – | research points | same |
| other word inside `Research` | – | – | – | – | `AppReleaseAssert("Error loading GlobalResearch")` — **fatal** | same |
| `Event` conditions | as §4.1.15; `LOCATIONNAME` resolved against locations loaded so far | – | – | – | conjunction: fires when all are true | `GlobalWorld::ParseEvents`, `GlobalEvent::Read` |
| `Action` lines | – | – | – | – | executed **one per evaluation** in order; a first token other than `Action` is debug-asserted but parsed as an action; an unknown action word leaves `m_type` uninitialised → undefined on execute | `GlobalEventAction::Read` |
| `SetMission LOC FILE` | WORD WORD | – | – | – | set the location's mission | same |
| `RunScript NAME` | WORD | – | – | – | `Script::RunScript`: a name containing `.txt` runs `data/scripts/NAME`; otherwise a Sepulveda phrase sequence, **no-op in FULL** (e.g. `RunScript gardenfinished`) | same |
| `MakeAvailable LOC` | WORD | – | – | – | location becomes available | same |
| `Tutorial` lines | WORD WORD INT | chapter | – | `-1` deletes | Darwinia demo tutorial chapter; whole section only under `USE_SEPULVEDA_HELP_TUTORIAL` [B:DEAD] | `GlobalWorld::ParseTutorial` |
| `LevelTimes INDEX VALUE` | INT INT | – | – | no bounds check | `m_spLevelTimes[INDEX] = VALUE` | `GlobalWorld::ParseLevelTimes` |
| `SquaddiesUsed INDEX VALUE` | INT INT | – | – | no bounds check | **bug**: also writes `m_spLevelTimes[INDEX]` | `GlobalWorld::ParseSquaddiesUsed` |

Research names (case-insensitive, `GlobalResearch::GetTypeName`, index order): `Darwinian`, `Officer`, `Squad`, `Laser`, `Grenade`, `Rocket`, `Controller`, `AirStrike`, `Armour`, `TaskManager`, `Engineer`, `Harvester`, `Nuke`, `Subversion`, `PersonalShield`, `HotFeet`, `GunTurret`, `Shaman`, `Infection`, `MagicalForest`, `DarkForest`, `AntNest`, `Plague`, `Egg`, `MeteorShower`, `FireExtinguisher`, `Rage`, `Tank`, `DropShip` [C] `code/global_world.cpp`.

Writers [C] `GlobalWorld::SaveGame`: `WriteLocations` (`"\t%4d %4d %30s %40s"`), `WriteBuildings` (`"\t%4d %4d %6d %6d %6d %6d"`), `GlobalResearch::Write` (29 rows + `CurrentResearch` + `CurrentPoints`), `GlobalEvent::Write`, `WriteLevelTimes`/`WriteSquaddiesUsed` (empty), `WriteTutorial` (DEMO2 only).

**Verification.** Grammar from X1a's reading of `GlobalWorld::LoadGame` and helpers [C]; not re-read line by line for this document (legacy-only format) — the block keywords and the research-name list were re-checked by grep. Data: all five shipped files have Locations, Buildings, Research, Events: `game.txt` 11/2/11/18 (locations / global buildings / research rows / events), `game_unlockall.txt` 11/2/11/13, `game_demo.txt` 1/1/9/0, `game_demo2.txt` 8/8/9/1, `purity/game.txt` 11/0/11/6 [C] X1a survey; `game_demo2.txt` uses `BuildingOnline:launchpad:1` (colon instead of comma — equivalent).

**Minimal valid example**

```text
Locations_StartDefinition
    1  1  map_sample.txt  mission_sample.txt
Locations_EndDefinition

Research_StartDefinition
    Research Officer 0 2
    CurrentResearch Officer
    CurrentPoints 10
Research_EndDefinition

Events_StartDefinition
    Event BuildingOnline:sample,20
        Action MakeAvailable sample
    End
Events_EndDefinition
```

**Discrepancies.** `docs/top level map.txt` says each level file names its map at the start and that global data has "X Z position"; the pairing is actually given by `Locations` rows and `locations.txt` stores X Y Z [C].

<a id="fmt-locations-txt"></a>
### 4.5 `locations.txt` — world-map positions [B:LEGACY]

**Purpose.** Positions of campaign locations on the sphere world map [C] `code/global_world.cpp:GlobalWorld::LoadLocations`, `GlobalWorld::SaveLocations`.

**Files.** `data/locations.txt` (read through `Resource::GetTextReader("locations.txt")`; a missing file dereferences `NULL` — crash) [C].

**Encoding.** ASCII, CRLF.

```ebnf
locations_file = { blank | ID X Y Z { ignored } EOL } ;      (* INT F32 F32 F32, MISSING => CRASH *)
```

| Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `ID` | INT | – | – | an id already loaded from the game file; others ignored (e.g. 12 in the shipped file) | location | `LoadLocations` |
| `X`, `Y`, `Z` | F32 | world-map renderer units [I] | – | – | `GlobalLocation::m_pos` | same |

**Verification.** From X1a's reading of `LoadLocations`/`SaveLocations` [C]; shipped file 12 rows, 11 matching `game.txt` ids [C].

**Minimal valid example**

```text
1   -120.00   40.00   95.00
```

**Discrepancies.** The editor writer (`GlobalWorldEditorWindow`, [B:EDITOR]) emits `"%-6d %-8.2 %-8.2 %-8.2\n"` — the `%-8.2` conversions lack a type letter, so a saved file is malformed [C] `GlobalWorld::SaveLocations`.

<a id="fmt-map-crc"></a>
### 4.6 Map identity CRC and `code/mapcrc.h`

**Purpose.** A map's identity is the CRC of its level-file bytes; a build tool writes the shipped values into a header used for map whitelists, demo limits and tutorial buttons [C] `contrib/systemiv/lib/math/filecrc.cpp:FileCRC`, `ReadFileData`, `contrib/unrar/crc.cpp:CRC`, `code/level_file.cpp:LevelFile::CalculeCRC`, `tools/MapCRC/main.cpp:GenerateMapCRCs`. Tool, build integration and consumers: [31 §10](31_platform_build_and_tools.md).

**Files.** Input: every top-level `data/levels/*.txt`; output `code/mapcrc.h` (87 `MAPID_*` defines).

**Algorithm (binary)**

| Aspect | Value |
|---|---|
| Bytes covered | raw file bytes read with `BinaryReader::ReadU8` until `m_eof`, **plus one trailing 0xFF** (the failing EOF read is stored before the loop tests `m_eof`) — identical for disk and archive readers ([fmt-binary-common](#fmt-binary-common)) |
| Size cap | static buffer `MAXSIZE` = 100 000; the loop stores index 0…100 000 before breaking, so a file ≥ 100 000 bytes overflows the buffer by one byte; largest shipped level 46 085 bytes |
| CRC | CRC-32, reflected polynomial 0xEDB88320, **initial value 0, no final XOR** (`CRC(0, buf, len)`) — not the standard CRC-32 |
| Result | `int`; −1 from `CalculeCRC` if the file cannot be opened |
| Line endings | hashed as stored: CRLF ↔ LF conversion changes the id |

**Generated header grammar** [C] `tools/MapCRC/main.cpp:GenerateMapCRCs`:

```ebnf
mapcrc_h  = '#ifndef _included_mapcrc_h' EOL , '#define _included_mapcrc_h' EOL , EOL , EOL ,
            '// Auto-generated IDs for each map.' EOL , '// DO NOT MODIFY' EOL , EOL , EOL ,
            { define_line } , EOL , EOL , '#endif' EOL ;
define_line = '#define   MAPID_' SANITISED ( padded to 40 ) ' 0x' HEX8 EOL ;   (* printf "#define   MAPID_%-40s 0x%08x\n" *)
SANITISED   = upper-case file name without ".txt", with '.', '-' and ' ' replaced by '_' ;
```

| Field | Type | Semantics |
|---|---|---|
| `SANITISED` | identifier | `MAPID_MP_KOTH_2P_1` etc. |
| `HEX8` | 8 lower-case hex digits | the CRC above |

**Verification.** `FileCRC`/`ReadFileData` re-read for this document; `BinaryDataReader::ReadU8` confirmed to return 0xFF and set `m_eof` past the end. `_work/scripts/P11_mapcrc.py` reproduces all 87 `MAPID_*` values in `code/mapcrc.h` (87 OK, 0 mismatches) [C] (P11; re-run by S31).

**Minimal valid example.** A 2-byte file `AB` hashes the 3 bytes `41 42 FF` with init 0, no final XOR; the header line would be `#define   MAPID_EXAMPLE` + padding + ` 0x<crc>`.

**Discrepancies.** Peers start a match by map *file name* only; no CRC is exchanged before the simulation starts, so a locally modified map desyncs instead of being rejected [I] ([22](22_determinism_and_networking.md)).
