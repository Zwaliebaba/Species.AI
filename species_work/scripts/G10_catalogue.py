"""G10: generate the Species sound-event catalogue tables from the reference
sound-event CSV, using Species source ids, event ids and asset ids.

Usage:
  python3 -I G10_catalogue.py <X2b_sound_events.csv> <asset_manifest.csv> <out_md> <out_report>

The mapping tables below translate reference names into Species names. They
live only in this run script (species_work/), never in the design set.
"""
import csv
import re
import sys
from collections import OrderedDict, Counter

# ---------------------------------------------------------------------------
# Sources: (reference group, reference type) -> (species source, kind, title)
# kind: unit | creature | weapon | structure | crate | ui | ambience
SOURCES = OrderedDict([
    (('ENTITY', 'Darwinian'), ('worker', 'unit', 'Worker')),
    (('ENTITY', 'Officer'), ('leader', 'unit', 'Leader')),
    (('ENTITY', 'Squadie'), ('trooper', 'unit', 'Trooper')),
    (('ENTITY', 'Armour'), ('transport', 'unit', 'Transport')),
    (('ENTITY', 'Harvester'), ('collector', 'unit', 'Collector')),
    (('ENTITY', 'Engineer'), ('builder', 'unit', 'Builder')),
    (('ENTITY', 'SpaceInvader'), ('bomber', 'unit', 'Bomber')),
    (('OTHER', 'Spirit'), ('essence_orb', 'unit', 'Essence orb')),
    (('ENTITY', 'Virii'), ('crawler', 'creature', 'Crawler')),
    (('ENTITY', 'Egg'), ('crawler_egg', 'creature', 'Crawler egg')),
    (('ENTITY', 'SporeGenerator'), ('brood_drifter', 'creature', 'Brood Drifter')),
    (('ENTITY', 'Centipede'), ('centipede', 'creature', 'centipede')),
    (('ENTITY', 'Spider'), ('spider', 'creature', 'spider')),
    (('ENTITY', 'ArmyAnt'), ('ant', 'creature', 'ant')),
    (('ENTITY', 'SoulDestroyer'), ('reaper_serpent', 'creature', 'Reaper Serpent')),
    (('ENTITY', 'TriffidEgg'), ('seed_pod', 'creature', 'Seed Pod')),
    (('ENTITY', 'SpaceShip'), ('abductor', 'creature', 'Abductor')),
    (('ENTITY', 'Nuke'), ('barrage_missile', 'weapon', 'barrage missile')),
    (('ENTITY', 'Meteor'), ('meteor', 'weapon', 'meteor')),
    (('OTHER', 'Laser'), ('laser_bolt', 'weapon', 'laser bolt')),
    (('OTHER', 'Grenade'), ('grenade', 'weapon', 'grenade')),
    (('OTHER', 'Rocket'), ('rocket', 'weapon', 'rocket')),
    (('OTHER', 'AirStrikeBomb'), ('airstrike_bomb', 'weapon', 'airstrike bomb')),
    (('BUILDING', 'SpawnPoint'), ('spawn_node', 'structure', 'Spawn Node')),
    (('BUILDING', 'TrunkPort'), ('transit_gate', 'structure', 'Transit Gate')),
    (('BUILDING', 'RadarDish'), ('beam_dish', 'structure', 'Beam Dish')),
    (('BUILDING', 'SolarPanel'), ('solar_array', 'structure', 'Solar Array')),
    (('BUILDING', 'Pylon'), ('grid_pylon', 'structure', 'grid pylon')),
    (('BUILDING', 'FuelGenerator'), ('fuel_pump', 'structure', 'fuel pump')),
    (('BUILDING', 'FuelPipe'), ('fuel_line', 'structure', 'fuel line')),
    (('BUILDING', 'FuelStation'), ('fuel_depot', 'structure', 'fuel depot')),
    (('BUILDING', 'EscapeRocket'), ('launch_vessel', 'structure', 'Launch Vessel')),
    (('BUILDING', 'MultiwiniaZone'), ('control_zone', 'structure', 'Control Zone')),
    (('BUILDING', 'Statue'), ('relic', 'structure', 'Relic')),
    (('BUILDING', 'ControlStation'), ('command_console', 'structure', 'Command Console')),
    (('BUILDING', 'PulseBomb'), ('core_device', 'structure', 'Core Device')),
    (('BUILDING', 'GunTurret'), ('turret', 'structure', 'turret')),
    (('BUILDING', 'LaserFence'), ('barrier_emitter', 'structure', 'Barrier Emitter')),
    (('BUILDING', 'Tree'), ('tree', 'structure', 'tree')),
    (('BUILDING', 'AntHill'), ('hive', 'structure', 'Hive')),
    (('BUILDING', 'Triffid'), ('spitter_plant', 'structure', 'Spitter Plant')),
    (('BUILDING', 'Crate'), ('supply_crate', 'crate', 'Supply crate')),
    (('OTHER', 'CratePowerup'), ('crate_outcome', 'crate', 'Supply crate outcome')),
    (('OTHER', 'MultiwiniaInterface'), ('match_ui', 'ui', 'match UI')),
    (('OTHER', 'Interface'), ('power_slots', 'ui', 'Power slots panel')),
    (('OTHER', 'Gesture'), ('power_input', 'ui', 'Power input')),
    (('OTHER', 'Ambience'), ('ambience', 'ambience', 'ambience')),
])

# Events: (reference type, reference event) -> (species event id, trigger text)
# A trigger text names the game moment in Species terms.
EV = {
    # Worker
    ('Darwinian', 'Create'): ('spawn', 'a Worker appears (spawn hook)'),
    ('Darwinian', 'Attack'): ('attack', 'a laser bolt or a Convert laser bolt is fired'),
    ('Darwinian', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Darwinian', 'Die'): ('die', 'death'),
    ('Darwinian', 'Selected'): ('selected', 'crowd selection or a click selection, local team only'),
    ('Darwinian', 'GivenOrders'): ('ordered', 'a Leader signpost hands it an order'),
    ('Darwinian', 'GivenOrdersFromPlayer'): ('ordered_by_player', 'a crowd order; also alternating with `worker.lift_relic` while carrying'),
    ('Darwinian', 'SeenThreatAttack'): ('attack_bark', 'a brave reaction to a new threat'),
    ('Darwinian', 'SeenThreatAttackHorn'): ('attack_horn', 'a rare brave reaction (R-AUD-317)'),
    ('Darwinian', 'SeenThreatRunAway'): ('flee_bark', 'a scared reaction: a grenade or airstrike target marker nearby, a burning tree, or enemies'),
    ('Darwinian', 'VictoryJump'): ('victory_jump', 'a victory celebration hop'),
    ('Darwinian', 'OnFire'): ('burning', 'while burning, a presentation chance per tick (R-AUD-316)'),
    ('Darwinian', 'LiftBuilding'): ('lift_relic', 'the carry chant while carrying a carryable structure (R-AUD-318)'),
    ('Darwinian', 'EnterTeleport'): ('enter_gate', 'a successful teleport entry'),
    ('Darwinian', 'ExitTeleport'): ('exit_gate', 'arrival at the far Beam Dish'),
    # Leader
    ('Officer', 'Create'): ('spawn', 'a Leader is created by promotion'),
    ('Officer', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Officer', 'Die'): ('die', 'death'),
    ('Officer', 'EnterTeleport'): ('enter_gate', 'a successful teleport entry'),
    ('Officer', 'ExitTeleport'): ('exit_gate', 'arrival at the far Beam Dish'),
    ('Officer', 'Selected'): ('selected', 'selection by the local team'),
    ('Officer', 'SetOrderGoto'): ('order_waypoint', 'a waypoint order, or the mode cycle reaching the waypoint mode'),
    ('Officer', 'SetOrderFormation'): ('order_formation', 'the Leader forms up'),
    ('Officer', 'SetOrderFollow'): ('order_follow', 'the order becomes Follow'),
    ('Officer', 'SetOrderNone'): ('order_none', 'the order becomes None'),
    # Trooper
    ('Squadie', 'Create'): ('spawn', 'a Trooper appears'),
    ('Squadie', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Squadie', 'Die'): ('die', 'death'),
    ('Squadie', 'EnterTeleport'): ('enter_gate', 'it enters a Beam Dish'),
    ('Squadie', 'ExitTeleport'): ('exit_gate', 'arrival at the far Beam Dish'),
    ('Squadie', 'FireLaser'): ('fire_laser', 'each laser shot'),
    ('Squadie', 'Attack'): ('attack', 'each laser shot, with `trooper.fire_laser`'),
    ('Squadie', 'WeaponReturns'): ('weapon_ready', 'the secondary-weapon cooldown expires'),
    ('Squadie', 'ThrowGrenade'): ('throw_grenade', 'the Trooper throws the Strike team grenade'),
    # Transport
    ('Armour', 'Create'): ('spawn', 'a Transport appears'),
    ('Armour', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Armour', 'Die'): ('die', 'destruction'),
    ('Armour', 'SetOrders'): ('ordered', 'the player orders it to move or deploy'),
    ('Armour', 'Selected'): ('selected', 'the local player selects it'),
    ('Armour', 'LoadDarwinian'): ('load_worker', 'a Worker boards'),
    ('Armour', 'UnloadDarwinian'): ('unload_worker', 'a Worker leaves'),
    # Collector
    ('Harvester', 'Create'): ('spawn', 'a Collector appears'),
    ('Harvester', 'DropSpirit'): ('drop_essence', 'each Essence orb delivered to a Spawn Node'),
    ('Harvester', 'Selected'): ('selected', 'the local player selects it'),
    ('Harvester', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Harvester', 'Die'): ('die', 'destruction'),
    # Builder
    ('Engineer', 'Create'): ('spawn', 'a Builder appears'),
    ('Engineer', 'LoseHealth'): ('hurt', 'a non-laser health loss'),
    ('Engineer', 'Die'): ('die', 'death'),
    ('Engineer', 'Selected'): ('selected', 'the local player selects it'),
    ('Engineer', 'DropSpirit'): ('drop_essence', 'each Essence orb delivered'),
    # Bomber
    ('SpaceInvader', 'Create'): ('spawn', 'a Bomber appears'),
    ('SpaceInvader', 'DropGrenade'): ('drop_bomb', 'it releases a bomb near the target'),
    ('SpaceInvader', 'LoseHealth'): ('hurt', 'any health loss'),
    ('SpaceInvader', 'Die'): ('die', 'destruction'),
    # Essence orb
    ('Spirit', 'PickedUp'): ('collected', 'a Builder attaches the orb'),
    # Crawler
    ('Virii', 'Create'): ('spawn', 'a Crawler hatches'),
    ('Virii', 'Attack'): ('attack', 'a bite on arrival at its target'),
    ('Virii', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Virii', 'ChangeDirection'): ('turn', 'an idle Crawler picks a new wander direction'),
    ('Virii', 'Die'): ('die', 'death'),
    ('Egg', 'Create'): ('spawn', 'an egg is laid'),
    # Brood Drifter
    ('SporeGenerator', 'Create'): ('spawn', 'a Brood Drifter appears'),
    ('SporeGenerator', 'LoseHealth'): ('hurt', 'any health loss'),
    ('SporeGenerator', 'Die'): ('die', 'death'),
    ('SporeGenerator', 'LayEgg'): ('lay_egg', 'it lays a Crawler egg'),
    # centipede
    ('Centipede', 'Create'): ('spawn', 'each segment appears, growth segments included'),
    ('Centipede', 'Attack'): ('attack', 'a segment touches an enemy (per enemy per tick)'),
    ('Centipede', 'LoseHealth'): ('hurt', 'a segment loses health'),
    ('Centipede', 'Die'): ('die', 'a segment dies'),
    ('Centipede', 'Panic'): ('panic', 'it starts to panic'),
    ('Centipede', 'Grow'): ('grow', 'it grows a segment'),
    ('Centipede', 'EnemySighted'): ('enemy_sighted', 'it acquires a target'),
    # spider
    ('Spider', 'Create'): ('spawn', 'a spider appears'),
    ('Spider', 'Attack'): ('attack', 'it starts a pounce (with `spider.pounce`)'),
    ('Spider', 'Pounce'): ('pounce', 'it leaps'),
    ('Spider', 'LayEgg'): ('lay_egg', 'it lays a Crawler egg'),
    ('Spider', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Spider', 'Die'): ('die', 'death'),
    ('Spider', 'FootFall'): ('footstep', 'each foot plants'),
    ('Spider', 'PounceLand'): ('pounce_land', 'it lands after a pounce'),
    # ant
    ('ArmyAnt', 'Create'): ('spawn', 'an ant appears'),
    ('ArmyAnt', 'Attack'): ('attack', 'each bite'),
    ('ArmyAnt', 'LoseHealth'): ('hurt', 'any health loss'),
    ('ArmyAnt', 'Die'): ('die', 'death'),
    # Reaper Serpent
    ('SoulDestroyer', 'Create'): ('spawn', 'a Reaper Serpent appears'),
    ('SoulDestroyer', 'LoseHealth'): ('hurt', 'any health loss'),
    ('SoulDestroyer', 'Die'): ('die', 'death'),
    ('SoulDestroyer', 'Attack'): ('attack', 'it passes through an enemy within its damage range (per victim)'),
    ('SoulDestroyer', 'EnemySighted'): ('enemy_sighted', 'it acquires a target'),
    ('SoulDestroyer', 'Panic'): ('panic', 'it starts to panic'),
    # Seed Pod
    ('TriffidEgg', 'Create'): ('spawn', 'a Seed Pod is launched or appears'),
    ('TriffidEgg', 'Bounce'): ('bounce', 'a ground bounce above the bounce-sound force'),
    ('TriffidEgg', 'BurstOpen'): ('burst_open', 'its burst timer elapses and it releases its kind'),
    ('TriffidEgg', 'Die'): ('die', 'destruction by damage'),
    ('TriffidEgg', 'LoseHealth'): ('hurt', 'any health loss'),
    # Abductor
    ('SpaceShip', 'Create'): ('spawn', 'the Abductor appears'),
    ('SpaceShip', 'TractorBeam'): ('tractor_beam', 'it starts abducting at its target'),
    # barrage missile
    ('Nuke', 'Create'): ('spawn', 'a barrage missile is launched'),
    ('Nuke', 'Explode'): ('explode', 'it reaches its target and detonates'),
    ('Nuke', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Nuke', 'Die'): ('die', 'destruction'),
    # meteor
    ('Meteor', 'Create'): ('spawn', 'a meteor appears'),
    ('Meteor', 'Land'): ('land', 'impact at its target'),
    ('Meteor', 'LoseHealth'): ('hurt', 'any health loss'),
    ('Meteor', 'Die'): ('die', 'destruction'),
    # laser bolt
    ('Laser', 'InjuredEntity'): ('hurt_actor', 'a bolt wounds a Worker without killing it; at the victim, for the shooter\'s team'),
    ('Laser', 'Create'): ('spawn', 'every laser bolt fired'),
    ('Laser', 'HitBuilding'): ('hit_structure', 'a bolt hits a structure'),
    ('Laser', 'KilledEntity'): ('kill', 'a bolt or a turret round kills an actor'),
    # grenade
    ('Grenade', 'Create'): ('spawn', 'a grenade or an airstrike target marker is thrown'),
    ('Grenade', 'Bounce'): ('bounce', 'a bounce off the terrain'),
    ('Grenade', 'Flash'): ('fuse_blink', 'once per whole second since the throw, as the fuse light blinks'),
    ('Grenade', 'Explode'): ('explode', 'detonation'),
    # rocket
    ('Rocket', 'Create'): ('spawn', 'a rocket is launched'),
    ('Rocket', 'Explode'): ('explode', 'the rocket explodes; it stops `rocket.spawn`'),
    # airstrike bomb
    ('AirStrikeBomb', 'Explode'): ('explode', 'an airstrike bomb or a firebomb canister explodes'),
    ('AirStrikeBomb', 'Flash'): ('fuse_blink', 'once per whole second, as the fuse light blinks'),
    ('AirStrikeBomb', 'Bounce'): ('bounce', 'a bounce off the terrain'),
    # Spawn Node
    ('SpawnPoint', 'TurnOn'): ('activate', 'an ownership evaluation gives it an owner'),
    ('SpawnPoint', 'TurnOff'): ('deactivate', 'an ownership evaluation leaves it with no team'),
    # Transit Gate
    ('TrunkPort', 'PowerUp'): ('online', 'the gate comes online'),
    # Beam Dish
    ('RadarDish', 'BeginRotation'): ('rotate_start', 'the dish is aimed; any running rotation sound stops first'),
    ('RadarDish', 'EndRotation'): ('rotate_stop', 'the dish finishes turning; it stops `beam_dish.rotate_start`'),
    ('RadarDish', 'ConnectionEstablished'): ('link_up', 'the dish locks onto a receiving dish'),
    ('RadarDish', 'ConnectionLost'): ('link_lost', 'the link breaks; it stops `beam_dish.link_up`'),
    # Solar Array
    ('SolarPanel', 'TriggerSurge'): ('surge', 'the array emits an energy surge'),
    ('SolarPanel', 'Operate'): ('charging', 'the array enters its operating state; stopped when it leaves it'),
    # grid pylon
    ('Pylon', 'TriggerSurge'): ('surge', 'an energy surge reaches the pylon'),
    # fuel chain
    ('FuelGenerator', 'PumpUp'): ('pump_up', 'the pump arm reaches its upper extreme'),
    ('FuelGenerator', 'PumpDown'): ('pump_down', 'the pump arm reaches its lower extreme'),
    ('FuelPipe', 'PumpFuel'): ('flow', 'the line\'s fuel level rises above the flow-sound level; stopped at or below it'),
    ('FuelStation', 'LoadPassenger'): ('load_passenger', 'a Worker boards the Launch Vessel'),
    # Launch Vessel
    ('EscapeRocket', 'Happy'): ('fuelled', 'the fuelled loop of the vessel\'s sound selection'),
    ('EscapeRocket', 'Unhappy'): ('hungry', 'the needs-fuel loop of the vessel\'s sound selection'),
    ('EscapeRocket', 'Malfunction'): ('malfunction', 'the malfunction loop of the vessel\'s sound selection'),
    ('EscapeRocket', 'Flight'): ('flight', 'the vessel is in flight'),
    ('EscapeRocket', 'EngineBurn'): ('engine_burn', 'the engines burn'),
    ('EscapeRocket', 'Explode'): ('explode', 'the vessel is wrecked'),
    ('EscapeRocket', 'Damage'): ('damaged', 'any damage'),
    ('EscapeRocket', 'Refueling'): ('refuelling', 'the refuelling loop of the vessel\'s sound selection'),
    # Control Zone
    ('MultiwiniaZone', 'Create'): ('spawn', 'initialisation; the presence loop'),
    ('MultiwiniaZone', 'ChangeTeam'): ('owner_changed', 'the owner of the zone or Flag post changes (R-AUD-026)'),
    ('MultiwiniaZone', 'CaptureStatue'): ('relic_scored', 'a Relic is delivered into the zone'),
    # Relic
    ('Statue', 'Create'): ('spawn', 'a Relic appears (not during the grace period)'),
    ('Statue', 'ChangeTeam'): ('owner_changed', 'the Relic\'s owner changes'),
    ('Statue', 'Drop'): ('drop', 'the Relic is put down'),
    ('Statue', 'Lift'): ('lift', 'the Relic is lifted'),
    ('Statue', 'Carry'): ('carry', 'loop while lifted; stopped when it is put down'),
    # Command Console, Core Device
    ('ControlStation', 'Create'): ('spawn', 'initialisation; the presence loop'),
    ('PulseBomb', 'Create'): ('spawn', 'initialisation; the presence loop'),
    # turret
    ('GunTurret', 'Create'): ('spawn', 'initialisation'),
    ('GunTurret', 'FireShell'): ('fire_gun', 'a turret round or a gun-turret Convert bolt'),
    ('GunTurret', 'FireFlame'): ('fire_flame', 'a fireball'),
    ('GunTurret', 'RegisterKill'): ('kill_tick', 'a kill credited to the turret'),
    ('GunTurret', 'TargetSighted'): ('target_sighted', 'a search finds a new target'),
    ('GunTurret', 'FireRocket'): ('fire_rocket', 'a rocket turret fires (the rocket has its own `rocket.spawn`)'),
    # Barrier Emitter
    ('LaserFence', 'Create'): ('spawn', 'initialisation; the presence loop'),
    ('LaserFence', 'Spark'): ('spark', 'a spark while enabled'),
    ('LaserFence', 'Damage'): ('damaged', 'any damage'),
    ('LaserFence', 'Electrocute'): ('electrocute', 'an actor touches an active field'),
    # tree
    ('Tree', 'Create'): ('spawn', 'every tree at initialisation, besides its ambient loop'),
    ('Tree', 'CreateNormal'): ('ambient_normal', 'an ordinary tree\'s presence loop'),
    ('Tree', 'CreateMagic'): ('ambient_essence', 'an essence tree\'s presence loop'),
    ('Tree', 'CreateEvil'): ('ambient_blight', 'a blight tree\'s presence loop'),
    ('Tree', 'Burn'): ('burning', 'loop from ignition; stopped when the fire is out or the tree is removed'),
    # Hive
    ('AntHill', 'Damage'): ('damaged', 'each damage and each health-band crossing'),
    ('AntHill', 'Explode'): ('explode', 'destruction; its Essence orbs are released'),
    # Spitter Plant
    ('Triffid', 'Damage'): ('damaged', 'each damage'),
    ('Triffid', 'Burn'): ('burning', 'loop from ignition when its damage crosses 0'),
    ('Triffid', 'LaunchEgg'): ('launch', 'a Seed Pod launch'),
    # Supply crate
    ('Crate', 'Falling'): ('falling', 'a crate is created in the sky; stopped when it lands'),
    ('Crate', 'Land'): ('land', 'the crate touches the ground'),
    ('Crate', 'BeginCapture'): ('capture_begin', 'a team takes the majority of an ownerless crate'),
    ('Crate', 'Capturing_0pc'): ('capture_progress_00', 'progress bucket 0, fired with `supply_crate.capture_begin`'),
    ('Crate', 'Capturing_20pc'): ('capture_progress_20', 'progress bucket 1'),
    ('Crate', 'Capturing_40pc'): ('capture_progress_40', 'progress bucket 2'),
    ('Crate', 'Capturing_60pc'): ('capture_progress_60', 'progress bucket 3'),
    ('Crate', 'Capturing_80pc'): ('capture_progress_80', 'progress bucket 4'),
    ('Crate', 'Capturing_100pc'): ('capture_progress_100', 'progress bucket 5, the crate opens'),
    ('Crate', 'Captured'): ('captured', 'the crate opens and its outcome runs'),
    ('Crate', 'LostCapture'): ('capture_lost', 'the owner loses the majority and the crate returns to no team'),
    ('Crate', 'Expired'): ('expired', 'an unopened crate expires'),
    # Power slots panel, Power input
    ('Interface', 'MouseOverIcon'): ('icon_hover', 'the Power-slot highlight moves to another slot or tab'),
    ('Interface', 'DeleteTask'): ('discard', 'the player discards a Power from the slots'),
    ('Interface', 'SelectTask'): ('select', 'the player selects a Power by its slot key'),
    ('Interface', 'TaskManagerSelectTask'): ('selection_set', 'a team\'s selection is set'),
    ('Interface', 'TaskManagerDeselectTask'): ('selection_cleared', 'a team\'s selection is cleared'),
    ('Interface', 'SetOrderGoto'): ('match_ui.order_given', 'the player gives a crowd order'),
    ('Gesture', 'GestureBegin'): ('begin', 'the player starts a Power from the Power slots panel'),
    ('Gesture', 'GestureSuccess'): ('confirm', 'a Leader promotion, a Builder placement, or a weapon Power chosen'),
    # ambience
    ('Ambience', 'EnterLocation'): ('match_enter', 'every match start; stopped when the match loop exits'),
    ('Ambience', 'ExitLocation'): ('match_exit', 'after the match loop exits'),
    # match UI
    ('MultiwiniaInterface', 'CountdownTick'): ('countdown_tick', 'the grace countdown and the match clock (R-AUD-303, R-AUD-305)'),
    ('MultiwiniaInterface', 'CountdownZeroTick'): ('countdown_zero', 'the grace countdown reaches 0 (R-AUD-304); time-up (R-AUD-306)'),
    ('MultiwiniaInterface', 'SelectionCircle'): ('selection_circle', 'the player starts growing a selection circle; stopped when it ends'),
    ('MultiwiniaInterface', 'LobbyAmbience'): ('lobby_bed', 'the front end is built; stopped when the front end exits'),
    ('MultiwiniaInterface', 'MenuRollOver'): ('menu_hover', 'focus or the pointer enters a menu control'),
    ('MultiwiniaInterface', 'MenuSelect'): ('menu_select', 'a menu button is activated'),
    ('MultiwiniaInterface', 'MenuCancel'): ('menu_back', 'a back or close button'),
    ('MultiwiniaInterface', 'UnitHighlight'): ('unit_highlight', 'the cursor highlight moves to another object'),
    ('MultiwiniaInterface', 'SelectUnit'): ('unit_select', 'a unit selected by clicking'),
    ('MultiwiniaInterface', 'MouseError'): ('error', 'an invalid click'),
    ('MultiwiniaInterface', 'ToggleUnitState'): ('toggle_state', 'a click on an own selected Transport or Leader'),
    ('MultiwiniaInterface', 'ChatMessage'): ('chat_message', 'a chat line arrives in the lobby or in a match'),
    ('MultiwiniaInterface', 'GameOver'): ('game_over', 'a result is declared (R-AUD-307)'),
    ('MultiwiniaInterface', 'YouWon'): ('victory', 'the declared winner is the local team'),
    ('MultiwiniaInterface', 'YouLost'): ('defeat', 'the declared winner is another team, or a draw'),
}

# Crate outcomes: reference event -> Species outcome id
OUTCOMES = OrderedDict([
    ('Squad', 'strike_team'), ('Harvester', 'collector'), ('Armour', 'transport'),
    ('Darwinians', 'reinforcements'), ('AntNest', 'hive'), ('Eggs', 'brood'),
    ('Airstrike', 'airstrike'), ('MagicalForest', 'essence_grove'),
    ('DarkForest', 'blight_grove'), ('CrateMania', 'supply_storm'),
    ('GunTurret', 'gun_turret'), ('Meteor', 'meteor_shower'), ('HotFeet', 'speed_boost'),
    ('Nuke', 'missile_barrage'), ('Plague', 'sickness'), ('Randomiser', 'allegiance_scrambler'),
    ('PersonalShield', 'shield'), ('Subversion', 'convert'), ('SpawnMania', 'spawn_surge'),
    ('Bitzkreig', 'frenzy'), ('Rocketwinians', 'rocket_troops'), ('Infection', 'swarm'),
    ('Rage', 'fury'), ('Triffids', 'spitter_plants'), ('SpaceShip', 'abductor'),
])
OUTCOME_TRIGGER = {
    'strike_team': 'the Strike team Power is placed',
    'collector': 'the Collector Power is placed',
    'transport': 'the Transport Power is placed',
    'airstrike': 'an airstrike or a Firebomb Run runs',
    'gun_turret': 'a gun turret, rocket turret or flame turret outcome runs',
    'supply_storm': 'the outcome runs; played for no team',
    'spawn_surge': 'the outcome runs; played for no team',
    'frenzy': 'the outcome runs; played for no team',
    'rocket_troops': 'the outcome runs; played for no team',
    'fury': 'the outcome runs; played for no team',
    'spitter_plants': 'the unopened-crate outcome runs; played for no team',
    'abductor': 'the unopened-crate outcome runs; played for no team',
}

# Reference sound names (sample or group) -> Species asset id
ASSET = {
    '0lasers': 'sfx.unit.worker.attack', 'laserreload': 'sfx.unit.worker.attack_b',
    '1anthilldamage': 'sfx.building.hive.damaged', 'airstrikeexplode': 'sfx.weapon.airstrike_bomb.explode',
    'antatack': 'sfx.creature.ant.attack', 'antcreat': 'sfx.creature.ant.presence_loop',
    'antdeath': 'sfx.creature.ant.die', 'antlosehealth': 'sfx.creature.ant.hurt',
    'armourtransform': 'sfx.building.turret.spawn', 'bigmetalgroan': 'sfx.building.fuel_pump.pump_up_b',
    'carrystatue': 'sfx.unit.worker.lift_relic', 'choir1': 'sfx.building.launch_vessel.flight',
    'combatvoxattack': 'sfx.unit.worker.attack_bark', 'combatvoxinjured': 'sfx.weapon.laser_bolt.hit_bark',
    'combatvoxretreat': 'sfx.unit.worker.flee_bark_b', 'darwinrunaway': 'sfx.unit.worker.flee_bark',
    'cratefalling': 'sfx.crate.supply_crate.falling', 'darwiniancreate': 'sfx.unit.worker.spawn',
    'darwiniandie': 'sfx.unit.worker.die', 'laserhitdarwinian': 'sfx.unit.worker.die_b',
    'darwinianlosehealth': 'sfx.unit.worker.hurt', 'darwinnianburncry': 'sfx.unit.worker.burning',
    'darwinseethreatattack': 'sfx.unit.worker.attack_horn', 'dselect': 'sfx.unit.worker.selected',
    'elec_cackle': 'sfx.building.barrier_emitter.spark', 'elecfence@damage': 'sfx.building.barrier_emitter.damaged',
    'engineerdie': 'sfx.unit.builder.die', 'engineerinjury': 'sfx.unit.builder.hurt',
    'enterteleport': 'sfx.unit.trooper.enter_gate', 'exitteleport': 'sfx.unit.trooper.exit_gate',
    'flameturretfire': 'sfx.building.turret.fire_flame', 'gdspeechgroup': 'sfx.building.tree.ambient_blight',
    'highaltitude': 'amb.match_bed_c', 'newsamplegroup8': 'amb.match_bed', 'multilobbyambiences': 'amb.lobby_bed',
    'incubatoraddspirit': 'sfx.unit.collector.drop_essence', 'kickverb': 'sfx.building.fuel_pump.pump_down_b',
    'loadpassenger': 'sfx.building.fuel_depot.load_passenger', 'mouseovericon': 'sfx.ui.power_slots.icon_hover',
    'newsamplegroup1': 'sfx.unit.trooper.presence_loop', 'newsamplegroup11': 'sfx.creature.crawler.turn',
    'newsamplegroup13': 'sfx.creature.spider.die', 'newsamplegroup25': 'sfx.creature.centipede.attack',
    'newsamplegroup26': 'sfx.creature.centipede.presence_loop', 'newsamplegroup27': 'sfx.creature.centipede.die',
    'newsamplegroup28': 'sfx.creature.centipede.hurt', 'newsamplegroup3': 'sfx.weapon.grenade.explode',
    'newsamplegroup4': 'sfx.weapon.grenade.spawn', 'newsamplegroup5': 'sfx.unit.trooper.hurt',
    'newsamplegroup6': 'sfx.weapon.grenade.bounce', 'newsamplegroup7': 'sfx.weapon.rocket.explode',
    'newsamplegroup9': 'sfx.creature.crawler.die', 'officercreate': 'sfx.unit.leader.spawn',
    'officerleftright': 'sfx.unit.leader.order_waypoint', 'officerordergoto': 'sfx.unit.transport.ordered',
    'radarconnect': 'sfx.building.beam_dish.link_up', 'radarlose': 'sfx.building.beam_dish.link_lost',
    'relayhover': 'sfx.building.command_console.presence_loop', 'shiphappy': 'sfx.building.launch_vessel.fuelled',
    'shiphungry': 'sfx.building.launch_vessel.hungry', 'shipscream': 'sfx.building.launch_vessel.damaged',
    'solarpanelenable': 'sfx.building.solar_array.surge', 'soulattack': 'sfx.creature.reaper_serpent.attack',
    'souldestdeath': 'sfx.creature.reaper_serpent.die', 'souldestpain': 'sfx.creature.reaper_serpent.hurt',
    'souldestroycreate': 'sfx.creature.reaper_serpent.presence_loop',
    'soulenemysighted': 'sfx.creature.reaper_serpent.enemy_sighted',
    'spidercreate': 'sfx.creature.spider.presence_loop', 'spiderdeath': 'sfx.creature.spider.hurt',
    'spideregglay': 'sfx.creature.spider.lay_egg', 'spiderfeet': 'sfx.creature.spider.footstep',
    'spiderpounce': 'sfx.creature.spider.pounce', 'sporegeneratordie': 'sfx.creature.brood_drifter.die',
    'sporegengroan': 'sfx.creature.brood_drifter.presence_loop', 'sporegenlosehealt': 'sfx.creature.brood_drifter.hurt',
    'squaddiedeath': 'sfx.unit.trooper.die', 'tankcreate': 'sfx.unit.transport.spawn',
    'tankdeath': 'sfx.unit.transport.die', 'tankloseheath': 'sfx.unit.transport.hurt',
    'theramins': 'sfx.creature.abductor.presence_loop', 'treeatmos': 'sfx.building.tree.ambient_normal',
    'treeonfire': 'sfx.building.tree.burning', 'triffeggbursts': 'sfx.creature.seed_pod.burst_open',
    'triffidburn': 'sfx.building.spitter_plant.burning', 'triffideggbounce': 'sfx.creature.seed_pod.bounce',
    'triffidegglaunc': 'sfx.creature.seed_pod.spawn', 'turretfire': 'sfx.unit.trooper.fire_laser',
    'unithighlight': 'sfx.ui.match_ui.unit_highlight', 'victoryjum': 'sfx.unit.worker.victory_jump',
    'virusattack': 'sfx.creature.crawler.attack', 'viruspain': 'sfx.creature.crawler.hurt',
    'ylonelecsurge': 'sfx.building.grid_pylon.surge',
    # single samples
    'airstrike_drop': 'sfx.unit.bomber.drop_bomb', 'airstrike_hover2': 'sfx.unit.bomber.presence_loop',
    'beep_harsh_malfunction': 'sfx.ui.match_ui.error', 'chat': 'sfx.ui.match_ui.chat_message',
    'countdownbeep3': 'sfx.ui.match_ui.countdown_tick', 'crate_capturemyteam': 'sfx.crate.supply_crate.capture_begin',
    'crate_captureotherteam': 'sfx.crate.supply_crate.capture_begin_c', 'crate_lostcapture': 'sfx.crate.supply_crate.capture_lost',
    'cratecapture_10a': 'sfx.crate.supply_crate.capture_progress_00', 'cratecapture_20a': 'sfx.crate.supply_crate.capture_progress_20',
    'cratecapture_30a': 'sfx.crate.supply_crate.capture_progress_40', 'cratecapture_40a': 'sfx.crate.supply_crate.capture_progress_60',
    'cratecapture_50a': 'sfx.crate.supply_crate.capture_progress_80', 'cratecapturedaccent': 'sfx.crate.supply_crate.captured',
    'crateland1': 'sfx.crate.supply_crate.land', 'cratereward_forest2': 'sfx.building.tree.spawn',
    'deselect1': 'sfx.ui.power_slots.discard', 'droneloop': 'sfx.ui.match_ui.selection_circle',
    'enemy_capture_statue': 'sfx.building.relic.owner_changed', 'enemycapturebugle1': 'sfx.building.control_zone.relic_scored_c',
    'engineburn': 'sfx.building.launch_vessel.engine_burn', 'engineer_hover': 'sfx.unit.builder.presence_loop',
    'engineer_reprog_radar_loop': 'sfx.crate.supply_crate.capture_begin_b', 'engineexplode': 'sfx.building.launch_vessel.explode',
    'fence_hum': 'sfx.building.barrier_emitter.presence_loop', 'filtersweep': 'sfx.ui.power_input.begin',
    'fuelpipe2_loop': 'sfx.building.fuel_line.flow', 'fuelpumpexhale1': 'sfx.building.fuel_pump.pump_up',
    'fuelpumpup1': 'sfx.building.fuel_pump.pump_down', 'gesturesuccess': 'sfx.ui.power_input.confirm',
    'grenade_beep': 'sfx.weapon.grenade.fuse_blink', 'grenade_return3': 'sfx.unit.trooper.weapon_ready',
    'ignitionzap1': 'sfx.building.launch_vessel.engine_burn_b', 'koh_zone_captured': 'sfx.building.control_zone.owner_changed',
    'koh_zone_lost': 'sfx.building.control_zone.owner_changed_b', 'koh_zoneactive': 'sfx.building.control_zone.presence_loop',
    'menuscroll1': 'sfx.ui.match_ui.menu_back', 'menuscroll2': 'sfx.ui.match_ui.menu_select',
    'meteorhitclose1': 'sfx.weapon.meteor.land', 'meteorhitfar1': 'sfx.weapon.meteor.land_b',
    'mouseclick1': 'sfx.ui.match_ui.unit_select', 'nukedetonatefar': 'sfx.weapon.barrage_missile.explode_b',
    'nukedetonatenear': 'sfx.weapon.barrage_missile.explode', 'player_capture_statue': 'sfx.building.control_zone.relic_scored',
    'playercapturebugle5': 'sfx.building.control_zone.relic_scored_b', 'radar_motor_run': 'sfx.building.beam_dish.rotate_start_b',
    'radar_start_turn': 'sfx.building.beam_dish.rotate_start', 'radar_stop_turn': 'sfx.building.beam_dish.rotate_stop',
    'rocket_fire4_run': 'sfx.weapon.rocket.presence_loop', 'rocketlauncher': 'sfx.weapon.rocket.spawn',
    'rocketthrust2loop': 'sfx.weapon.meteor.presence_loop', 'rocketthrustloop': 'sfx.weapon.barrage_missile.presence_loop',
    'rumble2_loop': 'sfx.unit.collector.presence_loop', 'shipheartbeat1': 'sfx.building.launch_vessel.fuelled_b',
    'shipmalfunction1': 'sfx.building.launch_vessel.malfunction', 'solarcharge': 'sfx.building.solar_array.charging',
    'sonar': 'sfx.unit.essence_orb.collected', 'spawnpoint_playerturnsoff2': 'sfx.building.spawn_node.deactivate',
    'spawnpoint_playerturnson2': 'sfx.building.spawn_node.activate', 'statuedrop3': 'sfx.building.relic.drop',
    'statuespawn1': 'sfx.building.relic.spawn', 'tank_hover': 'sfx.unit.transport.presence_loop',
    'tractorbeam2': 'sfx.creature.abductor.tractor_beam', 'ufo_engine1': 'sfx.creature.abductor.presence_loop_c',
    'ufo_engine2': 'sfx.creature.abductor.presence_loop_b', 'white_noise': 'amb.match_bed_b',
}

# Base events of every structure, used where a kind has no own entry
BASE = {'Create': ('spawn', 'initialisation'),
        'ChangeTeam': ('owner_changed', 'the owner changes'),
        'Damage': ('damaged', 'any damage')}
# Reference events that the Species units drop with their feature
DROPPED = {('Darwinian', 'TakenControl'), ('Darwinian', 'EscapedControl'),
           ('Officer', 'SetOrderAbsorb'), ('Crate', 'Capturing_%pc(bug)')}

LINKS = {'HeightAboveGround': 'height', 'Ypos': 'altitude', 'Velocity': 'speed',
         'CameraDistance': 'camera_distance', 'Xpos': 'pos_x', 'Zpos': 'pos_z'}
POS = {'2D': '2d', 'Stat': 'fix', 'Att': 'att'}
LOOP = {'Once': 'once', 'Loop': 'loop', 'LoopADSR': 'note'}
INST = {'MonoRandom': 'share-rnd', 'MonoNearest': 'share-near'}
TEAM = {'MyTeam': 'own', 'Enemy': 'enemy'}
FXNAME = {'Echo': 'echo', 'DSound_Echo': 'stereo echo', 'Gargle': 'gargle',
          'DSound_WavesReverb': 'studio reverb'}

# Effect-use numbering: (reference type, reference event) -> FX id
FXID = {('Virii', 'LoseHealth'): 'FX-01', ('Virii', 'Die'): 'FX-02', ('Squadie', 'Create'): 'FX-03',
        ('Squadie', 'LoseHealth'): 'FX-04', ('SporeGenerator', 'Die'): 'FX-05', ('Centipede', 'Die'): 'FX-06',
        ('Spider', 'Die'): 'FX-07', ('Darwinian', 'LoseHealth'): 'FX-08', ('Darwinian', 'Create'): 'FX-09',
        ('Armour', 'Create'): 'FX-10', ('Harvester', 'DropSpirit'): 'FX-11', ('RadarDish', 'ConnectionLost'): 'FX-12',
        ('Crate', 'Falling'): 'FX-13', ('Grenade', 'Explode'): 'FX-14', ('Ambience', 'EnterLocation'): 'FX-15'}


def db(v):
    x = round(5.0 * (float(v) - 10.0), 2)
    s = '%.2f' % x
    s = s.rstrip('0').rstrip('.')
    if s in ('-0', ''):
        s = '0'
    return s.replace('-', '−')


def num(v, nd=2):
    s = ('%.' + str(nd) + 'f') % float(v)
    if '.' in s:
        s = s.rstrip('0').rstrip('.')
    return s.replace('-', '−')


def parse_spec(spec, is_vol):
    once = spec.endswith('/once')
    if once:
        spec = spec[:-5]
    m = re.match(r'rnd (\S+)\.\.(\S+?)(?: s(\S+))?$', spec)
    conv = db if is_vol else num
    if m:
        out = 'rnd %s to %s' % (conv(m.group(1)), conv(m.group(2)))
        if m.group(3):
            out += ' s%s' % m.group(3)
    else:
        m = re.match(r'(\w+) (\S+)\.\.(\S+)->(\S+)\.\.(\S+?)(?: s(\S+))?$', spec)
        if m:
            out = '%s %s to %s → %s to %s' % (LINKS[m.group(1)], num(m.group(2)), num(m.group(3)),
                                                     conv(m.group(4)), conv(m.group(5)))
            if m.group(6):
                out += ' s%s' % m.group(6)
        else:
            out = conv(spec)
    if once:
        out += ' /note'
    return out


def parse_block(text):
    m = re.match(r'\[(Sample|Group) ([^\]]+)\]\s*(\S+)\s*(.*)', text.strip())
    kind, name, mode, rest = m.groups()
    parts = mode.split('/')
    pos, loop = POS[parts[0]], LOOP[parts[1]]
    inst = team = ''
    for p in parts[2:]:
        if p in INST:
            inst = INST[p]
        if p in TEAM:
            team = TEAM[p]
    fields = {}
    for tok in re.split(r' (?=md\d|v=|f=|a=|s=|r=|ld=|fx=)', ' ' + rest):
        tok = tok.strip()
        if not tok:
            continue
        if tok.startswith('md'):
            fields['md'] = tok[2:]
        else:
            k, v = tok.split('=', 1)
            fields[k] = v
    return {'name': name.lower(), 'pos': pos, 'loop': loop, 'inst': inst, 'team': team, 'fields': fields}


def playback(b):
    toks = [b['pos'], b['loop']]
    if b['inst']:
        toks.append(b['inst'])
    if b['team']:
        toks.append(b['team'])
    return ' · '.join(toks)


def envelope(f):
    out = []
    for k, lab in (('a', 'a'), ('s', 's'), ('r', 'r'), ('ld', 'ld')):
        if k in f:
            out.append('%s %s' % (lab, parse_spec(f[k], False)))
    return '; '.join(out)


SILENT_STATUS = {
    # Resolved bindings of events that the reference leaves silent
    'laser_bolt.spawn': 'bound: new group (DEC-564)',
    'laser_bolt.hit_structure': 'bound: new group (DEC-564)',
    'laser_bolt.kill': 'bound: new group (DEC-564)',
    'trooper.throw_grenade': 'bound: new group (DEC-097)',
    'match_ui.game_over': 'bound: new game-over announcement (DEC-097)',
    'match_ui.victory': 'bound: `music.match.sting_victory`, 2d, announcer bus (DEC-097, DEC-049)',
    'match_ui.defeat': 'bound: `music.match.sting_defeat`, 2d, announcer bus (DEC-097, DEC-049)',
}
DEC565 = {
    'supply_crate.expired', 'turret.target_sighted', 'turret.fire_rocket', 'barrier_emitter.electrocute',
    'relic.lift', 'relic.carry', 'transport.load_worker', 'transport.unload_worker', 'leader.order_follow',
    'leader.order_none', 'spider.pounce_land', 'centipede.grow', 'centipede.panic', 'centipede.enemy_sighted',
    'reaper_serpent.panic', 'spitter_plant.launch', 'transit_gate.online', 'transport.selected',
    'builder.selected', 'collector.selected', 'power_slots.select', 'power_slots.selection_set',
    'power_slots.selection_cleared', 'match_ui.order_given', 'builder.drop_essence', 'collector.hurt',
    'collector.die', 'bomber.hurt', 'bomber.die', 'barrage_missile.hurt', 'barrage_missile.die',
    'meteor.hurt', 'meteor.die', 'ambience.match_exit',
}
# Species no-team layers (DEC-097): (event id) -> layer row values
NOTEAM = {
    'control_zone.owner_changed': ('sfx.building.control_zone.owner_changed_b', 'att \u00b7 once \u00b7 none', '300', '0'),
    'relic.owner_changed': ('sfx.building.relic.owner_changed', '2d \u00b7 once \u00b7 none', '100', '\u221210.15'),
}


def main():
    import json
    src_csv, manifest, out_json, out_rep = sys.argv[1:5]
    rows = list(csv.DictReader(open(src_csv, newline='', encoding='utf-8')))
    man_ids = []
    for r in csv.DictReader(open(manifest, newline='', encoding='utf-8')):
        a = r['asset_id']
        if a.split('.')[0] in ('sfx', 'amb', 'music') and a not in man_ids:
            man_ids.append(a)
    bound_assets = Counter()
    per_source = OrderedDict((k, {'live': [], 'silent': []}) for k in SOURCES)
    unmapped, dropped = [], []
    for r in rows:
        key = (r['group'], r['object_type'])
        if key not in SOURCES or r['fired_in_FULL'] != 'yes':
            continue
        src = SOURCES[key][0]
        if r['object_type'] == 'CratePowerup':
            if r['event'] not in OUTCOMES:
                continue
            sev = OUTCOMES[r['event']]
            trig = OUTCOME_TRIGGER.get(sev, 'the outcome runs')
        else:
            evkey = (r['object_type'], r['event'])
            if evkey in DROPPED:
                dropped.append(evkey)
                continue
            if evkey not in EV and r['group'] == 'BUILDING' and r['event'] in BASE:
                sev, trig = BASE[r['event']]
            elif evkey not in EV:
                unmapped.append(evkey)
                continue
            else:
                sev, trig = EV[evkey]
        full = sev if '.' in sev else '%s.%s' % (src, sev)
        per_source[key]['live' if r['in_data'] == 'yes' else 'silent'].append((full, trig, r))
    out = {'tables': OrderedDict(), 'silent': [], 'summary': []}
    total_layers = 0
    for key, d in per_source.items():
        src, kind, title = SOURCES[key]
        lines = []
        for full, trig, r in d['live']:
            blocks = [parse_block(t) for t in r['blueprint'].split('||')]
            for i, b in enumerate(blocks):
                aid = ASSET.get(b['name'])
                if src == 'crate_outcome':
                    aid = 'sfx.crate.outcome.' + full.split('.', 1)[1]
                if aid is None:
                    unmapped.append(('asset', b['name']))
                    aid = '??'
                bound_assets[aid] += 1
                f = b['fields']
                fx = ''
                if 'fx' in f:
                    fx = '%s %s' % (FXID[(r['object_type'], r['event'])], FXNAME[f['fx']])
                ev = '`%s`' % full if i == 0 else '\u2033 layer %d' % (i + 1)
                lines.append('| %s | %s | `%s` | %s | %s | %s | %s | %s | %s |' % (
                    ev, trig if i == 0 else '', aid, playback(b), num(f.get('md', '100')),
                    parse_spec(f['v'], True) if 'v' in f else '\u221215.0',
                    parse_spec(f['f'], False) if 'f' in f else '',
                    envelope(f), fx))
                total_layers += 1
            if full in NOTEAM:
                a, pb, md, lv = NOTEAM[full]
                lines.append('| \u2033 layer %d | no team (R-AUD-025, DEC-097) | `%s` | %s | %s | %s |  |  |  |' % (
                    len(blocks) + 1, a, pb, md, lv))
                total_layers += 1
        out['tables'][src] = lines
        for full, trig, r in d['silent']:
            st = SILENT_STATUS.get(full) or ('silent (DEC-565)' if full in DEC565 else 'silent')
            out['silent'].append('| `%s` | %s | %s |' % (full, trig, st))
        nb = sum(len(r['blueprint'].split('||')) for _f, _t, r in d['live'])
        nsil = len(d['silent'])
        out['summary'].append([src, title, kind, len(d['live']), nb, nsil])
    json.dump(out, open(out_json, 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
    rep = ['live pairs: %d' % sum(len(d['live']) for d in per_source.values()),
           'silent pairs: %d' % len(out['silent']),
           'layers incl. no-team: %d' % total_layers,
           'unmapped: %s' % unmapped, 'dropped: %s' % dropped,
           'assets with no event: %s' % [a for a in man_ids if a not in bound_assets],
           'bound ids not in manifest: %s' % [a for a in bound_assets if a not in man_ids]]
    open(out_rep, 'w', encoding='utf-8').write('\n'.join(rep) + '\n')


if __name__ == '__main__':
    main()
