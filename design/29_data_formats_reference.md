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

