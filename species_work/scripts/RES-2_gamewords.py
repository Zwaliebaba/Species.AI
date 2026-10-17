"""RES-2 helper: print code lines inside `namespace Engine` blocks of sketches that contain game words.

Usage: python3 -I RES-2_gamewords.py <folder> [<words-file>]
"""
import os
import re
import sys

WORDS = ('Worker Leader Crawler Trooper Transport Bomber Builder Collector EssenceOrb Orb StrikeTeam Waypoint '
         'Abductor Brood Centipede Reaper SeedPod Spider Wisp Barrier BeamDish Breach Console ControlZone Turret '
         'CoreDevice EnergyGrid Surge Exclusion Flag Fuel Pylon Hive HomePost JumpPad Vessel Population Relay Relic '
         'Solar SourceNode Spawn Spitter TransitGate Tree Wall MatchMode FlagChain LaunchRace RelicRun Siege '
         'SpawnControl ZoneControl CatchUp Comeback Crate Supply Power Achievement Team Match Bot Faction Rogue '
         'Visitor Actor Structure Lobby Tutorial Explainer Mode Hud Snapshot Sim Mission Airstrike Firebomb Meteor '
         'Lightning Shatter Laser Rocket Missile Grenade Casing Sickness Frenzy Fury Shield Swarm Abduct Cheat '
         'Leaderboard Session Map Route Seasonal Kill Score Objective Unit Creature Building Player Gate Plant '
         'Sea Sky Terrain Cloud Trail Flame Fire Volcano Smoke Bite Boost Promot Demot Order Formation Command').split()
PAT = re.compile(r'\b\w*(' + '|'.join(WORDS) + r')\w*')
NS_OPEN = re.compile(r'^\s*namespace\s+(\w+)')
NS_CLOSE = re.compile(r'^\s*\}\s*//\s*namespace')


def main():
    root = sys.argv[1]
    for r, _, names in os.walk(root):
        for n in sorted(names):
            if not n.endswith('.md'):
                continue
            p = os.path.join(r, n)
            lines = open(p, encoding='utf-8').read().split('\n')
            code = False
            ns = None
            for i, l in enumerate(lines, 1):
                if l.strip().startswith('```'):
                    code = not code
                    ns = None
                    continue
                if not code:
                    continue
                m = NS_OPEN.match(l)
                if m:
                    ns = m.group(1)
                    continue
                if NS_CLOSE.match(l):
                    ns = None
                    continue
                if ns != 'Engine':
                    continue
                c = l.split('//')[0]
                hits = sorted(set(x.group(0) for x in PAT.finditer(c)))
                if hits:
                    print(f'{os.path.relpath(p, root)}:{i}: {" ".join(hits)}')


if __name__ == '__main__':
    main()
