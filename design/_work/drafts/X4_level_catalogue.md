# X4_level_catalogue — Level catalogue (`data/levels/**`, `data/thumbnails/**`)
Scope: every file in `data/levels/` (88 entries: 57 `mp_*.txt`, 12 `map_*.txt`, 17 `mission_*.txt`, `level_enum.txt`, dir `old/`), `data/levels/old/` (30 `mp_*.txt`), `data/thumbnails/` (57 `.bmp`); plus the code that enumerates/permits maps (`code/game_menu.cpp:GameMenu::CreateMapList/AddMap`, `code/UI/MapData.*`, `code/UI/LevelSelectButton.h`, `code/UI/MapImageButton.h`, `code/UI/GameMenuWindow.cpp`, `code/app.cpp:IsMapPermitted/GetMapID`, `code/mapcrc.h`, `code/lib/universal_include.h` `PERMITTED_MAPS`, `code/network/demo_limitations.cpp`, `code/achievement_tracker.cpp`), the packaging lists `tools/multiwinia_exclusions*.txt`, `tools/multiwinia_inclusions_*.txt`, `tools/build_multiwinia_data.bat`, and the Darwinia references in `data/game*.txt`, `data/purity/game.txt`, `data/scripts/*.txt`, `data/locations.txt`. Build baseline: see `build_matrix.md` (`Release|Win32` + `TARGET_PC_FULLGAME`, `BLOCK_OLD_DARWINIA_OBJECTS` on, `INCLUDEGAMEMODE_TANKBATTLE` off, `LOCATION_EDITOR` off).

Ownership boundaries: X1a owns the formal level-file grammar (EBNF); P11 owns the parser, loading sequence, terrain generation, MapCRC and `MultiwiniaOptions` field semantics (`P11_terrain_world.md` §8.1–8.8, §23); P02 owns mode rules (`P02_game_modes.md`). This draft is the **catalogue**: which levels exist, what each contains, which build can see them, and the conventions designers followed.

Generated artefacts (all re-runnable, `python3 -I`, paths as arguments):

| Artefact | What | How produced |
|---|---|---|
| `OUT/_work/scripts/X4_levels.py` | parser modelled on `code/level_file.cpp:LevelFile::ParseMapFile/ParseMissionFile/ParseMultiwiniaOptions`, `code/game_menu.cpp:AddMap/CreateMapList`, `code/worldobject/building.cpp:Building::Read/BuildingIsBlocked`, CRC per `LevelFile::CalculeCRC` | `python3 -I X4_levels.py <SRC_ROOT> <OUT>/_work/drafts --summary` |
| `drafts/X4_levels.csv` | **116 rows** (86 top-level level files + 30 `old/`), 125 columns: identity, CRC/MAPID check, `level_enum` id, English name, sections, pairing, references, every `MultiwiniaOptions` key, landscape + tile/flatten ranges, lights, building totals and mode-object counts per start position, camera mounts/animations, instant units, routes, objectives, difficulty, leaderboard, thumbnail, data-shipping per build, `PERMITTED_MAPS` per target, demo default, tutorial use | X4_levels.py |
| `drafts/X4_level_buildings.csv` | **828 rows**: level × building type → count, per-position split, known/blocked/forced-global flags | X4_levels.py |
| `drafts/X4_thumbnails.csv` | 57 rows: thumbnail → level, BMP format, shipping per build | X4_levels.py |
| `drafts/X4_mode_maplists.csv` | 102 rows: per game type, menu-listed maps in menu order (all-in-tree and retail data sets) | X4_levels.py |
| `OUT/_work/scripts/X4_md_tables.py` | renders the markdown tables in this draft from the CSVs | `python3 -I X4_md_tables.py <OUT>/_work/drafts <table>` |

Cross-checks: building counts equal `drafts/P06_level_building_usage.csv` for every (type, top-level file) pair (0 differences) [C]; every top-level file's CRC equals its `MAPID_*` in `code/mapcrc.h` (86/86 level files + `level_enum.txt` per P11 = 87/87) [C].

---

## [→ 08_levels_and_level_design] 1. Inventory, file kinds and how maps are found

### 1.1 Counts and kinds [C] (directory listing)

| Location | Kind | Count | Build tag | Notes |
|---|---|---|---|---|
| `data/levels/mp_*.txt` | Multiwinia map (single file) | 57 | [B:FULL] (49 ship in retail data, see §3) | 6 mode families + `mp_skirmish_*` (=Domination), `mp_sandbox*`, `mp_benchmark_1`, `mp_assault_stress`, `mp_tankbattle_*` |
| `data/levels/map_*.txt` | Darwinia location map | 12 | [B:LEGACY] (excluded from retail data) | biosphere, containment, escort, garden, generator, launchpad, mine, pattern_buffer, receiver, sandbox, temple, yard |
| `data/levels/mission_*.txt` | Darwinia mission overlay | 17 | [B:LEGACY] (excluded from retail data) | paired by name prefix with a `map_` file (§11) |
| `data/levels/level_enum.txt` | achievement level index | 1 | [B:FULL] code, **excluded from retail data** (§13) | 59 `name id` pairs; ids 50–59 name PDLC maps absent from the tree |
| `data/levels/old/mp_*.txt` | superseded Multiwinia maps | 30 | never loaded by any build (§12) | not listed by the non-recursive scan; `levels/old/*` excluded from every data build |
| `data/thumbnails/*.bmp` | map-select thumbnail | 57 | [B:FULL] | 56 `mp_*` + `default.bmp` (§10) |

### 1.2 File pairing [C]
- **Multiwinia maps are single files.** Every Multiwinia start sets the mission to `"null"` (`code/network/clienttoserver.cpp` `NET_DARWINIA_START_GAME` handler: `strcpy(g_app->m_requestedMission, "null")`; also `code/interface/multiwinia_window.cpp` Play/Edit buttons). `LevelFile::LevelFile` parses a mission file only when the name is not `"null"`, then `ParseMapFile` — so an `mp_*` file carries both "map" blocks (options, landscape, tiles, flatten areas, lights, static buildings) and the "mission" blocks the map parser also accepts (camera mounts/animations, instant units, primary objectives) [C] `code/level_file.cpp:LevelFile::LevelFile`, `ParseMapFile(TextReader*)`.
- **Darwinia levels are pairs** `map_<location>.txt` + `mission_<location>_<name>.txt`; the pairing is by location name (Darwinia global world `Locations` table `id avail mapFile missionFile`, and `GlobalWorld::LoadGame` lists `mission_<location>*.txt`) [C] `code/global_world.cpp:GlobalWorld::LoadGame`, `ParseLocations`. 11 maps have ≥1 mission; `map_launchpad` pairs with `mission_launchpad.txt` (no `_name` suffix).

### 1.3 Two on-disk section layouts of `mp_*` files [C] data, [I] origin

| Layout | Block order | Files | Origin |
|---|---|---|---|
| A ("old writer") | `MultiwiniaOptions, Landscape, LandscapeTiles, LandFlattenAreas, Lights, Buildings, CameraMounts, CameraAnimations, Buildings(empty), InstantUnits, PrimaryObjectives` | 47 top-level + all 28 `old/` files that have options | an earlier `SaveMapFile` that also wrote camera animations, dynamic buildings and objectives [I] |
| B ("current writer") | `MultiwiniaOptions, Landscape, LandscapeTiles, LandFlattenAreas, Lights, Buildings, CameraMounts, InstantUnits` | 10: `mp_assault_2p_3`, `mp_assault_3p_1`, `mp_assault_3p_3`, `mp_assault_4P_2`, `mp_blitzkrieg_4P_2`, `mp_domination_2P_1`, `mp_domination_4P_1`, `mp_koth_2P_3`, `mp_koth_2P_3b`, `mp_skirmish_3P_1` | exactly what `LevelFile::SaveMapFile` emits today when `g_app->m_multiwinia` is set (camera-anim, dynamic-building and objective writers are commented out) [C] `code/level_file.cpp:SaveMapFile` |

Forensic corollary [C]: the same 10 files — and only they — carry `Name NewLevel` and an explicit `DefenderPopulationCap` line, because `WriteMultiwiniaOptions` always writes `Name` (default `"NewLevel"`) and always writes `DefenderPopulationCap` [C] `WriteMultiwiniaOptions`. Layout A files have `PrimaryObjectives NeverTrue` (or nothing); the second, empty `Buildings_StartDefinition` block is parsed as **static** buildings (the map parser's first `Buildings` branch wins) and contains no rows in any shipped file [C] (column `buildings_in_2nd_block` = 0 everywhere).

### 1.4 How the menu finds maps [C] `code/game_menu.cpp:GameMenu::CreateMapList`, `AddMap`

1. `Resource::ListResources("levels/", "mp_*", false)` — lists `data/levels/` on disk plus pre-loaded archive entries whose directory part equals `data/levels/` exactly; **non-recursive**, so `levels/old/` is never listed [C] `code/lib/resource.cpp:ListResources`. Results are merged with `OrderedInsert` (strcmp ascending, duplicates dropped; its loop stops at `Size()-1`, so a name sorting after every entry but the last is appended at the end — order is "nearly alphabetical") [C] `contrib/systemiv/lib/filesys/file_system.cpp:OrderedInsert`.
2. Only `.txt` files are opened (`GetExtensionPart == "txt"`); `AddMap` reads lines until `MultiwiniaOptions_EndDefinition` with **case-sensitive** key matching (`strcmp`) for `Name`, `GameTypes`, `NumPlayers`, `Author`, `Description`, `PlayTime`, `Difficulty`, `CoopMode`, `ForceCoop`, `OfficialLevel` [C].
3. `GameTypes`: **every** token that exactly equals a blueprint name (`Domination`, `KingOfTheHill`, `CaptureTheStatue`, `Assault`, `RocketRiot`, `Blitzkrieg`, `TankBattle` — order of `GAMETYPE` blocks in `data/multiwinia.txt`) adds the map to `m_maps[mode]`. (In contrast `LevelFile::ParseMultiwiniaOptions` keeps only the **first** token.) A file with no matching token is discarded with "Warning: Discarded map %s because it didn't contain a game type" [C].
4. `MapData::CalculeMapId()` = `LevelFile::CalculeCRC(fileName)`; −1 (file unreadable as a resource) marks `m_customMap` [C] `code/UI/MapData.cpp`.
5. `AddMap` returns `m_officialMap`; the count of official maps goes to `AchievementTracker::SetNumLevels` [C].
6. Per mode, a stable bubble sort orders maps by `NumPlayers` ascending, then difficulty rank (substring `basic`=1, `inter`=2, `advan`=3, absent=0) [C]. Resulting retail order: §9.1.
7. Under `LOCATION_EDITOR` only, `<profile>/maps/*.mwm` custom maps are added the same way [B:EDITOR] [C].

Downstream identity uses [C]: `App::GetMapID(mode, crc)` → index into `m_maps[mode]`; `GameMenu::GetMapName(mode, crc|file)` → string id `<stem>_name` (`GetMapNameId` strips `.txt`, appends `_name`; `GetMapDescriptionId` appends `_description`) — looked up in the language table, whose `HashTable` hashes with the case bit masked (`& 0xDF`) and compares with `stricmp`, so `mp_assault_3p_1_name` finds the key `mp_assault_3P_1_name` [C] `contrib/systemiv/lib/tosser/hash_table.cpp:HashFunc`, `GetIndex`. The level's own `Name` option is shown only for custom maps (`m_customMap`) [C] `code/UI/LevelSelectButton.h:Render`. `PlayTime` is shown as `"0:%02d"` (hours:minutes) and copied into the mode's time option on selection; 0 → blueprint default [C] `LevelSelectButton::MouseUp/Render`.

Thumbnail lookup [C] `code/UI/MapImageButton.h:Render`, `code/UI/GameMenuWindow.cpp:PreloadThumbnails`: `thumbnails/<file stem>.` + `TEXTURE_EXTENSION` (`"bmp"`), passed through `strlwr`; if `DoesTextureExist` fails, `thumbnails/default.bmp`. Archive entry names are lower-cased when `.dat` archives are indexed (`strlwr(file->m_filename)` in `code/lib/resource.cpp`), so mixed-case thumbnail files resolve [I]. The `LevelFile::m_thumbnailFilename` field (default `"default.bmp"`) is used only by the `.mwm` writer [C] `code/mapfile.cpp`.

## [→ 08_levels_and_level_design] 2. Master catalogue of the Multiwinia maps (`data/levels/mp_*.txt`, 57 files)

Columns: CRC = `LevelFile::CalculeCRC` value, which **is** the map id used by menu, metaserver listing, demo gating and `PERMITTED_MAPS` (P11 §8.7); `level_enum id` = achievement index (§13); "Data shipped in" = which data builds package the file (§3.2: retail = `tools/multiwinia_exclusions.txt`; prev = PC preview; hw = hardware-compat; stress = Assault stress test; βA/βB/βC = beta-test groups); "FULLGAME menu" = what the baseline binary does with the file (`listed` = shown under its mode(s) with retail data; `permitted, data absent` = would be listed but the retail data omits the file; `mode not compiled` = TankBattle, `IsGameModePermitted` false without `INCLUDEGAMEMODE_TANKBATTLE`; `never listed` = no valid `GameTypes`); "PERMITTED_MAPS of other targets" = compile-time whitelists (`code/lib/universal_include.h`; FULLGAME defines none → all maps permitted) ; "Demo default" = `DemoLimitations::Initialise` fallback list used when the metaserver sends none (`code/network/demo_limitations.cpp`); Tutorial = map hard-wired by `GameMenuWindow::SetupTutorial` (Tutorial1) / `SetupTutorial2` (Tutorial2). Grid = `ceil(worldSize / cellSize)` per axis (`SurfaceMap2D` ctor; `detailScaleFactor` is hard-coded 1 in `Landscape::Init`) [C]. All values [C] from the files / code; generated by `X4_md_tables.py master`.

| File | English name (IP) | MAPID macro | CRC (=map id) | level_enum id | GameTypes | Players | World size / cell (grid) | Thumbnail | Data shipped in | FULLGAME menu (retail data) | PERMITTED_MAPS of other targets | Demo default | Tutorial | Class |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| mp_assault_2P_1.txt | Beneath the Baleful Eye | MAPID_MP_ASSAULT_2P_1 | 0xd4cd2a76 | 1 | Assault | 2 | 3000×3000 / 22.00 (137x137) | mp_assault_2P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_assault_2P_2.txt | The Woeful Plateau | MAPID_MP_ASSAULT_2P_2 | 0xabbfd0b7 | 2 | Assault | 2 | 2991×2991 / 20.00 (150x150) | mp_assault_2P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_assault_2p_3.txt | Dog Green Sector | MAPID_MP_ASSAULT_2P_3 | 0x56caa33d | 3 | Assault | 2 | 5143×5143 / 17.03 (302x302) | mp_assault_2p_3.bmp | retail βA | listed | — |  |  | official, retail |
| mp_assault_3p_1.txt | The Third Kind | MAPID_MP_ASSAULT_3P_1 | 0xb55bfb4f | 4 | Assault | 2 | 5000×5000 / 22.32 (225x225) | mp_assault_3P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_assault_3P_2.txt | The Hard Way | MAPID_MP_ASSAULT_3P_2 | 0x86e07c2c | 5 | Assault | 3 | 5000×5000 / 26.00 (193x193) | mp_assault_3P_2.bmp | retail βA | listed | — |  |  | official, retail |
| mp_assault_3p_3.txt | Fort Zulu | MAPID_MP_ASSAULT_3P_3 | 0x0617fad4 | 6 | Assault | 3 | 2549×2549 / 9.95 (257x257) | mp_assault_3P_3.bmp | retail βA | listed | — |  |  | official, retail |
| mp_assault_4P_1.txt | Beyond the Pale | MAPID_MP_ASSAULT_4P_1 | 0xf9a4dc74 | 7 | Assault | 4 | 4070×4070 / 20.00 (204x204) | mp_assault_4P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_assault_4P_2.txt | The Last Stand | MAPID_MP_ASSAULT_4P_2 | 0x9a54fad8 | 8 | Assault | 4 | 7000×7000 / 26.00 (270x270) | mp_assault_4P_2.bmp | retail βA | listed | — |  |  | official, retail |
| mp_assault_stress.txt | Assault Stress Test | MAPID_MP_ASSAULT_STRESS | 0x6d970875 | — | none | 2 | 4000×4000 / 20.00 (200x200) | — (default.bmp) | stress | never listed | STRESS |  |  | stress-test; hidden (no valid GameTypes); not in retail data |
| mp_benchmark_1.txt | Stress Test | MAPID_MP_BENCHMARK_1 | 0x49bda3c0 | — | none | 4 | 3000×3000 / 18.00 (167x167) | mp_benchmark_1.bmp | hw | never listed | HW |  |  | benchmark; hidden (no valid GameTypes); not in retail data |
| mp_blitzkrieg_2p_1.txt | Confrontation Point | MAPID_MP_BLITZKRIEG_2P_1 | 0x5f6db8c1 | 9 | Blitzkrieg | 2 | 3500×3500 / 12.00 (292x292) | mp_blitzkrieg_2p_1.bmp | retail | listed | — |  |  | official, retail |
| mp_blitzkrieg_2p_2.txt | The Bleak Mountains | MAPID_MP_BLITZKRIEG_2P_2 | 0xb96c43ef | 10 | Blitzkrieg | 2 | 4000×4000 / 18.00 (223x223) | mp_blitzkrieg_2p_2.bmp | retail βB | listed | βB |  |  | official, retail |
| mp_blitzkrieg_2P_3.txt | The Eye of the World | MAPID_MP_BLITZKRIEG_2P_3 | 0xddeed663 | 11 | Blitzkrieg | 2 | 3000×3000 / 16.00 (188x188) | mp_blitzkrieg_2p_3.bmp | retail | listed | — |  |  | official, retail |
| mp_blitzkrieg_2P_4.txt | The Apex of Infinity | MAPID_MP_BLITZKRIEG_2P_4 | 0xfd2d4309 | 12 | Blitzkrieg | 2 | 3000×3000 / 20.00 (150x150) | mp_blitzkrieg_2p_4.bmp | retail | listed | — |  |  | official, retail |
| mp_blitzkrieg_3P_1.txt | Upon the Blazing Sea | MAPID_MP_BLITZKRIEG_3P_1 | 0x26843f9a | 13 | Blitzkrieg | 3 | 4000×4000 / 12.00 (334x334) | mp_blitzkrieg_3P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_blitzkrieg_3P_2.txt | The Wheel | MAPID_MP_BLITZKRIEG_3P_2 | 0x5493f725 | 14 | Blitzkrieg | 3 | 2938×2938 / 16.00 (184x184) | mp_blitzkrieg_3P_2.bmp | retail βB | listed | βB |  |  | official, retail |
| mp_blitzkrieg_4P_2.txt | Gridlock | MAPID_MP_BLITZKRIEG_4P_2 | 0x8a7a997b | 15 | Blitzkrieg | 4 | 4000×4000 / 20.00 (200x200) | mp_blitzkrieg_4P_2.bmp | retail βB | listed | βB |  |  | official, retail |
| mp_cts_2p_1.txt | Mountain of Lost Souls | MAPID_MP_CTS_2P_1 | 0x00486fb0 | 16 | CaptureTheStatue | 2 | 2332×2332 / 15.76 (148x148) | mp_cts_2p_1.bmp | retail prev | listed | PREV βA | Y |  | official, retail |
| mp_cts_2p_2.txt | The Bloody Shrine | MAPID_MP_CTS_2P_2 | 0xa6483bea | 17 | CaptureTheStatue | 2 | 2332×2332 / 15.76 (148x148) | mp_cts_2p_2.bmp | retail βA | listed | — |  |  | official, retail |
| mp_cts_2p_3.txt | Price of War | MAPID_MP_CTS_2P_3 | 0xe933d619 | 18 | CaptureTheStatue | 2 | 2000×2000 / 12.00 (167x167) | mp_cts_2p_3.bmp | retail | listed | — |  |  | official, retail |
| mp_cts_2p_4.txt | The Unending Peaks | MAPID_MP_CTS_2P_4 | 0x1b0eb72d | 19 | CaptureTheStatue | 2 | 2000×2000 / 9.00 (223x223) | mp_cts_2p_4.bmp | retail βA | listed | — |  |  | official, retail |
| mp_cts_3p_1.txt | Melting Pot | MAPID_MP_CTS_3P_1 | 0x8a093da3 | 20 | CaptureTheStatue | 3 | 3000×3000 / 15.80 (190x190) | mp_cts_3p_1.bmp | retail βA | listed | βA |  |  | official, retail |
| mp_cts_3P_2.txt | The Tortured Expanse | MAPID_MP_CTS_3P_2 | 0xbd40d916 | 21 | CaptureTheStatue | 3 | 2999×2999 / 16.00 (188x188) | mp_cts_3P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_cts_3p_3.txt | The Outreach Peaks | MAPID_MP_CTS_3P_3 | 0x8d1184ad | — | CaptureTheStatue | 3 | 2570×2570 / 12.00 (215x215) | mp_cts_3p_3.bmp | — | permitted, data absent | — |  |  | not in retail data |
| mp_cts_4P_1.txt | Field of Unlife | MAPID_MP_CTS_4P_1 | 0x532eacda | 22 | CaptureTheStatue | 4 | 3000×3000 / 16.00 (188x188) | mp_cts_4P_1.bmp | retail βA | listed | — |  |  | official, retail |
| mp_cts_4P_2.txt | Scorched Earth | MAPID_MP_CTS_4P_2 | 0x79f72913 | 23 | CaptureTheStatue | 4 | 3428×3428 / 16.00 (215x215) | mp_cts_4P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_cts_4p_3.txt | Maze of Despair | MAPID_MP_CTS_4P_3 | 0x926e6071 | 24 | CaptureTheStatue | 4 | 2720×2720 / 15.00 (182x182) | mp_cts_4p_3.bmp | retail | listed | βA |  |  | official, retail |
| mp_domination_2P_1.txt | Barriers to Progress | MAPID_MP_DOMINATION_2P_1 | 0x1959ed1e | 25 | Domination | 2 | 2000×2000 / 12.00 (167x167) | mp_domination_2P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_domination_2P_2.txt | The Eternal Guardians | MAPID_MP_DOMINATION_2P_2 | 0x15e6fc2c | 26 | Domination | 2 | 2700×2700 / 15.76 (172x172) | mp_domination_2P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_domination_3P_1.txt | Field of Unending Dawn | MAPID_MP_DOMINATION_3P_1 | 0x297bcdfb | 27 | Domination | 3 | 2345×2345 / 16.00 (147x147) | mp_domination_3P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_domination_4P_1.txt | The Holy Tree | MAPID_MP_DOMINATION_4P_1 | 0xe5667f49 | 28 | Domination | 4 | 3000×3000 / 16.00 (188x188) | mp_domination_4P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_domination_4P_2.txt | A Monument to the Heavens | MAPID_MP_DOMINATION_4P_2 | 0x6cbcaf9f | 29 | Domination | 4 | 3000×3000 / 14.00 (215x215) | mp_domination_4P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_2P_1.txt | The Eternal Guardians | MAPID_MP_KOTH_2P_1 | 0xfa61f648 | 30 | KingOfTheHill | 2 | 2700×2700 / 15.76 (172x172) | mp_koth_2P_1.bmp | retail prev hw βA | listed | PREV HW | Y | Tutorial1 | official, retail |
| mp_koth_2P_2.txt | Shrine of Memory | MAPID_MP_KOTH_2P_2 | 0xd009251b | 31 | Domination | 2 | 2677×2677 / 14.00 (192x192) | mp_koth_2P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_2P_3.txt | The Bitter Front | MAPID_MP_KOTH_2P_3 | 0xa29190a0 | 32 | Domination | 2 | 2754×2754 / 16.00 (173x173) | mp_koth_2P_3.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_2P_3b.txt | The Bitter Front | MAPID_MP_KOTH_2P_3B | 0xf63c8b47 | 33 | KingOfTheHill | 2 | 2754×2754 / 16.00 (173x173) | mp_koth_2P_3b.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_3P_1.txt | Pursuit of Victory | MAPID_MP_KOTH_3P_1 | 0xc5dbe61e | 34 | KingOfTheHill | 3 | 3000×3000 / 12.00 (250x250) | mp_koth_3P_1.bmp | retail βA | listed | — | Y | Tutorial2 | official, retail |
| mp_koth_3P_2.txt | Field of Unending Dawn | MAPID_MP_KOTH_3P_2 | 0xa7581956 | 35 | KingOfTheHill | 3 | 2345×2345 / 16.00 (147x147) | mp_koth_3P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_3P_3.txt | The Deceitful Vale | MAPID_MP_KOTH_3P_3 | 0xdb2380e7 | 36 | KingOfTheHill | 3 | 2800×2800 / 16.00 (175x175) | mp_koth_3P_3.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_3P_4.txt | The Walled Garden | MAPID_MP_KOTH_3P_4 | 0xc59930ef | 37 | KingOfTheHill | 3 | 2750×2750 / 12.00 (230x230) | mp_koth_3P_4.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_4P_1.txt | The Holy Tree | MAPID_MP_KOTH_4P_1 | 0x88897d17 | 38 | KingOfTheHill | 4 | 3000×3000 / 16.00 (188x188) | mp_koth_4P_1.bmp | retail prev hw | listed | PREV HW | Y |  | official, retail |
| mp_koth_4P_3.txt | A Monument to the Heavens | MAPID_MP_KOTH_4P_3 | 0x2ba30b8b | 39 | KingOfTheHill | 4 | 3000×3000 / 14.00 (215x215) | mp_koth_4P_3.bmp | retail | listed | — |  |  | official, retail |
| mp_koth_4P_4.txt | Barbs of Madness | MAPID_MP_KOTH_4P_4 | 0x1d6b4438 | 40 | KingOfTheHill | 4 | 3000×3000 / 18.00 (167x167) | mp_koth_4P_4.bmp | retail | listed | — |  |  | official, retail |
| mp_rocketriot_2P_2.txt | Echoes of the Future | MAPID_MP_ROCKETRIOT_2P_2 | 0x0101a2ca | 41 | RocketRiot | 2 | 5000×5000 / 18.00 (278x278) | mp_rocketriot_2P_2.bmp | retail | listed | — |  |  | official, retail |
| mp_rocketriot_2p_3.txt | Stairway to Heaven | MAPID_MP_ROCKETRIOT_2P_3 | 0x9df8d46e | 42 | RocketRiot | 2 | 3756×3756 / 27.08 (139x139) | mp_rocketriot_2p_3.bmp | retail βA βC | listed | βC |  |  | official, retail |
| mp_rocketriot_2P_4.txt | Bridge of Light | MAPID_MP_ROCKETRIOT_2P_4 | 0x2ca54a88 | 43 | RocketRiot | 2 | 2000×2000 / 9.87 (203x203) | mp_rocketriot_2P_4.bmp | retail βC | listed | βC |  |  | official, retail |
| mp_rocketriot_3p_1.txt | The Second Beginning | MAPID_MP_ROCKETRIOT_3P_1 | 0x847aae73 | 44 | RocketRiot | 3 | 5000×5000 / 18.00 (278x278) | mp_rocketriot_3p_1.bmp | retail βC | listed | βC | Y |  | official, retail |
| mp_rocketriot_3P_2.txt | Wake of Eternity | MAPID_MP_ROCKETRIOT_3P_2 | 0xacef90f0 | 45 | RocketRiot | 3 | 4285×4285 / 18.00 (239x239) | mp_rocketriot_3P_2.bmp | retail βA | listed | — |  |  | official, retail |
| mp_rocketriot_3P_3.txt | Heart of Chaos | MAPID_MP_ROCKETRIOT_3P_3 | 0x133f8ea6 | 46 | RocketRiot | 3 | 4500×4500 / 16.00 (282x282) | mp_rocketriot_3P_3.bmp | retail | listed | — |  |  | official, retail |
| mp_rocketriot_4P_1.txt | Threads of Fate | MAPID_MP_ROCKETRIOT_4P_1 | 0xc3c28aa1 | 47 | RocketRiot | 4 | 4186×4186 / 18.00 (233x233) | mp_rocketriot_4P_1.bmp | retail βA | listed | — |  |  | official, retail |
| mp_sandbox.txt | — | MAPID_MP_SANDBOX | 0xb2db706d | — | none | 2 | 3290×3290 / 20.35 (162x162) | — (default.bmp) | — | never listed | — |  |  | sandbox/test; hidden (no valid GameTypes); not in retail data |
| mp_sandbox_all.txt | — | MAPID_MP_SANDBOX_ALL | 0xa3f63f95 | — | none | — | 3290×3290 / 20.35 (162x162) | — (default.bmp) | — | never listed | — |  |  | sandbox/test; hidden (no valid GameTypes); not in retail data |
| mp_skirmish_2p_4.txt | Hamburger Hill | MAPID_MP_SKIRMISH_2P_4 | 0xa3a30f75 | 48 | Domination | 2 | 2000×2000 / 12.00 (167x167) | mp_skirmish_2p_4.bmp | retail | listed | — |  |  | official, retail |
| mp_skirmish_3P_1.txt | Leap of Faith | MAPID_MP_SKIRMISH_3P_1 | 0x177cc7b3 | 49 | Domination | 3 | 6000×6000 / 20.00 (300x300) | mp_skirmish_3P_1.bmp | retail | listed | — |  |  | official, retail |
| mp_tankbattle_2P.txt | 2P TankBattle Arena | MAPID_MP_TANKBATTLE_2P | 0x840ab50d | — | TankBattle | 2 | 2000×2000 / 12.00 (167x167) | mp_tankbattle_2P.bmp | — | mode not compiled | — |  |  | tankbattle [B:DEBUG mode]; not in retail data |
| mp_tankbattle_3P.txt | 3P TankBattle Arena | MAPID_MP_TANKBATTLE_3P | 0x2b3a6c4c | — | TankBattle | 3 | 2000×2000 / 12.00 (167x167) | mp_tankbattle_3P.bmp | — | mode not compiled | — |  |  | tankbattle [B:DEBUG mode]; not in retail data |
| mp_tankbattle_4P.txt | 4P TankBattle Arena | MAPID_MP_TANKBATTLE_4P | 0x43fa87ed | — | TankBattle | 4 | 2000×2000 / 12.00 (167x167) | mp_tankbattle_4P.bmp | — | mode not compiled | — |  |  | tankbattle [B:DEBUG mode]; not in retail data |

Summary [C]:
- **49 maps are menu-listed in the retail product**: Domination 9 (5 `mp_domination_*`, 2 `mp_koth_*` whose `GameTypes` is `Domination` — `mp_koth_2P_2`, `mp_koth_2P_3` — and 2 `mp_skirmish_*`), King of the Hill 9, Capture the Statue 9, Assault 8, Rocket Riot 7, Blitzkrieg 7. No shipped file lists more than one game type. This equals `level_enum.txt` ids 1–49 and `VALID_ACHIEVEMENT_LEVELS = 49` (`code/achievement_tracker.cpp`, with `INCLUDEGAMEMODE_ASSAULT`).
- **8 files are in the tree but not in retail data**: `mp_assault_stress`, `mp_benchmark_1`, `mp_sandbox`, `mp_sandbox_all`, `mp_tankbattle_2P/3P/4P`, `mp_cts_3p_3` (details §4).
- Player counts: 2-player 22 maps, 3-player 15, 4-player 12 among the 49 (`NumPlayers`; note `mp_assault_3p_1` is named 3p but declares `NumPlayers 2`).
- Name collisions (same English name, shared terrain): `mp_koth_2P_1`/`mp_domination_2P_2` "The Eternal Guardians", `mp_koth_3P_2`/`mp_domination_3P_1` "Field of Unending Dawn", `mp_koth_4P_1`/`mp_domination_4P_1` "The Holy Tree", `mp_koth_4P_3`/`mp_domination_4P_2` "A Monument to the Heavens", `mp_koth_2P_3`/`mp_koth_2P_3b` "The Bitter Front"; `mp_rocketriot_2p_3` shares "Stairway to Heaven" with the thumbnail-only `mp_skirmish_2P_3` string. Identical `LandscapeTiles` blocks (byte-identical after whitespace normalisation) [C]: koth_2P_1≡domination_2P_2, koth_3P_2≡domination_3P_1, koth_4P_1≡domination_4P_1, koth_4P_3≡domination_4P_2, benchmark_1≡koth_4P_4, sandbox≡sandbox_all, koth_2P_3b≡old/mp_shaman_2P_3, old/mp_temple≡map_temple (Darwinia terrain reused). Designers therefore re-skinned one terrain for two modes by swapping the objective buildings.

## [→ 08_levels_and_level_design] 3. Availability per build target and runtime gate

### 3.1 Compile-time whitelists and runtime gates [C]

| Target / gate | Mechanism | Maps | Evidence |
|---|---|---|---|
| `TARGET_PC_FULLGAME` (baseline) [B:FULL] | `PERMITTED_MAPS` undefined → `App::IsMapPermitted(id)` returns true when `IsFullVersion()`; mode filter `IsGameModePermitted` (all six modes; TankBattle not compiled) | every map whose `GameTypes` names a compiled mode | `code/app.cpp:IsMapPermitted`, `IsGameModePermitted` |
| Runtime demo (`IS_DEMO` ≡ `!IsFullVersion()`) [B:DEMO] | `DemoLimitations` from metaserver (`NET_METASERVER_DEMOMAPS` CRC list, `NET_METASERVER_DEMOMODES` bitmask) else defaults: modes KOTH, CTS, Rocket Riot; maps `MP_KOTH_2P_1`, `MP_KOTH_4P_1`, `MP_CTS_2P_1`, `MP_KOTH_3P_1`, `MP_ROCKETRIOT_3P_1`; `m_maxGameSize 4`, `m_maxDemoPlayers 4`, `m_allowDemoServers true` | 5 (default) | `code/network/demo_limitations.cpp:DemoLimitations::Initialise`; menu greys restricted maps (`dialog_fullonly`), selecting one opens `PageBuyMeNow` (`LevelSelectButton::MouseUp`) |
| `TARGET_PC_PREVIEW` | `PERMITTED_MAPS {MP_KOTH_2P_1, MP_KOTH_4P_1, MP_CTS_2P_1, 0}` | 3 | `universal_include.h` |
| `TARGET_PC_HARDWARECOMPAT` | `{MP_KOTH_2P_1, MP_BENCHMARK_1, MP_KOTH_4P_1, 0}` | 3 (benchmark has `GameTypes none` → never in the menu, §4) | same |
| `TARGET_ASSAULT_STRESSTEST` | `{MP_ASSAULT_STRESS, 0}` | 1 (also `GameTypes none`) | same |
| `TARGET_BETATEST_GROUP_A` | `{MP_CTS_2P_1, MP_CTS_3P_1, MP_CTS_4P_3, 0}` | 3 | same |
| `TARGET_BETATEST_GROUP_B` | `{MP_BLITZKRIEG_2P_2, MP_BLITZKRIEG_3P_2, MP_BLITZKRIEG_4P_2, 0}` | 3 | same |
| `TARGET_BETATEST_GROUP_C` | `{MP_ROCKETRIOT_2P_4, MP_ROCKETRIOT_2P_3, MP_ROCKETRIOT_3P_1, 0}` | 3 | same |
| `TARGET_BETATEST_GROUP_ALL`, `TARGET_DEBUG`, `TARGET_MULTIWINIA_DEMOONLY` | none (`GROUP_ALL` has a commented-out 13-map list: KOTH_2P_1, KOTH_3P_1, ASSAULT_2P_3, ASSAULT_3P_2, ASSAULT_3P_3, ASSAULT_4P_2, ROCKETRIOT_2P_3, ROCKETRIOT_3P_2, ROCKETRIOT_4P_1, CTS_2P_2, CTS_2P_4, CTS_3P_1, CTS_4P_1) | all | same |

Where `PERMITTED_MAPS` is enforced [C]: (1) `GameMenuWindow::SetupMapSelectPage` skips non-permitted maps (`#ifdef PERMITTED_MAPS … continue`) and marks buttons inactive via `IsMapPermitted(gameType, id)`; (2) `DoEnterLocation` (`code/main.cpp`) computes `IsMapPermitted(CalculeCRC(requestedMap))` and, if false, sets `m_requestedLocationId = -1` and returns to the main menu after loading; (3) tutorial buttons are made inactive when their map is missing or not permitted (`GameMenuWindow` tutorial page). The list terminator is `0` (`for( i = 0; permittedMaps[i]; i++ )`), so a map whose CRC were 0 could never be permitted [C].

Hard-wired map picks [C] `code/UI/GameMenuWindow.cpp`: `SetupTutorial` → KOTH + `MAPID_MP_KOTH_2P_1` (AI Easy, blueprint defaults); `SetupTutorial2` → KOTH + `MAPID_MP_KOTH_3P_1`; both under `INCLUDE_TUTORIAL` [B:FULL]. `Location::GetTeamPosition` forces position = team index in a tutorial (`m_multiwiniaTutorial`) [C] `code/location.cpp`.

### 3.2 Data packaging per build [C] `tools/build_multiwinia_data.bat`, `tools/listfiles.pl`, `tools/copyfiles.pl`

`main.dat` = every `data/` file not matched by the exclusion list (`listfiles.pl`: `**/x` prunes any path component named `x`; other lines are `bsd_glob`-expanded and compared to each found path case-insensitively, a match prunes the file/subtree), then the platform's inclusion list is copied back in. Platform argument → lists:

| Script / platform | Exclusions | Inclusions (copied back) | Level/thumbnail effect |
|---|---|---|---|
| `build_multiwinia_retail.bat` (`retail` → ELSE branch), `build_multiwinia.bat`, `_gold`, `_crashreporter` (no arg → ELSE) | `multiwinia_exclusions.txt` | – | drops `levels/old/*`, `levels/pdlc/*`, `levels/level_enum.txt`, `levels/map_*`, `levels/mission_*`, `levels/mp_assault_stress.txt`, `levels/mp_benchmark_*`, `levels/mp_sandbox*`, `levels/mp_tankbattle_*`, `levels/mp_cts_3p_3.txt`, `thumbnails/mp_benchmark_*`, `thumbnails/mp_tankbattle_*`, `thumbnails/mp_cts_3p_3.bmp` → ships 49 `mp_*` + 52 thumbnails (incl. `default.bmp` and the level-less `mp_koth_2P_4.bmp`, `mp_skirmish_2p_3.bmp`) |
| `build_multiwinia_pc_preview.bat` (`pcpreview`) | `multiwinia_exclusions_beta_test.txt` (drops all `levels/mp_*`, `thumbnails/mp_*`) | `multiwinia_inclusions_previewcopy.txt`: koth_2P_1, koth_4P_1, cts_2P_1 (+thumbs) | 3 maps = its `PERMITTED_MAPS` |
| `build_multiwinia_hwcompat*.bat` (`pchwcompat`) | `multiwinia_exclusions_hwcompat.txt` | `multiwinia_inclusions_hwcompat.txt`: koth_2P_1, benchmark_1, koth_4P_1 (+thumbs) | 3 maps = its `PERMITTED_MAPS` |
| (`pcassaultstress`; no script in tree) | `multiwinia_exclusions_hwcompat.txt` | `multiwinia_inclusions_assaultstresstest.txt`: assault_stress (no thumbnail) | 1 map |
| (`pcbetatest` + group in the language slot `%2`; no script in tree) | `multiwinia_exclusions_beta_test.txt` | `multiwinia_inclusions_beta_test_group_<a|b|c>.txt` | A: 13 maps (koth_2p_1, koth_3P_1, assault_2p_3/3p_2/3p_3/4p_2, rocketriot_2p_3/3p_2/4p_1, cts_2p_2/2p_4/3p_1/4p_1); B: blitzkrieg_2p_2/3P_2/4P_2; C: rocketriot_2P_4/2p_3/3p_1 |
| `platambx` | `multiwinia_exclusions_beta_test.txt` | `multiwinia_inclusions_ambx.txt` (**file absent from tree**) | [U] |

Per-file results are columns `ships_retail … ships_beta_c` of `X4_levels.csv` / `X4_thumbnails.csv`.

## [→ 08_levels_and_level_design] 4. Hidden, test, stress, benchmark, sandbox and TankBattle maps [C] data, tags as stated

| File | Class | Why it is not a normal map | Contents worth knowing | Build tag |
|---|---|---|---|---|
| `mp_assault_stress.txt` ("Assault Stress Test") | stress test | `GameTypes none` → discarded by `AddMap` in every build; excluded from retail data; included + permitted only for `TARGET_ASSAULT_STRESSTEST` | Assault layout: 1 PulseBomb (pos 1, `startTime 600`), 3 ControlStation, 7 GunTurret, 26 LaserFence (pos 1), 30 Wall, 6 SolarPanel/5 Pylon/2 PylonEnd, TrunkPort ×5 (pos 0) + ×2 (pos 1), 3 AIObjective/10 AIObjectiveMarker/30 AITarget; `AttackingTeams 0`; `trunkportreinforcements 250`, `defenderreinforcementtype Armour`; 90 Darwinians at pos 1 | stress target only; in FULL [B:DEAD-data] |
| `mp_benchmark_1.txt` ("Stress Test" — "spawn Multiwinians indefinitely") | benchmark | `GameTypes none` (never menu-listed); excluded from retail; included + permitted for `TARGET_PC_HARDWARECOMPAT` | terrain/zones copied from `mp_koth_4P_4` (identical tiles); 5 SpawnPoint per position + 4 neutral, 15 MultiwiniaZone (neutral), 4 neutral TrunkPort, 1 SpawnPopulationLock (radius 1169.84, max 1200), 150 Darwinians per position; `PopulationCap -1`, `CoopMode` | hwcompat only [U] how it is launched (no code references the id except `PERMITTED_MAPS`) |
| `mp_sandbox.txt` | developer sandbox | `GameTypes none`; excluded from retail | 11 buildings: ControlTower, Incubator, **Statue** (blocked by `BuildingIsBlocked` → dropped at parse), 3 Wall, 5 AITarget; `PopulationCap 2500`, `NumPlayers 2`, `AttackingTeams 1`; `PrimaryObjectives BuildingOnline :sandbox,2` (Darwinia condition; `GetLocationId("sandbox")` = −1 in Multiwinia → debug assert only) | none |
| `mp_sandbox_all.txt` | developer sandbox | as above; also **no `NumPlayers`** (→ −1) | same terrain as `mp_sandbox`; 2 ControlTower, 2 Incubator, 4 SpawnPoint (pos 1), 11 SpawnLink, 1 SpawnPointMaster, 2 Tree | none |
| `mp_tankbattle_2P/3P/4P.txt` ("2P/3P/4P TankBattle Arena") | TankBattle mode | `GameTypes TankBattle` → listed under mode 6, but `IsGameModePermitted(TankBattle)` is false without `INCLUDEGAMEMODE_TANKBATTLE`; excluded from retail data | 2000×2000/12 arenas, only Trees (18/18/23); one `Tank` instant unit per position; `PopulationCap -1`; 4P has `CoopMode`; 3P/4P have **no camera mounts** | [B:DEBUG] |
| `mp_cts_3p_3.txt` ("The Outreach Peaks") | cut CTS map | complete CTS map but excluded from retail data (explicit exclusion line), no `level_enum` id, no `Difficulty` | the only CTS map with trunk ports instead of spawn points: 1 TrunkPort per position, `trunkportreinforcements 100`, `attackerreinforcementtype HotFeet`, 3 team drop zones (size 75) + 6 statue spawns (size 50), `PlayTime 10`, no starting Darwinians; 24-bit thumbnail (also excluded) | FULL code would list it if the file were present ("permitted, data absent") |

## [→ 08_levels_and_level_design] 5. Per-level option table (`MultiwiniaOptions`, all 57 `mp_*` files)

Values exactly as interpreted by `LevelFile::ParseMultiwiniaOptions` (last line wins for scalar keys; list keys accumulate). Blank cells (—) = key absent → defaults: `PopulationCap` −1, `DefenderPopulationCap` −1, `PlayTime` −1 (menu shows "-" and uses the mode default), `Difficulty` NULL (menu rank 0, sorted first), no effects, level options at their defaults (P11 §8.3). Key semantics: P11 §8.3; consumers: P02 (modes, reinforcements), P05 (caps), P08 (`InvalidCrates`). `AttackingTeams`/`CoopGroup*` values are **start positions** (0-based), not team ids. Generated by `X4_md_tables.py options`.

| File | PopulationCap | DefenderPopCap | PlayTime | Difficulty | Effects | LevelOptions | InvalidCrates | AttackingTeams | Coop | CoopGroup1 / 2 positions | OfficialLevel | Name |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| mp_assault_2P_1.txt | 2000 | 350 | 15 | basic | — | trunkportreinforcements=100 | Squad Airstrike Nuke Meteor | 1 | — | — | Y | — |
| mp_assault_2P_2.txt | 2000 | 275 | 15 | advanced | — | trunkportreinforcements=100 trunkportarmour=1 | — | 1 | — | — | Y | — |
| mp_assault_2p_3.txt | 2000 | -1 | 15 | intermediate | — | trunkportreinforcements=100 trunkportarmour=3 defenderreinforcementtype=AirStrike forceradarteammatch noradarcontrol | Squad Airstrike Nuke Meteor | 0 | — | — | Y | NewLevel |
| mp_assault_3p_1.txt | 1600 | 250 | 15 | advanced | — | trunkportreinforcements=75 | Armour | 1 | — | — | Y | NewLevel |
| mp_assault_3P_2.txt | 1500 | 250 | 15 | advanced | — | trunkportreinforcements=75 | Armour | 1 2 |  ForceCoop | 0 / 1 2 | Y | — |
| mp_assault_3p_3.txt | 1800 | -1 | 15 | intermediate | snow | trunkportreinforcements=100 | — | 1 2 |  ForceCoop | 0 / 1 2 | Y | NewLevel |
| mp_assault_4P_1.txt | 1600 | 200 | 15 | advanced | — | trunkportreinforcements=75 | Armour | 0 1 |  ForceCoop | 0 1 / 2 3 | Y | — |
| mp_assault_4P_2.txt | 1500 | 200 | 20 | advanced | — | trunkportreinforcements=60 trunkportarmour=3 | Armour | 2 3 |  ForceCoop | 0 1 / 2 3 | Y | NewLevel |
| mp_assault_stress.txt | 2000 | — | — | advanced | — | trunkportreinforcements=250 defenderreinforcementtype=Armour | — | 0 | — | — | Y | — |
| mp_benchmark_1.txt | -1 | — | — | intermediate | — | — | — | — | Coop | — | Y | — |
| mp_blitzkrieg_2p_1.txt | 1500 | -1 | — | basic | — | trunkportreinforcements=100 trunkportarmour=3 | — | — | — | — | Y | — |
| mp_blitzkrieg_2p_2.txt | 2000 | -1 | — | advanced | — | trunkportreinforcements=100 trunkportarmour=3 | — | — | — | — | Y | — |
| mp_blitzkrieg_2P_3.txt | 2000 | -1 | — | intermediate | — | trunkportreinforcements=100 trunkportarmour=3 | — | — | — | — | Y | — |
| mp_blitzkrieg_2P_4.txt | 1800 | — | — | advanced | snow | nogunturrets | — | — | — | — | Y | — |
| mp_blitzkrieg_3P_1.txt | 1500 | -1 | — | intermediate | — | airstrikecapturezones=3 | — | — | — | — | Y | — |
| mp_blitzkrieg_3P_2.txt | 2000 | — | — | basic | lightning | airstrikecapturezones=3 | — | — | — | — | Y | — |
| mp_blitzkrieg_4P_2.txt | 1400 | -1 | — | advanced | snow | airstrikecapturezones=3 | — | — | — | — | Y | NewLevel |
| mp_cts_2p_1.txt | 2000 | -1 | — | basic | — | — | — | — | — | — | Y | — |
| mp_cts_2p_2.txt | 2000 | — | — | intermediate | snow | instantspawnpointcapture | — | — | — | — | Y | — |
| mp_cts_2p_3.txt | 2000 | -1 | — | intermediate | — | — | — | — | — | — | Y | — |
| mp_cts_2p_4.txt | 2000 | — | — | basic | — | — | — | — | — | — | Y | — |
| mp_cts_3p_1.txt | 1800 | -1 | — | intermediate | dustwind | instantspawnpointcapture | — | — | — | — | Y | — |
| mp_cts_3P_2.txt | 1500 | -1 | — | intermediate | — | instantspawnpointcapture | — | — | — | — | Y | — |
| mp_cts_3p_3.txt | 2000 | -1 | 10 | — | — | trunkportreinforcements=100 attackerreinforcementtype=HotFeet | — | — | — | — | Y | — |
| mp_cts_4P_1.txt | 2000 | -1 | — | advanced | — | — | — | — | — | — | Y | — |
| mp_cts_4P_2.txt | 2000 | — | — | advanced | lightning | instantspawnpointcapture | — | — | Coop | — | Y | — |
| mp_cts_4p_3.txt | 2000 (lines: -1/2000) | — | — | advanced | — | — | — | — | — | — | Y | — |
| mp_domination_2P_1.txt | 2000 | -1 | — | intermediate | — | — | — | — | — | — | Y | NewLevel |
| mp_domination_2P_2.txt | 2000 | -1 | — | basic | — | — | — | — | — | — | Y | — |
| mp_domination_3P_1.txt | 1800 | -1 | 10 | basic | — | — | — | — | — | — | Y | — |
| mp_domination_4P_1.txt | 2000 | -1 | 15 | intermediate | — | — | — | — | Coop | — | Y | NewLevel |
| mp_domination_4P_2.txt | -1 | -1 | 10 | advanced | — | — | — | — | Coop | 0 2 / 1 3 | Y | — |
| mp_koth_2P_1.txt | 2000 | — | — | basic | — | — | — | — | — | — | Y | — |
| mp_koth_2P_2.txt | 2000 | -1 | 10 | intermediate | — | — | — | — | — | — | Y | — |
| mp_koth_2P_3.txt | 1500 | -1 | 10 | intermediate | snow SpecialLighting | — | — | — | — | — | Y | NewLevel |
| mp_koth_2P_3b.txt | 1500 | -1 | 10 | intermediate | snow SpecialLighting | — | — | — | — | — | Y | NewLevel |
| mp_koth_3P_1.txt | 1800 | -1 | 10 | intermediate | SpecialLighting | — | — | — | — | — | Y | — |
| mp_koth_3P_2.txt | 1800 | — | 5 | basic | — | — | — | — | — | — | Y | — |
| mp_koth_3P_3.txt | 1800 | -1 | 10 | intermediate | — | — | — | — | — | — | Y | — |
| mp_koth_3P_4.txt | 1800 | -1 | 10 | intermediate | — | trunkportreinforcements=100 | — | — | — | — | Y | — |
| mp_koth_4P_1.txt | 2000 | -1 | 10 | intermediate | — | — | — | — | Coop | — | Y | — |
| mp_koth_4P_3.txt | -1 | -1 | 10 | advanced | — | — | — | — | Coop | 0 2 / 1 3 | Y | — |
| mp_koth_4P_4.txt | 2000 | — | — | basic | — | — | — | — | Coop | — | Y | — |
| mp_rocketriot_2P_2.txt | 1500 | -1 | — | intermediate | — | — | — | — | — | — | Y | — |
| mp_rocketriot_2p_3.txt | 1800 | -1 | — | intermediate | — | — | — | — | — | — | Y | — |
| mp_rocketriot_2P_4.txt | 1500 | -1 | — | basic | — | — | Armour | — | — | — | Y | — |
| mp_rocketriot_3p_1.txt | 1800 | -1 | — | basic | — | — | — | — | — | — | Y | — |
| mp_rocketriot_3P_2.txt | 1800 | -1 | — | intermediate | — | — | — | — | — | — | Y | — |
| mp_rocketriot_3P_3.txt | 1800 | -1 | — | advanced | — | nogunturrets | — | — | — | — | Y | — |
| mp_rocketriot_4P_1.txt | 1800 | -1 | — | advanced | — | — | — | — | — | — | Y | — |
| mp_sandbox.txt | 2500 | — | — | basic | — | — | — | 1 | — | — | — | — |
| mp_sandbox_all.txt | -1 | — | — | — | — | — | — | — | — | — | — | — |
| mp_skirmish_2p_4.txt | 1800 | -1 | — | basic | — | — | — | — | — | — | Y | — |
| mp_skirmish_3P_1.txt | 1800 | -1 | — | advanced | — | — | — | — | — | — | Y | NewLevel |
| mp_tankbattle_2P.txt | -1 | — | — | basic | — | — | — | — | — | — | — | — |
| mp_tankbattle_3P.txt | -1 | — | — | basic | — | — | — | — | — | — | — | — |
| mp_tankbattle_4P.txt | -1 | — | — | basic | — | — | — | — | Coop | — | — | — |

Observations [C]:
- `PopulationCap`: 1400–2500 when set; −1 on `mp_domination_4P_2`, `mp_koth_4P_3`, `mp_benchmark_1`, `mp_sandbox_all`, `mp_tankbattle_*`. `mp_cts_4p_3` has two lines (−1 then 2000 → 2000).
- `DefenderPopulationCap` is set (200–350) only on 6 Assault maps; the 4 other Assault files leave it −1 or absent.
- `PlayTime` is set on 21 files (5 ×1, 10 ×11, 15 ×8, 20 ×1); every shipped Assault map has 15 (20 on `mp_assault_4P_2`), matching its PulseBomb `startTime` seconds/60 (900 s / 1200 s) — see §9.5.
- `Effects` used: `snow` 6 maps, `SpecialLighting` 3, `lightning` 2, `dustwind` 1; `soulrain` never used. `old/mp_koth_2P_4` and `old/mp_shaman_2P_3` also use `snow`.
- `InvalidCrates` used: `Squad Airstrike Nuke Meteor` (2 Assault maps), `Armour` (4 Assault maps + `mp_rocketriot_2P_4`).
- `LevelOptions` used: `trunkportreinforcements` (60, 75, 100, 250), `trunkportarmour` (1, 3), `instantspawnpointcapture`, `nogunturrets`, `airstrikecapturezones 3`, `defenderreinforcementtype` (`AirStrike`, `Armour`), `attackerreinforcementtype HotFeet` (cut map only), `forceradarteammatch`, `noradarcontrol`. Never used in top-level files: `nohandicap` (only `old/mp_koth_2P_4`), `attackerreinforcements`, `defenderreinforcements`, `futurewinianteam`, `attackreinforcementoption`, `defendreinforcementoption` (the last three only in `old/mp_assault_3P_1`).
- `Author`, `Description` are never set (descriptions come from the language file); `Name` only as the placeholder `NewLevel`.
- Coop: `CoopMode` (optional) on 8 files (6 retail: `mp_cts_4P_2`, `mp_domination_4P_1`, `mp_domination_4P_2`, `mp_koth_4P_1`, `mp_koth_4P_3`, `mp_koth_4P_4`; plus `mp_benchmark_1`, `mp_tankbattle_4P`) — all 4-player; `ForceCoop` on 4 Assault maps (3P_2, 3p_3, 4P_1, 4P_2); explicit `CoopGroup1/2Positions` on those 4 plus `mp_domination_4P_2`, `mp_koth_4P_3` (groups `0 2 / 1 3`).

## [→ 08_levels_and_level_design] 6. Notable objects per map

Counts by **start position** (`team` field of the building line; 255 = neutral; positions are mapped to real team ids at load by `Location::GetTeamFromPosition`, 4 = monster team, 5 = Futurewinian team — `Location::GetMonsterTeamId/GetFuturewinianTeamId`) [C] `code/location.cpp:InitBuildings`, `GetTeamFromPosition`. "Bldgs" counts every row in all `Buildings` blocks (incl. Trees and markers). Instant units `type@position×count`. All [C]; generated by `X4_md_tables.py objects`; full per-type counts in `X4_level_buildings.csv`.

| File | Bldgs | SpawnPoint by pos | SpawnLink/Master | SpawnPopLock (radius/max) | TrunkPort by pos | MultiwiniaZone by pos | Zone sizes | EscapeRocket | FuelStation | PulseBomb @startTime | ControlStation | GunTurret | RadarDish | Wall/LaserFence | JumpPad | RestrictionZone | AITarget/AIObjective/Marker | Trees | StaticShape shapes | InstantUnits type@pos×n | Camera mounts |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| mp_assault_2P_1.txt | 218 | — | 0/0 | 1 (633.50/500) | 0:1;1:4 | — | — | — | — | 1 @900.00 | 255:3 | 255:20 | 0 | 78/0 | 0 | 0 | 41/12/36 | 13 | rockhead.shp temple2.shp | Armour@0x2 Darwinian@0x325 | player0 player1 |
| mp_assault_2P_2.txt | 230 | — | 0/0 | 0 | 0:1;1:5 | — | — | — | — | 1 @900.00 | 255:3 | 255:12 | 0 | 14/58 | 3 | 1 | 29/9/28 | 36 | — | Darwinian@0x209 | player1 player0 |
| mp_assault_2p_3.txt | 160 | — | 0/0 | 0 | 0:3 | — | — | — | — | 1 @900.00 | 255:5 | 255:19 | 4 | 20/23 | 0 | 1 | 22/11/25 | 5 | beachheadobstruction.shp | Darwinian@0x8 Darwinian@1x300 | player0 player1 |
| mp_assault_3p_1.txt | 281 | — | 0/0 | 0 | 0:1;1:6 | — | — | — | — | 1 @900.00 | 255:3 | 255:18 | 2 | 94/48 | 0 | 2 | 31/10/27 | 0 | spaceship.shp temple3.shp temple4.shp | Armour@0x1 Darwinian@0x103 Darwinian@5x35 | player1 player2 player0 |
| mp_assault_3P_2.txt | 247 | — | 0/0 | 1 (313.91/150) | 0:1;1:4;2:4 | — | — | — | — | 1 @900.00 | 255:3 | 255:13 | 0 | 29/66 | 0 | 2 | 37/12/27 | 6 | — | Armour@0x2 Darwinian@0x180 | player1 player2 player0 |
| mp_assault_3p_3.txt | 161 | — | 0/0 | 0 | 1:4;2:4 | — | — | — | — | 1 @900.00 | 0:3 | 255:40 | 0 | 25/0 | 0 | 0 | 39/6/29 | 0 | temple2.shp | Darwinian@0x260 | — |
| mp_assault_4P_1.txt | 230 | — | 0/0 | 0 | 0:3;1:3;2:1;3:1 | — | — | — | — | 1 @900.00 | 255:4 | 255:18 | 6 | 0/81 | 0 | 1 | 31/14/24 | 26 | temple3.shp temple4.shp | Armour@2x1 Armour@3x1 Darwinian@2x100 Darwinian@3x68 | player1 player0 player2 player3 |
| mp_assault_4P_2.txt | 380 | — | 0/0 | 0 | 0:1;1:1;2:4;3:4 | — | — | — | — | 1 @1200.00 | 255:3 | 255:20 | 0 | 34/124 | 0 | 2 | 51/18/44 | 0 | beachheadobstruction.shp | Darwinian@0x63 Darwinian@1x95 | player0 player1 player2 player3 |
| mp_assault_stress.txt | 130 | — | 0/0 | 0 | 0:5;1:2 | — | — | — | — | 1 @600.00 | 255:3 | 255:7 | 0 | 30/26 | 0 | 0 | 30/3/10 | 0 | — | Darwinian@1x90 | — |
| mp_benchmark_1.txt | 80 | 0:5;1:5;2:5;3:5;255:4 | 4/1 | 1 (1169.84/1200) | 255:4 | 255:15 | 122.00 122.63 193.87 50.00 555.24 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 13/0/0 | 18 | — | Darwinian@0x150 Darwinian@1x150 Darwinian@2x150 Darwinian@3x150 | player0 player2 player1 player3 |
| mp_blitzkrieg_2p_1.txt | 58 | 255:1 | 0/1 | 0 | 0:1;1:1 | 0:1;1:1;255:1 | 100.00 | — | — | — | — | — | 0 | 0/23 | 0 | 0 | 12/0/0 | 7 | — | Darwinian@0x40 Darwinian@1x40 | player0 player2 player1 |
| mp_blitzkrieg_2p_2.txt | 80 | 0:2;1:2;255:2 | 8/1 | 6 (500.00/400) | 0:1;1:1 | 0:1;1:1;255:3 | 100.00 | — | — | — | — | 255:4 | 0 | 17/0 | 0 | 0 | 31/0/0 | 0 | — | Darwinian@0x8 Darwinian@1x8 | player0 player1 |
| mp_blitzkrieg_2P_3.txt | 60 | 255:4 | 9/1 | 0 | 0:1;1:1;255:1 | 0:1;1:1;255:1 | 100.00 | — | — | — | — | — | 0 | 0/21 | 2 | 0 | 17/0/0 | 0 | — | — | player0 player1 |
| mp_blitzkrieg_2P_4.txt | 87 | 255:3 | 7/1 | 2 (500.00/300) | 0:1;1:1;255:1 | 0:1;1:1;255:4 | 100.00 | — | — | — | — | 255:18 | 0 | 0/0 | 0 | 0 | 19/0/0 | 9 | beachheadobstruction.shp | Darwinian@0x36 Darwinian@1x36 | player0 player1 |
| mp_blitzkrieg_3P_1.txt | 100 | — | 0/0 | 4 (500.00/300 919.22/400) | 0:1;1:1;2:1 | 0:1;1:1;2:1;255:4 | 100.00 | — | — | — | — | — | 0 | 0/34 | 0 | 0 | 19/0/0 | 16 | — | Darwinian@0x15 Darwinian@1x15 Darwinian@2x15 | player0 player1 player2 |
| mp_blitzkrieg_3P_2.txt | 81 | — | 0/0 | 7 (500.00/150 500.00/200) | 0:1;1:1;2:1 | 0:1;1:1;2:1;255:4 | 75.00 | — | — | — | — | — | 0 | 0/20 | 0 | 3 | 24/0/0 | 16 | — | — | player0 player1 player2 |
| mp_blitzkrieg_4P_2.txt | 114 | — | 0/0 | 0 | 0:1;1:1;2:1;3:1 | 0:1;1:1;2:1;3:1;255:5 | 100.00 | — | — | — | — | — | 0 | 0/39 | 0 | 4 | 37/0/0 | 0 | — | — | player3 player2 player1 player0 |
| mp_cts_2p_1.txt | 28 | 0:2;1:2;255:2 | 0/1 | 4 (500.00/150 500.00/300) | — | 0:1;1:1;255:2 | 40.00 80.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 9/0/0 | 4 | — | Darwinian@0x40 Darwinian@1x40 | start player0 player1 |
| mp_cts_2p_2.txt | 34 | 0:2;1:2 | 0/1 | 2 (651.07/400) | — | 0:1;1:1;255:2 | 69.80 80.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 13/0/0 | 10 | — | Darwinian@0x40 Darwinian@1x40 | start player0 player1 |
| mp_cts_2p_3.txt | 43 | 0:1;1:1;255:2 | 6/1 | 0 | — | 0:1;1:1;255:2 | 109.27 50.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 11/0/0 | 17 | — | Darwinian@0x40 Darwinian@1x40 | player0 player1 |
| mp_cts_2p_4.txt | 29 | 0:2;1:2 | 0/1 | 5 (272.77/200 272.77/250 338.02/250) | — | 0:1;1:1;255:1 | 100.00 50.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 5/0/0 | 11 | — | Darwinian@0x40 Darwinian@1x40 | player0 player1 |
| mp_cts_3p_1.txt | 69 | 0:2;1:2;2:2 | 0/1 | 6 (277.28/200 500.00/300) | — | 0:1;1:1;2:1;255:3 | 100.00 50.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 18/0/0 | 24 | rockhead.shp temple2.shp | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 | player0 player1 player2 |
| mp_cts_3P_2.txt | 79 | 0:3;1:3;2:3 | 0/1 | 0 | — | 0:1;1:1;2:1;255:5 | 100.00 50.00 60.00 60.54 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 16/0/0 | 45 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 | player0 player1 player2 |
| mp_cts_3p_3.txt | 62 | — | 0/0 | 0 | 0:1;1:1;2:1 | 0:1;1:1;2:1;255:6 | 50.00 75.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 35/0/0 | 15 | — | — | player2 player1 player0 |
| mp_cts_4P_1.txt | 51 | 0:1;1:1;2:1;3:1;255:8 | 0/1 | 1 (654.87/400) | — | 0:1;1:1;2:1;3:1;255:3 | 100.00 60.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 23/0/0 | 7 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player0 player1 player2 player3 |
| mp_cts_4P_2.txt | 108 | 0:2;1:2;2:2;3:2 | 0/1 | 9 (400.14/100 400.14/200 400.14/300) | — | 0:1;1:1;2:1;3:1;255:6 | 100.00 50.00 63.38 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 21/0/0 | 59 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player2 player0 player1 player3 |
| mp_cts_4p_3.txt | 59 | 0:2;1:2;2:2;3:2 | 17/1 | 0 | — | 0:1;1:1;2:1;3:1;255:3 | 100.00 75.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 26/0/0 | 0 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player0 player1 player2 player3 |
| mp_domination_2P_1.txt | 70 | 0:1;1:1;255:6 | 13/1 | 1 (917.22/400) | — | — | — | — | — | — | — | — | 0 | 0/14 | 0 | 0 | 12/0/0 | 6 | rockhead.shp | Darwinian@0x45 Darwinian@1x45 | player0 player1 |
| mp_domination_2P_2.txt | 55 | 0:1;1:1;255:6 | 7/1 | 0 | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 13/0/0 | 21 | rockhead.shp | Darwinian@0x40 Darwinian@1x40 | start player0 player1 |
| mp_domination_3P_1.txt | 58 | 0:2;1:2;2:2;255:4 | 14/1 | 0 | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 10/0/0 | 12 | rockhead.shp temple4.shp | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 | player0 player1 player2 |
| mp_domination_4P_1.txt | 64 | 0:1;1:1;2:1;3:1;255:6 | 18/1 | 0 | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 27/0/0 | 8 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player0 player1 player2 player3 |
| mp_domination_4P_2.txt | 95 | 0:2;1:2;2:2;3:2;255:4 | 20/1 | 0 | — | — | — | — | — | — | — | — | 8 | 0/0 | 0 | 0 | 16/0/0 | 20 | rockhead.shp temple2.shp | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player3 player2 player0 player1 |
| mp_koth_2P_1.txt | 48 | 0:1;1:1;255:4 | 7/1 | 4 (400.00/200) | — | 255:3 | 150.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 15/0/0 | 7 | rockhead.shp | Darwinian@0x40 Darwinian@1x40 | start player1 player0 |
| mp_koth_2P_2.txt | 92 | 0:3;1:3;255:3 | 10/1 | 0 | 255:1 | — | — | — | — | — | — | — | 6 | 0/0 | 0 | 0 | 16/0/0 | 37 | carryspider.shp rockhead.shp statue.shp temple4.shp | — | player0 player1 |
| mp_koth_2P_3.txt | 61 | 0:2;1:2;255:6 | 11/1 | 4 (600.00/200 650.00/250 700.00/400) | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 21/0/0 | 11 | rockhead.shp | Darwinian@0x40 Darwinian@1x40 | player0 player1 |
| mp_koth_2P_3b.txt | 72 | 0:2;1:2;255:6 | 9/1 | 4 (650.00/400 700.00/400) | — | 255:4 | 100.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 24/0/0 | 13 | rockhead.shp temple2.shp | Darwinian@0x40 Darwinian@1x40 | player0 player1 |
| mp_koth_3P_1.txt | 96 | 0:1;1:1;2:1;255:6 | 23/1 | 7 (323.11/200 700.00/300) | 255:1 | 255:4 | 160.00 80.00 | — | — | — | 255:6 | — | 12 | 0/0 | 0 | 3 | 21/0/0 | 5 | rockhead.shp temple4.shp | Darwinian@0x45 Darwinian@1x45 Darwinian@2x45 | player0 player1 player2 |
| mp_koth_3P_2.txt | 51 | 0:2;1:2;2:2;255:3 | 14/1 | 1 (814.94/1000) | — | 255:4 | 80.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 10/0/0 | 9 | rockhead.shp | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 | player0 player1 player2 |
| mp_koth_3P_3.txt | 69 | 0:2;1:2;2:2;255:3 | 15/1 | 7 (297.73/300 437.74/300 448.19/300 500.00/300 536.16/300 571.66/300 573.77/300) | 255:1 | 0:1;1:1;2:1;255:2 | 101.17 140.00 150.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 16/0/0 | 15 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 | player2 player0 player1 |
| mp_koth_3P_4.txt | 59 | — | 0/0 | 0 | 0:2;1:2;2:2 | 255:1 | 86.20 | — | — | — | — | 255:6 | 0 | 12/0 | 0 | 0 | 12/0/0 | 22 | — | — | player0 player1 player2 |
| mp_koth_4P_1.txt | 84 | 0:1;1:1;2:1;3:1;255:6 | 18/1 | 9 (500.00/200 546.29/200 571.09/200 654.87/400) | — | 255:11 | 100.00 125.00 200.00 50.00 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 27/0/0 | 8 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player0 player1 player2 player3 |
| mp_koth_4P_3.txt | 107 | 0:2;1:2;2:2;3:2;255:4 | 20/1 | 0 | — | 255:12 | 100.00 220.00 233.01 50.00 90.00 | — | — | — | — | — | 8 | 0/0 | 0 | 0 | 16/0/0 | 20 | rockhead.shp temple2.shp | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player3 player2 player0 player1 |
| mp_koth_4P_4.txt | 68 | 0:3;1:3;2:3;3:3 | 4/1 | 1 (1169.84/1200) | 255:4 | 255:15 | 122.00 122.63 193.87 50.00 555.24 | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 13/0/0 | 18 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 Darwinian@3x40 | player0 player2 player1 player3 |
| mp_rocketriot_2P_2.txt | 161 | 255:2 | 0/1 | 0 | 0:1;1:1 | — | — | 0:1;1:1 | 0:3;1:3 | — | — | — | 0 | 0/0 | 0 | 2 | 15/0/10 | 44 | — | — | player1 player0 |
| mp_rocketriot_2p_3.txt | 186 | — | 0/0 | 0 | 0:1;1:1 | — | — | 0:1;1:1 | 0:3;1:3 | — | — | — | 0 | 0/31 | 0 | 0 | 17/0/6 | 24 | — | — | start player1 player0 |
| mp_rocketriot_2P_4.txt | 98 | — | 0/0 | 0 | 0:1;1:1 | — | — | 0:1;1:1 | 0:3;1:3 | — | — | — | 0 | 0/0 | 0 | 0 | 9/0/6 | 14 | — | — | player0 player1 |
| mp_rocketriot_3p_1.txt | 141 | — | 0/0 | 0 | 0:1;1:1;2:1 | — | — | 0:1;1:1;2:1 | 0:3;1:3;2:3 | — | — | — | 0 | 0/0 | 0 | 3 | 18/0/15 | 0 | — | — | player0 player1 player2 |
| mp_rocketriot_3P_2.txt | 184 | — | 0/0 | 0 | 0:1;1:1;2:1 | — | — | 0:1;1:1;2:1 | 0:3;1:3;2:3 | — | — | — | 0 | 0/0 | 0 | 4 | 19/0/21 | 18 | — | — | player0 player1 player2 |
| mp_rocketriot_3P_3.txt | 143 | — | 0/0 | 0 | 0:1;1:1;2:1 | — | — | 0:1;1:1;2:1 | 0:3;1:3;2:3 | — | — | 255:6 | 0 | 0/0 | 0 | 0 | 18/0/15 | 0 | — | Darwinian@0x8 Darwinian@1x8 Darwinian@2x8 | player0 player1 player2 |
| mp_rocketriot_4P_1.txt | 219 | 255:12 | 0/1 | 0 | 0:1;1:1;2:1;3:1 | — | — | 0:1;1:1;2:1;3:1 | 0:3;1:3;2:3;3:3 | — | — | — | 0 | 0/0 | 0 | 4 | 27/0/17 | 0 | — | — | player0 player1 player2 player3 |
| mp_sandbox.txt | 11 | — | 0/0 | 0 | — | — | — | — | — | — | — | — | 0 | 3/0 | 0 | 0 | 5/0/0 | 0 | — | — | start |
| mp_sandbox_all.txt | 22 | 1:4 | 11/1 | 0 | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 0/0/0 | 2 | — | — | start |
| mp_skirmish_2p_4.txt | 45 | 0:1;1:1;255:8 | 11/1 | 0 | 255:1 | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 13/0/0 | 9 | — | — | player0 player2 player1 |
| mp_skirmish_3P_1.txt | 189 | 0:2;1:2;2:2;255:9 | 0/1 | 0 | — | — | — | — | — | — | — | 255:6 | 0 | 0/95 | 18 | 3 | 42/0/0 | 9 | — | Darwinian@0x40 Darwinian@1x40 Darwinian@2x40 | player0 player1 player2 |
| mp_tankbattle_2P.txt | 18 | — | 0/0 | 0 | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 0/0/0 | 18 | — | Tank@0x1 Tank@1x1 | player0 player1 |
| mp_tankbattle_3P.txt | 18 | — | 0/0 | 0 | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 0/0/0 | 18 | — | Tank@0x1 Tank@1x1 Tank@2x1 | — |
| mp_tankbattle_4P.txt | 23 | — | 0/0 | 0 | — | — | — | — | — | — | — | — | 0 | 0/0 | 0 | 0 | 0/0/0 | 23 | — | Tank@0x1 Tank@1x1 Tank@2x1 Tank@3x1 | — |

### 6.1 Building-type usage matrix (top-level `mp_*` files)

Generated by `X4_md_tables.py buildings_matrix` [C]. Only one blocked type appears (`Statue` in `mp_sandbox`, dropped at parse under `BLOCK_OLD_DARWINIA_OBJECTS`); no unknown type names; every `TrunkPort/ControlTower/RadarDish/Incubator` row has `isGlobal 1` (else `ParseBuildings` would `AppReleaseAssert`) [C].

| Building type | Total (mp_* top level) | Files | Per file |
|---|---|---|---|
| AIObjective | 95 | 9 | assault_2P_1:12, assault_2P_2:9, assault_2p_3:11, assault_3P_2:12, assault_3p_1:10, assault_3p_3:6, assault_4P_1:14, assault_4P_2:18, assault_stress:3 |
| AIObjectiveMarker | 340 | 16 | assault_2P_1:36, assault_2P_2:28, assault_2p_3:25, assault_3P_2:27, assault_3p_1:27, assault_3p_3:29, assault_4P_1:24, assault_4P_2:44, assault_stress:10, rocketriot_2P_2:10, rocketriot_2P_4:6, rocketriot_2p_3:6, rocketriot_3P_2:21, rocketriot_3P_3:15, rocketriot_3p_1:15, rocketriot_4P_1:17 |
| AITarget | 1112 | 53 | assault_2P_1:41, assault_2P_2:29, assault_2p_3:22, assault_3P_2:37, assault_3p_1:31, assault_3p_3:39, assault_4P_1:31, assault_4P_2:51, assault_stress:30, benchmark_1:13, blitzkrieg_2P_3:17, blitzkrieg_2P_4:19, blitzkrieg_2p_1:12, blitzkrieg_2p_2:31, blitzkrieg_3P_1:19, blitzkrieg_3P_2:24, blitzkrieg_4P_2:37, cts_2p_1:9, cts_2p_2:13, cts_2p_3:11, cts_2p_4:5, cts_3P_2:16, cts_3p_1:18, cts_3p_3:35, cts_4P_1:23, cts_4P_2:21, cts_4p_3:26, domination_2P_1:12, domination_2P_2:13, domination_3P_1:10, domination_4P_1:27, domination_4P_2:16, koth_2P_1:15, koth_2P_2:16, koth_2P_3:21, koth_2P_3b:24, koth_3P_1:21, koth_3P_2:10, koth_3P_3:16, koth_3P_4:12, koth_4P_1:27, koth_4P_3:16, koth_4P_4:13, rocketriot_2P_2:15, rocketriot_2P_4:9, rocketriot_2p_3:17, rocketriot_3P_2:19, rocketriot_3P_3:18, rocketriot_3p_1:18, rocketriot_4P_1:27, sandbox:5, skirmish_2p_4:13, skirmish_3P_1:42 |
| ControlStation | 36 | 10 | assault_2P_1:3, assault_2P_2:3, assault_2p_3:5, assault_3P_2:3, assault_3p_1:3, assault_3p_3:3, assault_4P_1:4, assault_4P_2:3, assault_stress:3, koth_3P_1:6 |
| ControlTower | 3 | 2 | sandbox:1, sandbox_all:2 |
| EruptionMarker | 1 | 1 | blitzkrieg_3P_2:1 |
| EscapeRocket | 19 | 7 | rocketriot_2P_2:2, rocketriot_2P_4:2, rocketriot_2p_3:2, rocketriot_3P_2:3, rocketriot_3P_3:3, rocketriot_3p_1:3, rocketriot_4P_1:4 |
| FuelGenerator | 57 | 7 | rocketriot_2P_2:6, rocketriot_2P_4:6, rocketriot_2p_3:6, rocketriot_3P_2:9, rocketriot_3P_3:9, rocketriot_3p_1:9, rocketriot_4P_1:12 |
| FuelPipe | 178 | 7 | rocketriot_2P_2:22, rocketriot_2P_4:10, rocketriot_2p_3:56, rocketriot_3P_2:21, rocketriot_3P_3:24, rocketriot_3p_1:21, rocketriot_4P_1:24 |
| FuelStation | 57 | 7 | rocketriot_2P_2:6, rocketriot_2P_4:6, rocketriot_2p_3:6, rocketriot_3P_2:9, rocketriot_3P_3:9, rocketriot_3p_1:9, rocketriot_4P_1:12 |
| GunTurret | 207 | 14 | assault_2P_1:20, assault_2P_2:12, assault_2p_3:19, assault_3P_2:13, assault_3p_1:18, assault_3p_3:40, assault_4P_1:18, assault_4P_2:20, assault_stress:7, blitzkrieg_2P_4:18, blitzkrieg_2p_2:4, koth_3P_4:6, rocketriot_3P_3:6, skirmish_3P_1:6 |
| Incubator | 3 | 2 | sandbox:1, sandbox_all:2 |
| JumpPad | 23 | 3 | assault_2P_2:3, blitzkrieg_2P_3:2, skirmish_3P_1:18 |
| LaserFence | 703 | 15 | assault_2P_2:58, assault_2p_3:23, assault_3P_2:66, assault_3p_1:48, assault_4P_1:81, assault_4P_2:124, assault_stress:26, blitzkrieg_2P_3:21, blitzkrieg_2p_1:23, blitzkrieg_3P_1:34, blitzkrieg_3P_2:20, blitzkrieg_4P_2:39, domination_2P_1:14, rocketriot_2p_3:31, skirmish_3P_1:95 |
| MultiwiniaZone | 176 | 27 | benchmark_1:15, blitzkrieg_2P_3:3, blitzkrieg_2P_4:6, blitzkrieg_2p_1:3, blitzkrieg_2p_2:5, blitzkrieg_3P_1:7, blitzkrieg_3P_2:7, blitzkrieg_4P_2:9, cts_2p_1:4, cts_2p_2:4, cts_2p_3:4, cts_2p_4:3, cts_3P_2:8, cts_3p_1:6, cts_3p_3:9, cts_4P_1:7, cts_4P_2:10, cts_4p_3:7, koth_2P_1:3, koth_2P_3b:4, koth_3P_1:4, koth_3P_2:4, koth_3P_3:5, koth_3P_4:1, koth_4P_1:11, koth_4P_3:12, koth_4P_4:15 |
| PowerSplitter | 26 | 7 | rocketriot_2P_2:3, rocketriot_2P_4:3, rocketriot_2p_3:3, rocketriot_3P_2:4, rocketriot_3P_3:4, rocketriot_3p_1:4, rocketriot_4P_1:5 |
| PulseBomb | 9 | 9 | assault_2P_1:1, assault_2P_2:1, assault_2p_3:1, assault_3P_2:1, assault_3p_1:1, assault_3p_3:1, assault_4P_1:1, assault_4P_2:1, assault_stress:1 |
| Pylon | 301 | 17 | assault_2P_2:17, assault_3P_2:25, assault_3p_1:9, assault_4P_1:3, assault_4P_2:22, assault_stress:5, blitzkrieg_2p_1:1, blitzkrieg_3P_1:5, blitzkrieg_4P_2:5, domination_2P_1:6, rocketriot_2P_2:19, rocketriot_2P_4:18, rocketriot_2p_3:12, rocketriot_3P_2:40, rocketriot_3P_3:25, rocketriot_3p_1:26, rocketriot_4P_1:63 |
| PylonEnd | 88 | 17 | assault_2P_2:3, assault_3P_2:4, assault_3p_1:3, assault_4P_1:2, assault_4P_2:6, assault_stress:2, blitzkrieg_2p_1:2, blitzkrieg_3P_1:3, blitzkrieg_4P_2:4, domination_2P_1:2, rocketriot_2P_2:6, rocketriot_2P_4:6, rocketriot_2p_3:6, rocketriot_3P_2:9, rocketriot_3P_3:9, rocketriot_3p_1:9, rocketriot_4P_1:12 |
| RadarDish | 46 | 7 | assault_2p_3:4, assault_3p_1:2, assault_4P_1:6, domination_4P_2:8, koth_2P_2:6, koth_3P_1:12, koth_4P_3:8 |
| RestrictionZone | 35 | 14 | assault_2P_2:1, assault_2p_3:1, assault_3P_2:2, assault_3p_1:2, assault_4P_1:1, assault_4P_2:2, blitzkrieg_3P_2:3, blitzkrieg_4P_2:4, koth_3P_1:3, rocketriot_2P_2:2, rocketriot_3P_2:4, rocketriot_3p_1:3, rocketriot_4P_1:4, skirmish_3P_1:3 |
| SmokeMarker | 6 | 1 | assault_3p_1:6 |
| SolarPanel | 231 | 17 | assault_2P_2:10, assault_3P_2:12, assault_3p_1:9, assault_4P_1:6, assault_4P_2:18, assault_stress:6, blitzkrieg_2p_1:6, blitzkrieg_3P_1:9, blitzkrieg_4P_2:12, domination_2P_1:6, rocketriot_2P_2:21, rocketriot_2P_4:16, rocketriot_2p_3:15, rocketriot_3P_2:24, rocketriot_3P_3:18, rocketriot_3p_1:21, rocketriot_4P_1:22 |
| SpawnLink | 276 | 23 | benchmark_1:4, blitzkrieg_2P_3:9, blitzkrieg_2P_4:7, blitzkrieg_2p_2:8, cts_2p_3:6, cts_4p_3:17, domination_2P_1:13, domination_2P_2:7, domination_3P_1:14, domination_4P_1:18, domination_4P_2:20, koth_2P_1:7, koth_2P_2:10, koth_2P_3:11, koth_2P_3b:9, koth_3P_1:23, koth_3P_2:14, koth_3P_3:15, koth_4P_1:18, koth_4P_3:20, koth_4P_4:4, sandbox_all:11, skirmish_2p_4:11 |
| SpawnPoint | 286 | 34 | benchmark_1:24, blitzkrieg_2P_3:4, blitzkrieg_2P_4:3, blitzkrieg_2p_1:1, blitzkrieg_2p_2:6, cts_2p_1:6, cts_2p_2:4, cts_2p_3:4, cts_2p_4:4, cts_3P_2:9, cts_3p_1:6, cts_4P_1:12, cts_4P_2:8, cts_4p_3:8, domination_2P_1:8, domination_2P_2:8, domination_3P_1:10, domination_4P_1:10, domination_4P_2:12, koth_2P_1:6, koth_2P_2:9, koth_2P_3:10, koth_2P_3b:10, koth_3P_1:9, koth_3P_2:9, koth_3P_3:9, koth_4P_1:10, koth_4P_3:12, koth_4P_4:12, rocketriot_2P_2:2, rocketriot_4P_1:12, sandbox_all:4, skirmish_2p_4:10, skirmish_3P_1:15 |
| SpawnPointMaster | 34 | 34 | benchmark_1:1, blitzkrieg_2P_3:1, blitzkrieg_2P_4:1, blitzkrieg_2p_1:1, blitzkrieg_2p_2:1, cts_2p_1:1, cts_2p_2:1, cts_2p_3:1, cts_2p_4:1, cts_3P_2:1, cts_3p_1:1, cts_4P_1:1, cts_4P_2:1, cts_4p_3:1, domination_2P_1:1, domination_2P_2:1, domination_3P_1:1, domination_4P_1:1, domination_4P_2:1, koth_2P_1:1, koth_2P_2:1, koth_2P_3:1, koth_2P_3b:1, koth_3P_1:1, koth_3P_2:1, koth_3P_3:1, koth_4P_1:1, koth_4P_3:1, koth_4P_4:1, rocketriot_2P_2:1, rocketriot_4P_1:1, sandbox_all:1, skirmish_2p_4:1, skirmish_3P_1:1 |
| SpawnPopulationLock | 87 | 22 | assault_2P_1:1, assault_3P_2:1, benchmark_1:1, blitzkrieg_2P_4:2, blitzkrieg_2p_2:6, blitzkrieg_3P_1:4, blitzkrieg_3P_2:7, cts_2p_1:4, cts_2p_2:2, cts_2p_4:5, cts_3p_1:6, cts_4P_1:1, cts_4P_2:9, domination_2P_1:1, koth_2P_1:4, koth_2P_3:4, koth_2P_3b:4, koth_3P_1:7, koth_3P_2:1, koth_3P_3:7, koth_4P_1:9, koth_4P_4:1 |
| StaticShape | 193 | 19 | assault_2P_1:5, assault_2p_3:21, assault_3p_1:11, assault_3p_3:10, assault_4P_1:5, assault_4P_2:27, blitzkrieg_2P_4:19, cts_3p_1:8, domination_2P_1:1, domination_2P_2:5, domination_3P_1:11, domination_4P_2:18, koth_2P_1:5, koth_2P_2:12, koth_2P_3:3, koth_2P_3b:7, koth_3P_1:4, koth_3P_2:3, koth_4P_3:18 |
| Statue | 1 | 1 | sandbox:1 |
| Tree | 755 | 45 | assault_2P_1:13, assault_2P_2:36, assault_2p_3:5, assault_3P_2:6, assault_4P_1:26, benchmark_1:18, blitzkrieg_2P_4:9, blitzkrieg_2p_1:7, blitzkrieg_3P_1:16, blitzkrieg_3P_2:16, cts_2p_1:4, cts_2p_2:10, cts_2p_3:17, cts_2p_4:11, cts_3P_2:45, cts_3p_1:24, cts_3p_3:15, cts_4P_1:7, cts_4P_2:59, domination_2P_1:6, domination_2P_2:21, domination_3P_1:12, domination_4P_1:8, domination_4P_2:20, koth_2P_1:7, koth_2P_2:37, koth_2P_3:11, koth_2P_3b:13, koth_3P_1:5, koth_3P_2:9, koth_3P_3:15, koth_3P_4:22, koth_4P_1:8, koth_4P_3:20, koth_4P_4:18, rocketriot_2P_2:44, rocketriot_2P_4:14, rocketriot_2p_3:24, rocketriot_3P_2:18, sandbox_all:2, skirmish_2p_4:9, skirmish_3P_1:9, tankbattle_2P:18, tankbattle_3P:18, tankbattle_4P:23 |
| TrunkPort | 123 | 31 | assault_2P_1:5, assault_2P_2:6, assault_2p_3:3, assault_3P_2:9, assault_3p_1:7, assault_3p_3:8, assault_4P_1:8, assault_4P_2:10, assault_stress:7, benchmark_1:4, blitzkrieg_2P_3:3, blitzkrieg_2P_4:3, blitzkrieg_2p_1:2, blitzkrieg_2p_2:2, blitzkrieg_3P_1:3, blitzkrieg_3P_2:3, blitzkrieg_4P_2:4, cts_3p_3:3, koth_2P_2:1, koth_3P_1:1, koth_3P_3:1, koth_3P_4:6, koth_4P_4:4, rocketriot_2P_2:2, rocketriot_2P_4:2, rocketriot_2p_3:2, rocketriot_3P_2:3, rocketriot_3P_3:3, rocketriot_3p_1:3, rocketriot_4P_1:4, skirmish_2p_4:1 |
| Wall | 356 | 11 | assault_2P_1:78, assault_2P_2:14, assault_2p_3:20, assault_3P_2:29, assault_3p_1:94, assault_3p_3:25, assault_4P_2:34, assault_stress:30, blitzkrieg_2p_2:17, koth_3P_4:12, sandbox:3 |
| WallBuster | 3 | 1 | assault_2P_1:3 |

## [→ 08_levels_and_level_design] 7. Landscape parameters per level (all map-bearing files: 57 `mp_*`, 12 `map_*`, 30 `old/`)

`Landscape` block values, `LandscapeTiles` parameter ranges (min..max over the file's tiles), flatten-area count and `type:count` (0 Absolute, 1 Add, 2 Subtract, 3 Smooth, 4 Subtract2 — `LandscapeFlattenArea` enum), light count. Colour-ramp names shortened (`landscape_X.bmp`, `waves_X.bmp`, `water_X.bmp` in `data/terrain/`). Semantics of every field: P11 §23.3. [C]; generated by `X4_md_tables.py landscape`.

| File | worldSize | cellSize | grid | outsideHeight | maxHeight | landColour | waves | water | tiles | tile size | fracDim | heightScale | desiredHeight | genMethods | guidePower | lowland | flatten | flatten types | lights |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| map_biosphere.txt | 4580 | 20.00 | 229x229 | -26.13 | — | earth | earth | icecaps | 8 | 1124..1714 | 1.15 | 1.82 | 150..250 | 1 | 0 | 1..1.01 | 0 | — | 2 |
| map_containment.txt | 4442 | 17.20 | 259x259 | -2.25 | — | default | containmentfield | default | 12 | 124..1944 | 0.63..2.07 | 0.01..2.94 | 50..309 | 0 1 | 0 | 0.93..1.19 | 0 | — | 2 |
| map_escort.txt | 3972 | 12.87 | 309x309 | -26.13 | — | default | generator | default | 4 | 804..1884 | 1.14..4 | 0.22..2.07 | 84..290 | 1 | 0 4 | 1 | 0 | — | 2 |
| map_garden.txt | 2002 | 10.66 | 188x188 | -26.13 | — | default | default | default | 7 | 564..884 | 1.87..3.23 | 2..4.66 | 43..107 | 1 | 0 | 0.7..0.87 | 0 | — | 2 |
| map_generator.txt | 5372 | 15.61 | 345x345 | -26.13 | — | mine | generator | default | 11 | 284..1884 | 0.8..1.55 | 0..2.59 | 50..406 | 0 1 2 | 0 4 | 0..1 | 1 | 0:1 | 2 |
| map_launchpad.txt | 3677 | 15.76 | 234x234 | -15.39 | — | launchpad | launchpad | launchpad | 13 | 104..1450 | 0.71..5 | 1..3.12 | 42..300 | 0 1 | 0 1 2 | 1 | 0 | — | 2 |
| map_mine.txt | 3782 | 20.00 | 190x190 | -26.13 | — | mine | containmentfield | default | 13 | 234..1334 | 0.8..1.5 | 0.2..1.2 | 23..300 | 0 1 2 | 0 2 3 4 | 0..1 | 4 | 0:4 | 2 |
| map_pattern_buffer.txt | 3900 | 16.00 | 244x244 | -26.13 | — | icecaps | icecaps | icecaps | 6 | 384..1534 | 1.2..3 | 0..1 | 21..424 | 1 2 | 3 4 5 | 0..1 | 0 | — | 2 |
| map_receiver.txt | 4800 | 20.00 | 240x240 | -26.13 | — | mine | containmentfield | default | 18 | 424..1500 | 1..1.5 | 0.4..1 | 50..400 | 1 | 0 | 1 | 0 | — | 2 |
| map_sandbox.txt | 5000 | 20.35 | 246x246 | -15.39 | — | default | default | default | 2 | 1834..2224 | 0.57..1.2 | 1..1.12 | 398..400 | 0 1 | 3 5 | 1 | 0 | — | 2 |
| map_temple.txt | 4360 | 14.29 | 306x306 | -26.13 | — | desert | earth | icecaps | 9 | 934..1394 | 1.2 | 1 | 50..275 | 1 | 0 | 1..1.77 | 0 | — | 2 |
| map_yard.txt | 4990 | 21.17 | 236x236 | -26.13 | — | default | containmentfield | default | 12 | 677..1958 | 1.2..1.23 | 1.92..2 | 50..393 | 1 | 0 5 | 1 | 3 | 0:3 | 2 |
| mp_assault_2P_1.txt | 3000 | 22.00 | 137x137 | -10.00 | — | desert | default | default | 5 | 284..2884 | 1.2..5.72 | 1..2.5 | 106..200 | 1 | 3 4 | 0.05..1 | 0 | — | 2 |
| mp_assault_2P_2.txt | 2991 | 20.00 | 150x150 | -10.00 | 0.00 | firetop | icecaps | icecaps | 7 | 754..2734 | 1.2..1.8 | 1..2.5 | 200 | 1 | 2 4 | 0..1 | 0 | — | 2 |
| mp_assault_2p_3.txt | 5143 | 17.03 | 302x302 | -10.00 | 0.00 | dark | default | default | 10 | 124..1887 | 1.2..7.88 | 0..2 | 35..262 | 0 1 2 | 2 3 4 5 | 0..2 | 3 | 0:3 | 2 |
| mp_assault_3p_1.txt | 5000 | 22.32 | 225x225 | -10.00 | 0.00 | earth | earth | icecaps | 11 | 224..3134 | 1.2..3 | 1..4.25 | 58..200 | 1 | 0 2 4 5 | 1 | 2 | 2:2 | 2 |
| mp_assault_3P_2.txt | 5000 | 26.00 | 193x193 | -10.00 | 0.00 | launchpad | earth | icecaps | 1 | 4484 | 1.1 | 3.52 | 305 | 1 | 2 | 1 | 0 | — | 2 |
| mp_assault_3p_3.txt | 2549 | 9.95 | 257x257 | -10.00 | 0.00 | icecaps | icecaps | icecaps | 7 | 245..2376 | 1.2..3.02 | 0.98..1.49 | 62..176 | 0 1 2 | 0 3 4 5 | 1..1.06 | 1 | 3:1 | 2 |
| mp_assault_4P_1.txt | 4070 | 20.00 | 204x204 | -10.00 | 0.00 | desert | default | default | 15 | 384..1984 | 1.2..3.23 | 1 | 128..261 | 0 1 | 0 1 3 4 5 | 1 | 5 | 3:4;4:1 | 2 |
| mp_assault_4P_2.txt | 7000 | 26.00 | 270x270 | -10.00 | 0.00 | icecaps | earth | icecaps | 19 | 164..1534 | 0.87..2.11 | 1..6.07 | 89..303 | 1 | 0 2 3 4 | 1..1.15 | 5 | 2:2;3:3 | 2 |
| mp_assault_stress.txt | 4000 | 20.00 | 200x200 | -10.00 | — | default | default | default | 5 | 1074..1754 | 1.2 | 1 | 200 | 1 | 3 4 | 1 | 0 | — | 2 |
| mp_benchmark_1.txt | 3000 | 18.00 | 167x167 | -10.00 | — | stronghold | default | default | 10 | 234..2954 | 1.2 | 1..4 | 200 | 0 1 | 0 2 | 1 | 0 | — | 2 |
| mp_blitzkrieg_2p_1.txt | 3500 | 12.00 | 292x292 | -10.00 | 0.00 | earth | earth | icecaps | 6 | 284..1354 | 4.86 | 10 | 36..282 | 1 | 0 1 2 3 | 0.03..1 | 2 | 3:2 | 2 |
| mp_blitzkrieg_2p_2.txt | 4000 | 18.00 | 223x223 | -10.00 | 0.00 | earth | earth | icecaps | 8 | 514..3354 | 1.2..3.69 | 1 | 77..426 | 1 2 | 0 3 | 1 | 4 | 3:4 | 2 |
| mp_blitzkrieg_2P_3.txt | 3000 | 16.00 | 188x188 | -10.00 | 0.00 | default | default | default | 12 | 474..1544 | 4 | 4..5.37 | 49..237 | 1 | 1 4 | 0.13..1.87 | 0 | — | 2 |
| mp_blitzkrieg_2P_4.txt | 3000 | 20.00 | 150x150 | -10.00 | — | icecaps | earth | icecaps | 7 | 314..2884 | 1.2..5 | 1 | 117..381 | 1 | 0 2 4 | 1 | 1 | 3:1 | 2 |
| mp_blitzkrieg_3P_1.txt | 4000 | 12.00 | 334x334 | -10.00 | 0.00 | marble | containmentfield | yellow | 7 | 634..2234 | 1.2..4 | 1..4 | 200..296 | 1 | 0 1 | 1 | 1 | 3:1 | 2 |
| mp_blitzkrieg_3P_2.txt | 2938 | 16.00 | 184x184 | -10.00 | — | dark | default | default | 9 | 279..2279 | 1.2 | 1 | 200 | 1 | 0 4 5 | 1 | 0 | — | 2 |
| mp_blitzkrieg_4P_2.txt | 4000 | 20.00 | 200x200 | -10.00 | 0.00 | icecaps | earth | icecaps | 25 | 384..1464 | 1.2 | 1 | 91..200 | 1 | 0 1 4 | 1 | 0 | — | 2 |
| mp_cts_2p_1.txt | 2332 | 15.76 | 148x148 | -15.39 | 0.00 | launchpad | launchpad | launchpad | 4 | 242..1903 | 1.2..2.07 | 2.04..4 | 220..334 | 0 1 | 0 2 | 1 | 0 | — | 2 |
| mp_cts_2p_2.txt | 2332 | 15.76 | 148x148 | -15.39 | — | icecaps | icecaps | icecaps | 1 | 1784 | 3.25 | 1 | 201 | 2 | 5 | 1 | 0 | — | 2 |
| mp_cts_2p_3.txt | 2000 | 12.00 | 167x167 | -10.00 | 0.00 | jailbreak | containmentfield | default | 5 | 352..1655 | 0.84..1.2 | 1..2.5 | 38..294 | 1 | 0 2 5 | 1..1.13 | 0 | — | 2 |
| mp_cts_2p_4.txt | 2000 | 9.00 | 223x223 | -10.00 | — | earth | default | default | 3 | 284..1534 | 1.2..2.3 | 2.2..4 | 200..322 | 1 | 0 2 | 1..3 | 0 | — | 2 |
| mp_cts_3p_1.txt | 3000 | 15.80 | 190x190 | -10.00 | 0.00 | desert | yellow | default | 5 | 254..2924 | 1.2..2 | 1..4.82 | 200..320 | 1 | 0 2 | 1 | 0 | — | 2 |
| mp_cts_3P_2.txt | 2999 | 16.00 | 188x188 | -18.32 | 0.00 | icecaps | containmentfield | default | 17 | 439..1125 | 1.2..3.07 | 2 | 41..489 | 1 | 0 2 3 | 1 | 6 | 3:4;4:2 | 2 |
| mp_cts_3p_3.txt | 2570 | 12.00 | 215x215 | -10.00 | 200.00 | trippy | dull | icecaps | 5 | 542..2042 | 1.2..2.17 | 0..2.8 | 200..238 | 1 | 4 5 | 1..1.91 | 0 | — | 2 |
| mp_cts_4P_1.txt | 3000 | 16.00 | 188x188 | -10.00 | 0.00 | default | containmentfield | default | 14 | 167..1367 | 1.2 | 4.5 | 68..239 | 1 | 2 | 1.2..1.57 | 6 | 0:4;1:1;3:1 | 2 |
| mp_cts_4P_2.txt | 3428 | 16.00 | 215x215 | -21.05 | — | dark | default | default | 18 | 310..1013 | 2 | 6 | 120..283 | 1 | 2 | 1 | 0 | — | 2 |
| mp_cts_4p_3.txt | 2720 | 15.00 | 182x182 | -26.13 | 0.00 | icecaps | icecaps | icecaps | 7 | 480..2577 | 3..5 | 1 | 35..318 | 2 | 0 2 3 5 | 0 | 0 | — | 2 |
| mp_domination_2P_1.txt | 2000 | 12.00 | 167x167 | -10.00 | 0.00 | silver | yellow | yellow | 8 | 184..1934 | 0.96..4.03 | 1..2.87 | 49..305 | 0 1 | 0 2 3 | 1 | 0 | — | 2 |
| mp_domination_2P_2.txt | 2700 | 15.76 | 172x172 | -15.39 | 0.00 | launchpad | launchpad | launchpad | 3 | 834..2264 | 1.2 | 1..4 | 220..300 | 0 1 | 0 2 | 1 | 0 | — | 2 |
| mp_domination_3P_1.txt | 2345 | 16.00 | 147x147 | -10.00 | 0.00 | default | containmentfield | default | 1 | 2286 | 1.2 | 2.29 | 200 | 1 | 2 | 0.88 | 0 | — | 2 |
| mp_domination_4P_1.txt | 3000 | 16.00 | 188x188 | -10.00 | 0.00 | default | containmentfield | default | 15 | 167..1367 | 1.2 | 4.5 | 88..239 | 1 | 2 | 1.2..1.57 | 1 | 0:1 | 2 |
| mp_domination_4P_2.txt | 3000 | 14.00 | 215x215 | -30.00 | 0.00 | mine | generator | default | 5 | 344..1434 | 1.2 | 4 | 200 | 1 | 2 | 1.1 | 0 | — | 2 |
| mp_koth_2P_1.txt | 2700 | 15.76 | 172x172 | -15.39 | — | launchpad | launchpad | launchpad | 3 | 834..2264 | 1.2 | 1..4 | 220..300 | 0 1 | 0 2 | 1 | 0 | — | 2 |
| mp_koth_2P_2.txt | 2677 | 14.00 | 192x192 | -10.00 | 0.00 | shrine | default | default | 3 | 1335 | 1.2 | 4 | 200 | 1 | 2 | 1.1 | 0 | — | 2 |
| mp_koth_2P_3.txt | 2754 | 16.00 | 173x173 | -10.00 | 0.00 | icecaps | icecaps | icecaps | 10 | 384..946 | 1.2 | 3 | 61..200 | 1 | 2 | 0.9..1.4 | 0 | — | 2 |
| mp_koth_2P_3b.txt | 2754 | 16.00 | 173x173 | -10.00 | 0.00 | icecaps | icecaps | icecaps | 11 | 384..1150 | 1.2 | 3 | 34..200 | 1 | 2 | 0.9..1.4 | 4 | 0:4 | 2 |
| mp_koth_3P_1.txt | 3000 | 12.00 | 250x250 | -10.00 | 0.00 | earth | earth | icecaps | 8 | 584..1294 | 1.2 | 2.7 | 200..242 | 1 | 2 | 1.2 | 1 | 3:1 | 2 |
| mp_koth_3P_2.txt | 2345 | 16.00 | 147x147 | -10.00 | — | default | containmentfield | default | 1 | 2286 | 1.2 | 2.29 | 200 | 1 | 2 | 0.88 | 0 | — | 2 |
| mp_koth_3P_3.txt | 2800 | 16.00 | 175x175 | -18.32 | 0.00 | mine | containmentfield | default | 8 | 567..949 | 1.2 | 2..3 | 63..200 | 1 | 2 | 0.9..1.22 | 3 | 0:3 | 2 |
| mp_koth_3P_4.txt | 2750 | 12.00 | 230x230 | -10.00 | 0.00 | earth | icecaps | icecaps | 3 | 294..2244 | 1.2..2.14 | 1..6.4 | 605..846 | 1 2 | 0 4 | 1 | 1 | 3:1 | 2 |
| mp_koth_4P_1.txt | 3000 | 16.00 | 188x188 | -10.00 | 0.00 | default | containmentfield | default | 15 | 167..1367 | 1.2 | 4.5 | 88..239 | 1 | 2 | 1.2..1.57 | 3 | 0:2;3:1 | 2 |
| mp_koth_4P_3.txt | 3000 | 14.00 | 215x215 | -30.00 | 0.00 | mine | generator | default | 5 | 344..1434 | 1.2 | 4 | 200 | 1 | 2 | 1.1 | 0 | — | 2 |
| mp_koth_4P_4.txt | 3000 | 18.00 | 167x167 | -10.00 | — | stronghold | default | default | 10 | 234..2954 | 1.2 | 1..4 | 200 | 0 1 | 0 2 | 1 | 0 | — | 2 |
| mp_rocketriot_2P_2.txt | 5000 | 18.00 | 278x278 | -6.93 | 0.00 | dark | icecaps | icecaps | 6 | 384..1734 | 1.2..2 | 2.07..3 | 200..270 | 1 | 0 2 | 1 | 0 | — | 2 |
| mp_rocketriot_2p_3.txt | 3756 | 27.08 | 139x139 | -26.13 | 0.00 | mine | generator | default | 15 | 284..2602 | 2.52..5 | 0.95..1.15 | 66..543 | 2 | 0 1 2 | 0.39..1.17 | 0 | — | 2 |
| mp_rocketriot_2P_4.txt | 2000 | 9.87 | 203x203 | -10.00 | 0.00 | earth | default | default | 1 | 1964 | 1.2 | 1 | 200 | 1 | 4 | 1 | 0 | — | 2 |
| mp_rocketriot_3p_1.txt | 5000 | 18.00 | 278x278 | -15.39 | 0.00 | launchpad | launchpad | launchpad | 7 | 1320..1360 | 1.2 | 3 | 127..282 | 1 | 1 2 | 1 | 0 | — | 2 |
| mp_rocketriot_3P_2.txt | 4285 | 18.00 | 239x239 | -26.13 | 0.00 | firetop | containmentfield | default | 22 | 327..1054 | 1..1.2 | 0.4..1 | 30..290 | 2 | 0 2 3 4 | 0..1 | 0 | — | 2 |
| mp_rocketriot_3P_3.txt | 4500 | 16.00 | 282x282 | -10.00 | 0.00 | mine | earth | launchpad | 7 | 1254..1574 | 1.12 | 4 | 484 | 1 | 2 | 1.3 | 3 | 3:3 | 2 |
| mp_rocketriot_4P_1.txt | 4186 | 18.00 | 233x233 | -10.00 | 0.00 | icecaps | yellow | icecaps | 17 | 265..1068 | 1.7 | 1..4.74 | 200..260 | 0 1 | 0 4 | 0..1 | 0 | — | 2 |
| mp_sandbox.txt | 3290 | 20.35 | 162x162 | -15.39 | — | default | default | default | 1 | 2224 | 0.57 | 1.12 | 398 | 0 | 3 | 1 | 0 | — | 2 |
| mp_sandbox_all.txt | 3290 | 20.35 | 162x162 | -15.39 | — | default | default | default | 1 | 2224 | 0.57 | 1.12 | 398 | 0 | 3 | 1 | 0 | — | 2 |
| mp_skirmish_2p_4.txt | 2000 | 12.00 | 167x167 | -10.00 | 0.00 | earth | earth | icecaps | 2 | 384..1834 | 4.86..4.9 | 10 | 125..200 | 1 | 0 | 1 | 0 | — | 2 |
| mp_skirmish_3P_1.txt | 6000 | 20.00 | 300x300 | -10.00 | 0.00 | firetop | default | default | 10 | 744..2854 | 1.4 | 1.07..4 | 303..961 | 1 | 0 2 | 1..1.74 | 1 | 2:1 | 2 |
| mp_tankbattle_2P.txt | 2000 | 12.00 | 167x167 | -10.00 | — | default | default | default | 1 | 1934 | 1.82 | 3.96 | 200 | 1 | 2 | 0.72 | 0 | — | 2 |
| mp_tankbattle_3P.txt | 2000 | 12.00 | 167x167 | -10.00 | — | jailbreak | earth | icecaps | 2 | 384..1924 | 1.2..1.51 | 1..4.06 | 200..396 | 1 | 0 2 | 1..1.08 | 0 | — | 2 |
| mp_tankbattle_4P.txt | 2000 | 12.00 | 167x167 | -27.89 | — | dark | default | default | 1 | 1934 | 0.74 | 2.9 | 368 | 1 | 2 | 1.25 | 0 | — | 2 |
| old/mp_assault1.txt | 2000 | 12.00 | 167x167 | -10.00 | — | default | default | default | 7 | 514..614 | 1.2..3.08 | 1..5 | 66..250 | 1 | 0 3 4 | 1 | 0 | — | 2 |
| old/mp_assault_3P_1.txt | 4000 | 20.00 | 200x200 | -10.00 | — | icecaps | default | default | 7 | 1034..2024 | 1..2.58 | 0.4..1 | 29..393 | 1 2 | 2 3 4 5 | 0..1 | 1 | 0:1 | 2 |
| old/mp_cauldron.txt | 2220 | 12.00 | 185x184 | -10.00 | — | mine2 | icecaps | icecaps | 3 | 384..1924 | 0.8 | 2 | 40..240 | 0 | 0 | 1 | 0 | — | 2 |
| old/mp_chess1.txt | 2676 | 12.00 | 223x223 | -10.00 | — | default | default | default | 1 | 1985 | 1.2 | 1 | 200 | 2 | 4 | 1 | 0 | — | 2 |
| old/mp_crossroads.txt | 3000 | 14.00 | 215x215 | -10.00 | — | desert | desert | icecaps | 1 | 2900 | 1.2 | 1 | 200 | 1 | 5 | 1 | 0 | — | 2 |
| old/mp_crucible.txt | 2000 | 12.00 | 167x167 | -10.00 | — | desert | default | default | 2 | 450..1604 | 1.8 | 1.2 | 75..200 | 1 | 0 | 0.5 | 1 | 0:1 | 2 |
| old/mp_firetop.txt | 2000 | 12.00 | 167x167 | -10.00 | — | firetop | default | default | 1 | 1900 | 1.2 | 1 | 200 | 1 | 5 | 1 | 0 | — | 2 |
| old/mp_gateway.txt | 2950 | 16.00 | 185x185 | -10.00 | — | mine | containmentfield | default | 3 | 1624 | 1.7 | 1.4 | 170..200 | 1 | 2 | 0.5 | 0 | — | 2 |
| old/mp_ghosttown.txt | 3000 | 16.00 | 188x188 | -10.00 | — | icecaps | default | default | 1 | 2750 | 1.8 | 1.8 | 230 | 1 | 0 | 1 | 0 | — | 2 |
| old/mp_hamburgerhill.txt | 2000 | 12.00 | 167x167 | -10.00 | — | earth | earth | icecaps | 1 | 1834 | 4.86 | 10 | 200 | 1 | 0 | 1 | 0 | — | 2 |
| old/mp_kingspass.txt | 2533 | 18.00 | 141x141 | -10.00 | — | containment | desert | default | 1 | 2447 | 1.2 | 1 | 200 | 1 | 5 | 1 | 0 | — | 2 |
| old/mp_knightspatch.txt | 2000 | 12.00 | 167x167 | -10.00 | — | earth | default | default | 3 | 574..1950 | 2 | 7 | 50..200 | 1 | 0 | 1 | 0 | — | 1 |
| old/mp_koth_2P_4.txt | 3000 | 16.00 | 188x188 | -10.00 | — | icecaps | launchpad | icecaps | 16 | 750..1154 | 1 | 3.5 | 200 | 1 | 5 | 1 | 0 | — | 2 |
| old/mp_marblecanyon.txt | 3000 | 12.00 | 250x250 | -10.00 | — | marble2 | default | default | 2 | 1664..2510 | 0.8..3 | 1..1.5 | 200..260 | 1 2 | 5 | 0..1 | 0 | — | 2 |
| old/mp_rocketriot_2p_1.txt | 5372 | 20.00 | 269x269 | -26.13 | — | mine | generator | default | 5 | 1294..2094 | 1.2..1.5 | 1 | 97..200 | 0 1 | 1 | 1 | 0 | — | 2 |
| old/mp_sabrepoint.txt | 2500 | 12.00 | 209x209 | -10.00 | — | mine | generator | default | 4 | 544..1900 | 1.2 | 1.5 | 200 | 1 | 0 | 0.8 | 0 | — | 2 |
| old/mp_shaman.txt | 2393 | 16.00 | 150x150 | -10.00 | — | desert | default | default | 10 | 622..814 | 2 | 3 | 200..270 | 1 | 0 | 0.5 | 0 | — | 2 |
| old/mp_shaman2.txt | 1500 | 12.00 | 125x125 | -10.00 | — | containment | icecaps | icecaps | 1 | 1457 | 2 | 3 | 200 | 1 | 0 | 0.5 | 0 | — | 2 |
| old/mp_shaman_2P.txt | 2300 | 14.00 | 165x165 | -12.37 | — | default | default | default | 2 | 508..1939 | 1.2 | 1..4 | 250..259 | 0 1 | 0 2 | 1 | 0 | — | 2 |
| old/mp_shaman_2P_1.txt | 2500 | 16.00 | 157x157 | -10.00 | — | earth | containmentfield | default | 11 | 547..1329 | 1.9 | 6.87 | 107..200 | 1 | 2 | 1 | 0 | — | 1 |
| old/mp_shaman_2P_2.txt | 4000 | 20.00 | 200x200 | -10.00 | — | default | generator | default | 13 | 364..1854 | 1 | 1..2.7 | 54..214 | 1 | 2 5 | 1..1.2 | 0 | — | 2 |
| old/mp_shaman_2P_3.txt | 2754 | 16.00 | 173x173 | -10.00 | — | icecaps | icecaps | icecaps | 11 | 384..1150 | 1.2 | 3 | 34..200 | 1 | 2 | 0.9..1.4 | 4 | 0:4 | 2 |
| old/mp_shaman_3P_1.txt | 3000 | 16.00 | 188x188 | -10.00 | — | mine | containmentfield | default | 23 | 361..979 | 1.2 | 3 | 200 | 1 | 2 | 0.9 | 0 | — | 2 |
| old/mp_shaman_3P_2.txt | 2300 | 12.00 | 192x192 | -10.00 | — | marble | default | default | 2 | 494..1982 | 1.2 | 4 | 50..200 | 1 | 2 | 0.8 | 0 | — | 2 |
| old/mp_shaman_3P_3.txt | 3000 | 12.00 | 250x250 | -12.98 | — | shrine | default | default | 12 | 494..854 | 1.2 | 4 | 200 | 1 | 2 | 1.3 | 0 | — | 2 |
| old/mp_silverisle.txt | 3391 | 20.00 | 170x170 | -10.00 | — | silver | default | default | 13 | 333..1161 | 3 | 0.5..1 | 50..200 | 1 | 0 | 1 | 1 | 0:1 | 2 |
| old/mp_taintedflats.txt | 3000 | 12.00 | 250x250 | -10.00 | — | default | default | default | 1 | 2634 | 2 | 2 | 249 | 1 | 1 | 1 | 0 | — | 2 |
| old/mp_temple.txt | 4360 | 14.29 | 306x306 | -26.13 | — | desert | earth | icecaps | 9 | 934..1394 | 1.2 | 1 | 50..275 | 1 | 0 | 1..1.77 | 0 | — | 2 |
| old/mp_thestretch.txt | 3252 | 16.00 | 204x204 | -10.00 | — | earth | earth | icecaps | 1 | 3089 | 1.2 | 1 | 100 | 1 | 5 | 1 | 0 | — | 2 |
| old/mp_twistedhighlands.txt | 2570 | 12.00 | 215x215 | -10.00 | — | default | default | default | 1 | 2234 | 1.2 | 1.99 | 330 | 1 | 1 | 1 | 0 | — | 2 |

## [→ 08_levels_and_level_design] 8. Level-design capabilities evidenced by the shipped data

What the designers actually used (top-level `mp_*`, 57 files, unless stated) — a new game's level format must support at least this envelope. All [C] from `X4_levels.csv` / ad-hoc aggregation over the same files; field semantics P11 §23.3.

### 8.1 Landscape envelope

| Parameter | Used range / values | Notes |
|---|---|---|
| World size | 2000–7000 units, **square in every top-level file** (28 distinct sizes); only `old/mp_cauldron` is non-square (2220×2200) | most common 3000 (12 files), 2000 (8) |
| `cellSize` | 9.00–27.08 (17 distinct) → height grid 137×137 … 334×334 cells | finer cells on small CTS/RR maps (9.0 `mp_cts_2p_4`, 9.87 `mp_rocketriot_2P_4`, 9.95 `mp_assault_3p_3`) |
| `outsideHeight` | −6.93 … −30.0 (−10.00 in 40 files) | 8 distinct values |
| `maxHeight` | written as 0.00 in 40 files, 200.00 in `mp_cts_3p_3` only, absent in 16 | parsed into `LandscapeDef::m_maxHeight` (P11) |
| Colour ramps (`data/terrain/`) | land 14 distinct: icecaps 10, default 10, earth 8, dark 5, launchpad 5, mine 5, desert 3, firetop 3, stronghold 2, jailbreak 2, marble 1, trippy 1, silver 1, shrine 1; waves 8: default 17, earth 11, containmentfield 10, icecaps 8, launchpad 4, yellow 3, generator 3, dull 1; water 4: default 30, icecaps 20, launchpad 5, yellow 2 | several are Darwinia location names (launchpad, mine, containment, generator) = reused Darwinia art (IP register) |
| Tiles per map | 1–25 (445 tiles total) | |
| Tile `size` | 124–4484 | |
| Tile `posY` (base height) | −25.93 … 83.9 | |
| `fractalDimension` | 0.57–7.88 | |
| `heightScale` | 0.0–10.0 | |
| `desiredHeight` | 30–961 | |
| `generationMethod` | 0 ×30, 1 ×356, 2 ×59 | all three methods used |
| `lowlandSmoothingFactor` | 0.0–3.0 | |
| Guide-grid power | 0 ×71, 1 ×26, 2 ×209, 3 ×30, 4 ×93, 5 ×16 → hand-painted guide grids on 374/445 tiles | grid side = 2^power (P11 §23.5) |
| Flatten areas | 53 areas in 21 files (0–6 per file); types 0 Absolute ×17, 1 Add ×1, 2 Subtract ×5, 3 Smooth ×27, 4 Subtract2 ×3; half-size 8.4–1000; centre y −26 … 886.2; threshold −3.9 … 348.9 | Smooth is the most-used type |

### 8.2 Lighting and effects
- **Exactly two directional lights in every top-level map** (and in every `map_*`; only `old/mp_knightspatch` and `old/mp_shaman_2P_1` have one). Colours are over-bright multipliers: component range 0.26–5.23 (R), 0.26–3.15 (G), 0.00–2.07 (B); the common pattern is one neutral grey "fill" light (equal RGB, 0.33–1.25) plus one warm "key" light (R > G > B, e.g. `2.77 1.07 0`, `3.19 2.07 1.22`) [C] data, [I] the fill/key reading. Direction vectors are normalised at load; the `y` component used is 0.00–0.86 [C].
- Weather/visual `Effects`: snow, lightning, dustwind, SpecialLighting (see §5). Music is **not** a level property: no level block or option names a track (the level grammar has none) [C] `code/level_file.cpp`; music selection belongs to P16.

### 8.3 Building placement patterns
- **Spawn economy graph** (Domination/KOTH/CTS/Blitzkrieg 2P maps): every map with `SpawnPoint`s has exactly **one `SpawnPointMaster`** whose link list starts a tree of `SpawnLink`s (link lists = next link ids and/or spawn-point ids) reaching every spawn point (34 files; 0–23 links) [C]; graph semantics P06 (`drafts/P06_spawn_graph.csv`). Start positions own 1–3 spawn points each; 3–9 neutral capturable ones sit between them.
- `SpawnPopulationLock` areas (87 in 22 files): radius 272.77–1169.84, max population 100–1200 — used to cap crowding at hubs.
- AI hints: `AITarget` (1112 in 53 files: link id −1…206, priority modifier −1…1, `checkCliffs` 0/1); Assault-style `AIObjective` chains (95 in 9 files, `nextObjective` link) with `AIObjectiveMarker`s (340 in 16 files: scan range 0–306.6, objective id, armour-objective flag, pickup-only flag); P08 owns their use.
- Fortification kit: `Wall` 356 (11 files; optional objective link), `LaserFence` 703 (15 files; next-fence id chain, scale 0.25–3.0, mode 2 Enabled ×658 / 0 Disabled ×20 / 5 AlwaysOn ×23 / 4 NeverOn ×2 — `LaserFence::Mode*`), `GunTurret` 207 (14 files, **all neutral**, turret type in the `state` field: 0 Standard ×196, 3 Flame ×11 — `GunTurret::GunTurretType*`; target pos/radius/force all 0), `RestrictionZone` 35 (14 files; size 300–1118.8; `blockPowerups` 0/1), `JumpPad` 23 (3 files; force 76.17–227.38, angle 0.51–1.08), `RadarDish` 46 (7 files; flags `forceConnection`, `autoConnectAtStart`, optional valid-link list).
- Power/fuel chains: `SolarPanel` 231, `Pylon` 301, `PylonEnd` 88, `PowerSplitter` 26 (each line = next power link; splitter = link list) feed `FuelGenerator` 57 → `FuelPipe` 178 → `FuelStation` 57 → `EscapeRocket` 19 (Rocket Riot), and appear decoratively/functionally on several Assault and Blitzkrieg maps.
- Set dressing: `Tree` 755 (45 files; 11 extra tokens: height, budsize, pushUp, pushOut, iterations, seed, two colour words, flags — P07), `StaticShape` 193 (19 files; scale 0.83–6.1; shapes `beachheadobstruction.shp` 67, `temple2.shp` 56, `rockhead.shp` 32, `temple4.shp` 27, `temple3.shp` 8, `spaceship.shp` 1), effect markers `EruptionMarker` (1, `mp_blitzkrieg_3P_2`), `SmokeMarker` (6, `mp_assault_3p_1`), `WallBuster` (3, `mp_assault_2P_1`).

### 8.4 Starting units, camera, scripting
- `InstantUnits`: starting Darwinians per position are 8–150 (40 on most 2–4P maps; 45 on `mp_domination_2P_1`, `mp_koth_3P_1`); Assault gives only the defending side(s) starting Darwinians (+1–2 `Armour`); `Tank` ×1 per position on TankBattle; `Darwinian@5×35` on `mp_assault_3p_1` = a Futurewinian (team 5) garrison. No instant unit uses a route, waypoint or `runAsTask` [C].
- Camera mounts: `player<p>` for each start position (`main.cpp` cuts to `"player%d"` of the local team's position on game start) and optionally `start` (6 maps; `DoEnterLocation` targets `"start"` after load, fallback position `(maxX, 1000, maxX)` looking `(-1, -0.7, -1)`) [C] `code/main.cpp`. Missing mounts: `mp_assault_3p_3`, `mp_tankbattle_3P/4P` have none; `mp_blitzkrieg_2p_1`, `mp_skirmish_2p_4` define a spurious `player2` on 2-player maps.
- **Unused by any shipped level** (`data/levels/**`): `Routes` blocks, camera-animation entries (47 `CameraAnimations` blocks, all empty), `RunningPrograms`, `LeaderboardStats`, `Difficulty` (except 3 Darwinia attract missions: `CreatedAsDifficulty 1`) [C]. `PrimaryObjectives` in Multiwinia maps is `NeverTrue` or absent — level files never script a Multiwinia win condition [C].

### 8.5 Map editor
The in-game editor that produced these files (`LocationEditor`, editor windows, `MultiwiniaWindow`/`NewMapWindow` with default file `mp_blank.txt`/`mp_newmap.txt` — neither present in the tree) is compiled only under `LOCATION_EDITOR` [B:EDITOR]; specified by P17a. `LevelFile::Save` writes to `levels/<map>` only under `TARGET_DEBUG`, else to `<profile>/maps/` [C] `code/level_file.cpp:Save`.

