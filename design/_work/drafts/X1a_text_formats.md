# X1a_text_formats — Text data formats (formal grammars, part A)
Scope: `data/levels/**`, `data/purity/*`, `data/game*.txt`, `data/locations.txt`, `data/multiwinia*.txt`, `data/*betatesters.txt`, `data/default_preferences.txt`, `data/input/**`, `data/language/*`, `data/scripts/*`, root `testbedconfig.txt`, `MultiwiniaSupport.ini`, plus every parser that reads them and every runtime-written text file found on the way. Build baseline: see build_matrix.md (`Release|Win32` + `TARGET_PC_FULLGAME`).

Conventions used in every grammar below:

* EBNF: `a b` sequence, `a | b` alternative, `[ a ]` optional, `{ a }` zero or more, `'lit'` literal (case-insensitive unless marked **(cs)** = case-sensitive), `EOL` end of a physical line.
* All grammars are stated **after** the shared line-cleaning and tokenising pass of §1 (`fmt-textreader`). "Token" means a maximal run of non-separator characters on a cleaned line.
* `INT` = whatever C `atoi` accepts (optional sign, decimal digits, leading junk stops the scan, no error reporting; an unparsable token yields 0). `FLOAT` = whatever C `atof` accepts (decimal, optional exponent). `F32` = `FLOAT` then rounded to IEEE-754 single precision and widened back to `double` (`iv_atof` ≡ `(double)(float)atof(s)`, `contrib/systemiv/lib/math/math_stubs.cpp:iv_atof`, `iv_trunc_low_nibble`) [C]. The F32 rounding is a determinism measure; a re-implementation must replicate it for any value that feeds the simulation.
* "MISSING ⇒ CRASH" means the parser calls `GetNextToken()` without checking `TokenAvailable()`; a short line gives a `NULL` token which is passed to `atoi`/`atof`/`strcpy`/`stricmp` → access violation or CRT invalid-parameter termination [C: code path; I: exact failure mode is MSVC CRT behaviour; no `_set_invalid_parameter_handler` exists in `code/` or `contrib/systemiv/lib`].

## Format index

| Anchor | Format | Files | Section | Build |
|---|---|---|---|---|
| `fmt-textreader` | shared line reader / tokeniser | all text formats | §1 | [B:FULL] |
| `fmt-level-file` | level map / mission files | `data/levels/**`, `data/purity/map_*`, `mission_*` | §2 | mp: [B:FULL]; campaign: [B:LEGACY] |
| `fmt-level-enum` | achievement level ids | `data/levels/level_enum.txt` | §2.19 | [B:FULL] |
| `fmt-game-txt` | campaign world | `data/game*.txt`, `data/purity/game.txt` | §3 | [B:LEGACY]/[B:DEAD] |
| `fmt-locations-txt` | world-map positions | `data/locations.txt` | §3.4 | [B:LEGACY] |
| `fmt-multiwinia-txt` | game types & parameters | `data/multiwinia.txt`, `data/multiwinia-testbed.txt` | §4 | [B:FULL] / [B:DEAD] |
| `fmt-betatesters` | credit name lists | `data/betatesters.txt`, `data/multiwinia_betatesters.txt` | §5 | [B:LEGACY] / [B:FULL] |
| `fmt-preferences` | preferences | `<P>preferences.txt`, `data/default_preferences.txt` | §6 | [B:FULL] |
| `fmt-input-bindings` | key/mouse/pad bindings | `data/input/input_preferences*.txt`, `<P>user_input_preferences.txt` | §7 | [B:FULL] |
| `fmt-keyboard-layout` | layout overrides | `data/input/keyboards/*.txt` | §7.6 | [B:FULL] |
| `fmt-locales` | LANGID → layout | `data/input/locales.txt` | §7.7 | [B:FULL] |
| `fmt-language` | phrase tables | `data/language/*.txt`, `strings_*.txt` overlays | §8 | [B:FULL] |
| `fmt-script` | cutscene scripts | `data/scripts/*.txt` | §9 | [B:LEGACY] |
| `fmt-purity-misc` | purity data set | `data/purity/*` | §10 | [B:DEAD] |
| `fmt-testbedconfig` | test-bed sequence | `testbedconfig.txt` | §11 | [B:DEAD] |
| `fmt-support-ini` | crash-report file list | `MultiwiniaSupport.ini` | §12 | [B:FULL tool] |
| `fmt-runtime-files` | runtime-written files (index) | profile directory, cwd | §13 | various |
| `fmt-profile-save` | campaign save games | `<P>users/<profile>/*` | §13.3 | [B:LEGACY] |
| `fmt-authkey` | authentication key | `<P>authkey.dev`, `<P>authkey` | §13.4 | [B:FULL] |
| `fmt-sync-dump` | desync dumps | `<P>syncerror-*.txt`, `<P>syncdiff-*.txt` | §13.5 | [B:FULL] |
| `fmt-mwm` | custom map container | `<P>maps/*.mwm` | §13.6 | [B:EDITOR] |

Generated tables: `_work/drafts/X1a_level_field_usage.csv` (every block/field/value × file set, from `_work/scripts/X1a_level_survey.py`), `_work/drafts/X1a_encoding_survey.csv` (from `_work/scripts/X1a_text_survey.py`). Other helper scripts: `_work/scripts/X1a_language_survey.py`, `X1a_input_lines.py`, `X1a_script_check.py`, `X1a_extract_read_funcs.py`, `X1a_coverage.py` (all run with `python3 -I`, paths as arguments).

<a id="fmt-textreader"></a>
## [→ 29_data_formats_reference] 1. Shared line reader and tokeniser (`TextReader` family)

### 1.1 Purpose, classes and where they are chosen

Every text format in this document is read through one of five reader classes, all compiled into the baseline [B:FULL] [C] `targets/vs2005/Darwinia.vcproj` (entries `contrib\systemIV\lib\filesys\text_stream_readers.cpp`, `contrib\systemIV\lib\unicode\unicode_text_stream_reader.cpp`):

| Class | Source | Input | Encoding | Used when |
|---|---|---|---|---|
| `TextReader` (abstract) | `contrib/systemiv/lib/filesys/text_stream_readers.{h,cpp}` | – | – | base: cleaning + tokenising |
| `TextFileReader` | same | loose disk file, `fopen(...,"r")` (CRT text mode → CRLF read as LF) | single-byte (no transcoding; bytes ≥0x80 pass through) | disk file without a UTF-16 BOM |
| `TextDataReader` | same | in-memory block from a `.dat` archive | single-byte, raw bytes (CR kept, removed by cleaning) | archive entry without a UTF-16 BOM |
| `UnicodeTextFileReader` | `contrib/systemiv/lib/unicode/unicode_text_stream_reader.{h,cpp}` | loose disk file, `fopen(...,"rb")` | UTF-16 with BOM `FF FE` (LE) or `FE FF` (BE); 2 bytes per code unit, no surrogate handling | disk file whose first two bytes are a BOM |
| `UnicodeTextDataReader` | same | archive block | as above | archive entry beginning with a BOM |

Selection is done by `code/lib/resource.cpp:Resource::GetTextReader(name)` [C]:

1. (`USE_DARWINIA_MOD_SYSTEM` only — [B:DEAD]) `<profile>/mods/<mod>/<name>`, then (`TARGET_OS_VISTA`) `mods/<mod>/<name>`.
2. Loose file `data/<name>` (relative to the working directory): try `UnicodeTextFileReader`; if `IsUnicode()` is false (no BOM) delete it and use `TextFileReader`.
3. Archive entry `data/<name>` — `UnicodeTextDataReader`, else `TextDataReader`. Archive keys are lower-cased by `Resource::ParseArchive` and stored in a `HashTable` whose hash folds case and whose compare is `stricmp`, so the lookup is **case-insensitive** [C] `code/lib/resource.h` (`HashTable<MemMappedFile*> m_resourceFiles`), `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::HashFunc`.
4. Otherwise `NULL` and the debug log line `Failed to find text resource: <path>`.

A few callers bypass `Resource` and construct `TextFileReader`/`UnicodeTextFileReader` on an absolute or cwd-relative path directly (preferences, profile saves, `testbedconfig.txt`, `origin`, input bindings): they are listed per format. `contrib/systemiv/lib/filesys/file_system.cpp:FileSystem::GetTextReader` is compiled but `g_fileSystem` is never created by Multiwinia [C] (grep: no `new FileSystem` under `code/`) — ignore it.

`TextFileReader` and `UnicodeTextFileReader` call `FindCaseInsensitive(path)` before opening (`contrib/systemiv/lib/filesys/filesys_utils.cpp`) so loose files are matched case-insensitively on case-sensitive file systems [C]; on Win32 this is a no-op in effect [I].

### 1.2 Physical lines

* A line ends at LF (`0x0A` / `L'\n'`). The final line may lack an LF. Lines may be arbitrarily long: the buffer starts at 128 characters and doubles; a line longer than 65 536 characters triggers `AppReleaseAssert("Text file contains line with more than 65536 characters")` [C] `TextReader::DoubleMaxLineLen`, `UnicodeTextReader::DoubleMaxLineLen`.
* End of file: only `TextFileReader` processes an unterminated last line (its `ReadLine` returns `true` for a final line without LF and `false` on the following call). `TextDataReader` (archive), `UnicodeTextFileReader` and `UnicodeTextDataReader` load the unterminated last line but return `false` for it, so the `while (in->ReadLine())` loop body **never sees it** [C] `TextDataReader::ReadLine` (`eof = true` when no `\n` is found), `UnicodeTextFileReader::ReadUnicodeLine` (returns `NULL` as soon as `feof`), `UnicodeTextDataReader::ReadLine`. Rule for authors: **always end a text file with a newline** — five shipped files do not (`level_enum.txt`, `testbedconfig.txt`, `japanese.txt`, `korean.txt`, `portuguese.txt`; consequences in §2.19, §8.2, §11).
* A U+0000 code unit in a UTF-16 disk file is skipped without being stored (the slot keeps stale data) [C] `UnicodeTextFileReader::ReadUnicodeLine` (`temp != NULL` test) — never emit NUL.

### 1.3 Line cleaning (`TextReader::CleanLine`, `UnicodeTextReader::CleanLine`) — applied to every line, in this order [C]

1. **De-obfuscation.** On the first line only, if the line starts with `redshirt2` (case-insensitive, 9 chars) the file is marked encrypted and those 9 characters are removed. For every subsequent character `> 32` in an encrypted file: `i = (i+1) mod 16; c' = c − K[i]; if c' < 33 then c' += 95` with `K = {31,7,9,1,11,2,5,5,3,17,40,12,35,22,27,2}` and `i` starting at 0 and persisting across lines. Characters `≤ 32` (space, tab, CR, LF) are untouched. The writer side is `TextFileWriter(..., encrypt=true)` / `UnicodeTextFileWriter` (`c' = c + K[i]; if c' ≥ 128 then c' −= 95`) [C] `code/unicode/unicode_text_file_writer.cpp:UnicodeTextFileWriter::printf`. No shipped file under `data/` is encrypted (survey: no file starts with `redshirt2`) [C] `X1a_text_survey.py`; encryption is used only for runtime-written profile saves (§13).
2. **Comment removal.** Scan left to right; the first `#` and everything after it is deleted. There is **no escape and no quoting**: a `#` inside what looks like a string still starts a comment. While scanning, runs of `<` / `>` are counted and three adjacent angle brackets trip `AppDebugAssert` (merge-conflict detector; debug builds only — [B:DEBUG]).
3. **Line-end removal.** Truncate at the first `\n`, then truncate at the first `\r`. Consequence: a stray CR in the middle of a line silently discards the rest of that line.

Edge case (UTF-16 disk files): `UnicodeTextFileReader::ReadLine` converts the wide line to the narrow mirror buffer (`UnicodeString::CopyUnicodeToCharArray`, every code unit > 255 becomes `?`) **before** `CleanLine` runs on the wide buffer, so the narrow mirror still contains the comment text; `UnicodeTextDataReader::ReadLine` cleans first and converts afterwards [C]. Tokenising of the first token is done on the narrow mirror (`TextReader::GetNextToken` is not virtual) while `TokenAvailable`/`GetNextUnicodeToken`/`GetRestOfUnicodeLine` are overridden and use the wide buffer. Both buffers have identical indices (1 code unit ↔ 1 byte), so the rest-of-line read from the wide buffer is correct; but a first token with a comment glued to it (`key#note`) is returned as `key#note` from the narrow mirror (with a separator before `#` it is unaffected) [C]. A line that is entirely a comment is correctly skipped because `TokenAvailable` looks at the cleaned wide buffer [C].

### 1.4 Tokens

* Separator set (default): space, TAB, LF, CR, `:` and `,` — `" \t\n\r:,"` for the narrow readers; the Unicode readers additionally treat form-feed `\f` as a separator (`L" \f\t\n\r:,"`) [C] `DEFAULT_SEPERATOR_CHARS` in both `.cpp` files. `SetSeperatorChars` (max 15 characters) can replace the set, but no caller in `code/` or `contrib/systemiv/lib` uses it (grep) — every format in this document uses the default set [C].
* `GetNextToken()`: skip separators; if end of line → `NULL`; else return the maximal run of non-separators and overwrite the following separator with NUL. `:` and `,` are therefore exactly equivalent to whitespace — `BuildingOnline :mine,90` is the three tokens `BuildingOnline`, `mine`, `90`.
* `GetRestOfLine()`: skip separators, return the remainder of the line **verbatim** (internal separators, trailing spaces and tabs preserved — there is no right-trim) or `NULL` at end of line. Because of the leading skip, a "rest of line" value can never begin with space, tab, `:` or `,`.
* `TokenAvailable()`: true iff a non-separator character remains on the line.
* There is **no quoting, no escaping and no line continuation** at this level. Individual formats add their own conventions (the language table's `\n`, §8).
* Case: the reader never changes case. Keyword comparisons are done by each parser (mostly `stricmp`, i.e. ASCII case-insensitive; exceptions flagged **(cs)**).

### 1.5 Formal summary

```
file          := [ 'redshirt2' ] { line }                 ; header only on the very first line (case-insensitive)
line          := { char - LF } ( LF | EOF )
cleaned_line  := prefix of line before the first '#', then before the first LF, then before the first CR
separator     := ' ' | TAB | LF | CR | ':' | ',' [ | FF  (UTF-16 readers only) ]
token         := 1*( char - separator )
token_line    := { separator } [ token { 1*separator token } ] { separator }
rest_of_line  := everything after the leading separators, verbatim
```

### 1.6 Encoding survey of the files in scope [C] `_work/scripts/X1a_text_survey.py` → `_work/drafts/X1a_encoding_survey.csv` (190 files)

| Group | Files | Encoding | Line ends / final newline |
|---|---|---|---|
| `data/levels/*.txt` | 87 | ASCII | CRLF; `level_enum.txt` lacks a final newline |
| `data/levels/old/*.txt` | 30 | ASCII | CRLF |
| `data/purity/*.txt` | 4 | ASCII | CRLF |
| `data/scripts/*.txt` | 42 | ASCII | CRLF |
| `data/input/*.txt`, `data/input/keyboards/*.txt` | 6 | ASCII | CRLF |
| `data/*.txt` in scope | 10 | ASCII, except `betatesters.txt` and `multiwinia_betatesters.txt` (Latin-1 bytes in accented names) | CRLF |
| `data/language/*.txt` | 9 | `english.txt` single-byte (2 Latin-1 bytes); French, German, Italian, Korean, Portuguese, Spanish **UTF-16BE + BOM**; Japanese, Russian **UTF-16LE + BOM** | CRLF; Portuguese mixes 217 LF-only lines; Japanese, Korean, Portuguese lack a final newline |
| root `testbedconfig.txt`, `MultiwiniaSupport.ini` | 2 | ASCII | CRLF; `testbedconfig.txt` lacks a final newline |

No file starts with `redshirt2`; no UTF-8 BOM is present (a UTF-8 BOM would *not* be recognised — it would become part of the first token). Latin-1 bytes pass through the narrow reader unchanged and are then widened byte-for-byte by `UnicodeString(char const*)` (each byte becomes the code unit of the same value, i.e. ISO-8859-1) [C] `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::UnicodeString(char const*)`. The reverse conversion (`CopyUnicodeToCharArray`) maps every code unit > 255 to `?` [C].

### 1.7 Re-implementation notes

* Implement one tokeniser with: BOM sniffing (UTF-16LE/BE, else bytes-as-Latin-1), optional `redshirt2` de-obfuscation (only needed to read legacy saves), `#` comments, first-CR truncation, the separator set above, `next_token`, `rest_of_line`, `token_available`, a per-reader separator override, and EOF semantics that *do* process an unterminated last line (fixing the archive asymmetry).
* Keep the `F32` rounding helper for numeric fields that feed simulation.

<a id="fmt-level-file"></a>
## [→ 29_data_formats_reference] [→ 08_levels_and_level_design] 2. Level files (`map_*.txt`, `mp_*.txt`, `mission_*.txt`)

### 2.1 Purpose, files, build reality

A level is described by **one map file** (terrain, lights, static buildings, Multiwinia options — and, in Multiwinia, also units/camera data) plus an optional **mission file** (dynamic buildings, units, objectives, camera data; Darwinia campaign only). `LevelFile(missionName, mapName, textFileLevel)` parses the mission file first, then the map file [C] `code/level_file.cpp:LevelFile::LevelFile`. `Location::LoadLevel` passes `textFileLevel = (extension of map name == "txt")`; any other extension is loaded as a `.mwm` container (§13.6) [C] `code/location.cpp:Location::LoadLevel`. In Multiwinia `m_requestedMission` is `"null"` (set in `App::Initialise`, again when the client receives the map name from the server, and by the editor's map window) so only the map file is read [C] `code/app.cpp:App::Initialise`, `code/network/clienttoserver.cpp` (L2618), `code/interface/multiwinia_window.cpp`.

| File set | Count | Kind | Loaded by | Build tag |
|---|---|---|---|---|
| `data/levels/mp_*.txt` | 57 | map | Multiwinia map list (`GameMenu::CreateMapList`, filter `mp_*`) and `LevelFile` | [B:FULL]; 8 of them (`mp_assault_stress`, `mp_benchmark_1`, `mp_sandbox`, `mp_sandbox_all`, `mp_tankbattle_2P/3P/4P`, `mp_cts_3p_3`) are excluded from the retail `main.dat` by `tools/multiwinia_exclusions.txt`, leaving **49 shipped maps** (= `VALID_ACHIEVEMENT_LEVELS` 49 in `code/achievement_tracker.cpp`) [C] |
| `data/levels/map_*.txt` | 12 | map | Darwinia campaign via `game.txt` locations | [B:LEGACY]; excluded from retail (`levels/map_*`) [C] `tools/multiwinia_exclusions.txt` |
| `data/levels/mission_*.txt` | 17 | mission | Darwinia campaign | [B:LEGACY]; excluded (`levels/mission_*`) |
| `data/levels/old/*.txt` | 30 | map (`mp_*`) | nothing: `ListResources("levels/","mp_*")` matches only the exact directory `data/levels/` [C] `code/lib/resource.cpp:Resource::ListResources` (`stricmp(dirPart, desiredDir)`) | [B:DEAD] data; excluded (`levels/old/*`) |
| `data/purity/map_mine.txt`, `mission_mine_enable.txt` | 2 | map / mission | nothing in code (`data/purity/` is never opened) | [B:DEAD] data; excluded by every exclusion list (`purity`) |

Encoding: single-byte ASCII, CRLF, every file terminated by a newline [C] survey. Read through `Resource::GetTextReader("levels/<name>")`; if not found, the map is also tried as `<profile>/maps/<name>` with a plain `TextFileReader` [C] `LevelFile::ParseMapFile(char const*)`.

### 2.2 Top-level grammar and block dispatch

```
level_file   := { blank_line | block }
block        := START_LINE { body_line } END_LINE
START_LINE   := BlockName '_StartDefinition' [ ignored tokens ]      ; first token, case-insensitive
END_LINE     := BlockName '_EndDefinition'                          ; first token of the line, case-insensitive
blank_line   := line with no token after cleaning (§1)
```

The dispatcher reads the first token of each non-blank line and, if it matches a known start keyword, hands the reader to that block's parser, which consumes lines up to and including its own end keyword. Every block parser has its own body grammar (below). Anything else at top level hits `AppDebugAssert(0)` — a no-op in release — and the line is skipped [C] `LevelFile::ParseMapFile(TextReader*)`, `LevelFile::ParseMissionFile`. A block whose end keyword is missing swallows the rest of the file.

| Block (`X_StartDefinition`) | Allowed in map file | Allowed in mission file | Parser | Notes |
|---|---|---|---|---|
| `MultiwiniaOptions` | yes | — (asserts, skipped) | `ParseMultiwiniaOptions` | Multiwinia-only; must be first so the menu scanner can stop early (§2.4.3) |
| `Landscape` | yes | asserts | `ParseLandscapeData` | |
| `LandscapeTiles` | yes | asserts | `ParseLandscapeTiles` | |
| `LandFlattenAreas` | yes | asserts | `ParseLandFlattenAreas` | |
| `Lights` | yes | asserts | `ParseLights` | first non-empty Lights block wins (see §2.8) |
| `Buildings` | yes → **static** (`m_dynamic=false`) | yes → **dynamic** (`m_dynamic=true`) | `ParseBuildings(in, dynamic)` | `ParseMapFile` contains a second, unreachable `Buildings` branch with `dynamic=true`; a map file's *second* Buildings block is therefore also static [C] |
| `CameraMounts` | yes | yes | `ParseCameraMounts` | |
| `CameraAnimations` | yes | yes | `ParseCameraAnims` | |
| `InstantUnits` | yes | yes | `ParseInstantUnits` | |
| `Routes` | yes | yes | `ParseRoutes` | no shipped file uses it |
| `PrimaryObjectives` | yes | yes | `ParsePrimaryObjectives` | |
| `RunningPrograms` | — | yes | `ParseRunningPrograms` | save-game only; no shipped file |
| `Difficulty` | — | yes | `ParseDifficulty` | |
| `LeaderboardStats` | — | yes | `ParseLeaderboardStats` | save-game only; no shipped file |

"Asserts" = `AppDebugAssert(0)` then the block body lines are re-read as top-level lines (each one also asserting) — in release the block is effectively ignored.

Canonical block order written by `LevelFile::SaveMapFile` (the editor/save path): `MultiwiniaOptions` (only when `g_app->m_multiwinia` exists), `Landscape`, `LandscapeTiles`, `LandFlattenAreas`, `Lights`, `Buildings`(static), then (Multiwinia only) `CameraMounts`, `InstantUnits` [C] `LevelFile::SaveMapFile`. Shipped `mp_*` maps follow one of two orders: 47 files `MultiwiniaOptions > Landscape > LandscapeTiles > LandFlattenAreas > Lights > Buildings > CameraMounts > CameraAnimations > Buildings > InstantUnits > PrimaryObjectives`, 10 files `… > Buildings > CameraMounts > InstantUnits` [C] `X1a_level_survey.py`. Mission files are written as `Difficulty, CameraMounts, CameraAnimations, Buildings(dynamic), InstantUnits, Routes, PrimaryObjectives, RunningPrograms, LeaderboardStats` [C] `LevelFile::SaveMissionFile`.

### 2.3 Object defaults before parsing [C] `LevelFile::LevelFile`, `code/level_file.h:LandscapeDef`, `LandscapeFlattenArea`, `InstantUnit`, `CamAnimNode`

| Member | Default | Member | Default |
|---|---|---|---|
| `m_landscapeColourFilename` | `landscape_default.bmp` | `m_levelName` | `NewLevel` |
| `m_wavesColourFilename` | `waves_default.bmp` | `m_description` | `LevelDescription` |
| `m_waterColourFilename` | `water_default.bmp` | `m_author` | `Author` |
| `m_thumbnailFilename` | `default.bmp` (no file keyword sets it; `.mwm` only) | `m_difficultyRating` | NULL |
| `m_levelDifficulty` | −1 | `m_gameTypes` | −1 |
| `m_populationCap`, `m_defenderPopulationCap`, `m_numPlayers`, `m_defaultPlayTime` | −1 | `m_coop`, `m_forceCoop`, `m_official` | false |
| `m_levelOptions[0..14]` | −1, then options 0, 1, 4, 9, 10 forced to 0 | `m_effects[0..4]`, `m_invalidCrates[*]`, `m_attacking[*]` | false |
| `m_mapId` | −1 | `m_leaderboardLevelTime` / `m_lastLeaderboardLevelTime` | 0.0 / 9 999 999.0 |
| `LandscapeDef.m_cellSize` | 12.0 | `LandscapeDef.m_worldSizeX/Z` | 2000 / 2000 |
| `LandscapeDef.m_outsideHeight` | −10 | `LandscapeDef.m_maxHeight` | 0.0 |

### 2.4 `MultiwiniaOptions` block

#### 2.4.1 Grammar

```
mw_options  := 'MultiwiniaOptions_StartDefinition' EOL { mw_line } 'MultiwiniaOptions_EndDefinition' EOL
mw_line     := blank
             | 'PopulationCap' INT | 'DefenderPopulationCap' INT | 'NumPlayers' INT | 'PlayTime' INT
             | 'GameTypes' GAMETYPE { GAMETYPE }                    ; LevelFile keeps only the first
             | 'Effects' { EFFECT }                                 ; EFFECT compared case-sensitively
             | 'LevelOptions' { level_option }
             | 'InvalidCrates' { CRATENAME }
             | 'AttackingTeams' { INT }
             | 'CoopGroup1Positions' { INT } | 'CoopGroup2Positions' { INT }
             | 'CoopMode' | 'ForceCoop' | 'OfficialLevel'
             | 'Name' REST | 'Author' REST | 'Description' REST
             | 'Difficulty' WORD
             | OTHER_WORD { any }                                   ; silently ignored
level_option:= 'nohandicap' | 'instantspawnpointcapture' | 'nogunturrets' | 'forceradarteammatch' | 'noradarcontrol'
             | ('trunkportreinforcements' | 'trunkportarmour' | 'attackerreinforcements' | 'defenderreinforcements'
                | 'airstrikecapturezones' | 'futurewinianteam' | 'attackreinforcementoption' | 'defendreinforcementoption') INT
             | ('defenderreinforcementtype' | 'attackerreinforcementtype') RESEARCHNAME
             | OTHER_WORD                                           ; silently ignored (no argument consumed)
GAMETYPE    := 'Domination' | 'KingOfTheHill' | 'CaptureTheStatue' | 'Assault' | 'RocketRiot' | 'Blitzkrieg' | 'TankBattle'   (cs)
EFFECT      := 'snow' | 'dustwind' | 'soulrain' | 'lightning' | 'SpecialLighting'                                         (cs)
```

Keywords (first tokens and `level_option` words) are case-insensitive in `LevelFile` (`stricmp`) but **case-sensitive in the menu scanner** (`GameMenu::AddMap`, `strcmp`) — always write them exactly as above. `GAMETYPE` must equal a `GAMETYPE` name from `multiwinia.txt` (§4) — `strcmp`, case-sensitive, in both parsers. `MISSING ⇒ CRASH` applies to every `INT`/`WORD`/`RESEARCHNAME` argument above (no `TokenAvailable` check) except list-valued keywords. Keywords may repeat; the last occurrence wins for scalars (`mp_cts_4p_3.txt` has `PopulationCap -1` then `PopulationCap 2000` → 2000) and lists accumulate.

#### 2.4.2 Field table [C] `LevelFile::ParseMultiwiniaOptions` unless stated

| Keyword | Value type | Units | Default (absent) | Range / valid values | Semantics (consumer) |
|---|---|---|---|---|---|
| `PopulationCap` | INT | Darwinians | −1 = no map cap | ≥1 or −1; shipped −1…2500 | total Darwinian cap shared by the teams in play: each team's cap = `cap / numTeams` [C] `code/worldobject/spawnpoint.cpp:SpawnPoint` (L732–L744) |
| `DefenderPopulationCap` | INT | Darwinians | −1 | −1 or ≥1; shipped −1, 200, 250, 275, 350 | Assault defender cap [C] `spawnpoint.cpp` L717, `code/worldobject/trunkport.cpp` L362 |
| `NumPlayers` | INT | teams | −1 | 2–4 in shipped maps (`NumPlayers` absent only in `mp_sandbox_all.txt`) | number of start positions; lobby uses it; Assault rotation counts [C] `code/multiwinia.cpp` L1138, L1777; `GameMenu::AddMap` |
| `PlayTime` | INT | minutes | −1 | shipped 5, 10, 15, 20 | stored as `MapData::m_playTime` [C] `GameMenu::AddMap`; used by the lobby as the suggested match length [I] (consumer owned by P18) |
| `GameTypes` | GAMETYPE | – | −1 (map rejected by menu) | names of `multiwinia.txt` | `LevelFile`: index of the **first** matching blueprint → `m_gameTypes`; menu: **every** matching token adds the map to that game type's list; a map with no valid token is discarded ("didn't contain a game type") [C] `GameMenu::AddMap` |
| `Effects` | EFFECT* | – | none | `snow`→Snow, `dustwind`→DustStorm, `soulrain`→SoulRain, `lightning`→Lightning, `SpecialLighting`→SpecialLighting | `Location::Advance` runs `AdvanceChristmas/AdvanceSoulRain/AdvanceDustWind/AdvanceLightning`; SpecialLighting switches the lighting mode [C] `code/location.cpp` L1388–L1391, L2008 |
| `LevelOptions` | option list | – | see §2.3 | §2.4.4 | |
| `InvalidCrates` | crate names | – | none | names of `Crate::GetName` (case-insensitive) | crate reward excluded on this map [C] `code/worldobject/crate.cpp` L274 |
| `AttackingTeams` | INT* | team **position** index | none | 0…`NUM_TEAMS`−1 (6); **no bounds check** — out-of-range writes past `m_attacking[6]` | Assault: positions that attack [C] `code/multiwinia.cpp` L1037, L1087 |
| `CoopGroup1Positions`, `CoopGroup2Positions` | INT* | position index | empty | ≥2 entries expected by `Location` (it reads `[0]` and `[1]`) | co-op pairing [C] `code/location.cpp` L2667–L3208 |
| `CoopMode` | flag | – | false | – | co-op allowed (menu `m_coop`) |
| `ForceCoop` | flag | – | false | – | co-op mandatory |
| `OfficialLevel` | flag | – | false | – | `m_official = true`, `m_mapId = 0`; the menu counts official maps for achievements [C] `GameMenu::CreateMapList` |
| `Name` | REST (verbatim, trailing blanks kept) | – | `NewLevel` | ≤511 bytes (`strcpy` into `char[512]`, no check) | level display name; shipped files that have it all say `NewLevel` (10 files) — real names come from the language table [I] |
| `Author` | REST | – | `Author` | ≤63 bytes (`char[64]`, **no length check**) | unused by shipped maps |
| `Description` | REST | – | `LevelDescription` | ≤511 bytes | unused by shipped maps |
| `Difficulty` | WORD | – | NULL | shipped `basic`, `intermediate`, `advanced`; the menu sorts by substring `basic`<`inter`<`advan`, anything else = 0 | lobby sort & display [C] `GameMenu::CreateMapList` |

#### 2.4.3 Menu scanner (second, independent parser) [C] `code/game_menu.cpp:GameMenu::AddMap`, `GameMenu::CreateMapList`

The main menu builds its map list by reading **every** `data/levels/mp_*` file with extension `txt` (loose files and archive entries; `.mwm` files from `<profile>/maps/` only under `LOCATION_EDITOR`), tokenising each line with the default separators and testing the first token with case-sensitive `strcmp` against `Name`, `GameTypes`, `NumPlayers`, `Author`, `Description`, `PlayTime`, `Difficulty`, `CoopMode`, `ForceCoop`, `OfficialLevel`, stopping at the first `MultiwiniaOptions_EndDefinition`. It does **not** check that it is inside the options block — a file without the block is scanned to the end. It then computes the map CRC (`MapData::CalculeMapId`, see 31 MapCRC) and sorts each game-type list by `NumPlayers`, then difficulty. Consequence for authors: keep the options block first and spell keywords exactly.

#### 2.4.4 `LevelOptions` sub-options (`LevelFile::NumLevelOptions` = 15) [C] `code/level_file.h` enum, `ParseMultiwiniaOptions`

| # | Word | Arg | Default | Shipped values (files) | Consumer / semantics |
|---|---|---|---|---|---|
| 0 | `nohandicap` | – | 0 | 1 file (old/) | handicap multiplier forced to 1.0 [C] `spawnpoint.cpp` L1171 |
| 1 | `instantspawnpointcapture` | – | 0 | 5 | spawn points capture instantly [C] `spawnpoint.cpp` L984 |
| 2 | `trunkportreinforcements` | INT (Darwinians per wave) | −1 | 60, 75, 100, 250 | Rocket-Riot-style trunk-port reinforcements [C] `code/multiwinia.cpp` L987, L2466 |
| 3 | `trunkportarmour` | INT (max armour) | −1 | 1, 3 | trunk-port armour spawns [C] `multiwinia.cpp` L1003; `crate.cpp` L226 |
| 4 | `nogunturrets` | – | 0 | 2 | no GunTurret reinforcement tasks [C] `multiwinia.cpp` L1006 |
| 5 | `attackerreinforcements` | INT | −1 | none | Assault attacker reinforcement count [C] `multiwinia.cpp` L991 |
| 6 | `defenderreinforcements` | INT | −1 | none | Assault defender reinforcement count [C] L990 |
| 7 | `defenderreinforcementtype` | research name → `GlobalResearch::GetType` (−1 if unknown) | −1 | `AirStrike`, `Armour`, `DropShip`(old/) | defender reinforcement task type [C] L998 |
| 8 | `attackerreinforcementtype` | research name | −1 | `HotFeet`, `AirStrike`(old/) | attacker reinforcement task type [C] L999 |
| 9 | `forceradarteammatch` | – | 0 | 1 | two radar dishes connect only if same owner [C] `code/worldobject/radardish.cpp` L100 |
| 10 | `noradarcontrol` | – | 0 | 1 | radar dishes cannot be aimed [C] `code/location_input.cpp` L263, `code/worldobject/ai.cpp` L981, `code/gamecursor.cpp` L1518 |
| 11 | `airstrikecapturezones` | INT (zones) | −1 | 3 | Blitzkrieg: zones needed before turrets become airstrikes [C] `multiwinia.cpp` L1119 |
| 12 | `futurewinianteam` | INT (team position) | −1 | 2 (old/) | that position's Darwinians are Futurewinians [C] `code/team.cpp` L1147, `code/worldobject/darwinian.cpp` L112 |
| 13 | `attackreinforcementoption` | INT | −1 | 1, 2 (old/) | `task->m_option` for attacker reinforcement tasks [C] `multiwinia.cpp` L1170 |
| 14 | `defendreinforcementoption` | INT | −1 | none | `task->m_option` for defender tasks [C] L1148 |

Writer quirk: `WriteMultiwiniaOptions` emits only options whose value is **> 0**, and the `futurewinianteam` case falls through into `attackreinforcementoption` (missing `break`), so a saved map with a Futurewinian team also gains a spurious `attackreinforcementoption <same value>` [C] `LevelFile::WriteMultiwiniaOptions`. Research-type options with value 0 (`Darwinian`) can never be saved.

### 2.5 `Landscape` block [C] `LevelFile::ParseLandscapeData`, `WriteLandscapeData`

```
landscape   := 'Landscape_StartDefinition' EOL { ls_line } 'Landscape_EndDefinition' EOL
ls_line     := KEY VALUE [ ignored ]                 ; KEY case-insensitive; unknown KEY ignored
```

**No blank or comment-only lines are allowed inside this block**: the parser calls `GetNextToken()` without `TokenAvailable()` and passes the `NULL` to `stricmp` (MISSING ⇒ CRASH). A known KEY without a value passes `NULL` to `atoi`/`iv_atof`/`strcpy` (crash). No shipped file violates this [C] survey.

| Key | Type | Units | Default | Shipped range / values | Semantics |
|---|---|---|---|---|---|
| `worldSizeX` | INT (`atoi`) | world units (wu) | 2000 | all 100 map files | landscape extent along x |
| `worldSizeZ` | INT | wu | 2000 | all | extent along z |
| `cellSize` | F32 | wu per height-map cell | 12.0 | all | height-map resolution |
| `outsideHeight` | F32 | wu | −10 | all | height outside the tiles (sea floor) |
| `maxHeight` | `atof` → float (not F32-rounded, stored as `float`) | wu | 0.0 | 41 mp files | upper height clamp; 0 = none [I: see 23] |
| `landColourFile` | WORD | file in `data/terrain/` | `landscape_default.bmp` | all | height→colour ramp bitmap |
| `wavesColourFile` | WORD | file in `data/terrain/` | `waves_default.bmp` | all | wave colour ramp |
| `waterColourFile` | WORD | file in `data/terrain/` | `water_default.bmp` | all | water colour ramp |

Algorithmic meaning of every landscape field: [→ 23_terrain_and_landscape] (P11 §23.3).

### 2.6 `LandscapeTiles` block [C] `LevelFile::ParseLandscapeTiles`, `WriteLandscapeTiles`, `code/landscape.cpp:LandscapeTile::GuideGridSetPower`, `GuideGridFromString`, `GuideGridToString`

```
tiles       := 'LandscapeTiles_StartDefinition' EOL { blank | tile_row } 'landscapeTiles_endDefinition' EOL
tile_row    := posX posY posZ size fractalDim heightScale desiredHeight genMethod seed lowlandSmoothing guidePower [ guideGrid ]
guideGrid   := { HEXPAIR }                      ; exactly 2·(2^guidePower − 1)^2 characters, present iff guidePower > 0
HEXPAIR     := LETTER LETTER                    ; LETTER ∈ 'A'..'P' (value = c − 'A', 0..15); cell = 16·hi + lo (0..255)
```

| Col | Field | Type | Units | Notes |
|---|---|---|---|---|
| 1 | `m_posX` | INT (`atoi` — decimals truncated) | wu | tile origin x |
| 2 | `m_posY` | F32 | wu | vertical offset |
| 3 | `m_posZ` | INT | wu | tile origin z |
| 4 | `m_size` | INT | wu | tile edge length |
| 5 | `m_fractalDimension` | F32 | – | roughness |
| 6 | `m_heightScale` | F32 | – | |
| 7 | `m_desiredHeight` | F32 | wu | |
| 8 | `m_generationMethod` | INT | enum | 0/1 in shipped maps |
| 9 | `m_randomSeed` | INT | – | deterministic generator seed |
| 10 | `m_lowlandSmoothingFactor` | F32 | – | |
| 11 | `guidePower` | INT | – | 0…5 in shipped files; grid resolution `2^p − 1` cells per side (p=0 → no grid) |
| 12 | guide grid | WORD | – | row-major, `z` outer, `x` inner; **no length check** — a short string reads past the token (UB) |

All 11 leading columns are MISSING ⇒ CRASH. Shipped tile rows: 209 with p=0 (11 tokens), 41/292/40/101/46 with p=1…5 (12 tokens) — all exact [C] survey. Semantics: [→ 23] P11 §23.4–§23.5.

### 2.7 `LandFlattenAreas` block [C] `LevelFile::ParseLandFlattenAreas`, `WriteLandFlattenAreas`

```
flatten_row := centreX centreY centreZ size [ flattenType heightThreshold ]
```

| Col | Field | Type | Units | Default | Range |
|---|---|---|---|---|---|
| 1–3 | `m_centre` | F32 ×3 | wu | – | – |
| 4 | `m_size` | F32 | wu (radius) [I] | 40.0 (ctor) | – |
| 5 | `m_flattenType` | INT | enum `FlattenTypeAbsolute`=0, `Add`=1, `Subtract`=2, `Smooth`=3, `Subtract2`=4 | 0 when the column is absent | 0–4 |
| 6 | `m_heightThreshold` | F32 | wu | 40.0 (ctor) | read only when col 5 present (then MISSING ⇒ CRASH) |

Shipped: 14 rows with 4 tokens (old campaign-era maps), 54 rows with 6 tokens [C] survey. Semantics: [→ 23] §23.7.

### 2.8 `Lights` block [C] `LevelFile::ParseLights`, `WriteLights`

```
light_row := frontX frontY frontZ red green blue          ; all F32
```

`m_front = (x, y, z, 0)` then `Light::Normalise()` (infinitely distant directional light); `m_colour = (r, g, b, 0)` — colour components are unclamped multipliers (shipped values reach 2.00). Precedence: if lights were already loaded when a `Lights` block is met, **that whole block is ignored** [C] `ParseLights` (`ignoreLights`). The in-code comment says this lets a mission file override map lights, but `ParseMissionFile` rejects `Lights` blocks (`AppDebugAssert(0)`), so in practice only a second `Lights` block in the same map file is affected [C]. Every shipped map has exactly one block with 1–2 lights (198 rows / 100 files) [C] survey. The writer dumps `g_app->m_location->m_lights`, i.e. the *current* lights, with `%6.2f` precision [C] `WriteLights`.

### 2.9 `Buildings` block [C] `LevelFile::ParseBuildings`, `code/worldobject/building.cpp:Building::CreateBuilding(char*)`, `Building::Read`, `Building::BuildingIsBlocked`, every subclass `Read`

#### 2.9.1 Grammar and load rules

```
buildings   := 'Buildings_StartDefinition' EOL { blank | b_row } 'Buildings_EndDefinition' EOL
b_row       := TYPENAME id x z team frontX frontZ isGlobal { extra }
```

1. `TYPENAME` is compared case-insensitively with `Building::GetTypeName(i)` for i = 0…74 (§2.9.3). If it matches a type for which `BuildingIsBlocked(i)` is true (`BLOCK_OLD_DARWINIA_OBJECTS`, defined in FULL — 39 types) or no type at all, `CreateBuilding` returns NULL and **the whole line is ignored** (no error). `Invalid` (type 0) also yields NULL.
2. Base columns (`Building::Read`, all MISSING ⇒ CRASH):

| Col | Field | Type | Units / range | Semantics |
|---|---|---|---|---|
| 2 | `id` | INT | 0…; must be unique among all buildings of this `LevelFile` (map + mission), else `AppReleaseAssert("%s UniqueId was not unique in %s")` — **fatal in release** | `WorldObjectId(team, UNIT_BUILDINGS=−100, −1, id)`; referenced by every link column |
| 3 | `x` | F32 | wu | position x (y is taken from the landscape at init [I]) |
| 4 | `z` | F32 | wu | position z |
| 5 | `team` | INT, stored as `unsigned char` | 0…5 (`NUM_TEAMS`=6, `code/globals.h`); shipped values 0 (747 rows), 1 (1899), 2 (352), 3 (119), 255 (5325); 255 = no team (e.g. `Location::IsFriend` returns false for 255, `code/location.cpp` L2615) [C] | owner |
| 6 | `frontX` | F32 | – | facing; `m_front = (frontX, 0, frontZ)` normalised (ctor default front (1,0,0) supplies y = 0) |
| 7 | `frontZ` | F32 | – | |
| 8 | `isGlobal` | INT → bool | 0/1 | campaign-global building (tracked by `GlobalWorld`). **Must be 1** for `TrunkPort`, `ControlTower`, `RadarDish`, `Incubator`, `FenceSwitch`, else `AppReleaseAssert("Non-global %s found in %s")` — fatal in release. All shipped rows comply (173/150/164/96/0 rows) [C] survey |

3. Extra columns are read by the subclass (§2.9.3). Columns beyond those a class reads are ignored; required columns are MISSING ⇒ CRASH.
4. Dynamic flag: rows from a mission file get `m_dynamic = true`, rows from a map file `false`.
5. Difficulty scaling (Darwinia legacy): for team 1 buildings, `SpawnPopulationLock.maxPopulation ×= f`, `AntHill.numAntsInside ×= f`, `AISpawnPoint.period /= f`, `Triffid.reloadTime /= f`, with `f = 1 + difficultyLevel/5` when the mission has no `Difficulty` block. `App::UpdateDifficultyFromPreferences` hard-codes `m_difficultyLevel = 0`, so `f = 1.0` in every build [C] `code/app.cpp:App::UpdateDifficultyFromPreferences`, `LevelFile::ParseBuildings`.
6. Determinism note: every row constructs a template object; some constructors draw from the synchronised RNG (P06 §Determinism), so the **set and order of rows must be identical on every peer** [→ 22].

#### 2.9.2 Building templates → world objects

The parsed objects are *templates*; `Location` later creates the live building with `CreateBuilding(type)` + `Initialise(template)`. Field semantics and behaviour are specified in [→ 03_buildings_and_world_objects] (P06, P07, P08, P02 sections). This table only fixes the syntax.

#### 2.9.3 Per-type extra columns (all 75 type names: `Invalid` + 74 real types) [C] each class's `Read`; class hierarchy from `code/worldobject/*.h`

Legend: `R` required (MISSING ⇒ CRASH), `O` optional (read only if a token remains; nested — a later O is read only if the earlier one was present), `*` = list until end of line. "Blocked" rows are ignored in every build in scope.

| # | Type name | Class (Read) | Extra columns after `isGlobal` | Optional defaults | Blocked | Shipped rows (files) |
|---|---|---|---|---|---|---|
| 0 | `Invalid` | – | – (CreateBuilding returns NULL) | – | n/a | 0 |
| 1 | `Factory` | Factory | R `initialCapacity` INT | – | yes | 0 |
| 2 | `Cave` | Cave (base) | – | – | yes | 0 |
| 3 | `RadarDish` | RadarDish | O `forceConnection` INT, O `autoConnectAtStart` INT, `validLinkId`* INT | 0, 0, empty | no | 164 (22) |
| 4 | `LaserFence` | LaserFence | R `nextLaserFenceId` INT, O `scale` F32, O `mode` F32→int (`LaserFence::Mode*`: 0 Disabled, 1 Enabling, 2 Enabled, 3 Disabling, 4 NeverOn, 5 AlwaysOn) | 1.0, 0 (Disabled) | no | 764 (17) |
| 5 | `ControlTower` | ControlTower | R `controlBuildingId` INT | – | no | 150 (20) |
| 6 | `GunTurret` | GunTurret | O `state` INT (= turret type: 0 Standard, 1 Rocket, 2 Mortar, 3 Flame, 4 Subversion), O group {`targetX` F32, `targetZ` F32, `targetRadius` F32, `targetForce` F32} (all four R once `targetX` is present) | 0, (0,0,0,0) | no | 264 (23) |
| 7 | `Bridge` | Bridge | R `nextBridgeId` INT, R `bridgeType` INT | – | yes | 0 |
| 8 | `Powerstation` | Powerstation | R `linkedBuildingId` INT | – | yes | 0 |
| 9 | `Tree` | Tree | R `height` F32, R `budsize` F32, R `pushOut` F32, R `pushUp` F32, R `iterations` INT, R `seed` INT, R `branchColour` INT (packed RGBA), R `leafColour` INT (packed RGBA), O `leafDropRate` INT, O `spiritDropRate` INT, O `evil` INT (==1 → true) | 0, 0, false | no | 1041 (69) |
| 10 | `Wall` | Wall | O `objectiveLink` INT | −1 | no | 374 (12) |
| 11 | `TrunkPort` | TrunkPort | O `targetLocationId` INT | −1 | no | 173 (49) |
| 12 | `ResearchItem` | ResearchItem | R `researchType` research name (`GlobalResearch::GetType`, case-insensitive; unknown → −1), O `level` INT | 1 | yes | 8 (6) — ignored |
| 13 | `Library` | Library (base) | – | – | yes | 0 |
| 14 | `Generator` | PowerBuilding | R `powerLink` INT | – | yes | 1 (1) — ignored |
| 15 | `Pylon` | PowerBuilding | R `powerLink` INT | – | no | 389 (22) |
| 16 | `PylonStart` | PylonStart | R `powerLink` INT, R `reqBuildingId` INT | – | yes | 1 (1) — ignored |
| 17 | `PylonEnd` | PowerBuilding | R `powerLink` INT | – | no | 98 (21) |
| 18 | `SolarPanel` | PowerBuilding | R `powerLink` INT | – | no | 274 (21) |
| 19 | `PowerSplitter` | PowerSplitter | R `powerLink` INT, `linkId`* INT | empty | no | 27 (8) |
| 20 | `TrackLink` | MineBuilding | R `trackLink` INT | – | yes | 111 (2) — ignored |
| 21 | `TrackJunction` | TrackJunction | R `trackLink` INT, `trackLinks`* INT | – | yes | 3 (1) — ignored |
| 22 | `TrackStart` | TrackStart | R `trackLink` INT, R `reqBuildingId` INT | – | yes | 1 (1) — ignored |
| 23 | `TrackEnd` | TrackEnd | R `trackLink` INT, R `reqBuildingId` INT | – | yes | 2 (2) — ignored |
| 24 | `Refinery` | MineBuilding | R `trackLink` INT | – | yes | 1 (1) — ignored |
| 25 | `Mine` | MineBuilding | R `trackLink` INT | – | yes | 4 (1) — ignored |
| 26 | `Yard` | ConstructionYard (base) | – | – | yes | 1 (1) — ignored |
| 27 | `DisplayScreen` | DisplayScreen (base) | – | – | yes | 0 |
| 28 | `UpgradePort` | UpgradePort (base) | – | – | yes | 0 |
| 29 | `PrimaryUpgrade` | PrimaryUpgradePort (base) | – | – | yes | 0 |
| 30 | `Incubator` | Incubator | O `numStartingSpirits` INT | 0 | no | 96 (30) |
| 31 | `AntHill` | AntHill | R `numAntsInside` INT (values <0 or >10 000 are logged and replaced by 0) | – | no | 13 (2) |
| 32 | `SafeArea` | SafeArea | R `size` F32, R `entitiesRequired` INT, R `entityTypeRequired` INT (entity enum) | – | yes | 4 (3) — ignored |
| 33 | `Triffid` | Triffid | R `size` F32, R `pitch` F32, R `force` F32, R `variance` F32, R `reloadTime` F32, R `useTrigger` INT, R `triggerX` F32, R `triggerZ` F32, R `triggerRadius` F32, then up to 10 O spawn flags INT (==1 → true) in order Virii, Centipede, Spider, Spirits, Eggs, TriffidEggs, Ants, SporeGenerator, SoulDestroyer, Darwinians | missing flags = false | no | 47 (13) |
| 34 | `SpiritReceiver` | ReceiverBuilding | R `spiritLink` INT | – | yes | 49 (1) — ignored |
| 35 | `ReceiverLink` | ReceiverBuilding | R `spiritLink` INT | – | yes | 83 (2) — ignored |
| 36 | `SpiritSpawner` | ReceiverBuilding | R `spiritLink` INT | – | yes | 1 (1) — ignored |
| 37 | `SpiritProcessor` | ReceiverBuilding | R `spiritLink` INT | – | yes | 1 (1) — ignored |
| 38 | `SpawnPoint` | SpawnBuilding | `link`* INT (`SetBuildingLink` each) | empty | no | 471 (55) |
| 39 | `SpawnPopulationLock` | SpawnPopulationLock | R `searchRadius` F32 (wu), R `maxPopulation` INT | – | no | 166 (39) |
| 40 | `SpawnPointMaster` | MasterSpawnPoint (SpawnBuilding) | `link`* INT | empty | no | 55 (55) |
| 41 | `SpawnLink` | SpawnBuilding | `link`* INT | empty | no | 464 (37) |
| 42 | `AITarget` | AITarget | O `linkId` INT, O `priorityModifier` `atof` (full double, not F32), O `checkCliffs` INT | −1, 0.0, 1 | no | 1577 (81) |
| 43 | `AISpawnPoint` | AISpawnPoint | R `activatorId` INT, R `entityType` INT (entity enum), R `count` INT, R `period` INT, O `spawnLimit` INT, O `routeId` INT | −1, −1 | **yes** (in the block list although Darwinia-only) | 19 (6) — ignored |
| 44 | `BlueprintStore` | BlueprintBuilding | R `buildingLink` INT | – | yes | 1 (1) — ignored |
| 45 | `BlueprintConsole` | BlueprintConsole | R `buildingLink` INT, R `segment` INT | – | yes | 4 (1) — ignored |
| 46 | `BlueprintRelay` | BlueprintRelay | R `buildingLink` INT, R `altitude` F32 | – | yes | 12 (1) — ignored |
| 47 | `ScriptTrigger` | ScriptTrigger | R `linkId` INT, R `range` F32, R `scriptFilename` WORD (`strcpy`, ≤255), R `entityType` WORD: `always`, `never`, `camenter`, `camview` (case-insensitive) or an entity type name | – | yes | 25 (11) — ignored |
| 48 | `Spam` | Spam (base) | – | – | yes | 3 (1) — ignored |
| 49 | `GodDish` | GodDish (base) | – | – | yes | 2 (2) — ignored |
| 50 | `StaticShape` | StaticShape | R `scale` F32, R `shapeName` WORD (file in `data/shapes/`) | – | no | 257 (30) |
| 51 | `FuelGenerator` | FuelBuilding | R `fuelLink` INT | – | no | 64 (9) |
| 52 | `FuelPipe` | FuelBuilding | R `fuelLink` INT | – | no | 217 (9) |
| 53 | `FuelStation` | FuelBuilding | R `fuelLink` INT | – | no | 64 (9) |
| 54 | `EscapeRocket` | EscapeRocket | R `fuelLink` INT, R `fuel` F32, R `passengers` INT, R `spawnBuildingId` INT, R `spawnCompleted` INT | – | no | 22 (9) |
| 55 | `FenceSwitch` | FenceSwitch | R `linkedBuildingId` INT, R `linkedBuildingId2` INT, R `switchValue` INT (−1 → copy of `linkedBuildingId`), R `lockable` INT, R `script` WORD, O `locked` INT (==1) | false | yes | 0 |
| 56 | `DynamicHub` | DynamicHub | R `buildingLink` INT, R `shapeName` WORD, R `requiredScore` INT, R `currentScore` INT, R `minActiveLinks` INT | – | yes | 0 |
| 57 | `DynamicNode` | DynamicNode | R `buildingLink` INT, R `shapeName` WORD, R `scoreValue` INT, R `scoreSupplied` INT | – | yes | 0 |
| 58 | `FeedingTube` | FeedingTube | R `receiverId` INT | – | yes | 0 |
| 59 | `MultiwiniaZone` | MultiwiniaZone | R `size` `atof` (full double; radius in wu [I]), `blitzkriegLink`* INT | empty | no | 182 (29) |
| 60 | `ChessPiece` | ChessPiece (base) | – | – | yes | 0 |
| 61 | `ChessBase` | ChessBase | R `width` INT, R `depth` INT | – | yes | 2 (1) — ignored |
| 62 | `CloneLab` | CloneLab → Incubator | O `numStartingSpirits` INT | 0 | no | 15 (4, old/ only) |
| 63 | `ControlStation` | ControlStation | R `controlBuildingId` INT | – | no | 54 (13) |
| 64 | `Portal` | Portal (base) | – | – | yes | 91 (10, old/) — ignored |
| 65 | `Crate` | Crate (base) | – | – | yes (crates are spawned at runtime) | 0 |
| 66 | `Statue` | Statue (base) | – | – | yes | 1 (1) — ignored |
| 67 | `WallBuster` | WallBuster (base) | – | – | no | 3 (1) |
| 68 | `PulseBomb` | PulseBomb | R `startTime` F32 (s), `link`* INT (`SetBuildingLink`) | empty | no | 10 (10) |
| 69 | `RestrictionZone` | RestrictionZone | R `size` F32 (wu), O `blockPowerups` INT | false | no | 35 (14) |
| 70 | `JumpPad` | JumpPad | R `force` F32, R `angle` F32 (radians per P06) | – | no | 27 (4) |
| 71 | `AIObjective` | AIObjective | R `nextObjective` INT | – | no | 98 (10) |
| 72 | `AIObjectiveMarker` | AIObjectiveMarker | R `scanRange` F32 (wu), R `objectiveId` INT, O `armourObjective` INT, O `pickupOnly` INT | 0, 0 | no | 345 (17) |
| 73 | `EruptionMarker` | EruptionMarker (base) | – | – | no | 1 (1) |
| 74 | `SmokeMarker` | SmokeMarker (base) | – | – | no | 9 (2) |

(The enum has 75 entries 0…74 including `Invalid`; `NumBuildingTypes` = 75.) Row counts are over all 118 level files; per-file-set counts and token-count ranges are in `_work/drafts/X1a_level_field_usage.csv` [C] `X1a_level_survey.py`. Forced-global types at construction (`RadarDish`, `ControlTower`, `TrunkPort`, `Incubator`, `DynamicHub`, `FenceSwitch`) are overwritten by column 8 anyway [C] `Building::CreateBuilding(int)`.

Integer overflow note: `Tree` colour columns are `atoi`'d packed RGBA values; all shipped values fit in a signed 32-bit int (e.g. `1681873407`) [C] survey. A writer must emit them as signed decimal.

### 2.10 `CameraMounts` block [C] `LevelFile::ParseCameraMounts`, `WriteCameraMounts`

```
mount_row := NAME posX posY posZ frontX frontY frontZ upX upY upZ          ; 10 tokens, all MISSING ⇒ CRASH
```

| Col | Field | Type | Notes |
|---|---|---|---|
| 1 | `m_name` | WORD, `strncpy` 63 chars into `char[64]` | names of 63+ characters are **not NUL-terminated** (bug) — keep ≤ 62. Looked up case-insensitively (`LevelFile::GetCameraMount`) |
| 2–4 | `m_pos` | F32 ×3, wu | camera position |
| 5–7 | `m_front` | F32 ×3 | normalised after reading |
| 8–10 | `m_up` | F32 ×3 | normalised after reading |

Naming conventions consumed by code: `player<N>` (N = the local team's *position* index) is the start camera of that position in Multiwinia [C] `code/main.cpp` (L458, `sprintf(camname,"player%d", GetTeamPosition(myTeamId))`); `start` is the default start mount [C] `code/main.cpp` (L1344, `SetTarget("start")`). Shipped `mp_*` maps: `player0` 51, `player1` 51, `player2` 31, `player3` 13, `start` 7 rows [C] survey. The name `CamPosBefore` is reserved (`MAGIC_MOUNT_NAME_START_POS`, the pose before an animation started) and need not exist [C] `code/level_file.h`.

### 2.11 `CameraAnimations` block [C] `LevelFile::ParseCameraAnims`, `WriteCameraAnims`

```
cam_anims  := 'CameraAnimations_StartDefinition' EOL { blank | animation } 'CameraAnimations_EndDefinition' EOL
animation  := ANIMNAME EOL { blank | node } 'End' EOL                ; ANIMNAME strncpy 63 (same caveat as mounts)
node       := ('Move' | 'Cut') MOUNTNAME DURATION                   ; case-insensitive; all MISSING ⇒ CRASH
```

| Field | Type | Range | Failure |
|---|---|---|---|
| transition | `Move` = `CamAnimNode::TransitionMove` (0), `Cut` = `TransitionCut` (1) | – | unknown word → `AppReleaseAssert("Bad camera animation camera mode in level file %s")` — fatal in release |
| `m_mountName` | WORD (`strdup`) | must name a mount defined **earlier** (in this file or the mission file parsed before) unless it is `CamPosBefore` | else release-assert "Bad camera animation mount name" — fatal |
| `m_duration` | F32, seconds | 0.0 ≤ d < 60.0 | else release-assert "Bad camera animation transition time" — fatal |

Shipped files: 96 blocks, all empty (no animation in any file) [C] survey; consumed only by `Script` `CamAnim` (§9) [B:LEGACY].

### 2.12 `InstantUnits` block [C] `LevelFile::ParseInstantUnits`, `WriteInstantUnits`, `code/level_file.h:InstantUnit`

```
iu_row := ENTITYTYPE team x z count inAUnit state spread [ waypointX waypointZ ] [ routeId [ routeWaypointId [ runAsTask ] ] ]
```

| Col | Field | Type | Units | Default if absent | Notes |
|---|---|---|---|---|---|
| 1 | `m_type` | entity type name (case-insensitive, `Entity::GetTypeId`) | – | – | unknown name or `Entity::EntityIsBlocked` (FULL blocks `AI`, `ArmyAnt`, `DropShip`, `Egg`, `Lander`, `LaserTroop`, `Meteor`, `Nuke`, `Officer`, `Shaman`, `SpaceInvader`, `Tentacle`, `TriffidEgg`, `Tripod`, `InvalidType`, `SpaceShip`) → line skipped |
| 2 | `m_teamId` | INT | team id | R | |
| 3–4 | `m_posX`, `m_posZ` | F32 | wu | R | centre of the group |
| 5 | `m_number` | INT | entities | R | |
| 6 | `m_inAUnit` | INT → bool | – | R | `Squadie` with `inAUnit = 0` is skipped (legacy crash guard) |
| 7 | `m_state` | INT | entity state enum | R (ctor −1) | −1 = default state |
| 8 | `m_spread` | F32 | wu (radius) | R (ctor 200.0) | |
| 9–10 | `m_waypointX/Z` | F32 | wu | 0.0, 0.0 | the pair is entered when *one* token remains, so a single extra token ⇒ CRASH |
| 11 | `m_routeId` | INT | route id | −1 | |
| 12 | `m_routeWaypointId` | INT | – | −1 | |
| 13 | `m_runAsTask` | INT → bool | – | false | |

Accepted entity type names (index = `Entity::Type*`): 0 `InvalidType`, 1 `LaserTroop`, 2 `Engineer`, 3 `Virii`, 4 `Squadie` (= `TypeInsertionSquadie`), 5 `Egg`, 6 `SporeGenerator`, 7 `Lander`, 8 `Tripod`, 9 `Centipede`, 10 `SpaceInvader`, 11 `Spider`, 12 `Darwinian`, 13 `Officer`, 14 `ArmyAnt`, 15 `Armour`, 16 `SoulDestroyer`, 17 `TriffidEgg`, 18 `AI`, 19 `Shaman`, 20 `Harvester`, 21 `Nuke`, 22 `Tentacle`, 23 `Meteor`, 24 `Tank`, 25 `SpaceShip`, 26 `DropShip` [C] `code/worldobject/entity.cpp:Entity::GetTypeName`, `code/worldobject/entity.h` enum. The same numeric indices are used by the `entityType` columns of `AISpawnPoint` and `SafeArea`.

Shipped rows: `Darwinian` 390 rows in 64 files (39 mp), `Armour` 7 (4 mp), `Tank` 9 (3 mp, TankBattle maps), `Engineer` 3 (old/); Darwinia missions add `Virii`, `Centipede`, `Spider`, `SporeGenerator`, `SoulDestroyer`; blocked and skipped: `AI` 7, `ArmyAnt` 1, `Shaman` 9 [C] survey. Difficulty scaling for team 1 (`number ×= f`, centipede splitting, `spread ×= 1.2^(difficulty/5)`) is inert because `f = 1` (§2.9.1). Note the centipede-copy path calls `sfrand(60)` (non-synchronised random) — dead with `f = 1` [C].

### 2.13 `Routes` block [C] `LevelFile::ParseRoutes`, `ParseRoute`, `WriteRoutes`, `code/routing_system.h:WayPoint`

```
routes   := 'Routes_StartDefinition' EOL { blank | route | other_line } 'Routes_EndDefinition' EOL
route    := 'Route' ID EOL { blank | waypoint } 'End' EOL            ; ID INT, debug-asserted 0 ≤ ID < 10000
waypoint := '0' x y z      ; WayPoint::Type3DPos   (F32 ×3)
          | '1' x z        ; WayPoint::TypeGroundPos (F32 ×2)
          | '2' buildingId ; WayPoint::TypeBuilding (INT)
other_line := any line whose first token is not 'Route' (ignored)
```

The waypoint type is `atoi(first token)` (a debug assert checks it starts with a digit); any other number creates a waypoint at (0,0,0) with that type [C]. No shipped file contains a `Routes` block [C] survey.

### 2.14 `PrimaryObjectives` block [C] `LevelFile::ParsePrimaryObjectives`, `WritePrimaryObjectives`, `code/global_world.cpp:GlobalEventCondition`

```
objectives := 'PrimaryObjectives_StartDefinition' EOL { objective } 'PrimaryObjectives_EndDefinition' EOL   ; NO blank lines (NULL ⇒ CRASH)
objective  := condition [ STRINGID [ CUTSCENE ] ]
condition  := ('BuildingOnline' | 'BuildingOffline') LOCATIONNAME BUILDINGID     ; e.g. "BuildingOnline :mine,90"
            | 'ResearchOwned' RESEARCHNAME
            | 'DebugKey' INT
            | 'NeverTrue'
            | 'AlwaysTrue' | 'NotInLocation'                                   ; debug-asserted as invalid here
```

| Field | Semantics |
|---|---|
| condition word | `GlobalEventCondition::GetType` (case-insensitive) over `AlwaysTrue`(0), `BuildingOnline`(1), `BuildingOffline`(2), `ResearchOwned`(3), `NotInLocation`(4), `DebugKey`(5), `NeverTrue`(6); unknown → −1 (debug assert), the line is still stored |
| `LOCATIONNAME` | resolved with `GlobalWorld::GetLocationId` — the name derived from `game.txt` (`map_<name>.txt` → `<name>`, §3). In Multiwinia the global world is empty, so the id is −1 [C] `code/app.cpp:App::Initialise` (`new GlobalWorld` without `LoadGame`) |
| `BUILDINGID` | INT, the building's unique id in that location |
| `STRINGID` | language phrase id of the objective text (`m_stringId`) |
| `CUTSCENE` | script file run when the objective completes (`m_cutScene`) |

If the block is empty or absent, `GenerateAutomaticObjectives` adds a single `NeverTrue` objective; it also derives *secondary* objectives from `ResearchItem`/`TrunkPort` buildings (both blocked or campaign-only) [C]. Shipped: 77 files `NeverTrue` (one `NeverTrue -1 -1`), 19 files `BuildingOnline` (16 missions, `mp_sandbox`/`mp_sandbox_all` with `:sandbox,2`, `map_sandbox`), 1 `ResearchOwned` [C] survey. In Multiwinia objectives have no gameplay effect [I: only `Location::MissionComplete` (Darwinia) evaluates them].

### 2.15 Mission-only blocks [B:LEGACY]

| Block | Grammar | Semantics [C] |
|---|---|---|
| `Difficulty` | `'CreatedAsDifficulty' INT` (other words ignored) | `m_levelDifficulty = n − 1`, clamped ≥ 0 (1-based in file). Present in 3 shipped missions, value 1 |
| `RunningPrograms` | `ENTITYTYPE count state data waypointX waypointZ { posX posZ health }×count` — all MISSING ⇒ CRASH; `count` ≤ 10 (fixed arrays `m_positionX[10]`, **no bounds check**) | save-game record of running Engineer/Squad tasks (`data` = engineer spirits or squad weapon type). The writer's squad branch uses `%8.2` without a conversion letter, producing malformed output [C] `LevelFile::WriteRunningPrograms` |
| `LeaderboardStats` | `'Time' FLOAT`, `'Kills' INT`, `'Survivors' …` | `Time` is parsed with `sscanf("%f")` into a `double` (`m_leaderboardLevelTime`) — type mismatch (garbage); `Kills` written straight into `m_location->m_teams[2]->m_teamKills` (requires a live location) [C] `LevelFile::ParseLeaderboardStats` |

### 2.16 Writer notes (editor / save path) [C] `LevelFile::Save`, `SaveMapFile`, `SaveMissionFile`, `Write*`

* Map save target: `TARGET_DEBUG` → `data/levels/<map>` via `Resource::GetTextFileWriter`; otherwise `<profile>/maps/<map>` (plain, unencrypted). Mission save: `<profile>/users/<profile name>/<mission>` (encrypted unless `TARGET_DEBUG`), else `data/levels/<mission>` [B:EDITOR]/[B:LEGACY].
* Each building row is `"\t%-20s"` type name then `%-8d %-8.2f %-8.2f %-8d %-8.2f %-8.2f %-8d` for the base columns, then the class's own `Write` [C] `Building::Write`. **Precision loss**: positions and fronts are written with 2 decimals, F32 columns of subclasses mostly `%.2f` — a load/save round-trip is lossy.
* `WriteLandscapeTiles` writes `desiredHeight` with `%6.0f` (integer rounding) and concatenates `lowlandSmoothing%6.2f` and `guidePower%6d` without a separator — fine only because of the field widths [C].

### 2.17 Minimal valid examples (written for this document)

Multiwinia map (`mp_example_2p.txt`):

```
MultiwiniaOptions_StartDefinition
	GameTypes	KingOfTheHill
	PopulationCap	1000
	NumPlayers	2
	Difficulty	basic
MultiwiniaOptions_EndDefinition

Landscape_StartDefinition
	worldSizeX 1200
	worldSizeZ 1200
	cellSize 12.0
	outsideHeight -10.0
	landColourFile landscape_default.bmp
	wavesColourFile waves_default.bmp
	waterColourFile water_default.bmp
Landscape_EndDefinition

LandscapeTiles_StartDefinition
	100 0.00 100 1000 1.20 1.00 120 1 12345 1.00 0
LandscapeTiles_EndDefinition

LandFlattenAreas_StartDefinition
	600.0 20.0 600.0 80.0 0 40.0
LandFlattenAreas_EndDefinition

Lights_StartDefinition
	0.10 0.40 0.90   1.00 1.00 1.00
Lights_EndDefinition

Buildings_StartDefinition
	SpawnPointMaster    1    600.00  600.00  255  1.00  0.00  0   2  3
	SpawnPoint          2    300.00  600.00  0    1.00  0.00  0
	SpawnPoint          3    900.00  600.00  1   -1.00  0.00  0
	MultiwiniaZone      4    600.00  600.00  255  1.00  0.00  0   150.00
Buildings_EndDefinition

CameraMounts_StartDefinition
	player0  200.00 300.00 600.00  0.70 -0.70 0.00  0.70 0.70 0.00
	player1 1000.00 300.00 600.00 -0.70 -0.70 0.00 -0.70 0.70 0.00
CameraMounts_EndDefinition

InstantUnits_StartDefinition
	Darwinian  0  300.0  600.0  20  0  -1  50.0
	Darwinian  1  900.0  600.0  20  0  -1  50.0
InstantUnits_EndDefinition
```

Campaign mission (`mission_example.txt`, [B:LEGACY]):

```
Difficulty_StartDefinition
	CreatedAsDifficulty 1
Difficulty_EndDefinition

Buildings_StartDefinition
	Incubator  10  500.00 500.00 1  1.00 0.00 1  0
Buildings_EndDefinition

PrimaryObjectives_StartDefinition
	BuildingOnline :example,10	example_objective1
PrimaryObjectives_EndDefinition
```

### 2.18 Discrepancies between shipped level files and the parsers [C] `X1a_level_survey.py` (CSV `X1a_level_field_usage.csv`)

| # | Finding | Files | Effect |
|---|---|---|---|
| 1 | `GameTypes none` (not a game type) | `mp_assault_stress`, `mp_benchmark_1`, `mp_sandbox`, `mp_sandbox_all` + 6 old/ | map dropped from the lobby list ("didn't contain a game type"); these are dev/benchmark/tutorial maps reached by other paths [I] |
| 2 | `GameTypes Shaman`, `GameTypes Skirmish` | 10 + 6 old/ maps | unknown names → ignored (old naming) |
| 3 | Top-level `GameTypes` line before any block | `old/mp_chess1.txt` | debug assert; ignored |
| 4 | `CarryableBuilding` building type | `map_sandbox.txt` | not a type name → line ignored |
| 5 | `AIObjective` with 2 extra tokens (needs 1) | `mp_assault_stress.txt` (3 rows), `old/mp_assault_3P_1.txt` (3) | surplus ignored |
| 6 | `Triffid` with 20 extra tokens (max 19) | `old/mp_shaman_2P.txt` (2 rows) | surplus ignored |
| 7 | 39 blocked Darwinia types still present (`Portal` 91 rows in old/, `ScriptTrigger` 25, `TrackLink` 111, `SpiritReceiver` 49, `ReceiverLink` 83, …) | campaign maps, old/ | silently ignored in Multiwinia |
| 8 | Blocked entity types in `InstantUnits` (`AI`, `ArmyAnt`, `Shaman`) | missions, old/ | skipped |
| 9 | `Name NewLevel` placeholder | 10 mp maps | menu shows language-table names instead [I] |
| 10 | `Author`, `Description`, `Routes`, `RunningPrograms`, `LeaderboardStats`, `maxHeight` in non-mp maps — parsed but never present (except `maxHeight` in 41 mp maps) | – | features exist only for editor/save files |
| 11 | Duplicate `PopulationCap` | `mp_cts_4p_3.txt` | last (2000) wins |
| 12 | Second `Buildings` block in 47 mp maps (after `CameraAnimations`) | 47 mp | also static (the dynamic branch is unreachable in `ParseMapFile`) |
| 13 | No blank line inside `Landscape`/`PrimaryObjectives` blocks, all required columns present, all `isGlobal` constraints met | all 118 files | the strict parts of the grammar are respected by shipped data |

<a id="fmt-level-enum"></a>
### 2.19 `data/levels/level_enum.txt` — achievement level ids [C] `code/achievement_tracker.cpp:AchievementTracker::AchievementTracker`

```
level_enum := { blank | MAPFILENAME LEVELID }          ; LEVELID INT; MISSING ⇒ CRASH (atoi(NULL)) for a one-token line
```

* Maps a map file name (case-insensitive `HashTable` key) to an achievement level id; reverse map id → name. Ids 1…49 are exactly the 49 retail maps (every `mp_*` file not excluded by `tools/multiwinia_exclusions.txt`, one id each, no gaps — verified by script); ids 50…59 are `mp_PDLC1_*`/`mp_PDLC2_*` map names that do not exist in this tree [C] file. Ids > `MAX_LEVEL_ID` (49) are ignored by `AddVictoryOnLevel`/`AddVisitedLevel`; the "Master of Multiwinia"/"Explorer" achievements need `VALID_ACHIEVEMENT_LEVELS` = 49 (41 without `INCLUDEGAMEMODE_ASSAULT`) wins/visits; bitsets `m_won[8]`, `m_visited[8]` (64 bits, bit = id) [C].
* Both maps copy the token (`HashTable::PutData` stores `newStr(key)`; `LevelListReverseMap` is `std::map<int,std::string>`), so the reader buffer may be freed [C] `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::PutData`, `code/achievement_tracker.h`. **A duplicate map name (case-insensitive) is fatal**: `HashTable::GetInsertPos` calls `AppAbort("Error with HashTable")` when the key already exists [C] `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::GetInsertPos`. The shipped file has no duplicates [C].
* The file lacks a final newline (last line `mp_PDLC2_rocketriot_3P_1.txt 59`): read from disk it is processed, read from an archive (`TextDataReader`) it would be dropped [C] §1.2 — harmless (id 59 > 49).
* `tools/multiwinia_exclusions.txt` excludes `levels/level_enum.txt` from the retail `main.dat` → in a packaged retail build the file is not found ("Failed to load level_enum.txt") unless shipped another way [C] exclusion list; [U] whether a patch/DLC archive supplies it (resolve: inspect a retail `patch.dat`/PDLC archive).

<a id="fmt-game-txt"></a>
## [→ 29_data_formats_reference] 3. Campaign world files: `game.txt` family [B:LEGACY]

### 3.1 Purpose, files, reachability

`GlobalWorld::LoadGame(name)` builds the Darwinia campaign world (locations, global buildings, research, scripted events). It is reached only through `App::LoadProfile` / `App::SaveProfile`, whose callers are campaign/prologue paths (`Script::RunCommand_EnterLocation`, attract mode, user-profile and mods windows, `ControlTower`) [C] `code/app.cpp:App::LoadProfile`, grep of callers. In the FULL Multiwinia flow `App::Initialise` creates an **empty** `GlobalWorld` (the `LoadProfile()` call is commented out) [C] `code/app.cpp:App::Initialise`. Hence every file in this section is [B:LEGACY] (compiled, unreachable from the Multiwinia menus).

| File | Selected when | Tag |
|---|---|---|
| `data/game.txt` | `GAMEDATAFILE` = `game.txt` in FULL (`DEMOBUILD`/`DEMO2` undefined); `m_gameDataFile = "game.txt"` [C] `code/app.cpp` (`#define GAMEDATAFILE`, L369) | [B:LEGACY] |
| `data/game_demo.txt` | only with `DEMOBUILD` | [B:DEAD] |
| `data/game_demo2.txt` | only with `DEMO2` | [B:DEAD] |
| `data/game_unlockall.txt` | `LoadProfile` when full version and profile name is `AccessAllAreas` or `AttractMode` (then every trunk port is forced online and every location available) [C] `App::LoadProfile` | [B:LEGACY] |
| `data/purity/game.txt` | never (no code opens `purity/`) | [B:DEAD] |
| `<profile>/users/<name>/game.txt` | save game, tried first (§13.3) | [B:LEGACY] |

Load order: user save (`UnicodeTextFileReader`→`TextFileReader`) else `Resource::GetTextReader(name)`; then `locations.txt` (§3.4); then every location's map file and every `levels/mission_<locationname>*.txt` are parsed to register global buildings and mark missions complete; finally `EvaluateEvents()` [C] `GlobalWorld::LoadGame`.

### 3.2 Grammar [C] `GlobalWorld::LoadGame`, `ParseLocations`, `ParseBuildings`, `ParseEvents`, `GlobalEvent::Read`, `GlobalEventAction::Read`, `GlobalResearch::Read`, `ParseTutorial`, `ParseLevelTimes`, `ParseSquaddiesUsed`

```
game_file    := { blank | section | other_line }                    ; other_line silently ignored
section      := locations | gbuildings | research | events | tutorial | leveltimes | squaddies
locations    := 'Locations_StartDefinition' EOL { blank | loc_row } 'Locations_EndDefinition' EOL
loc_row      := ID AVAILABLE MAPFILE MISSIONFILE                    ; INT INT WORD WORD — MISSING ⇒ CRASH
gbuildings   := 'Buildings_StartDefinition' EOL { blank | gb_row } 'Buildings_EndDefinition' EOL
gb_row       := ID TEAMID LOCID TYPE LINK ONLINE                     ; six INT — MISSING ⇒ CRASH
research     := 'Research_StartDefinition' EOL { blank | r_line } 'Research_EndDefinition' EOL
r_line       := 'Research' RESEARCHNAME PROGRESS LEVEL | 'CurrentResearch' RESEARCHNAME | 'CurrentPoints' INT
events       := 'Events_StartDefinition' EOL { blank | event } 'Events_EndDefinition' EOL
event        := 'Event' { condition } EOL { blank | action_line } 'End' EOL
condition    := 'AlwaysTrue' | 'NotInLocation' | 'NeverTrue'
              | ('BuildingOnline' | 'BuildingOffline') LOCATIONNAME BUILDINGID
              | 'ResearchOwned' RESEARCHNAME
              | 'DebugKey' INT
action_line  := 'Action' ( 'SetMission' LOCATIONNAME MISSIONFILE | 'RunScript' SCRIPTNAME | 'MakeAvailable' LOCATIONNAME )
tutorial     := 'Tutorial_StartDefinition' EOL { WORD WORD INT EOL } 'Tutorial_EndDefinition' EOL      ; e.g. "CurrentChapter = 3"
leveltimes   := 'LevelTimes_StartDefinition' EOL { INDEX VALUE EOL } 'LevelTimes_EndDefinition' EOL
squaddies    := 'SquaddiesUsed_StartDefinition' EOL { INDEX VALUE EOL } 'SquaddiesUsed_EndDefinition' EOL
```

All keywords are case-insensitive. `:` and `,` are separators, which is why conditions are conventionally written `BuildingOnline:garden,8` (three tokens) [C] `data/game.txt`.

### 3.3 Field table

| Section / field | Type | Semantics and rules [C] |
|---|---|---|
| `loc_row.ID` | INT | location id (1…12 in Darwinia); referenced by `gb_row.LOCID`, `TrunkPort.targetLocationId`, `locations.txt` |
| `loc_row.AVAILABLE` | INT→bool | location reachable on the world map |
| `loc_row.MAPFILE` | WORD ≤255 | `levels/` map file; **the location name is derived from it**: drop the first 4 characters and the last 4 (`map_garden.txt` → `garden`). A file not following `map_<name>.txt` yields a wrong name; a name shorter than 4 characters underflows (bug) |
| `loc_row.MISSIONFILE` | WORD ≤255 | current mission or `null` |
| `gb_row` | 6×INT | `GlobalBuilding{ id, teamId, locationId, type (Building enum), link, online }`; looked up by (id, locationId) only — shipped rows use type 0 |
| `Research NAME PROGRESS LEVEL` | research name (case-insensitive, 29 names: `Darwinian`, `Officer`, `Squad`, `Laser`, `Grenade`, `Rocket`, `Controller`, `AirStrike`, `Armour`, `TaskManager`, `Engineer`, `Harvester`, `Nuke`, `Subversion`, `PersonalShield`, `HotFeet`, `GunTurret`, `Shaman`, `Infection`, `MagicalForest`, `DarkForest`, `AntNest`, `Plague`, `Egg`, `MeteorShower`, `FireExtinguisher`, `Rage`, `Tank`, `DropShip` — `GlobalResearch::GetTypeName`), INT, INT | sets level then progress. **An unknown name gives index −1 and writes `m_researchLevel[-1]` (out of bounds)** |
| `CurrentResearch NAME` | research name | current research (−1 if unknown) |
| `CurrentPoints INT` | INT | research points |
| any other word in `Research` | – | `AppReleaseAssert("Error loading GlobalResearch")` — **fatal in release** |
| `Event` conditions | §2.14 condition grammar; `LOCATIONNAME` resolved against the locations loaded so far (`GetLocationId`, −1 → debug assert) | conjunction: the event fires when all conditions are true |
| `Action` lines | – | actions execute **one per evaluation** in order (`GlobalEvent::Execute` pops the first action); a first token other than `Action` is debug-asserted but still parsed as an action line; an unknown action word leaves `m_type` uninitialised (debug assert only) — undefined behaviour on execute |
| `SetMission LOC FILE` | – | set the location's mission file |
| `RunScript NAME` | – | `Script::RunScript`: a name containing `.txt` runs `data/scripts/NAME` (§9); a name without `.txt` is a phrase-sequence id handled only under `USE_SEPULVEDA_HELP_TUTORIAL` → **no-op in FULL** (e.g. `RunScript gardenfinished` in `game.txt`) |
| `MakeAvailable LOC` | – | location becomes available |
| `Tutorial` lines | 2nd token ignored, 3rd INT | chapter for the Darwinia demo tutorial; `-1` deletes it — whole section compiled only under `USE_SEPULVEDA_HELP_TUTORIAL` [B:DEAD] |
| `LevelTimes INDEX VALUE` | INT INT | `m_spLevelTimes[INDEX] = VALUE` — no bounds check |
| `SquaddiesUsed INDEX VALUE` | INT INT | **bug**: also writes `m_spLevelTimes[INDEX]` (not a squaddie table) |

Writers: `WriteLocations` (`"\t%4d %4d %30s %40s"`), `WriteBuildings` (`"\t%4d %4d %6d %6d %6d %6d"`), `GlobalResearch::Write` (all 29 research rows + `CurrentResearch` + `CurrentPoints`), `GlobalEvent::Write`, `WriteLevelTimes`/`WriteSquaddiesUsed` (both empty), `WriteTutorial` (DEMO2 only) [C] `GlobalWorld::SaveGame`.

Shipped content: all five files have the four sections Locations, Buildings, Research, Events; `game.txt` defines 11 locations (ids 1–11), 2 global buildings, 11 research rows, 18 events; `game_unlockall.txt` 11/2/11/13; `game_demo.txt` 1/1/9/0; `game_demo2.txt` 8/8/9/1; `purity/game.txt` 11/0/11/6 (locations/buildings/research/events); `game_demo2.txt` uses `BuildingOnline:launchpad:1` (colon instead of comma — equivalent) [C].

<a id="fmt-locations-txt"></a>
### 3.4 `data/locations.txt` — world-map positions [B:LEGACY] [C] `GlobalWorld::LoadLocations`, `SaveLocations`

```
locations_file := { blank | ID X Y Z [ ignored ] }        ; INT F32 F32 F32 — MISSING ⇒ CRASH
```

Sets `GlobalLocation::m_pos` of an **already loaded** location (an id not in the game file — e.g. 12 in the shipped file — is ignored). Coordinates are positions on the sphere world (radius-scale units of the world-map renderer [I]). Read through `Resource::GetTextReader("locations.txt")`; a missing file dereferences `NULL` (crash) [C]. The editor writer (`GlobalWorldEditorWindow`, [B:EDITOR]) emits `"%-6d %-8.2 %-8.2 %-8.2\n"` — the `%-8.2` conversions lack a type letter, so a saved file is malformed [C] `GlobalWorld::SaveLocations`.

Minimal example (both files, written for this document):

```
# game.txt
Locations_StartDefinition
	1  1  map_example.txt  mission_example.txt
Locations_EndDefinition
Research_StartDefinition
	Research Darwinian 0 1
	CurrentResearch Darwinian
	CurrentPoints 0
Research_EndDefinition
Events_StartDefinition
	Event BuildingOnline:example,10
		Action RunScript example_done.txt
		End
Events_EndDefinition
```
```
# locations.txt
1   100.00  -150.00  80.00
```

<a id="fmt-multiwinia-txt"></a>
## [→ 29_data_formats_reference] 4. `data/multiwinia.txt` — game types and their parameters [B:FULL]

### 4.1 Purpose, files, parser

Defines the Multiwinia game types (in **file order**, which *is* the game-type index used by code, maps, network and UI) and each type's lobby parameters. Loaded once by `Multiwinia::LoadBlueprints` through `Resource::GetTextReader`; a missing file is `AppReleaseAssert("Couldn't load multiwinia.txt")` (fatal). `data/multiwinia-testbed.txt` replaces it only under `TESTBED_ENABLED` [B:DEAD] [C] `code/multiwinia.cpp:Multiwinia::LoadBlueprints`.

### 4.2 Grammar

```
mw_file   := { blank | gametype }
gametype  := 'GAMETYPE' NAME EOL { param } 'END' EOL        ; NO blank or comment-only line between GAMETYPE and END
param     := PARAMNAME MIN MAX DEFAULT CHANGE EOL            ; WORD F32 F32 F32 INT — MISSING ⇒ CRASH
```

* The top-level keyword is only debug-asserted to be `GAMETYPE` (`stricmp`); in release **any** first token starts a game type [C].
* Inside a block the parser does `ReadLine(); GetNextToken()` with no `TokenAvailable` check, so a blank or comment-only line inside a block passes `NULL` to `stricmp(param,"END")` (crash). `END` is case-insensitive.
* `NAME`, `PARAMNAME`: `strcpy` into `char[256]`; phrase ids are derived as `multiwinia_gametype_<lower(NAME)>` and `multiwinia_param_<lower(PARAMNAME)>` [C] `MultiwiniaGameBlueprint::MultiwiniaGameBlueprint`, `MultiwiniaGameParameter::MultiwiniaGameParameter`.
* At most `MULTIWINIA_MAXPARAMS` = 64 parameters per type are storable in `Multiwinia::m_params[64]`; the loader does not check [I: `SetGameOptions` copies `numParams` ints into `m_params`].

### 4.3 Field table

| Field | Type | Semantics [C] |
|---|---|---|
| `NAME` | WORD (case-sensitive identity) | blueprint name; level files' `GameTypes` must match it with `strcmp`. Index order must equal the `Multiwinia::GameType*` enum: 0 `Domination` (= `GameTypeSkirmish`), 1 `KingOfTheHill`, 2 `CaptureTheStatue`, 3 `Assault`, 4 `RocketRiot`, 5 `Blitzkrieg` (`GameTypeBlitzkreig`), 6 `TankBattle` [C] `code/multiwinia.h` enum |
| `PARAMNAME` | WORD | looked up case-insensitively by `Multiwinia::GetGameOption(name)` (returns −1 if absent) |
| `MIN`, `MAX`, `DEFAULT` | F32 (stored as `double`, used as int by the lobby) | allowed range and initial value |
| `CHANGE` | INT | UI widget: `-1` = checkbox, `0` = drop-down menu, `> 0` = slider step [C] `code/UI/GameMenuWindow.cpp` (L4589–L4620) |

Shipped parameters (`data/multiwinia.txt`) [C]:

| Game type (index) | Parameters `name min max default change` |
|---|---|
| Domination (0) | ScoreMode 0 1 0 0; TimeLimit 0 60 10 1; Handicap 0 90 90 10; CrateDropRate 0 300 40 10; StartingPowerups 0 3 0 1; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; CrateDropMode 0 1 1 0 |
| KingOfTheHill (1) | ScoreMode 0 3 0 0; TimeLimit 1 60 5 1; Handicap 0 90 90 10; CrateDropRate 0 300 40 10; StartingPowerups 0 3 0 1; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; CrateDropMode 0 1 1 0 |
| CaptureTheStatue (2) | TimeLimit 1 60 10 1; CrateDropRate 0 300 40 10; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; CrateDropMode 0 1 1 0 |
| Assault (3) | Handicap 0 90 90 10; CrateDropRate 0 300 40 10 |
| RocketRiot (4) | CrateDropRate 0 300 40 10; ReinforcementTimer 10 300 60 10; ReinforcementCount 1 100 40 1; MaxArmour 1 10 2 1; TurretFrequency 0 10 7 1; MaxTurrets 0 5 3 1 |
| Blitzkrieg (5) | TimeLimit 0 60 15 1; CrateDropRate 0 300 40 10; ReinforcementTimer 10 300 60 10; ReinforcementCount 1 100 40 1; MaxArmour 1 10 2 1; TurretFrequency 0 10 2 1; RetributionMode 0 1 1 −1; SuddenDeath 0 1 1 −1; MaxTurrets 0 5 2 1 |
| TankBattle (6) | NumTanks 3 10 3 1 (game type compiled out: `INCLUDEGAMEMODE_TANKBATTLE` undefined) |

Units/semantics per parameter (minutes, seconds, percent): [→ 01_game_modes] P02 §2. `multiwinia-testbed.txt` differs only by an extra comment and `TimeLimit 0 60 10 1` appended to RocketRiot [C] diff.

Minimal example:

```
GAMETYPE Domination
    TimeLimit   0  60  10  1
END
```

<a id="fmt-betatesters"></a>
## [→ 29_data_formats_reference] 5. Credit name lists: `betatesters.txt`, `multiwinia_betatesters.txt`

| File | Parser | Grammar | Tag |
|---|---|---|---|
| `data/multiwinia_betatesters.txt` (69 lines, 3 Latin-1 names) | `code/UI/GameMenuWindow.cpp:ReadMultiwiniaBetaTesters` | every physical line → `GetRestOfUnicodeLine()` (leading separators stripped, `#` comments removed, empty lines give an empty entry) + `" \n"`; the concatenation replaces the credits phrase whose text is exactly `*BETATESTERS` (`multiwinia_credits_55`) in `ReadCredits` | [B:FULL] (Credits page) |
| `data/betatesters.txt` (38 lines, 1 Latin-1 name) | `code/loaders/credits_loader.cpp:CreditsLoader::GenerateBetaTesterList` | `GetRestOfLine()`; lines with `strlen > 1` become names; **an empty line returns NULL and `strlen(NULL)` crashes** (the shipped file has none); laid out in 2 columns | [B:LEGACY] (`CreditsLoader` is reached only via `Script` `RunCredits` and the Darwinia sphere-world camera; `RunBootloader` is `USE_LOADERS`-only [B:DEAD]) |

```
name_list := { line }          ; line = free text; leading space/TAB/':'/',' stripped; '#' starts a comment
```

Encoding: single-byte; accented names are Latin-1 bytes and render as the corresponding U+00xx characters (§1.6). Minimal example: `Jane 'Example' Doe` on its own line. A name beginning with `:` or `,` loses that character; a name containing `#` is truncated.

<a id="fmt-preferences"></a>
## [→ 29_data_formats_reference] 6. Preferences (`preferences.txt`, `data/default_preferences.txt`, `user_input_preferences.txt`) [B:FULL]

### 6.1 Files and life cycle [C] `code/lib/preferences.cpp:PrefsManager`, `code/app.cpp:App::App`, `App::GetPreferencesPath`

| File | Role | Read by | Written by | Encoding |
|---|---|---|---|---|
| `<profile>/preferences.txt` (`<profile>` = `%LOCALAPPDATA%\Introversion\Multiwinia\`, or cwd with `--NoProfile`; Mac `~/Library/Preferences/uk.co.introversion.<APP_NAME>.txt`) | user settings | `PrefsManager::Load` (UTF-16 if BOM, else bytes) | `PrefsManager::Save` (13 call sites) | written as **UTF-16LE with BOM** (`UnicodeTextFileWriter`, `_O_U16TEXT`, CRLF [I: CRT text mode translation]) |
| `data/default_preferences.txt` | overrides of the hard-coded first-run defaults | `PrefsManager::CreateDefaultValues` via `Resource::GetTextReader` | never | ASCII |
| `<profile>/user_input_preferences.txt` | user key bindings (§7.5) | `InputManager::parseInputPrefs` (not `PrefsManager`) | a second `PrefsManager` instance in `ApplyKeybindingsButton::MouseUp` | UTF-16LE with BOM |

Life cycle: `App::App` parses `init.dat`, then constructs `g_prefsManager = new PrefsManager(GetPreferencesPath())` [C]. `Load()` reads the file; if it does not exist (or fails the first-line check below) it calls `CreateDefaultValues()` — in the latter case it also saves immediately. Then, unconditionally, `SetInt("MultiwiniaControlMethod", 1)` ("Hack by Chris to disable old-style control"), and the constructor adds `ControlMethod = 1` if `GetInt("ControlMethod")` returns −1 [C] `PrefsManager::Load`, `PrefsManager::PrefsManager`. Because the manager is created before `main.dat` is parsed, a packaged install finds `default_preferences.txt` only if it is a loose file [I, from archive order; P14 §31.5.3].

### 6.2 Line grammar (reader side)

Each physical line is first cleaned by the `TextReader` (§1: `#` comment, CR/LF removal) and then reduced with `GetRestOfUnicodeLine()` (leading space/TAB/`:`/`,` stripped) before `PrefsManager::AddLine` sees it [C] `PrefsManager::Load`.

```
prefs_file := { line }
line       := empty_line | item_line
empty_line := text with no alphanumeric character before an optional '#'      ; IsLineEmpty
item_line  := { non_alnum } KEY SEPCHAR { WS | '=' } VALUE
KEY        := 1*ALNUM                         ; iswalnum (locale-dependent; ASCII letters/digits in practice); case-SENSITIVE
SEPCHAR    := exactly one character (normally ' ' or '='), overwritten and skipped
VALUE      := rest of the line, verbatim (trailing blanks kept)
```

Value typing (`PrefsItem::PrefsItem`) [C]:

| Condition on VALUE | Stored type | Conversion |
|---|---|---|
| first char is `-` or a digit, and VALUE contains no `.` | `TypeInt` | `wistringstream >> int` (leading number; junk after it ignored; failure → 0) |
| first char is `-` or a digit, exactly one `.` | `TypeFloat` | `wistringstream >> float` |
| first char is `-` or a digit, two or more `.` | `TypeString` | e.g. `127.0.0.1` |
| anything else | `TypeString` | verbatim |

Lookup is **type-strict**: `GetInt` on a float item, `GetFloat` on an int item, or `GetString` on a number returns the caller's default [C] `PrefsManager::GetInt/GetFloat/GetString`. Example: `DarwinianScaleFactor = 2` is an int and `GetFloat("DarwinianScaleFactor", …)` ignores it — always write floats with a decimal point. Duplicate keys: the first occurrence wins while loading a file ("Duplicate preference item %s. Ignoring"); lines added while `s_overwrite` is set (the `default_preferences.txt` pass) replace the existing item [C] `PrefsManager::AddLine`.

Edge cases [C]: (a) the **first line** of an existing file is validated — every character before the first space or `=` must lie in `'A'…'z'` (ASCII 65–122, so digits fail); otherwise the whole file is discarded, defaults are recreated and saved ("Unicode file fuck up check"); (b) a key that runs to the end of the line (no separator) makes the value pointer start one past the terminator (out-of-bounds read); (c) the value-skip loop's `value == L'\0'` test compares a pointer, not a character (harmless because `iswspace('\0')` is false).

### 6.3 Writer (`PrefsManager::Save`) [C]

1. All items' `m_hasBeenWritten` flags are cleared; the file is truncated and a UTF-16 BOM written.
2. For each remembered input line (`m_fileText`, in load order): an *empty* line is written verbatim **without adding a newline** — so blank lines and comments read from disk (already reduced to empty strings by the reader) vanish; only the literal `"\n"` lines added by `CreateDefaultValues` survive. For an item line the key is re-extracted by scanning to the first `=`: it starts at the first alphanumeric and ends at the **last non-alphanumeric character before `=`**; the item is written as `KEY = VALUE\n` (`%ls = %d`, `%ls = %.2f`, `%ls = %ls`).
3. Items not yet written are appended in `std::map` order (`wcscmp`, ordinal).

Hazards for implementers (all [C], reproduce only if byte-compatible output is required): a template line of the form `Key=1` (no character between key and `=`) is copied verbatim without a newline, glueing it to the next line and losing the following key on the next load; a template line containing no `=` makes the scan run past the end of the string; `Key  = 1` (two spaces) yields the key `"Key "` and creates a bogus empty item; floats are rounded to 2 decimals; a `%` inside a copied line is interpreted as a format specifier.

### 6.4 Hard-coded first-run defaults (FULL, Win32) and `default_preferences.txt` overrides [C] `PrefsManager::CreateDefaultValues`, `data/default_preferences.txt`

Order of `AddLine` calls (blank `"\n"` lines omitted): `ServerAddress = 127.0.0.1`, `BypassNetwork = 0`, `ServerPort = 4000`, `TextLanguage = unknown`, `TextSpeed = 15`, `HelpEnabled = 1` (0 under `DEMOBUILD`), `SoundLibrary = dsound` (`HAVE_DSOUND`), `SoundMixFreq = 22050` (44100 Mac), `SoundMasterVolume = 255`, `SoundChannels = 32` (16/32 Mac by CPU), `SoundHW3D = 0`, `SoundSwapStereo = 0`, `SoundMemoryUsage = 1`, `SoundBufferSize = 512`, `SoundDSP = 0`, `RenderLandscapeDetail = 1`, `RenderWaterDetail = 1`, `RenderBuildingDetail = 1`, `RenderEntityDetail = 1`, `RenderCloudDetail = 1`, `RenderPixelShader = 1`, `DarwinianScaleFactor = 2.00`, `ControlMouseButtons = 3` (1 Mac), `ControlMethod = 1`, `MultiwiniaControlMethod = 1`, (Linux/Mac: `RenderLandscapeMode = 2`, `ManuallyScaleTextures = 0`), `BootLoader = firsttime`, `UserProfile = AccessAllAreas`, `ScreenOverscan = 0`, `RenderSpecialLighting = 0`, `Difficulty = 1` (`OTHER_DIFFICULTY`). `DEMOBUILD` variants (`StartMap`, `UserProfile = DemoUser`, …) are [B:DEAD].

`data/default_preferences.txt` (17 lines) then overrides/adds: `NetworkServerPort = 4000`, `NetworkClientPort = 4001`, `ServerAddress = 127.0.0.1`, `BypassNetwork = 0`, `ServerPort = 4000`, `TextSpeed = 15`, `ControlMethod = 1`, `WaitVerticalRetrace = 0`, `DarwinianScaleFactor = 2.0`, `MultiwiniaControlMethod = 1`; the line `#TextLanguage = english` is a comment (ignored) [C] file. The complete register of keys read anywhere in code (with defaults at each call site) is in P17b `_work/drafts/P17b_prefs_accesses.csv` and P14 `_work/drafts/P14_platform_lib_pref_keys.csv` [→ 09_meta_systems].

Discrepancies [C] (grep of `code/` for the literal key and for any `#define` naming it): `ServerAddress`, `ServerPort` and `BypassNetwork` are written by both default sources but **never read** (dead keys; the live ports are `NetworkServerPort`/`NetworkClientPort` via `PREFS_NETWORKSERVERPORT`/`PREFS_NETWORKCLIENTPORT`, `code/network/network_defines.h`, read in `code/network/server.cpp`, `code/network/clienttoserver.cpp`, `code/UI/GameMenuWindow.cpp`); `WaitVerticalRetrace` defaults to **1** in code (`code/renderer.cpp`, L146) but `default_preferences.txt` sets 0, so a fresh development install starts without v-sync while a packaged install (file not found, §6.1) starts with it [I]; `DarwinianScaleFactor = 2.0` (float) is consistent with the code default `2.00`.

Minimal example (`preferences.txt`, shown as text; the game writes UTF-16LE):

```
TextLanguage = english
ScreenWidth = 1280
RenderSpecialLighting = 0
DarwinianScaleFactor = 2.00
ServerAddress = 127.0.0.1
```

<a id="fmt-input-bindings"></a>
## [→ 29_data_formats_reference] [→ 06_player_control_and_input] 7. Input binding files [B:FULL]

### 7.1 Files and layering [C] `code/main.cpp:ReadInputPreferences`, `code/lib/input/file_paths.cpp`

| Layer (order) | Path | Reader | `replace` | Selection |
|---|---|---|---|---|
| 1 system | `data/input/input_preferences.txt` (Win32), `input_preferences_mac.txt` (Mac), `input_preferences_darwinia.txt` (pref `MultiwiniaControlMethod` == 0, which `PrefsManager::Load` forces to 1 → unreachable [C]) | `Resource::GetTextReader` | false (append) | `InputPrefs::GetSystemPrefsPath` |
| 2 keyboard layout | `data/input/keyboards/<layout>.txt` | `Resource::GetTextReader` (missing file silently skipped) | true | `<layout>` = pref `KeyboardLayout`, default = `locales.txt` entry for `GetKeyboardLayout(0) & 0xFFFF` (Win32 LANGID); empty on Mac |
| 3 user | `<profile>/user_input_preferences.txt` | `UnicodeTextFileReader`, else `TextFileReader` | true | `InputPrefs::GetUserPrefsPath` |

`replace = true` replaces the control's *primary* (first) binding instead of appending [C] `ControlBindings::bind` (P10 §1.10).

### 7.2 Line grammar [C] `code/lib/input/input.cpp:InputManager::parseInputPrefs`, `parseInputSpecString`, `code/lib/input/inputspec.cpp:InputSpecTokens`

```
binding_file := { line }
line         := blank
              | CONTROL '=' spec                      ; binding line
              | CONTROL '~' ICONPATH                  ; icon line
              | other                                  ; rejected ("Assignment not found."), parsing continues
CONTROL      := control name, case-insensitive (164 names of code/lib/input/control_types.inc; list: P10 §2.2 /
                _work/drafts/P10_input_camera_bindings.csv)
'=' / '~'    := must be the whole SECOND token (default TextReader separators); "Ctl=key a" is rejected
spec         := rest of the line (GetRestOfLine), re-split on WHITESPACE ONLY (istringstream >> string):
                ':' and ',' are ordinary characters inside a spec
ICONPATH     := rest of the line, trailing CR/LF removed; stored with ControlBindings::setIcon (last wins)
```

An unknown CONTROL ("Control ID not found."), an unparsable spec ("Parse failed - <err> (state = n)") or a refused bind ("Binding failed.") only print a message (to `cout`; to `<profile>/inputprefs_debug.txt` on Mac or `TARGET_DEBUG`) — parsing never aborts [C].

### 7.3 Spec grammar (driver chain) [C] `code/main.cpp:InitialiseInputManager`, each `code/lib/input/inputdriver_*.cpp:parseInputSpecification`

The token list is offered to the drivers in registration order; the first that returns a success state wins, otherwise the error of the driver that got furthest is reported. Order: `Conjoin`, `Chord`, `Invert`, `Idle`, `W32` (key/mouse), `XInput` (only if `XINPUT1_3.dll` loads), `Prefs`, `Value`, `Alias`. This order makes `&&` bind loosest, then `++`, then `not`.

```
spec        := conj
conj        := part { '&&' part }                      ; ≥1 '&&' required for this driver; ≤1 part may be non-BOOL
part        := chord | unary                           ; each part is re-parsed by the whole chain
chord       := leafnc '++' leafnc { '++' leafnc }      ; expands to N conjunctions: part i "down", others "pressed"; BOOL only
unary       := ('not' | '!') unary | leaf              ; 'not' case-insensitive; operand must be BOOL
leaf        := 'key' KEYNAME BCOND
             | 'mouse' MBUTTON BCOND
             | 'mouse' ('axes' | 'wheel') ACOND
             | 'XInput' XBUTTON ( BCOND | 'repeat' )
             | 'XInput' XANALOG ACOND [ 'with' 'sensitivity' N ]      ; N integer 1…200, default 100
             | 'XInput' XDIR ( BCOND | 'repeat' )                     ; sensitivity fixed at 100
             | ('idle' | 'Idle' …) ( 'atleast' | 'reaches' ) SECONDS  ; driver word case-insensitive; SECONDS integer > 0
             | 'idle' 'no' 'longer'                                  ; 'longer' case-insensitive
             | 'pref' [ 'not' | '!' ] PREFKEY | 'pref' '!'PREFKEY     ; key must EXIST in g_prefsManager at parse time
             | 'value' ( 'Pecan' | 'Hummingbird' )
             | 'alias' CONTROL
leafnc      := a leaf without its trailing condition word (the chord driver appends it)
BCOND       := 'down' | 'up' | 'pressed'
ACOND       := 'release' | 'position' | 'move' | 'zero' | 'read'
MBUTTON     := 'left' | 'right' | 'middle' | 'any'
XBUTTON     := 'leftbumper' | 'rightbumper' | 'dpadleft' | 'dpadright' | 'dpadup' | 'dpaddown' | 'a' | 'b' | 'x' | 'y'
             | 'start' | 'back' | 'plug' | 'lefttriggerb' | 'righttriggerb'           (type BOOL)
             | 'leftthumbstick' | 'rightthumbstick'                                  (type 2D|BOOL: click with BCOND)
XANALOG     := 'lefttrigger' | 'righttrigger'                                        (type 1D)
             | 'leftthumbstick' | 'rightthumbstick'                                  (type 2D|BOOL: axes with ACOND)
XDIR        := 'leftthumbstickleft' | 'leftthumbstickright' | 'leftthumbstickup' | 'leftthumbstickdown'
             | 'rightthumbstickleft' | 'rightthumbstickright' | 'rightthumbstickup' | 'rightthumbstickdown'
KEYNAME     := case-insensitive entry of getKeyNames()[0…KEY_TILDE=223] or 'any' (KEY_ANY=255): BACKSPACE TAB ENTER
               SHIFT CONTROL ALT PAUSE CAPSLOCK ESC SPACE PGUP PGDN END HOME LEFT UP RIGHT DOWN INSERT DEL 0…9 A…Z
               0_PAD…9_PAD ASTERISK PLUS_PAD MINUS_PAD DEL_PAD SLASH_PAD F1…F12 NUMLOCK SCRLOCK COLON EQUALS COMMA
               MINUS STOP SLASH AT OPENBRACE BACKSLASH CLOSEBRACE QUOTE TILDE, plus 'Unknown' (index 109)
```

Case sensitivity: driver words `key`, `mouse`, `XInput`, `pref`, `value`, `alias`, mouse/XInput control names, condition words, `with`, `sensitivity`, `atleast`, `reaches`, `no` are **case-sensitive** (`std::string ==`); `idle`, `not`, `longer`, key names and control names are case-insensitive [C] `W32InputDriver::acceptDriver`, `XInputDriver::acceptDriver/getControlID`, `IdleInputDriver::acceptDriver/getConditionID`, `InputDriver::getDefaultConditionID`, `W32InputDriver::getKeyId`, `ControlBindings::getControlID`.

Condition typing: `getDefaultConditionID(name, type)` scans the 8-entry table {`down`,`up`,`pressed`: BOOL; `release`,`position`,`move`,`zero`,`read`: ANALOG} for a name whose type matches the control's type. **An unknown or type-mismatched condition word does not fail**: the scan stops on the NULL terminator (index 8 < `NumInputConditions` 10) and returns `COND_DOWN` with the spec type set to 0 [C] `code/lib/input/inputdriver.cpp:InputDriver::getDefaultConditionID`. Examples in shipped files: `XInput lefttrigger repeat`, `XInput righttrigger repeat` (analog + `repeat`) parse as "down". Key names containing a space (`unknown 0`…) can never be written because specs are whitespace-split; `META` (index 224 > `KEY_TILDE`) is unreachable [C].

Parser states (`InputParserState`, `code/lib/input/inputdriver.h`): `STATE_ERROR`, `STATE_WANT_DRIVER`, `STATE_WANT_CONTROL`, `STATE_WANT_COND`, `STATE_WANT_MODIFIER`, `STATE_WANT_OPTIONAL`, `STATE_BAD_EXTRA`, then `STATE_DONE`/`STATE_OVERSTEP`/`STATE_CONJ_ERROR` (success = `PARSE_SUCCESS(state)`); a trailing surplus token yields `STATE_OVERSTEP` for single-word drivers (`alias`, `pref`, `value`) [C] respective drivers. Semantics of each condition and driver at run time: [→ 06] P10 §1.3–§1.6.

### 7.4 Shipped files [C] `X1a_input_lines.py`

| File | binding lines | icon lines | rejected | spec heads |
|---|---|---|---|---|
| `input_preferences.txt` | 224 | 18 | 2: `UnitPrimaryFireDirected ~ …` (unknown control), `TaskManagerSwitchTask INPUT_TYPE_BOOL` (no `=`) | `key` 94, `XInput` 88, `mouse` 32, `SpaceNav` 4 (no such driver → parse failed), conjunctions 4, chord 1, `idle` 1 |
| `input_preferences_darwinia.txt` | 220 | 18 | same 2 | `key` 97, `XInput` 87, `mouse` 30, conj 4, chord 1, idle 1 |
| `input_preferences_mac.txt` | 231 | 18 | same 2 | `key` 94, `XInput` 88, `mouse` 32, `SpaceNav` 4, conj 11, chord 1, idle 1 |
| `keyboards/de_DE.txt` | 8 | 0 | 0 | `key` 8 |
| `keyboards/fr_FR.txt` | 8 | 0 | 0 | `key` 8 |

Per-line decoded bindings: P10 `_work/drafts/P10_input_camera_preflines.csv`.

### 7.5 `user_input_preferences.txt` (runtime-written) [C] `code/interface/prefs_keybindings_window.cpp:ApplyKeybindingsButton::MouseUp`

Written by a dedicated `PrefsManager(GetUserPrefsPath())` that is `Clear()`ed, filled with `SetString(<control name>, <binding string>)` for each of the user-rebindable controls (and the derived `IconsTaskManagerHide` = Display binding with `down`→`up`), then `Save()`d — so the file has the §6 writer format: UTF-16LE + BOM, one `ControlName = spec` line per control in ordinal `wcscmp` order. It is read back by `parseInputPrefs` (layer 3), which needs `=` as a separate token — satisfied by the `%ls = %ls` writer. Because a UTF-16 disk file's narrow mirror is not comment/CR-stripped (§1.3), `parseInputPrefs` explicitly trims a trailing `\n`/`\r` from icon paths; spec strings are immune because they are whitespace-split [C] `parseInputPrefs`. Note: the constructor of that `PrefsManager` first runs `Load()` — when the file does not exist yet this calls `CreateDefaultValues()` (all general defaults) before `Clear()` discards them [C].

Minimal example (system or user file):

```
# camera on the arrow keys, zoom with Z or the mouse wheel button
CameraForwards  = key up pressed
CameraBackwards = key down pressed
CameraZoom      = key z pressed
CameraZoom      = mouse middle down
SkipMessage     = key enter down && not key shift pressed
MenuActivate    ~ icons/button_a.bmp
```

<a id="fmt-keyboard-layout"></a>
### 7.6 Keyboard layout files `data/input/keyboards/<layout>.txt`

Same grammar as §7.2, applied with `replace = true`. Shipped: `de_DE.txt` (QWERTZ: `CameraZoom = key y pressed`, …) and `fr_FR.txt` (AZERTY: Left `q`, Up `a`, Forwards `z`, Zoom `w`), 8 camera bindings each [C] files. A layout that has no file (e.g. `en_US`) is silently skipped. `InputPrefs::IsKeyboardFrenchLocale()` (layout == `fr_FR`) also changes a help image [C] `code/lib/input/file_paths.cpp`; P10 §1.10. The path is cached on first use; "Restore defaults" in Options does not re-apply this layer (P10).

<a id="fmt-locales"></a>
### 7.7 `data/input/locales.txt` [C] `code/lib/input/file_paths.cpp:readLocaleNames`

```
locales_file := { blank | LANGID LAYOUTNAME }        ; LANGID = atoi(first token); LAYOUTNAME = rest of line verbatim
```

Shipped: `1031 de_DE`, `1033 en_US`, `1036 fr_FR`, `2057 en_UK`, `4105 en_CA` (first line blank). `LAYOUTNAME` keeps trailing blanks (they would become part of the file name `keyboards/<name>.txt`); later duplicates of an id are ignored (`std::map::insert`). Only consulted on Windows (`GetKeyboardLayout(0) & 0xFFFF`) when pref `KeyboardLayout` is absent [C] `InputPrefs::GetKeyboardLayout`.

Docs-vs-code (input): `docs/input_preferences_syntax.txt` omits icon lines (`~`), the `value` and `alias` drivers, the `repeat` condition, `key any`/`mouse any`, the XInput direction controls and `lefttriggerb`/`righttriggerb`; it states that a missing preference makes `pref X` false, but the code **rejects the binding at parse time** when the key does not exist (`PrefsInputDriver::parseInputSpecification` returns `STATE_WANT_CONTROL`) [C].

<a id="fmt-language"></a>
## [→ 29_data_formats_reference] 8. Language files `data/language/<language>.txt` [B:FULL]

### 8.1 Files, encoding, selection [C] `code/lib/language_table.cpp:LangTable::ParseLanguageFile`, `code/app.cpp:App::InitLanguage`, `App::TrySetLanguage`, `App::SetLanguage`

| File | Encoding (survey) | Lines | Phrases | Notes |
|---|---|---|---|---|
| `english.txt` | single-byte (2 Latin-1 bytes) | 3120 | 2423 | reference set |
| `french.txt` | UTF-16BE + BOM | 3219 | 2514 | 7 English ids missing, 98 extra |
| `german.txt` | UTF-16BE + BOM | 3186 | 2487 | 8 missing, 72 extra; one literal `\N` |
| `italian.txt` | UTF-16BE + BOM | 3035 | 2361 | 62 missing |
| `spanish.txt` | UTF-16BE + BOM | 3219 | 2514 | 7 missing, 98 extra |
| `russian.txt` | UTF-16LE + BOM | 2770 | 2141 | 282 missing |
| `japanese.txt` | UTF-16LE + BOM, no final newline | 281 | 220 | partial (2205 ids missing) |
| `korean.txt` | UTF-16BE + BOM, no final newline | 218 | 160 | partial (2265 missing) |
| `portuguese.txt` | UTF-16BE + BOM, mixed CRLF/LF, no final newline | 218 | 160 | partial (2264 missing) |

[C] `X1a_language_survey.py`. The language files are packed into `language.dat` (not `main.dat`; `language` is excluded from `main.dat`) by `tools/build_language_dat.bat` [C] `tools/multiwinia_exclusions.txt`; P14 §31.11.

Selection: `InitLanguage` tries, in order, pref `TextLanguage` (default `"unknown"`; not on Mac), the OS language (`GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SENGLANGUAGE)`, e.g. `German`), `english`, then the first file listed in `language/`; the first name for which `language/<lower(name)>.txt` exists is used and stored back into `TextLanguage` (failure of all four is fatal) [C] `App::InitLanguage`, `code/lib/system_info.cpp`. `SetLanguage(name)` then parses `language/<name>.txt` and, if present, a **mod overlay** `strings_<name>.txt` or else `strings_default.txt` from the `data/` root into the same table (later definitions override) [C] `App::SetLanguage`. `setlocale(LC_CTYPE, name)` is called; `chinese-trad`, `chinese-simp`, `korean`, `japanese` switch every font to the `square` title font with horizontal scale 1.0 (others 0.6) [C].

### 8.2 Line grammar

```
lang_file   := { blank | phrase_line }
phrase_line := KEY [ SEP TEXT ]
KEY         := first token, narrow separator set " \t\n\r:,"; looked up case-INSENSITIVELY
SEP         := 1*( ' ' | TAB | ':' | ',' | FF(UTF-16 only) )
TEXT        := rest of line (GetRestOfUnicodeLine), after §1 cleaning — a '#' anywhere ends the text
```

Rules [C] `ParseLanguageFile`:

1. Duplicate KEY (case-insensitive): **last definition wins** (old phrase deleted). Shipped duplicates: `buildingname_grenadeturret`, `multiwinia_help_title_crates` (all full files), plus `multiwinia_help_breakformation`, `multiwinia_help_clearinfo` in Italian.
2. Escape: the two characters `\n` become **space + newline** (`" \n"`). No other escape exists; any other backslash sequence (e.g. `\N` in German) stays literal; there is no way to write `#`, a leading space/`:`/`,`, or a literal `[` … `]` pair (§8.3).
3. Post-processing: if the last character is a newline it is removed; if the character before last is CR or FF it is replaced by NUL. An empty TEXT (key alone on the line) reads index −1 (UB; occurs once in French, German and Russian).
4. Encoding: UTF-16 files are decoded unit-by-unit (no surrogate pairs); single-byte files are Latin-1 (§1.6). Narrow mirrors (used by legacy `char*` callers) turn code units > 255 into `?`.
5. Unterminated last line: dropped for UTF-16 and archive files (§1.2) — `multiwinia_exclamation` is lost from `japanese.txt` and `korean.txt`, `multiwinia_option_5_5_1` from `portuguese.txt` [C] survey.

### 8.3 Inline markup (expanded once per input mode when the tables are built) [C] `LangTable::RebuildTable`, `buildCaption`, `consumeMarker`, `consumeIfMarker`, `consumeKeyMarker`, `consumeOtherMarker`, `markerLength`

```
text      := { char | marker }
marker    := '[IFMODE KEYBOARD]' | '[ELSE]' | '[ENDIF]'        ; nestable conditional; mode word compared case-insensitively
           | '[KEY' CONTROLNAME ']'                           ; e.g. [KEYCAMERAFORWARDS]
           | '[' PHRASEKEY ']' | '[ ' PHRASEKEY ']'            ; inline another phrase (leading space variant emits a space)
```

* `[IFMODE KEYBOARD]` … `[ELSE]` … `[ENDIF]`: text between is written only in keyboard mode / only in gamepad mode. Only the word `KEYBOARD` is understood; `[IFMODE GAMEPAD]` increments the nesting depth but falls through to the phrase-reference rule (consumed, writes nothing). No shipped phrase uses IFMODE [C] survey.
* `[KEY<name>]`: `<name>` (1…63 chars) is looked up with `InputManager::getControlID` (case-insensitive) and replaced by the bound input's description noun (`getBoundInputDescription`). An unknown control or unbound control falls through to the phrase-reference rule and is removed. English uses 87 such markers [C].
* `[<key>]`: `<key>` (1…63 chars, lower-cased) is replaced by the mode-specific raw phrase (`<key>_kbd`/`<key>_xin` preferred, else `<key>`), recursively expanded; an unknown key is removed. **Any `[…]` pair is therefore markup**: `multiwinia_notpermitted  [Not permitted]` (English) expands to an empty string [C] by code path; the id is not referenced anywhere in `code/` (grep), so there is no visible effect [C].
* Output is capped at `SEPULVEDA_MAX_PHRASE_LENGTH` = 1024 code units; marker contents ≥ 64 code units are removed unexpanded.
* Hazard: an unterminated `[` makes `markerLength` return 0 and `consumeOtherMarker` move the read position back by one, so `buildCaption` **never terminates** [C]. No shipped phrase has one [C] survey.
* Hazard: `consumeMarker`/`consume*Marker` copy the partially built caption through freshly allocated, uninitialised buffers (`UnicodeString::Resize` copies only `min(old,new)` units of an empty source); correct output depends on heap block reuse [I]. A re-implementation should simply append the expansion to the output buffer.

### 8.4 Mode-specific keys and lookup [C] `RebuildTable`, `LookupPhrase`, `RawLookupPhrase`, `DoesPhraseExist`

* Suffixes: `<key>_kbd` (keyboard/mouse) and `<key>_xin` (Xbox controller). Two hash tables are built, one per `InputMode` (`INPUT_MODE_KEYBOARD`, `INPUT_MODE_GAMEPAD`). For mode M the table key is the id without suffix; a suffixed variant for M wins over the plain id; an id existing only with the *other* suffix yields an empty string in M. Ids starting with `part_` are excluded from the tables (usable only inside markers). Shipped: 120 `_kbd` and 75 `_xin` ids in each full file, no `part_` ids [C] survey.
* `LANGUAGEPHRASE(id)` → `LookupPhrase`: current-mode table; **a missing id returns the id string itself** (no fallback to English). `RawLookupPhrase` returns `ERROR (<id>)`. `ISLANGUAGEPHRASE(id)` tests the current-mode table [C]. With a partial language (Japanese, Korean, Portuguese) most UI text therefore shows raw ids [C] by code; [U] whether a retail edition shipped complete files for these languages (resolve: inspect the retail `language.dat`).
* Phrase-id conventions derived by code (non-exhaustive list of generators): `multiwinia_gametype_<type>`, `multiwinia_param_<param>`, `buildingname_<Type>`, `entityname_<Type>`, `researchname_<Type>`, `cratename_<Name>`, `multiwinia_credits_<n>` (n = 1, 2, … until the first missing id), `bootloader_credits_<n>` [C] respective `GetNameTranslated`/`GetTypeNameTranslated`, `code/UI/GameMenuWindow.cpp:ReadCredits`.

### 8.5 Runtime placeholders inside TEXT

`*<letter>` is replaced at display time by `UnicodeString::ReplaceStringFlag(letter, value)` (every occurrence of `*` followed by that letter) [C] `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::ReplaceStringFlag`. Letters substituted somewhere in `code/`: `F`, `G`, `H`, `P`, `R`, `S`, `T`, `U`, `X` (most common `*T` = team/player name or text, `*S` = seconds); English also contains `*K`, `*A` (leaderboard strings with no live substitution found) and the whole-phrase macro `*BETATESTERS` (§5) [C] grep. Phrases are never used as printf format strings (grep), so `%` is literal.

### 8.6 Minimal example (written for this document)

```
# example.txt (UTF-16LE with BOM, or plain ASCII)
menu_start                Start game
menu_start_xin            Press START to begin
help_zoom_kbd             Press [KEYCAMERAZOOM] to zoom.\nRelease to stop.
example_teamwins          *T wins the match
```

<a id="fmt-script"></a>
## [→ 29_data_formats_reference] [→ 30_scripting_tutorial_and_cutscenes] 9. Cutscene scripts `data/scripts/*.txt` [B:LEGACY]

Lexical and syntactic grammar only; the meaning of each command is specified by P19 §1.3 [→ 30].

### 9.1 Files and execution model [C] `code/script.cpp:Script::RunScript`, `Script::Advance`, `Script::AdvanceScript`, `Script::GetOpCode`

* `RunScript(name)`: if `name` contains `.txt`, opens `data/scripts/<name>` through `Resource::GetTextReader`; otherwise it is a phrase-sequence id handled only under `USE_SEPULVEDA_HELP_TUTORIAL` (no-op in FULL). In a demo (`!IsFullVersion()`) names starting with `launchpad_purpose` are refused.
* `Advance()` runs at most **one line per call** (once per rendered frame) and only when no wait condition is pending; a blank or comment line consumes a frame. End of file closes the script. `SkipCutscene` input ends waits and, after `PermitEscape`, aborts the script.
* All 42 shipped scripts are Darwinia content; `scripts/*` is excluded from the retail `main.dat` [C] `tools/multiwinia_exclusions.txt`.

### 9.2 Grammar

```
script     := { line }
line       := [ command ] EOL                              ; after §1 cleaning ('#' comments)
command    := OPCODE [ ARG1 [ ARG2 [ ARG3 [ ARG4 ] ] ] ] { ignored }
OPCODE     := one of the 43 names of g_opCodeNames, case-insensitive; unknown → opcode −1 (debug assert), line ignored
ARG1       := token, available both as text (nextWord) and as F32 (nextFloat = iv_atof(token), 0 if absent)
ARGn (n>1) := token read on demand with GetNextToken() — MISSING ⇒ CRASH unless noted
```

Argument signatures (syntax only; R = required, O = optional, "w" = word, "f" = F32, "i" = INT):

| Opcodes | Arguments |
|---|---|
| `CamCut`, `CamAnim`, `EnterLocation`, `Say`, `TriggerSound`, `StopSound`, `DemoGesture`, `GiveResearch` | ARG1 w (absent → NULL passed on; most handlers dereference it) |
| `CamMove` | ARG1 w mount, ARG2 f duration (R) |
| `CamFov` | ARG1 f fov, ARG2 i immediate (O, default true) |
| `CamBuildingFocus` | ARG1 i building, ARG2 f range (R), ARG3 f height (R) |
| `CamBuildingApproach` | ARG1 i building, ARG2 f range, ARG3 f height, ARG4 f duration (all R) |
| `CamLocationFocus` | ARG1 w location, ARG2 f fov (R) |
| `Wait` | ARG1 f seconds (absent → 0) |
| `WaitRocket` | ARG1 i building, ARG2 w state (R), ARG3 i data (R) |
| `Highlight` | ARG1 i building |
| `SetMission` | ARG1 w location, ARG2 w mission file (R) |
| `SetCutsceneMode` | ARG1 i (`atoi(nextWord)` — absent ⇒ CRASH; handler is Sepulveda-only) |
| `DestroyBuilding` | ARG1 i building, ARG2 f intensity (R) |
| `ActivateTrunkPort`, `ActivateTrunkPortFull` | ARG1 i building |
| `CamGlobalWorldFocus`, `CamReset`, `ExitLocation`, `ShutUp`, `WaitSay`, `WaitCam`, `WaitFade`, `WaitPlayerNotBusy`, `ClearHighlights`, `GameOver`, `ResetResearch`, `RestoreResearch`, `RunCredits`, `GodDishActivate`, `GodDishDeactivate`, `GodDishSpawnSpam`, `GodDishSpawnResearch`, `TriggerSpam`, `PurityControl`, `ShowDarwinLogo`, `ShowDemoEndSequence`, `PermitEscape` | none (extra tokens ignored) |

Shipped files: 42 (`attractmode.txt` … `yard_primary3.txt`), all single-byte ASCII, CRLF, terminated [C] survey. Arity check over the 42 scripts + `data/purity/buyme.txt` (`_work/scripts/X1a_script_check.py`): 37 of the 43 opcodes are used (most frequent `Say` 273, `WaitSay` 112, `CamMove` 72, `Wait` 54, `CamBuildingFocus` 52), no unknown opcode, no missing required argument; one surplus argument (`CamCut … 1` in `launchpad_victory.txt`, ignored) [C]. Never used: `CamAnim`, `RunCredits`, `SetCutsceneMode`, `DestroyBuilding`, `ActivateTrunkPort`, `ActivateTrunkPortFull`. `SCRIPT_TEST_ENABLED` (`Script::TestScript`, a static checker of mount/animation names) is never defined [B:DEAD].

Minimal example:

```
# intro.txt
CamCut      start
Wait        2.5
CamMove     overview  4.0
WaitCam
TriggerSound cutscene1
```

<a id="fmt-purity-misc"></a>
## [→ 29_data_formats_reference] 10. `data/purity/*` (anti-piracy "purity control" data set) [B:DEAD]

| File | Content | Grammar |
|---|---|---|
| `data/purity/game.txt` | a cut-down copy of `game.txt` whose containment-completion event runs `buyme.txt` instead of the mine/generator missions (11 locations, no global buildings, 11 research rows, 6 events) | §3 |
| `data/purity/map_mine.txt` | variant of `levels/map_mine.txt` (smaller world 2000×2000, a single tile with a 5-power guide grid; no flatten areas) | §2 (map) |
| `data/purity/mission_mine_enable.txt` | variant mission with only two camera mounts (`start`, `buyme1`), no buildings/units | §2 (mission) |
| `data/purity/buyme.txt` | a cutscene **script** (§9): `EnterLocation mine`, `CamMove buyme1 80`, `WaitFade`, `TriggerSound cutscene2`, six `Say puritycontrol_<n>`, `WaitCam`, `WaitSay`, `StopSound cutscene2`, `PurityControl` | §9 |

No code path opens `data/purity/` (grep: the only `purity` hits in `code/` are `#ifdef PURITY_CONTROL` guards and the `PurityControl` script command) and every packaging exclusion list removes `purity` [C] `tools/*exclusions*.txt`. The `PurityControl` command deletes every file in `users/<profile>/`, opens the "buy now" web page and calls `exit(0)` [C] `code/script.cpp:Script::RunCommand_PurityControl` — documented for completeness; a new game must not replicate it. [I] The set was meant to be copied over `data/` in cracked builds (inferred from names and `buyme.txt` content).

<a id="fmt-testbedconfig"></a>
## [→ 29_data_formats_reference] 11. Root `testbedconfig.txt` — automated test-bed sequence [B:DEAD]

Parser: `GameMenuWindow::ReadTestBedConfig`, compiled only under `TESTBED_ENABLED` (never defined) [C] `code/UI/GameMenuWindow.cpp` (`#ifdef TESTBED_ENABLED` L619–L1338). Opened with a plain `TextFileReader("testbedconfig.txt")` relative to the working directory (not through `Resource`).

```
testbed_file := { blank | command }
command      := 'SetProgression' ( 'LINEAR' | 'RANDOM' | 'SEQUENCE' )          (cs)
              | 'SetRandSeed' INT                                               ; sscanf %d
              | 'SetTestBedName' WORD                                           ; sscanf %s into char[256] (no limit)
              | 'AddSequenceItem' GAMEINDEX RANDSEED                            ; both INT
              | 'SequenceIDToTerminateOn' INT
              | other                                                            ; ignored
```

All command words are case-sensitive (`strcmp`); a missing argument skips the command (`continue`). `GAMEINDEX` indexes `g_vGameEntries`, built by `BuildGameEntries` as the concatenation, over game types 0…5 (type 6 skipped), of every map in the lobby list of that type [C]; `SetTestBedName` also sets `App::m_testbedServerName`. The shipped file: `SetProgression RANDOM`, `SetTestBedName TESTBED_GodOfCode`, `AddSequenceItem 0 10`, `AddSequenceItem 10 20` (no final newline — processed anyway by `TextFileReader`, §1.2) [C]. Bug: `BuildGameEntries` stores `maps.GetData(i)->m_mapId` (game-type index) instead of `GetData(x)` [C].

<a id="fmt-support-ini"></a>
## [→ 29_data_formats_reference] 12. Root `MultiwiniaSupport.ini` — crash-report file list [B:FULL tool]

Consumer: the separate crash-report tool built from `contrib/XCrashReport` and shipped as `MultiwiniaSupport.exe` (copied by `tools/build_multiwinia_crashreporter.bat`, installed by `tools/InstallerScript_retail_crashreporter*.iss`) [C]. Parsed with Win32 `GetPrivateProfileString` [C] `contrib/XCrashReport/Dlg.cpp:CDlg::GetFileDetails`, `contrib/XCrashReport/IniDefines.h`.

```
ini        := { comment | section | entry }          ; Win32 INI rules: ';' comment lines, [Section], Key=Value
section    := '[FilesToAdd]' | '[RegistryToAdd]'
entry      := 'File' NNN '=' PATH ',' DESCRIPTION ',' FILETYPE          ; NNN = 001…999, consecutive
            | 'Registry' NNN '=' REGPATH ',' DESCRIPTION
```

Rules [C]: entries are read for NNN = 001, 002, … and reading **stops at the first missing number**; fields are split with `_tcstok` on `,`, CR, LF (so a field cannot contain a comma) and a missing field aborts the list; a `PATH` lacking either `\` or `:` is relative to the profile directory given to the tool or, failing that, to the tool's own directory; files that do not exist are skipped; if no file entry is accepted, the built-in defaults (`ERRORLOG.TXT`, `CRASH.DMP`) are used. For each registry entry a `RegistryNNN.txt` export is generated. The `[FilesToAdd]` INI is located in the module directory only, the `[RegistryToAdd]` INI in the profile directory when one is given (inconsistent) [C].

Shipped entries: `CRASH.DMP`, `ERRORLOG.TXT`, `DXDIAG.TXT`, `DXDIAG_INFO.TXT`, `BLACKBOX.TXT`, `PREFERENCES.TXT`, `userprefs.txt`, `authkey`, `debug.txt`; `[RegistryToAdd]` empty. Discrepancies [C]: the game writes `user_input_preferences.txt`, not `userprefs.txt`; the auth key is read from `authkey.dev` (`authkey` is only one of two save names, §13.4); `DXDIAG*.TXT` are produced only by the benchmark code (`BENCHMARK_AND_FTP`, off).

Minimal example:

```
[FilesToAdd]
File001=CRASH.DMP,Crash Dump,DMP File
File002=debug.txt,Debug Information,Text Document
```

<a id="fmt-runtime-files"></a>
## [→ 29_data_formats_reference] 13. Runtime-written files

`<P>` = profile directory (`%LOCALAPPDATA%\Introversion\Multiwinia\` on Win2k+, cwd with `--NoProfile` or `TARGET_DEBUG`) [C] `code/app.cpp:App::GetProfileDirectory` (P14 §31.4).

### 13.1 Index

| File | Writer → reader | Format | Tag |
|---|---|---|---|
| `<P>preferences.txt` | `PrefsManager::Save` → `PrefsManager::Load` | §6 (UTF-16LE) | [B:FULL] |
| `<P>user_input_preferences.txt` | keybindings window → `parseInputPrefs` | §7.5 | [B:FULL] |
| `<P>authkey.dev`, `<P>authkey` | `Authentication_SaveKey` → `Authentication_LoadKey` | §13.4 | [B:FULL] |
| `<P>syncerror-<date>-client<c>-team<t>.txt` | `Location::DumpWorldToSyncLog` | §13.5 | [B:FULL] |
| `<P>syncdiff-<date>-client<c>-team<t>.txt` | `DumpSyncDiffs` | §13.5 | [B:FULL] |
| `<P>debug.txt` | `AppDebugOut` (`DUMP_DEBUG_LOG`) | free text log, truncated at start-up, appended per message | [B:FULL] |
| `<P>blackbox.txt` | `AppGenerateBlackBox` on release asserts/aborts | §13.8 | [B:FULL] |
| `<P>ERRORLOG.TXT`, `<P>CRASH.DMP` | `RecordExceptionInfo` (SEH handler) | free text / Windows minidump (binary) | [B:FULL] (P14 §31.13) |
| `<P>users/<profile>/game.txt`, `<P>users/<profile>/mission_*.txt` | `GlobalWorld::SaveGame`, `LevelFile::SaveMissionFile` → `LoadGame`, `ParseMissionFile` | §3/§2 with `redshirt2` obfuscation | [B:LEGACY] |
| `<P>maps/<name>.mwm` (+ `<P>maps/<name>.txt` from `LevelFile::Save`) | editor → `MapFile`, `LevelFile::ParseMWMFile` | §13.6 | [B:EDITOR] |
| `serverHistory.dat` (cwd) | `Server::SaveHistory` when pref `RecordDemo` = 1 → `Server::LoadHistory("ServerHistory.dat")` when `RecordDemo` = 2 | §13.7 (binary) | [B:FULL, debug feature] |
| `./origin` (read only) | packaging → `App::Initialise` | §13.9 | [B:FULL] |
| `./version` | `--write-version` switch → packaging | §13.9 | [B:FULL] |
| `./language_errors.txt` | `LangTable::TestAgainstEnglish` (pref `TextLanguageTest` = 1) | §13.10 | [B:FULL, QA] |
| `<P>inputprefs_debug.txt` | `parseInputPrefs` diagnostics | free text | Mac / [B:DEBUG] |
| `sounderrors.txt`, `data/sounds.txt` (save) | `SoundSystem::LoadtimeVerify` (call commented out), `SaveBlueprints` (sound editor) | sound formats (X1b) | [B:DEAD]/[B:DEBUG] |
| `data/gestures.txt` (save) | `Gesture::SaveTrainingData` (gesture editor window) | gesture format (X1b) | [B:DEBUG] |
| `<P>benchmarks/game <key> <time>.txt` | `Benchmark` | – | [B:DEAD] (`BENCHMARK_AND_FTP`) |
| `TestBedLog.txt`, `test_log.txt`, `gltrace.txt`, memory-leak report | testbed / test harness / D3D GL trace / `ParseMemoryLeakFile` | – | [B:DEAD] (respective macros) or debug tools |
| `<P>users\<profile>.dsg` | Vista rich saved-game header + PNG thumbnail | binary | [B:DEAD] (`TARGET_OS_VISTA`) |
| `achievements.txt` | `ACHIEVEMENT_RECORD_FILENAME` defined, `AchievementTracker::Save/Load` are empty | – | never written [C] `code/achievement_tracker.cpp` |

### 13.2 Preferences — see §6.

<a id="fmt-profile-save"></a>
### 13.3 Campaign save files (`users/<profile>/…`) [B:LEGACY] [C] `GlobalWorld::SaveGame`, `LevelFile::SaveMissionFile`, `contrib/systemiv/lib/filesys/text_file_writer.cpp:TextFileWriter`

* Grammar: exactly §3 (`game.txt`) and §2 (mission blocks `Difficulty`, `CameraMounts`, `CameraAnimations`, `Buildings`, `InstantUnits`, `Routes`, `PrimaryObjectives`, `RunningPrograms`, `LeaderboardStats`).
* Obfuscation (release builds; plain under `TARGET_DEBUG`): the file starts with the 9 bytes `redshirt2` **without a newline**, immediately followed by the first content line; every byte `c` with `33 ≤ c ≤ 127` is replaced by `c + K[i]` (wrapping by −95 if ≥ 128) with the 16-entry key table and running index of §1.3; bytes ≤ 32 and ≥ 128 are unchanged. The reader strips the header and inverts the mapping [C].
* Not written when the profile name is `none`, `AccessAllAreas` or `AttractMode`, or for non-full versions, or outside single-player modes [C] `App::SaveProfile`.

<a id="fmt-authkey"></a>
### 13.4 Authentication key files [B:FULL] [C] `contrib/systemiv/lib/metaserver/authentication.cpp:Authentication_SaveKey`, `Authentication_LoadKey`, `Authentication_SetKey`, `code/app.cpp:App::InitMetaServer`

```
authkey_file := KEY LF                  ; writer: fprintf("%s\n"); reader: fscanf("%s") = first whitespace-delimited word
KEY          := 5 groups "XXXXXX-XXXXXX-XXXXXX-XXXXXX-XXX" (31 chars = AUTHENTICATION_KEYLEN 32 − 1), upper-case
              | 'hsh' …                  ; a hashed key (accepted without local check)
```

* Read path: `<P>authkey.dev` only (`App::InitMetaServer`). Save paths: `<P>authkey.dev` (paste from clipboard in the auth page `GameMenuWindow`, demo-key button `PlayDemoAuthkeyButton`) and `<P>authkey` (typing in `AuthInputField`) — a typed key is therefore **not** re-read on the next start (P14 contradiction confirmed) [C].
* `fscanf("%s")` has no length limit (buffer `char[256]` in `App::InitMetaServer`); a missing file yields the sentinel text `authkey not found`. `Authentication_SetKey` strips trailing characters that are not `A–Z`, `a–z` or `-`. A key passes the local check if it is a hash key or has exactly 31 characters; prefixes `DEMO` (demo key) and `AMBR` (Ambrosia distribution) are recognised [C] `Authentication_SimpleKeyCheck`, `Authentication_IsDemoKey`, `Authentication_IsAmbrosiaKey`.
* `Authentication_GenerateKey` is a stub that copies a fixed demo-key literal (not reproduced here) [C].

<a id="fmt-sync-dump"></a>
### 13.5 Desync dump files [B:FULL] [C] `code/network/clienttoserver.cpp:ClientToServer::GetSyncFileName`, `ProcessServerLetters` (`NET_DARWINIA_SYNCERROR` branch), `code/location.cpp:Location::DumpWorldToSyncLog`, `code/network/iframe.cpp:DumpSyncDiffs`, `code/network/syncdiff.cpp:SyncDiff::Print`, `contrib/systemiv/lib/debug_utils.cpp:HashDouble`

File name: `<P><prefix>-%Y-%m-%d--%H-%M-%S-client<clientId>-team<myTeamId>.txt`, the time being the **server start time** (`m_serverStartTime`, local time) [C]. Prefix `syncerror` when the server reports this client out of sync (first report only); `syncdiff` when the I-frame comparison finds differences (`Multiwinia` I-frame diff path).

`syncerror` file (opened in append mode; with `TRACK_SYNC_RAND` [B:DEBUG] the sync-random log is written first, truncating):

```
syncerror_file := [ 'SyncRandSeed ' UINT ' (as signed int: ' INT ')' LF { randlog_line LF } ]   ; TRACK_SYNC_RAND only
                  LF LF 'Begin Final Object Positions' LF { state_line LF }
state_line     := ENT | OFFICER | DARWINIAN | WOBJ | BUILDING | TREE | CRATE | BOXKITE           ; per class LogState
ENT            := 'ENT Type[' TYPENAME '] Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] front[' H ']'
WOBJ           := 'WOBJ Type[' INT '] Id[' INT '] pos[' H ',' H ',' H '], vel[' H ']'
BUILDING       := 'BUILDING Type[' TYPENAME '] Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] front[' H '] centre[' H '], radius[' H ']'
TREE           := 'TREE Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] front[' H '] centre[' H '] radius[' H '] height[' H '] hitcheckCentre[' H '] hitcheckRadius[' H ']'
OFFICER        := 'ENT OFFICER Id[' INT '] state[' INT '] pos[' H ',' H ',' H '] vel[' H '] front[' H '] waypoint[' H ']'
DARWINIAN      := 'DARWINIAN Id[' INT '] state[' INT '] pos[' H ',' H ',' H '] vel[' H '] front[' H '] waypoint[' H ',' H ',' H '] boxId[' INT ']'
CRATE          := 'CRATE Id[' INT '] pos[' H ',' H ',' H '], vel[' H '] radius[' H ']'
BOXKITE        := 'BOXKITE Id[' INT '] state[' INT '] pos[' H '], vel[' H ']'
H              := 16 lower-case hex digits: the IEEE-754 bit pattern of a double, LEAST significant nibble FIRST
```

Order of state lines: for each used team, every entity of every unit, then every "other" entity; then every effect except snow; then every building [C] `DumpWorldToSyncLog`. A vector quantity summarised by one `H` is the hash of the **sum of its components** (`vel[x+y+z]`, `front[x+y+z]`) [C] `Entity::LogState`, `WorldObject::LogState`.

`syncdiff` file (truncating): one line per difference: `syncdiff: id: (team: T, unit: U, index: I, uniqueId: Q) (x, y, z)  - <name> - <description>` (positions as default `ostream` doubles) [C] `SyncDiff::Print`. Determinism context: [→ 22] (X3).

<a id="fmt-mwm"></a>
### 13.6 Custom map container `.mwm` [B:EDITOR] [C] `code/mapfile.cpp:MapFile::MapFile`, `MapFile::Save`, `SaveTextures`, `LoadTextures`, `code/mapfile.h`, `contrib/systemiv/lib/network_stream_inlines.cpp:WriteNetworkValue`

```
mwm_file   := UNCOMPRESSED_LEN zlib_stream
UNCOMPRESSED_LEN := int32 little-endian (WriteNetworkValue<int> on a little-endian host)
zlib_stream:= compress2(level 9) of a serialised Directory named "NewMap"
directory  := { "MapData" : bytes                      ; the map text exactly as LevelFile::SaveMapFile writes it (§2)
             , "<Tex>Filename" : string               ; Tex ∈ MapLandscapeTexture | MapWavesTexture | MapWaterTexture | MapThumbnail
             , "<Tex>" : bytes                        ; raw RGBA pixels (4 bytes/pixel, width·height·4)
             , "TextureHeight<n>" : int, "TextureWidth<n>" : int }   ; n = 1-based index in the order present
```

A texture is embedded only when it is not already an archived resource (`MapFile::IsSaveRequired`) and the thumbnail only when not `default.bmp`. Loading registers embedded textures under their stored file names (`Resource::AddBitmap`) and parses `MapData` with a `TextDataReader` (so an unterminated last line is dropped, §1.2). The `Directory` serialisation itself is the network message container ([→ 22]/[→ 29] network section, P15). Only editor builds create or list `.mwm` files (`GameMenu::CreateMapList` lists them under `LOCATION_EDITOR`), but `Location::LoadLevel` would load one in any build if a non-`.txt` map name were requested [C].

### 13.7 `serverHistory.dat` (server message recording) [C] `code/network/server.cpp:Server::SaveHistory`, `LoadHistory`

`int32 count` (raw, host order) followed by `count` serialised `Directory` messages (`Directory::Write(ostream)`). Written at game shutdown when pref `RecordDemo` == 1, replayed into the server history when `RecordDemo` == 2 (file name `ServerHistory.dat` — same file on case-insensitive file systems) [C] `code/app.cpp:App::ShutdownCurrentGame`, `code/main.cpp` (L693, L1334). No version header; replay requires byte-identical code [I].

### 13.8 `blackbox.txt` [C] `contrib/systemiv/lib/debug_utils.cpp:AppGenerateBlackBox`

```
'=========================' LF '=   BLACK BOX REPORT    =' LF '=========================' LF LF
APP_NAME ' ' APP_VERSION ' built ' __DATE__ LF
'Date ' H ':' M ', ' D '/' MON '/' YYYY LF LF
[ "ERROR : '" message "'" LF ]
LF '=========================' LF '=      STACKTRACE       =' LF '=========================' LF LF
{ 'retAddress = ' %p LF }                ; EBP-chain walk (Win32/Linux), stops on misalignment or bad pointers
```

### 13.9 `origin` and `version` [C] `code/app.cpp:App::Initialise`, `code/main.cpp` (`--write-version`, L1571)

`origin` (cwd, optional, read only): first line via `TextFileReader` + `GetRestOfLine()` → `App::m_originVersion` (distributor tag sent to the metaserver; default `unknown`); comments/CR stripped, trailing blanks kept. `version`: written by the `--write-version` command-line switch as the bare `DARWINIA_VERSION` string (no newline), after which the program returns immediately; used by the packaging scripts.

### 13.10 `language_errors.txt` [C] `code/lib/language_table.cpp:LangTable::TestAgainstEnglish`

Truncated at each language load when pref `TextLanguageTest` = 1; one line per finding: `ERROR : Failed to find translation for string ID '<id>'`, `ERROR : String ID appears not to be translated : '<id>'` (text identical to English), `ERROR : Found new string ID not present in original English : '<id>'`. (A second writer in `code/sepulveda.cpp` appends to the same file under `USE_SEPULVEDA_HELP_TUTORIAL` [B:DEAD].)

## [→ 29_data_formats_reference] 14. Re-implementation guidance (format layer of the new game)

* Keep **one** tokeniser (§1) for all legacy-compatible text formats, but fix its traps: process an unterminated last line in every reader, accept UTF-8 (with or without BOM) in addition to UTF-16, add a quoting rule for free-text values, and never return a NULL token to a parser — every block parser must check token availability and report `file:line: expected <field>` instead of crashing.
* Make block parsers **table-driven** (keyword → field descriptor with type, default, range), so that the menu scanner and the level loader share one definition (the original has two divergent parsers, §2.4.3).
* Separate shipped content from save data: no `redshirt2` obfuscation; versioned headers on every runtime-written file; preferences written atomically with typed values and full float precision.
* Treat every name used as an identity (`GAMETYPE` names, building/entity/research type names, phrase ids, control names) as a stable *id* distinct from display text, and validate cross-references at load time (mount names, building link ids, objective location names).
* For determinism, parse numeric simulation fields with an explicit, platform-independent decimal parser and round to the simulation's numeric type (the original's F32 rounding, §Conventions).

## Cross-partition notes

- [→ 23_terrain_and_landscape] P11: semantics of `Landscape`, `LandscapeTiles` (guide grid), `LandFlattenAreas` fields; this document fixes their syntax (§2.5–§2.7).
- [→ 03_buildings_and_world_objects] P06/P07/P08/P02: behaviour of every building column in §2.9.3; the shipped row counts per type are in `X1a_level_field_usage.csv` (P06 has an equivalent `P06_level_building_usage.csv`).
- [→ 01_game_modes] P02: meaning and units of every `multiwinia.txt` parameter and of the `LevelOptions` (§2.4.4, §4.3).
- [→ 06_player_control_and_input] P10: run-time semantics of binding specs; P10 §1.11 grammar is consistent with §7.3 here, which adds the case-sensitivity rules, the thumbstick dual type, the `STATE_*` results and the shipped-file statistics.
- [→ 09_meta_systems] P17b/P14: the preference-key register (`P17b_prefs_accesses.csv`, `P14_platform_lib_pref_keys.csv`); §6 gives only the file format and the default sources.
- [→ 30_scripting_tutorial_and_cutscenes] P19: command semantics for §9.
- [→ 22_determinism_and_networking] P15/X3: `Directory` serialisation used by `.mwm` and `serverHistory.dat` (§13.6–§13.7); sync dump formats (§13.5) are the forensic output of the sync check.
- X1b (data formats part B): `sounds.txt`, `effects.txt`, `stats.txt`, `gestures.txt`, `.anim`, `.shp`, bitmaps, `.dat` archives, colour ramps are not covered here.
- P18: the credits page consumes `multiwinia_betatesters.txt` (§5) and `GameMenu::AddMap` builds the map list (§2.4.3).
- P14 correction: archive lookups are **case-insensitive** — `Resource::m_resourceFiles` is a `HashTable` (case-folding hash + `stricmp`) and SystemIV `FileSystem` uses a `BTree` with `stricmp` [C] `code/lib/resource.h`, `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::HashFunc`, `contrib/systemiv/lib/tosser/btree.cpp:BTree::LookupTree`; P14 §31.5.3/§31.5.4 state that callers must pass lower-case names / that the lookup is case-sensitive.

## Contradictions / open questions

1. P14 §31.5.3 vs code: archive lookup case sensitivity (see Cross-partition notes) — the code is case-insensitive [C].
2. `CameraAnimations` parsing comment says mission lights override map lights; `ParseMissionFile` rejects `Lights` blocks (§2.8) [C].
3. `level_enum.txt` is excluded from the retail `main.dat` although `AchievementTracker` needs it for the "Master of Multiwinia"/"Explorer" achievements — either the retail game ships it elsewhere or those achievements could never fire [U].
4. The auth key typed in the auth field is saved to `<P>authkey` but read from `<P>authkey.dev` (§13.4) — confirmed contradiction (also noted by P14).
5. Partial `japanese.txt`/`korean.txt`/`portuguese.txt` would show raw phrase ids for ~90 % of the UI (no English fallback) — either these languages were never selectable in a retail edition or complete files existed outside this tree [U].
6. `NetworkServerPort`/`NetworkClientPort` (live) coexist with the dead `ServerAddress`/`ServerPort`/`BypassNetwork` defaults — the latter are Darwinia leftovers [C].

## [U] Unknowns

1. Whether `level_enum.txt` reached retail installs (resolve: list a retail `main.dat`/`patch.dat`).
2. Whether retail editions shipped complete Japanese/Korean/Portuguese language files (resolve: retail `language.dat` and `tools/*language.list`).
3. Whether `default_preferences.txt` is reachable at first run in a packaged install (`PrefsManager` is created before `main.dat` is parsed) — affects first-run `WaitVerticalRetrace`, `NetworkClientPort` etc. (resolve: check `tools/init.list` against a retail `init.dat`, or run a clean install). P14 rates the "not found" outcome [I, strong].
4. Exact failure mode of a NULL token in release (`atoi(NULL)` access violation vs CRT invalid-parameter termination for `stricmp`) — immaterial for a re-implementation (resolve: run the original with a truncated file).
5. Whether `MultiwiniaSupport.exe` in `tools/multiwiniasupport/` was built from `contrib/XCrashReport` unchanged (resolve: compare resources/strings of the binary).

## Docs-vs-code discrepancies

| Doc | Doc says | Code does |
|---|---|---|
| `docs/input_preferences_syntax.txt` | grammar without `~` icon lines, `value`/`alias` drivers, `repeat`, `key any`/`mouse any`, thumbstick directions, `lefttriggerb`/`righttriggerb` | all of these are accepted (§7.3) [C] |
| `docs/input_preferences_syntax.txt` | "A preference value is always false if there is no corresponding preference manager entry" | `pref X` with a non-existent key **fails to parse** (binding rejected) [C] `PrefsInputDriver::parseInputSpecification` |
| `docs/input_preferences_syntax.txt` | user prefs override the FIRST binding of the system file | true for each user line, but several user lines for one control overwrite each other (replace semantics) [C] P10 §1.10 |
| `docs/top level map.txt` | "Each level file specifies a map file at the start"; global data has "X Z position" per map | map/mission pairing is given by `game.txt` `Locations` rows; `locations.txt` stores X Y Z [C] §3 |
| `docs/changes.txt` (Darwinia Steam notes) | default language read from `HKCU\software\valve\steam\language` | Multiwinia uses `GetLocaleInfo(LOCALE_SENGLANGUAGE)`; no registry read in `code/` [C] §8.1 |
| `data/multiwinia.txt` header comment | `Change: 0 = DropDownMenu, -1 = CheckBox` | consistent with `GameMenuWindow` [C] |
| `data/language/english.txt` comment | `*BETATESTERS … is replaced by all the names of the beta testers` | consistent; requires the phrase to be exactly `*BETATESTERS` [C] `ReadCredits` |

## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Darwinia", "Darwinian(s)", "Futurewinian", "Rocketwinians" | name/brand | `data/multiwinia.txt`, crate names `code/worldobject/crate.cpp:Crate::GetName`, level options | "the game", "Sprite", "Future Sprite", "Rocket Sprites" |
| Game-type names `Domination`, `KingOfTheHill`, `CaptureTheStatue`, `Assault`, `RocketRiot`, `Blitzkrieg`, `TankBattle` (as presented in this specific set) | name | `data/multiwinia.txt` | `ZoneControl`, `HoldTheCentre`, `CarryTheIdol`, `AttackDefend`, `RocketRace`, `ZoneChain`, `ArmourMatch` |
| Map file names `mp_*` and location names (`garden`, `containment`, `mine`, `generator`, `yard`, `escort`, `pattern_buffer`, `biosphere`, `receiver`, `temple`, `launchpad`, `sandbox`) | name | `data/levels/`, `data/game.txt` | `map_<mode>_<players>p_<n>`, `region_<n>` |
| Every phrase text in `data/language/*.txt` (UI strings, credits, help) | text | `data/language/` | write new text; keep only the id scheme |
| Beta-tester and credit names | text (personal names) | `data/betatesters.txt`, `data/multiwinia_betatesters.txt`, `multiwinia_credits_*` | never reuse; generate own credits |
| Dr Sepulveda / `puritycontrol_*` dialogue, `buyme` cutscene | character/text | `data/purity/buyme.txt`, `code/sepulveda.cpp` | none (do not implement) |
| Building/entity/research type names used as file keywords (`SpawnPointMaster`, `TrunkPort`, `SoulDestroyer`, `Virii`, `Triffid`, …) | name | §2.9.3, §2.12, §3.3 | new neutral ids (`SpawnHub`, `Portal`, …) |
| Asset names referenced from level files (`rockhead.shp`, `landscape_launchpad.bmp`, `waves_*.bmp`, `water_*.bmp`) | art | level files | new asset names |
| Value-driver names `Pecan`, `Hummingbird`; test-bed name `TESTBED_GodOfCode`; profile names `AccessAllAreas`, `AttractMode` | name | `code/lib/input/inputdriver_values.inc`, `testbedconfig.txt`, `code/app.cpp` | `AlwaysTrue`, `FeatureFlagA`, `DevProfile` |
| Obfuscation header `redshirt2` and its key table | brand-ish/secret | `text_stream_readers.cpp`, `text_file_writer.cpp` | none (use no obfuscation) |
| Folder/bundle names `Introversion\Multiwinia`, `uk.co.introversion.*` | brand | `App::GetProfileDirectory`, `App::GetPreferencesPath` | `<Company>\<Game>` |
| Auth-key format and demo-key literal | brand/credential | `contrib/systemiv/lib/metaserver/authentication.cpp` | own licensing scheme |
| Crash tool name `MultiwiniaSupport` | brand | root INI, `contrib/XCrashReport` | `<Game>Support` |

## Appendix — dead/legacy code

| Item | Tag | Ref |
|---|---|---|
| `game_demo.txt` / `game_demo2.txt` selection (`DEMOBUILD`, `DEMO2`) | [B:DEAD] | `code/app.cpp` `GAMEDATAFILE` |
| Whole campaign world loader (`game.txt`, `game_unlockall.txt`, `locations.txt`, missions, `map_*`) | [B:LEGACY] | §3, §2.1 |
| `Tutorial_StartDefinition` section, `RunScript <phrase-id>` sequences, `SetCutsceneMode` | [B:DEAD] (`USE_SEPULVEDA_HELP_TUTORIAL`) | `GlobalWorld::ParseTutorial`, `Script::RunScript` |
| `multiwinia-testbed.txt`, `testbedconfig.txt`, `TestBedLog.txt` | [B:DEAD] (`TESTBED_ENABLED`) | §4.1, §11 |
| `data/purity/*`, `Script::RunCommand_PurityControl` | [B:DEAD] data / [B:LEGACY] command | §10 |
| `input_preferences_darwinia.txt` (needs `MultiwiniaControlMethod` = 0, forced to 1) | [B:LEGACY] | §7.1 |
| `input_preferences_mac.txt` | Mac only | §7.1 |
| Mod overlay paths `<P>mods/<mod>/…` | [B:DEAD] (`USE_DARWINIA_MOD_SYSTEM`) | `Resource::GetTextReader` |
| SystemIV `FileSystem` (compiled, never instantiated) | [B:FULL] unreachable | §1.1 |
| `Script::TestScript` | [B:DEAD] (`SCRIPT_TEST_ENABLED`) | §9 |
| `CreditsLoader::GenerateBetaTesterList` (`betatesters.txt`) | [B:LEGACY] | §5 |
| Benchmark output, Vista `.dsg`, gesture/sound editor saves, `achievements.txt` | [B:DEAD]/[B:DEBUG]/never written | §13.1 |
| `LevelFile::WriteRunningPrograms`, `WriteLeaderboardStats`, `GlobalWorld::SaveLocations` (malformed `%-8.2` output) | [B:LEGACY]/[B:EDITOR] | §2.15, §3.4 |
| Building types blocked by `BLOCK_OLD_DARWINIA_OBJECTS` (39) and entity types blocked by `EntityIsBlocked` (16) | [B:LEGACY] (lines ignored) | §2.9.3, §2.12 |
