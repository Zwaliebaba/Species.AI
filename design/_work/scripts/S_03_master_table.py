"""S03: build the master Building-type table (markdown) for 03_buildings_and_world_objects.md.
Usage: python3 -I S_03_master_table.py <SRC multiwinia root> <census.tsv from S_03_building_census.py>
Reads code/worldobject/building.h (enum order), building.cpp (name table, blocked list, forced-global list,
factory switch) and joins the level census. Section anchors / live status are supplied by the table below.
"""
import sys, os, re

# enum -> (MW status, section number, heading text used in 03 for the anchor)
LIVE = {
 'TypeInvalid': ('n/a', '', ''),
 'TypeFactory': ('LEGACY', '12.10', 'Factory'),
 'TypeCave': ('LEGACY', '12.16', 'Cave'),
 'TypeRadarDish': ('LIVE-L', '5.3', 'RadarDish'),
 'TypeLaserFence': ('LIVE-L', '7.1', 'LaserFence'),
 'TypeControlTower': ('UNUSED', '11.1', 'ControlTower'),
 'TypeGunTurret': ('LIVE-L+R', '5.1', 'GunTurret'),
 'TypeBridge': ('LEGACY', '12.15', 'Bridge'),
 'TypePowerstation': ('LEGACY', '6.8', 'Powerstation'),
 'TypeTree': ('LIVE-L+R', '7.3', 'Tree'),
 'TypeWall': ('LIVE-L', '7.2', 'Wall'),
 'TypeTrunkPort': ('LIVE-L', '8.6', 'TrunkPort'),
 'TypeResearchItem': ('LEGACY', '12.26', 'ResearchItem'),
 'TypeLibrary': ('LEGACY', '12.25', 'Library'),
 'TypeGenerator': ('LEGACY', '6.6', 'Generator'),
 'TypePylon': ('LIVE-L', '6.3', 'Pylon'),
 'TypePylonStart': ('LEGACY', '6.7', 'PylonStart'),
 'TypePylonEnd': ('LIVE-L', '6.4', 'PylonEnd'),
 'TypeSolarPanel': ('LIVE-L', '6.2', 'SolarPanel'),
 'TypePowerSplitter': ('LIVE-L', '6.5', 'PowerSplitter'),
 'TypeTrackLink': ('LEGACY', '12.29', 'TrackLink'),
 'TypeTrackJunction': ('LEGACY', '12.30', 'TrackJunction'),
 'TypeTrackStart': ('LEGACY', '12.31', 'TrackStart'),
 'TypeTrackEnd': ('LEGACY', '12.32', 'TrackEnd'),
 'TypeRefinery': ('LEGACY', '12.33', 'Refinery'),
 'TypeMine': ('LEGACY', '12.34', 'Mine'),
 'TypeYard': ('LEGACY', '12.1', 'ConstructionYard'),
 'TypeDisplayScreen': ('LEGACY', '12.2', 'DisplayScreen'),
 'TypeUpgradePort': ('LEGACY', '12.38', 'UpgradePort'),
 'TypePrimaryUpgradePort': ('LEGACY', '12.39', 'PrimaryUpgradePort'),
 'TypeIncubator': ('UNUSED', '11.2', 'Incubator'),
 'TypeAntHill': ('LIVE-R', '9.1', 'AntHill'),
 'TypeSafeArea': ('LEGACY', '12.35', 'SafeArea'),
 'TypeTriffid': ('LIVE-R', '9.2', 'Triffid'),
 'TypeSpiritReceiver': ('LEGACY', '12.7', 'SpiritReceiver'),
 'TypeReceiverLink': ('LEGACY', '12.5', 'ReceiverLink'),
 'TypeReceiverSpiritSpawner': ('LEGACY', '12.6', 'ReceiverSpiritSpawner'),
 'TypeSpiritProcessor': ('LEGACY', '12.4', 'SpiritProcessor'),
 'TypeSpawnPoint': ('LIVE-L', '4.5', 'SpawnPoint'),
 'TypeSpawnPopulationLock': ('LIVE-L (inert if PopulationCap set)', '4.6', 'SpawnPopulationLock'),
 'TypeSpawnPointMaster': ('LIVE-L', '4.4', 'MasterSpawnPoint'),
 'TypeSpawnLink': ('LIVE-L', '4.3', 'SpawnLink'),
 'TypeAITarget': ('LIVE-L+R', '10.1', 'AITarget'),
 'TypeAISpawnPoint': ('LEGACY', '10.4', 'AISpawnPoint'),
 'TypeBlueprintStore': ('LEGACY', '12.12', 'BlueprintStore'),
 'TypeBlueprintConsole': ('LEGACY', '12.13', 'BlueprintConsole'),
 'TypeBlueprintRelay': ('LEGACY', '12.14', 'BlueprintRelay'),
 'TypeScriptTrigger': ('LEGACY', '12.36', 'ScriptTrigger'),
 'TypeSpam': ('LEGACY', '12.24', 'Spam'),
 'TypeGodDish': ('LEGACY', '12.23', 'GodDish'),
 'TypeStaticShape': ('LIVE-L', '7.4', 'StaticShape'),
 'TypeFuelGenerator': ('LIVE-L', '8.9', 'FuelGenerator'),
 'TypeFuelPipe': ('LIVE-L', '8.10', 'FuelPipe'),
 'TypeFuelStation': ('LIVE-L', '8.11', 'FuelStation'),
 'TypeEscapeRocket': ('LIVE-L', '8.12', 'EscapeRocket'),
 'TypeFenceSwitch': ('LEGACY', '12.37', 'FenceSwitch'),
 'TypeDynamicHub': ('LEGACY', '12.21', 'DynamicHub'),
 'TypeDynamicNode': ('LEGACY', '12.22', 'DynamicNode'),
 'TypeFeedingTube': ('LEGACY', '12.19', 'FeedingTube'),
 'TypeMultiwiniaZone': ('LIVE-L', '8.1', 'MultiwiniaZone'),
 'TypeChessPiece': ('LEGACY', '12.18', 'ChessPiece'),
 'TypeChessBase': ('LEGACY', '12.17', 'ChessBase'),
 'TypeCloneLab': ('UNUSED', '11.3', 'CloneLab'),
 'TypeControlStation': ('LIVE-L', '5.4', 'ControlStation'),
 'TypePortal': ('DEBUG', '9.3', 'Portal'),
 'TypeCrate': ('LIVE-R', '', 'Crate'),
 'TypeStatue': ('LIVE-R', '8.4', 'Statue'),
 'TypeWallBuster': ('LIVE-L', '8.5', 'WallBuster'),
 'TypePulseBomb': ('LIVE-L', '8.7', 'PulseBomb'),
 'TypeRestrictionZone': ('LIVE-L', '7.5', 'RestrictionZone'),
 'TypeJumpPad': ('LIVE-L', '5.5', 'JumpPad'),
 'TypeAIObjective': ('LIVE-L', '10.2', 'AIObjective'),
 'TypeAIObjectiveMarker': ('LIVE-L', '10.3', 'AIObjectiveMarker'),
 'TypeEruptionMarker': ('LIVE-L', '7.7', 'EruptionMarker'),
 'TypeSmokeMarker': ('LIVE-L', '7.6', 'SmokeMarker'),
}

def link(e):
    st, sec, head = LIVE[e]
    if e == 'TypeCrate':
        return '[04](04_crates_powerups_and_weapons.md)'
    if not sec: return '–'
    anchor = (sec.replace('.', '') + '-' + head.lower())
    return f'[§{sec}](#{anchor})'


def main():
    src, census = sys.argv[1], sys.argv[2]
    h = open(os.path.join(src, 'code/worldobject/building.h'), encoding='latin-1').read()
    body = h[h.index('TypeInvalid'):h.index('NumBuildingTypes')]
    enum = re.findall(r'\b(Type\w+)', body)
    c = open(os.path.join(src, 'code/worldobject/building.cpp'), encoding='latin-1').read()
    names_blk = c[c.index('static char *buildingNames[]'):]
    names_blk = names_blk[:names_blk.index('};')]
    names = re.findall(r'"([^"]+)"', names_blk)
    blk = c[c.index('bool Building::BuildingIsBlocked'):]
    blk = blk[:blk.index('return false;')]
    blocked = set(re.findall(r'Building::(Type\w+)', blk))
    fac = c[c.index('Building *Building::CreateBuilding( int _type )'):]
    fac = fac[:fac.index('return building;')]
    classes = dict(re.findall(r'case (Type\w+)\s*:\s*building = new (\w+)', fac))
    glob = fac[fac.index('if( _type == TypeRadarDish'):]
    glob = set(re.findall(r'_type == (Type\w+)', glob[:glob.index('m_isGlobal = true')]))
    cen = {}
    for ln in open(census, encoding='utf-8'):
        if ln.startswith('#') or ln.startswith('type\t'): continue
        p = ln.rstrip('\n').split('\t')
        cen[p[0]] = p
    print('| # | Enum | Level-file name | Class | Blocked | Forced global | Official MP maps (lines) | Dev mp_ maps | Campaign / old maps | Modes in official maps (maps/total) | MW status | Section |')
    print('|---|---|---|---|---|---|---|---|---|---|---|---|')
    for i, (e, n) in enumerate(zip(enum, names)):
        r = cen.get(n.lower())
        if r:
            off = f"{r[1]} ({r[2]})"; dev = r[3]; camp = f"{r[5]} / {r[7]}"; modes = r[8] or '–'
        else:
            off, dev, camp, modes = '0 (0)', '0', '0 / 0', '–'
        print(f"| {i} | `{e}` | `{n}` | {('`'+classes[e]+'`') if e in classes else '— (NULL)'} | {'**yes**' if e in blocked else 'no'} | {'yes' if e in glob else ''} | {off} | {dev} | {camp} | {modes} | {LIVE[e][0]} | {link(e)} |")
    print(f"| {len(enum)} | `NumBuildingTypes` | — | — | | | | | | | | |")
    print(f"<!-- enum={len(enum)} names={len(names)} blocked={len(blocked)} global={len(glob)} -->")

if __name__ == '__main__':
    main()
