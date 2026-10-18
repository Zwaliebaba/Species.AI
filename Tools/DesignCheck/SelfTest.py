#!/usr/bin/env python3
"""SelfTest.py: prove that the hashed scans of CheckDesign.py find what the plain-text lists hold, and nothing else.

Usage:
  python3 Tools/DesignCheck/SelfTest.py <names list> <strings list>

<names list> is the plain-text source of IpDenylist.sha256 and <strings list> that of OriginalStrings.sha256 (one entry
per line). The lists are kept outside the repository; this test runs wherever they are available.

Checks:
  1 sync      the hash files hold exactly the digests of the two lists
  2 names     every line of the names list is found by gate 1 in every context below that applies to it: a sentence,
              upper case, title case, a code span, a table cell, a quoted string, wrapped across a line break (several
              words), inside snake_case and CamelCase identifiers (letters and digits only), and with a plural ending
              and a prefix (one word of six or more characters)
  3 strings   every line of the strings list is found by gate 2 as a quoted string: straight and curly quotes, upper
              case, title case, and with closing punctuation inside the quotes
  4 clean     a sample paragraph made of Species terms gives no hit in gate 1 or gate 2
  5 leak      the files of Tools/DesignCheck/ and Tools/DesignModels/ give no gate 1 hit (node_modules/ and the
              npm-generated package-lock.json, which holds registry URLs and base64 digests only, are not read)
It never prints a list entry: a failure names the check, the context and the line number in the list. Exit status 0
when every check passes, 1 otherwise.
"""
import argparse
import bisect
import importlib.util
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS = os.path.dirname(HERE)

CLEAN_SAMPLE = '''\
In Spawn Control every Worker leaves a Spawn Node, follows the spawn network to a Relay and joins the team at the
Relic Base. A Leader drops a Waypoint beacon, a Builder raises a Solar Array beside a Beam Dish, and a Collector
carries an Essence orb home while a Bomber and a Trooper hold the Control Zone. Crawlers hatch from a Crawler egg
near the Hive, a Wisp drifts over the exclusion zone, and an Abductor, a Brood Drifter and a Reaper Serpent roam
past the Seed Pod and the Spitter Plant. The Strike team boards a Transport for the Launch Vessel in Launch Race;
Flag Chain, Relic Run, Siege and Zone Control use the Comeback Bonus, the Catch-up bonus, the grace period and
sudden death. The Barrier Emitter, the Breach Charge, the Command Console, the Core Device, the fuel depot, the
fuel pump, the grid pylon, the jump pad, the Population Limiter, the Transit Gate and the Relic Spawn Zone are
world objects of the Rogue faction and the Visitor faction alike.

| Key | Value | Unit | Range or notes | Requirement | DEC |
|---|---|---|---|---|---|
| `ref.unit.worker.move_speed` | 4 | wu per tick | 1 to 8 | R-UNIT-001 | DEC-001 |

The engine names follow AGENTS.md: `Species::SpawnNode`, `Engine::SceneTarget`, `MatchMode::ZoneControl`,
`TurretType::Rocket`, `unit.strike_team`, `building.relic_base`, "Ready to start the match", Direct3D 12, DXGI,
glTF, `d3dx12.h`, XAudio2, GameInput, WinPixEventRuntime, MSBuild and the Xbox Accessibility Guidelines.
'''


def load_check_design():
  spec = importlib.util.spec_from_file_location('CheckDesign', os.path.join(HERE, 'CheckDesign.py'))
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)
  return module


def entries(path):
  with open(path, encoding='utf-8') as fh:
    return [(ln, line.rstrip('\n')) for ln, line in enumerate(fh, 1) if line.strip()]


def name_contexts(name):
  """(context, text) pairs that embed one names-list line."""
  words = name.split()
  out = [
    ('sentence', f'The sample text names {name} in a sentence.'),
    ('upper case', f'THE SAMPLE TEXT NAMES {name.upper()} IN A SENTENCE.'),
    ('title case', f'The Sample Text Names {name.title()} In A Sentence.'),
    ('code span', f'Use `{name}`, then stop.'),
    ('table cell', f'| row | {name} | value |'),
    ('quoted', f'The label reads "{name}".'),
  ]
  if len(words) > 1:
    out.append(('wrapped', f'The sample text names {words[0]}\n{" ".join(words[1:])} across a line break.'))
  if re.fullmatch(r'[a-z0-9]+', name):
    out.append(('snake_case identifier', f'Call get_{name}_count() here.'))
    if name[0].isalpha():
      out.append(('CamelCase identifier', f'Call Get{name[0].upper()}{name[1:]}Count() here.'))
  if re.fullmatch(r'[a-z0-9]+', name) and len(name) >= 6:
    out.append(('plural ending', f'Two {name}s and more.'))
    out.append(('prefix', f'The pre{name} case.'))
  return out


def string_contexts(s):
  return [
    ('straight quotes', f'The label reads "{s}" on screen.'),
    ('curly quotes', f'The label reads “{s}” on screen.'),
    ('upper case', f'The label reads "{s.upper()}" on screen.'),
    ('title case', f'The label reads "{s.title()}" on screen.'),
    ('closing punctuation', f'The label reads "{s}!" on screen.'),
  ]


def main(argv=None):
  ap = argparse.ArgumentParser(description='Self-test of the hashed denylists of CheckDesign.py.')
  ap.add_argument('names', help='plain-text names list (source of IpDenylist.sha256)')
  ap.add_argument('strings', help='plain-text strings list (source of OriginalStrings.sha256)')
  args = ap.parse_args(argv)
  cd = load_check_design()
  failures = []

  # 1 sync
  ip_hashes = cd.load_hashes(cd.IP_DENYLIST)
  str_hashes = cd.load_hashes(cd.STRINGS_DENYLIST)
  names = entries(args.names)
  strings = entries(args.strings)
  ip_from_list = {cd.digest(cd.normalise_phrase(n)) for _, n in names}
  str_from_list = {cd.digest(cd.normalise_string(s)) for _, s in strings}
  for label, have, want in (('IpDenylist.sha256', ip_hashes, ip_from_list),
                            ('OriginalStrings.sha256', str_hashes, str_from_list)):
    missing, extra = len(want - have), len(have - want)
    if missing or extra:
      failures.append(f'sync: {label} lacks {missing} digests of its list and holds {extra} digests not in it '
                      '(rebuild with BuildDenylist.py --replace)')
    print(f'1 sync     {label}: {len(have)} digests, {len(want)} from the list, {missing} missing, {extra} extra')

  # 2 names
  denylist = cd.Denylist(ip_hashes)
  n_checks = 0
  for ln, name in names:
    want = cd.digest(cd.normalise_phrase(name))
    for context, text in name_contexts(name):
      n_checks += 1
      if not any(cd.digest(phrase) == want for _, phrase in denylist.scan(text)):
        failures.append(f'names: line {ln} of the names list is not found ({context})')
  print(f'2 names    {len(names)} lines, {n_checks} embeddings checked, '
        f'{sum(1 for f in failures if f.startswith("names"))} not found')

  # 3 strings
  n_checks = 0
  for ln, s in strings:
    want = cd.digest(cd.normalise_string(s))
    for context, text in string_contexts(s):
      n_checks += 1
      if not any(cd.digest(cd.normalise_string(hit)) == want for _, hit in cd.scan_strings(text, str_hashes)):
        failures.append(f'strings: line {ln} of the strings list is not found ({context})')
  print(f'3 strings  {len(strings)} lines, {n_checks} embeddings checked, '
        f'{sum(1 for f in failures if f.startswith("strings"))} not found')

  # 4 clean
  rows = cd.load_pattern_rows(cd.TECH_PATTERNS)
  newlines = [m.start() for m in re.finditer('\n', CLEAN_SAMPLE)]
  low = CLEAN_SAMPLE.lower()

  def line_of(pos):
    return bisect.bisect_right(newlines, pos) + 1

  clean_hits = [('gate 1 denylist', ln) for ln, _ in denylist.scan(CLEAN_SAMPLE)]
  clean_hits += [('gate 2 strings', ln) for ln, _ in cd.scan_strings(CLEAN_SAMPLE, str_hashes)]
  for row in rows:
    clean_hits += [(f'gate {"1" if row.gate == "hard" else "2"} {row.cls}', ln)
                   for ln, _ in row.scan(CLEAN_SAMPLE, low, line_of)]
  for where, ln in clean_hits:
    failures.append(f'clean: the Species sample gives a {where} hit on its line {ln}')
  print(f'4 clean    {len(CLEAN_SAMPLE.split())} words of Species text, {len(clean_hits)} hits')

  # 5 leak
  hard = [r for r in rows if r.gate == 'hard']
  leak_hits = []
  for folder in (HERE, os.path.join(TOOLS, 'DesignModels')):
    if not os.path.isdir(folder):
      continue
    for path in cd.iter_files(folder):
      parts = path.split(os.sep)
      if 'node_modules' in parts or '__pycache__' in parts or parts[-1] == 'package-lock.json':
        continue
      text = cd.read_text(path)
      if text is None:
        continue
      rel = os.path.relpath(path, TOOLS).replace(os.sep, '/')
      nl = [m.start() for m in re.finditer('\n', text)]
      lowt = text.lower()

      def line_in(pos, nl=nl):
        return bisect.bisect_right(nl, pos) + 1

      leak_hits += [(rel, ln) for ln, _ in denylist.scan(text)]
      leak_hits += [(rel, ln) for r in hard for ln, _ in r.scan(text, lowt, line_in)]
  for rel, ln in leak_hits:
    failures.append(f'leak: {rel}:{ln} gives a gate 1 hit')
  print(f'5 leak     Tools/DesignCheck and Tools/DesignModels: {len(leak_hits)} gate 1 hits')

  for f in failures:
    print('FAIL ' + f)
  print('SelfTest: ' + ('PASS' if not failures else f'FAIL ({len(failures)})'))
  return 1 if failures else 0


if __name__ == '__main__':
  sys.exit(main())
