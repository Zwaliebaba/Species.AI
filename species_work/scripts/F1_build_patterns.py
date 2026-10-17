#!/usr/bin/env python3
"""F1: build the hard-fail and soft-flag pattern lists and the original-string list.

Usage:
  python3 -I F1_build_patterns.py <ORIG dir> <f1 dir> <WORK dir>

Reads  <f1>/identifiers.tsv, <f1>/prose_vocab.tsv, <WORK>/rename_map.tsv,
       ORIG/41 §5.2, ORIG/91 §4, ORIG/_work/drafts/{X4_levels,P17b_strings,P02_game_modes_strings,
       P17a_ui_framework_editor_ui_strings}.csv, ORIG/04, ORIG/06, ORIG/09 (quoted strings).
Writes <WORK>/forbidden_patterns.tsv, <WORK>/soft_patterns.tsv, <WORK>/original_strings.txt,
       <f1>/map_names.tsv, <f1>/string_keys.tsv, <f1>/pattern_build_report.txt

Pattern files: columns pattern, flags (i = case-insensitive, empty = case-sensitive), class, note.
Every pattern is compiled here; the build aborts on an invalid regex or a duplicate.
ORIG content is data only.
"""
import csv
import os
import re
import sys
import unicodedata

# ---------------------------------------------------------------------------
# 1 Curated hard-fail patterns
# ---------------------------------------------------------------------------
BRAND = [
    # product, studio and their forms
    (r'multiwin(?:i|a_)', 'i', 'product name in every form (multiwinia, multiwinian(s), Multiwinia* identifiers, multiwina_ typo keys)'),
    (r'darwinia', 'i', 'product and prequel name in every form (darwinia, darwinian(s), DARWINIA_*, NET_DARWINIA_*, DarwiniaWindow)'),
    (r'darwinn', 'i', 'misspelt sound-group names darwinnian*'),
    (r'\bdarwin_\w+', 'i', 'voice set prefix darwin_*'),
    (r'darwin[ _]?research', 'i', 'in-fiction corporation and its logo'),
    (r'introversion', 'i', 'studio name, folders, hosts, bundle ids'),
    (r'\bMW\b', '', 'product abbreviation'),
    (r'\bmw(?:title|logo|help|sounds)\b', 'i', 'product-prefixed asset names'),
    (r'\bivlogo\b', 'i', 'studio splash art'),
    (r'\bDWs\b', '', 'unit abbreviation in the frame overlay'),
    (r'<GAME_TITLE>|\bGAME_TITLE\b', '', 'placeholder resolved to Species'),
    # characters, factions, fiction
    (r'sepulveda', 'i', 'mentor character'),
    (r'\bDr\.? S\b', '', 'mentor character short form'),
    (r'protologic', 'i', 'story material'),
    (r'evilinian', 'i', 'faction nickname'),
    (r'futurewinian', 'i', 'faction nickname'),
    (r'rocketwinian', 'i', 'crate outcome nickname'),
    (r'futurewar', 'i', 'exporter and fiction name'),
    (r'squadd?ie', 'i', 'unit name (Squadie, squaddie_*, InsertionSquadie)'),
    (r'virii', 'i', 'creature name in every form'),
    (r'triffid', 'i', 'third-party literary creature name'),
    (r'\bwyndham\b', 'i', 'author of the literary creature'),
    (r'soul ?destroyer', 'i', 'creature name'),
    (r'spore ?generator', 'i', 'creature name'),
    (r'space ?invader', 'i', 'third-party trade mark echo'),
    (r'smell of napalm', 'i', 'film quote'),
    (r'\bpokey\b', 'i', 'third-party sound chip name'),
    (r'speccy', 'i', 'font and loader homage names'),
    (r'\bzx ?spectrum\b', 'i', 'third-party computer homage'),
    (r'dmacrew', 'i', 'demo-scene homage art'),
    (r'\bThe Beast\b', '', 'debug creature name'),
    # framework and library brands of the original
    (r'\bEclipse\b', '', 'UI framework name (capitalised proper noun)'),
    (r'\bEclipse[A-Z_]\w*', '', 'UI framework identifiers (EclipseLMouseDown and the rest)'),
    (r'\beclipse(?:\.(?:cpp|h)\b|/)', 'i', 'UI framework source files and folder'),
    (r'\bEcl(?:Window|Button)\b', '', 'UI framework base classes'),
    (r'\bsystem ?iv\b', 'i', 'library brand (SystemIV, systemiv)'),
    # the studio's other titles
    (r'\bUplink\b', '', 'studio title (capitalised)'),
    (r'\bdefcon\b', 'i', 'studio title and service identifiers'),
    (r'subversion', 'i', 'studio title and the original outcome and turret name'),
    (r'prison ?architect', 'i', 'studio title'),
    (r'scanner ?sombre', 'i', 'studio title'),
    # codenames, profiles, test names
    (r'\bPecan\b', '', 'codename'),
    (r'\bHummingbird\b', '', 'codename'),
    (r'AccessAllAreas|AttractMode|unlockall', 'i', 'developer profiles'),
    (r'godofcode', 'i', 'test-bed codename'),
    (r'puritycontrol|purity_control|\bPurity Control\b', 'i', 'kill-switch data set'),
    (r'\bTestbed (?:Server|Client)\b', '', 'test-bed captions'),
    (r'\bOpen/CPU\b|\(CPU\)|\(LEFT\)|\(DEMO\)', '', 'team-type texts'),
    # third-party brands, distributors, partners
    (r'trueskill', 'i', 'Microsoft trade mark'),
    (r'\bxbla\b', 'i', 'Xbox Live Arcade abbreviation'),
    (r'xbox ?live ?arcade', 'i', 'distribution brand'),
    (r'xbox ?360', 'i', 'console brand naming'),
    (r'\b360 control help\b', 'i', 'console help page'),
    (r'ControlXBoxMenuCycle', 'i', 'console-named controls'),
    (r'_xbox\b', 'i', 'console phrase suffix'),
    (r'\bAmbrosia\b', '', 'distributor'),
    (r'\bAMBR\b', '', 'distributor key prefix'),
    (r'\bBuka\b', '', 'distributor'),
    (r'\beGames\b', '', 'distributor'),
    (r'cinemaware', 'i', 'distributor'),
    (r'msn_oberon|\bOberon\b', '', 'portal partner'),
    (r'\bLogitech G15\b|\bG15\b', '', 'hardware partner'),
    (r'\bambx\b', 'i', 'hardware partner'),
    (r'`Sparkle`|\bSparkle(?= update|\.framework| framework| updater|\)|,? sleep)|\+ Sparkle\b', '', 'Mac updater framework (identifier and update-check contexts only)'),
    (r'carrybillboard', 'i', 'magazine billboard meshes'),
    (r'\bprices_(?:ambrosia|steam)\b|\bprices\.bmp\b', 'i', 'store art'),
    (r'\bapp ?id 1520\b', 'i', 'store product id'),
    # hosts, keys and product identifiers
    (r'80\.175\.29\.66', '', 'route-probe address'),
    (r'0B9432AF', 'i', 'crash-report module id'),
    (r'F58175C7-E99C-4151-800D-7BB5E89E4960', 'i', 'Game Explorer GUID'),
    (r'redshirt', 'i', 'text cipher marker'),
    (r'\bhsh\b', '', 'key hash prefix'),
    (r'\bmultiwinia\.co\.uk\b|\bsavedarwinia\b', 'i', 'product hosts'),
    # personal data
    (r'\bLeander\b|\bChris\b|\bGary\b', '', 'developer names from code comments'),
    (r'betatesters?', 'i', 'beta-tester lists'),
    # IP fiction strings and names
    (r'survival of the flattest', 'i', 'product subtitle'),
    (r'\bRUN PROGRAM\b|\bSYNTAX ERROR\b|\bPROGRAM TERMINATED\b|\bOBJECTIVES COMPLETED\b|\bACQUIRED\b|\bUPGRADED\b', '', 'task-manager fiction strings'),
    (r'powerup limit reached|two powerups max', 'i', 'power-up limit messages'),
    (r'\bCrate Drop!', '', 'crate announcement text'),
    (r'Gq/s', '', 'output unit of a campaign building'),
    (r'hail to the king', 'i', 'achievement name'),
    (r'unclad skies', 'i', 'achievement name'),
    (r'aggravated assault', 'i', 'achievement name'),
    (r'\bBlitz ?Master\b', 'i', 'achievement name'),
    (r'\bWrong Game\b', '', 'achievement name and message'),
    (r'\bDominator\b|\bHoarder\b|\bGenocide\b|\bCarnage\b|\bMaster Player\b', '', 'achievement names (capitalised)'),
    # mode names and abbreviations
    (r'king of the hill|KingOfTheHill', 'i', 'mode name'),
    (r'capture ?the ?statue', 'i', 'mode name'),
    (r'rocket ?riot', 'i', 'mode name'),
    (r'tank ?battle', 'i', 'debug mode name'),
    (r'b(?:litz|litk|itz)kr(?:ie|ei)g', 'i', 'mode name and its misspelt code forms (Blitzkreig, Blitkrieg, Bitzkreig)'),
    (r'\bKOTH\b|\bCTS\b|\bASLT\b|\bBLITZ\b', '', 'mode abbreviations'),
    (r'\bAssault ?Mode\b|\bAssaultMode\b', '', 'mode key token'),
    # outcome, unit and building coinages
    (r'hot ?feet', 'i', 'crate outcome name'),
    (r'mega ?rage', 'i', 'crate outcome name'),
    (r'spawn ?mania', 'i', 'crate outcome name'),
    (r'crate ?mania', 'i', 'crate outcome name'),
    (r'magical ?forest', 'i', 'crate outcome name'),
    (r'\bDark ?Forest\b', '', 'crate outcome name'),
    (r'napalm ?strike', 'i', 'crate outcome name'),
    (r'personal ?shield', 'i', 'crate outcome name'),
    (r'nuke ?barrage', 'i', 'crate outcome name'),
    (r'\bAnts? ?Nest\b|\bAntNest\b', '', 'crate outcome name'),
    (r'insertion ?squad', 'i', 'unit name'),
    (r'\bLaserTroop', '', 'unit name'),
    (r'trunk ?port', 'i', 'building name'),
    (r'pulse ?bomb', 'i', 'building name'),
    (r'\bWMD\b', '', 'building nickname'),
    (r'control ?station', 'i', 'building name'),
    (r'escape ?rocket', 'i', 'building name'),
    (r'wall ?buster', 'i', 'building name'),
    (r'battle ?cannon', 'i', 'turret barrel name'),
    (r'pattern ?buffer', 'i', 'campaign building and location'),
    (r'\bTaskManager\w*|taskmanager_', '', 'power container identifiers'),
    (r'CrateRew(?:ar|ra)d\w*', '', 'crate outcome identifiers and sample groups (incl. misspelt CrateRewrad)'),
    # map and level identity
    (r'\bmp_[a-z0-9]+\w*', 'i', 'level file names mp_*'),
    (r'\bMAPID_\w*', '', 'map id macros'),
    (r'\blevel_enum\b', 'i', 'level enumeration file'),
    (r'\bPERMITTED_MAPS\b', '', 'map gate macro'),
    # sound, music and voice names
    (r'\btresk\b|tresk_', 'i', 'licensed music'),
    (r'trash80', 'i', 'licensed music'),
    (r'\bdma_sc', 'i', 'licensed music'),
    (r'twoataris|LoaderAmiga|speccyloader', 'i', 'retro loader tunes'),
    (r'gdspeech', 'i', 'voice set'),
    (r'\bYesSir\w*|\bDSelect\w*|\bDStrain\w*', '', 'voice sets'),
    (r'multilobbyamb', 'i', 'lobby ambience set'),
    (r'shipheartbeat|alien_lands|worldview(?:dry|wet)|\bworldamb\b', 'i', 'signature ambiences'),
    (r'redvirus_|\bkoh_zone', 'i', 'sample groups'),
    (r'\b(?:Player|Enemy)_Capture_Statue\b', '', 'voice events'),
    # tools and third-party code
    (r'\banimator\.(?:exe|dat)\b', 'i', 'panel animation tool'),
    (r'autopatch', 'i', 'updater binary'),
    (r'\bShapeExporter\b', '', 'mesh exporter tool'),
    (r'xcrashreport', 'i', 'crash-report sample code'),
    (r'\bncftp\w*|\bpscp\b|\bUPX\b|Loki Setup|aoTuV|RARLAB', 'i', 'third-party tools'),
    (r'Bruce Dawson|Hans Dietrich|Jonathan de Halleux', 'i', 'third-party code authors'),
    (r'\bGetWinVer\b|\bMiniVersion\b', '', 'third-party code'),
    # terrain ramps, water and wave tables (exact names)
    (r'\blandscape_(?:beach|containment|dark|default|desert|earth|field|firetop|icecaps(?:_peaked)?|jailbreak|launchpad|marble2?|mine2?|ridges|shrine|silver|stronghold|trippy|volcanic(?:plain)?)\b', 'i', 'terrain colour ramp names'),
    (r'\bwaves_(?:containmentfield|default|desert|dull|earth|generator|icecaps|launchpad|yellow)\b', 'i', 'wave table names'),
    (r'\bwater_(?:default|icecaps|launchpad|yellow)\b', 'i', 'water texture names'),
    (r'\bicecaps\b', 'i', 'terrain theme word'),
    # string key families (IP28)
    (r'\b(?:cratename|crate_message|crate_tooltip|crate_hint|buildingname|entityname|researchname|research_explanation|spawnname|engineer|bootloader|launchpad|pixelmines|yardend|authentication|control_help|outro|temple|containment|prologue_objective|kills_milestone|gc)_\w+', 'i', 'original string key families'),
    (r'\bintro_\d+\b', 'i', 'intro caption keys'),
    (r'\besrb_warning\b', 'i', 'rating notice key'),
    (r'\bPLAYERJOIN\b|\bPLAYERLEAVE\b', '', 'chat tokens'),
]

PROVENANCE = [
    (r'\b(code|data|contrib|targets|docs)/[\w./-]+', '', 'source path (prompt §7.1)'),
    (r'\btools/(?!design-check)[\w./-]+', '', 'source path outside the retained tooling (prompt §7.1)'),
    (r':L\d+\b', '', 'line reference'),
    (r'\[(C|I|U)\]', '', 'evidence tag'),
    (r'\[B:[A-Z]+\]', '', 'build tag'),
    (r'\bB:(?:FULL|LEGACY|DEAD|D3D|DEMO|DEBUG|EDITOR)\b', '', 'build tag without brackets'),
    (r'Verbatim excerpt', '', 'closing-section and excerpt names'),
    (r'Source map', '', 'closing-section name'),
    (r'Docs-vs-code', '', 'closing-section name'),
    (r'_work/', '', 'recovery work folder'),
    (r'[Cc]overage matrix', '', 'recovery coverage matrix'),
    (r'\.(bmp|shp|mwm|dat|vcproj)\b', 'i', 'original file extension used as a file reference'),
    (r'\bf909630\b', '', 'recovered commit id'),
    (r'\b\d\d-U\d+\b', '', 'recovery unknown ids (NN-U<n>)'),
    (r'\b[PSX]\d+[ab]?_\w+\.(?:csv|md|tsv|py|json)\b', '', 'recovery draft and script files'),
    (r'\bINTERNAL_STUDY\b', '', 'recovery licence posture'),
    (r'\b(?:00_overview|01_game_modes|02_units_and_entities|03_buildings_and_world_objects|04_crates_powerups_and_weapons|05_economy_and_population|06_player_control_and_input|07_ai|08_levels_and_level_design|09_meta_systems|20_architecture|21_simulation_and_entity_model|22_determinism_and_networking|23_terrain_and_landscape|24_rendering|25_textures_and_assets|26_animation_and_vfx|27_audio_engine|28_ui_framework|29_data_formats_reference|30_scripting_tutorial_and_cutscenes|31_platform_build_and_tools|32_debug_test_and_performance|40_carry_forward_assessment|41_risks_and_open_questions|90_glossary|91_cross_reference_index|99_coverage_matrix)\.md\b', '', 'link to an original corpus document'),
]

LEGACY_TECH = [
    (r'OpenGL', '', 'legacy graphics API'),
    (r'\bopengl\w*', 'i', 'legacy graphics API identifiers (opengl_directx)'),
    (r'\bGL_\w+', '', 'legacy graphics API constants'),
    (r'\bgl(?!TF)[A-Z]\w+', '', 'legacy graphics API calls; glTF is allowed'),
    (r'\bglu[A-Z]\w*', '', 'legacy graphics utility calls'),
    (r'\bwgl[A-Z]\w+', '', 'legacy window-system calls'),
    (r'\bWGL\b', '', 'legacy window-system binding'),
    (r'\bARB_\w+', '', 'legacy graphics extensions'),
    (r'Direct3D ?9', '', 'legacy graphics API version'),
    (r'\bD3D9\b', '', 'legacy graphics API version'),
    (r'\bD3DX(?!12)', '', 'legacy helper library; d3dx12.h is allowed'),
    (r'DirectX ?9', '', 'legacy runtime version'),
    (r'DirectX SDK', '', 'legacy SDK'),
    (r'DirectSound', '', 'legacy audio API'),
    (r'\bDSound\w*', '', 'legacy audio API effect names'),
    (r'waveOut', '', 'legacy audio API'),
    (r'XInput ?1\.3', '', 'legacy input runtime'),
    (r'xinput1_3', 'i', 'legacy input runtime'),
    (r'\bx87\b', '', 'legacy floating-point unit'),
    (r'_control87', '', 'legacy floating-point control'),
    (r'Visual Studio 200[58]', '', 'legacy toolchain'),
    (r'\bVS ?200[58]\b', 'i', 'legacy toolchain'),
    (r'MSVC ?200[58]', '', 'legacy toolchain'),
    (r'\bvsprops\b', 'i', 'legacy toolchain property sheets'),
    (r'\bMBCS\b', '', 'legacy character set'),
    (r'\bRAR\b', '', 'legacy archive format'),
    (r'unrar', 'i', 'legacy archive library'),
    (r'Inno Setup', '', 'legacy installer'),
    (r'\bNSIS\b', '', 'legacy installer'),
    (r'GnuWin32', '', 'legacy tool set'),
    (r'OpenMP', '', 'legacy threading'),
    (r'display lists?', 'i', 'legacy graphics API feature'),
    (r'lost device', 'i', 'legacy graphics API failure mode'),
    (r'metaserver', 'i', 'legacy online service'),
    (r'authkey', 'i', 'legacy licence keys'),
    (r'\bSDL\b', '', 'legacy platform library'),
    (r'\bFixedFunc\d\w*|\bFixedPipeline\b|fixedpipeline', '', 'legacy fixed-function emulation shaders'),
]

# ---------------------------------------------------------------------------
# 2 Soft-flag patterns (prompt §7.2) and renamed-term drift
# ---------------------------------------------------------------------------
SOFT = [
    (r'\boriginal(ly)?\b', 'i', 'history', 'prompt §7.2'),
    (r'\bshipped\b', 'i', 'history', 'prompt §7.2'),
    (r'\brecover(ed|y)\b', 'i', 'history', 'prompt §7.2'),
    (r'\blegacy\b', 'i', 'history', 'prompt §7.2'),
    (r'\bcarry[- ]forward\b', 'i', 'history', 'prompt §7.2'),
    (r'\bport(ed|ing)\b', 'i', 'history', 'prompt §7.2'),
    (r'\bport of\b', 'i', 'history', 'prompt §7.2; the gameplay word port (a manning slot) is not matched'),
    (r'\bused to\b', 'i', 'history', 'prompt §7.2'),
    (r'\bretail\b', 'i', 'history', 'prompt §7.2'),
    (r'\bsource (tree|code)\b', 'i', 'history', 'prompt §7.2'),
    (r'\bcommit\b', 'i', 'history', 'prompt §7.2'),
    (r'\bdemo\b', 'i', 'history', 'prompt §7.2'),
    (r'\blockstep\b', 'i', 'scoped', 'prompt §7.2: allowed only in reference/12 and design-phase DEC entries'),
    (r'immediate[- ]mode', 'i', 'scoped', 'prompt §7.2: allowed for developer tooling UI only'),
    (r'fixed[- ]function', 'i', 'scoped', 'prompt §7.2: allowed for the output-merger and rasteriser stages only'),
    (r'\bLinux\b', '', 'scoped', 'prompt §7.2: allowed only for server or CI hosting'),
    (r'\bmacOS\b', '', 'scoped', 'prompt §7.2: allowed only for server or CI hosting'),
    (r'\bMac OS\b', '', 'scoped', 'prompt §7.2: allowed only for server or CI hosting'),
    (r'\bTODO\b', '', 'unfinished', 'prompt §7.2'),
    (r'\bTBD\b', '', 'unfinished', 'prompt §7.2'),
    (r'\?\?\?', '', 'unfinished', 'prompt §7.2'),
    (r'@strings:original_strings.txt', 'i', 'orig-string', 'quoted string of three or more words equal to an original English string (normalised)'),
]
# Original names that are ordinary English words: fix to the ledger term, or justify.
RENAMED = [
    (r'\bDomination\b', 'mode name; use Spawn Control'),
    (r'\bAssault\b', 'mode name; use Siege'),
    (r'\bDOM\b|\bRR\b|\bTANK\b', 'mode abbreviations; write the Species mode name'),
    (r'\bRetribution\b', 'use Comeback Bonus'),
    (r'\bHandicap\b', 'use Catch-up bonus'),
    (r'\bOfficers?\b', 'use Leader'),
    (r'\bEngineers?\b', 'use Builder'),
    (r'\bHarvesters?\b', 'use Collector'),
    (r'\bArmour\b', 'use Transport'),
    (r'\bSquads?\b', 'use Strike team'),
    (r'\bStatues?\b', 'use Relic'),
    (r'\bSpirits?\b|\bSouls?\b', 'use Essence orb'),
    (r'\bMonster team\b', 'use Rogue faction'),
    (r'\bPlague\b', 'use Sickness'),
    (r'\bInfection\b', 'use Swarm'),
    (r'\bRage\b', 'use Fury'),
    (r'\bRandomi[sz]er\b', 'use Allegiance Scrambler'),
    (r'\bSlow Down\b|\bSpeed Up\b', 'use Time Dilation / Time Rush'),
    (r'\bnapalm\b', 'use firebomb'),
    (r'\bZombies?\b', 'use Wisp'),
    (r'\bPortal\b', 'use Rift'),
    (r'\bTentacles?\b|\bShaman\b|\bTripod\b|\bLander\b', 'dropped creatures'),
    (r'\bSpace Ship\b', 'use Abductor'),
    (r'\bSpawn Points?\b|\bSpawn Links?\b|\bPopulation Lock\b', 'use Spawn Node / Relay / Population Limiter'),
    (r'\bSolar Panels?\b|\bRadar Dish(?:es)?\b|\bLaser Fences?\b', 'use Solar Array / Beam Dish / Barrier Emitter'),
    (r'\bFuel Station\b|\bFuel Generator\b|\bRestriction Zone\b', 'use fuel depot / fuel pump / exclusion zone'),
    (r'\bTask Manager\b', 'use Power slots (allowed only for the Windows tool)'),
    (r'\bIncubator\b', 'dropped building'),
    (r'\bExplorer\b', 'achievement name; use Tourist (allowed for Windows File Explorer)'),
    (r'\bUplink Console\b|\bWarp Gate\b|\bRelay Dish\b|\bEnergy Collector\b|\bSpirit Grove\b|\bArmour Duel\b|\bLightning War\b', '90 placeholders replaced by the collision screen'),
]

STD_ALLOW = set('''QueryPerformanceCounter CreateThread Direct3D DirectX XInput Win32 HLSL MAX_PATH NDEBUG
WIN32_LEAN_AND_MEAN std::map std::mt19937 std::vector NET_GAME_ GAME_TITLE'''.split())
STOP_LOWER = set('stricmp qsort sqrt untitled unicode climbing erase'.split())
STOP_TOKENS = set('random_number EULA.txt Recalculat'.split())
ARTEFACT = re.compile(r'^(?:_[a-z_]+|[PSX]\d+[ab]?_\w+|build_matrix|phase2_log|manifest|class_assignment|S_\w+|_workscripts\w*|code\w+)$')
VERBS = set('''Get Set Is Has Add Remove Put Create Delete Find Update Init Initialise Render Load Save Read Write Begin End
Start Stop Reset Clear Insert Erase Mark Count Change Calculate Advance Do Make New Check Process Handle Draw Free'''.split())
RENAMED_STEMS = ['multiwin', 'darwinia', 'darwinian', 'darwinn', 'sepulveda', 'futurewinian', 'evilinian', 'rocketwinian', 'virii',
                 'triffid', 'squadie', 'squaddie', 'souldestroyer', 'sporegenerator', 'spaceinvader', 'trunkport', 'pulsebomb',
                 'wallbuster', 'insertionsquad', 'spirit', 'officer', 'monster', 'bitzkreig', 'blitzkrieg', 'blitkrieg', 'spawnmania',
                 'spawnpoint', 'laserfence', 'radardish', 'spaceship', 'armour', 'harvester', 'engineer', 'retribution', 'handicap',
                 'statue', 'koth', 'christmas', 'taskmanager', 'cratereward', 'eclipse', 'systemiv', 'introversion']
SMALL = {'the', 'of', 'to', 'a', 'an', 'on', 'in', 'upon', 'and'}
GENERIC_MAP_NAMES = {'Gridlock', 'Melting Pot', 'Scorched Earth', 'The Last Stand', 'The Hard Way', 'Leap of Faith', 'Beyond the Pale',
                     'The Wheel', 'Stress Test', 'Price of War', 'Heart of Chaos', 'Bridge of Light', 'Pillar of Light',
                     'Confrontation Point', 'Pursuit of Victory', 'The Bitter Front', 'The Second Beginning', 'Echoes of the Future',
                     'The Walled Garden', 'Barriers to Progress'}


def norm_string(s):
    """Normalise an English string for the original-string match."""
    s = s.replace('\\n', ' ')
    s = unicodedata.normalize('NFKD', s).encode('ascii', 'ignore').decode('ascii')
    s = s.lower()
    s = re.sub(r'%[-+ #0-9.]*[a-z]', ' ', s)  # printf placeholders
    s = re.sub(r'\*[a-z]\b', ' ', s)  # *T, *P team and player placeholders
    s = re.sub(r"[^a-z0-9']+", ' ', s)
    s = re.sub(r"(?<![a-z])'|'(?![a-z])", ' ', s)
    return re.sub(r'\s+', ' ', s).strip()


def read_tsv(path):
    with open(path, encoding='utf-8', newline='') as fh:
        return list(csv.DictReader(fh, delimiter='\t'))


def read_csv(path):
    with open(path, encoding='utf-8', newline='') as fh:
        return list(csv.DictReader(fh))


def main():
    orig, f1, work = sys.argv[1:4]
    drafts = os.path.join(orig, '_work', 'drafts')
    vocab = {r['word']: int(r['lower_count']) for r in read_tsv(os.path.join(f1, 'prose_vocab.tsv'))}

    def english(w):
        return vocab.get(w.lower(), 0) >= 3

    ledger = read_tsv(os.path.join(work, 'rename_map.tsv'))
    species_ids = set()
    for r in ledger:
        for tok in re.findall(r'[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*', re.sub(r'<[^>]*>', ' ', r['species_identifier'])):
            species_ids.add(tok)
            species_ids.update(tok.split('::'))
    species_ids |= {'ControlZone', 'OrderMarker', 'default_view'}

    out = []  # (pattern, flags, class, note)

    def push(p, flags, cls, note):
        out.append((p, flags, cls, note))

    for p, f, n in BRAND:
        push(p, f, 'brand', n)
    for p, f, n in PROVENANCE:
        push(p, f, 'provenance', n)
    for p, f, n in LEGACY_TECH:
        push(p, f, 'legacy-tech', n)

    # map display names (ORIG/41 §5.2, ORIG/08 §3 via X4_levels.csv) and level file stems
    names = {}
    with open(os.path.join(orig, '41_risks_and_open_questions.md'), encoding='utf-8') as fh:
        in52 = False
        for line in fh:
            if line.startswith('### 5.2 '):
                in52 = True
                continue
            if in52 and line.startswith('### '):
                break
            if in52 and line.startswith('| `data/levels/'):
                c = [x.strip() for x in line.strip().strip('|').split(' | ')]
                if c[1] and c[1] != '—':
                    names.setdefault(c[1], set()).add('41 §5.2')
    stems = set()
    for r in read_csv(os.path.join(drafts, 'X4_levels.csv')):
        if r['english_name']:
            names.setdefault(r['english_name'], set()).add('08 §3 (X4_levels)')
        stems.add(os.path.splitext(os.path.basename(r['file']))[0])
    with open(os.path.join(f1, 'map_names.tsv'), 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['display_name', 'sources', 'generic_phrase'])
        for n in sorted(names):
            wr.writerow([n, '; '.join(sorted(names[n])), 'yes' if n in GENERIC_MAP_NAMES else 'no'])
    for n in sorted(names):
        words = n.split()
        if len(words) >= 3 and words[0] in ('The', 'A'):
            lead, words = r'(?:%s\s+)?' % words[0], words[1:]
        else:
            lead = ''
        parts = []
        for w in words:
            parts.append('(?i:%s)' % re.escape(w) if w.lower() in SMALL else re.escape(w))
        push(r'\b' + lead + r'\s+'.join(parts) + r'\b', '', 'map-name', 'map display name "%s" (case-sensitive except small words)' % n)
    campaign = sorted(s for s in stems if not s.lower().startswith('mp_'))
    push(r'\b(?:' + '|'.join(re.escape(s) for s in campaign) + r')\b', 'i', 'map-name', 'campaign and superseded level files (map_*, mission_*, old/*)')

    # string keys from the three string CSVs
    keys = set()
    strings = []
    for r in read_csv(os.path.join(drafts, 'P17b_strings.csv')):
        keys.add(r['id'])
        strings.append((r['english'], 'P17b'))
    for r in read_csv(os.path.join(drafts, 'P02_game_modes_strings.csv')):
        keys.add(r['key'])
        strings.append((r['english'], 'P02'))
    for r in read_csv(os.path.join(drafts, 'P17a_ui_framework_editor_ui_strings.csv')):
        if r['kind'] == 'key':
            keys.add(r['key_or_text'])
            strings.append((r['english_text'], 'P17a'))
        else:
            strings.append((r['key_or_text'], 'P17a'))
    fam_re = re.compile(r'^(?:multiwin|darwinia|cratename|crate_message|crate_tooltip|crate_hint|buildingname|entityname|researchname|'
                        r'research_explanation|spawnname|engineer|bootloader|launchpad|pixelmines|yardend|authentication|control_help|'
                        r'outro|temple|containment|prologue_objective|kills_milestone|gc|taskmanager)', re.I)
    exact_keys = sorted(k for k in keys if k and '_' in k and not fam_re.match(k) and k not in species_ids)
    exact_keys += sorted(k for k in keys if k and '_' not in k and re.fullmatch(r'[a-z0-9]+', k) and len(k) >= 8 and not english(k))
    with open(os.path.join(f1, 'string_keys.tsv'), 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['key', 'covered_by'])
        for k in sorted(keys):
            wr.writerow([k, 'family pattern' if fam_re.match(k) else ('exact pattern' if k in exact_keys else 'not distinctive')])
    # one alternation per first letter keeps the file readable
    by_letter = {}
    for k in exact_keys:
        by_letter.setdefault(k[0].lower(), []).append(k)
    for letter in sorted(by_letter):
        ks = by_letter[letter]
        push(r'\b(?:' + '|'.join(re.escape(k) for k in sorted(ks, key=lambda x: (-len(x), x))) + r')\b', 'i', 'string-key',
             'original string keys starting with "%s" (%d keys, exact)' % (letter, len(ks)))

    # identifiers
    classes = set()
    with open(os.path.join(orig, '91_cross_reference_index.md'), encoding='utf-8') as fh:
        in4 = False
        for line in fh:
            if line.startswith('## 4 '):
                in4 = True
                continue
            if in4 and line.startswith('## '):
                break
            m = re.match(r'\| `([A-Za-z0-9_]+)` \|', line)
            if in4 and m:
                classes.add(m.group(1))
    idents = read_tsv(os.path.join(f1, 'identifiers.tsv'))
    scopes = {r['identifier'].split('::')[0] for r in idents if '::' in r['identifier']}
    classes |= {s for s in scopes if re.fullmatch(r'[A-Z][A-Za-z0-9]*', s)}
    for r in read_tsv(os.path.join(f1, 'glossary_rows.tsv')):
        pass  # glossary §2 IP rows are covered by the brand and ledger patterns
    file_stems = {os.path.splitext(r['identifier'])[0].lower() for r in idents if r['shape'] == 'file'}
    covered_ext = ('bmp', 'shp', 'mwm', 'dat', 'vcproj')
    brand_res = [re.compile(p, re.I if 'i' in f else 0) for p, f, _ in BRAND + LEGACY_TECH]

    def covered(tok):
        return any(rx.search(tok) for rx in brand_res)

    def parts_of(tok):
        return [p for p in re.split(r'_|(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])', tok) if p]

    def has_stem(tok):
        t = tok.lower()
        return any(s in t for s in RENAMED_STEMS)

    stats = {}
    seen = set()
    for r in idents:
        tok, shape = r['identifier'], r['shape']
        if tok in STD_ALLOW or tok in STOP_TOKENS or tok in species_ids or ARTEFACT.match(tok) or tok in seen:
            continue
        if covered(tok):
            stats['covered by brand or legacy pattern'] = stats.get('covered by brand or legacy pattern', 0) + 1
            continue
        pat = None
        if shape == 'qualified':
            scope = tok.split('::')[0]
            if scope in ('std',):
                continue
            pat = r'\b' + re.escape(tok) + r'\b'
        elif shape == 'compound':
            ps = parts_of(tok)
            if tok[0].islower():
                if has_stem(tok):
                    pat = r'\b' + re.escape(tok) + r'\b'
            elif tok not in classes and len(ps) <= 3 and ps[0] in VERBS and all(english(p) for p in ps[1:]):
                stats['generic API verb names exempt'] = stats.get('generic API verb names exempt', 0) + 1
                continue
            else:
                pat = r'\b' + re.escape(tok) + r'\b'
        elif shape == 'member':
            if has_stem(tok):
                pat = r'\b' + re.escape(tok) + r'\b'
        elif shape == 'macro':
            pat = r'\b' + re.escape(tok) + (r'\w*' if tok.endswith('_') else r'\b')
        elif shape == 'snake':
            low = tok.lower()
            if tok.endswith('_'):
                if has_stem(tok) or tok.isupper() or low in ('iv_', 'mission_', 'kills_milestone_', 'dialog_position_', 'dialog_tied_'):
                    pat = r'\b' + re.escape(tok) + r'\w*'
            elif has_stem(tok) or low in file_stems or re.search(r'\d', tok) or low.startswith('iv_'):
                pat = r'\b' + re.escape(tok) + r'\b'
        elif shape == 'single':
            # single capitalised words: code form only (a capitalised English word may start a sentence),
            # bare only when the token contains a digit (Vector3, Matrix34)
            if re.search(r'\d', tok):
                pat = r'\b' + re.escape(tok) + r'\b'
            elif tok in classes or tok in scopes:
                pat = r'`' + re.escape(tok) + r'(?:\(\))?`|\b' + re.escape(tok) + r'::(?=[~A-Za-z_])'
        elif shape == 'lower':
            if len(tok) >= 5 and not english(tok) and vocab.get(tok, 0) < 3 and not re.fullmatch(r'u?int\d+|[a-z]{1,3}\d*', tok) \
                    and tok not in STOP_LOWER:
                pat = r'\b' + re.escape(tok) + r'\b'
        elif shape == 'file':
            stem, ext = os.path.splitext(tok)
            ext = ext[1:].lower()
            if ext in covered_ext or ext in ('csv', 'md', 'py', 'tsv', 'json'):
                continue
            sparts = parts_of(stem)
            if len(sparts) == 1 and english(stem) and not has_stem(stem):
                continue
            pat = r'\b' + re.escape(tok) + r'\b'
        if pat is None:
            continue
        seen.add(tok)
        flags = 'i' if shape in ('lower', 'file', 'snake') and not tok.isupper() else ''
        cls = 'identifier'
        stats[shape] = stats.get(shape, 0) + 1
        push(pat, flags, cls, '%s (%s; ORIG %s)' % (tok, shape, r['sources']))

    # validate and de-duplicate
    final, dup = [], set()
    for p, f, c, n in out:
        key = (p, f)
        if key in dup:
            continue
        dup.add(key)
        re.compile(p, re.I if 'i' in f else 0)
        final.append((p, f, c, n))
    with open(os.path.join(work, 'forbidden_patterns.tsv'), 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['pattern', 'flags', 'class', 'note'])
        wr.writerows(final)

    soft = [(p, f, c, n) for p, f, c, n in SOFT] + [(p, '', 'renamed-term', n) for p, n in RENAMED]
    for p, f, c, n in soft:
        if not p.startswith('@'):
            re.compile(p, re.I if 'i' in f else 0)
    with open(os.path.join(work, 'soft_patterns.tsv'), 'w', encoding='utf-8', newline='') as fh:
        wr = csv.writer(fh, delimiter='\t', lineterminator='\n')
        wr.writerow(['pattern', 'flags', 'class', 'note'])
        wr.writerows(soft)

    # original strings: the three CSVs plus every string quoted in ORIG/04, ORIG/06 and ORIG/09
    quoted = re.compile(r'"([^"\n]{3,300})"|“([^”\n]{3,300})”')
    for doc in ('04_crates_powerups_and_weapons.md', '06_player_control_and_input.md', '09_meta_systems.md'):
        with open(os.path.join(orig, doc), encoding='utf-8') as fh:
            for line in fh:
                for m in quoted.finditer(line):
                    strings.append((m.group(1) or m.group(2), doc[:2]))
    normed = {}
    for s, src in strings:
        n = norm_string(s or '')
        if len(n.split()) >= 3:
            normed.setdefault(n, set()).add(src)
    with open(os.path.join(work, 'original_strings.txt'), 'w', encoding='utf-8', newline='') as fh:
        for n in sorted(normed):
            fh.write(n + '\n')

    with open(os.path.join(f1, 'pattern_build_report.txt'), 'w', encoding='utf-8') as fh:
        cls_count = {}
        for _, _, c, _ in final:
            cls_count[c] = cls_count.get(c, 0) + 1
        fh.write('forbidden patterns: %d %s\n' % (len(final), cls_count))
        fh.write('soft patterns: %d\n' % len(soft))
        fh.write('identifier patterns by shape: %s\n' % stats)
        fh.write('map display names: %d (generic phrases %d); campaign level files: %d\n' % (len(names), len(GENERIC_MAP_NAMES & set(names)), len(campaign)))
        fh.write('string keys: %d (exact patterns %d)\n' % (len(keys), len(exact_keys)))
        fh.write('original strings (normalised, 3+ words): %d\n' % len(normed))
    print(open(os.path.join(f1, 'pattern_build_report.txt'), encoding='utf-8').read())


if __name__ == '__main__':
    main()
