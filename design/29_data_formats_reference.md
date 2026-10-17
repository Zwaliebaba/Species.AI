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
| 1 | `m_name` | WORD, `strncpy` 63 chars into `char[64]` | – | – | keep ≤ 62 characters: `strncpy(m_name, word, 63)` does not terminate a 63+ character name; whether `m_name[63]` is zero depends on `new CameraMount()` value-initialising the POD [I: C++03 value-initialisation; MSVC 2005 behaviour not verified] (Resolved in part: X1a stated the name is unterminated; the class has no constructor, so this is compiler-dependent) | mount name, looked up case-insensitively (`LevelFile::GetCameraMount`) | `ParseCameraMounts` |
| 2–4 | `m_pos` | F32 ×3 | wu | – | – | camera position | same |
| 5–7 | `m_front` | F32 ×3 | direction | – | non-zero | normalised after reading | same |
| 8–10 | `m_up` | F32 ×3 | direction | – | non-zero | normalised after reading | same |

Names consumed by code [C]: `player<N>` (N = the local team's position index) is the start camera in Multiwinia (`code/main.cpp`, `sprintf(camname, "player%d", …)`); `start` is the default start mount (`code/main.cpp`, `SetTarget("start")`); `CamPosBefore` is reserved (`MAGIC_MOUNT_NAME_START_POS`, `code/level_file.h`). Shipped `mp_*`: `player0` 51, `player1` 51, `player2` 31, `player3` 13, `start` 7 rows [C] survey.

#### 4.1.12 `CameraAnimations` block [C] `LevelFile::ParseCameraAnims`, `WriteCameraAnims`

```ebnf
cam_anims = 'CameraAnimations_StartDefinition' EOL , { blank | animation } , 'CameraAnimations_EndDefinition' EOL ;
animation = ANIMNAME EOL , { blank | node } , 'End' EOL ;          (* ANIMNAME strncpy 63 into char[64]; same termination caveat as mounts *)
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

---

## 5 Configuration, input and credit lists

<a id="fmt-preferences"></a>
### 5.1 Preferences (`preferences.txt`, `default_preferences.txt`)

**Purpose.** Typed key/value user settings [C] `code/lib/preferences.cpp:PrefsManager` (`g_prefsManager`), `PrefsItem::PrefsItem(UnicodeString)`, `PrefsManager::Load`, `AddLine`, `IsLineEmpty`, `CreateDefaultValues`, `Save`, `SaveItem`. The key register (≈ 75 keys with call-site defaults) is in [09](09_meta_systems.md) (`_work/drafts/P17b_prefs_accesses.csv`, `_work/drafts/P14_platform_lib_pref_keys.csv`).

**Files**

| File | Role | Read by | Written by | Encoding |
|---|---|---|---|---|
| `<P>preferences.txt` (`<P>` = `%LOCALAPPDATA%\Introversion\Multiwinia\`, or the working directory with `--NoProfile`; [31 §5](31_platform_build_and_tools.md)) | user settings | `PrefsManager::Load` (UTF-16 if BOM, else bytes) | `PrefsManager::Save` (13 call sites) | written **UTF-16LE with BOM**, CRLF (`UnicodeTextFileWriter`, `_O_U16TEXT`) [C] `code/unicode/unicode_text_file_writer.cpp` |
| `data/default_preferences.txt` | overrides of the hard-coded first-run defaults | `CreateDefaultValues` via `Resource::GetTextReader` | never | ASCII |
| `<P>user_input_preferences.txt` | user key bindings — same writer format, different reader ([fmt-input-bindings](#fmt-input-bindings)) | `InputManager::parseInputPrefs` | a second `PrefsManager` in `code/interface/prefs_keybindings_window.cpp:ApplyKeybindingsButton::MouseUp` | UTF-16LE with BOM |

Life cycle [C] `code/app.cpp:App::App`, `PrefsManager::PrefsManager`, `PrefsManager::Load`: `App::App` parses `init.dat`, then constructs `PrefsManager(GetPreferencesPath())`. `Load` clears, opens the file (UTF-16, else 8-bit); if it cannot be opened → `CreateDefaultValues()`. Otherwise each line's `GetRestOfUnicodeLine()` goes to `AddLine`, except that the **first line** is validated: every character before the first blank or `=` must lie in `L'A'…L'z'` (code points 65–122; digits fail) — if not, nothing is added, defaults are recreated and saved immediately. Then, unconditionally, `SetInt("MultiwiniaControlMethod", 1)` ("Hack by Chris to disable old-style control"); the constructor adds `ControlMethod = 1` if `GetInt("ControlMethod")` returns −1. `App::SetPreferenceOverrides()` runs right after construction ([09](09_meta_systems.md)).

**Grammar (reader)** — each physical line is cleaned by the tokeniser (§3.1: `#` comment, CR/LF), reduced by `GetRestOfUnicodeLine()` (leading blank/`:`/`,` stripped), then parsed by `PrefsItem::PrefsItem`:

```ebnf
prefs_file = { line } ;
line       = empty_line | item_line ;
empty_line = text with no alphanumeric character before an optional '#' ;      (* IsLineEmpty *)
item_line  = { non_alnum } , KEY , SEPCHAR , { WS | '=' } , VALUE ;
KEY        = 1*ALNUM ;                (* iswalnum; '_' or '.' END the key; case-SENSITIVE (std::map ordered by wcscmp) *)
SEPCHAR    = any one character ;      (* overwritten by NUL and skipped unconditionally - normally ' ' or '=' *)
VALUE      = rest of the line, verbatim (trailing blanks kept; never contains '#') ;
```

**Value typing** [C] `PrefsItem::PrefsItem`:

| Condition on `VALUE` | Stored type | Conversion |
|---|---|---|
| first char `-` or a digit, no `.` | `TypeInt` | `wistringstream >> int` (leading number; `"1024x768"` → 1024; failure → 0) |
| first char `-` or a digit, exactly one `.` | `TypeFloat` | `wistringstream >> float` |
| first char `-` or a digit, two or more `.` | `TypeString` | e.g. `127.0.0.1` |
| anything else (incl. empty) | `TypeString` | verbatim |

**Field table**

| Element | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| key | ALNUM run | – | case-sensitive | item identity | `PrefsItem::PrefsItem` |
| value | int / float / string by the rule above | caller's default at each `Get*` site when absent | – | lookup is **type-strict**: `GetInt` on a float or string item, `GetFloat` on an int item (e.g. `DarwinianScaleFactor = 2`), `GetString` on a number → caller's default | `PrefsManager::GetInt`, `GetFloat`, `GetString`, `GetUnicodeString` |
| duplicate key | – | – | – | while loading a file: **first occurrence wins** (`"Duplicate preference item %s. Ignoring"`); during the `default_preferences.txt` pass (`s_overwrite = true`) the new value replaces the old | `PrefsManager::AddLine` |

**First-run defaults** [C] `PrefsManager::CreateDefaultValues` (order of `AddLine` calls; blank `"\n"` lines omitted): `ServerAddress = 127.0.0.1`, `BypassNetwork = 0`, `ServerPort = 4000`, `TextLanguage = unknown`, `TextSpeed = 15`, `HelpEnabled = 1`, `SoundLibrary = dsound`, `SoundMixFreq = 22050` (44100 Mac), `SoundMasterVolume = 255`, `SoundChannels = 32` (16 on slow Macs), `SoundHW3D = 0`, `SoundSwapStereo = 0`, `SoundMemoryUsage = 1`, `SoundBufferSize = 512`, `SoundDSP = 0`, `RenderLandscapeDetail = 1`, `RenderWaterDetail = 1`, `RenderBuildingDetail = 1`, `RenderEntityDetail = 1`, `RenderCloudDetail = 1`, `RenderPixelShader = 1`, `DarwinianScaleFactor = 2.00`, `ControlMouseButtons = 3` (1 Mac), `ControlMethod = 1`, `MultiwiniaControlMethod = 1`, (Linux/Mac only: `RenderLandscapeMode = 2`, `ManuallyScaleTextures = 0`), `BootLoader = firsttime`, `UserProfile = AccessAllAreas`, `ScreenOverscan = 0`, `RenderSpecialLighting = 0`, `Difficulty = 1` (`OTHER_DIFFICULTY`). `DEMOBUILD` variants (`StartMap`, `UserProfile = DemoUser`) are [B:DEAD]. Then each line of resource `default_preferences.txt` is added with `s_overwrite = true`. Shipped `data/default_preferences.txt` (17 lines) sets: `NetworkServerPort = 4000`, `NetworkClientPort = 4001`, `ServerAddress = 127.0.0.1`, `BypassNetwork = 0`, `ServerPort = 4000`, `TextSpeed = 15`, `ControlMethod = 1`, `WaitVerticalRetrace = 0`, `DarwinianScaleFactor = 2.0`, `MultiwiniaControlMethod = 1`, `#TextLanguage = english` (a comment) [C] file.

**Writer** [C] `PrefsManager::Save`, `SaveItem`:

1. Clear every item's `m_hasBeenWritten`; open `UnicodeTextFileWriter(m_filename, encrypt = false, assertOnFail = false)` (silently return if it cannot be opened); BOM U+FEFF.
2. For each remembered template line (`m_fileText`, load order): an empty line is written verbatim **without adding a newline** — blank lines and comments read from disk were reduced to empty strings, so they vanish; only the literal `"\n"` lines from `CreateDefaultValues` survive. For an item line the key is re-extracted by scanning to the first `=`: it starts at the first alphanumeric character and ends at the **last non-alphanumeric character before `=`**; the current item is written once as `KEY = VALUE\n` with `%ls = %d`, `%ls = %.2f` or `%ls = %ls`.
3. Items not yet written are appended in `std::map` (`wcscmp`) order.

**Verification.** Re-read for this document: `PrefsItem::PrefsItem(UnicodeString)`, `PrefsManager::Load`, `AddLine`, `IsLineEmpty`, `Save`, `SaveItem`. Data: `data/default_preferences.txt` (17 lines, 10 items, 1 commented) parses to the items listed; no shipped `preferences.txt` exists (runtime file).

**Minimal valid example** (shown as text; the game writes UTF-16LE):

```text
TextLanguage = german
ScreenWidth = 1600
ScreenHeight = 900
SoundMasterVolume = 200
DarwinianScaleFactor = 1.50
```

**Discrepancies and hazards** [C]:

* `ServerAddress`, `ServerPort`, `BypassNetwork` are written by both default sources but never read (live ports are `NetworkServerPort`/`NetworkClientPort`, `code/network/network_defines.h`).
* `WaitVerticalRetrace` defaults to **1** in `code/renderer.cpp` but `default_preferences.txt` sets 0; because `PrefsManager` is constructed before `main.dat` is mounted, a packaged install does not find `default_preferences.txt` unless it is loose, so first-run defaults come only from code [C structure; 31 §8.9] — whether any release shipped it loose is **29-U2**.
* Writer hazards: a template line `Key=1` (nothing between key and `=`) has no detectable key end and is copied verbatim **without a newline**, gluing it to the next line (the next key is lost on the following load) and the item is appended again at the end; `Key  = 1` (two blanks) yields the key `"Key "` and inserts a bogus empty item; a non-blank template line without `=` makes the key scan run past the end of the string; floats are rounded to 2 decimals; values pass through `vswprintf` as part of the format string, so `%` in a value is interpreted. Resolved: P14 and X1a each described part of the `Key=1` case; both effects occur.
* Reader edge cases: a key that runs to the end of the line makes the value pointer start one past the terminator (out-of-bounds read); the value-skip loop's `value == L'\0'` test compares a pointer, not a character (harmless).
* In the `s_overwrite` pass an existing key's template line is appended a second time (the second copy is skipped on save because the item is already written).

<a id="fmt-input-bindings"></a>
### 5.2 Input binding files

**Purpose.** Map the 164 game controls to keyboard, mouse and XInput inputs through a chain of parser drivers [C] `code/lib/input/input.cpp:InputManager::parseInputPrefs`, `parseInputSpecString`, `code/lib/input/inputspec.cpp:InputSpecTokens`, `code/main.cpp:InitialiseInputManager`, `ReadInputPreferences`, each `code/lib/input/inputdriver_*.cpp:parseInputSpecification`. Run-time meaning of every driver and condition, and the full default binding table: [06](06_player_control_and_input.md).

**Files and layering** [C] `code/main.cpp:ReadInputPreferences`, `code/lib/input/file_paths.cpp`:

| Layer | Path | Reader | `replace` | Selection |
|---|---|---|---|---|
| 1 system | `data/input/input_preferences.txt` (Win32); `input_preferences_mac.txt` (Mac); `input_preferences_darwinia.txt` (only if pref `MultiwiniaControlMethod` == 0, which `PrefsManager::Load` forces to 1 → unreachable) | `Resource::GetTextReader` | false (append) | `InputPrefs::GetSystemPrefsPath` |
| 2 keyboard layout | `data/input/keyboards/<layout>.txt` ([fmt-keyboard-layout](#fmt-keyboard-layout)) | `Resource::GetTextReader` (missing → skipped) | true | `InputPrefs::GetKeyboardLayout` |
| 3 user | `<P>user_input_preferences.txt` | `UnicodeTextFileReader`, else `TextFileReader` | true | `InputPrefs::GetUserPrefsPath` |

`replace = true` replaces the control's primary (first) binding instead of appending [C] `ControlBindings::bind`.

**Encoding.** Shipped files ASCII, CRLF; the user file is UTF-16LE with BOM (written by `PrefsManager`).

**Line grammar** [C] `InputManager::parseInputPrefs`:

```ebnf
binding_file = { line } ;
line         = blank
             | CONTROL '=' SPEC EOL            (* binding line *)
             | CONTROL '~' ICONPATH EOL        (* icon line *)
             | other EOL ;                     (* "Assignment not found." - parsing continues *)
CONTROL      = control name, case-insensitive (164 names of code/lib/input/control_types.inc) ;
(* '=' and '~' must each be the whole SECOND token (default separators): "Ctl=key a" is rejected *)
SPEC         = REST, re-split on WHITESPACE ONLY (istringstream >> string): ':' and ',' are ordinary characters here ;
ICONPATH     = REST with a trailing LF and/or CR removed ;   (* stored by ControlBindings::setIcon; last wins *)
```

An unknown `CONTROL` ("Control ID not found."), an unparsable spec ("Parse failed - <err> (state = n)") or a refused bind ("Binding failed.") only print a message to `cout` (to `<P>inputprefs_debug.txt` on Mac or `TARGET_DEBUG`) — parsing never aborts [C].

**Spec grammar (driver chain).** The token list is offered to the drivers in registration order; the first that returns a success state wins, otherwise the error of the driver that got furthest is reported. Registration order [C] `InitialiseInputManager`: `Conjoin`, `Chord`, `Invert`, `Idle`, `W32` (keyboard/mouse), `XInput` (only when `XINPUT1_3.dll` loads), `Prefs`, `Value`, `Alias`. Hence `&&` binds loosest, then `++`, then `not`.

```ebnf
spec    = conj ;
conj    = part , { '&&' , part } ;                    (* this driver needs >= 1 '&&'; at most one part may be non-BOOL *)
part    = chord | unary ;                             (* each part is re-parsed by the whole chain *)
chord   = leafnc , '++' , leafnc , { '++' , leafnc } ;   (* expands to N conjunctions: part i "down", others "pressed"; BOOL only *)
unary   = ( 'not' | '!' ) , unary | leaf ;            (* 'not' case-insensitive; operand must be BOOL *)
leaf    = 'key' KEYNAME BCOND
        | 'mouse' MBUTTON BCOND
        | 'mouse' ( 'axes' | 'wheel' ) ACOND
        | 'XInput' XBUTTON ( BCOND | 'repeat' )
        | 'XInput' XANALOG ACOND [ 'with' 'sensitivity' N ]   (* N integer 1..200, default 100 *)
        | 'XInput' XDIR ( BCOND | 'repeat' )                  (* sensitivity fixed at 100 *)
        | 'idle' ( 'atleast' | 'reaches' ) SECONDS             (* 'idle' case-insensitive; SECONDS integer > 0 *)
        | 'idle' 'no' 'longer'                                 (* 'longer' case-insensitive *)
        | 'pref' [ 'not' | '!' ] PREFKEY | 'pref' '!'PREFKEY   (* key must EXIST in g_prefsManager at parse time *)
        | 'value' ( 'Pecan' | 'Hummingbird' )
        | 'alias' CONTROL ;
leafnc  = leaf without its trailing condition word ;           (* the chord driver appends it *)
BCOND   = 'down' | 'up' | 'pressed' ;
ACOND   = 'release' | 'position' | 'move' | 'zero' | 'read' ;
MBUTTON = 'left' | 'right' | 'middle' | 'any' ;
XBUTTON = 'leftbumper' | 'rightbumper' | 'dpadleft' | 'dpadright' | 'dpadup' | 'dpaddown' | 'a' | 'b' | 'x' | 'y'
        | 'start' | 'back' | 'plug' | 'lefttriggerb' | 'righttriggerb'          (* BOOL *)
        | 'leftthumbstick' | 'rightthumbstick' ;                                 (* 2D|BOOL: click with BCOND *)
XANALOG = 'lefttrigger' | 'righttrigger'                                         (* 1D *)
        | 'leftthumbstick' | 'rightthumbstick' ;                                 (* 2D|BOOL: axes with ACOND *)
XDIR    = 'leftthumbstickleft' | 'leftthumbstickright' | 'leftthumbstickup' | 'leftthumbstickdown'
        | 'rightthumbstickleft' | 'rightthumbstickright' | 'rightthumbstickup' | 'rightthumbstickdown' ;
KEYNAME = case-insensitive entry of getKeyNames()[0 .. KEY_TILDE = 223] or 'any' (KEY_ANY = 255) ;
```

`KEYNAME` values [C] `code/lib/input/keynames.cpp` (via `W32InputDriver::getKeyId`): `BACKSPACE`, `TAB`, `ENTER`, `SHIFT`, `CONTROL`, `ALT`, `PAUSE`, `CAPSLOCK`, `ESC`, `SPACE`, `PGUP`, `PGDN`, `END`, `HOME`, `LEFT`, `UP`, `RIGHT`, `DOWN`, `INSERT`, `DEL`, `0`…`9`, `A`…`Z`, `0_PAD`…`9_PAD`, `ASTERISK`, `PLUS_PAD`, `MINUS_PAD`, `DEL_PAD`, `SLASH_PAD`, `F1`…`F12`, `NUMLOCK`, `SCRLOCK`, `COLON`, `EQUALS`, `COMMA`, `MINUS`, `STOP`, `SLASH`, `AT`, `OPENBRACE`, `BACKSLASH`, `CLOSEBRACE`, `QUOTE`, `TILDE`, plus `Unknown` (index 109); key names containing a blank (`unknown 0` …) cannot be written because specs are whitespace-split; `META` (index 224 > `KEY_TILDE`) is unreachable.

**Field table**

| Element | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| driver words `key`, `mouse`, `XInput`, `pref`, `value`, `alias`, mouse/XInput control names, condition words, `with`, `sensitivity`, `atleast`, `reaches`, `no` | word (cs, `std::string ==`) | – | as listed | – | `W32InputDriver::acceptDriver`, `XInputDriver::acceptDriver`, `XInputDriver::getControlID`, `IdleInputDriver::acceptDriver` |
| `idle`, `not`, `longer`, key names, control names | word (case-insensitive) | – | – | – | `IdleInputDriver::getConditionID`, `W32InputDriver::getKeyId`, `ControlBindings::getControlID` |
| condition word | word | **unknown or type-mismatched → `COND_DOWN` with spec type 0** (the scan stops on the NULL terminator, index 8 < `NumInputConditions` 10) | 8 table entries: `down`, `up`, `pressed` (BOOL); `release`, `position`, `move`, `zero`, `read` (ANALOG) | trigger condition | `code/lib/input/inputdriver.cpp:InputDriver::getDefaultConditionID` |
| `with sensitivity N` | integer | 100 | 1…200 | analogue scaling | `XInputDriver::parseInputSpecification` |
| `SECONDS` | integer | – | > 0 | idle threshold | `IdleInputDriver::parseInputSpecification` |
| parser state | enum `InputParserState` | – | `STATE_ERROR`, `STATE_WANT_DRIVER`, `STATE_WANT_CONTROL`, `STATE_WANT_COND`, `STATE_WANT_MODIFIER`, `STATE_WANT_OPTIONAL`, `STATE_BAD_EXTRA`, then success states `STATE_DONE`, `STATE_OVERSTEP`, `STATE_CONJ_ERROR` (`PARSE_SUCCESS(state)`) | a surplus token yields `STATE_OVERSTEP` for single-word drivers (`alias`, `pref`, `value`) | `code/lib/input/inputdriver.h` |

**Verification.** Re-read for this document: `InputManager::parseInputPrefs`, the driver registration in `InitialiseInputManager`, `InputDriver::getDefaultConditionID` and its condition table. Survey `X1a_input_lines.py` re-run over the 5 shipped files:

| File | binding lines | icon lines | rejected | spec heads |
|---|---|---|---|---|
| `data/input/input_preferences.txt` | 224 | 18 | 2: `UnitPrimaryFireDirected ~ …` (unknown control), `TaskManagerSwitchTask INPUT_TYPE_BOOL` (no `=`) | `key` 94, `XInput` 88, `mouse` 32, `SpaceNav` 4 (no such driver → parse failed), conjunctions 4, chord 1, `idle` 1 |
| `data/input/input_preferences_darwinia.txt` | 220 | 18 | same 2 | `key` 97, `XInput` 87, `mouse` 30, conjunctions 4, chord 1, `idle` 1 |
| `data/input/input_preferences_mac.txt` | 231 | 18 | same 2 | `key` 94, `XInput` 88, `mouse` 32, `SpaceNav` 4, conjunctions 11, chord 1, `idle` 1 |
| `data/input/keyboards/de_DE.txt` | 8 | 0 | 0 | `key` 8 |
| `data/input/keyboards/fr_FR.txt` | 8 | 0 | 0 | `key` 8 |

`_work/scripts/P10_bindings.py` (P10) independently re-implements the spec parser and decodes every line (`_work/drafts/P10_input_camera_preflines.csv`). Shipped specs `XInput lefttrigger repeat` and `XInput righttrigger repeat` (analogue + `repeat`) parse as "down" through the fallback above [C].

**User file** [C] `code/interface/prefs_keybindings_window.cpp:ApplyKeybindingsButton::MouseUp`: written by a dedicated `PrefsManager(GetUserPrefsPath())` that is `Clear()`ed, filled with `SetString(<control>, <binding string>)` for each user-rebindable control (plus the derived `IconsTaskManagerHide` = the Display binding with `down` → `up`) and `Save()`d — §5.1 writer format, one `Control = spec` line per control in `wcscmp` order; `=` is a separate token as `parseInputPrefs` requires. Because the narrow mirror of a UTF-16 disk file is not comment/CR-stripped (§3.1), `parseInputPrefs` trims a trailing LF/CR from icon paths explicitly; specs are immune (whitespace split). The constructor's `Load()` of a not-yet-existing file runs `CreateDefaultValues()` before `Clear()` discards them [C].

**Minimal valid example**

```text
# pan with WASD, select with left mouse, pad A confirms
CameraForwards  = key w pressed
CameraLeft      = key a pressed
CameraBackwards = key s pressed
CameraRight     = key d pressed
UnitSelect      = mouse left down
MenuActivate    = XInput a down
MenuActivate    ~ icons/button_a.bmp
```

**Discrepancies** (docs-vs-code, see the table at the end): `docs/input_preferences_syntax.txt` omits `~` icon lines, the `value` and `alias` drivers, `repeat`, `key any`/`mouse any`, the XInput direction controls and `lefttriggerb`/`righttriggerb`, and claims that a missing preference makes `pref X` false, whereas the code **rejects the binding at parse time** (`PrefsInputDriver::parseInputSpecification` returns `STATE_WANT_CONTROL`) [C].

<a id="fmt-keyboard-layout"></a>
### 5.3 Keyboard-layout override files

**Purpose.** Re-bind layout-dependent keys (camera movement) for non-QWERTY keyboards [C] `code/main.cpp:ReadInputPreferences`, `code/lib/input/file_paths.cpp:InputPrefs::GetKeyboardLayout`, `InputPrefs::IsKeyboardFrenchLocale`.

**Files.** `data/input/keyboards/de_DE.txt` (QWERTZ, e.g. `CameraZoom = key y pressed`), `data/input/keyboards/fr_FR.txt` (AZERTY: Left `q`, Up `a`, Forwards `z`, Zoom `w`); 8 camera bindings each. `<layout>` = pref `KeyboardLayout`, default = the [fmt-locales](#fmt-locales) entry for `GetKeyboardLayout(0) & 0xFFFF` (Win32 LANGID); empty on Mac. A layout without a file (e.g. `en_US`) is silently skipped [C].

**Encoding / grammar.** Exactly the [fmt-input-bindings](#fmt-input-bindings) line grammar, applied with `replace = true`.

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| binding line | as §5.2 | – | – | replaces the control's primary binding | `InputManager::parseInputPrefs` |
| file name | `<layout>.txt` | – | must match a `LAYOUTNAME` | `IsKeyboardFrenchLocale()` (layout == `fr_FR`) also switches a help image ([fmt-mwhelp](#fmt-mwhelp)) | `InputPrefs::IsKeyboardFrenchLocale` |

**Verification.** `X1a_input_lines.py`: both files 8 binding lines, 1 comment, 0 rejected.

**Minimal valid example** (`keyboards/xx_XX.txt`)

```text
# Dvorak-style camera keys
CameraForwards  = key comma pressed
CameraBackwards = key o pressed
```

**Discrepancies.** The layout path is cached on first use; "Restore defaults" in Options does not re-apply this layer ([06](06_player_control_and_input.md)) [C].

<a id="fmt-locales"></a>
### 5.4 `locales.txt` — Windows LANGID → keyboard layout

**Purpose / files.** `data/input/locales.txt`, read by `code/lib/input/file_paths.cpp:readLocaleNames`; consulted on Windows only, when pref `KeyboardLayout` is absent [C] `InputPrefs::GetKeyboardLayout`.

```ebnf
locales_file = { blank | LANGID LAYOUTNAME EOL } ;     (* LANGID = atoi(first token); LAYOUTNAME = REST verbatim *)
```

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| `LANGID` | INT | – | Win32 LANGID | key (`std::map::insert` — later duplicates ignored) | `readLocaleNames` |
| `LAYOUTNAME` | REST | – | trailing blanks become part of the file name | `keyboards/<LAYOUTNAME>.txt` | same |

**Verification.** Shipped entries: `1031 de_DE`, `1033 en_US`, `1036 fr_FR`, `2057 en_UK`, `4105 en_CA` (first line blank) [C] file.

**Minimal valid example**

```text
1043 nl_NL
```

**Discrepancies.** None found.

<a id="fmt-betatesters"></a>
### 5.5 Credit name lists (`betatesters.txt`, `multiwinia_betatesters.txt`)

**Purpose.** Free-text name lists merged into the credits [C] `code/UI/GameMenuWindow.cpp:ReadMultiwiniaBetaTesters`, `ReadCredits`, `code/loaders/credits_loader.cpp:CreditsLoader::GenerateBetaTesterList`.

| File | Parser | Rule | Tag |
|---|---|---|---|
| `data/multiwinia_betatesters.txt` (69 lines, 3 Latin-1 names) | `ReadMultiwiniaBetaTesters` | every physical line → `GetRestOfUnicodeLine()` + `" \n"`; empty lines give empty entries; the concatenation replaces the credits phrase whose text is exactly `*BETATESTERS` (`multiwinia_credits_55`) in `ReadCredits` | [B:FULL] (Credits page) |
| `data/betatesters.txt` (38 lines, 1 Latin-1 name) | `CreditsLoader::GenerateBetaTesterList` | `GetRestOfLine()`; lines with `strlen > 1` become names, laid out in 2 columns; **an empty line returns NULL and `strlen(NULL)` crashes** (the shipped file has none) | [B:LEGACY] (reached only via script `RunCredits` and the Darwinia sphere-world camera; `RunBootloader` is `USE_LOADERS`-only) |

```ebnf
name_list = { name_line } ;
name_line = REST EOL ;      (* leading blank, TAB, ':' or ',' stripped; '#' starts a comment *)
```

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| name | REST, single byte (Latin-1 widened byte-for-byte) | – | no leading `:`/`,`, no `#` | one credited person | as above |

**Verification.** Both parsers from X1a's reading [C]; files surveyed (CRLF, Latin-1 bytes, no blank line in `betatesters.txt`) [C] `X1a_text_survey.py`.

**Minimal valid example**

```text
A. Tester
B. "Nick" Example
```

**Discrepancies.** A name beginning with `:` or `,` loses that character; a name containing `#` is truncated [C].

---

## 6 Localisation and scripts

<a id="fmt-language"></a>
### 6.1 Language phrase tables and caption markup

**Purpose.** Phrase id → localised text, with input-mode variants and inline markup expanded once per input mode [C] `code/lib/language_table.cpp:LangTable::ParseLanguageFile`, `RebuildTables`, `RebuildTable`, `buildCaption`, `consumeMarker`, `consumeIfMarker`, `consumeKeyMarker`, `consumeOtherMarker`, `buildPhrase`, `markerLength`, `LookupPhrase`, `RawLookupPhrase`, `DoesPhraseExist`; selection `code/app.cpp:App::InitLanguage`, `App::TrySetLanguage`, `App::SetLanguage`.

**Files** [C] `X1a_language_survey.py` (re-run for this document):

| File | Encoding | Lines | Phrases | Coverage vs English |
|---|---|---|---|---|
| `data/language/english.txt` | single byte (2 Latin-1 bytes) | 3120 | 2423 | reference |
| `data/language/french.txt` | UTF-16BE + BOM | 3219 | 2514 | 7 missing, 98 extra |
| `data/language/german.txt` | UTF-16BE + BOM | 3186 | 2487 | 8 missing, 72 extra; one literal `\N` |
| `data/language/italian.txt` | UTF-16BE + BOM | 3035 | 2361 | 62 missing |
| `data/language/spanish.txt` | UTF-16BE + BOM | 3219 | 2514 | 7 missing, 98 extra |
| `data/language/russian.txt` | UTF-16LE + BOM | 2770 | 2141 | 282 missing |
| `data/language/japanese.txt` | UTF-16LE + BOM, no final newline | 281 | 220 | 2205 missing (partial) |
| `data/language/korean.txt` | UTF-16BE + BOM, no final newline | 218 | 160 | 2265 missing (partial) |
| `data/language/portuguese.txt` | UTF-16BE + BOM, mixed CRLF/LF, no final newline | 218 | 160 | 2264 missing (partial) |

The language files are packed into `language.dat` (excluded from `main.dat`) by `tools/build_language_dat.bat` ([31 §12](31_platform_build_and_tools.md)). Selection [C] `App::InitLanguage`: pref `TextLanguage` (default `unknown`; not on Mac), then the OS language (`GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE)`), `english`, then the first file listed in `language/`; the first name for which `language/<lower(name)>.txt` exists is used and stored back into `TextLanguage` (failure of all four is fatal). `SetLanguage(name)` parses `language/<name>.txt` and then, if present, an overlay `strings_<name>.txt` or else `strings_default.txt` from the `data/` root into the same table (later definitions override); calls `setlocale(LC_CTYPE, name)`; `chinese-trad`, `chinese-simp`, `korean`, `japanese` switch every font to the `square` title font with horizontal scale 1.0 (others 0.6) [C] `App::SetLanguage`.

**Line grammar** [C] `LangTable::ParseLanguageFile`:

```ebnf
lang_file   = { blank | phrase_line } ;
phrase_line = KEY , SEP , TEXT , EOL ;
KEY         = first token (narrow separator set " \t\n\r:,") ;      (* stored case-insensitively *)
SEP         = 1*( ' ' | TAB | ':' | ',' | FF ) ;                     (* FF only for UTF-16 files *)
TEXT        = REST after the section 3.1 cleaning ;                   (* a '#' anywhere ends the text *)
```

Rules [C]:

1. Duplicate `KEY` (case-insensitive `BTree` lookup): the **later definition replaces the earlier** (opposite of preferences). Shipped duplicates: `buildingname_grenadeturret`, `multiwinia_help_title_crates` (all full files), `multiwinia_help_breakformation`, `multiwinia_help_clearinfo` (Italian).
2. Escape: the two characters `\n` become **blank + newline** (`" \n"`); any other backslash stays literal (German `\N`). There is no way to write `#`, a leading blank/`:`/`,`, or a literal `[`…`]` pair.
3. Post-processing: a trailing newline is removed, and a CR or FF at length − 2 is replaced by NUL. An empty `TEXT` in a UTF-16 file reads index −1 (undefined; once each in French, German, Russian); in an 8-bit file a key alone on its line makes `GetRestOfLine()` return `NULL` and `UnicodeString(char const*)` calls `strlen(NULL)` — crash (english.txt has no such line) [C] `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::UnicodeString(char const*)`. (Resolved: P14 marked this crash [I]; the constructor shows it directly.)
4. UTF-16 files are decoded unit by unit (no surrogate pairs); 8-bit files are Latin-1.
5. The unterminated last line of UTF-16 or archived files is dropped (§3.1): `multiwinia_exclamation` is lost from `japanese.txt` and `korean.txt`, `multiwinia_option_5_5_1` from `portuguese.txt` [C] survey.

**Mode variants and final tables** [C] `RebuildTables`, `RebuildTable`, `wrong_suffix`, `chomp_mode_suffix`, `specific_key_exists`: two hash tables (case-insensitive) are built, keyboard (`INPUT_MODE_KEYBOARD`, suffix `_kbd`) and gamepad (`INPUT_MODE_GAMEPAD`, suffix `_xin`), lazily on first lookup and explicitly after `SetLanguage`.

| Raw key form | Entry in the table for mode M |
|---|---|
| `foo`, no `foo_<M>` exists | `foo` → caption(`foo`, M) |
| `foo`, `foo_<M>` exists | skipped (the specific one wins) |
| `foo_<M>` | `foo` → caption(`foo_<M>`, M) |
| `foo_<other>`, neither `foo` nor `foo_<M>` exists | `foo` → empty string (feature hidden in mode M) |
| `foo_<other>` otherwise | skipped |
| `part_*` | never in the final tables; usable only as `[part_x]` references |

**Caption markup** (expanded at table-build time) [C] `buildCaption`, `consumeMarker`, `consumeIfMarker`, `consumeKeyMarker`, `consumeOtherMarker`, `markerLength`:

```ebnf
text   = { char | marker } ;
marker = '[IFMODE KEYBOARD]' | '[ELSE]' | '[ENDIF]'      (* nestable conditional; the mode word is compared case-insensitively *)
       | '[KEY' CONTROLNAME ']'                          (* e.g. [KEYCAMERAFORWARDS] *)
       | '[' PHRASEKEY ']' | '[ ' PHRASEKEY ']' ;        (* inline another phrase; the leading-blank form emits one blank first *)
```

| Element | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| `[IFMODE KEYBOARD]` … `[ELSE]` … `[ENDIF]` | marker | – | only `KEYBOARD` understood | text written only in keyboard / only in gamepad mode; `[IFMODE X]` for any other X increments the depth but falls through to the phrase-reference rule (consumed, writes nothing); no shipped phrase uses IFMODE | `consumeIfMarker` |
| `[KEY<name>]` | marker | – | name 1…63 chars | replaced by the description noun of the input bound to control `<name>` (`InputManager::getControlID`, `getBoundInputDescription`); unknown/unbound → falls through to the phrase rule and is removed; English uses 87 | `consumeKeyMarker` |
| `[<key>]` | marker | – | key 1…63 chars, lower-cased | replaced by the mode-specific raw phrase (`<key>_kbd`/`<key>_xin` preferred, else `<key>`), recursively expanded; unknown → removed. **Any `[…]` pair is markup**: English `multiwinia_notpermitted  [Not permitted]` expands to an empty string (the id is unused, grep) | `consumeOtherMarker`, `buildPhrase` |
| output length | – | – | `SEPULVEDA_MAX_PHRASE_LENGTH` = 1024 code units; key buffers `MAX_READ_KEY_LENGTH` = 64, `MAX_FINAL_KEY_LENGTH` = 96 | truncation | `buildCaption` |
| runtime placeholder `*<letter>` | – | – | letters substituted in `code/`: `F`, `G`, `H`, `P`, `R`, `S`, `T`, `U`, `X` (`*T` team/player name, `*S` seconds); English also has `*K`, `*A` (no live substitution found) and the whole-phrase macro `*BETATESTERS` | replaced at display time by `UnicodeString::ReplaceStringFlag` (every `*` + letter); phrases are never printf formats, so `%` is literal | `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::ReplaceStringFlag` |

**Lookup** [C] `LookupPhrase`, `RawLookupPhrase`, `DoesPhraseExist`, `code/lib/language_table.h`: `LANGUAGEPHRASE(k)` = current-mode table lookup; NULL key or no table → `"ERROR (null)"`; **a missing id returns the id text itself** (no fallback to English); `RawLookupPhrase(k)` tries `k_<mode>` then `k`, else `"ERROR (k)"`; the miss result points into a shared member overwritten by the next miss (not thread-safe). With a partial language most UI text shows raw ids; whether retail editions shipped complete Japanese/Korean/Portuguese files is **29-U3**. Phrase-id generators in code: `multiwinia_gametype_<type>`, `multiwinia_param_<param>`, `buildingname_<Type>`, `entityname_<Type>`, `researchname_<Type>`, `cratename_<Name>`, `multiwinia_credits_<n>` (n = 1, 2, … until the first missing id), `bootloader_credits_<n>` [C] respective `GetNameTranslated`/`GetTypeNameTranslated`, `code/UI/GameMenuWindow.cpp:ReadCredits`. Word wrapping: [28](28_ui_framework.md).

**Verification.** Re-read: `ParseLanguageFile` (key via narrow `GetNextToken`, text via `GetRestOfUnicodeLine` for UTF-16 or `GetRestOfLine` for 8-bit), `markerLength`, `consumeIfMarker`, `consumeOtherMarker`. Survey `X1a_language_survey.py` over the 9 files: 120 `_kbd` and 75 `_xin` ids in each full file, no `part_` ids, 87 `[KEY…]` markers and 1 `[phrase]` marker per full file, the 3 dropped last lines above.

**Minimal valid example** (written for this document)

```text
# sample.txt - ASCII, or UTF-16 with BOM
menu_play                 Play
menu_play_xin             Press START to play
hint_zoom_kbd             Hold [KEYCAMERAZOOM] to zoom.\nRelease to stop.
hint_score                *T scored *S points
```

**Discrepancies / hazards.** An unterminated `[` makes `markerLength` return 0 and `consumeOtherMarker` move the read position back by one (`inOffset += len + 1` with `len = −2`), so `buildCaption` re-reads the same `[` indefinitely — a hang during `RebuildTables` [I: derived from the code arithmetic; no shipped phrase contains one, survey]. `consume*Marker` copy the partly built caption through freshly allocated buffers whose contents depend on heap reuse [I]. `docs/changes.txt` (Darwinia Steam notes) says the default language comes from `HKCU\software\valve\steam\language`; Multiwinia uses `GetLocaleInfo` and reads no registry key [C].

<a id="fmt-script"></a>
### 6.2 Cut-scene scripts [B:LEGACY]

**Purpose.** One-command-per-line Darwinia cut-scene scripts [C] `code/script.cpp:Script::RunScript`, `Script::Advance`, `Script::AdvanceScript`, `Script::GetOpCode`, `g_opCodeNames`. Meaning of each command: [30](30_scripting_tutorial_and_cutscenes.md).

**Files.** `data/scripts/*.txt` — 42 files, all Darwinia content; `scripts/*` is excluded from the retail `main.dat` [C] `tools/multiwinia_exclusions.txt`. `RunScript(name)`: a name containing `.txt` opens `data/scripts/<name>`; otherwise a Sepulveda phrase sequence (no-op in FULL); in a demo (`!IsFullVersion()`) names starting `launchpad_purpose` are refused [C]. No Multiwinia path ever starts a script ([30](30_scripting_tutorial_and_cutscenes.md)).

**Encoding.** ASCII, CRLF, newline-terminated (42/42) [C] survey.

**Execution model** [C]: `Advance()` runs at most **one line per call** (once per rendered frame) and only when no wait is pending; a blank or comment line consumes a frame; end of file closes the script; `SkipCutscene` input ends waits and, after `PermitEscape`, aborts the script.

```ebnf
script  = { line } ;
line    = [ command ] EOL ;
command = OPCODE [ ARG1 [ ARG2 [ ARG3 [ ARG4 ] ] ] ] { ignored } ;
OPCODE  = one of the 43 names of g_opCodeNames, case-insensitive ;   (* unknown -> opcode -1, debug assert, line ignored *)
ARG1    = token, available as text (nextWord) and as F32 (nextFloat = iv_atof, 0 if absent) ;
ARGn    = token read on demand with GetNextToken() (n > 1) ;           (* MISSING => CRASH unless noted *)
```

| Opcodes | Arguments (R required, O optional; w word, f F32, i INT) | Parser ref |
|---|---|---|
| `CamCut`, `CamAnim`, `EnterLocation`, `Say`, `TriggerSound`, `StopSound`, `DemoGesture`, `GiveResearch` | ARG1 w (absent → NULL passed on; most handlers dereference it) | `Script::AdvanceScript` |
| `CamMove` | ARG1 w mount, ARG2 f duration (s) R | same |
| `CamFov` | ARG1 f fov (degrees), ARG2 i immediate O (default true) | same |
| `CamBuildingFocus` | ARG1 i building, ARG2 f range R, ARG3 f height R | same |
| `CamBuildingApproach` | ARG1 i building, ARG2 f range, ARG3 f height, ARG4 f duration (all R) | same |
| `CamLocationFocus` | ARG1 w location, ARG2 f fov R | same |
| `Wait` | ARG1 f seconds (absent → 0) | same |
| `WaitRocket` | ARG1 i building, ARG2 w state R, ARG3 i data R | same |
| `Highlight` | ARG1 i building | same |
| `SetMission` | ARG1 w location, ARG2 w mission file R | same |
| `SetCutsceneMode` | ARG1 i (`atoi(nextWord)` — absent ⇒ CRASH; handler Sepulveda-only) | same |
| `DestroyBuilding` | ARG1 i building, ARG2 f intensity R | same |
| `ActivateTrunkPort`, `ActivateTrunkPortFull` | ARG1 i building | same |
| `CamGlobalWorldFocus`, `CamReset`, `ExitLocation`, `ShutUp`, `WaitSay`, `WaitCam`, `WaitFade`, `WaitPlayerNotBusy`, `ClearHighlights`, `GameOver`, `ResetResearch`, `RestoreResearch`, `RunCredits`, `GodDishActivate`, `GodDishDeactivate`, `GodDishSpawnSpam`, `GodDishSpawnResearch`, `TriggerSpam`, `PurityControl`, `ShowDarwinLogo`, `ShowDemoEndSequence`, `PermitEscape` | none (extra tokens ignored) | same |

**Verification.** `_work/scripts/X1a_script_check.py` (re-run) over 43 files (42 scripts + `data/purity/buyme.txt`): 37 of 43 opcodes used (`Say` 273, `WaitSay` 112, `CamMove` 72, `Wait` 54, `CamBuildingFocus` 52, `Highlight` 44, `WaitCam` 29, `CamReset` 29, `ClearHighlights` 26, `WaitFade` 24, `CamCut` 20, `CamFov` 20, `CamLocationFocus` 15, `EnterLocation` 14, `ExitLocation` 14, `TriggerSound` 14, `StopSound` 8, `PermitEscape` 7, `GodDishSpawnResearch` 5, `SetMission` 4, `DemoGesture` 4, `ShutUp` 4, `TriggerSpam` 4, `GiveResearch` 3, `GodDishSpawnSpam` 3, `WaitPlayerNotBusy` 2, `WaitRocket` 2, and once each `GameOver`, `ShowDarwinLogo`, `CamGlobalWorldFocus`, `CamBuildingApproach`, `ShowDemoEndSequence`, `ResetResearch`, `GodDishActivate`, `GodDishDeactivate`, `RestoreResearch`, `PurityControl`); no unknown opcode; no missing required argument; one surplus argument (`CamCut … 1` in `data/scripts/launchpad_victory.txt`, ignored). Never used: `CamAnim`, `RunCredits`, `SetCutsceneMode`, `DestroyBuilding`, `ActivateTrunkPort`, `ActivateTrunkPortFull`. `SCRIPT_TEST_ENABLED` (`Script::TestScript`) is never defined [B:DEAD].

**Minimal valid example**

```text
# outro.txt
CamFov       45 1
CamMove      overview 6.0
WaitCam
Wait         1.5
ExitLocation
```

**Discrepancies.** None in shipped data beyond the surplus argument above.

<a id="fmt-purity-misc"></a>
### 6.3 `purity/buyme.txt` — purity-control script [B:DEAD]

**Purpose.** A cut-scene script ([fmt-script](#fmt-script) grammar) for a "buy me" outro: `EnterLocation mine`, `CamMove buyme1 80`, `WaitFade`, `TriggerSound cutscene2`, six `Say puritycontrol_<n>` (n = 1…6), `WaitCam`, `WaitSay`, `StopSound cutscene2`, `PurityControl` [C] file.

**Files.** `data/purity/buyme.txt` only. No code path opens `data/purity/` (the only `purity` hits in `code/` are `#ifdef PURITY_CONTROL` guards and the `PurityControl` command) and every packaging exclusion list removes `purity` [C] `tools/multiwinia_exclusions.txt`.

**Encoding / grammar / fields.** As [fmt-script](#fmt-script); verified by `X1a_script_check.py` (included in the 43 files).

The `PurityControl` command (compiled into [B:FULL]) deletes every file in `users/<profile>/`, opens the "buy now" web page and calls `exit(0)` [C] `code/script.cpp:Script::RunCommand_PurityControl` — documented for completeness; a new game must not replicate it.

**Minimal valid example.** Any [fmt-script](#fmt-script) file, e.g. one line `Wait 1.0`.

**Discrepancies.** [I] The set was meant to be copied over `data/` in cracked builds (inferred from names and content).

<a id="fmt-purity"></a>
### 6.4 `data/purity/` — purity-control data set (directory convention) [B:DEAD]

**Purpose.** A self-contained Darwinia promotional/anti-piracy data set; each file uses a format defined elsewhere.

| File | Format | Content [C] file |
|---|---|---|
| `data/purity/buyme.txt` | [fmt-purity-misc](#fmt-purity-misc) | outro script |
| `data/purity/game.txt` | [fmt-game-txt](#fmt-game-txt) | cut-down `game.txt` whose containment-completion event runs `buyme.txt`: 11 locations, 0 global buildings, 11 research rows, 6 events |
| `data/purity/map_mine.txt` | [fmt-level-file](#fmt-level-file) (map) | `mine` map variant: world 2000×2000, one tile with a 5-power guide grid, no flatten areas; colour files `landscape_mine.bmp`, `waves_containmentfield.bmp`, `water_default.bmp` |
| `data/purity/mission_mine_enable.txt` | [fmt-level-file](#fmt-level-file) (mission) | camera mounts `start`, `buyme1`; no buildings or units |

**Verification.** `X1a_level_survey.py` (purity set, 2 files: block orders in §4.1.2) and `X1a_script_check.py` (buyme). `PURITY_CONTROL` is never defined [C].

**Minimal valid example.** Not applicable (convention over other formats).

**Discrepancies.** None.

---

## 7 Entity, gesture and audio tables

<a id="fmt-stats-txt"></a>
### 7.1 `stats.txt` — per-entity-type base stats

**Purpose.** Health, speed and reload rate for every `Entity` type [C] `code/worldobject/entity.cpp:EntityBlueprint::Initialise`, `EntityBlueprint::GetStat`, `Entity::SetType`. Required: `AppReleaseAssert("Couldn't open stats.txt")`.

**Files.** `data/stats.txt` (40 lines, 27 data rows). **Encoding.** ASCII, CRLF.

```ebnf
stats_file = { blank_or_comment | stat_row } ;
stat_row   = NAME HLTH SPED RATE EOL ;       (* WORD F32 F32 F32, MISSING => CRASH *)
```

**Binding is positional**: the k-th non-blank row is entity type k (enum order of §4.1.13); `NAME` is stored (`EntityBlueprint::m_names`, `strdup`) but never used to bind. More than `NumEntityTypes` (27) rows → `AppReleaseAssert("Too many entity blueprints defined")` (fatal); fewer rows leave the rest 0 [C].

| Field | Type | Units (file header) | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `NAME` | WORD | – | – | label only | name for debugging | `EntityBlueprint::Initialise` |
| `HLTH` | F32 | hit points | 0 | 0…255 effective (instance copy is `unsigned char`) | initial / maximum health | same; `Entity::SetType` |
| `SPED` | F32 | "m/s" = world units per second | 0 | – | `GetStat(type, StatSpeed)` returns `SPED × (1 + m_difficultyLevel/10)` for every type except `SpaceInvader`; `m_difficultyLevel` is 0 in Multiwinia unless a level carries `CreatedAsDifficulty` (only 3 Darwinia missions) → factor 1.0 [C] `code/app.cpp:App::UpdateDifficultyFromPreferences`, `code/location.cpp:Location::Init` | `EntityBlueprint::GetStat` |
| `RATE` | F32 | seconds between shots | 0 | – | reload time | same |

**Storage truncation** [C] `code/worldobject/entity.h:Entity::m_stats` (`unsigned char m_stats[NumStats]`): per-instance copies truncate fractions (Squadie RATE 0.5 → 0) and wrap values > 255 (SpaceShip SPED 500 → 244 on MSVC [I], [02](02_units_and_entities.md)); code reading `EntityBlueprint::GetStat` directly (e.g. `code/worldobject/insertion_squad.cpp`) sees the double.

Shipped rows [C] `data/stats.txt`:

| Row | Enum | NAME in file | HLTH | SPED | RATE |
|---|---|---|---|---|---|
| 0 | `TypeInvalid` | Invalid | 0 | 0 | 0 |
| 1 | `TypeLaserTroop` | LaserTroop | 20 | 15 | 1 |
| 2 | `TypeEngineer` | Engineer | 255 | 28 | 0 |
| 3 | `TypeVirii` | Virii | 10 | 32 | 0 |
| 4 | `TypeSquadie` | InsertionSquadie (label differs from `Entity::GetTypeName` "Squadie") | 75 | 30 | 0.5 |
| 5 | `TypeEgg` | Egg | 10 | 0 | 0 |
| 6 | `TypeSporeGenerator` | SporeGenerator | 200 | 20 | 10 |
| 7 | `TypeLander` | Lander | 255 | 40 | 1 |
| 8 | `TypeTripod` | Tripod | 155 | 20 | 1 |
| 9 | `TypeCentipede` | Centipede | 255 | 80 | 1 |
| 10 | `TypeSpaceInvader` | SpaceInvader | 10 | 40 | 1 |
| 11 | `TypeSpider` | Spider | 200 | 50 | 1 |
| 12 | `TypeDarwinian` | Darwinian | 50 | 20 | 3 |
| 13 | `TypeOfficer` | Officer | 255 | 30 | 3 |
| 14 | `TypeArmyAnt` | ArmyAnt | 20 | 25 | 0 |
| 15 | `TypeArmour` | Armour | 200 | 60 | 5 |
| 16 | `TypeSoulDestroyer` | SoulDestroyer | 255 | 150 | 1 |
| 17 | `TypeTriffidEgg` | TriffidEgg | 50 | 100 | 1 |
| 18 | `TypeAI` | AI | 255 | 255 | 255 |
| 19 | `TypeShaman` | Shaman | 200 | 25 | 2 |
| 20 | `TypeHarvester` | Harvester | 255 | 28 | 0 |
| 21 | `TypeNuke` | Nuke | 255 | 100 | 0 |
| 22 | `TypeTentacle` | Tentacle | 255 | 100 | 0 |
| 23 | `TypeMeteor` | Meteor | 255 | 350 | 0 |
| 24 | `TypeTank` | Tank | 150 | 50 | 2 |
| 25 | `TypeSpaceShip` | SpaceShip | 255 | 500 | 0 |
| 26 | `TypeDropShip` | DropShip | 255 | 255 | 0 |

**Verification.** `EntityBlueprint::Initialise` and `GetStat` re-read for this document (positional `entityIndex`, `iv_atof` for the three numbers, `TokenAvailable` guard); `X1a_text_survey.py`: 27 non-empty token lines.

**Minimal valid example** (a file may stop early — types without a row keep 0 stats; a real table needs all 27 rows in enum order):

```text
# NAME        HLTH  SPED  RATE
None          0     0     0
Trooper       25    18    1.5
Builder       200   30    0
```

**Discrepancies.** `data/stats.txt` header says `SPED` in m/s and `NAME` = creature name; rows bind by position, speed is scaled by difficulty and instance copies are truncated to `unsigned char` [C].

<a id="fmt-gestures"></a>
### 7.2 `gestures.txt` — gesture-classifier training set

**Purpose.** Training vectors for the Rubine-style gesture recogniser (algorithm and symbol meanings: [06](06_player_control_and_input.md)) [C] `code/app.cpp:App::App` (`new Gesture("gestures.txt")`), `code/gesture.cpp:Gesture::LoadTrainingData`, `TrainSystem`, `SaveTrainingData`. Required: `AppReleaseAssert("Couldn't open Gesture data file")`. Gestures are not used in Multiwinia play, but the file is parsed at start-up. Written only by the gesture editor (`GESTURE_EDITOR`, [B:DEBUG]) [C] `code/interface/gesture_window.cpp`.

**Files.** `data/gestures.txt` (17 lines = 1 + 8 × 2). **Encoding.** ASCII, CRLF; longest line 2 767 characters.

```ebnf
gestures_file = count_line , { symbol } ;                (* exactly numSymbols symbol records *)
count_line    = INT EOL ;                                (* numSymbols *)
symbol        = trainer_count_line , [ vector_line ] ;   (* vector_line present iff trainerCount > 0 *)
trainer_count_line = INT EOL ;                           (* trainerCount <= 32 *)
vector_line   = { FLOAT } EOL ;                          (* trainerCount x 14 floats on ONE line: f0..f13 per trainer *)
```

Strictly line-positional: every `ReadLine()` is followed by unguarded `GetNextToken()` calls, so a blank line, a comment-only line or a vector split over two lines crashes (`atoi`/`atof(NULL)`) [C].

| Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `numSymbols` | INT | – | – | shipped 8 | number of symbol classes; symbol id = record order ([06](06_player_control_and_input.md)) | `Gesture::LoadTrainingData` |
| `trainerCount` | INT | examples | – | 0…32 (`GESTURE_MAX_TRAINERS`; larger overflows `Classification::m_trainers[32]`, no check); shipped 20 each | training examples of this symbol | same |
| `f0` | FLOAT (`atof`) | – | – | always 0.000000 | unused slot (features are numbered 1…`GESTURE_MAX_FEATURES` = 13) | same |
| `f1`…`f13` | FLOAT | pixels, radians, px²/ms², ms per feature ([06](06_player_control_and_input.md)) | – | – | Rubine features of one stroke | same |

**Verification.** `LoadTrainingData` re-read (loop `f = 0 … GESTURE_MAX_FEATURES` inclusive = 14 values per trainer); `code/gesture.h` constants re-read. Shipped file: 8 symbols × 20 trainers, each vector line 280 values + trailing blank [C].

**Minimal valid example** (2 symbols; the second has no trainers; values illustrative)

```text
2
1
0.000000 0.250000 0.968246 95.000000 1.100000 60.000000 0.600000 0.800000 120.000000 0.200000 0.450000 0.010000 3.100000 350.000000
0
```

**Discrepancies.** Writer asymmetry [C] `SaveTrainingData`: it writes `"%d\n"`, the vectors as `"%f "`, and **always** a trailing `"\n"` — for a symbol with 0 trainers this produces an empty line that `LoadTrainingData` then reads as the next trainer count (crash). The example above is hand-written without that empty line.

<a id="fmt-gesture-demo"></a>
### 7.3 `gesture_demos/*.txt` — recorded gesture strokes [B:DEAD]

**Purpose.** Mouse-path recordings replayed by the Darwinia mentor tutorial [C] `code/gesture_demo.cpp:GestureDemo::GestureDemo`, `GetCoords`, `GetTotalDuration`; recorder `GestureRecordingThread` writes `gesture_demos/new_gesture_demo.txt`; loader `code/lib/resource.cpp:Resource::GetGestureDemo`. All of it is inside `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` (never defined) and the files are excluded from packaging (`gesture_demos/*`) [C].

**Files.** `data/gesture_demos/airstrike.txt` (59 samples), `armour.txt` (72), `controller.txt` (47), `engineer.txt` (39), `grenade.txt` (16), `officer.txt` (27), `rocket.txt` (45), `squad.txt` (36). **Encoding.** ASCII.

```ebnf
demo_file   = { sample_line } ;
sample_line = X Y EOL ;            (* FLOAT FLOAT, written "%.3f %.3f"; every physical line is a sample - blank => atof(NULL) crash *)
```

| Field | Type | Units | Default | Range (shipped) | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `X` | FLOAT | screen heights | – | 0.271–0.971 | x / screen height at record time; origin left | `GestureDemo::GestureDemo` |
| `Y` | FLOAT | screen heights | – | 0.091–0.645 | y / screen height; origin top | same |
| sample period | constant | ms | 40 (`GESTURE_DEMO_SAMPLE_PERIOD`, 25 Hz) | – | duration = samples × 0.040 s | `GetTotalDuration` |

Playback [C] `GetCoords`: `index = t · 25`, interpolate between the floor and ceil samples, multiply both coordinates by the current `ScreenH()`.

**Verification.** From X1b's reading of `gesture_demo.cpp` [C]; sample counts and ranges from the X1b survey of all 8 files [C].

**Minimal valid example**

```text
0.400 0.300
0.450 0.350
0.500 0.300
```

**Discrepancies.** Interpolation weights are swapped (`factor1 = frac` multiplies the lower sample), so motion between samples runs backwards within each 40 ms step; the header comment says positions are "scaled for the current screen resolution" but both axes are scaled by the height only [C].

<a id="fmt-sounds-txt"></a>
### 7.4 `sounds.txt` — sound-event blueprints

**Purpose.** Binds every (sound source, event) to playback parameters, DSP effects and samples, and defines sample groups [C] `code/sound/soundsystem.cpp:SoundSystem::LoadBlueprints`, `ParseSoundEvent`, `ParseSoundEffect`, `ParseSampleGroup`, `NewSampleGroup`, `GetSampleGroup`; `code/sound/sound_parameter.cpp:SoundParameter::Read`, `GetParameterType`, `GetLinkType`, `GetUpdateType`; writer `SaveBlueprints`, `WriteSoundEvent`, `WriteSampleGroup`, `SoundParameter::Write` ([B:DEBUG] sound editor). Required: `AppReleaseAssert("Couldn't open sounds.txt")`. Runtime semantics: [10](10_audio_design.md), [27](27_audio_engine.md).

**Files.** `data/sounds.txt` (5 450 lines, 162 804 bytes). **Encoding.** ASCII, CRLF.

```ebnf
sounds_file      = { blank_or_comment | source_block | samplegroup | ignored_top } ;
source_block     = source_kind TYPENAME EOL , { blank_or_comment | event_block } , 'END' EOL ;
source_kind      = 'ENTITY' | 'BUILDING' | 'OTHER' ;
event_block      = 'EVENT' EVENTNAME EOL , { event_field } , 'END' EOL ;        (* NO blank/comment lines inside *)
event_field      = 'SOUNDNAME' NAME EOL
                 | 'SOURCETYPE' INT EOL | 'POSITIONTYPE' INT EOL | 'INSTANCETYPE' INT EOL
                 | 'LOOPTYPE' INT EOL | 'TEAMMATCHTYPE' INT EOL | 'PRIORITY' INT EOL
                 | 'MINDISTANCE' FLOAT EOL
                 | param_field_name param_spec EOL
                 | effect_block ;
param_field_name = 'VOLUME' | 'FREQUENCY' | 'ATTACK' | 'SUSTAIN' | 'RELEASE' | 'LOOPDELAY' ;
effect_block     = 'EFFECT' EFFECTNAME EOL , { PARAMLABEL param_spec EOL } , 'END' EOL ;   (* <= 13 params, positional; NO blank lines *)
param_spec       = 'PARAMETER' ( fixed | ranged | linked ) [ update_type ] ;
fixed            = 'TypeFixedValue'   FLOAT ;                       (* value *)
ranged           = 'TypeRangedRandom' FLOAT FLOAT FLOAT ;           (* lower upper smooth *)
linked           = 'TypeLinked'       FLOAT FLOAT FLOAT FLOAT FLOAT LINKNAME ;   (* inLower outLower inUpper outUpper smooth link *)
LINKNAME         = 'Nothing' | 'HeightAboveGround' | 'Xpos' | 'Ypos' | 'Zpos' | 'Velocity' | 'CameraDistance' ;
update_type      = 'UpdateConstantly' | 'UpdateOncePerLoop' ;        (* absent -> UpdateConstantly *)
samplegroup      = 'SAMPLEGROUP' GROUPNAME EOL , { ANYWORD SAMPLENAME EOL } , 'END' EOL ;   (* NO blank lines *)
ignored_top      = any other first token , EOL ;                       (* e.g. a stray END *)
```

All keywords and enumerator names are case-insensitive (`stricmp`); `#` comments are allowed only where blank lines are allowed.

Parser-structure facts [C] (`LoadBlueprints`, `ParseSoundEvent`, `ParseSoundEffect`, `ParseSampleGroup` re-read for this document):

* **Blank/comment lines** are allowed at top level and between `EVENT` blocks (those loops test `TokenAvailable`) and **crash** inside `EVENT`, `EFFECT` and `SAMPLEGROUP` bodies (`ReadLine(); GetNextToken(); stricmp(tok, "END")` → NULL).
* Inside a source block any non-`END` line is treated as an `EVENT` header (debug assert) and its second token becomes the event name.
* `TYPENAME` resolution (case-insensitive): `ENTITY` → `Entity::GetTypeId` over the 27 names of §4.1.13; `BUILDING` → `Building::GetTypeId` over the 75 names of §4.1.10; `OTHER` → `SoundSourceBlueprint::GetSoundSoundType` over `Laser`, `Grenade`, `Rocket`, `AirStrikeBomb`, `Spirit`, `Sepulveda`, `Gesture`, `Ambience`, `Music`, `Interface`, `CratePowerup`, `MultiwiniaInterface`. An unknown name returns −1 and is stored at index −1 (debug assert only → memory corruption in release). A duplicate source block for the same type replaces the earlier one (debug assert only; the old one leaks).
* Event identifier = `"<TYPENAME> <EVENTNAME>"` with `TYPENAME` as written (≤ 127 chars; `"sample group"` for groups) [C] `SoundInstance::SetEventName`.
* `SOUNDNAME` is lower-cased in place and its last `.extension` stripped (`RemoveExtension`) before storing (≤ 255 chars); with `SOURCETYPE 0` it names a sample stem, with `SOURCETYPE 1` a `SAMPLEGROUP` (looked up case-insensitively at play time; first match wins).
* An unknown event field is a debug assert; release skips the line.
* `SoundParameter::Read`: the word after the field name is only debug-asserted to be `PARAMETER`; an unknown type name gives `m_type = −1` and no values are read; after the type-specific values, *if a token remains* it is the update type (unknown → −1); further tokens ignored; `Recalculate()` is called immediately. Values are `atof` (not F32).
* `EFFECT EFFECTNAME`: matched case-insensitively against [fmt-effects-txt](#fmt-effects-txt) names (unknown → debug assert, type −1 → crash when used). Each following line: first token = label (ignored), rest = `param_spec`; parameter *k* binds to blueprint parameter *k* by **position**, into `DspHandle::m_params[MAX_PARAMS = 13]` (no bounds check).
* `SAMPLEGROUP`: the first token of each member line is ignored (shipped files write `SAMPLE`); member names are lower-cased and extension-stripped; group names stored as written (≤ 255); **duplicate group names are both kept**, look-ups return the first.

**Field table (event fields)**

| Field | Type | Units | Default (`SoundInstance` ctor) | Valid values | Semantics | Shipped usage (296 EVENT blocks) | Parser ref |
|---|---|---|---|---|---|---|---|
| `SOUNDNAME` | token | – | `"[???]"` | sample stem or group name | what to play | 296 (one literally `[???]`, `OTHER Sepulveda EVENT TextAppear`, dead source) | `ParseSoundEvent` |
| `SOURCETYPE` | INT | enum | 0 | 0 Sample, 1 SampleGroupRandom | sample vs group | 0: 143, 1: 153 | same |
| `POSITIONTYPE` | INT | enum | 2 | 0 Type2D, 1 Type3DStationary, 2 Type3DAttachedToObject, 3 TypeInEditor | positioning | 0: 83, 1: 42, 2: 169, 3: 2 | same |
| `INSTANCETYPE` | INT | enum | 0 | 0 Polyphonic, 1 MonophonicRandom, 2 MonophonicNearest | voice policy | 0: 290, 1: 3, 2: 3 | same |
| `LOOPTYPE` | INT | enum | 0 | 0 SinglePlay, 1 Looped, 2 LoopedADSR | looping | 0: 210, 1: 70, 2: 16 | same |
| `TEAMMATCHTYPE` | INT | enum | 0 | 0 AllTeams, 1 MyTeamOnly, 2 EnemyTeamOnly | audibility filter | 0: 284, 1: 8, 2: 4 | same |
| `PRIORITY` | INT | – | – | any | parsed and discarded (backwards compatibility) | 0 | same |
| `MINDISTANCE` | FLOAT | world units | 100.0 | > 0 | 3-D attenuation start | 296 | same |
| `VOLUME` | param | volume units 0–10 | fixed 7.0 | – | loudness | 296 | `SoundParameter::Read` |
| `FREQUENCY` | param | multiplier | fixed 1.0 | – | pitch | 119 | same |
| `ATTACK`, `SUSTAIN`, `RELEASE`, `LOOPDELAY` | param | seconds | fixed 0.0 | – | envelope / loop gap | 27 / 10 / 64 / 18 | same |
| `EFFECT … END` | block | – | none | ≤ 13 params | DSP chain | 22 (Echo 12, DSound_Echo 5, Gargle 3, DSound_I3DL2Reverb 1, DSound_WavesReverb 1) | `ParseSoundEffect` |

Parameter tokens in shipped data: `TypeFixedValue` 512, `TypeRangedRandom` 79, `TypeLinked` 30; links `Velocity` 16, `HeightAboveGround` 7, `CameraDistance` 4, `Ypos` 3; update types `UpdateConstantly` 509, `UpdateOncePerLoop` 112 (none omitted). Top level: 21 `ENTITY`, 35 `BUILDING`, 12 `OTHER` blocks, 131 `SAMPLEGROUP` blocks (130 distinct names, 1 126 member lines) [C] survey.

**Sample resolution** [C] `code/sound/sample_cache.cpp:CachedSample::CachedSample`, `code/lib/resource.cpp:Resource::GetSoundStreamDecoder`, `Resource::SoundExists`: a stem `s` resolves to `sounds/s.ogg`, else `mwsounds/s.ogg` (loose, then archives); if neither exists, `AppReleaseAssert("Failed to open sound stream decoder")` — fatal.

**Writer format** (editor output) [C] `WriteSoundEvent`, `SoundParameter::Write`: `\tEVENT %-20s`, then fixed-order fields `SOUNDNAME`, `SOURCETYPE`, `POSITIONTYPE`, `INSTANCETYPE`, `LOOPTYPE`, `TEAMMATCHTYPE`, `MINDISTANCE %2.2f`, `VOLUME` always, `FREQUENCY` only if not fixed 1.0, `ATTACK`/`SUSTAIN`/`RELEASE`/`LOOPDELAY` only if not fixed 0.0, each `%-18s PARAMETER %-18s` + `%8.2f` values (+ link name) + two blanks + update type; effects as `\t\tEFFECT %s` + one `\t\t\t` line per blueprint parameter + `\t\tEND`; `\tEND`. Sources are written in enum order preceded by a `# ====…` comment line and followed by `END` and two blank lines; groups as `\tSAMPLE  <name>`. Two decimals → an editor round trip quantises every parameter to 0.01.

**Verification.** Parsers re-read as listed. `_work/scripts/X1b_sounds_grammar.py` (independent state machine, re-run) over `data/sounds.txt`: 0 blank-line hazards, 0 unknown sources, 0 duplicate source blocks, effect parameter counts equal to the blueprint counts in every block (Echo 3, DSound_Echo 5, Gargle 3, DSound_I3DL2Reverb 13, DSound_WavesReverb 4), 0 parameter-label mismatches against [fmt-effects-txt](#fmt-effects-txt) order; all 1 228 referenced sample files exist (only unresolved name `[???]`). P16's `P16_parse_sounds.py` produced the same event set (`_work/drafts/P16_sound_events.csv`).

**Minimal valid example** (written for this document)

```text
SAMPLEGROUP crowd
    SAMPLE  cheer1
    SAMPLE  cheer2
END

OTHER Interface
    EVENT ButtonPress
        SOUNDNAME          click1.ogg
        SOURCETYPE         0
        POSITIONTYPE       0
        INSTANCETYPE       0
        LOOPTYPE           0
        TEAMMATCHTYPE      0
        MINDISTANCE        50.00
        VOLUME             PARAMETER TypeFixedValue         8.00  UpdateConstantly
        FREQUENCY          PARAMETER TypeRangedRandom       0.95     1.05     0.00  UpdateOncePerLoop
    END
    EVENT Crowd
        SOUNDNAME          crowd
        SOURCETYPE         1
        POSITIONTYPE       0
        INSTANCETYPE       1
        LOOPTYPE           0
        TEAMMATCHTYPE      0
        MINDISTANCE        100.00
        VOLUME             PARAMETER TypeFixedValue         6.00  UpdateConstantly
        EFFECT             SimpleReverb
            WetDryMix      PARAMETER TypeFixedValue        25.00  UpdateConstantly
        END
    END
END
```

**Discrepancies** [C]:

* `SAMPLEGROUP officercreate` is defined twice (identical; harmless).
* `SOUNDNAME [???]` in a dead source.
* 16 sample files referenced by `data/sounds.txt` are in neither `tools/sounds.txt` nor `tools/menu_sounds.txt`; five (`mwsounds/rollover1a`, `rollover1c`, `mouseclick1`, `chat`, `beep_harsh_malfunction`) reach `sounds2.dat` through the catch-all rule ([fmt-pack-list](#fmt-pack-list)); the other **11 are excluded from every archive** (`sounds/tresk*`, `trash80*`, `dma_sc*`, `twoataris*`, `pang*`, `speccyloader.ogg`, `matrix.ogg`, `fodder.ogg`) although `OTHER Music` events `Cutscene1`–`Cutscene5`, `LoaderRaytrace`, `LoaderFodder`, `LoaderSoul`, `LoaderSpeccy`, `LoaderAmiga`, `LoaderMatrix`, `LoaderGameOfLife`, `EndSequence`, `Credits`, `StartSequence` reference them → triggering any in a packaged build would hit the fatal `CachedSample` assert [C]; unreachable in Multiwinia [I] ([10](10_audio_design.md)).
* Code fires events with no data block (e.g. CRATE `Expired`, match-end voice events) — silent [C] ([10](10_audio_design.md)).

<a id="fmt-effects-txt"></a>
### 7.5 `effects.txt` — DSP filter blueprints (audio)

**Purpose.** Declares the 14 real-time audio filter types and their parameter ranges, used by `EFFECT` blocks of [fmt-sounds-txt](#fmt-sounds-txt) and the sound editor [C] `code/sound/soundsystem.cpp:SoundSystem::LoadEffects`, `DspBlueprint::GetParameter`; consumption `code/sound/sound_instance.cpp:DspHandle::Advance`. Despite its name the file is audio-only (the original spec listed it under animation/VFX). Required: `AppReleaseAssert("Couldn't load effects.txt")`.

**Files.** `data/effects.txt` (115 lines). **Encoding.** ASCII, CRLF.

```ebnf
effects_file = { blank_or_comment | effect } ;
effect       = 'EFFECT' NAME EOL , { param_line } , 'END' EOL ;   (* NO blank line after EFFECT or inside *)
param_line   = NAME MIN MAX DEFAULT DATATYPE EOL ;                 (* WORD FLOAT FLOAT FLOAT INT, MISSING => CRASH *)
```

Top level skips blank lines; any other first token is only debug-asserted to be `EFFECT`. Inside an effect, `ReadLine(); GetNextToken(); stricmp(param, "END")` runs unguarded (blank line → crash). **Binding is positional**: effect index = order of blocks = `SoundLibrary3d` filter enum (`code/sound/sound_library_3d.h`), stored with `DArray::PutData` into an array pre-sized to `NUM_FILTERS`; parameter index = line order. The effect *name* matters only for `sounds.txt` look-up (case-insensitive) and the editor; parameter names are labels [C].

| Index | `EFFECT` name | Enum | Parameters (name min..max default, type) |
|---|---|---|---|
| 0 | DSound_Chorus | `DSP_DSOUND_CHORUS` | WetDryMix 0..100 50 f; Depth 0..100 10 f; Feedback −99..99 25 f; Frequency 0..10 1.1 f; Waveform 0..1 1 L; Delay 0..20 16 f; Phase 0..4 3 L |
| 1 | DSound_Compressor | `DSP_DSOUND_COMPRESSOR` | Gain −60..60 0 f; Attack 0.01..500 10 f; Release 50..3000 200 f; Threshold −60..0 −20 f; Ratio 1..100 3 f; PreDelay 0..4 4 f |
| 2 | DSound_Distortion | `DSP_DSOUND_DISTORTION` | Gain −60..0 −18 f; Edge 0..100 15 f; PostEQCentreFreq 100..8000 2400 f; PostEQBandwidth 100..8000 2400 f; PreLowpassCutoff 100..8000 8000 f |
| 3 | DSound_Echo | `DSP_DSOUND_ECHO` | WetDryMix 0..100 50 f; Feedback 0..100 50 f; LeftDelay 1..2000 500 f; RightDelay 1..2000 500 f; PanDelay 0..1 0 L |
| 4 | DSound_Flanger | `DSP_DSOUND_FLANGER` | WetDryMix 0..100 50 f; Depth 0..100 100 f; Feedback −99..99 −50 f; Frequency 0..10 0.25 f; Waveform 0..1 1 L; Delay 0..4 2 f; Phase 0..4 2 L |
| 5 | DSound_Gargle | `DSP_DSOUND_GARGLE` | RateHz 1..100 20 L; WaveShape 0..1 0 L |
| 6 | DSound_I3DL2Reverb | `DSP_DSOUND_I3DL2REVERB` | Room −10000..0 −1000 L; RoomHF −10000..0 −100 L; RoomRolloffFactor 0..10 0 f; DecayTime 0.1..20 1.49 f; DecayHFRatio 0.1..2 0.83 f; Reflections −10000..1000 −2602 L; ReflectionsDelay 0..0.3 0.01 f; Reverb −10000..2000 200 L; ReverbDelay 0..0.1 0.01 f; Diffusion 0..100 100 f; Density 0..100 100 f; HFReference 20..20000 5000 f; Quality 0..3 2 L |
| 7 | DSound_ParamEQ | `DSP_DSOUND_PARAMEQ` | Centre 80..16000 8000 f; Bandwidth 1..36 12 f; Gain −15..15 0 f |
| 8 | DSound_WavesReverb | `DSP_DSOUND_WAVESREVERB` | InGain −96..0 0 f; ReverbMix −96..0 0 f; ReverbTime 0.01..3000 1000 f; HighFreqRTRatio 0.01..0.99 0.01 f |
| 9 | ResonantLowPass | `DSP_RESONANTLOWPASS` | CutoffFrequency 100..20000 2500 f; Resonance 0.1..20 7 f; Gain 0..10 1 f |
| 10 | BitCrusher | `DSP_BITCRUSHER` | BitRate 1..16 16 f |
| 11 | Gargle | `DSP_GARGLE` | WetDryMix 0..100 50 f; RateHz 0.01..100 20 f; WaveShape 0..1 0 L |
| 12 | Echo | `DSP_ECHO` | WetDryMix 0..100 50 f; Delay 1..3000 200 f; Attenuation 0..100 50 f |
| 13 | SimpleReverb | `DSP_SIMPLE_REVERB` | WetDryMix 0..100 50 f |

(f = data type 0, float; L = data type 1, `long`.) Filter maths: [27](27_audio_engine.md).

| Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| effect `NAME` | WORD ≤ 255 | – | – | – | editor label; `sounds.txt` look-up key | `LoadEffects` |
| parameter `NAME` | WORD ≤ 255 | – | – | – | editor label only | same |
| `MIN`, `MAX` | FLOAT (`atof`) | per parameter (%, ms, dB, Hz) | – | – | editor slider limits only (not enforced at run time) | same |
| `DEFAULT` | FLOAT | as above | – | – | **not applied at run time**: parameters omitted in a `sounds.txt` EFFECT block stay `SoundParameter()` = fixed 0.0; the editor's "add effect" seeds through `GetParameter(i, &defaultVal)` which fills the **min** slot (argument-order bug), so editor-created effects start at their minimum [C] `code/interface/soundeditor_window.cpp:DspEffectButton::MouseUp` | same |
| `DATATYPE` | INT | – | – | 0 float, 1 long | value passed to the DSP as `float` (0) or as a `long` reinterpreted into the float array (1); any other → `AppReleaseAssert("Unknown datatype")` | `DspHandle::Advance` |

**Verification.** `LoadEffects` re-read for this document (unguarded inner loop, positional `PutData`). Data: 14 blocks, 91 token lines [C] `X1a_text_survey.py`; `X1b_sounds_grammar.py` cross-check of every `sounds.txt` EFFECT block against this order (0 mismatches).

**Minimal valid example** (as the 14th block it would bind to `DSP_SIMPLE_REVERB` whatever its name)

```text
EFFECT RoomVerb
    WetDryMix   0   100   35   0
END
```

**Discrepancies.** `data/effects.txt` header says "Default: the starting value" — not applied (above). Hazard: `DspHandle::Advance` loops over all 13 slots and calls `GetParameter(i, …, &dataType)` even beyond the blueprint's count, leaving `dataType` uninitialised → possible spurious "Unknown datatype" assert; reached only when pref `SoundDSP` ≠ 0 (default 0) [C]. A file with a different number or order of effects silently re-binds DSP types.

<a id="fmt-ogg"></a>
### 7.6 Ogg Vorbis samples

**Purpose.** All sound samples and music [C] `code/sound/sound_stream_decoder.cpp:SoundStreamDecoder::ReadOggHeader`, `ReadOggData`, `ReadFunc`, `SeekFunc`, `TellFunc`; `code/sound/sample_cache.cpp:CachedSample::CachedSample`.

**Files.** `data/sounds/*.ogg` (954, inherited from Darwinia, all lower-case names), `data/mwsounds/*.ogg` (378, Multiwinia; 359 with upper-case letters, e.g. `Attack44.ogg`, `CrateReward_Tank.ogg`) — 1 332 in total. Per-file catalogue: [10](10_audio_design.md).

**Encoding / container.** Ogg Vorbis I, decoded with libvorbisfile through custom callbacks over `BinaryReader` (`ov_open_callbacks`; read/seek/tell map to the loose-file or in-memory archive reader; seek works for both). Output is always 16-bit signed native-endian PCM (`ov_read(…, 2, 1, …)`).

| Field / check | Type | Units | Default | Valid range | Failure | Parser ref |
|---|---|---|---|---|---|---|
| stream open | – | – | – | `ov_open_callbacks == 0` | `AppReleaseAssert` (fatal) | `SoundStreamDecoder::ReadOggHeader` |
| total length | frames | samples | – | `ov_pcm_total > 0`; cached samples 1…268 435 456 frames, ≤ 1 GiB of PCM | fatal | `ReadOggHeader`, `CachedSample::CachedSample` |
| channels | int | – | – | 1…8 (debug authoring check: should be **mono**, `IsSoundSourceOK`) | fatal outside range | `CachedSample::CachedSample`, `code/sound/soundsystem.cpp:SoundSystem::IsSoundSourceOK` |
| sample rate | int | Hz | – | 1…192 000 | fatal | `CachedSample::CachedSample` |
| decode errors | – | – | – | no `OV_HOLE`/`OV_EBADLINK` | fatal | `ReadOggData` |
| addressing | stem | – | – | no directory, no extension, any case; no blanks (debug check) | – | `CachedSample::CachedSample`, `Resource::SoundExists` |

Naming and placement [C]: a stem resolves to `sounds/<stem>.ogg` first, then `mwsounds/<stem>.ogg`; no stem exists in both directories; variants are numbered suffixes grouped by `SAMPLEGROUP`; upper-case `mwsounds/` names work because Windows file look-up and the archive hash table are case-insensitive ([fmt-dat-archive](#fmt-dat-archive)).

**WAV.** A RIFF/WAVE PCM reader (8/16-bit, mono/stereo, `fmt `/`data` chunks) exists (`SoundStreamDecoder::ReadWavHeader`, `ReadWavData`), selected by the reader's file extension, but `Resource::GetSoundStreamDecoder` always appends `.ogg`, so **WAV is unreachable** [C].

**Verification.** `_work/scripts/X1b_ogg_survey.py` (re-run) over all 1 332 files: every file mono; 1 330 at 44 100 Hz and 2 at 22 050 Hz (one per directory); nominal bit-rates `sounds/` 64 kbps ×862, 128 ×89, 32/60/80 ×1 each; `mwsounds/` 128 ×369, 96 ×8, 40 ×1; no blanks, no non-Ogg, no unreadable headers, no name in both directories [C]. Encoder/decoder binaries in tree: `tools/oggenc.exe`, `tools/oggdec.exe` (not read).

**Minimal valid example.** A mono 44 100 Hz Ogg Vorbis I stream saved as `data/mwsounds/sample_ping1.ogg`, referenced from `sounds.txt` as `SOUNDNAME sample_ping1`.

**Discrepancies.** `code/sound/sample_cache.cpp` comment "Largest song takes 13 MB" — the largest shipped track decodes to 29.0 MiB (the whole PCM buffer is allocated on open) [C] ([27](27_audio_engine.md)).

---

## 8 Geometry, images and animation

<a id="fmt-shp"></a>
### 8.1 `.shp` — hierarchical flat-shaded mesh ("shape")

**Purpose.** Every 3-D model (buildings, units, props) is a rigid hierarchy of *fragments* (sub-meshes with a local transform that code may animate) plus named *markers* (attachment frames). Text format, case-insensitive keywords [C] `code/lib/shape.cpp:Shape::Load`, `ShapeFragment::ShapeFragment(TextReader*, char const*)`, `ParsePositionBlock`, `ParseNormalBlock`, `ParseColourBlock`, `ParseVertexBlock`, `ParseAllStripBlocks`, `ParseStripBlock`, `ParseTriangleBlock`, `GenerateNormals`, `RegisterPositions`, `ShapeMarker::ShapeMarker(TextReader*, char const*)`. There is no binary variant [C]. Rendering, team colouring, hit tests: [24](24_rendering.md); per-shape catalogue: [25](25_textures_and_assets.md).

**Files.** `data/shapes/*.shp` — 139 files, 441 fragments, 425 markers; max 20 fragments and 38 markers per file; max marker depth 7; max 10 632 triangles, 6 245 positions and 121 colours in one fragment; 39 files use the strip encoding, 98 the triangle-list encoding, none mixes both; 2 contain markers only (`battlecannonbase.shp` 8 markers, `portal.shp` 5 markers, hanging off `SceneRoot`). Loaded as `data/shapes/<lower-cased name>` (loose, then archive) [C] `code/lib/resource.cpp:Resource::GetShapeCopy`, `DoesShapeExist`.

**Encoding.** ASCII; 138 files CRLF, `primaryupgradeport.shp` LF only.

```ebnf
shp_file        = { top_line } ;
top_line        = fragment_def | marker_def | other_line ;          (* other_line (incl. blank) ignored *)
fragment_def    = 'Fragment' NAME EOL , { frag_line } , geometry_end ;
frag_line       = 'ParentName' NAME EOL
                | 'up' F F F EOL | 'front' F F F EOL | 'pos' F F F EOL
                | 'Positions' INT EOL , pos_records
                | 'Normals'   INT EOL , vec_records
                | 'Colours'   INT EOL , col_records
                | 'Vertices'  INT EOL , vtx_records
                | ignored_line ;                                    (* e.g. "right x y z", blank, unknown keyword *)
geometry_end    = strips_block | triangles_block ;                   (* fragment parsing STOPS here *)
pos_records     = N * ( INT F F F EOL ) ;      (* id x y z;  lines not starting with a digit are skipped *)
vec_records     = N * ( INT F F F EOL ) ;      (* id nx ny nz *)
col_records     = N * ( INT INT INT INT EOL ) ;  (* id r g b *)
vtx_records     = N * ( INT INT INT EOL ) ;    (* id positionId colourId *)
strips_block    = 'Strips' INT EOL , strip , { strip } ;              (* INT strips, ids 0..INT-1 in order *)
strip           = 'Strip' INT EOL , [ 'Material' NAME EOL ] , 'Verts' INT EOL , { vref_line } ;
vref_line       = 1*( 'v' INT ) EOL ;           (* total token count = Verts *)
triangles_block = 'Triangles' INT EOL , N * ( INT INT INT EOL ) ;     (* exactly N physical lines, a,b,c *)
marker_def      = 'Marker' NAME EOL , { marker_line } , ( 'MarkerEnd' EOL | EOF ) ;
marker_line     = 'ParentName' NAME | 'Depth' INT | 'front' F F F | 'up' F F F | 'pos' F F F | ignored_line ;
F               = F32 ;
```

Record-block rules [C]:

* Inside id blocks a line whose first token does not start with a decimal digit is skipped (blank/comment allowed). Ids must be `0, 1, 2, …`; a wrong id in **Positions** silently abandons the block (no positions; the fragment then has no geometry); in Normals/Colours/Vertices it is only a debug assert (release writes at the wrong index). EOF inside a block is a debug assert.
* **Triangles** are read with exactly N `ReadLine` calls and no skipping: a blank or comment line inside crashes (`atoi(NULL)`).
* **Strips**: lines not starting with `Strip` are skipped between strips; after `Strip k` the next physical line must be `Material …` (name read and discarded) or `Verts n`; `n > 2` is debug-asserted; vertex tokens must start with `v` (debug assert) and be `< Vertices` (debug assert). `Strips 0` makes the strip loop read to EOF.
* A fragment line with a keyword but no value (`Normals` alone) passes NULL to `atoi`/`strdup` — crash.
* A fragment with no `Strips`/`Triangles` line never terminates: the fragment loop does not recognise `Fragment`/`Marker`, so it swallows the rest of the file.
* `Normals` are parsed and **always discarded** (`GenerateNormals` replaces them with one face normal per triangle); every shipped file has `Normals: 0`.

**Field table**

| Field | Type | Units | Default | Range / validation | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `Fragment` NAME | token | – | – | ≤ 100 fragments per file (debug assert; release stack overflow); unique (debug assert only for non-root parents) | fragment name, looked up case-insensitively | `Shape::Load`, `ShapeFragment::LookupFragment` |
| `ParentName` (fragment) | token | – | `unknown` | `SceneRoot` (case-insensitive; shipped `sceneroot`) or another fragment's name | parent; an unknown parent leaves the fragment unattached (not rendered, leaked) | `Shape::Load` |
| `up`, `front` | 3 × F32 | unit vectors | identity (`up` (0,1,0), `front` (0,0,1)) | normalised after parse | local orientation: `Matrix34::Normalise`: `f ← normalise(f); r ← normalise(u × f); u ← f × r` | `ShapeFragment::ShapeFragment`, `code/lib/matrix34.cpp:Matrix34::Normalise` |
| `pos` | 3 × F32 | world units | (0,0,0) | – | local translation | same |
| `Positions` N | INT + records | wu, fragment-local | none | N ≤ 65 535 (indices `unsigned short`) | unique vertex positions | `ParsePositionBlock` |
| `Colours` N | INT + records | 0…255 | none | > 255 wraps (`unsigned char`) | palette; **on load every channel × 0.85, truncated** (255 → 216); alpha forced 255 | `ParseColourBlock`, `code/lib/rgb_colour.cpp:RGBAColour::operator*=` |
| `Vertices` N | INT + records | indices | none | posId < Positions, colId < Colours (unchecked) | (position, colour) pairs used by faces | `ParseVertexBlock` |
| `Strips` / `Strip` / `Material` / `Verts` / `v<i>` | INT / INT / NAME / INT / tokens | vertex indices | – | n > 2 | strip i ≥ 2 with window (a,b,c) = (v[i−2], v[i−1], v[i]) emits a triangle only if a, b, c are pairwise distinct; even i → (a,b,c), odd i → (c,b,a) | `ParseStripBlock` |
| `Triangles` T | INT + records | vertex indices | – | < Vertices (unchecked) | explicit triangle list | `ParseTriangleBlock` |
| `Marker` NAME | token | – | – | ≤ 100 per file (debug assert) | attachment frame; `LookupMarker` (case-insensitive, depth-first: own markers, then children) or prefix scan (case-sensitive `strstr`) | `Shape::Load`, `ShapeFragment::LookupMarker`, `code/worldobject/building.cpp:Building::SetShapeLights`, `SetShapePorts` |
| `ParentName` (marker) | token | – | `unknown` | must name a fragment (debug assert) | fragment the marker hangs from | `Shape::Load` |
| `Depth` | INT | levels | **uninitialised** if absent | = number of ancestors of the parent fragment including `SceneRoot` + 1 (1 on `SceneRoot`, 2 on a root-level fragment) | size of `m_parents[]` (`m_parents[0]` = SceneRoot … `m_parents[Depth−1]` = parent) | `ShapeMarker::ShapeMarker`, `Shape::Load` |
| `up`/`front`/`pos` (marker) | 3 × F32 | as fragments | identity | normalised | frame relative to the parent fragment | `ShapeMarker::ShapeMarker` |
| `MarkerEnd` | keyword | – | – | optional for the last marker (omitted in `clonelab.shp`, `controlstation.shp`, `portal.shp`) | ends the marker block | same |

Derived at load [C] `RegisterPositions`, `GenerateNormals`: a synthetic `SceneRoot` (identity, no geometry) holds the root fragments; bounding sphere per fragment = AABB centre, radius = max distance (`iv_sqrt`) (a cylinder alternative is computed but never enabled); face normal `n = normalise((b−a) × (c−b))`, emitted at all three corners (flat shading). `Matrix34` stores rows `r, u, f, pos` (row-vector convention); fragment world matrix = `frag.T × parent.T × … × root.T × object`; marker world matrix = `marker.T × parents[Depth−1].T × … × parents[0].T × object` [C] `ShapeMarker::GetWorldMatrix`, `ShapeFragment::Render`.

**Marker names consumed by code** [C] grep of `LookupMarker` and marker literals:

| Mechanism | Names | Consumer | Purpose |
|---|---|---|---|
| prefix scan (`strstr`, case-sensitive) | `MarkerLight*` | `Building::SetShapeLights` | building signal lights |
| prefix scan | `MarkerPort*` | `Building::SetShapePorts` | Darwinian operator ports (pushed 5 wu out, dropped to terrain) |
| exact | `MarkerStatus`, `MarkerStatus0%d` | `code/worldobject/building.cpp`, `gunturret.cpp`, `generator.cpp`, `spiritreceiver.cpp` | status lights |
| exact | `MarkerEnd` | `code/worldobject/entity_leg.cpp:EntityLeg::EntityLeg` | leg-segment tip (also a marker *name* in `spider_leg_*.shp`, `tripod_leg_part.shp`) |
| exact / numbered | `MarkerBarrel`, `MarkerKills`, `MarkerBarrelEnd0%d`, `MarkerShells0%d` | `code/worldobject/gunturret.cpp` | barrel mount, kill counter, muzzles, shell ejection |
| exact | `MarkerTurret`, `MarkerEntrance`, `MarkerBarrelEnd` | `code/worldobject/tank.cpp` | turret, entrance, muzzle |
| exact | `MarkerEntrance`, `MarkerFlag` | `code/worldobject/armour.cpp` | load point, team flag |
| exact | `MarkerFlag` | `code/worldobject/officer.cpp` | officer flag |
| exact | `MarkerCarry` | `code/worldobject/armyant.cpp` | carried object |
| exact | `MarkerLaser`, `MarkerBrass`, `MarkerEye1`, `MarkerEye2` | `code/worldobject/insertion_squad.cpp` | muzzle, casing, eye glints |
| exact | `MarkerSpiritLink`, `MarkerDoor` | `code/worldobject/spawnpoint.cpp` | link origin, door |
| exact | `MarkerSpiritLink`, `MarkerHead`, `MarkerStatus0%d` | `code/worldobject/spiritreceiver.cpp` | link origin, head, lights |
| exact | `MarkerDestination1`, `MarkerDestination2`, `MarkerSurface` | `code/worldobject/trunkport.cpp` | portal destinations, surface |
| exact | `MarkerFuel`, `MarkerTip`, `MarkerEntrance`, `MarkerBooster`, `MarkerWindow0%d` | `code/worldobject/rocket.cpp` (`MarkerBooster` also `code/worldobject/weapons.cpp`) | rocket fuel inlet, tip, door, exhaust, windows |
| exact | `MarkerEngine%d` | `code/worldobject/harvester.cpp`, `code/worldobject/spaceship.cpp` | engine exhausts |
| exact | `MarkerEggLay`, `MarkerLegX` | `code/worldobject/spider.cpp` (`MarkerLegX` also `tripod.cpp`) | egg point, leg roots |
| exact | `MarkerEggs`, `MarkerTail0%d` | `code/worldobject/sporegenerator.cpp` | egg launch, tail |
| exact | `MarkerTail` | `code/worldobject/souldestroyer.cpp` | tail |
| exact | `MarkerPrimitive0%d`, `MarkerSpike0%d`, `MarkerRay0%d` | `code/worldobject/constructionyard.cpp` | build spots, spikes, rays |
| exact | `MarkerLightPos`, `MarkerDishPos`, `MarkerReprogrammer%d`, `MarkerConsole%d` | `code/worldobject/controltower.cpp` | |
| exact | `MarkerDishDirection` | `code/worldobject/controlstation.cpp` | |
| exact | `MarkerFocus` | `code/worldobject/radardish.cpp`, `code/worldobject/feedingtube.cpp` | beam focus |
| exact | `MarkerPowerLocation`, `MarkerCounter`, `MarkerGlow0%d`, `MarkerStatus0%d` | `code/worldobject/generator.cpp` | |
| exact | `MarkerSpirits`, `MarkerExit`, `MarkerDock`, `MarkerSpiritIncoming0`/`1`/`2` | `code/worldobject/incubator.cpp` | |
| exact | `MarkerTrack1`/`2`, `MarkerContents1`/`2`/`3`, `MarkerWheel01`/`02`/`03`, `MarkerCounter` | `code/worldobject/mine.cpp` | |
| exact | `MarkerBlueprint` (`blueprintstore.cpp`), `MarkerSignal` (`bridge.cpp`), `MarkerSpawnPoint` (`cave.cpp`), `MarkerSpiritStore` (`factory.cpp`), `MarkerFence01`/`02` (`laserfence.cpp`), `MarkerResearch%02d` (`library.cpp`), `MarkerGrab1`/`2` (`researchitem.cpp`), `MarkerConnectionLocation` (`switch.cpp`), `MarkerTeleportEntrance` (`teleport.cpp`), `MarkerLaunchPoint`, `MarkerTriffidStem` (`triffid.cpp`) | `code/worldobject/` files as listed | many [B:LEGACY] (blocked Darwinia buildings) |

Marker names present in data but not referenced by any literal: `MarkerIsland*`, `MarkerTrunk<N>to<M>` (`globalworld_*.shp`), `MarkerFloat*`, `MarkerControl*`, `MarkerGun*`, `MarkerRocket*`, `MarkerParticleA`/`B`, `MarkerPlinth`, `MarkerFlowerCentre`, `MarkerBreach` [C] grep; whether any is built by `sprintf` elsewhere is **29-U4**.

**Verification.** All shape parser functions above re-read for this document. `_work/scripts/X1b_shp_survey.py` (re-run) over 139 files: keywords `fragment` 441, `parentname` 866, `up`/`front`/`pos` 866 each, `positions`/`normals`/`colours`/`vertices` 441 each, `triangles` 290, `strips` 151, `strip` 163, `verts` 163, `material` 140 (35 files), `marker` 425, `depth` 425, `markerend` 422, `right` 15; fragments = geometry blocks = 441; 0 files with non-zero normals; 0 colour values > 255; anomaly: `carrycube.shp` duplicate fragment names. Per-shape table: `_work/drafts/P12_shapes.csv`, `_work/drafts/X2a_shapes.csv`.

**Minimal valid example** (written for this document: a strip-encoded triangle pair under a parent fragment, plus a marker on the child)

```text
Fragment: Base
    ParentName: SceneRoot
    up: 0 1 0
    front: 0 0 1
    pos: 0 0 0
    Positions: 3
        0: 0 0 0
        1: 4 0 0
        2: 0 0 4
    Normals: 0
    Colours: 1
        0: 90 160 220
    Vertices: 3
        0: 0 0
        1: 1 0
        2: 2 0
    Triangles: 1
        0,2,1

Fragment: Flag
    ParentName: Base
    up: 0 1 0
    front: 0 0 1
    pos: 0 3 0
    Positions: 4
        0: 0 0 0
        1: 0 2 0
        2: 2 0 0
        3: 2 2 0
    Normals: 0
    Colours: 1
        0: 240 200 40
    Vertices: 4
        0: 0 0
        1: 1 0
        2: 2 0
        3: 3 0
    Strips: 1
    Strip: 0
    Verts: 4
        v0 v1 v2 v3

Marker: MarkerFlag
    ParentName: Flag
    Depth: 3
    up: 0 1 0
    front: 1 0 0
    pos: 1 1 0
    MarkerEnd
```

(`Depth 3`: `SceneRoot` → `Base` → `Flag`.)

**Discrepancies** [C]:

| Observation | Effect | Ref |
|---|---|---|
| `carrycube.shp` has two fragments named `Box04`, both parented to `sceneroot` | no assert fires (the duplicate check only runs for non-root parents); both render; `LookupFragment("Box04")` returns the first | survey + `Shape::Load` |
| `carryweights.shp` (two fragments, 100 positions / 50 vertices) and `sporegenerator.shp` (43 / 42) | `m_positionsInWS` is sized by **Vertices** but indexed by position id in accurate hit tests → heap overrun (`ShapeFragment::RayHit`, `SphereHit`) | survey |
| 3 files lack the final `MarkerEnd` | harmless (EOF ends the marker) | survey |
| `right:` lines (15 in `fieldgun_shell.shp`, `garbagecollector.shp`, `gunturret.shp`), `Material` lines, `# N triangles…` comments | ignored legacy exporter output | survey + parser |
| in-engine writer `Shape::WriteToFile` | skips `SceneRoot`, writes colours without undoing × 0.85 (round trip darkens), and uses `"%5.2 "`, `"%7.3 "`, `"%6.3 "` with no conversion letter — cannot have produced the shipped files; callers are tool-only [I] | `ShapeFragment::WriteToFile`, `ShapeMarker::WriteToFile` |

<a id="fmt-max"></a>
### 8.2 `.max` sources and the shape tool chain [authoring]

**Purpose.** 3ds Max scene sources of the shapes and the exporter that produced `.shp` [C] `tools/ShapeExporter/ShapeExporter.cpp`, `tools/ShapeExporter/ShapeExporter.h`, `tools/ShapeExporter/DllEntry.cpp`; maths test harness `tools/3DGeometry/main.cpp`. Tool details: [31 §11](31_platform_build_and_tools.md).

**Files.** `data/shapes/source/*.max` — 133 files, 12 246 016 bytes; Microsoft OLE2 Compound File Binary containers (magic `D0 CF 11 E0 A1 B1 1A E1`) = Autodesk 3ds Max scenes [C] magic bytes. Never read by the game; excluded by the `shapes/source` line of `tools/multiwinia_exclusions.txt` and by the `main.dat` extension filter [C] `tools/build_multiwinia_data.bat`. 110 have a same-named `.shp` (case-insensitive); 23 `.max` have no `.shp` of that name (e.g. `AnthillNew`, `BattleCannon`, `GlobalWorld`, `LaserTroop`, `SpiderLegLower` — several correspond to renamed shapes such as `spider_leg_lower.shp`, `laser_troop.shp`, `flying_egg.shp`) and 29 `.shp` have no `.max` (e.g. `armyant2`/`3`, `carry*`, `clonelab`, `portal`, `pulsebomb`, `wallbuster`, `trunkport_mp`) [C] name comparison.

**Encoding / layout.** Proprietary Autodesk binary inside CFB; no grammar is given (not needed by the game).

**Exporter output contract** (the only producer of `.shp`) [C] `ShapeExporter::DoExport`, `ShapeExporter::BuildShapeFragment`:

| Aspect | Rule |
|---|---|
| Registration | extension `shp`, description "Futurewar Shape Format v1.0", version 100, class id `0x3288038e, 0x40eff553` |
| Hierarchy | root fragment `"scene root"` (spaces stripped → `sceneroot`); walks the Max node tree from the scene root's children with depth 1; nodes whose name starts with `marker` (case-insensitive) become markers with `Depth` = recursion depth; other geometry nodes become fragments (non-geometry, target and zero-face nodes skipped; children of a marker not visited) |
| Axes | Max Z-up → game Y-up: every point `(x, y, z)` → `(x, z, −y)`; local transform = `nodeTM × inverse(parentTM)`; **fragment** `front` stored negated `(−f.x, −f.z, f.y)`, **marker** `front` un-negated `(f.x, f.z, −f.y)` [C]; i.e. a marker's front points along the helper's local +Y, a fragment's along −Y [I] |
| Geometry | positions de-duplicated within 0.001 (squared 1e-6); colours from vertex colours (× 255, truncated, exact de-dup), else material diffuse × 255, else wire colour; vertices = unique (position, colour) pairs; faces as `Triangles`; `Normals: 0`; `nvtristrip.h` included but unused → the 39 strip-format shipped shapes came from an older exporter [I] |
| Build state | calls a 3-argument `ShapeFragment(name, parent, false)` constructor that no longer exists in `code/lib/shape.h` (2 arguments) and relies on the broken `WriteToFile` formats → does not build against this tree [C] |

`tools/3DGeometry` is an Allegro/AllegroGL viewer for ray–sphere / sphere–triangle tests from `code/lib/math_utils.cpp`; not a format or converter [B:DEAD] [C] `tools/3DGeometry/main.cpp`.

**Verification.** Magic bytes of all 133 files and the exporter source read by X1b [C]; name comparison scripted (X1b).

**Minimal valid example.** Not applicable (proprietary binary); the exporter's output contract is the [fmt-shp](#fmt-shp) grammar.

**Discrepancies.** Exporter description "Futurewar Shape Format v1.0" vs the diverged game parser [C]; recommended replacement: glTF 2.0 with node hierarchy, per-vertex colour, empties for markers [I].

<a id="fmt-bmp"></a>
### 8.3 BMP bitmaps and textures

**Purpose.** Every image the game loads (textures, sprites, icons, help pictures, thumbnails, font pages, terrain ramps, animation sprites) is a Windows BMP decoded by `BitmapRGBA` [C] `code/lib/bitmap.cpp:BitmapRGBA::LoadBmp`, `ReadBMPFileHeader`, `ReadWinBMPInfoHeader`, `ReadOS2BMPInfoHeader`, `ReadBMPPalette`, `Read4BitLine`, `Read8BitLine`, `Read24BitLine`, `Read32BitLine`; texture pipeline `code/lib/resource.cpp:Resource::GetTextureWithFlags`, `GetTexture`, `GetTextureWithAlpha`, `ProcessBmp`, `GetBitmap`, `DoesTextureExist`, `code/lib/bitmap.cpp:ConvertToTexture`. Upload algorithm and per-asset catalogue: [24](24_rendering.md), [25](25_textures_and_assets.md).

**Files** [C] `_work/scripts/X1b_bmp_survey.py` (re-run) — 506 `.bmp` under `data/`:

| Directory | Count | Bit depth (all 40-byte info header) | Notes |
|---|---|---|---|
| `data/animations/` | 25 | 24 ×6, 32 ×19 (all 19 with real alpha) | 11 non-POT; 2 use the pink key |
| `data/icons/` | 121 | 24 ×64, 32 ×21, 8 ×25, 4 ×11 | 27 non-POT |
| `data/mwhelp/` | 51 | 24 ×48, 32 ×3 | 49 non-POT |
| `data/sepulveda/` | 5 | 8 | 256×256 |
| `data/sprites/` | 17 | 24 ×16 (9 pink-keyed), 32 ×1 | |
| `data/terrain/` | 36 | 24 ×32, 8 ×4 | 9 non-POT (`waves_*` 400×10) |
| `data/textures/` | 54 | 24 ×19, 32 ×5, 8 ×26, 4 ×3, **8-bit RLE8 ×1** (`textures/shaman_background.bmp`) | 21 non-POT; 5 pink-keyed |
| `data/textures/plusmenu/` | 2 | 24 | non-POT |
| `data/textures/unicode/` | 138 | 24 ×90, 4 ×44, 8 ×4 | font pages; 3 non-POT |
| `data/thumbnails/` | 57 | 8 ×53, 24 ×4 | 400×300 |

None is OS/2, top-down or `BI_BITFIELDS`; the only compressed file is the RLE8 texture; no row-padding or palette-offset hazard [C] survey.

**Byte layout accepted by the loader** (little-endian):

| Offset | Size | Field | Loader behaviour | Parser ref |
|---|---|---|---|---|
| 0 | 2 | `bfType` | should be `0x4D42` "BM" (debug assert only) | `ReadBMPFileHeader` |
| 2 | 4 | `bfSize` | read, ignored | same |
| 6 | 2 + 2 | reserved | ignored | same |
| 10 | 4 | `bfOffBits` | **not used for seeking**; only derives the palette size | `LoadBmp` |
| 14 | 4 | `biSize` | 40 → Windows header; 12 → OS/2 header; anything else (V4 108, V5 124) → debug assert, release continues with an uninitialised header | `LoadBmp` |
| 18 (Win) | 4,4,2,2,4,4,4,4,4,4 | width, height, planes, bitCount, compression, sizeImage, xppm, yppm, clrUsed, clrImportant | only width, height (as **unsigned** — negative/top-down unsupported), bitCount, compression used | `ReadWinBMPInfoHeader` |
| 18 (OS/2) | 2,2,2,2 | width, height, planes, bitCount | compression = 0 | `ReadOS2BMPInfoHeader` |
| 54 (Win) / 26 (OS/2) | `ncol` × 4 / × 3 | palette B, G, R (, reserved) | `ncol = (bfOffBits − 54)/4` or `(bfOffBits − 26)/3`; no clamp to 256 (overflow if larger); palette alpha 255 | `ReadBMPPalette` |
| after the palette | – | pixel rows, **bottom-up**: file row 0 → `m_lines[0]` (bottom row; matches the GL texture origin) | read sequentially; a gap between palette and `bfOffBits` would be decoded as pixels | `LoadBmp` |

| `biBitCount` | Per pixel | Row padding consumed? | Notes |
|---|---|---|---|
| 4 | nibble index; **high nibble = left pixel** (x), low nibble = x + 1 | **no** | rows must be a multiple of 4 bytes (width multiple of 8) or the image shears; odd widths write one pixel past the row |
| 8 | byte index | **no** | width must be a multiple of 4 |
| 24 | B, G, R; alpha := 255 | yes, `(4 − 3w) & 3` bytes | |
| 32 | B, G, R, A (alpha used as-is) | n/a | the only way to ship a real alpha channel |
| other (1, 16) | – | – | debug assert; release leaves pixels uninitialised |

`biCompression` must be `BI_RGB` (0) — debug assert only; **RLE4/RLE8 are not decoded** [C]. `BitmapRGBA::Initialise(reader, type)` debug-asserts `type == "bmp"` and always calls `LoadBmp`, so `.jpg`, `.tex`, `.dds` (named in Xbox/legacy archive filters) cannot be decoded on PC [C].

**Texture-name rules** [C] `Resource::GetTextureWithFlags`, `ProcessBmp`, `GetBitmap`:

| Rule | Behaviour |
|---|---|
| Name | path relative to `data/` **including the extension** (`sprites/darwinian.bmp`); most code builds names with `.bmp` or `"." TEXTURE_EXTENSION` (`#define TEXTURE_EXTENSION "bmp"`, `code/lib/universal_include.h`; 33 uses) — [I] the macro let an Xbox build switch to `.tex` |
| Lookup order | (a) texture cache keyed by the **exact name string** (case-sensitive `hash_map<std::string>`); (b) registered bitmap store (run-time images such as `water_lightmap`, case-insensitive); (c) file: name lower-cased → `GetBinaryReader` → loose then archive. Missing → `AppReleaseAssert("Failed to load texture %s")` (fatal); callers that may miss guard with `DoesTextureExist` |
| Flags vs cache | flags are not part of the cache key: the first request for a name decides mask/alpha/mip-map processing for all later requests |
| Default flags | `GetTexture(name, mipmapping = true, masked = true, compressed = false)` → `WithMipmaps | WithMask`; **every BMP requested through the default overload is colour-keyed** |
| `WithMask` | `ConvertPinkToTransparent`: pixels exactly RGBA (255, 0, 255, **255**) become (128, 128, 128, 0); 24-bit and paletted files have alpha 255 so the key works; a 32-bit magenta pixel with alpha ≠ 255 is not keyed |
| `WithAlpha` | `GetTextureWithAlpha` (9 call sites) → `ConvertColourToAlpha`: alpha := **green** channel, RGB := white |
| Upload | RGBA8; GL requires power-of-two only without `GL_ARB_texture_non_power_of_two` or with pref `ManuallyScaleTextures` ≠ 0 (square only without `GL_ARB_texture_rectangle`); D3D always power-of-two; resizing rounds **up** (bilinear); wrap CLAMP, MAG LINEAR, MIN LINEAR_MIPMAP_LINEAR (also when not mip-mapped) — [24](24_rendering.md) |
| `GetBitmap(name)` | CPU copy cached under `name`; a later `GetTexture` on the same name finds it in the bitmap store and modifies it in place (pink → transparent) |

**Verification.** `LoadBmp` and the four line readers re-read for this document (nibble order, padding expression, no `Seek`). Survey: 506 files, results above.

**Minimal valid example** (written for this document): a 2×1 24-bit image (62 bytes): file header `42 4D 3E 00 00 00 00 00 00 00 36 00 00 00`; info header `28 00 00 00 02 00 00 00 01 00 00 00 01 00 18 00 00 00 00 00 08 00 00 00 13 0B 00 00 13 0B 00 00 00 00 00 00 00 00 00 00`; one row `00 00 FF  FF 00 FF  00 00` (red, then the pink key, 2 padding bytes). Requested via `GetTexture`, the right-hand pixel becomes transparent grey (128,128,128,0).

**Discrepancies.** `textures/shaman_background.bmp` (RLE8, 512×512, used by `code/shaman_interface.cpp:ShamanInterface::RenderBackground`) would be read as raw indices running past the data (`ReadU8` returns 0xFF at EOF) → corrupt image [C]; whether the Shaman interface is reachable in Multiwinia is settled in [02](02_units_and_entities.md) (Shaman is cut). The `code/lib/bitmap.h` comment on `ConvertColourToAlpha` says "luminance"; code uses green only [C]. 4-/8-bit row padding not consumed: latent (all shipped widths are multiples of 8) [C].

<a id="fmt-png-output"></a>
### 8.4 PNG screenshot output [runtime]

**Purpose.** Screenshots ("posters") written when control `ControlCreateScreenShot` fires and no window is open [C] `code/renderer.cpp:Renderer::Render`, `code/lib/poster_maker.cpp:PosterMaker::AddFrame`, `PosterMaker::SavePoster`, `code/lib/bitmap.cpp:BitmapRGBA::SavePng`, `WritePng`. There is no PNG **reader** in the game.

**Files.** `<P>screenshots/screenshot<N>.png` with the first unused N starting at 1 (`App::GetScreenshotDirectory`; Desktop under `TARGET_OS_VISTA`, `~/Pictures/Multiwinia/` on Mac) [C] `code/app.cpp:App::GetScreenshotDirectory`. Under `TARGET_OS_VISTA` [B:DEAD] `App::SaveThumbnailScreenshot` writes a 256×192 PNG into the rich saved-game header.

**Layout** (standard PNG via libpng) [C] `WritePng`:

| Field | Value |
|---|---|
| IHDR | width × height of the poster bitmap, bit depth 8, colour type RGB (`PNG_COLOR_TYPE_RGB`; RGBA only if `_saveAlpha`, which `SavePng` never passes — the argument is ignored) |
| Rows | flipped from the bottom-up `m_lines` to top-down |
| Compression | `Z_BEST_SPEED`, mem level 8, `Z_DEFAULT_STRATEGY`, window bits 15, method 8, buffer 8192 |
| Poster size | pref `RenderPosterResolution` (default 1): N × N tiles of the screen size, each rendered with `RenderFrame(false)` and read back by `AddFrame` |

**Verification.** `PosterMaker::SavePoster`, `Renderer::Render` trigger block, `SavePng`/`WritePng` re-read for this document [C].

**Minimal valid example.** Any 8-bit RGB PNG, e.g. a 1×1 image; the game writes but never reads them.

**Discrepancies.** `SavePng(_filename, _saveAlpha)` ignores `_saveAlpha` [C]. `WriteBmp` (24-bit Windows BMP, 2835 px/m) exists for tools [C].

<a id="fmt-colour-ramp"></a>
### 8.5 Terrain, water and wave colour ramps

**Purpose.** Three per-level BMP look-ups colour the landscape, the flat sea texture and the animated wave mesh. A level names them in its `Landscape` block (`landColourFile`, `wavesColourFile`, `waterColourFile`, resolved as `terrain/<name>`; defaults `landscape_default.bmp`, `waves_default.bmp`, `water_default.bmp`) [C] `code/level_file.cpp:LevelFile::ParseLandscapeData`. With the Christmas mod active the three are forced to `landscape_icecaps.bmp`, `waves_earth.bmp`, `water_icecaps.bmp` [C] `code/landscape_renderer.cpp:LandscapeRenderer::LandscapeRenderer`, `code/water.cpp:Water::Water`, `Water::Render`.

**Files.** `data/terrain/landscape_*.bmp` (23, 64×64, 24-bit), `data/terrain/water_*.bmp` (4, 128×128, 8-bit paletted), `data/terrain/waves_*.bmp` (9, 400×10, 24-bit); no ramp uses the pink key; look-ups scale by the bitmap's actual size, so other sizes are legal [C] survey.

**Encoding.** [fmt-bmp](#fmt-bmp); the semantics below are a convention over the pixel grid.

| Ramp | Field | Mapping | Parser ref |
|---|---|---|---|
| landscape (2-D, CPU via `GetBitmap`) | column x | `x = int(u · width)` clamped, `u = (1 − gradient)^0.4`, `gradient = normal.y` (1 flat … 0 vertical) → **left = flat, right = vertical** | `code/landscape_renderer.cpp:LandscapeRenderer::GetLandscapeColour`, `BuildColourArray` |
| landscape | row y (row 0 = first stored row = **bottom row of the picture as viewed**) | `y = int(v · height)` clamped, `v = 1 − h/highest + jitter`, `highest` = max height of this map, `jitter = sfrand(0.45/(max(h,0)+2))` (±0.1125 at sea level, shrinking with height; RNG reseeded per triangle from x, z) → **top of the picture = sea level, bottom = highest peak** | same |
| water (tiled texture) | whole image | `GetTexture("terrain/<file>", mipmaps, masked)`, unit 0, `GL_REPEAT`, LINEAR / LINEAR_MIPMAP_LINEAR, `GL_MODULATE`; mapped on the sea plane at y = −9.0 over `[−sizeX/2, 1.5·sizeX] × [−sizeZ/2, 1.5·sizeZ]` with coordinates `t/30 … t/30 + 30` (`t` = `g_gameTimer.GetAccurateTime()` s) → repeats 30× and scrolls 1/30 tile per second; unit 1 multiplies by the generated shore mask `water_lightmap` | `code/water.cpp:Water::Render`, `Water::RenderFlatWaterTiles` |
| waves (1-D table) | row y = 1 only | `table[x] = pixel(x, 1)`, alpha 255 (0 in negative mode), `numColours = width`; per vertex `brightness = (h1 + h2 + h3) · 4.0 · (1 − depth) + shoreNoise`, `shoreNoise = ((1 − depth) + sfrand(1 − depth) · 0.25) · 250`, strip pairs mixed 0.7/0.3, `index = Round(brightness)` clamped → **x = 0 calm open water, higher x brighter crest/shore foam** [arithmetic C; interpretation I] | `code/water.cpp:Water::Water`, `Water::UpdateDynamicWater`, `code/water.h:Water::GetColour` |

Full algorithms: [23](23_terrain_and_landscape.md).

**Verification.** From X1b/P11 readings of the renderer and water code [C]; survey of all 36 ramp files (sizes, depths) [C] `X1b_bmp_survey.py`.

**Minimal valid examples**

* `landscape_sample.bmp`: 64×64 24-bit; columns 0–40 grass→earth, 41–63 rock; top rows sand, bottom rows snow.
* `waves_sample.bmp`: 300×2 24-bit; the top row as viewed (row 1) a gradient from deep blue (x = 0) to white (x = 299); bottom row unused.
* `water_sample.bmp`: any seamlessly tiling 64×64 image.

**Discrepancies.** 9 landscape ramps are referenced by no level (`beach`, `containment`, `field`, `icecaps_peaked`, `marble2`, `mine2`, `ridges`, `volcanic`, `volcanicplain`) [C] P11. The D3D path of `Water::Water` writes `bmp.GetPixel(x,1)` on a `const BitmapRGBA *` → compile error under `USE_DIRECT3D` [C] ([31 §7.9](31_platform_build_and_tools.md)).

<a id="fmt-anim"></a>
### 8.6 `.anim` — animated help panel

**Purpose.** A short looping 2-D cartoon shown during a Multiwinia game's grace period explaining the mode [C] `code/multiwiniahelp.cpp:MultiwiniaHelp::InitialiseAnimatedPanel`, `RenderAnimatedPanel`; parser `code/animatedpanel.cpp:AnimatedPanel::Read`, `AnimatedPanelObject::Read`, `AnimatedPanelKeyframe::Read`; interpolation `AnimatedPanel::CalculateObjectProperties`, `CalculateRampUpAndDown`; renderer `code/animatedpanel_renderer.cpp:AnimatedPanelRenderer::RenderObjects`. Authored with the external `animator.exe` (repository root; its data archive `animator.dat` holds `data/animations/*.bmp`, `data/fonts/verdana-bold.bmp`, `data/dialogs/animator-*.txt`) [C] strings, RAR listing via `_work/scripts/P14_rar_list.py`. `AnimatedPanel::Write` exists with no caller. Playback model: [26](26_animation_and_vfx.md). [B:FULL]

**Files** [C] `InitialiseAnimatedPanel`, `X1b_anim_survey.py` (re-run):

| Game type | File | WIDTH×HEIGHT | START–STOP (s) | Objects | Keyframes |
|---|---|---|---|---|---|
| Domination (`GameTypeSkirmish`) | `data/animations/domination.anim` | 800×250 | 0.00–15.01 | 28 | 169 |
| KingOfTheHill | `data/animations/koth.anim` | 800×250 | 0.00–15.66 | 17 | 108 |
| CaptureTheStatue | `data/animations/cts.anim` | 924×250 | 0.00–20.01 | 23 | 111 |
| Assault | `data/animations/assault_defend.anim` | 900×250 | 0.00–12.02 | 14 | 135 |
| RocketRiot | `data/animations/rocket.anim` | 1000×250 | 0.00–30.01 | 21 | 173 |
| Blitzkrieg (`GameTypeBlitzkreig`) | `data/animations/blitz.anim` | 900×250 | 0.00–24.01 | 28 | 180 |
| TankBattle | none → no panel | | | | |

All 25 bitmaps in `data/animations/` are referenced by at least one `.anim`; no `.anim` references a missing image [C].

**Encoding.** ASCII, CRLF.

```ebnf
anim_file   = { header_line | object | ignored_line } ;            (* blank lines skipped *)
header_line = ( 'WIDTH' INT | 'HEIGHT' INT | 'START' FLOAT | 'STOP' FLOAT ) EOL ;
object      = 'BEGIN' 'Object' EOL , { keyframe | ignored_line } , 'END' EOL ;
keyframe    = 'BEGIN' 'Keyframe' EOL , { kf_field | ignored_line } , 'END' EOL ;
kf_field    = ( 'TIMEINDEX' FLOAT | 'IMAGENAME' NAME | 'XPOS' FLOAT | 'YPOS' FLOAT | 'COLOUR' INT
              | 'SCALE' FLOAT | 'ANGLE' FLOAT | 'FLIPHORIZ' INT | 'ALPHA' INT ) EOL ;
```

Keywords case-insensitive; unknown keywords ignored at every level; header lines may appear anywhere outside objects (last wins); an `END` at top level is ignored. Hazards [C]: at top level the second token is read without a check, so a one-token `WIDTH`/`HEIGHT`/`START`/`STOP`/`BEGIN` line crashes (`atoi`/`atof`/`stricmp(NULL)`); inside objects/keyframes the value is `NULL` when absent and `atof(NULL)`/`strcpy(…, NULL)` crash; `IMAGENAME` is copied into a 256-byte buffer unchecked.

| Field | Type | Units | Default | Range (shipped) | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `WIDTH`, `HEIGHT` | INT | panel units | 600, 300 | 800–1000, 250 | logical canvas; on screen `scale = ScreenH/1300`, `screenW = WIDTH·scale`, `screenH = HEIGHT·scale`, centred horizontally, `screenY = ScreenH − HEIGHT·1.15·scale`; border 5 px, title band 25 px, caption band 15 px | `AnimatedPanel::Read`, `MultiwiniaHelp::InitialiseAnimatedPanel` |
| `START`, `STOP` | FLOAT | s | 0, 30 | 0 / 12.02–30.01 | loop window: the clock starts at **0** (not START), advances by wall-clock seconds and is reset to START when it exceeds STOP | same; `RenderObjects` |
| `TIMEINDEX` | FLOAT | s | 0.0 | 0–30.66 | key time; keys after STOP are never reached | `AnimatedPanelKeyframe::Read` |
| `IMAGENAME` | NAME | file | `darwinian.bmp` | 25 names | sprite `animations/<name>`; `GetBitmap` (size) + `GetTexture` (default flags → mip-mapped, pink-keyed) | same |
| `XPOS`, `YPOS` | FLOAT | panel units | 100, 100 | x 28–940, y −75–233 | sprite **centre**; screen = `screenX + XPOS·s'`, `screenY + YPOS·s'`, `s' = screenW/WIDTH`; y grows downward | same |
| `SCALE` | FLOAT | × source pixels | 0.3 | 0.0–2.5 | size = bitmap size × SCALE × s' | same |
| `ANGLE` | FLOAT | degrees | 0 | −1000…1500 | rotation about the centre (`RotateAroundZ(2π·ANGLE/360)`); values beyond ±360 make spins when interpolated | same |
| `COLOUR` | INT enum | – | 0 | 0–3 | 0 `ColourNone` white; 1 `ColourFriendly` local team colour; 2 `ColourEnemy` colour of the first team (index order) that is in use and not a friend of the local team (white if none); 3 `ColourInvisible` alpha 0 | same; `RenderObjects` |
| `FLIPHORIZ` | INT bool | – | 0 | 0/1 | mirror horizontally | same |
| `ALPHA` | INT | 0–255 | 255 | 0–255 | vertex alpha (ignored when COLOUR = 3) | same |

Playback [C] `CalculateObjectProperties`, `CalculateRampUpAndDown`: per object (file order = draw order) at time t: `left` = key with the greatest time ≤ t (first on ties), `right` = key with the smallest time ≥ t; neither → not drawn; only `right` → **not drawn**; only `left` → hold; same key or `|right.t − left.t| < 0.01` → `left`; else `lin = (t − left.t)/(right.t − left.t)`, `ramp = 2f²` for f < 0.5 else `1 − 2(1 − f)²`; XPOS, YPOS, SCALE use `ramp`; ANGLE and ALPHA use `lin` (ALPHA truncated); IMAGENAME, COLOUR, FLIPHORIZ step from `left`. Quads are drawn with `SRC_ALPHA, ONE_MINUS_SRC_ALPHA`, clipped to the panel.

**Verification.** `AnimatedPanel::Read`, `AnimatedPanelObject::Read`, `AnimatedPanelKeyframe::Read` and the constructors' defaults re-read for this document. Survey of the 6 files: e.g. `assault_defend.anim` 14 objects, 135 keys, COLOUR {0: 94, 1: 17, 2: 18, 3: 6}, FLIPHORIZ {0: 126, 1: 9}; `cts.anim` keys up to 30.00 with STOP 20.01 [C].

**Minimal valid example** (one sprite fading in while sliding)

```text
WIDTH  700
HEIGHT 250
START  0.00
STOP   3.00

BEGIN Object
    BEGIN Keyframe
        TIMEINDEX  0.00
        IMAGENAME  flag.bmp
        XPOS       150.00
        YPOS       120.00
        COLOUR     2
        SCALE      0.60
        ALPHA      0
    END
    BEGIN Keyframe
        TIMEINDEX  3.00
        IMAGENAME  flag.bmp
        XPOS       550.00
        YPOS       120.00
        COLOUR     2
        SCALE      0.60
        ALPHA      255
    END
END
```

**Discrepancies.** Keys beyond STOP are dead data (`cts.anim`, `domination.anim`, `koth.anim` have keys at 30.00) [C]. Several `data/animations/` sprites share names with files elsewhere but are different images (MD5 differs): `flag.bmp`, `flag_down.bmp`, `flag_locked.bmp`, `flag_up.bmp` vs `data/icons/`; `darwinian.bmp` vs `data/sprites/` and `data/textures/`; `cloudyglow.bmp`, `muzzleflash.bmp`, `laser.bmp` vs `data/textures/`; only `darwinian_ouch4.bmp` is byte-identical to `data/sprites/darwinian_ouch4.bmp` [C] md5.

<a id="fmt-icons"></a>
### 8.7 `data/icons/` naming convention

**Purpose / parser.** Names built by code for icon textures [C] `code/taskmanager_interface_icons.cpp:TaskManagerInterfaceIcons::TaskManagerInterfaceIcons`, `code/multiwiniahelp.cpp:MultiwiniaHelp::MultiwiniaHelp`, `code/interface/helpandoptions_windows.cpp:CrateButton::CrateButton`, `code/UI/mapDetailsButton.h`, grep of `"icons/` literals.

**Files / encoding.** `data/icons/*.bmp` (121; 64×64 or 128×128 mostly; 4/8/24/32-bit — [fmt-bmp](#fmt-bmp)).

```ebnf
icon_name = 'icons/' ( 'icon_' NAME | 'gesture_' NAME | 'banner_' ORDER | 'button_' PADBUTTON [ '_shadow' ]
                     | 'mouse_' CURSORSTATE | 'menu_' LEVEL | FREENAME ) '.bmp' ;
```

| Pattern | Built from | Example | Consumer |
|---|---|---|---|
| `icons/icon_<name>.bmp` | `GlobalResearch::GetTypeName(i)`, `Task::GetTaskName(type)` or crate name (`Crate::GetName`) | `icon_squad.bmp`, `icon_nuke.bmp`, `icon_hotfeet.bmp` | task bar, crate help |
| `icons/gesture_<name>.bmp` | research type name | `gesture_officer.bmp` | gesture pictures |
| `icons/banner_<order>.bmp` | officer order / task banner | `banner_goto.bmp` | task banners |
| `icons/button_<pad button>[_shadow].bmp` | controller glyph names `a`, `b`, `x`, `y`, `lb`, `rb`, `lt`, `rt`, `la`, `ra`, `dpad`, `tab` | `button_lb_shadow.bmp` | controller help, input icon lines ([fmt-input-bindings](#fmt-input-bindings)) |
| `icons/mouse_<cursor state>.bmp` | cursor images | `mouse_movehere.bmp` | `MouseCursor` |
| `icons/menu_<level>.bmp`, `icons/menu_%dplayer.bmp` | menu badges | `menu_basic.bmp` | map details |
| `icons/<free name>.bmp` | `MultiwiniaHelp` icon tabs (`"icons/%s.bmp"`), HUD | `compass.bmp`, `nukesymbol.bmp` | help, HUD |

**Verification.** Literal grep + existence check by X1b [C]; catalogue per file: [25](25_textures_and_assets.md).

**Minimal valid example.** `data/icons/icon_shield.bmp` — a 64×64 24-bit image with pink-key background, requested as `icons/icon_shield.bmp`.

**Discrepancies.** Names referenced but absent from `data/icons/`: `menu_campaign.bmp`, `menu_prologue.bmp`, `menu_coop.bmp`, `menu_forcecoop.bmp`, `menu_<n>player.bmp`, `compass_empty.bmp`, `icon_enter.bmp`, `shaman_shadow.bmp` — fatal if requested without a `DoesTextureExist` guard [C]; reachability (e.g. `MarkerSystem::RenderCompass` → `compass_empty.bmp`) and whether `patch.dat` supplies them: [25](25_textures_and_assets.md).

<a id="fmt-mwhelp"></a>
### 8.8 `data/mwhelp/` naming convention

**Purpose / parser.** Help pictures [C] `code/multiwiniahelp.cpp:MultiwiniaHelp::MultiwiniaHelp` (tab icons) and help-render functions, `code/interface/helpandoptions_windows.cpp` (help-page windows), `code/UI/GameTypeImageButton.h`, `code/UI/GameMenuWindow.cpp:GameMenuWindow::PreloadThumbnails`, `code/UI/HelpImageButton.h`.

**Files / encoding.** `data/mwhelp/*.bmp` (51; sizes 150×150, 200×200, 300×200, 400×300, 512×128, 600×389, 426×406, 150×149); all names built with `"." TEXTURE_EXTENSION`.

| Pattern | Meaning |
|---|---|
| `mwhelp/general<page>.bmp` | general help pages (shipped 1–5) |
| `mwhelp/help_<gameType>_<page>.bmp` | per-mode help picture; `gameType` = Multiwinia game-type index 0 Domination … 5 Blitzkrieg; pages 1-based (shipped `help_0_1`, `help_1_1`–`2`, `help_2_1`–`3`, `help_3_1`–`3`, `help_4_1`–`4`, `help_5_1`–`3`); text id `multiwinia_helppage_<gameType+1>_<page>` |
| `mwhelp/mode_<blueprint name>.bmp` | game-mode tile; name from [fmt-multiwinia-txt](#fmt-multiwinia-txt) matched case-insensitively against the lower-case files (`mode_domination.bmp` …), plus fixed `mode_prologue.bmp`, `mode_campaign.bmp` |
| `mwhelp/<topic>.bmp` | in-game help tab icons: `starthere`, `koth`, `spawnpoint`, `spawnpoint_assault`, `crates`, `liftstatues`, `capturestatues`, `wallbuster`, `pulsebomb`, `pulsebomb_attack`, `pulsebomb_defend`, `rocketriot1`, `rocketriot2`, `reinforcements`, `blitzkrieg1`, `blitzkrieg2`, `gear`, `control_camera`, `control_camera_french` (the last chosen when `InputPrefs::IsKeyboardFrenchLocale()`) |

**Verification.** Literal grep + existence check by X1b [C].

**Minimal valid example.** `data/mwhelp/help_0_2.bmp` (400×300 24-bit) would be shown as Domination help page 2 with text id `multiwinia_helppage_1_2`.

**Discrepancies.** Referenced but absent: `mwhelp/helppage_<page>_<image>.bmp`, `basic_select[_xbox]`, `basic_promote[_xbox]`, `darwinian`, `officercontrol`, `darwiniancontrol` (Xbox/legacy help flows) [C]; `data/mwhelp/` contains no `.txt` files although earlier notes expected them [C].

<a id="fmt-thumbnail"></a>
### 8.9 `data/thumbnails/` naming convention

**Purpose / parser.** Lobby map thumbnails [C] `code/UI/MapImageButton.h:MapImageButton::Render`, `code/UI/GameMenuWindow.cpp:GameMenuWindow::PreloadThumbnails`.

```ebnf
thumbnail_name = 'thumbnails/' lower( MAPFILE without '.txt' ) '.bmp' ;     (* fallback 'thumbnails/default.bmp' *)
```

| Field | Rule |
|---|---|
| name | level file name without `.txt`, **lower-cased** (`strlwr`) before look-up; if `DoesTextureExist` fails → `thumbnails/default.bmp` |
| loading | `GetTexture(name, mipmaps = false, masked = false)`, drawn with LINEAR min/mag |
| shipped | 57 files, 400×300 (53 8-bit paletted, 4 24-bit); disk names keep mixed case (`mp_koth_2P_1.bmp`), which works only because file and archive look-ups are case-insensitive |

**Verification.** X1b survey + `X4_thumbnails.csv` (X4 level catalogue) [C].

**Minimal valid example.** `data/thumbnails/mp_example_2p.bmp`, 400×300 8-bit, for the map of §4.1.19.

**Discrepancies.** 5 thumbnails are excluded with their maps from retail (`mp_benchmark_*`, `mp_tankbattle_*`, `mp_cts_3p_3`) [C] `tools/multiwinia_exclusions.txt`.

<a id="fmt-sepulveda"></a>
### 8.10 `data/sepulveda/` — mentor portraits [B:DEAD]

**Purpose / parser.** Five 256×256 8-bit portraits of the Darwinia mentor (index 1–3 neutral, 4 "eyes shut", 5 "mouth open" per code comments), cycled every 0.4 s with random zoom/offset [C] `code/sepulveda.cpp:Sepulveda::Sepulveda`, `Sepulveda::RenderFace`, `code/clouds.cpp:Clouds::RenderFace`.

```ebnf
portrait_name = 'sepulveda/sepulveda' DIGIT '.bmp' ;     (* DIGIT 1..5 *)
```

**Verification.** `sepulveda.cpp` is entirely inside `USE_SEPULVEDA_HELP_TUTORIAL`; `Clouds::RenderFace` is compiled but runs only while `Clouds::m_faceTime > 0`, which nothing sets (initialised −1) → unreachable [C] grep. Files: `data/sepulveda/sepulveda1.bmp` … `sepulveda5.bmp` (8-bit) [C] survey.

**Minimal valid example.** Not applicable (dead convention).

**Discrepancies.** Still packed into `main.dat` (not excluded) although unreachable [C]. IP: character art.

<a id="fmt-psp"></a>
### 8.11 `.psp` — Paint Shop Pro image [authoring]

**Files.** `data/icons/banner_template.psp` (32 020 bytes). **Layout** [C] byte dump: ASCII `"Paint Shop Pro Image File\n\x1a"` + 5 NUL bytes (32-byte signature), major version 3, minor 0, then a `~BK\0` general-image-attributes block (length 38) giving width 64, height 64.

| Field | Offset | Type | Value |
|---|---|---|---|
| signature | 0 | 32 bytes | as above |
| version | 32 | u16 major, u16 minor | 3, 0 |
| attribute block | 36 | `~BK\0` + u32 length | 38 |
| width, height | in the block | i32 | 64, 64 |

**Purpose.** Layered authoring template for the seven 64×64 24-bit `icons/banner_*.bmp` task banners (`banner_absorb`, `banner_deploy`, `banner_follow`, `banner_formation`, `banner_goto`, `banner_none`, `banner_unload`) [I name + matching size].

**Verification.** Not referenced by any code or build script; `*.psp` is not in the `main.dat` filter [C].

**Minimal valid example.** Not applicable (third-party authoring format).

**Discrepancies.** None; authoring-only, IP (art).

<a id="fmt-grd"></a>
### 8.12 `.grd` — Photoshop gradient [authoring]

**Files.** `data/Untitled Gradients.grd` (244 bytes). **Layout** [C] byte dump: `"8BGR"` signature, version 3, gradient count 1, Pascal-string name (length 25) "Foreground to Transparent", then colour/transparency stop records (Adobe Photoshop 3–5 gradient preset).

| Field | Type | Value |
|---|---|---|
| signature | 4 bytes | `8BGR` |
| version | u16 | 3 |
| count | u16 | 1 |
| name | Pascal string | "Foreground to Transparent" |

**Verification.** Not referenced by code or build scripts, not packaged [C] grep + `tools/build_multiwinia_data.bat`. Confirmed unused authoring residue.

**Minimal valid example.** Not applicable.

**Discrepancies.** The blank in the file name would only cosmetically affect the `dir`/`sed` list pipeline [C].

---

## 9 Shaders

<a id="fmt-shader"></a>
### 9.1 HLSL shaders (`data/shaders/*`) [B:D3D]

**Purpose.** Vertex/pixel shaders of the Direct3D 9 path (fixed-function emulation, deform effect, water reflection) [C] `code/lib/shader.cpp:Shader::Shader`, `Shader::Create`, `MemFile`, `MyD3DXInclude::Open`; users `code/lib/FixedPipeline.cpp:FixedPipeline::FixedPipeline`, `code/deform.cpp:DeformEffect::DeformEffect`, `code/water_reflection.cpp:WaterReflectionEffect::WaterReflectionEffect`. The whole of `shader.cpp` is inside `#ifdef USE_DIRECT3D`, so the files are **unused in the baseline OpenGL build**, although packed into `init.dat` (all 14 via `tools/init.list`) and `main.dat` (all except the six `FixedFunc*`) [C]. Per-shader inputs/outputs/uniforms: [24](24_rendering.md).

**Files** (line counts) [C]: `data/shaders/FixedFunc0.vs` 88, `FixedFunc0Maps.ps` 6, `FixedFunc1.vs` 28, `FixedFunc1Maps.ps` 9, `FixedFunc2.vs` 33, `FixedFunc2Maps.ps` 11, `deform.vs` 17, `deformDiamond.ps` 27, `deformFullscreen.ps` 21, `deformSphere.ps` 25, `globals.h` 4, `water-meshwaves.ps` 16, `water-meshwaves.ps3` 21, `water-meshwaves.vs` 23.

**Encoding.** Plain-text HLSL (ASCII); not tokenised by `TextReader` — the whole file is handed to `D3DXCompileShader`.

**File-level grammar**

```ebnf
shader_file  = hlsl_source ;                         (* HLSL; must define the entry point below *)
vertex_file  = hlsl_source containing 'vs_main' ;    (* extension .vs *)
pixel_file   = hlsl_source containing 'ps_main' ;    (* extension .ps or .ps3 *)
include_line = '#include "' NAME '"' ;              (* resolved as resource shaders/NAME *)
```

| Convention | Rule | Parser ref |
|---|---|---|
| compilation | at run time with `D3DXCompileShader` (flags 0, no macros, no precompiled blobs) | `Shader::Create` |
| entry points | `vs_main` in every vertex shader, `ps_main` in every pixel shader | same |
| profiles | the device's best: `D3DXGetVertexShaderProfile` / `D3DXGetPixelShaderProfile` | same |
| extensions | `.vs` vertex, `.ps` pixel, `.ps3` pixel variant requiring a `ps_3_x` profile (chosen when the profile string's 4th character > `'2'`; falls back to `.ps` if creation fails) | `code/water_reflection.cpp:WaterReflectionEffect::WaterReflectionEffect` |
| includes | `#include "x"` resolved by `MyD3DXInclude` as resource `shaders/x` (loose or archive); only `globals.h` is included (by the three `deform*.ps`) and defines `DEFORM_PRECISION 10` | `MyD3DXInclude::Open` |
| pairing | a `Shader` = one `.vs` + one `.ps` path; a compile failure is debug-asserted; `AppReleaseAssert("Shader create error…")` if either object cannot be created | `Shader::Shader`, `Shader::Create` |
| constants | set by name through the D3DX constant tables of both stages (`SetFloat`, `SetUniform`, `SetMatrix`, `SetBool`, `SetInt`) | `Shader` setters |

Pairs created [C]: `FixedFunc0.vs` + `FixedFunc0Maps.ps`, `FixedFunc1.vs` + `FixedFunc1Maps.ps`, `FixedFunc2.vs` + `FixedFunc2Maps.ps`; `deform.vs` + `deformFullscreen.ps` / `deformSphere.ps` / `deformDiamond.ps`; `water-meshwaves.vs` + `water-meshwaves.ps3` or `.ps`.

**Verification.** From X1b/P13 readings of `shader.cpp` and the three users [C]; files counted by `S_31_archive_contents.py` (both archives) [C].

**Minimal valid example** (written for this document; any `vs_2_0`/`ps_2_0` profile)

```hlsl
// tint.vs
float4x4 worldViewProj;
void vs_main(in float4 p : POSITION, in float4 c : COLOR, out float4 oP : POSITION, out float4 oC : COLOR)
{ oP = mul(p, worldViewProj); oC = c; }
// tint.ps
float4 tint;
void ps_main(in float4 c : COLOR, out float4 o : COLOR) { o = c * tint; }
```

**Discrepancies.** The D3D configuration probably does not compile at this commit ([31 §7.9](31_platform_build_and_tools.md)), so these files are effectively dead in every buildable PC configuration [I].

---

## 10 Packaging: archives and pack lists

<a id="fmt-dat-archive"></a>
### 10.1 `.dat` archives (RAR)

**Purpose.** Shipped data is packed into RAR archives that the game decompresses entirely into memory at start-up [C] `code/lib/resource.cpp:Resource::ParseArchive`, `GetUncompressedFile`, `GetTextReader`, `GetBinaryReader`, `ListResources`; `contrib/unrar/unrar.cpp:UncompressedArchive::UncompressedArchive`, `MemMappedFile::MemMappedFile`; `contrib/unrar/extract.cpp:CmdExtract::ExtractArchive` (modified to unpack into memory); `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::HashFunc`, `GetIndex`. Build pipeline and per-SKU contents: [31 §8](31_platform_build_and_tools.md).

**Files.** Runtime archives (none committed): `init.dat`, `language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat`, `sounds.dat`, `sounds2.dat`; `ambx.dat` is installed by the euro installer but never opened. The repository's `animator.dat` (tool data) is an example: archive flags 0x0008 (solid), file headers with unpack version 29, method 0x35 (best), names `data\animations\…` [C] RAR header walk.

**Encoding / container.** Standard **RAR 4.x** (signature `52 61 72 21 1A 07 00` "Rar!"), produced by `tools/Rar.exe`, read by the bundled UnRAR 3.30 beta 1 source (`contrib/unrar/version.h`: 3.30 β1, 14 Nov 2003; unpack methods 1.5, 2.0/2.6, 2.9) [C]. **No encryption**: every `ParseArchive` call passes `password = NULL` and the build never uses `-p`/`-hp` [C] `code/app.cpp:App::App`, `App::Initialise`, `App::LoadSounds`, `tools/build_multiwinia_data.bat`. Entries may be any format of this document; text entries could in principle be "redshirt2"-obfuscated — none are.

**Layout of what the game relies on** (RAR internals are delegated to UnRAR):

| Element | Rule | Ref |
|---|---|---|
| entry name | stored as `data\…` (rar `a -idp` from the repository root) or `data/…`; the loader converts `\` → `/` and lower-cases (`strlwr`) | `MemMappedFile::MemMappedFile`, `Resource::ParseArchive` |
| entry count | ≤ `MAX_FILES` = 16 384 per archive (excess makes `AllocNewFile` return NULL → crash in the modified extractor) | `contrib/unrar/unrar.h`, `contrib/unrar/unrar.cpp` |
| zero-size entries and directories | dropped (and leaked) | `Resource::ParseArchive` (`m_size > 0`) |
| duplicate names across archives | the later archive's entry replaces the earlier one | same |
| lookup key | `data/` + resource name; hash masks the ASCII case bit (`& 0xDF`), compare `stricmp` → **case-insensitive** | `HashTable::HashFunc`, `HashTable::GetIndex` |
| precedence | loose `data/<name>` always beats any archive; among archives `sounds2 > sounds > branding > menusounds > patch > main > language > init` | `Resource::GetTextReader`, `GetBinaryReader` |

Resolved: P14 stated callers must pass lower-case names (case-sensitive lookup); X1a and X1b stated case-insensitive; the hash function and `stricmp` comparison above settle it — **case-insensitive** [C].

**How archives are built** [C] `tools/build_multiwinia_data.bat`, `tools/build_language_dat.bat`:

| Archive | RAR command | Entry names | Content rule |
|---|---|---|---|
| `init.dat` | `rar a -m5 -s -idp init.dat @list` | `data\…` | `tools/init.list`: 14 shaders, `sprites/darwinian.bmp`, `textures/ivlogo.bmp` |
| `language.dat` | `rar a -m5 -s -idp` | `data\…` | `*.bmp *.dds *.tex *.txt` from the selected `tools/*language.list` |
| `main.dat` | `rar a -m5 -s -idp` | `data\…` | all `data/` minus `completeExclusions`, minus any path containing `sounds/`, filtered to `*.tex *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h *.anim` (Xbox adds `*.wmv`) — `.max`, `.psp`, `.grd`, `.ogg` never enter it |
| `menusounds.dat` | `rar a -m2 -s -idp` | `data/…` | `tools/menu_sounds.txt` |
| `sounds.dat` | `rar a -m2 -s -idp` | `data/…` | `tools/sounds.txt` |
| `sounds2.dat` | `rar a -m2 -s -idp` | `data/…` | every `.ogg` not in `completeExclusions` and not in the two sound lists |
| `patch.dat`, `branding.dat` | not built in tree | – | loaded if present |

Switches [I, RAR CLI]: `a` add, `-m5` best / `-m2` fast (Ogg already compressed), `-s` solid, `-idp` no percentage display, `@file` list file.

**Load order** [C] `code/app.cpp:App::App` (`init.dat`, before preferences), `App::Initialise` (`language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat`), `App::LoadSounds` (`sounds.dat`, `sounds2.dat`); a missing archive is skipped silently; an exception during extraction drops that archive. Archives are never decompressed concurrently ([31 §8.7](31_platform_build_and_tools.md)).

`ListResources(dir, filter)` merges the loose directory listing with archive entries whose directory part equals `data/<dir>` (case-insensitive) and whose file part matches the wildcard filter (`*`, `?`; `Resource::WildCmp` is **case-sensitive**, archive names are lower case); entries are inserted with `OrderedInsert` (`contrib/systemiv/lib/filesys/file_system.cpp`), which keeps `strcmp` order and skips exact duplicates only against the last element → a loose file and its archive twin can both appear (always when their case differs, e.g. loose `mp_koth_2P_1.txt` vs archived `mp_koth_2p_1.txt`) [C].

**Verification.** `Resource::ParseArchive`, `GetTextReader`, `GetBinaryReader`, `HashTable::HashFunc`, `GetIndex`, `GetInsertPos` re-read for this document. Archive contents computed by `_work/scripts/S_31_archive_contents.py` (re-implements the batch/Perl logic; retail `main.dat` 565 files, `init.dat` 16, `menusounds.dat` 18, `sounds.dat` 1 297, `sounds2.dat` 6) [C scripted, 31]; `animator.dat` header walked with `_work/scripts/P14_rar_list.py` [C].

**Minimal valid example.** `rar a -m5 -s -idp sample.dat data\stats.txt` from the repository root; mounted with `ParseArchive("sample.dat", NULL)` it serves resource `stats.txt` (key `data/stats.txt`) unless a loose `data/stats.txt` exists.

**Discrepancies.** RAR is a proprietary format (creating archives needs the licensed `rar` tool; the UnRAR licence forbids writing RAR-compatible archivers) — replace with a zip/PAK-style container keeping: ordered mount list, last mount wins, loose-file override, case-insensitive keys, `/`-separated names relative to `data/` [I]. Exact `tools/Rar.exe` version is **29-U5**.

<a id="fmt-pack-list"></a>
### 10.2 Build-time pack lists (exclusion, inclusion and sound lists)

**Purpose.** Text lists that decide which `data/` files go into which archive; read only by the build scripts, never by the game [C] `tools/build_multiwinia_data.bat` (`:makeMenuSounds`, `:makeSounds`, `:makeSounds2`, `:makeMain`), `tools/listfiles.pl`, `tools/copyfiles.pl`, `tools/build_language_dat.bat`. Per-SKU contents of every list: [31 §8.5–§8.6](31_platform_build_and_tools.md).

**Files.** Exclusion lists `tools/multiwinia_exclusions.txt` (retail, 49 lines), `tools/multiwinia_exclusions_beta_test.txt`, `tools/multiwinia_exclusions_hwcompat.txt`, `tools/full_exclusions.txt`, `tools/demo_exclusions.txt`, `tools/demo2_exclusions.txt`, `tools/vista_exclusions.txt`, `tools/pdlc_exclusions.txt`; inclusion lists `tools/multiwinia_inclusions_*.txt` (6); sound lists `tools/sounds.txt`, `tools/menu_sounds.txt`; archive lists `tools/init.list`, `tools/*language.list`, `tools/patch.full.list` (Darwinia-era).

**Encoding.** ASCII; CRLF and LF accepted (`\r\n` → `\n`, `chomp`).

```ebnf
exclusion_file = { excl_line EOL } ;
excl_line      = '**/' NAME          (* prune any file or directory whose basename == NAME (exact, case-sensitive File::Find $_ compare) *)
               | GLOB ;              (* '\' normalised to '/'; expanded with File::Glob::bsd_glob after chdir(data/);
                                        a pattern without wildcards is kept verbatim (GLOB_NOMAGIC); a visited path equal,
                                        ignoring case, to any expanded entry is pruned (a directory prunes its subtree) *)
inclusion_file = { RELPATH EOL } ;   (* exact file paths relative to data/, no globs; copyfiles.pl dies "<path> <errno>" if missing *)
sound_list     = { RELPATH EOL } ;   (* relative to data/, forward slashes, e.g. "mwsounds/attack1.ogg" *)
archive_list   = { RELPATH EOL } ;   (* init.list / *language.list: rar @listfile entries *)
```

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| exclusion `**/NAME` | basename | – | exact, case-sensitive | prune by basename anywhere | `tools/listfiles.pl` (`wanted`) |
| exclusion `GLOB` | bsd_glob pattern | – | relative to `data/` | prune matching paths (case-insensitive comparison of expanded entries) | same |
| inclusion path | relative path | – | must exist | copied back in after exclusion | `tools/copyfiles.pl` |
| sound list path | relative path | – | `sounds/…` or `mwsounds/…` | (1) `sed` prefixes `data/` (all lines of `sounds.txt`; lines starting `sounds`/`mwsounds` of `menu_sounds.txt`) → RAR `@listfile` for `sounds.dat` / `menusounds.dat`; (2) both lists are concatenated with the platform exclusion list into `completeExclusions`, so listed sounds are excluded from the catch-all `sounds2.dat` | `tools/build_multiwinia_data.bat` |

`listfiles.pl` prints every surviving **file** as a `/`-separated path relative to `data/`, in `File::Find` traversal order [C].

**Verification.** `X1b_sounds_grammar.py` (re-run): `tools/sounds.txt` 1 297 entries (935 `sounds/`, 362 `mwsounds/`), `tools/menu_sounds.txt` 18 entries (10 `mwsounds/multilobbyamb1`–`10.ogg`; `sounds/shipheartbeat1`, `mouse2`, `mouse3`, `menuscroll2`, `menuscroll1`, `alien_lands3`, `evil`, `rumble1`), 0 missing on disk, no overlap, no duplicates; of 1 332 Oggs 1 315 are listed and 17 remain, of which 11 are excluded and 6 go to `sounds2.dat` (`mwsounds/rollover1a`, `rollover1b`, `rollover1c`, `mouseclick1`, `chat`, `beep_harsh_malfunction`). Exclusion semantics re-implemented by `_work/scripts/S_31_archive_contents.py` [C scripted].

**Minimal valid example** (an exclusion list and a sound list)

```text
**/.svn
levels/old/*
textures/unused_*
```

```text
mwsounds/lobby_loop1.ogg
sounds/click.ogg
```

**Discrepancies.** All list entries resolve to existing files only when compared case-insensitively (e.g. `mp_koth_2p_1.txt` vs on-disk `mp_koth_2P_1.txt`); on a case-sensitive file system 30 entries would fail [C scripted, 31]. `Marketing/LogitechG15/landrotate.bmp` in the exclusion lists matches nothing [C].

---

## 11 Runtime-written, network and tool files

<a id="fmt-runtime-files"></a>
### 11.1 Index of runtime-written files

**Purpose.** Every file the running game (or its support tool) writes or reads outside `data/`. `<P>` = profile directory (`%LOCALAPPDATA%\Introversion\Multiwinia\` on Win2000+, the working directory with `--NoProfile` or `TARGET_DEBUG`) [C] `code/app.cpp:App::GetProfileDirectory` ([31 §5](31_platform_build_and_tools.md)); `./` = working directory.

| File | Writer → reader | Format | Tag |
|---|---|---|---|
| `<P>preferences.txt` | `PrefsManager::Save` → `PrefsManager::Load` | [fmt-preferences](#fmt-preferences) (UTF-16LE) | [B:FULL] |
| `<P>user_input_preferences.txt` | keybindings window → `InputManager::parseInputPrefs` | [fmt-input-bindings](#fmt-input-bindings) | [B:FULL] |
| `<P>authkey.dev`, `<P>authkey` | `Authentication_SaveKey` → `Authentication_LoadKey` | [fmt-authkey](#fmt-authkey) | [B:FULL] |
| `<P>syncerror-<date>-client<c>-team<t>.txt`, `<P>syncdiff-…txt` | `Location::DumpWorldToSyncLog`, `DumpSyncDiffs` | [fmt-sync-dump](#fmt-sync-dump) | [B:FULL] |
| `<P>debug.txt` | `AppDebugOut` (`DUMP_DEBUG_LOG`) | [fmt-debug-log](#fmt-debug-log) | [B:FULL] |
| `<P>blackbox.txt` | `AppGenerateBlackBox` | [fmt-blackbox](#fmt-blackbox) | [B:FULL] |
| `<P>ERRORLOG.TXT`, `<P>CRASH.DMP` | `RecordExceptionInfo` | [fmt-crash-report](#fmt-crash-report) | [B:FULL] |
| `<P>screenshots/screenshot<N>.png` | `PosterMaker::SavePoster` | [fmt-png-output](#fmt-png-output) | [B:FULL] |
| `<P>users/<profile>/game.txt`, `<P>users/<profile>/mission_*.txt` | `GlobalWorld::SaveGame`, `LevelFile::SaveMissionFile` → `LoadGame`, `ParseMissionFile` | [fmt-profile-save](#fmt-profile-save) | [B:LEGACY] |
| `<P>maps/<name>.mwm` (+ `<P>maps/<name>.txt` from `LevelFile::Save`) | editor → `MapFile`, `LevelFile::ParseMWMFile` | [fmt-mwm](#fmt-mwm) | [B:EDITOR] |
| `./serverHistory.dat` | `Server::SaveHistory` (pref `RecordDemo` = 1) → `Server::LoadHistory("ServerHistory.dat")` (pref = 2) | [fmt-server-history](#fmt-server-history) | [B:FULL], debug feature |
| `./origin` (read only), `./version` | packaging ↔ `App::Initialise`, `--write-version` | [fmt-origin-version](#fmt-origin-version) | [B:FULL] |
| `./language_errors.txt` | `LangTable::TestAgainstEnglish` (pref `TextLanguageTest` = 1) | [fmt-language-errors](#fmt-language-errors) | [B:FULL], QA |
| `<P>inputprefs_debug.txt` | `parseInputPrefs` diagnostics | free text, one message per rejected line | Mac / [B:DEBUG] |
| `<P>dxdiag_info.txt`, `<P>multiwinia.zip` | `MultiwiniaSupport.exe` | `dxdiag /t` text; zip bundle ([fmt-support-ini](#fmt-support-ini)) | support tool |
| `sounderrors.txt`, `data/sounds.txt` (save) | `SoundSystem::LoadtimeVerify` (call commented out), `SaveBlueprints` (sound editor) | [fmt-sounds-txt](#fmt-sounds-txt) writer | [B:DEAD] / [B:DEBUG] |
| `data/gestures.txt` (save) | `Gesture::SaveTrainingData` | [fmt-gestures](#fmt-gestures) | [B:DEBUG] |
| `<P>dxdiag.txt`, `<P>benchmarks/game <key> <time>.txt` | `Benchmark` | free text | [B:DEAD] (`BENCHMARK_AND_FTP`) |
| `TestBedLog.txt`, `test_log.txt`, `gltrace.txt`, memory-leak report | test bed / test harness / D3D GL trace / `ParseMemoryLeakFile` | free text | [B:DEAD] (respective macros) or debug tools ([32](32_debug_test_and_performance.md)) |
| `<P>users\<profile>.dsg` | Vista rich saved-game header + PNG thumbnail | binary | [B:DEAD] (`TARGET_OS_VISTA`) |
| `achievements.txt` | `ACHIEVEMENT_RECORD_FILENAME` defined; `AchievementTracker::Save`/`Load` are empty | – | never written [C] `code/achievement_tracker.cpp` |

**Verification.** Index assembled from X1a §13.1 and [31 §5.3](31_platform_build_and_tools.md); writer functions spot-checked for this document (`ClientToServer::GetSyncFileName`, `Server::SaveHistory`, `Authentication_SaveKey`/`LoadKey`, `PosterMaker::SavePoster`, `AchievementTracker::Load`).

**Minimal valid example.** See each linked section.

**Discrepancies.** Installers' `[UninstallDelete]` and `MultiwiniaSupport.ini` assume user files under the install directory; they are written to `<P>` on Win2000+ [C] ([31 §5](31_platform_build_and_tools.md)).

<a id="fmt-profile-save"></a>
### 11.2 Campaign save files [B:LEGACY]

**Purpose / files.** Darwinia campaign progress: `<P>users/<profile>/game.txt` and `<P>users/<profile>/mission_*.txt` [C] `code/global_world.cpp:GlobalWorld::SaveGame`, `code/level_file.cpp:LevelFile::SaveMissionFile`, `contrib/systemiv/lib/filesys/text_file_writer.cpp:TextFileWriter`; read back first by `GlobalWorld::LoadGame` and `LevelFile::ParseMissionFile`.

**Encoding.** 8-bit text; "redshirt2"-obfuscated in release builds, plain under `TARGET_DEBUG` ([fmt-redshirt](#fmt-redshirt)); `fopen("w")` text mode → CRLF.

**Grammar / fields.** Exactly [fmt-game-txt](#fmt-game-txt) and the mission blocks of [fmt-level-file](#fmt-level-file) (`Difficulty`, `CameraMounts`, `CameraAnimations`, `Buildings`, `InstantUnits`, `Routes`, `PrimaryObjectives`, `RunningPrograms`, `LeaderboardStats`), after de-obfuscation.

| Rule | Value | Ref |
|---|---|---|
| when written | not for profile names `none`, `AccessAllAreas`, `AttractMode`, not for non-full versions, and only in single-player modes | `code/app.cpp:App::SaveProfile` |
| header | `redshirt2` with no newline, then the first line | `TextFileWriter::TextFileWriter` |
| encoded bytes | 33…127 shifted by the running key, ≤ 32 and ≥ 128 unchanged | `TextFileWriter::printf` |

**Verification.** Writer constructor and `printf` re-read for this document; readers per [fmt-redshirt](#fmt-redshirt). No sample in the tree.

**Minimal valid example.** The [fmt-redshirt](#fmt-redshirt) worked example: a mission save whose first content line is `Kills 7` begins with the bytes `redshirt2Rrmwu <` CR LF.

**Discrepancies.** `ResetProfileButton` deletes `users/<name>/` relative to the working directory while saves live under `<P>` — the wrong folder (legacy only) [C] ([09](09_meta_systems.md)).

<a id="fmt-authkey"></a>
### 11.3 Authentication key files

**Purpose.** The product key sent to the metaserver [C] `contrib/systemiv/lib/metaserver/authentication.cpp:Authentication_SaveKey`, `Authentication_LoadKey`, `Authentication_SetKey`, `Authentication_SimpleKeyCheck`, `Authentication_IsDemoKey`, `Authentication_IsAmbrosiaKey`, `Authentication_IsHashKey`; `code/app.cpp:App::InitMetaServer`. Validation protocol: [22](22_determinism_and_networking.md).

**Files.** Read: `<P>authkey.dev` only (`App::InitMetaServer`). Written: `<P>authkey.dev` by `code/UI/GameMenuWindow.cpp:GameMenuWindow::PasteFromClipboard` and `code/UI/PlayDemoAuthkeyButton.h`; `<P>authkey` by `code/UI/AuthInputField.h` (every keystroke) [C].

**Encoding.** ASCII; written with `fopen("wt")` + `fprintf("%s\n")`.

```ebnf
authkey_file = KEY , LF ;
KEY          = GROUP6 '-' GROUP6 '-' GROUP6 '-' GROUP6 '-' GROUP3      (* 31 characters = AUTHENTICATION_KEYLEN 32 - 1 *)
             | 'hsh' , HEXWORDS ;                                      (* hashed key, accepted without local check *)
GROUP6       = 6 * UPPER ;  GROUP3 = 3 * UPPER ;
HEXWORDS     = 5 * ( 4*hexdigit ) ;     (* SHA-1 words printed %04x: at least 4 digits each -> 23..43 characters total *)
```

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| key | first whitespace-delimited word (`fscanf("%s")`, no length limit, buffer `char[256]` in `App::InitMetaServer`) | `authkey not found` when the file is missing | 31 chars, or `hsh…` | trailing characters not in `A–Z`, `a–z`, `-` are trimmed; passes the local check if it is a hash key or exactly 31 characters; prefixes `DEMO` (demo key) and `AMBR` (Ambrosia distribution) recognised | `Authentication_LoadKey`, `Authentication_SetKey`, `Authentication_SimpleKeyCheck` |
| hash form | `HashData(key, token)` | – | – | SHA-1 of `"<key>-<token>"` (≤ 511 chars), output `hsh` + five words `%04x`; sent instead of the raw key using the server's hash token | `contrib/systemiv/lib/math/hash.cpp:HashData` |

**Verification.** `Authentication_SaveKey`, `Authentication_LoadKey`, `Authentication_SetKey` and the save call sites re-read for this document [C].

**Minimal valid example**

```text
ABCDEF-GHIJKL-MNOPQR-STUVWX-YZA
```

**Discrepancies.** A hand-typed key is saved to `<P>authkey` but only `<P>authkey.dev` is read at start-up, so it is not re-read on the next launch [C] (Resolved: P14 and the meta contradiction note described the two writers; both are right, see [31 §5.3](31_platform_build_and_tools.md)). `Authentication_GenerateKey` is a stub returning a fixed demo key (not reproduced) [C]. `MultiwiniaSupport.ini` collects `authkey`, not `authkey.dev` [C].

<a id="fmt-sync-dump"></a>
### 11.4 Desync dump files

**Purpose.** Forensic output when the server reports this client out of sync, or when the I-frame comparison finds differences [C] `code/network/clienttoserver.cpp:ClientToServer::GetSyncFileName`, `ClientToServer::ProcessServerLetters` (`NET_DARWINIA_SYNCERROR` branch), `code/location.cpp:Location::DumpWorldToSyncLog`, `code/network/iframe.cpp:DumpSyncDiffs`, `code/network/syncdiff.cpp:SyncDiff::Print`, `contrib/systemiv/lib/debug_utils.cpp:HashDouble`. Sync design: [22](22_determinism_and_networking.md).

**Files.** `<P><prefix>-%Y-%m-%d--%H-%M-%S-client<clientId>-team<myTeamId>.txt`; the time is the **server start time** (`m_serverStartTime`, local time); prefix `syncerror` (first out-of-sync report only) or `syncdiff` (I-frame diff path) [C] `GetSyncFileName`.

**Encoding.** ASCII; `syncerror` opened in append mode (`fopen "a"`), `syncdiff` truncating.

```ebnf
syncerror_file = [ 'SyncRandSeed ' UINT ' (as signed int: ' INT ')' LF , { randlog_line LF } ]   (* TRACK_SYNC_RAND only *)
                 LF LF 'Begin Final Object Positions' LF , { state_line LF } ;
state_line     = ENT | OFFICER | DARWINIAN | WOBJ | BUILDING | TREE | CRATE | BOXKITE ;
ENT            = 'ENT Type[' TYPENAME '] Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] front[' H ']' ;
WOBJ           = 'WOBJ Type[' INT '] Id[' INT '] pos[' H ',' H ',' H '], vel[' H ']' ;
BUILDING       = 'BUILDING Type[' TYPENAME '] Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] front[' H '] centre[' H '], radius[' H ']' ;
TREE           = 'TREE Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] front[' H '] centre[' H '] radius[' H '] height[' H '] hitcheckCentre[' H '] hitcheckRadius[' H ']' ;
OFFICER        = 'ENT OFFICER Id[' INT '] state[' INT '] pos[' H ',' H ',' H '] vel[' H '] front[' H '] waypoint[' H ']' ;
DARWINIAN      = 'DARWINIAN Id[' INT '] state[' INT '] pos[' H ',' H ',' H '] vel[' H '] front[' H '] waypoint[' H ',' H ',' H '] boxId[' INT ']' ;
CRATE          = 'CRATE Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] radius[' H ']' ;
BOXKITE        = 'BOXKITE Id[' INT '] state[' INT '] pos[' H '], vel[' H ']' ;
H              = 16 * hexdigit ;      (* IEEE-754 bits of a double, LEAST significant nibble FIRST, lower case *)
syncdiff_line  = 'syncdiff: id: (team: ' T ', unit: ' U ', index: ' I ', uniqueId: ' Q ') (' x ', ' y ', ' z ')  - ' NAME ' - ' DESCRIPTION LF ;
```

| Field | Type | Semantics | Parser ref |
|---|---|---|---|
| order of state lines | – | for each used team: every entity of every unit, then every "other" entity; then every effect except snow; then every building | `Location::DumpWorldToSyncLog` |
| `H` of a vector quantity summarised by one value | hex | hash of the **sum of its components** (`vel[x+y+z]`, `front[x+y+z]`) | `Entity::LogState`, `WorldObject::LogState` |
| syncdiff positions | default `ostream` doubles | – | `SyncDiff::Print` |

**Verification.** `GetSyncFileName`, `HashDouble`, the opening of `DumpWorldToSyncLog` re-read for this document; line grammars from X1a's reading of every `LogState` [C].

**Minimal valid example** (the double 1.0 = 0x3FF0000000000000 is written least-significant nibble first as `0000000000000ff3`)

```text


Begin Final Object Positions
CRATE Id[12] pos[0000000000000ff3,0000000000000000,0000000000000ff3], vel[0000000000000000] radius[0000000000000ff3]
```

**Discrepancies.** None found; the files are diagnostics with no reader.

<a id="fmt-mwm"></a>
### 11.5 `.mwm` custom map container [B:EDITOR]

**Purpose.** A user map with embedded textures [C] `code/mapfile.cpp:MapFile::MapFile`, `MapFile::Save`, `SaveTextures`, `SaveTexture`, `LoadTextures`, `LoadTexture`, `GetTextReader`, `IsSaveRequired`; `code/mapfile.h`; `code/level_file.cpp:LevelFile::ParseMWMFile`.

**Files.** `<P>maps/<name>.mwm` (directory from `App::GetMapDirectory`); only editor builds create or list them (`GameMenu::CreateMapList` under `LOCATION_EDITOR`), but `Location::LoadLevel` would load one in any build if a non-`.txt` map name were requested [C].

**Byte layout**

```ebnf
mwm_file         = uncompressed_len:i32 , zlib_stream ;   (* i32 via WriteNetworkValue (little-endian) *)
zlib_stream      = compress2(level 9) of the fmt-directory serialisation of a Directory named "NewMap" ;
newmap_directory = field 'MapData' : VOID                 (* the map text exactly as LevelFile::Save writes it, fmt-level-file *)
                 , { texture_fields } ;
texture_fields   = field TEXNAME 'Filename' : STRING , field TEXNAME : VOID
                 , field 'TextureHeight' n : INT , field 'TextureWidth' n : INT ;
TEXNAME          = 'MapLandscapeTexture' | 'MapWavesTexture' | 'MapWaterTexture' | 'MapThumbnail' ;
```

| Field | Type | Units | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `uncompressed_len` | i32 | bytes | – | ≥ 0 | buffer size for `uncompress` | `MapFile::MapFile` |
| `MapData` | VOID | bytes | – | – | map text, parsed with a `TextDataReader` (unterminated last line dropped, §3.1) | `MapFile::GetTextReader` |
| `<Tex>Filename` | STRING | – | – | `terrain/<file>` or the thumbnail path | name under which the pixels are registered (`Resource::AddBitmap`) | `SaveTexture`, `LoadTexture` |
| `<Tex>` | VOID | RGBA8 pixels | – | width × height × 4 bytes, bottom row first | texture data | same |
| `TextureHeight<n>`, `TextureWidth<n>` | INT | pixels | −1 if absent (then the texture is skipped) | – | n = 1-based index in the order present (landscape, waves, water, thumbnail) | same |

A texture is embedded only when it is not already an archived resource (`IsSaveRequired`), and the thumbnail only when not `default.bmp` [C].

**Verification.** `MapFile::MapFile`, `Save`, `SaveTexture`, `SaveTextures`, `LoadTextures`, `LoadTexture` re-read for this document; field names from `code/mapfile.h` (`MAPFILE_MAPDATA`, `MAPFILE_LANDSCAPETEX`, `MAPFILE_WAVESTEX`, `MAPFILE_WATERTEX`, `MAPFILE_TEXTUREWIDTH`, `MAPFILE_TEXTUREHEIGHT`, `MAPFILE_THUMBNAIL`). No shipped sample.

**Minimal valid example.** `uncompressed_len` = length of the serialised node `NewMap` that holds a single VOID field `MapData` containing the text of the §4.1.19 map, followed by its zlib stream; no texture fields (all three colour files are archived resources).

**Discrepancies / hazards** [C]: `Save` allocates the compression buffer with the *uncompressed* size and ignores `compress2`'s result (incompressible data would fail silently); `MapFile::MapFile` reads `file length` bytes after already consuming the 4-byte length (the last 4 bytes of the buffer are uninitialised; zlib stops at the stream end); the stored length is trusted for the `uncompress` buffer.

<a id="fmt-server-history"></a>
### 11.6 `serverHistory.dat` — server message recording

**Purpose.** Records the server's sequenced letter history for replay [C] `code/network/server.cpp:Server::SaveHistory`, `Server::LoadHistory`; triggers `code/app.cpp:App::ShutdownCurrentGame`, `code/main.cpp` (`RecordDemo` checks).

**Files.** `./serverHistory.dat` (written at game shutdown when pref `RecordDemo` == 1); `./ServerHistory.dat` (read after a location loads when `RecordDemo` == 2) — the same file on case-insensitive file systems.

**Byte layout**

```ebnf
history_file = count:i32 , count * directory ;     (* i32 raw, host byte order; directory = fmt-directory *)
```

| Field | Type | Semantics | Parser ref |
|---|---|---|---|
| `count` | i32 (raw `ostream::write`) | number of history letters | `Server::SaveHistory` |
| letters | `Directory::Write(ostream)` each | the server-to-client letters in sequence order | same; `Server::LoadHistory` |

**Verification.** `Server::SaveHistory` and the `RecordDemo` call sites re-read for this document [C]. No version header; replay requires byte-identical code and data [I].

**Minimal valid example.** `00 00 00 00` — an empty history (count 0).

**Discrepancies.** On load `m_sequenceId` is set to `lastSeq + 1` and the comparison `seqId > m_sequenceId` skips the update when equal (quirk, [22](22_determinism_and_networking.md)) [I].

<a id="fmt-team-controls"></a>
### 11.7 Packed team controls (`ALIVE_DATA` payload)

**Purpose.** The per-0.1 s player-input record every client sends inside its ALIVE letter (VOID field `dg`) [C] `code/team.cpp:TeamControls::Pack`, `TeamControls::Unpack`, `TeamControls::GetFlags`, `TeamControls::SetFlags`; sender `code/main.cpp:SendTeamControlsToNetwork`; consumer `Location::UpdateTeam` ([22](22_determinism_and_networking.md)).

**Encoding.** Binary, `WriteNetworkValue` (little-endian), written into a 256-byte `ostrstream`.

| Offset | Field | Type | Present when | Semantics |
|---|---|---|---|---|
| 0 | flags | u16 | always | 0x0001 unitMove, 0x0002 directUnitMove, 0x0004 primaryFireTarget, 0x0008 secondaryFireTarget, 0x0010 targetDirected, 0x0020 secondaryFireDirected, 0x0040 cameraEntityTracking, 0x0080 unitSecondaryMode, 0x0100 endSetTarget, 0x0200 consoleController, 0x0400 leftButtonPressed |
| 2 | `m_directUnitMoveDx`, `m_directUnitMoveDy` | i16, i16 | flag 0x0002 | direct-control movement |
| +0 | `m_directUnitTargetDx`, `m_directUnitTargetDy` | i16, i16 | flag 0x0010 or 0x0020 | direct-control aim |
| +0 | `m_mousePos` | 3 × f64 (Vector3) | always | world-space mouse position |

Total 26, 30 or 34 bytes.

**Verification.** `GetFlags`, `SetFlags`, `Pack`, `Unpack` and the `TeamControls` declaration (`code/team.h`) re-read for this document [C].

**Minimal valid example.** Flags 0x0001 (unit move), mouse at (100, 0, 200): `01 00` + `00 00 00 00 00 00 59 40` + `00 00 00 00 00 00 00 00` + `00 00 00 00 00 00 69 40` (26 bytes).

**Discrepancies.** The header comment refers to `NetworkUpdate::GetByteStream`, which no longer exists [C].

<a id="fmt-blackbox"></a>
### 11.8 `blackbox.txt` — release-assert report

**Purpose / files.** `<P>blackbox.txt`, written by `contrib/systemiv/lib/debug_utils.cpp:AppGenerateBlackBox` on release asserts and `AppReleaseExit` ([31 §6.5](31_platform_build_and_tools.md)).

```ebnf
blackbox = '=========================' LF '=   BLACK BOX REPORT    =' LF '=========================' LF LF
           APP_NAME ' ' APP_VERSION ' built ' BUILD_DATE LF
           'Date ' HOUR ':' MINUTE ', ' DAY '/' MONTH '/' YEAR LF LF
           [ "ERROR : '" MESSAGE "'" LF ]
           LF '=========================' LF '=      STACKTRACE       =' LF '=========================' LF LF
           { 'retAddress = ' POINTER LF } ;      (* EBP-chain walk; stops on misaligned, non-increasing or unreadable frames *)
```

| Field | Type | Semantics |
|---|---|---|
| `APP_NAME`, `APP_VERSION`, `BUILD_DATE` | text | `code/lib/universal_include.h` macros and `__DATE__` |
| `MESSAGE` | text | the formatted assert message |
| `POINTER` | `%p` | return addresses; symbolised offline by `tools/trace.pl` + a linker map |

**Verification.** From X1a and [31 §6.5](31_platform_build_and_tools.md) readings of `AppGenerateBlackBox` [C].

**Minimal valid example**

```text
=========================
=   BLACK BOX REPORT    =
=========================

Game 1.0 built Jan  1 2008
Date 12:30, 1/2/2008

ERROR : 'Couldn't open stats.txt'

=========================
=      STACKTRACE       =
=========================

retAddress = 0040A1B2
```

**Discrepancies.** Release builds use `/O2` (frame-pointer omission), so the EBP chain is usually truncated [I] ([31 §6.5](31_platform_build_and_tools.md)).

<a id="fmt-crash-report"></a>
### 11.9 `ERRORLOG.TXT` and `CRASH.DMP` — SEH crash report

**Purpose / files.** `<P>ERRORLOG.TXT` (text) and `<P>CRASH.DMP` (Windows minidump, `MiniDumpWriteDump(…, MiniDumpWithIndirectlyReferencedMemory, …)`), written by `code/lib/exception_handler.cpp:RecordExceptionInfo` from the `__except` around `RunTheGame` in `code/main.cpp:AppMain`; both are collected by the support tool ([fmt-support-ini](#fmt-support-ini)). Full description: [31 §6.2–§6.3](31_platform_build_and_tools.md).

| Part of `ERRORLOG.TXT` (order) | Content |
|---|---|
| header | `"<exe> caused <description> (0x%08x) in module <module> at %04x:%08x"`, `"Exception handler called in AppMain."` |
| system information | crash reference code (low 32 bits of the current FILETIME, hex), local time, exe path, Windows user name, OS name and `major.minor.build`, processor count/type, memory load %, physical/page/virtual totals and free (MB, rounded up) |
| access violation detail | `"Read from"` / `"Write to location %08x"` |
| registers | EDI ESI EAX EBX ECX EDX EIP EBP SegCs EFlags ESP SegSs |
| code bytes | `NumCodeBytes` = 16 bytes at EIP (`??` if unreadable) |
| stack dump | from ESP to the TIB stack top, ≤ `MaxStackDump` = 3072 DWORDs, 4 per row with ASCII |
| module list | every image found by `VirtualQuery`: path, base/size, checksum, timestamp, file size/time, version resource |
| trailer | `===== [end of ERRORLOG.TXT] =====` |

Exception descriptions: 24 codes ([31 §6.3](31_platform_build_and_tools.md)). The file is opened `CREATE_ALWAYS`, so each crash overwrites the previous log [C].

**Verification.** From [31 §6.2](31_platform_build_and_tools.md) (P14 reading of `RecordExceptionInfo`) [C]; not re-read for this document.

**Minimal valid example.** Not applicable (machine-generated diagnostic; the minidump is Microsoft's format).

**Discrepancies.** Worker threads have no SEH wrapper — a fault there produces no `ERRORLOG.TXT` [C] ([31 §6.2](31_platform_build_and_tools.md)).

<a id="fmt-debug-log"></a>
### 11.10 `debug.txt` — debug log

**Purpose / files.** `<P>debug.txt`: every `AppDebugOut` message (`DUMP_DEBUG_LOG` is defined in FULL), truncated at start-up, then opened, appended and closed on every call under a mutex [C] `contrib/systemiv/lib/debug_utils.cpp:AppDebugOutRedirect`, `code/main.cpp:Initialise` ([31 §5.3](31_platform_build_and_tools.md)).

```ebnf
debug_log = { MESSAGE } ;     (* free text exactly as formatted by each AppDebugOut call; most messages end with LF *)
```

| Field | Type | Semantics |
|---|---|---|
| message | printf-formatted text | e.g. `Loading archive: main.dat`, `Failed to find text resource: data/<name>`, `Duplicate preference item %s. Ignoring` |

**Verification.** Message texts quoted elsewhere in this document were confirmed in their parsers [C].

**Minimal valid example**

```text
Loading archive: init.dat
File init.dat, 16 files (0.120000 seconds)
```

**Discrepancies.** Opening and closing the file per message is slow but safe across threads [C].

<a id="fmt-origin-version"></a>
### 11.11 `origin` and `version` files

**Purpose / files.** `./origin` (optional, read only): distributor tag; `./version`: written by the `--write-version` command-line switch [C] `code/app.cpp:App::Initialise`, `code/main.cpp:AppMain`.

```ebnf
origin_file  = TAG [ EOL ] , { ignored_line } ;    (* first line via TextFileReader + GetRestOfLine *)
version_file = DARWINIA_VERSION ;                  (* bare string, no newline *)
```

| Field | Type | Default | Semantics | Parser ref |
|---|---|---|---|---|
| `TAG` | REST (comments/CR stripped, trailing blanks kept) | `unknown` | `App::m_originVersion`, sent to the metaserver (e.g. `buka`) | `App::Initialise` |
| `DARWINIA_VERSION` | text | `dev` in this tree | build version string used by packaging scripts; the program returns immediately after writing | `AppMain` |

**Verification.** From X1a's reading of both call sites [C].

**Minimal valid example** (`origin`)

```text
retail-eu
```

**Discrepancies.** `DARWINIA_VERSION "dev"` suggests this tree is a development configuration rather than the exact retail binary [I] ([31 §5.3](31_platform_build_and_tools.md)).

<a id="fmt-language-errors"></a>
### 11.12 `language_errors.txt` — translation QA report

**Purpose / files.** `./language_errors.txt`, truncated at each language load when pref `TextLanguageTest` = 1 [C] `code/lib/language_table.cpp:LangTable::TestAgainstEnglish`. (A second writer in `code/sepulveda.cpp` appends under `USE_SEPULVEDA_HELP_TUTORIAL` [B:DEAD].)

```ebnf
lang_errors = { error_line LF } ;
error_line  = "ERROR : Failed to find translation for string ID '" ID "'"
            | "ERROR : String ID appears not to be translated : '" ID "'"          (* text identical to English *)
            | "ERROR : Found new string ID not present in original English : '" ID "'" ;
```

| Field | Type | Semantics |
|---|---|---|
| `ID` | phrase id | the offending key ([fmt-language](#fmt-language)) |

**Verification.** From X1a's reading of `TestAgainstEnglish` [C].

**Minimal valid example**

```text
ERROR : Failed to find translation for string ID 'menu_play'
```

**Discrepancies.** None found.

<a id="fmt-user-info"></a>
### 11.13 Registration system/user info texts [B:LEGACY]

**Purpose.** Inputs of the Darwinia-era key generator, compiled into the exe but never called [C] `code/keygen.cpp:GenerateKey`, `code/lib/user_info.cpp:GetUserInfoData`; `targets/linux/Makefile.keygen-1` built it as a server tool.

```ebnf
sys_info  = 'SysInfoGenVersion' ... LF 'CpuVendor:' ... LF 'CpuModel:' ... LF 'Num Processors:' ... LF 'Country:' ... LF
            'Language:' ... LF 'Num sound devices:' ... LF 'Primary sound device:' ... LF 'Graphics Card:' ... LF
            'Operating System:' ... LF 'Computer Name:' ... LF ;          (* 184..500 bytes; value at offset 18 must parse as 2 *)
user_info = 'Username:' VALUE LF 'Email:' VALUE LF ;                      (* 28..200 bytes *)
```

| Field | Type | Range | Semantics | Parser ref |
|---|---|---|---|---|
| system info text | ASCII lines in fixed order | 184…500 bytes | hashed: buffer as `len/4` LE u32 words, `rv = 28477717`; for `i = 0…1709`: `rv ^= word[(rv + i) mod (len/4)]` | `code/keygen.cpp` |
| user info text | 2 lines | 28…200 bytes | hashed the same way | same |
| password | `printf("%08x%08x", userKey, sysKey)` | 16 hex chars | result; on validation failure an error string ("system info file too short", "Username missing", …) is returned instead | same |

**Verification.** Length limits, line keys, constants 28477717 / 1710 and the `%08x` output re-checked by grep for this document [C].

**Minimal valid example** (`user_info`)

```text
Username: sample
Email: sample@example.invalid
```

**Discrepancies.** No caller in `code/` (only `#include "keygen.h"` in `code/app.cpp`) [C].

<a id="fmt-support-ini"></a>
### 11.14 `MultiwiniaSupport.ini` — crash-report file list

**Purpose.** Tells the separate crash-report tool (built from `contrib/XCrashReport`, shipped as `MultiwiniaSupport.exe`) which files and registry keys to bundle [C] `contrib/XCrashReport/Dlg.cpp:CDlg::GetFileDetails`, `contrib/XCrashReport/IniDefines.h`; packaging `tools/build_multiwinia_crashreporter.bat`. Tool behaviour: [31 §6.4](31_platform_build_and_tools.md).

**Files.** Root `MultiwiniaSupport.ini`. **Encoding.** ASCII, CRLF; parsed with Win32 `GetPrivateProfileString` (not `TextReader`).

```ebnf
ini     = { comment | section | entry } ;            (* Win32 INI rules: ';' comment lines, [Section], Key=Value *)
section = '[FilesToAdd]' | '[RegistryToAdd]' ;
entry   = 'File' NNN '=' PATH ',' DESCRIPTION ',' FILETYPE      (* NNN = 001..999, consecutive *)
        | 'Registry' NNN '=' REGPATH ',' DESCRIPTION ;
```

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| `NNN` | 3-digit number | – | from 001, consecutive | reading **stops at the first missing number** | `CDlg::GetFileDetails` |
| `PATH` | text without `,` | – | – | a path lacking both `\` and `:` is relative to the profile directory given to the tool, else to the tool's directory; non-existent files skipped | same |
| `DESCRIPTION`, `FILETYPE` | text without `,` | – | – | shown in the tool; fields split with `_tcstok` on `,`, CR, LF; a missing field aborts the list | same |
| (no accepted file entry) | – | built-in `ERRORLOG.TXT`, `CRASH.DMP` | – | defaults | same |
| `REGPATH` | registry path | – | – | exported as `RegistryNNN.txt` | same |

**Verification.** From X1a's reading of `Dlg.cpp` [C]; shipped entries: `CRASH.DMP`, `ERRORLOG.TXT`, `DXDIAG.TXT`, `DXDIAG_INFO.TXT`, `BLACKBOX.TXT`, `PREFERENCES.TXT`, `userprefs.txt`, `authkey`, `debug.txt`; `[RegistryToAdd]` empty [C] file.

**Minimal valid example**

```text
[FilesToAdd]
File001=ERRORLOG.TXT,Error log,Text Document
File002=preferences.txt,Settings,Text Document
```

**Discrepancies** [C]: the game writes `user_input_preferences.txt`, not `userprefs.txt`; the auth key is read from `authkey.dev` (`authkey` is only one of two save names); `DXDIAG*.TXT` are produced only by the benchmark code (`BENCHMARK_AND_FTP`, off) or by the tool itself (`dxdiag_info.txt`); the `[FilesToAdd]` INI is located in the module directory only, the `[RegistryToAdd]` INI in the profile directory when one is given (inconsistent).

<a id="fmt-testbedconfig"></a>
### 11.15 `testbedconfig.txt` — automated test-bed sequence [B:DEAD]

**Purpose / parser.** `code/UI/GameMenuWindow.cpp:GameMenuWindow::ReadTestBedConfig`, compiled only under `TESTBED_ENABLED` (never defined); opened with a plain `TextFileReader("testbedconfig.txt")` relative to the working directory [C].

**Files.** Root `testbedconfig.txt` (14 lines, 4 commands, no final newline — processed anyway by `TextFileReader`, §3.1). **Encoding.** ASCII.

```ebnf
testbed_file = { blank | command } ;
command      = 'SetProgression' ( 'LINEAR' | 'RANDOM' | 'SEQUENCE' )        (* (cs) *)
             | 'SetRandSeed' INT                                             (* sscanf %d *)
             | 'SetTestBedName' WORD                                         (* sscanf %s into char[256], no limit *)
             | 'AddSequenceItem' GAMEINDEX RANDSEED                          (* both INT *)
             | 'SequenceIDToTerminateOn' INT
             | other ;                                                       (* ignored *)
```

| Field | Type | Default | Range | Semantics | Parser ref |
|---|---|---|---|---|---|
| command words | WORD (cs, `strcmp`) | – | as listed | a missing argument skips the command (`continue`) | `ReadTestBedConfig` |
| `GAMEINDEX` | INT | – | index into `g_vGameEntries` | concatenation, over game types 0…5 (type 6 skipped), of every map in that type's lobby list (`BuildGameEntries`) | same |
| `SetTestBedName` | WORD | – | ≤ 255 | also sets `App::m_testbedServerName` | same |

**Verification.** From X1a's reading [C]; shipped file: `SetProgression RANDOM`, `SetTestBedName TESTBED_GodOfCode`, `AddSequenceItem 0 10`, `AddSequenceItem 10 20` [C] file, `X1a_text_survey.py` (NO-FINAL-LF).

**Minimal valid example**

```text
SetProgression LINEAR
SetRandSeed 77
AddSequenceItem 3 1234
```

**Discrepancies.** `BuildGameEntries` stores `maps.GetData(i)->m_mapId` (game-type index `i`) instead of `GetData(x)` [C].
