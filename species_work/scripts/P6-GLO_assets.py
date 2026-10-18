"""P6-GLO: mechanical changes to assets/asset_manifest.csv and the matching table cells of assets/GameAssets.md.

Usage: python3 -I P6-GLO_assets.py <design root> [--dry-run]

Each change asserts what it replaces, so the script refuses to write a file that differs from the one it was written for.
"""
import csv
import io
import math
import re
import sys
from decimal import Decimal, ROUND_CEILING, ROUND_FLOOR, ROUND_HALF_UP

root = sys.argv[1]
dry = '--dry-run' in sys.argv
MAN = root + '/assets/asset_manifest.csv'
GA = root + '/assets/GameAssets.md'

# ---------------------------------------------------------------- shared text transforms
TENTH = Decimal('0.1')


def tenth(v, mode):
  return max(Decimal(v).quantize(TENTH, rounding=mode), TENTH)


def duration(lo, hi=None):
  """Target duration rounded to a tenth of a second: a range is widened outwards, a single value rounded."""
  if hi is None or Decimal(lo) == Decimal(hi):
    return f'target duration about {tenth(lo, ROUND_HALF_UP)} s'
  a, b = tenth(lo, ROUND_FLOOR), tenth(hi, ROUND_CEILING)
  if a == b:
    return f'target duration about {a} s'
  return f'target duration about {a}-{b} s'


DUR_RANGE = re.compile(r'reference duration (\d+\.\d+)-(\d+\.\d+) s')
DUR_ONE = re.compile(r'reference duration (\d+\.\d+) s')
CLASSES = [100, 300, 1000, 3000, 10000, 25000]


def tri_class(m):
  n = int(m.group(1).replace(' ', '').replace(',', ''))
  c = next(c for c in CLASSES if n <= c)
  return f'<= {c} triangles budget'


TRI = re.compile(r'<= ([0-9][0-9 ,]*) triangles budget')
PROVENANCE = [
  ('; variant count and durations sized from the unbound reference takes for this moment (R-AUD-142)', None),
  ('; variant count proposed (the reference set holds no take for this moment)', '; variant count proposed (DEC-097)'),
]


def fix_spec(text, asset_id):
  text = re.sub(r'; \d+\.\d+ s reference length', '', text)
  text = DUR_RANGE.sub(lambda m: duration(m.group(1), m.group(2)), text)
  text = DUR_ONE.sub(lambda m: duration(m.group(1)), text)
  text = text.replace('; about 0.5-1.0 s;', '; target duration about 0.5-1.0 s;')
  for old, new in PROVENANCE:
    if old in text:
      if new is None:
        new = '; variant count proposed (DEC-564)' if 'laser_bolt' in asset_id else '; variant count proposed (DEC-097)'
      text = text.replace(old, new)
  text = TRI.sub(tri_class, text)
  return text


# ---------------------------------------------------------------- the manifest
OBJECTIVE_CARDS = ['help.hint.start_here', 'help.hint.spawn_nodes', 'help.hint.reinforcements', 'help.hint.supply_crates',
                   'help.hint.zone_control', 'help.hint.relic_run_capture', 'help.hint.relic_run_lift',
                   'help.hint.siege_core_device', 'help.hint.siege_attack', 'help.hint.siege_defend',
                   'help.hint.siege_spawn_nodes', 'help.hint.siege_breach_charge', 'help.hint.launch_race_01',
                   'help.hint.launch_race_02', 'help.hint.flag_chain_01', 'help.hint.flag_chain_02']
REMOVE = set(OBJECTIVE_CARDS) | {'icon.power.laser', 'icon.power.slots_upgrade', 'icon.hud.slot_discard', 'icon.power.rocket'}
CONSUMERS = {
  'cursor.pointer': ('R-CTRL-375;UI-076;UI-465', 'UI-076;UI-465'),
  'icon.power.grenade': ('R-CTRL-191;R-CTRL-231;R-CTRL-232;R-CTRL-366;R-CTRL-381;RND-378;reference/02_units/strike_team.md;'
                         'reference/04_supply_crates_powers_and_weapons/README.md', 'reference/02_units/strike_team.md'),
  'cursor.select': ('R-CTRL-350;reference/04_supply_crates_powers_and_weapons/README.md', 'RND-366'),
  'cursor.move_target': ('R-CTRL-350', 'RND-366'),
  'cursor.enter': ('R-CTRL-350', 'RND-366'),
  'cursor.select.shadow': ('R-CTRL-350;AST-029;RND-243;RND-366;RND-422', 'AST-029;RND-243;RND-366'),
  'cursor.move_target.shadow': ('R-CTRL-350;AST-029;RND-243;RND-366;RND-422', 'AST-029;RND-243;RND-366'),
  'cursor.enter.shadow': ('R-CTRL-350;AST-029;RND-243;RND-366;RND-422', 'AST-029;RND-243;RND-366'),
}
OUTCOME_TERMS = {
  'strike_team': 'Strike team', 'collector': 'Collector', 'transport': 'Transport', 'reinforcements': 'Reinforcements',
  'hive': 'Hive', 'brood': 'Brood', 'airstrike': 'airstrike', 'essence_grove': 'Essence Grove', 'blight_grove': 'Blight Grove',
  'supply_storm': 'Supply Storm', 'gun_turret': 'gun turret', 'meteor_shower': 'meteor shower', 'speed_boost': 'Speed Boost',
  'missile_barrage': 'Missile Barrage', 'sickness': 'Sickness', 'allegiance_scrambler': 'Allegiance Scrambler', 'shield': 'Shield',
  'convert': 'Convert', 'spawn_surge': 'Spawn Surge', 'frenzy': 'Frenzy', 'rocket_troops': 'Rocket Troops', 'swarm': 'Swarm',
  'fury': 'Fury', 'spitter_plants': 'Spitter Plants', 'abductor': 'Abductor',
}
NEW_STINGS = [('rocket_turret', 'rocket turret'), ('flame_turret', 'flame turret'), ('firebomb_run', 'Firebomb Run'),
              ('builder', 'Builder'), ('time_dilation', 'Time Dilation'), ('time_rush', 'Time Rush')]
ALSO = {
  'gun_turret': '; the rocket turret and flame turret outcomes play it too until DEC-565 binds a sting of their own (R-AUD-139)',
  'airstrike': '; the Firebomb Run outcome plays it too until DEC-565 binds a sting of its own (R-AUD-139)',
}
ROLES = {
  'icon.power.airstrike': ('airstrike outcome and power', 'airstrike and Firebomb Run outcomes and powers'),
  'icon.power.gun_turret': ('gun turret outcome and power', 'gun, rocket and flame turret outcomes and powers'),
  'uitex.difficulty_badge': ('Map difficulty badge (basic, intermediate, advanced) on mode and map buttons',
                             'Mode complexity badge (simple, medium, hard) on the mode buttons'),
  'icon.power.grenade': ('Strike team grenade weapon', 'Strike team grenade weapon'),
}
BADGE_FILES = {'content/textures/ui/ui_difficulty_basic.png': 'content/textures/ui/ui_complexity_simple.png',
               'content/textures/ui/ui_difficulty_intermediate.png': 'content/textures/ui/ui_complexity_medium.png',
               'content/textures/ui/ui_difficulty_advanced.png': 'content/textures/ui/ui_complexity_hard.png'}
MUSIC_OPTION_A = {'music.front_end.theme', 'music.match.bed'}
MUSIC_NOTE = '; authored only if DEC-049 selects option A (R-AUD-407)'


def edit_manifest():
  data = open(MAN, encoding='utf-8', newline='').read()
  assert '\r\n' not in data
  reader = csv.DictReader(io.StringIO(data))
  fields = reader.fieldnames
  rows = list(reader)
  ids = {r['asset_id'] for r in rows}
  assert REMOVE <= ids, REMOVE - ids
  out = []
  for r in rows:
    a = r['asset_id']
    if a in REMOVE:
      continue
    if a in CONSUMERS:
      old, new = CONSUMERS[a]
      assert r['consumers'] == old, (a, r['consumers'])
      r['consumers'] = new
    if a in ROLES:
      old, new = ROLES[a]
      assert r['role'] == old, (a, r['role'])
      r['role'] = new
    if a == 'uitex.difficulty_badge':
      r['species_filename'] = BADGE_FILES[r['species_filename']]
    if a.startswith('sfx.crate.outcome.'):
      key = a.split('.')[-1]
      old = 'Announcement sting when a Supply crate yields ' + key.replace('_', ' ')
      assert r['role'] == old, (a, r['role'])
      r['role'] = 'Announcement sting when a Supply crate yields ' + OUTCOME_TERMS[key] + ALSO.get(key, '')
    if a == 'sfx.building.launch_vessel.fuelled_b':
      old = 'mono, 48 kHz; positional, follows the source; seamless loop;'
      assert r['spec'].startswith(old)
      r['spec'] = r['spec'].replace(old, 'mono, 48 kHz; positional, following the source, as the fuelled layer; 2-D as a layer of '
                                    'the lobby bed `match_ui.lobby_bed`; seamless loop;')
    if a in MUSIC_OPTION_A:
      assert r['priority'] == 'launch'
      r['priority'] = 'post-launch'
      r['role'] += MUSIC_NOTE
    if r['category'] == 'Music':
      parts = r['spec'].split('; ')
      half = len(parts) // 2
      if len(parts) % 2 == 0 and parts[:half] == parts[half:]:
        r['spec'] = '; '.join(parts[:half])
      elif r['asset_id'] == 'music.match.bed' and r['spec'].count('stereo, 48 kHz') == 2:
        r['spec'] = 'stereo, 48 kHz; seamless loop; 3-5 min'
    r['spec'] = fix_spec(r['spec'], a)
    out.append(r)
    if a == 'sfx.crate.outcome.abductor':
      for key, term in NEW_STINGS:
        n = dict(r)
        n['asset_id'] = 'sfx.crate.outcome.' + key
        n['consumers'] = 'R-AUD-139;R-AUD-411'
        n['role'] = f'Announcement sting when a Supply crate yields {term}; bound only if DEC-565 binds it'
        n['spec'] = 'mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-8.2 s; variant count proposed (DEC-565)'
        n['priority'] = 'post-launch'
        n['species_filename'] = f'content/audio/sfx/crate_outcome/outcome_{key}.wav'
        out.append(n)
  assert all('reference length' not in r['spec'] and 'reference duration' not in r['spec'] for r in out)
  assert all('reference take' not in r['spec'] and 'reference set' not in r['spec'] for r in out)
  buf = io.StringIO()
  w = csv.DictWriter(buf, fieldnames=fields, lineterminator='\n')
  w.writeheader()
  w.writerows(out)
  print(f'manifest: {len(rows)} -> {len(out)} rows')
  if not dry:
    open(MAN, 'w', encoding='utf-8', newline='').write(buf.getvalue())


# ---------------------------------------------------------------- GameAssets.md table cells
def edit_game_assets():
  text = open(GA, encoding='utf-8').read()
  lines = text.split('\n')
  out = []
  removed = 0
  for line in lines:
    if line.startswith('| `content/'):
      cells = line.split(' | ')
      ids = re.findall(r'`([a-z]+\.[a-z0-9_.]+)`', cells[1]) if len(cells) > 1 else []
      if ids and ids[0] in REMOVE:
        removed += 1
        continue
      if ids and ids[0] == 'uitex.difficulty_badge':
        for old, new in BADGE_FILES.items():
          line = line.replace('`' + old + '`', '`' + new + '`')
        line = line.replace(ROLES['uitex.difficulty_badge'][0], ROLES['uitex.difficulty_badge'][1])
      if ids and ids[0].startswith('sfx.crate.outcome.'):
        key = ids[0].split('.')[-1]
        old = 'Supply crate yields ' + key.replace('_', ' ') + ' |'
        assert old in line, line
        line = line.replace(old, 'Supply crate yields ' + OUTCOME_TERMS[key] + ALSO.get(key, '').replace('; ', '; ', 1) + ' |')
      if ids and ids[0] == 'sfx.building.launch_vessel.fuelled_b':
        line = line.replace('mono, 48 kHz; positional, follows the source; seamless loop;',
                            'mono, 48 kHz; positional, following the source, as the fuelled layer; 2-D as a layer of the lobby bed; '
                            'seamless loop;')
      if ids and ids[0] in MUSIC_OPTION_A:
        assert '| launch | silence |' in line, line
        line = line.replace('| launch | silence |', '| post-launch | silence |')
      line = fix_spec(line, ids[0] if ids else '')
      if ids and ids[0] == 'sfx.crate.outcome.abductor':
        out.append(line)
        for key, term in NEW_STINGS:
          out.append(f'| `content/audio/sfx/crate_outcome/outcome_{key}.wav` | `sfx.crate.outcome.{key}` | Supply crate yields {term}; '
                     'bound only if DEC-565 binds it (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; '
                     'target duration about 2.0-8.2 s; variant count proposed (DEC-565) | 1 variant | post-launch |')
        continue
    out.append(line)
  new = '\n'.join(out)
  assert 'reference duration' not in new and 'reference length' not in new and 'unbound reference takes' not in new
  print(f'GameAssets: {removed} table rows removed')
  if not dry:
    open(GA, 'w', encoding='utf-8').write(new)


edit_manifest()
edit_game_assets()
