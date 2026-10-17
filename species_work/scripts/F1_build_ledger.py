#!/usr/bin/env python3
"""F1: build WORK/rename_map.tsv (the rename ledger).

Usage: python3 -I F1_build_ledger.py <glossary_rows.tsv> <identifiers.tsv> <out rename_map.tsv> <out check report .txt>

The curated Species vocabulary is the data below. Every ORIG/90 §3 row and every §2 row
whose IP column is "yes → …" is matched (exact text first, then a unique prefix) and the
script aborts when a row is missing. It also checks that no Species identifier equals an
original identifier (other than the 90 §3 placeholders that ORIG/21 already proposed).
"""
import csv
import re
import sys

KINDS = {'name', 'text', 'identifier', 'art', 'audio', 'map', 'brand'}
RULES = {'rename', 'generic-keep', 'drop'}
PENDING = 'pending owner batch 1, see name_collisions.md'

# ---------------------------------------------------------------------------
# Shared concept values: (species_term, species_identifier, string_id_prefix, rule, note)
# ---------------------------------------------------------------------------
C = {
    'worker': ('Worker', 'Worker', 'unit.worker', 'rename', ''),
    'leader': ('Leader', 'Leader', 'unit.leader', 'rename', ''),
    'beacon': ('Waypoint beacon', 'WaypointBeacon', 'unit.leader.waypoint_beacon', 'rename', 'a Leader set as a redirect sign for idle Workers'),
    'strike': ('Strike team', 'StrikeTeam', 'unit.strike_team', 'rename', 'the whole squad'),
    'trooper': ('Trooper', 'Trooper', 'unit.trooper', 'rename', 'one member of a Strike team'),
    'transport': ('Transport', 'Transport', 'unit.transport', 'rename', ''),
    'collector': ('Collector', 'Collector', 'unit.collector', 'rename', ''),
    'builder': ('Builder', 'Builder', 'unit.builder', 'rename', ''),
    'orb': ('Essence orb', 'EssenceOrb', 'unit.essence_orb', 'rename', ''),
    'rogue': ('Rogue faction', 'RogueFaction', 'faction.rogue', 'rename', ''),
    'visitor': ('Visitor faction', 'VisitorFaction', 'faction.visitor', 'rename', ''),
    'bot': ('Bot', 'Bot', 'bot', 'rename', 'a computer-controlled player'),
    'crawler': ('Crawler', 'Crawler', 'creature.crawler', 'rename', ''),
    'reaper': ('Reaper Serpent', 'ReaperSerpent', 'creature.reaper_serpent', 'rename', ''),
    'drifter': ('Brood Drifter', 'BroodDrifter', 'creature.brood_drifter', 'rename', ''),
    'spitter': ('Spitter Plant', 'SpitterPlant', 'building.spitter_plant', 'rename', 'crate-spawned world object'),
    'seedpod': ('Seed Pod', 'SeedPod', 'creature.seed_pod', 'rename', ''),
    'bomber': ('Bomber', 'Bomber', 'unit.bomber', 'rename', 'one aircraft of an airstrike or Firebomb Run'),
    'abductor': ('Abductor', 'Abductor', 'creature.abductor', 'rename', 'needs a new presentation'),
    'wisp': ('Wisp', 'Wisp', 'creature.wisp', 'rename', 'presentation-only ghost effect'),
    'rift_horror': ('Rift Horror', 'RiftHorror', 'creature.rift_horror', 'drop', 'debug-only content; its portal is the Rift'),
    'summoner': ('Summoner', '', '', 'drop', 'cut unit'),
    'triwalker': ('Tri-walker', '', '', 'drop', 'unreachable unit'),
    'spawn_node': ('Spawn Node', 'SpawnNode', 'building.spawn_node', 'rename', ''),
    'source_node': ('Source Node', 'SourceNode', 'building.source_node', 'rename', ''),
    'relay': ('Relay', 'Relay', 'building.relay', 'rename', 'plural Relays'),
    'limiter': ('Population Limiter', 'PopulationLimiter', 'building.population_limiter', 'rename', ''),
    'gate': ('Transit Gate', 'TransitGate', 'building.transit_gate', 'rename', 'replaces 90 name Warp Gate; ' + PENDING),
    'core': ('Core Device', 'CoreDevice', 'building.core_device', 'rename', ''),
    'console': ('Command Console', 'CommandConsole', 'building.command_console', 'rename', 'replaces 90 name Uplink Console; ' + PENDING),
    'vessel': ('Launch Vessel', 'LaunchVessel', 'building.launch_vessel', 'rename', ''),
    'depot': ('fuel depot', 'FuelDepot', 'building.fuel_depot', 'rename', 'generic term'),
    'solar': ('Solar Array', 'SolarArray', 'building.solar_array', 'rename', 'replaces 90 name Energy Collector; ' + PENDING),
    'dish': ('Beam Dish', 'BeamDish', 'building.beam_dish', 'rename', 'replaces 90 name Relay Dish; ' + PENDING),
    'barrier': ('Barrier Emitter', 'BarrierEmitter', 'building.barrier_emitter', 'rename', ''),
    'breach': ('Breach Charge', 'BreachCharge', 'building.breach_charge', 'rename', ''),
    'relic': ('Relic', 'Relic', 'building.relic', 'rename', ''),
    'relic_base': ('Relic Base', 'RelicBase', 'building.relic_base', 'rename', 'the delivery zone'),
    'relic_spawn': ('Relic Spawn Zone', 'RelicSpawnZone', 'building.relic_spawn_zone', 'rename', 'where a Relic appears; ' + PENDING),
    'zone': ('Control Zone', 'ControlZone', 'building.control_zone', 'rename', ''),
    'flag_post': ('Flag post', 'FlagPost', 'building.flag_post', 'rename', ''),
    'home_post': ('Home post', 'HomePost', 'building.home_post', 'rename', ''),
    'hive': ('Hive', 'Hive', 'building.hive', 'rename', ''),
    'hatchery': ('Hatchery', '', '', 'drop', 'placed only by campaign and developer maps'),
    'spawn_control': ('Spawn Control', 'SpawnControl', 'mode.spawn_control', 'rename', ''),
    'zone_control': ('Zone Control', 'ZoneControl', 'mode.zone_control', 'rename', ''),
    'relic_run': ('Relic Run', 'RelicRun', 'mode.relic_run', 'rename', ''),
    'siege': ('Siege', 'Siege', 'mode.siege', 'rename', ''),
    'launch_race': ('Launch Race', 'LaunchRace', 'mode.launch_race', 'rename', ''),
    'flag_chain': ('Flag Chain', 'FlagChain', 'mode.flag_chain', 'rename', ''),
    'vehicle_duel': ('Vehicle Duel', 'VehicleDuel', 'mode.vehicle_duel', 'drop', 'debug-only mode; replaces 90 name Armour Duel'),
    'crate': ('Supply crate', 'SupplyCrate', 'crate', 'rename', ''),
    'power': ('Power', 'Power', 'power', 'rename', 'a granted ability held in a power slot'),
    'slots': ('Power slots', 'PowerSlots', 'ui.power_slots', 'rename', ''),
    'comeback': ('Comeback Bonus', 'ComebackBonus', 'match.comeback_bonus', 'rename', ''),
    'catchup': ('Catch-up bonus', 'CatchUpBonus', 'match.catch_up_bonus', 'rename', ''),
    'o_strike': ('Strike team (outcome)', 'CrateOutcome::StrikeTeam', 'crate.strike_team', 'rename', ''),
    'o_transport': ('Transport (outcome)', 'CrateOutcome::Transport', 'crate.transport', 'rename', ''),
    'o_collector': ('Collector (outcome)', 'CrateOutcome::Collector', 'crate.collector', 'rename', ''),
    'o_builder': ('Builder (outcome)', 'CrateOutcome::Builder', 'crate.builder', 'rename', ''),
    'o_airstrike': ('airstrike', 'CrateOutcome::Airstrike', 'crate.airstrike', 'generic-keep', ''),
    'o_firebomb': ('Firebomb Run', 'CrateOutcome::FirebombRun', 'crate.firebomb_run', 'rename', ''),
    'o_barrage': ('Missile Barrage', 'CrateOutcome::MissileBarrage', 'crate.missile_barrage', 'rename', 'generic term'),
    'o_meteor': ('meteor shower', 'CrateOutcome::MeteorShower', 'crate.meteor_shower', 'generic-keep', ''),
    'o_shield': ('Shield', 'CrateOutcome::Shield', 'crate.shield', 'rename', ''),
    'o_convert': ('Convert', 'CrateOutcome::Convert', 'crate.convert', 'rename', ''),
    'o_speed': ('Speed Boost', 'CrateOutcome::SpeedBoost', 'crate.speed_boost', 'rename', ''),
    'o_fury': ('Fury', 'CrateOutcome::Fury', 'crate.fury', 'rename', 'generic word'),
    'o_reinf': ('Reinforcements', 'CrateOutcome::Reinforcements', 'crate.reinforcements', 'rename', ''),
    'o_surge': ('Spawn Surge', 'CrateOutcome::SpawnSurge', 'crate.spawn_surge', 'rename', ''),
    'o_rocket': ('Rocket Troops', 'CrateOutcome::RocketTroops', 'crate.rocket_troops', 'rename', ''),
    'o_storm': ('Supply Storm', 'CrateOutcome::SupplyStorm', 'crate.supply_storm', 'rename', ''),
    'o_frenzy': ('Frenzy', 'CrateOutcome::Frenzy', 'crate.frenzy', 'rename', ''),
    'o_dilation': ('Time Dilation', 'CrateOutcome::TimeDilation', 'crate.time_dilation', 'rename', ''),
    'o_rush': ('Time Rush', 'CrateOutcome::TimeRush', 'crate.time_rush', 'rename', ''),
    'o_grove': ('Essence Grove', 'CrateOutcome::EssenceGrove', 'crate.essence_grove', 'rename', 'replaces 90 name Spirit Grove; ' + PENDING),
    'o_blight': ('Blight Grove', 'CrateOutcome::BlightGrove', 'crate.blight_grove', 'rename', ''),
    'o_hive': ('Hive (outcome)', 'CrateOutcome::Hive', 'crate.hive', 'rename', ''),
    'o_brood': ('Brood', 'CrateOutcome::Brood', 'crate.brood', 'rename', ''),
    'o_swarm': ('Swarm', 'CrateOutcome::Swarm', 'crate.swarm', 'rename', ''),
    'o_scrambler': ('Allegiance Scrambler', 'CrateOutcome::AllegianceScrambler', 'crate.allegiance_scrambler', 'rename', ''),
    'o_sickness': ('Sickness', 'CrateOutcome::Sickness', 'crate.sickness', 'rename', ''),
    'o_abductor': ('Abductor (outcome)', 'CrateOutcome::Abductor', 'crate.abductor', 'rename', ''),
    'o_spitter': ('Spitter Plants (outcome)', 'CrateOutcome::SpitterPlants', 'crate.spitter_plants', 'rename', ''),
    'o_rift': ('Rift Horror (outcome)', 'CrateOutcome::RiftHorror', 'crate.rift_horror', 'drop', 'debug-only outcome'),
    'o_turrets': ('gun turret / rocket turret / flame turret (outcomes)', 'CrateOutcome::TurretGun', 'crate.turret_gun', 'generic-keep', 'also CrateOutcome::TurretRocket (crate.turret_rocket), CrateOutcome::TurretFlame (crate.turret_flame)'),
    'rules': ('match rules', 'MatchRules', 'match', 'rename', ''),
    'options': ('match options', 'MatchOptions', 'match.options', 'rename', 'map-level option block'),
    'order_marker': ('order marker', 'OrderMarker', 'unit.leader.order_marker', 'rename', ''),
    'netgame': ('player command ids', 'NET_GAME_*', '', 'rename', 'only where a wire-level name is unavoidable; the command catalogue uses PlayerCommand enumerators'),
    'uicore': ('UI core', 'UiCore', 'ui', 'rename', ''),
    'species': ('Species', 'Species', 'game', 'rename', 'game name: Species in types and namespaces, species in files and ids'),
    'studio': ('<STUDIO>', '<STUDIO>', '', 'rename', r'profile folder <STUDIO>\Species\ '),
    'drop': ('', '', '', 'drop', ''),
    'skill': ('Skill rating', 'SkillRating', 'ui.leaderboard.skill_rating', 'rename', ''),
    'mapname': ('map_<mode>_<players>p_<nn>', 'map_<mode>_<players>p_<nn>', 'map', 'rename', '<mode> is a Species mode id: spawn_control, zone_control, relic_run, siege, launch_race, flag_chain'),
    'newmaps': ('new map names and layouts', '', 'map', 'drop', 'no reference map name or layout is carried'),
    'ramp': ('terrain colour ramp', 'ramp_<theme>', '', 'rename', 'neutral theme names'),
    'fonts': ('licensed open UI and CJK fonts in a glyph cache', '', '', 'rename', ''),
    'newart': ('new art with role names', '', '', 'rename', 'final asset ids come from assets/asset_manifest.csv'),
    'newaudio': ('new audio named by function', '', '', 'rename', 'final asset ids come from assets/asset_manifest.csv'),
    'strings': ('Species string table', '', 'ui', 'rename', 'new text under Species string ids'),
    'scripts': ('new tutorial scripts and copy', '', 'tutorial', 'rename', ''),
    'seasonal': ('seasonal theme', 'SeasonalTheme', 'theme.seasonal', 'rename', 'new art'),
    'shaders': ('HLSL Shader Model 6 shaders', '', '', 'rename', 'authored from the rendering specification'),
    'mapdef': ('map definition', 'MapDefinition', 'map', 'rename', ''),
    'mode': ('mode', 'MatchMode', 'mode', 'rename', ''),
    'achievement': ('achievement', 'Achievement', 'achievement', 'rename', ''),
    'mode_abbr': ('Siege, Spawn Control, Launch Race, Flag Chain, Vehicle Duel', '', 'mode', 'rename', 'no mode abbreviations; write the full Species mode name'),
}


def concept(key, **over):
    t = dict(zip(('species_term', 'species_identifier', 'string_id_prefix', 'rule', 'note'), C[key]))
    t.update(over)
    return t


def v(term, ident, prefix, rule, note=''):
    return {'species_term': term, 'species_identifier': ident, 'string_id_prefix': prefix, 'rule': rule, 'note': note}


# ---------------------------------------------------------------------------
# ORIG/90 §3 rows: (match, kind, value). match = exact text or unique prefix.
# ---------------------------------------------------------------------------
G3 = [
    ('Darwinian (unit', 'name', concept('worker')),
    ('Multiwinian(s)', 'name', concept('worker', species_term='Worker(s)')),
    ('"Evil Darwinians"', 'text', v('hostile workers', 'Worker', 'unit.worker.hostile', 'rename', 'Workers of the Rogue faction; new message text')),
    ('=Officer', 'name', concept('leader')),
    ('GOTO officer', 'name', concept('beacon')),
    ('Insertion squad', 'name', concept('strike')),
    ('Squadie', 'name', concept('trooper')),
    ('=Armour', 'name', concept('transport')),
    ('=Harvester', 'name', concept('collector')),
    ('=Engineer', 'name', concept('builder')),
    ('Spirit / soul', 'name', concept('orb')),
    ('Monster team', 'name', concept('rogue')),
    ('Futurewinian team', 'name', concept('visitor')),
    ('Team colour names', 'text', v('team colour names: green, red, yellow, blue, orange, purple, cyan, pink', 'TeamColour', 'team.colour', 'generic-keep', 'ordinary colour words')),
    ('CPU player', 'text', concept('bot', note='the "(CPU)" label becomes new text')),
    ('=Virii', 'name', concept('crawler')),
    ('=Soul Destroyer', 'name', concept('reaper')),
    ('=Spore Generator', 'name', concept('drifter')),
    ('Triffid (plant)', 'name', concept('spitter')),
    ('=TriffidEgg', 'identifier', concept('seedpod')),
    ('Army ant', 'name', v('ant', 'AntCreature', 'creature.ant', 'rename', 'generic word')),
    ('=Centipede', 'name', v('centipede', 'CentipedeCreature', 'creature.centipede', 'generic-keep')),
    ('=Spider', 'name', v('spider', 'SpiderCreature', 'creature.spider', 'generic-keep')),
    ('Egg (Virii egg)', 'name', v('Crawler egg', 'CrawlerEgg', 'creature.crawler_egg', 'generic-keep', 'the word egg is kept')),
    ('SpaceInvader (bomber)', 'name', concept('bomber')),
    ('Space Ship (UFO)', 'name', concept('abductor')),
    ('Zombie (ghost effect)', 'name', concept('wisp')),
    ('The Beast / Portal', 'name', concept('rift_horror')),
    ('=Shaman', 'name', concept('summoner')),
    ('=Tripod', 'name', concept('triwalker')),
    ('Spawn point (', 'name', concept('spawn_node')),
    ('Master spawn point', 'name', concept('source_node')),
    ('=Spawn link', 'name', concept('relay')),
    ('=Spawn population lock', 'name', concept('limiter')),
    ('=Trunk port', 'name', concept('gate')),
    ('Pulse bomb', 'name', concept('core')),
    ('Control station', 'name', concept('console')),
    ('=Escape rocket', 'name', concept('vessel')),
    ('=Fuel station', 'name', concept('depot')),
    ('=Solar panel', 'name', concept('solar')),
    ('=Radar dish', 'name', concept('dish')),
    ('=Laser fence', 'name', concept('barrier')),
    ('=Wall buster', 'name', concept('breach')),
    ('Statue (CTS)', 'name', concept('relic')),
    ('=CTS drop zone', 'name', concept('relic_base')),
    ('KOTH hill', 'name', concept('zone')),
    ('Blitzkrieg flag', 'name', concept('flag_post', species_term='Flag post / Home post', note='home flag: Home post (HomePost, building.home_post)')),
    ('Ant hill (Ants Nest)', 'name', concept('hive')),
    ('=Incubator', 'name', concept('hatchery')),
    ('Gun turret, wall, tree, jump pad', 'name', v('turret, wall, tree, jump pad', 'Turret', 'building.turret', 'generic-keep', 'identifiers Turret, WallObject, TreeObject, JumpPadObject')),
    ('Darwinia buildings', 'name', concept('drop', note='campaign buildings')),
    ('Domination (code', 'name', concept('spawn_control')),
    ('King of the Hill (KOTH)', 'name', concept('zone_control')),
    ('Capture the Statue (CTS)', 'name', concept('relic_run')),
    ('=Assault', 'name', concept('siege')),
    ('=Rocket Riot', 'name', concept('launch_race')),
    ('Blitzkrieg (code', 'name', concept('flag_chain')),
    ('=Tank Battle', 'name', concept('vehicle_duel')),
    ('"Sudden Death"', 'text', v('sudden death, ready, grace period', 'SuddenDeathPhase', 'match.sudden_death', 'generic-keep', 'ordinary words; new text; identifiers SuddenDeathPhase, GracePeriod')),
    ('Crate ("Crate Drop!")', 'name', concept('crate', note='the drop announcement is new text: crate.drop_announce')),
    ('Task, power-up', 'name', concept('power')),
    ('Task Manager, task slots', 'name', concept('slots')),
    ('=Retribution', 'name', concept('comeback')),
    ('=Handicap', 'name', concept('catchup')),
    ('Squad (outcome)', 'name', concept('o_strike')),
    ('Armour / Harvester / Engineer', 'name', v('Transport / Collector / Builder (outcomes)', 'CrateOutcome::Transport', 'crate.transport', 'rename', 'also CrateOutcome::Collector (crate.collector), CrateOutcome::Builder (crate.builder)')),
    ('=Airstrike', 'name', concept('o_airstrike')),
    ('=Napalm Strike', 'name', concept('o_firebomb')),
    ('Napalm messages', 'text', v('new text "Firebombs inbound!"', '', 'crate.firebomb_run.announce', 'rename', 'film quote; never reproduced')),
    ('Nuke (Nuke Barrage)', 'name', concept('o_barrage')),
    ('Meteor (Meteor Shower)', 'name', concept('o_meteor')),
    ('Gun Turret / Rocket Turret', 'name', concept('o_turrets')),
    ('=Personal Shield', 'name', concept('o_shield')),
    ('=Subversion', 'name', concept('o_convert')),
    ('=Hot Feet', 'name', concept('o_speed')),
    ('=Rage', 'name', concept('o_fury')),
    ('Darwinians (', 'name', concept('o_reinf')),
    ('=Spawn Mania', 'name', concept('o_surge')),
    ('=Rocketwinians', 'name', concept('o_rocket')),
    ('=Crate Mania', 'name', concept('o_storm')),
    ('Mega Rage', 'name', concept('o_frenzy')),
    ('=Slow Down', 'name', concept('o_dilation')),
    ('=Speed Up', 'name', concept('o_rush')),
    ('=Magical Forest', 'name', concept('o_grove')),
    ('=Dark Forest', 'name', concept('o_blight')),
    ('=Ants Nest', 'name', concept('o_hive')),
    ('=Eggs', 'name', concept('o_brood')),
    ('Infection (code', 'name', concept('o_swarm')),
    ('=Randomiser', 'name', concept('o_scrambler')),
    ('=Plague', 'name', concept('o_sickness')),
    ('=Space Ship', 'name', concept('o_abductor')),
    ('=Triffids', 'name', concept('o_spitter')),
    ('=The Beast', 'name', concept('o_rift')),
    ('Task-manager fiction strings', 'text', v('new texts: Ready, Not available, Cancelled, Unlocked, Improved, Objectives complete, All power slots full, Comeback powers: slots full', '', 'ui.power_slots', 'rename', 'one row per string below')),
    ('`Multiwinia` (rules class)', 'identifier', concept('rules')),
    ('`MultiwiniaOptions`', 'identifier', concept('options')),
    ('=`MultiwiniaZone`', 'identifier', concept('zone', note='one type for the Control Zone, Relic Base, Relic Spawn Zone and flag posts')),
    ('=`MultiwiniaOfficerOrders`', 'identifier', concept('order_marker')),
    ('`NET_DARWINIA_*`', 'identifier', concept('netgame')),
    ('Eclipse (UI framework)', 'identifier', concept('uicore', note='controls UiPointerDown, UiPointerHeld, UiAltPointerDown')),
    ('`DarwiniaWindow`', 'identifier', v('UI window, UI button, map-editor hub page, map file picker, backdrop render', 'UiWindow', 'ui', 'rename', 'one row per identifier below')),
    ('`MultiwiniaControlMethod`', 'identifier', v('control scheme', 'ControlScheme', 'ui.options.control_scheme', 'rename', 'bindings file bindings_classic.txt')),
    ('Codenames `Pecan`', 'identifier', v('feature flags', 'AlwaysTrue', '', 'rename', 'Pecan to AlwaysTrue; Hummingbird to FeatureFlagX')),
    ('Camera mounts', 'identifier', v('camera views', 'spawn_view_<n>', 'camera.view', 'rename', 'start to default_view; CamPosBefore to previous_view')),
    ('`SepulvedaHelp`', 'identifier', v('advisor help, narrator', 'AdvisorHelp', '', 'drop', 'the advisor feature is dropped')),
    ('`MultiwiniaInterface`', 'identifier', v('match UI sound source', 'MatchUI', 'sfx.match_ui', 'rename')),
    ('Special profiles', 'identifier', concept('drop', note='developer and attract profiles')),
    ('"Futurewar Shape Format"', 'text', concept('drop', note='the mesh format is replaced; exporter text not carried')),
    ('"Gq/s"', 'text', v('units/s', '', '', 'rename', 'display unit')),
    ('"DWs"', 'text', v('Units', '', 'ui.debug.unit_count', 'rename', 'overlay label')),
    ('`MT19937`', 'identifier', v('own generator; public-domain hash', '', '', 'rename', 'std::mt19937 or an own generator; xxHash or BLAKE3')),
    ('`.mwm`', 'identifier', concept('drop', note='own extension')),
    ('"Multiwinia" (title', 'brand', concept('species')),
    ('Subtitle', 'text', v('Species tagline (new text)', '', 'ui.title.tagline', 'rename')),
    ('Title, logo, box', 'art', v('title wordmark; studio logo placeholder', 'game_title_wordmark', 'ui.title', 'rename', 'no store art; final ids from the asset manifest')),
    ('"Darwinia" (prequel', 'brand', v('', 'SPECIES_*', '', 'drop', 'campaign content is out of scope; identifiers use Species')),
    ('"Introversion Software"', 'brand', concept('studio')),
    ('Product and partner identifiers', 'identifier', concept('drop', note='own product ids')),
    ('Licensing material', 'brand', concept('drop', note='platform entitlements; never copied')),
    ('Store, metaserver', 'brand', concept('drop', note='online services are decided by the design phase')),
    ('Dr Sepulveda', 'name', v('Advisor', '', '', 'drop', 'feature dropped')),
    ('"Darwin Research Associates"', 'art', concept('drop', note='omitted')),
    ('Achievement names', 'name', v('Total Control, Stockpiler, Crowned, Clean Launch, Iron Defence, Giant Slayer, Completionist, Tourist, Massacre, Flag Sweep, Friendly Fire, Annihilator', 'Achievement', 'achievement', 'rename', 'one row per achievement below')),
    ('TrueSkill', 'brand', concept('skill')),
    ('Map file names', 'map', concept('mapname')),
    ('Map display names', 'map', concept('newmaps')),
    ('Map thumbnails', 'art', v('engine-generated map thumbnail', 'map_<mode>_<players>p_<nn>_thumb', 'map', 'rename')),
    ('Terrain colour ramps', 'art', concept('ramp')),
    ('Wave tables', 'art', v('shore-wave gradient; seabed texture', 'wavegrad_<theme>', '', 'rename', 'water tiles seabed_<theme>')),
    ('Fonts `speccy`', 'art', concept('fonts')),
    ('Darwinian sprite family', 'art', concept('newart', note='role families unit_*, fx_*, ui_*, icon_<ability>, help_<mode>_<page>, mode_<mode>')),
    ('All 1,332 audio samples', 'audio', concept('newaudio', note='sfx_<event>, vo_<role>_<line>, amb_<bed>')),
    ('Licensed music', 'audio', v('commissioned music', 'music_*', '', 'drop', 'never reused')),
    ('Christmas mode', 'art', concept('seasonal')),
    ('Gesture symbols', 'art', concept('drop')),
    ('POKEY', 'brand', concept('drop')),
    ('Distributors and partners', 'brand', concept('drop', note='platform entitlements instead of key prefixes')),
    ('Beta-tester credit lists', 'text', concept('drop', note='personal data; never shipped')),
    ('Developer names in comments', 'text', concept('drop', note='personal data')),
    ('All UI and narration text', 'text', concept('strings')),
    ('Literal English captions', 'text', concept('strings', species_term='new string-table entries')),
    ('Credits (', 'text', v('own credits', '', 'ui.credits', 'drop')),
    ('Cut-scene scripts', 'text', concept('scripts')),
    ('Darwinia campaign location names', 'name', concept('drop')),
    ('EULAs', 'text', concept('drop', note='own licence and rating text')),
    ('Controller glyphs', 'art', v('platform-neutral input glyphs', 'input_glyph_<button>', 'ui.glyph', 'rename', 'menu-cycle controls PadMenuCycleUp, PadMenuCycleDown')),
    ('Shader sources', 'art', concept('shaders')),
    ('Menu palettes', 'art', v('new palette keeping the token structure', '', '', 'rename')),
    ('Voice, bark and announcer sets', 'audio', v('new voice and ambience sets', 'vo_unit_select_01', '', 'rename', 'vo_reward_<function>, vo_match_win, amb_lobby_pad_01, amb_altitude_01')),
    ('Distinctive design expressions', 'art', v('new presentation of the same mechanic', '', '', 'rename')),
    ('Introversion tools and binaries', 'brand', v('own tools', '', '', 'drop', 'panel editor, crash reporter and updater are Species tools')),
]

# ---------------------------------------------------------------------------
# ORIG/90 §2 rows with IP "yes → …": (match, kind, value)
# ---------------------------------------------------------------------------
G2 = [
    ('=ASLT / DOM / RR / BLITZ / TANK', 'name', concept('mode_abbr')),
    ('=CTS', 'name', concept('relic_run')),
    ('=EULA', 'text', concept('drop', note='own licence')),
    ('=GDF', 'identifier', concept('drop', note='own ids')),
    ('=GOTO (order)', 'name', concept('beacon')),
    ('=KOTH', 'name', concept('zone_control')),
    ('=MT19937', 'identifier', v('own generator', '', '', 'rename', 'std::mt19937 or an own generator')),
    ('=MW', 'brand', concept('species')),
    ('=PDLC', 'map', concept('newmaps')),
    ('=POKEY', 'brand', concept('drop')),
    ('=SHA-1', 'identifier', v('public-domain hash', '', '', 'rename', 'xxHash or BLAKE3')),
    ('=TM', 'name', concept('slots')),
    ('=URL', 'brand', concept('drop', note='own endpoints from configuration')),
    ('=WMD', 'name', concept('core')),
    ('=XBLA / Xbox 360', 'brand', concept('drop', note='platform-neutral glyphs')),
    ('=Domination', 'name', concept('spawn_control')),
    ('=King of the Hill', 'name', concept('zone_control')),
    ('=Capture the Statue', 'name', concept('relic_run')),
    ('=Assault', 'name', concept('siege')),
    ('=Rocket Riot', 'name', concept('launch_race')),
    ('=Blitzkrieg', 'name', concept('flag_chain')),
    ('=Tank Battle', 'name', concept('vehicle_duel')),
    ('=Game type', 'name', concept('mode')),
    ('=Retribution', 'name', concept('comeback')),
    ('=Handicap', 'name', concept('catchup')),
    ('=Map', 'map', v('map', '', 'map', 'rename', 'new maps named map_<mode>_<players>p_<nn>')),
    ('=Tutorial', 'text', concept('scripts', species_term='new tutorial maps and copy')),
    ('=Animated panel', 'art', v('explainer animation', 'ExplainerAnimation', 'ui.help.explainer', 'rename')),
    ('=Achievement', 'name', concept('achievement')),
    ('=Authentication key', 'brand', concept('drop', note='platform entitlements')),
    ('=Christmas mode', 'art', concept('seasonal')),
    ('=Spawn network', 'name', v('spawn network', 'SpawnNetwork', 'building.spawn_network', 'generic-keep', 'the Source Node, Relays and Spawn Nodes')),
    ('=Promotion / demotion', 'name', v('promotion to Leader / demotion', 'PromoteToLeader', 'unit.leader.promote', 'rename')),
    ('Task (power', 'name', concept('power')),
    ('Power-up slot', 'name', v('Power slot', 'PowerSlot', 'ui.power_slots', 'rename')),
    ('=Hill (zone)', 'name', concept('zone')),
    ('Drop zone / statue spawn zone', 'name', concept('relic_base', species_term='Relic Base / Relic Spawn Zone', note='statue spawn zone: Relic Spawn Zone; ' + PENDING)),
    ('=Flag (Blitzkrieg)', 'name', concept('flag_post')),
    ('=Statue', 'name', concept('relic')),
    ('=Monster team', 'name', concept('rogue')),
    ('=Futurewinian team', 'name', concept('visitor')),
    ('=Crate', 'name', concept('crate')),
    ('Harvester (crate outcome)', 'name', concept('o_collector')),
    ('Engineer (crate outcome)', 'name', concept('o_builder')),
    ('Armour (crate outcome)', 'name', concept('o_transport')),
    ('Darwinians (', 'name', concept('o_reinf')),
    ('=Personal Shield', 'name', concept('o_shield')),
    ('=Subversion', 'name', concept('o_convert')),
    ('=Hot Feet', 'name', concept('o_speed')),
    ('=Infection', 'name', concept('o_swarm')),
    ('=Magical Forest', 'name', concept('o_grove')),
    ('Randomiser (crate outcome)', 'name', concept('o_scrambler')),
    ('=Ants Nest', 'name', concept('o_hive')),
    ('=Plague', 'name', concept('o_sickness')),
    ('=Eggs', 'name', concept('o_brood')),
    ('=Dark Forest', 'name', concept('o_blight')),
    ('=Crate Mania', 'name', concept('o_storm')),
    ('=Spawn Mania', 'name', concept('o_surge')),
    ('=Mega Rage', 'name', concept('o_frenzy')),
    ('=Rocketwinians', 'name', concept('o_rocket')),
    ('=Space Ship', 'name', concept('o_abductor')),
    ('=Triffids', 'name', concept('o_spitter')),
    ('=Napalm Strike', 'name', concept('o_firebomb')),
    ('=Slow Down', 'name', concept('o_dilation')),
    ('=Speed Up', 'name', concept('o_rush')),
    ('=The Beast', 'name', concept('o_rift')),
    ('=Darwinian', 'name', concept('worker')),
    ('=Officer', 'name', concept('leader')),
    ('=MultiwiniaOfficerOrders', 'identifier', concept('order_marker')),
    ('=Spirit', 'name', concept('orb')),
    ('InsertionSquad (squad)', 'identifier', concept('strike')),
    ('=Squadie', 'identifier', concept('trooper')),
    ('=Armour', 'name', concept('transport')),
    ('=Harvester', 'name', concept('collector')),
    ('=Engineer', 'name', concept('builder')),
    ('=SpaceInvader', 'identifier', concept('bomber')),
    ('=TriffidEgg', 'identifier', concept('seedpod')),
    ('=Virii', 'identifier', concept('crawler')),
    ('SporeGenerator (', 'identifier', concept('drifter')),
    ('SoulDestroyer (', 'identifier', concept('reaper')),
    ('=Zombie', 'identifier', concept('wisp')),
    ('=SpaceShip', 'identifier', concept('abductor')),
    ('=Shaman', 'identifier', concept('summoner')),
    ('=Tripod', 'identifier', concept('triwalker')),
    ('SpawnLink (', 'identifier', concept('relay')),
    ('MasterSpawnPoint (', 'identifier', concept('source_node')),
    ('SpawnPoint (', 'identifier', concept('spawn_node')),
    ('SpawnPopulationLock (', 'identifier', concept('limiter')),
    ('RadarDish (', 'identifier', concept('dish')),
    ('ControlStation (', 'identifier', concept('console')),
    ('SolarPanel (', 'identifier', concept('solar')),
    ('LaserFence (', 'identifier', concept('barrier')),
    ('MultiwiniaZone (', 'identifier', concept('zone')),
    ('WallBuster (', 'identifier', concept('breach')),
    ('TrunkPort (', 'identifier', concept('gate')),
    ('PulseBomb (', 'identifier', concept('core')),
    ('EscapeRocket (', 'identifier', concept('vessel')),
    ('AntHill (', 'identifier', concept('hive')),
    ('=Triffid', 'identifier', concept('spitter')),
    ('=Portal', 'identifier', v('Rift', 'Rift', 'building.rift', 'drop', 'debug-only portal of the Rift Horror')),
    ('=Randomiser', 'identifier', v('Allegiance Scrambler (effect)', 'AllegianceScrambler', 'crate.allegiance_scrambler', 'rename')),
    ('=Incubator', 'identifier', concept('hatchery')),
    ('BlueprintBuilding /', 'identifier', concept('drop', note='campaign buildings')),
    ('=Spam', 'identifier', concept('drop', note='campaign building; the live outcome is the Swarm')),
    ('=SpamInfection', 'identifier', v('Swarm streak', 'SwarmStreak', 'crate.swarm', 'rename', 'homing streak of the Swarm outcome')),
    ('Multiwinia (rules object)', 'identifier', concept('rules')),
    ('TaskManager / Task', 'identifier', v('Power slots / Power', 'PowerSlots', 'ui.power_slots', 'rename', 'Task to Power')),
    ('=Metaserver', 'brand', concept('drop', note='online services are decided by the design phase')),
    ('=Matchmaker', 'brand', concept('drop', note='online services are decided by the design phase')),
    ('Authentication (keys)', 'brand', concept('drop', note='platform entitlements')),
    ('=SystemIV', 'brand', concept('drop', note='own or standard libraries')),
    ('=Colour ramp', 'art', concept('ramp')),
    ('=Font atlas', 'art', concept('fonts')),
    ('Camera mount / camera animation', 'identifier', v('camera view', 'spawn_view_<n>', 'camera.view', 'rename', 'start to default_view')),
    ('=EffectProcessor', 'identifier', concept('drop')),
    ('=Eclipse', 'identifier', concept('uicore')),
    ('DarwiniaWindow / DarwiniaButton', 'identifier', v('UI window / UI button', 'UiWindow', 'ui', 'rename', 'DarwiniaButton to UiButton')),
    ('=Phrase id', 'text', v('Species string id', '', '', 'rename', 'dotted ids such as ui.lobby.ready')),
    ('=Gesture recognition', 'identifier', concept('drop')),
    ('=Sepulveda', 'identifier', v('Advisor', '', '', 'drop', 'feature dropped')),
    ('=Boot-loader', 'identifier', concept('drop')),
    ('Crash reporter (', 'brand', v('crash reporter', 'CrashReporter', '', 'rename', 'own tool')),
    ('.dat (archive)', 'identifier', v('pack file', 'PackFile', '', 'rename', 'own packaging names')),
    ('.shp (shape)', 'art', v('new meshes', '', '', 'rename', 'glTF sources per the asset pipeline')),
    ('=.max', 'art', v('new authoring sources', '', '', 'rename')),
    ('=.anim', 'art', v('new explainer choreographies', '', '', 'rename')),
    ('.bmp (BMP)', 'art', concept('newart')),
    ('=.ogg', 'audio', concept('newaudio')),
    ('=HLSL shader files', 'art', concept('shaders')),
    ('=Level file', 'map', concept('mapdef')),
    ('MultiwiniaOptions (block)', 'identifier', concept('options')),
    ('=level_enum.txt', 'map', v('map ids', '', 'map', 'rename', 'new ids')),
    ('=sounds.txt', 'audio', v('sound event set', '', 'sfx', 'rename', 'new event set')),
    ('gestures.txt / gesture demos', 'identifier', concept('drop')),
    ('=Language file', 'text', concept('strings')),
    ('=Cut-scene script', 'text', concept('scripts', species_term='new scripts')),
    ('Credit lists (betatesters)', 'text', concept('drop', note='personal data; never shipped')),
    ('=redshirt2', 'identifier', concept('drop', note='signed or hashed assets instead')),
    ('Map CRC (mapcrc.h)', 'map', v('map id', 'MapId', 'map', 'rename', 'new map ids')),
    ('=MultiwiniaSupport.ini', 'identifier', concept('drop')),
    ('Thumbnails / mwhelp / icons naming', 'art', concept('newart')),
    ('=Soul', 'name', concept('orb')),
    ('=Multiwinian(s)', 'name', concept('worker', species_term='Worker(s)')),
    ('=Multiwinia', 'brand', concept('species')),
    ('=Darwinia', 'brand', v('', 'SPECIES_*', '', 'drop', 'campaign content is out of scope; identifiers use Species')),
    ('=Introversion Software', 'brand', concept('studio')),
    ('=Evilinians', 'name', concept('rogue')),
    ('=Futurewinians', 'name', concept('visitor')),
    ('=Dr Sepulveda', 'name', v('Advisor', '', '', 'drop', 'feature dropped')),
    ('=Darwin Research Associates', 'art', concept('drop', note='omitted')),
    ('Pecan / Hummingbird', 'identifier', v('feature flags', 'AlwaysTrue', '', 'rename', 'Hummingbird to FeatureFlagX')),
    ('Speccy / editor / square', 'art', concept('fonts')),
    ('=Achievement names', 'name', v('Species achievement names', 'Achievement', 'achievement', 'rename', 'one row per achievement below')),
    ('=Map names', 'map', concept('newmaps')),
    ('=TrueSkill', 'brand', concept('skill')),
    ('=Gq/s', 'text', v('units/s', '', '', 'rename')),
    ('=NET_DARWINIA_*', 'identifier', concept('netgame')),
    ('Space Invader (name)', 'name', concept('bomber')),
    ('Triffid (name)', 'name', concept('spitter')),
    ('Distributor and partner names', 'brand', concept('drop')),
    ('Profile directory', 'identifier', v(r'<STUDIO>\Species\ profile folder', '', '', 'rename')),
]

# ---------------------------------------------------------------------------
# Extra rows: forms, code identifiers, collision-screen additions.
# (original, kind, value)
# ---------------------------------------------------------------------------
X = []


def add(orig, kind, val):
    X.append((orig, kind, val))


# plural, adjective and code forms of the names
for o, key, term in [
    ('Darwinians', 'worker', 'Workers'), ('Multiwinian', 'worker', 'Worker'), ('Multiwinians', 'worker', 'Workers'),
    ('darwinian', 'worker', 'Worker'), ('Officers', 'leader', 'Leaders'), ('Squadies', 'trooper', 'Troopers'),
    ('Squaddie', 'trooper', 'Trooper'), ('InsertionSquadie', 'trooper', 'Trooper'), ('Insertion Squad', 'strike', 'Strike team'),
    ('Spirits', 'orb', 'Essence orbs'), ('Souls', 'orb', 'Essence orbs'), ('Evilinian', 'rogue', 'Rogue faction'),
    ('Futurewinian', 'visitor', 'Visitor faction'), ('Soul Destroyers', 'reaper', 'Reaper Serpents'),
    ('Spore Generators', 'drifter', 'Brood Drifters'), ('Space Invaders', 'bomber', 'Bombers'),
    ('spawn points', 'spawn_node', 'Spawn Nodes'), ('spawn links', 'relay', 'Relays'), ('trunk ports', 'gate', 'Transit Gates'),
    ('pulse bombs', 'core', 'Core Devices'), ('control stations', 'console', 'Command Consoles'),
    ('escape rockets', 'vessel', 'Launch Vessels'), ('solar panels', 'solar', 'Solar Arrays'), ('radar dishes', 'dish', 'Beam Dishes'),
    ('laser fences', 'barrier', 'Barrier Emitters'), ('wall busters', 'breach', 'Breach Charges'), ('statues', 'relic', 'Relics'),
    ('WMD Control Panel', 'console', 'Command Console'), ('ant hill', 'hive', 'Hive'),
]:
    add(o, 'name', concept(key, species_term=term, note='form'))

# collision-screen additions and generic objects
add('Subversion turret', 'name', v('Convert Turret', 'TurretType::Convert', 'building.turret.convert', 'rename', 'follows the Convert rename; ' + PENDING))
add('Standard turret (gun turret)', 'name', v('gun turret', 'TurretType::Gun', 'building.turret.gun', 'generic-keep'))
add('Rocket turret', 'name', v('rocket turret', 'TurretType::Rocket', 'building.turret.rocket', 'generic-keep'))
add('Mortar turret ("grenade turret")', 'name', v('mortar turret', 'TurretType::Mortar', 'building.turret.mortar', 'generic-keep'))
add('Flame turret', 'name', v('flame turret', 'TurretType::Flame', 'building.turret.flame', 'generic-keep'))
add('Battle Cannon (barrel shape name)', 'art', concept('drop'))
add('wall', 'name', v('wall', 'WallObject', 'building.wall', 'generic-keep'))
add('tree', 'name', v('tree', 'TreeObject', 'building.tree', 'generic-keep'))
add('jump pad', 'name', v('jump pad', 'JumpPadObject', 'building.jump_pad', 'generic-keep'))
add('home flag', 'name', concept('home_post'))
add('statue spawn zone', 'name', concept('relic_spawn'))
add('Restriction Zone', 'name', v('exclusion zone', 'ExclusionZone', 'building.exclusion_zone', 'rename', 'generic term'))
add('power network', 'name', v('energy grid', 'EnergyGrid', 'building.energy_grid', 'rename', 'keeps Power for the supply-crate power; ' + PENDING))
add('power surge', 'name', v('energy surge', 'EnergySurge', 'building.energy_grid.surge', 'rename', 'with the energy grid; ' + PENDING))
add('Pylon', 'name', v('grid pylon', 'GridPylon', 'building.grid_pylon', 'rename', 'with the energy grid'))
add('PylonEnd', 'identifier', v('grid terminal', 'GridTerminal', 'building.grid_terminal', 'rename', 'with the energy grid'))
add('PowerSplitter', 'identifier', v('grid splitter', 'GridSplitter', 'building.grid_splitter', 'rename', 'with the energy grid'))
add('PowerBuilding', 'identifier', v('energy-grid structure', 'GridStructure', '', 'rename', 'base type'))
add('Fuel generator', 'name', v('fuel pump', 'FuelPump', 'building.fuel_pump', 'rename', 'turns energy surges into fuel pressure'))
add('Fuel pipe', 'name', v('fuel line', 'FuelLine', 'building.fuel_line', 'rename'))
add('Uplink Console (rejected 90 placeholder)', 'name', concept('console', note='collision with a studio title; ' + PENDING))
add('Warp Gate (rejected 90 placeholder)', 'name', concept('gate', note='collision with a franchise term; ' + PENDING))
add('Relay Dish (rejected 90 placeholder)', 'name', concept('dish', note='internal collision with Relay; ' + PENDING))
add('Energy Collector (rejected 90 placeholder)', 'name', concept('solar', note='internal collision with Collector; ' + PENDING))
add('Spirit Grove (rejected 90 placeholder)', 'name', concept('o_grove', note='reuses the original concept word; ' + PENDING))
add('Armour Duel (rejected 90 placeholder)', 'name', concept('vehicle_duel', note='reuses the original unit name'))
add('Lightning War (rejected 90 placeholder)', 'name', v('Flag Sweep', 'Achievement::FlagSweep', 'achievement.flag_sweep', 'rename', 'literal rendering of the original mode name; ' + PENDING))

# achievements
for o, term, ident in [
    ('Dominator', 'Total Control', 'TotalControl'), ('Hoarder', 'Stockpiler', 'Stockpiler'), ('Hail To The King', 'Crowned', 'Crowned'),
    ('Unclad Skies', 'Clean Launch', 'CleanLaunch'), ('Aggravated Assault', 'Iron Defence', 'IronDefence'),
    ('Master Player', 'Giant Slayer', 'GiantSlayer'), ('Master Of Multiwinia', 'Completionist', 'Completionist'),
    ('Explorer', 'Tourist', 'Tourist'), ('Carnage', 'Massacre', 'Massacre'), ('Blitz Master', 'Flag Sweep', 'FlagSweep'),
    ('Wrong Game', 'Friendly Fire', 'FriendlyFire'), ('Genocide', 'Annihilator', 'Annihilator'),
]:
    snake = re.sub(r'(?<!^)(?=[A-Z])', '_', ident).lower()
    note = 'replaces 90 name Lightning War; ' + PENDING if term == 'Flag Sweep' else ''
    add(o, 'name', v(term, 'Achievement::' + ident, 'achievement.' + snake, 'rename', note))
for o, ident in [('HailToTheKing', 'Crowned'), ('UncladSkies', 'CleanLaunch'), ('AggravatedAssault', 'IronDefence'), ('BlitzMaster', 'FlagSweep')]:
    add(o, 'identifier', v('achievement', 'Achievement::' + ident, '', 'rename', 'code form'))

# task-manager fiction strings and other texts
for o, term, sid in [
    ('RUN PROGRAM', 'Ready', 'ui.power_slots.ready'), ('SYNTAX ERROR', 'Not available', 'ui.power_slots.not_available'),
    ('PROGRAM TERMINATED', 'Cancelled', 'ui.power_slots.cancelled'), ('ACQUIRED', 'Unlocked', 'ui.power_slots.unlocked'),
    ('UPGRADED', 'Improved', 'ui.power_slots.improved'), ('OBJECTIVES COMPLETED', 'Objectives complete', 'ui.match.objectives_complete'),
    ('Powerup Limit Reached!', 'All power slots full', 'ui.power_slots.full'),
    ('Retribution : Two powerups max', 'Comeback powers: slots full', 'ui.power_slots.comeback_full'),
    ('Crate Drop!', 'new announcement text', 'crate.drop_announce'),
    ('Sudden Death', 'sudden death', 'match.sudden_death'), ('Grace period', 'grace period', 'match.grace_period'),
    ('Ready', 'ready', 'ui.lobby.ready'),
]:
    rule = 'generic-keep' if o in ('Sudden Death', 'Grace period', 'Ready') else 'rename'
    add(o, 'text', v(term, '', sid, rule, 'new text' if rule == 'rename' else 'ordinary words; new text'))

# mode code forms and key tokens
for o, key in [('GameTypeSkirmish', 'spawn_control'), ('GameTypeKingOfTheHill', 'zone_control'), ('KingOfTheHill', 'zone_control'),
               ('GameTypeCaptureTheStatue', 'relic_run'), ('CaptureTheStatue', 'relic_run'), ('GameTypeAssault', 'siege'),
               ('AssaultMode', 'siege'), ('GameTypeRocketRiot', 'launch_race'), ('RocketRiot', 'launch_race'),
               ('GameTypeBlitzkreig', 'flag_chain'), ('Blitzkreig', 'flag_chain'), ('GameTypeTankBattle', 'vehicle_duel'),
               ('TankBattle', 'vehicle_duel')]:
    t = concept(key)
    if t['species_identifier']:
        t['species_identifier'] = 'MatchMode::' + t['species_identifier']
    t['note'] = 'code form'
    add(o, 'identifier', t)
for o, mode_id in [('domination', 'spawn_control'), ('skirmish', 'spawn_control'), ('koth', 'zone_control'), ('cts', 'relic_run'),
                   ('assault', 'siege'), ('rocketriot', 'launch_race'), ('blitzkrieg', 'flag_chain')]:
    add('mp_%s_* (map file mode word)' % o, 'map', v('map_%s_<players>p_<nn>' % mode_id, 'map_%s_<players>p_<nn>' % mode_id, 'map', 'rename', 'map file name pattern'))
add('mp_tankbattle_*, mp_sandbox*, mp_benchmark_1, mp_assault_stress (map files)', 'map', concept('drop', note='test, sandbox, benchmark and debug maps'))
add('map_* / mission_* (campaign level files)', 'map', concept('drop', note='campaign levels'))
add('MAPID_*', 'identifier', v('map id', 'MapId', 'map', 'rename', 'new map ids'))
add('Bitzkreig', 'identifier', concept('o_frenzy', note='code form of Frenzy'))
add('CrateRewardBitzkreig', 'identifier', concept('o_frenzy', note='code form'))

# UI identifiers of 90 §3.7
for o, ident in [('EclipseLMouseDown', 'UiPointerDown'), ('EclipseLMousePressed', 'UiPointerHeld'), ('EclipseRMousePressed', 'UiAltPointerDown'),
                 ('DarwiniaWindow', 'UiWindow'), ('DarwiniaButton', 'UiButton'), ('MultiwiniaWindow', 'MapEditorHubPage'),
                 ('MultiwiniaFileDialog', 'MapFilePicker'), ('RenderBigDarwinian', 'RenderBackdrop'), ('EclWindow', 'UiPanelBase'),
                 ('EclButton', 'UiButtonBase'), ('MultiwiniaControlMethod', 'ControlScheme'), ('input_preferences_darwinia*', 'bindings_classic.txt'),
                 ('ControlXBoxMenuCycleUp', 'PadMenuCycleUp'), ('ControlXBoxMenuCycleDown', 'PadMenuCycleDown'),
                 ('Pecan', 'AlwaysTrue'), ('Hummingbird', 'FeatureFlagX'), ('player0 to player3 (camera mounts)', 'spawn_view_<n>'),
                 ('start (camera mount)', 'default_view'), ('CamPosBefore', 'previous_view')]:
    add(o, 'identifier', v(re.sub(r'(?<!^)(?=[A-Z])', ' ', ident).lower() if not ident.islower() else ident, ident, '', 'rename', 'code form'))

# world-object classes (ORIG/91 §4) and core engine types: Species identifiers.
TYPES = [
    # (original, species_identifier, species_term, rule, note)
    ('WorldObject', 'SimObject', 'simulated object', 'rename', 'root of every simulated object'),
    ('WorldObjectId', 'SimObjectId', 'simulated-object handle', 'rename', ''),
    ('Entity', 'Actor', 'actor', 'rename', 'base of every mobile unit and creature'),
    ('Entity::Type', 'ActorKind', 'actor kind', 'rename', 'enumeration; keep the order where it carries meaning'),
    ('EntityBlueprint', 'ActorStatTable', 'actor stat table', 'rename', 'health, speed and rate per actor kind'),
    ('Building', 'Structure', 'structure', 'rename', 'base of every static world object'),
    ('Building::Type', 'StructureKind', 'structure kind', 'rename', 'enumeration'),
    ('BuildingPort', 'StructurePort', 'structure port', 'rename', 'a manning slot'),
    ('CarryableBuilding', 'CarryableStructure', 'carryable structure', 'rename', ''),
    ('Unit', 'ActorGroup', 'actor group', 'rename', 'a team-owned group of same-kind actors'),
    ('Team', 'MatchTeam', 'team', 'rename', ''),
    ('LobbyTeam', 'LobbySlot', 'lobby slot', 'rename', ''),
    ('TeamControls', 'TeamCommandState', 'team command state', 'rename', 'per-tick input state of a team'),
    ('Location', 'MatchWorld', 'match world', 'rename', 'the running world of one match'),
    ('App', 'Application', 'application', 'rename', ''),
    ('GameTimer', 'TickClock', 'tick clock', 'rename', ''),
    ('GameType', 'MatchMode', 'mode', 'rename', 'enumeration'),
    ('CrateReward', 'CrateOutcome', 'supply-crate outcome', 'rename', 'enumeration'),
    ('AITypeEasy', 'BotDifficulty::Easy', 'easy bot', 'rename', ''),
    ('AITypeStandard', 'BotDifficulty::Standard', 'standard bot', 'rename', ''),
    ('AITypeHard', 'BotDifficulty::Hard', 'hard bot', 'rename', ''),
    ('TeamTypeCPU', 'TeamKind::Bot', 'bot team', 'rename', ''),
    ('TeamTypeLocalPlayer', 'TeamKind::LocalPlayer', 'local player team', 'rename', ''),
    ('TeamTypeRemotePlayer', 'TeamKind::RemotePlayer', 'remote player team', 'rename', ''),
    ('TeamTypeSpectator', 'TeamKind::Spectator', 'spectator', 'rename', ''),
    ('TeamTypeUnused', 'TeamKind::Unused', 'unused slot', 'rename', ''),
    ('LevelOptions', 'MapOption', 'map option', 'rename', 'enumeration of per-map options'),
    ('LevelEffect', 'MapEffect', 'map effect', 'rename', 'enumeration of per-map ambient effects'),
    ('GlobalResearch', 'PowerLevels', 'power and research levels', 'rename', 'power kinds and their levels'),
    ('GlobalWorld', '', '', 'drop', 'campaign world map'),
    ('LevelFile', 'MapDefinition', 'map definition', 'rename', ''),
    ('Landscape', 'TerrainField', 'terrain height field', 'rename', ''),
    ('LandscapeTile', 'TerrainTile', 'terrain tile', 'rename', ''),
    ('LandscapeDef', 'TerrainDefinition', 'terrain definition', 'rename', ''),
    ('LandscapeFlattenArea', 'TerrainFlattenArea', 'terrain flatten area', 'rename', ''),
    ('LandscapeRenderer', 'TerrainRenderer', 'terrain renderer', 'rename', ''),
    ('Water', 'SeaRenderer', 'sea renderer', 'rename', ''),
    ('EntityGrid', 'ActorGrid', 'actor grid', 'rename', 'spatial index of actors'),
    ('EntityGridCache', 'ActorGridCache', 'actor grid cache', 'rename', ''),
    ('ObstructionGrid', 'ObstacleGrid', 'obstacle grid', 'rename', 'spatial index of structures'),
    ('RoutingSystem', 'RouteGraph', 'route graph', 'rename', ''),
    ('LandRoutingSystem', 'LandRouteGraph', 'land route graph', 'rename', ''),
    ('SeaRoutingSystem', 'SeaRouteGraph', 'sea route graph', 'rename', ''),
    ('RouteNode', 'RouteGraphNode', 'route graph node', 'rename', ''),
    ('WayPoint', 'RoutePoint', 'route point', 'rename', ''),
    ('HistoricWayPoint', 'TrailPoint', 'trail point', 'rename', 'breadcrumb of a Strike team'),
    ('SliceDArray', 'SlicedPool', 'sliced pool', 'rename', 'pool whose members update in per-tick slices'),
    ('DArray', '', '', 'drop', 'standard containers'),
    ('FastDArray', '', '', 'drop', 'standard containers'),
    ('LList', '', '', 'drop', 'standard containers'),
    ('BTree', '', '', 'drop', 'standard containers'),
    ('HashTable', '', '', 'drop', 'standard containers'),
    ('SortingHashTable', '', '', 'drop', 'standard containers'),
    ('BoundedArray', '', '', 'drop', 'standard containers'),
    ('Array2D', 'Grid2D', '2-D grid', 'rename', ''),
    ('SurfaceMap2D', 'SurfaceGrid', 'world surface grid', 'rename', ''),
    ('UnicodeString', '', '', 'drop', 'standard UTF-8 strings'),
    ('Vector3', 'Vec3', '3-vector', 'rename', 'or the maths type chosen by ADR'),
    ('Matrix34', 'Mat3x4', '3x4 matrix', 'rename', 'or the maths type chosen by ADR'),
    ('RGBAColour', 'ColourRgba', 'RGBA colour', 'rename', ''),
    ('BitmapRGBA', 'ImageRgba8', 'RGBA8 image', 'rename', ''),
    ('TextReader', '', '', 'drop', 'typed schemas replace the text tokeniser'),
    ('BinaryReader', '', '', 'drop', ''),
    ('FileSystem', '', '', 'drop', ''),
    ('WorkQueue', 'JobQueue', 'job queue', 'rename', ''),
    ('Directory', '', '', 'drop', 'network payload container'),
    ('TaskManagerInterfaceMultiwinia', 'PowerSlotsPanel', 'power slots panel', 'rename', ''),
    ('TaskManager', 'PowerSlots', 'Power slots', 'rename', ''),
    ('Task', 'Power', 'Power', 'rename', ''),
    ('Renderer', 'FrameRenderer', 'frame renderer', 'rename', ''),
    ('Camera', 'MatchCamera', 'match camera', 'rename', ''),
    ('Shape', 'MeshAsset', 'mesh', 'rename', ''),
    ('ShapeFragment', 'MeshNode', 'mesh node', 'rename', ''),
    ('ShapeMarker', 'MeshSocket', 'mesh socket', 'rename', ''),
    ('Resource', 'AssetCache', 'asset cache', 'rename', ''),
    ('ParticleSystem', 'ParticlePool', 'particle pool', 'rename', ''),
    ('Particle', 'ParticleInstance', 'particle', 'rename', ''),
    ('ExplosionManager', 'ShatterEffects', 'shatter effects', 'rename', 'mesh-shattering explosions'),
    ('MarkerSystem', 'WorldMarkers', 'world markers', 'rename', ''),
    ('GameCursor', 'WorldCursor', 'world cursor', 'rename', ''),
    ('SoundSystem', 'AudioEngine', 'audio engine', 'rename', ''),
    ('SoundInstance', 'SoundVoice', 'sound voice', 'rename', ''),
    ('SoundSourceBlueprint', 'SoundSourceDef', 'sound source definition', 'rename', ''),
    ('SoundEventBlueprint', 'SoundEventDef', 'sound event definition', 'rename', ''),
    ('SoundParameter', 'SoundParam', 'sound parameter', 'rename', ''),
    ('SoundStreamDecoder', 'AudioDecoder', 'audio decoder', 'rename', ''),
    ('CachedSampleManager', 'SampleCache', 'sample cache', 'rename', ''),
    ('DspBlueprint', 'DspFilterDef', 'DSP filter definition', 'rename', ''),
    ('SoundLibrary2d', '', '', 'drop', 'device back-end'),
    ('SoundLibrary3d', '', '', 'drop', 'device back-end'),
    ('TextRenderer', 'GlyphRenderer', 'glyph renderer', 'rename', ''),
    ('WindowManager', '', '', 'drop', 'the platform layer defines its own'),
    ('InputManager', 'InputRouter', 'input router', 'rename', ''),
    ('InputDriver', 'InputSourceNode', 'input source node', 'rename', ''),
    ('UserInput', 'UiInputPump', 'UI input pump', 'rename', ''),
    ('LocationInput', 'MatchInputPump', 'match input pump', 'rename', ''),
    ('ControlBindings', 'ActionBindings', 'action bindings', 'rename', ''),
    ('LangTable', 'StringTable', 'string table', 'rename', ''),
    ('PrefsManager', 'SettingsStore', 'settings store', 'rename', ''),
    ('GameMenu', 'FrontEndController', 'front-end controller', 'rename', ''),
    ('GameMenuWindow', 'FrontEndScreen', 'front-end screen', 'rename', ''),
    ('GameOptionsWindow', 'OptionsScreen', 'options screen', 'rename', ''),
    ('LoadingScreen', 'LoadingView', 'loading view', 'rename', ''),
    ('AnimatedPanel', 'ExplainerAnimation', 'explainer animation', 'rename', ''),
    ('MultiwiniaHelp', 'HintOverlay', 'hint overlay', 'rename', ''),
    ('AchievementTracker', 'AchievementEvents', 'achievement events', 'rename', 'hooks only'),
    ('LeaderboardStats', 'MatchStatistics', 'match statistics', 'rename', 'hooks only'),
    ('LocationEditor', 'MapEditor', 'map editor', 'rename', 'hooks only'),
    ('MainEditWindow', 'MapEditorWindow', 'map editor window', 'rename', 'hooks only'),
    ('MapFile', '', '', 'drop', ''),
    ('MapCRC', 'MapId', 'map id', 'rename', ''),
    ('Clouds', 'SkyRenderer', 'sky renderer', 'rename', ''),
    ('Script', '', '', 'drop', 'cut-scene script interpreter'),
    ('IFrame', '', '', 'drop', 'networking'),
    ('Server', '', '', 'drop', 'networking'),
    ('ClientToServer', '', '', 'drop', 'networking'),
    ('ServerToClientLetter', '', '', 'drop', 'networking'),
    ('NetLib', '', '', 'drop', 'networking'),
    ('NetworkValue', '', '', 'drop', 'networking'),
    ('FTPManager', '', '', 'drop', 'networking'),
    ('BandwidthCounter', '', '', 'drop', 'networking'),
    ('Profiler', '', '', 'drop', 'debug build only'),
    ('AVIGenerator', '', '', 'drop', 'not compiled'),
    ('BenchMark', '', '', 'drop', 'not compiled'),
    ('StartSequence', '', '', 'drop', 'never constructed'),
    ('DemoEndSequence', '', '', 'drop', 'not compiled'),
    # world-object classes
    ('AI', 'BotController', 'bot controller', 'rename', 'the invisible per-team bot entity'),
    ('AIObjective', 'BotObjective', 'bot objective', 'rename', ''),
    ('AIObjectiveMarker', 'BotObjectiveMarker', 'bot objective marker', 'rename', ''),
    ('AITarget', 'BotNavNode', 'bot navigation node', 'rename', ''),
    ('AirStrikeMarker', 'AirstrikeTargetMarker', 'airstrike target marker', 'rename', ''),
    ('AirstrikeUnit', 'AirstrikeFlight', 'airstrike flight', 'rename', 'group of Bombers'),
    ('AntHill', 'Hive', 'Hive', 'rename', ''),
    ('Armour', 'Transport', 'Transport', 'rename', ''),
    ('ArmyAnt', 'AntCreature', 'ant', 'rename', ''),
    ('Centipede', 'CentipedeCreature', 'centipede', 'rename', ''),
    ('ControlStation', 'CommandConsole', 'Command Console', 'rename', PENDING),
    ('Crate', 'SupplyCrate', 'Supply crate', 'rename', ''),
    ('Darwinian', 'Worker', 'Worker', 'rename', ''),
    ('DustBall', 'DustVortex', 'dust vortex', 'rename', 'ambient map effect'),
    ('Egg', 'CrawlerEgg', 'Crawler egg', 'rename', ''),
    ('Engineer', 'Builder', 'Builder', 'rename', ''),
    ('EntityFoot', 'ProceduralFoot', 'procedural foot', 'rename', ''),
    ('EntityLeg', 'ProceduralLeg', 'procedural leg', 'rename', ''),
    ('EruptionMarker', 'VolcanoVent', 'volcano vent', 'rename', 'ambient emitter'),
    ('EscapeRocket', 'LaunchVessel', 'Launch Vessel', 'rename', ''),
    ('Fireball', 'FireballProjectile', 'fireball', 'rename', ''),
    ('Flag', 'ClothFlag', 'cloth flag', 'rename', 'render primitive'),
    ('FlameGrenade', 'FirebombCanister', 'firebomb canister', 'rename', ''),
    ('FuelBuilding', 'FuelStructure', 'fuel-chain structure', 'rename', ''),
    ('FuelGenerator', 'FuelPump', 'fuel pump', 'rename', ''),
    ('FuelPipe', 'FuelLine', 'fuel line', 'rename', ''),
    ('FuelStation', 'FuelDepot', 'fuel depot', 'rename', ''),
    ('Grenade', 'GrenadeProjectile', 'grenade', 'rename', ''),
    ('GunTurret', 'Turret', 'turret', 'rename', ''),
    ('Harvester', 'Collector', 'Collector', 'rename', ''),
    ('InsertionSquad', 'StrikeTeam', 'Strike team', 'rename', ''),
    ('JumpPad', 'JumpPadObject', 'jump pad', 'rename', ''),
    ('Laser', 'LaserBolt', 'laser bolt', 'rename', ''),
    ('LaserFence', 'BarrierEmitter', 'Barrier Emitter', 'rename', ''),
    ('Light', 'MapLight', 'map light', 'rename', ''),
    ('Lightning', 'LightningStrike', 'lightning strike', 'rename', 'ambient map effect'),
    ('MasterSpawnPoint', 'SourceNode', 'Source Node', 'rename', ''),
    ('SpawnPointMaster', 'SourceNode', 'Source Node', 'rename', 'map-file name'),
    ('Meteor', 'MeteorRock', 'meteor', 'rename', ''),
    ('MultiwiniaOfficerOrders', 'OrderMarker', 'order marker', 'rename', ''),
    ('MultiwiniaZone', 'ControlZone', 'Control Zone', 'rename', ''),
    ('MuzzleFlash', 'MuzzleFlashEffect', 'muzzle flash', 'rename', ''),
    ('Nuke', 'BarrageMissile', 'barrage missile', 'rename', 'one warhead of a Missile Barrage'),
    ('Officer', 'Leader', 'Leader', 'rename', ''),
    ('OfficerFormation', 'FormationSlot', 'formation slot', 'rename', ''),
    ('OfficerOrderTrail', 'OrderTrailMark', 'order trail mark', 'rename', ''),
    ('PulseBomb', 'CoreDevice', 'Core Device', 'rename', ''),
    ('PulseWave', 'CorePulseWave', 'Core Device pulse wave', 'rename', ''),
    ('RadarDish', 'BeamDish', 'Beam Dish', 'rename', PENDING),
    ('Randomiser', 'AllegianceScrambler', 'Allegiance Scrambler', 'rename', ''),
    ('EffectRandomiser', 'AllegianceScramblerEffect', 'Allegiance Scrambler effect', 'rename', ''),
    ('RestrictionZone', 'ExclusionZone', 'exclusion zone', 'rename', ''),
    ('Rocket', 'RocketProjectile', 'rocket', 'rename', ''),
    ('Shockwave', 'ShockwaveRing', 'shockwave ring', 'rename', ''),
    ('SmokeMarker', 'SmokeColumn', 'smoke column', 'rename', 'ambient emitter'),
    ('Snow', 'SnowFlake', 'snowflake', 'rename', 'ambient map effect'),
    ('SolarPanel', 'SolarArray', 'Solar Array', 'rename', PENDING),
    ('SoulDestroyer', 'ReaperSerpent', 'Reaper Serpent', 'rename', ''),
    ('SpaceInvader', 'Bomber', 'Bomber', 'rename', ''),
    ('SpaceShip', 'Abductor', 'Abductor', 'rename', ''),
    ('SpamInfection', 'SwarmStreak', 'Swarm streak', 'rename', ''),
    ('SpawnBuilding', 'SpawnStructure', 'spawn-network structure', 'rename', ''),
    ('SpawnBuildingLink', 'SpawnEdge', 'spawn-network edge', 'rename', ''),
    ('SpawnBuildingSpirit', 'TransitOrb', 'transit orb', 'rename', 'virtual orb travelling the spawn network'),
    ('SpawnLink', 'Relay', 'Relay', 'rename', ''),
    ('SpawnPoint', 'SpawnNode', 'Spawn Node', 'rename', ''),
    ('SpawnPopulationLock', 'PopulationLimiter', 'Population Limiter', 'rename', ''),
    ('Spider', 'SpiderCreature', 'spider', 'rename', ''),
    ('SpiderParameters', 'SpiderGait', 'spider gait', 'rename', ''),
    ('Spirit', 'EssenceOrb', 'Essence orb', 'rename', ''),
    ('SporeGenerator', 'BroodDrifter', 'Brood Drifter', 'rename', ''),
    ('Squadie', 'Trooper', 'Trooper', 'rename', ''),
    ('StaticShape', 'SceneryMesh', 'scenery mesh', 'rename', ''),
    ('Statue', 'Relic', 'Relic', 'rename', ''),
    ('Teleport', 'TeleportStructure', 'teleport', 'rename', 'base of the Beam Dish'),
    ('ThrowableWeapon', 'ThrownWeapon', 'thrown weapon', 'rename', ''),
    ('Tree', 'TreeObject', 'tree', 'rename', ''),
    ('Triffid', 'SpitterPlant', 'Spitter Plant', 'rename', ''),
    ('TriffidEgg', 'SeedPod', 'Seed Pod', 'rename', ''),
    ('TrunkPort', 'TransitGate', 'Transit Gate', 'rename', PENDING),
    ('TurretEmptyShell', 'ShellCasing', 'shell casing', 'rename', ''),
    ('TurretShell', 'TurretRound', 'turret round', 'rename', ''),
    ('Virii', 'Crawler', 'Crawler', 'rename', ''),
    ('ViriiHistory', 'CrawlerTrailSample', 'Crawler trail sample', 'rename', ''),
    ('ViriiUnit', 'CrawlerGroup', 'Crawler group', 'rename', ''),
    ('Wall', 'WallObject', 'wall', 'rename', ''),
    ('WallBuster', 'BreachCharge', 'Breach Charge', 'rename', ''),
    ('Zombie', 'Wisp', 'Wisp', 'rename', ''),
    ('Portal', 'Rift', 'Rift', 'drop', 'debug only'),
    ('Tentacle', 'RiftTentacle', 'Rift tentacle', 'drop', 'debug only'),
    ('Tank', '', '', 'drop', 'unit of the dropped duel mode'),
    ('Incubator', '', '', 'drop', 'campaign and developer maps only'),
    ('CloneLab', '', '', 'drop', 'unshipped prototype maps'),
    ('ControlTower', '', '', 'drop', 'campaign and developer maps only'),
    ('GunTurretTarget', '', '', 'drop', 'single-player only'),
    ('OfficerOrders', '', '', 'drop', 'single-player only'),
    ('Missile', '', '', 'drop', 'no live caller'),
    ('LandMine', '', '', 'drop', 'no live caller'),
    ('Bridge', '', '', 'drop', 'unreachable'),
    ('ChessBase', '', '', 'drop', 'prototype mode'),
    ('ChessPiece', '', '', 'drop', 'prototype mode'),
]
LEGACY_CLASSES = ('AISpawnPoint BlueprintBuilding BlueprintConsole BlueprintRelay BlueprintStore BoxKite Cave ConstructionYard '
                  'ControllerGrenade DisplayScreen DropShip DynamicBase DynamicHub DynamicNode Factory FeedingTube FenceSwitch '
                  'Generator GodDish Lander LaserTrooper Library Mine MineBuilding MineCart Powerstation PrimaryUpgradePort '
                  'PylonStart ReceiverBuilding ReceiverLink ReceiverSpiritSpawner Refinery ResearchItem SafeArea ScriptTrigger '
                  'Shaman Spam SpiritProcessor SpiritReceiver SpiritStore TrackEnd TrackJunction TrackLink TrackStart Tripod '
                  'TripodNavData UnprocessedSpirit UpgradePort Sepulveda EffectProcessor').split()
for o, ident, term, rule, note in TYPES:
    add(o, 'identifier', v(term, ident, '', rule, note))
for o in LEGACY_CLASSES:
    add(o, 'identifier', v('', '', '', 'drop', 'campaign, cut or unreachable content'))

# simulation constants and random-number functions
for o, ident, term, note in [
    ('NUM_TEAMS', 'kMaxTeams', 'team slots per match', 'tuning key ref.sim.max_teams'),
    ('NUM_SLICES_PER_FRAME', 'kUpdateSlicesPerTick', 'update slices per tick', 'tuning key ref.sim.update_slices'),
    ('SERVER_ADVANCE_PERIOD', 'kTickPeriod', 'tick period', 'tuning key ref.sim.tick_period'),
    ('GRACE_TIME', 'GracePeriod', 'grace period', 'tuning key ref.match.grace_period'),
    ('syncrand', 'SimRng::NextUint', 'simulation random integer', ''),
    ('syncfrand', 'SimRng::NextUnit', 'simulation random fraction', ''),
    ('syncsfrand', 'SimRng::NextSigned', 'simulation random signed fraction', ''),
    ('frand', 'FxRng::NextUnit', 'presentation random fraction', 'never feeds the simulation'),
    ('sfrand', 'FxRng::NextSigned', 'presentation random signed fraction', 'never feeds the simulation'),
    ('AppRandom', 'FxRng::NextUint', 'presentation random integer', 'never feeds the simulation'),
    ('AppSeedRandom', 'FxRng::Seed', 'presentation generator seed', ''),
    ('OrderGoto', 'LeaderOrder::Waypoint', 'waypoint order', ''),
    ('APP_NAME', 'kProductName', 'product name', 'value Species'),
    ('<GAME_TITLE>', 'Species', 'Species', '90 placeholder resolved by the game name'),
]:
    add(o, 'identifier', v(term, ident, '', 'rename', note))
add('<STUDIO>', 'brand', concept('studio', note='placeholder kept: the studio name is not decided'))
add('phrase-id families (multiwinia_*, crate_*, cratename_*, dialog_*, editor_*, taskmanager_*, buildingname_*, entityname_* and the rest of ORIG/41 IP28)',
    'text', v('Species string ids', '', '', 'rename', 'dotted ids <area>.<object>.<item>; no reference phrase id is reused'))


def load_glossary(path):
    with open(path, encoding='utf-8', newline='') as fh:
        return list(csv.DictReader(fh, delimiter='\t'))


def resolve(match, candidates):
    if match.startswith('='):
        hits = [c for c in candidates if c == match[1:]]
    else:
        hits = [c for c in candidates if c == match] or [c for c in candidates if c.startswith(match)]
    if len(hits) != 1:
        raise SystemExit('ledger: match %r resolves to %d rows: %r' % (match, len(hits), hits[:4]))
    return hits[0]


def main():
    gpath, idpath, out, report = sys.argv[1:5]
    g = load_glossary(gpath)
    g3 = [r['original'] for r in g if r['section'] != '2']
    g2 = [r['original'] for r in g if r['section'] == '2']
    rows, seen = [], {}
    used3, used2 = set(), set()

    def emit(orig, kind, val, source):
        if kind not in KINDS:
            raise SystemExit('bad kind %r for %r' % (kind, orig))
        if val['rule'] not in RULES:
            raise SystemExit('bad rule %r for %r' % (val['rule'], orig))
        key = (orig, kind)
        if key in seen:
            prev = seen[key]
            if (prev['species_identifier'], prev['rule']) != (val['species_identifier'], val['rule']):
                raise SystemExit('conflicting rows for %r: %r vs %r' % (orig, prev, val))
            return
        row = {'original': orig, 'kind': kind, **val}
        row['note'] = (val['note'] + ('; ' if val['note'] else '') + source).strip('; ')
        seen[key] = row
        rows.append(row)

    for match, kind, val in G3:
        o = resolve(match, g3)
        used3.add(o)
        emit(o, kind, val, '90 §3')
    for match, kind, val in G2:
        o = resolve(match, g2)
        used2.add(o)
        emit(o, kind, val, '90 §2')
    for o, kind, val in X:
        emit(o, kind, val, '')
    miss3 = [o for o in g3 if o not in used3]
    miss2 = [o for o in g2 if o not in used2]
    if miss3 or miss2:
        raise SystemExit('missing 90 rows: §3 %r; §2 %r' % (miss3, miss2))

    # identifier collision check against the original identifiers
    orig_ids = {}
    with open(idpath, encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh, delimiter='\t'):
            orig_ids[r['identifier']] = r['sources']
    placeholders_seen_in_orig = {'ControlZone', 'OrderMarker'}  # 90 §3.7 placeholders also proposed in ORIG/21
    collisions = []
    for r in rows:
        ident = re.sub(r'<[^>]*>', 'X', r['species_identifier'])
        ident = re.sub(r'\.[a-z]+\b', '', ident)
        for tok in re.findall(r'[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_][A-Za-z0-9_]*)*', ident):
            for t in {tok, tok.split('::')[0]}:
                if t in orig_ids and t not in placeholders_seen_in_orig and r['rule'] != 'drop':
                    collisions.append((t, r['original'], orig_ids[t]))

    cols = ['original', 'kind', 'species_term', 'species_identifier', 'string_id_prefix', 'rule', 'note']
    with open(out, 'w', encoding='utf-8', newline='') as fh:
        wr = csv.DictWriter(fh, fieldnames=cols, delimiter='\t', lineterminator='\n', extrasaction='ignore')
        wr.writeheader()
        wr.writerows(rows)
    with open(report, 'w', encoding='utf-8', newline='') as fh:
        fh.write('rows %d; 90 §3 rows covered %d/%d; 90 §2 IP rows covered %d/%d\n' % (len(rows), len(used3), len(g3), len(used2), len(g2)))
        by_rule = {}
        for r in rows:
            by_rule[r['rule']] = by_rule.get(r['rule'], 0) + 1
        fh.write('by rule: %s\n' % by_rule)
        fh.write('species identifiers equal to an original identifier: %d\n' % len(collisions))
        for c in collisions:
            fh.write('  %s (row %r, ORIG sources %s)\n' % c)
    print(open(report, encoding='utf-8').read())
    if collisions:
        sys.exit(1)


if __name__ == '__main__':
    main()
