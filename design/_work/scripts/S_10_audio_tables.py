"""S_10: generate the catalogue tables of 10_audio_design.md and verify the drafts' counts.

Usage (always with -I; paths are arguments, data files are treated as data only):
  python3 -I S_10_audio_tables.py <SRC multiwinia root> <OUT/_work> <target .md or ->

* Independently re-parses data/sounds.txt (sources, EVENT blocks, SAMPLEGROUPs, every field)
  and data/effects.txt, prints the statistics used in 10 §2-§3, and cross-checks them against
  _work/drafts/P16_sound_events.csv and _work/drafts/X2b_sound_events.csv.
* Renders, from X2b_sound_events.csv (469 rows = 256 bound pairs + 213 code-only pairs):
    EVENTS   - every object type x event bound in data (256 rows)
    CODEONLY - every object type x event fired by code with no data (213 rows)
    DEADDATA - bound pairs that never sound in FULL (74 rows)
    SOURCES  - per-source summary (68 sources)
    FIELDS   - shipped usage of every instance field
    FX       - every EFFECT use in the shipped data
* If a target .md is given, replaces the text between <!-- S10:<NAME>:BEGIN --> and
  <!-- S10:<NAME>:END --> markers with the rendered table; otherwise prints them.
"""
import sys, os, re, csv, collections

# Enclosing symbols for six call sites the X2b extractor could not name (verified by reading
# the source: class bodies defined inline in the .cpp files).
SYMBOL_FIX = {
    'code/interface/helpandoptions_windows.cpp:?:L33': 'code/interface/helpandoptions_windows.cpp:HelpWindowButton::MouseUp:L33',
    'code/interface/helpandoptions_windows.cpp:?:L50': 'code/interface/helpandoptions_windows.cpp:CrateHelpWindowButton::MouseUp:L50',
    'code/interface/helpandoptions_windows.cpp:?:L69': 'code/interface/helpandoptions_windows.cpp:OptionsButton::MouseUp:L69',
    'code/interface/leaderboards_window.cpp:?:L146': 'code/interface/leaderboards_window.cpp:LeaderboardSmallFontRenderingButton::Render:L146',
    'code/interface/multiwinia_window.cpp:?:L153': 'code/interface/multiwinia_window.cpp:SelectMapButton::MouseUp:L153',
    'code/interface/multiwinia_window.cpp:?:L171': 'code/interface/multiwinia_window.cpp:NewMapWindowButton::MouseUp:L171',
}

# Call sites whose event-name argument is a variable: what the variable holds (verified in code).
VARNAMES = {
    '(char *) _event': 'Music <script event> (Script::RunCommand_TriggerSound)',
    'newSound': 'Capturing_<20..100>pc / Capturing_%pc bug',
    'eventName': 'CratePowerup <Crate::GetName(reward)>',
    'requiredSoundName': 'EscapeRocket Refueling/Unhappy/Malfunction/Happy/Flight',
    '_event': 'Grenade/AirStrikeBomb Create/Bounce/Flash/Explode (ThrowableWeapon::TriggerSoundEvent)',
}

# Corrections to X2b row texts established while writing 10 (re-read in code/multiwinia.cpp:Multiwinia::Render):
# the zero tick for a timed game is nested inside 'if (m_timeRemaining > 0 ...)' and can never fire.
ROW_FIX = {
    ('OTHER', 'MultiwiniaInterface', 'CountdownZeroTick'): {
        'gameplay_trigger': 'Grace period reaches 0 (game starts; Multiwinia::AdvanceGracePeriod). The Multiwinia::Render site meant for a timed game reaching 0 is unreachable: it tests m_timeRemaining == 0 inside the m_timeRemaining > 0 guard (10 §1.6 R6).',
        'code_sites': 'code/multiwinia.cpp:Multiwinia::AdvanceGracePeriod:L2412 [FULL] ; code/multiwinia.cpp:Multiwinia::Render:L3664 [UNREACHABLE(inside the m_timeRemaining > 0 guard)]',
    },
    # Phase-5 fix (R5): code/startsequence.cpp is compiled into FULL (no file gate, listed in
    # targets/vs2005/Darwinia.vcproj); only RunBootloader (inside #ifdef USE_LOADERS) constructs
    # StartSequence, so the trigger is LEGACY (compiled, unreachable), not DEAD.
    ('OTHER', 'Music', 'StartSequence'): {
        'fired_in_FULL': 'no(LEGACY)',
        'gameplay_trigger': 'First-time intro (StartSequence is compiled, but only RunBootloader constructs it, under USE_LOADERS). Unreachable; not shipped.',
        'code_sites': 'code/startsequence.cpp:StartSequence::Advance:L76 [LEGACY(compiled; StartSequence only built in RunBootloader under USE_LOADERS)]',
    },
}

POS = {'0': '2D', '1': 'Stat', '2': 'Att', '3': 'Ed'}
LOOP = {'0': 'Once', '1': 'Loop', '2': 'LoopADSR'}
INST = {'0': 'Poly', '1': 'MonoRandom', '2': 'MonoNearest'}
TEAM = {'0': 'All', '1': 'MyTeam', '2': 'Enemy'}
SRCT = {'0': 'Sample', '1': 'Group'}
PARAMS = ('VOLUME', 'FREQUENCY', 'ATTACK', 'SUSTAIN', 'RELEASE', 'LOOPDELAY')


def parse_sounds(path):
    """Token-level re-implementation of SoundSystem::LoadBlueprints (keywords case-insensitive)."""
    lines = open(path, encoding='latin-1').read().splitlines()
    sources = []          # (group, type, [blocks])
    groups = []           # (name, [samples]) in file order, duplicates kept
    i = 0
    while i < len(lines):
        t = lines[i].split(); i += 1
        if not t or t[0].startswith('#'):
            continue
        k = t[0].upper()
        if k in ('ENTITY', 'BUILDING', 'OTHER'):
            src = (k, t[1], [])
            sources.append(src)
            ev = None; fx = None
            while i < len(lines):
                tt = lines[i].split(); i += 1
                if not tt or tt[0].startswith('#'):
                    continue
                kk = tt[0].upper()
                if fx is not None:
                    if kk == 'END':
                        ev['EFFECTS'].append(fx); fx = None
                    else:
                        fx[1].append(tt)
                    continue
                if ev is not None:
                    if kk == 'END':
                        src[2].append(ev); ev = None
                    elif kk == 'EFFECT':
                        fx = (tt[1], [])
                    elif kk in PARAMS:
                        ev[kk] = tt[2:]     # skip the PARAMETER token
                    else:
                        ev[kk] = tt[1] if len(tt) > 1 else ''
                    continue
                if kk == 'END':
                    break
                if kk == 'EVENT':
                    ev = {'EVENT': tt[1], 'EFFECTS': []}
        elif k == 'SAMPLEGROUP':
            g = (t[1], [])
            while i < len(lines):
                tt = lines[i].split(); i += 1
                if not tt:
                    continue
                if tt[0].upper() == 'END':
                    break
                g[1].append(tt[1].lower().rsplit('.', 1)[0] if len(tt) > 1 else '')
            groups.append(g)
    return sources, groups


def parse_effects(path):
    out = []
    cur = None
    for line in open(path, encoding='latin-1').read().splitlines():
        t = line.split()
        if not t or t[0].startswith('#'):
            continue
        if t[0].upper() == 'EFFECT':
            cur = (t[1], []); out.append(cur)
        elif t[0].upper() == 'END':
            cur = None
        elif cur is not None:
            cur[1].append(t[:5])
    return out


def ptype(p):
    if not p:
        return 'absent'
    return {'TypeFixedValue': 'fixed', 'TypeRangedRandom': 'random', 'TypeLinked': 'linked'}.get(p[0], p[0])


def stats(sources, groups, effects):
    out = collections.OrderedDict()
    blocks = [(g, ty, b) for g, ty, bl in sources for b in bl]
    out['sources'] = collections.Counter(g for g, _, _ in sources)
    out['blocks'] = collections.Counter(g for g, _, _ in blocks)
    out['pairs'] = len({(g, ty, b['EVENT'].lower()) for g, ty, b in blocks})
    for f, m in (('SOURCETYPE', SRCT), ('POSITIONTYPE', POS), ('INSTANCETYPE', INST),
                 ('LOOPTYPE', LOOP), ('TEAMMATCHTYPE', TEAM)):
        out[f] = collections.Counter(m.get(b.get(f, ''), b.get(f, '?')) for _, _, b in blocks)
    md = collections.Counter(b.get('MINDISTANCE') for _, _, b in blocks)
    out['MINDISTANCE'] = md
    vals = [float(x) for x in md.elements()]
    out['MINDISTANCE_range'] = (min(vals), max(vals))
    out['PRIORITY'] = sum(1 for _, _, b in blocks if 'PRIORITY' in b)
    for p in PARAMS:
        out[p] = collections.Counter(ptype(b.get(p)) for _, _, b in blocks)
        out[p + '_once'] = sum(1 for _, _, b in blocks if b.get(p) and b[p][-1] == 'UpdateOncePerLoop')
        out[p + '_noupd'] = sum(1 for _, _, b in blocks if b.get(p) and b[p][-1] not in ('UpdateOncePerLoop', 'UpdateConstantly'))
        links = collections.Counter(b[p][6] for _, _, b in blocks if b.get(p) and b[p][0] == 'TypeLinked')
        out[p + '_links'] = links
        fixed = [float(b[p][1]) for _, _, b in blocks if b.get(p) and b[p][0] == 'TypeFixedValue']
        out[p + '_fixed_range'] = (min(fixed), max(fixed)) if fixed else None
    smooth = 0
    for _, _, b in blocks:
        for p in PARAMS:
            v = b.get(p)
            if v and v[0] == 'TypeRangedRandom' and float(v[3]) > 0: smooth += 1
            if v and v[0] == 'TypeLinked' and float(v[5]) > 0: smooth += 1
    out['smoothed_params'] = smooth
    fxuse = collections.Counter(fx[0] for _, _, b in blocks for fx in b['EFFECTS'])
    out['fx_uses'] = fxuse
    out['fx_blocks'] = sum(1 for _, _, b in blocks if b['EFFECTS'])
    out['groups_defs'] = len(groups)
    out['groups_names'] = len({g[0].lower() for g in groups})
    out['sample_lines'] = sum(len(g[1]) for g in groups)
    first = collections.OrderedDict()
    for n, s in groups:
        first.setdefault(n.lower(), s)
    out['distinct_entries'] = sum(len(s) for s in first.values())
    out['dup_groups'] = [n for n, c in collections.Counter(g[0].lower() for g in groups).items() if c > 1]
    referenced = {b.get('SOUNDNAME', '').lower() for _, _, b in blocks if b.get('SOURCETYPE') == '1'}
    out['unreferenced_groups'] = sorted(n for n in first if n not in referenced)
    out['group_sizes'] = (min(len(s) for s in first.values()), max(len(s) for s in first.values()))
    out['effects'] = [(n, len(p)) for n, p in effects]
    vol = [b['VOLUME'] for _, _, b in blocks if b.get('VOLUME')]
    fx_v = [float(v[1]) for v in vol if v[0] == 'TypeFixedValue']
    out['VOLUME_fixed_mean'] = sum(fx_v) / len(fx_v)
    return out, blocks


RUN = re.compile(r'^(.*?)(\d+)$')


def compress_stems(stems):
    """['laser1','laser2',...,'laser24'] -> 'laser1–24'; keeps non-numbered stems."""
    byp = collections.OrderedDict()
    plain = []
    for s in stems:
        m = RUN.match(s)
        if m and not m.group(2).startswith('0'):
            byp.setdefault(m.group(1), []).append(int(m.group(2)))
        else:
            plain.append(s)
    parts = [((x.lower(), -1), x) for x in plain]
    for p, nums in byp.items():
        nums = sorted(set(nums))
        run = [nums[0], nums[0]]
        for n in nums[1:] + [None]:
            if n is not None and n == run[1] + 1:
                run[1] = n
                continue
            key = (p.lower(), run[0])
            if run[0] == run[1]:
                parts.append((key, '%s%d' % (p, run[0])))
            elif run[1] == run[0] + 1:
                parts.append((key, '%s%d, %s%d' % (p, run[0], p, run[1])))
            else:
                parts.append((key, '%s%d–%d' % (p, run[0], run[1])))
            if n is not None:
                run = [n, n]
    return ', '.join(t for _, t in sorted(parts))


def render_samples(field):
    if not field:
        return '—'
    byf = collections.OrderedDict()
    for p in field.split(';'):
        p = p.strip()
        if not p:
            continue
        folder, name = p.rsplit('/', 1)
        folder = folder.replace('data/', '')
        byf.setdefault(folder, []).append(name.rsplit('.', 1)[0])
    return '<br>'.join('`%s/`{%s}' % (f, compress_stems(s)) for f, s in byf.items())


def split_sites(field):
    """Split on ';' that are not inside [...] (status tags may contain ';')."""
    items, cur, depth = [], '', 0
    for ch in field:
        if ch == '[':
            depth += 1
        elif ch == ']':
            depth = max(0, depth - 1)
        if ch == ';' and depth == 0:
            items.append(cur); cur = ''
        else:
            cur += ch
    items.append(cur)
    return [i.strip() for i in items if i.strip()]


def render_sites(field):
    if not field:
        return '—'
    out = []
    for s in split_sites(field):
        tag = ''
        m = re.search(r'\s+\[([^\[\]]*(?:\[[^\]]*\][^\[\]]*)*)\]$', s)
        if m:
            tag = m.group(1).replace(';', ',')
            s = s[:m.start()]
        parts = s.split(' ', 1)
        site = SYMBOL_FIX.get(parts[0], parts[0])
        txt = '`%s`' % site
        if len(parts) > 1:
            rest = parts[1]
            # a second citation joined with ' / ' (Grenade::Advance / FlameGrenade::Advance)
            rest = re.sub(r'^/ (\S+)', lambda mm: '/ `code/worldobject/weapons.cpp:%s`' % mm.group(1), rest)
            txt += ' ' + esc(rest)
        if tag and tag != 'FULL':
            txt += ' [%s]' % esc(tag)
        out.append(txt)
    return '<br>'.join(out)


def esc(s):
    return s.replace('|', '\\|').replace('\n', ' ')


def render_blueprint(bp):
    return '<br>'.join(esc(x.strip()) for x in bp.split(' || ')) if bp else '—'


def table_events(rows):
    h = ['| # | Source | Event | Blueprint (one line per EVENT block) | Samples that can play | Fired in FULL | Where the object exists | Ed | Gameplay trigger | Code trigger site(s) |',
         '|---|---|---|---|---|---|---|---|---|---|']
    for n, r in enumerate(rows, 1):
        h.append('| %d | %s %s | %s | %s | %s (%s) | %s | %s | %s | %s | %s |' % (
            n, r['group'], r['object_type'], r['event'], render_blueprint(r['blueprint']),
            render_samples(r['samples']), r['n_samples'], r['fired_in_FULL'], esc(r['type_reach']),
            r['listed_in_ListSoundEvents'] or '—', esc(r['gameplay_trigger']), render_sites(r['code_sites'])))
    return '\n'.join(h)


def table_codeonly(rows):
    h = ['| # | Source | Event (fired, no data → silent) | Fired in FULL | Where the object exists | Ed | Meaning | Code trigger site(s) |',
         '|---|---|---|---|---|---|---|---|']
    for n, r in enumerate(rows, 1):
        h.append('| %d | %s %s | %s | %s | %s | %s | %s | %s |' % (
            n, r['group'], r['object_type'], esc(r['event']), r['fired_in_FULL'], esc(r['type_reach']),
            r['listed_in_ListSoundEvents'] or '—', esc(r['gameplay_trigger']), render_sites(r['code_sites'])))
    return '\n'.join(h)


def table_deaddata(rows):
    h = ['| # | Source | Event | Why it never sounds in FULL | Files |', '|---|---|---|---|---|']
    n = 0
    for r in rows:
        if r['fired_in_FULL'] == 'yes':
            continue
        n += 1
        h.append('| %d | %s %s | %s | %s — %s | %s |' % (
            n, r['group'], r['object_type'], r['event'], r['fired_in_FULL'], esc(r['type_reach']), r['n_samples']))
    return '\n'.join(h)


def table_sources(sources, rows):
    by = collections.defaultdict(list)
    for r in rows:
        by[(r['group'], r['object_type'])].append(r)
    h = ['| # | Source | EVENT blocks | Bound events | Bound & fired in FULL | Fired with no data | Where the object exists |',
         '|---|---|---|---|---|---|---|']
    for n, (g, ty, bl) in enumerate(sources, 1):
        rr = by.get((g, ty), [])
        bound = [r for r in rr if r['in_data'] == 'yes']
        live = [r for r in bound if r['fired_in_FULL'] == 'yes']
        code = [r for r in rr if r['in_data'] == 'no']
        reach = rr[0]['type_reach'] if rr else ''
        h.append('| %d | %s %s | %d | %d | %d | %d | %s |' % (n, g, ty, len(bl), len(bound), len(live), len(code), esc(reach)))
    return '\n'.join(h)


def fmt_counter(c):
    return ', '.join('%s %d' % (k, v) for k, v in c.most_common())


def table_fields(st):
    rows = [
        ('SOURCETYPE', fmt_counter(st['SOURCETYPE'])),
        ('POSITIONTYPE', fmt_counter(st['POSITIONTYPE'])),
        ('INSTANCETYPE', fmt_counter(st['INSTANCETYPE'])),
        ('LOOPTYPE', fmt_counter(st['LOOPTYPE'])),
        ('TEAMMATCHTYPE', fmt_counter(st['TEAMMATCHTYPE'])),
        ('MINDISTANCE', 'range %.2f–%.2f; 100.00 ×%d, 200.00 ×%d, %d distinct values' % (
            st['MINDISTANCE_range'][0], st['MINDISTANCE_range'][1], st['MINDISTANCE']['100.00'],
            st['MINDISTANCE']['200.00'], len(st['MINDISTANCE']))),
        ('PRIORITY', 'present in %d blocks' % st['PRIORITY']),
    ]
    for p in PARAMS:
        rng = st[p + '_fixed_range']
        rows.append((p, '%s; UpdateOncePerLoop on %d; links: %s%s' % (
            fmt_counter(st[p]), st[p + '_once'], fmt_counter(st[p + '_links']) or 'none',
            ('; fixed values %.2f–%.2f' % rng) if rng else '')))
    rows.append(('EFFECT', '%d uses in %d blocks: %s' % (sum(st['fx_uses'].values()), st['fx_blocks'], fmt_counter(st['fx_uses']))))
    h = ['| Field | Shipped usage over all %d EVENT blocks (independent re-parse of `data/sounds.txt`) |' % sum(st['blocks'].values()),
         '|---|---|']
    h += ['| %s | %s |' % r for r in rows]
    return '\n'.join(h)


def fmt_param(t):
    """['TypeFixedValue','27.42','UpdateConstantly'] -> '27.42'; ranged -> 'rnd lo..hi'; linked -> 'Link in..->out..'."""
    once = '/once' if t and t[-1] == 'UpdateOncePerLoop' else ''
    if not t:
        return '?'
    if t[0] == 'TypeFixedValue':
        return t[1] + once
    if t[0] == 'TypeRangedRandom':
        sm = (' s%s' % t[3]) if float(t[3]) > 0 else ''
        return 'rnd %s..%s%s%s' % (t[1], t[2], sm, once)
    if t[0] == 'TypeLinked':
        sm = (' s%s' % t[5]) if float(t[5]) > 0 else ''
        return '%s %s..%s->%s..%s%s%s' % (t[6], t[1], t[3], t[2], t[4], sm, once)
    return ' '.join(t)


def table_fx(blocks, effects):
    names = {n.lower(): [row[0] for row in p] for n, p in effects}
    h = ['| # | Source | Event | Sound | Effect | Parameters (file order = `data/effects.txt` order; value, `rnd lo..hi`, or `Link inLo..inHi->outLo..outHi`) |',
         '|---|---|---|---|---|---|']
    n = 0
    for g, ty, b in blocks:
        for fx in b['EFFECTS']:
            n += 1
            # each line is: <ParamName> PARAMETER <spec>; the name is not checked by the loader (position matters)
            ps = '; '.join('%s %s' % (t[0], fmt_param(t[2:])) for t in fx[1])
            h.append('| %d | %s %s | %s | %s | %s | %s |' % (n, g, ty, b['EVENT'], b.get('SOUNDNAME', ''), fx[0], esc(ps)))
    return '\n'.join(h)


def table_sourcemap(src, work):
    """Every code file that fires, stops or queries sound events (from X2b_callsites.csv + a scan)."""
    calls = list(csv.DictReader(open(os.path.join(work, 'drafts', 'X2b_callsites.csv'), encoding='utf-8')))
    manifest = {r['path'] for r in csv.DictReader(open(os.path.join(work, 'manifest.csv'), encoding='utf-8'))}
    ev = collections.OrderedDict()
    for c in calls:
        name = c['event_arg'].strip().strip('"')
        name = VARNAMES.get(name, name)
        ev.setdefault(c['file'], set()).add(name if name else '"" (empty-name stub, commented out)')
    stops = collections.defaultdict(int)
    queries = collections.defaultdict(int)
    for root, _, files in os.walk(os.path.join(src, 'code')):
        for f in files:
            if not f.endswith(('.cpp', '.h')):
                continue
            full = os.path.join(root, f)
            rel = os.path.relpath(full, src).replace(os.sep, '/')
            if rel.startswith('code/sound/'):
                continue
            txt = open(full, encoding='latin-1').read()
            stops[rel] += txt.count('StopAllSounds(')
            queries[rel] += txt.count('NumInstances(') + txt.count('NumInstancesPlaying(')
    files = sorted(set(ev) | {k for k, v in stops.items() if v} | {k for k, v in queries.items() if v})
    h = ['| File | Role (events fired with `Trigger*Event`; `StopAllSounds` / `NumInstances*` call counts) | Exists in tree |',
         '|---|---|---|']
    missing = []
    for f in files:
        names = sorted(ev.get(f, set()), key=str.lower)
        role = []
        if names:
            role.append('fires ' + ', '.join(esc(x.replace('\\', '')) for x in names))
        if stops.get(f):
            role.append('%d StopAllSounds' % stops[f])
        if queries.get(f):
            role.append('%d NumInstances queries' % queries[f])
        inm = 'yes' if f in manifest else 'NO'
        if f not in manifest:
            missing.append(f)
        h.append('| `%s` | %s | %s |' % (f, '; '.join(role), inm))
    print('source-map call-site files:', len(files), 'missing from manifest:', missing)
    return '\n'.join(h)


def replace_marker(text, name, body):
    b, e = '<!-- S10:%s:BEGIN -->' % name, '<!-- S10:%s:END -->' % name
    i, j = text.find(b), text.find(e)
    if i < 0 or j < 0:
        print('marker %s not found' % name)
        return text
    return text[:i + len(b)] + '\n' + body + '\n' + text[j:]


def main(src, work, target):
    sources, groups = parse_sounds(os.path.join(src, 'data', 'sounds.txt'))
    effects = parse_effects(os.path.join(src, 'data', 'effects.txt'))
    st, blocks = stats(sources, groups, effects)
    for k, v in st.items():
        print('%-22s %s' % (k, v))
    rows = list(csv.DictReader(open(os.path.join(work, 'drafts', 'X2b_sound_events.csv'), encoding='utf-8')))
    p16 = list(csv.DictReader(open(os.path.join(work, 'drafts', 'P16_sound_events.csv'), encoding='utf-8')))
    # cross-checks
    mine = collections.Counter((g, ty, b['EVENT'].lower()) for g, ty, b in blocks)
    x2b = {(r['group'], r['object_type'], r['event'].lower()): int(r['blocks']) for r in rows if r['in_data'] == 'yes'}
    bad = [k for k in set(mine) | set(x2b) if mine.get(k) != x2b.get(k)]
    print('X2b rows', len(rows), 'bound', len(x2b), 'code-only', sum(1 for r in rows if r['in_data'] == 'no'))
    print('pair/block-count mismatches vs X2b:', bad)
    p16k = collections.Counter((r['group'], r['object_type'], r['event'].lower(), r['SOUNDNAME'].lower(),
                                r['POSITIONTYPE'], r['LOOPTYPE']) for r in p16)
    minek = collections.Counter((g, ty, b['EVENT'].lower(), b.get('SOUNDNAME', '').lower(),
                                 b.get('POSITIONTYPE'), b.get('LOOPTYPE')) for g, ty, b in blocks)
    print('block mismatches vs P16:', list((p16k - minek).items()) + list((minek - p16k).items()))
    fired = collections.Counter(r['fired_in_FULL'] for r in rows if r['in_data'] == 'yes')
    print('bound fired_in_FULL:', dict(fired))
    print('code-only fired_in_FULL:', dict(collections.Counter(r['fired_in_FULL'] for r in rows if r['in_data'] == 'no')))
    for r in rows:
        r.update(ROW_FIX.get((r['group'], r['object_type'], r['event']), {}))
    bound = [r for r in rows if r['in_data'] == 'yes']
    code = [r for r in rows if r['in_data'] == 'no']
    tables = {
        'EVENTS': table_events(bound),
        'CODEONLY': table_codeonly(code),
        'DEADDATA': table_deaddata(bound),
        'SOURCES': table_sources(sources, rows),
        'FIELDS': table_fields(st),
        'FX': table_fx(blocks, effects),
        'SOURCEMAP': table_sourcemap(src, work),
    }
    if target == '-':
        for k, v in tables.items():
            print('\n=== %s\n%s' % (k, v))
        return
    text = open(target, encoding='utf-8').read()
    for k, v in tables.items():
        text = replace_marker(text, k, v)
    open(target, 'w', encoding='utf-8').write(text)
    print('wrote', target, len(text), 'bytes')


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3])
