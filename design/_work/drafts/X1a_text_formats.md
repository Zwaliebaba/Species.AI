# X1a_text_formats — Text data formats (formal grammars, part A)
Scope: `data/levels/**`, `data/purity/*`, `data/game*.txt`, `data/locations.txt`, `data/multiwinia*.txt`, `data/*betatesters.txt`, `data/default_preferences.txt`, `data/input/**`, `data/language/*`, `data/scripts/*`, root `testbedconfig.txt`, `MultiwiniaSupport.ini`, plus every parser that reads them and every runtime-written text file found on the way. Build baseline: see build_matrix.md (`Release|Win32` + `TARGET_PC_FULLGAME`).

Conventions used in every grammar below:

* EBNF: `a b` sequence, `a | b` alternative, `[ a ]` optional, `{ a }` zero or more, `'lit'` literal (case-insensitive unless marked **(cs)** = case-sensitive), `EOL` end of a physical line.
* All grammars are stated **after** the shared line-cleaning and tokenising pass of §1 (`fmt-textreader`). "Token" means a maximal run of non-separator characters on a cleaned line.
* `INT` = whatever C `atoi` accepts (optional sign, decimal digits, leading junk stops the scan, no error reporting; an unparsable token yields 0). `FLOAT` = whatever C `atof` accepts (decimal, optional exponent). `F32` = `FLOAT` then rounded to IEEE-754 single precision and widened back to `double` (`iv_atof` ≡ `(double)(float)atof(s)`, `contrib/systemiv/lib/math/math_stubs.cpp:iv_atof`, `iv_trunc_low_nibble`) [C]. The F32 rounding is a determinism measure; a re-implementation must replicate it for any value that feeds the simulation.
* "MISSING ⇒ CRASH" means the parser calls `GetNextToken()` without checking `TokenAvailable()`; a short line gives a `NULL` token which is passed to `atoi`/`atof`/`strcpy`/`stricmp` → access violation or CRT invalid-parameter termination [C: code path; I: exact failure mode is MSVC CRT behaviour; no `_set_invalid_parameter_handler` exists in `code/` or `contrib/systemiv/lib`].

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
3. Archive entry `data/<name>` (exact, case-sensitive key; archive keys are lower-cased by `Resource::ParseArchive`, so callers must pass lower-case names to hit an archive) — `UnicodeTextDataReader`, else `TextDataReader`.
4. Otherwise `NULL` and the debug log line `Failed to find text resource: <path>`.

A few callers bypass `Resource` and construct `TextFileReader`/`UnicodeTextFileReader` on an absolute or cwd-relative path directly (preferences, profile saves, `testbedconfig.txt`, `origin`, input bindings): they are listed per format. `contrib/systemiv/lib/filesys/file_system.cpp:FileSystem::GetTextReader` is compiled but `g_fileSystem` is never created by Multiwinia [C] (grep: no `new FileSystem` under `code/`) — ignore it.

`TextFileReader` and `UnicodeTextFileReader` call `FindCaseInsensitive(path)` before opening (`contrib/systemiv/lib/filesys/filesys_utils.cpp`) so loose files are matched case-insensitively on case-sensitive file systems [C]; on Win32 this is a no-op in effect [I].

### 1.2 Physical lines

* A line ends at LF (`0x0A` / `L'\n'`). The final line may lack an LF. Lines may be arbitrarily long: the buffer starts at 128 characters and doubles; a line longer than 65 536 characters triggers `AppReleaseAssert("Text file contains line with more than 65536 characters")` [C] `TextReader::DoubleMaxLineLen`, `UnicodeTextReader::DoubleMaxLineLen`.
* End of file: only `TextFileReader` processes an unterminated last line (its `ReadLine` returns `true` for a final line without LF and `false` on the following call). `TextDataReader` (archive), `UnicodeTextFileReader` and `UnicodeTextDataReader` load the unterminated last line but return `false` for it, so the `while (in->ReadLine())` loop body **never sees it** [C] `TextDataReader::ReadLine` (`eof = true` when no `\n` is found), `UnicodeTextFileReader::ReadUnicodeLine` (returns `NULL` as soon as `feof`), `UnicodeTextDataReader::ReadLine`. Rule for authors: **always end a text file with a newline** (every shipped file in scope does — survey §1.6).
* A U+0000 code unit in a UTF-16 disk file is skipped without being stored (the slot keeps stale data) [C] `UnicodeTextFileReader::ReadUnicodeLine` (`temp != NULL` test) — never emit NUL.

### 1.3 Line cleaning (`TextReader::CleanLine`, `UnicodeTextReader::CleanLine`) — applied to every line, in this order [C]

1. **De-obfuscation.** On the first line only, if the line starts with `redshirt2` (case-insensitive, 9 chars) the file is marked encrypted and those 9 characters are removed. For every subsequent character `> 32` in an encrypted file: `i = (i+1) mod 16; c' = c − K[i]; if c' < 33 then c' += 95` with `K = {31,7,9,1,11,2,5,5,3,17,40,12,35,22,27,2}` and `i` starting at 0 and persisting across lines. Characters `≤ 32` (space, tab, CR, LF) are untouched. The writer side is `TextFileWriter(..., encrypt=true)` / `UnicodeTextFileWriter` (`c' = c + K[i]; if c' ≥ 128 then c' −= 95`) [C] `code/unicode/unicode_text_file_writer.cpp:UnicodeTextFileWriter::printf`. No shipped file under `data/` is encrypted (survey: no file starts with `redshirt2`) [C] `X1a_text_survey.py`; encryption is used only for runtime-written profile saves (§13).
2. **Comment removal.** Scan left to right; the first `#` and everything after it is deleted. There is **no escape and no quoting**: a `#` inside what looks like a string still starts a comment. While scanning, runs of `<` / `>` are counted and three adjacent angle brackets trip `AppDebugAssert` (merge-conflict detector; debug builds only — [B:DEBUG]).
3. **Line-end removal.** Truncate at the first `\n`, then truncate at the first `\r`. Consequence: a stray CR in the middle of a line silently discards the rest of that line.

Edge case (UTF-16 disk files): `UnicodeTextFileReader::ReadLine` converts the wide line to the narrow mirror buffer (`UnicodeString::CopyUnicodeToCharArray`, every code unit > 255 becomes `?`) **before** `CleanLine` runs on the wide buffer, so the narrow mirror still contains the comment text; `UnicodeTextDataReader::ReadLine` cleans first and converts afterwards [C]. Tokenising of the first token is done on the narrow mirror (`TextReader::GetNextToken` is not virtual) while `TokenAvailable`/`GetNextUnicodeToken`/`GetRestOfUnicodeLine` are overridden and use the wide buffer. Because both buffers have identical indices (1 code unit ↔ 1 byte), this only matters for a line whose *first* token is followed by `#`: the narrow first token is still correct (a comment can only start after a separator or at the token start, see below) [I]. A line that is entirely a comment is correctly skipped because `TokenAvailable` looks at the cleaned wide buffer [C].

### 1.4 Tokens

* Separator set (default): space, TAB, LF, CR, `:` and `,` — `" \t\n\r:,"` for the narrow readers; the Unicode readers additionally treat form-feed `\f` as a separator (`L" \f\t\n\r:,"`) [C] `DEFAULT_SEPERATOR_CHARS` in both `.cpp` files. A caller may replace the set with `SetSeperatorChars` (max 15 characters) — only the input-binding parser does (§7).
* `GetNextToken()`: skip separators; if end of line → `NULL`; else return the maximal run of non-separators and overwrite the following separator with NUL. `:` and `,` are therefore exactly equivalent to whitespace — `BuildingOnline :mine,90` is the three tokens `BuildingOnline`, `mine`, `90`.
* `GetRestOfLine()`: skip separators, return the remainder of the line **verbatim** (internal separators, trailing spaces and tabs preserved — there is no right-trim) or `NULL` at end of line. Because of the leading skip, a "rest of line" value can never begin with space, tab, `:` or `,`.
* `TokenAvailable()`: true iff a non-separator character remains on the line.
* There is **no quoting, no escaping and no line continuation** at this level. Individual formats add their own conventions (the language table's `\n`, §9).
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

### 1.6 Encoding survey of the files in scope [C] `X1a_text_survey.py`

See the coverage table at the end of this document and the per-format sections; summary: every file is single-byte ASCII/Latin-1 with CRLF line ends **except** 8 of the 9 language files (UTF-16 with BOM: big-endian French, German, Italian, Korean, Portuguese, Spanish; little-endian Japanese, Russian). `data/betatesters.txt` and `data/multiwinia_betatesters.txt` contain Latin-1 bytes (accented names) which pass through the narrow reader unchanged and are then widened byte-for-byte by `UnicodeString(char const*)` (each byte becomes the code unit of the same value, i.e. ISO-8859-1) [C] `contrib/systemiv/lib/unicode/unicode_string.cpp:UnicodeString::UnicodeString(char const*)`. The reverse conversion (`CopyUnicodeToCharArray`) maps every code unit > 255 to `?` [C].

### 1.7 Re-implementation notes

* Implement one tokeniser with: BOM sniffing (UTF-16LE/BE, else bytes-as-Latin-1), optional `redshirt2` de-obfuscation (only needed to read legacy saves), `#` comments, first-CR truncation, the separator set above, `next_token`, `rest_of_line`, `token_available`, a per-reader separator override, and EOF semantics that *do* process an unterminated last line (fixing the archive asymmetry).
* Keep the `F32` rounding helper for numeric fields that feed simulation.

<a id="fmt-level-file"></a>
## [→ 29_data_formats_reference] [→ 08_levels_and_level_design] 2. Level files (`map_*.txt`, `mp_*.txt`, `mission_*.txt`)

### 2.1 Purpose, files, build reality

A level is described by **one map file** (terrain, lights, static buildings, Multiwinia options — and, in Multiwinia, also units/camera data) plus an optional **mission file** (dynamic buildings, units, objectives, camera data; Darwinia campaign only). `LevelFile(missionName, mapName, textFileLevel)` parses the mission file first, then the map file [C] `code/level_file.cpp:LevelFile::LevelFile`. `Location::LoadLevel` passes `textFileLevel = (extension of map name == "txt")`; any other extension is loaded as a `.mwm` container (§13.6) [C] `code/location.cpp:Location::LoadLevel`. In Multiwinia `m_requestedMission` stays `"null"` (set in `App::Initialise`) so only the map file is read [C] `code/app.cpp:App::Initialise`, [I] for every network-started game (the server sends only the map name — `code/network/clienttoserver.cpp` sets `m_requestedMap`).

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
| `PlayTime` | INT | minutes | −1 | shipped 5, 10, 15, 20 | default match length shown/used by the lobby [C] `GameMenu::AddMap` (`m_playTime`) |
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

