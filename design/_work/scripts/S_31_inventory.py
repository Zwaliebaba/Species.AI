"""S31 helper: build the one-row-per-file inventories of tools/, targets/, docs/ and the repository
root for 31_platform_build_and_tools.md from the P14 routed tables, apply the S31 corrections
(verified against the source), and check completeness against _work/manifest.csv.

Usage: python3 -I S_31_inventory.py <OUT_WORK_DIR>   (prints markdown to stdout, problems to stderr)
"""
import sys, os, re, csv

work = sys.argv[1]
routed = open(os.path.join(work, 'routed', '31.md'), encoding='utf-8').read()
manifest = [r['path'] for r in csv.DictReader(open(os.path.join(work, 'manifest.csv'), encoding='utf-8'))]
mset = set(manifest)

def section(title_prefix):
    i = routed.find(title_prefix)
    if i < 0:
        sys.exit('section not found: ' + title_prefix)
    j = routed.find('\n## ', i + 10)
    return routed[i: j if j > 0 else len(routed)]

def rows(text):
    out = []
    for line in text.splitlines():
        if line.startswith('| ') and not line.startswith('| #') and not line.startswith('| File') and '---' not in line:
            cells = [c.strip() for c in line.strip().strip('|').split(' | ')]
            out.append(cells)
    return out

# ---------------- corrections (S31, verified against source) ----------------
TOOLS_FIX = {
    'tools/SlashSort.exe': ('binary', '[I] Console filter that groups the lines of a memory log by source file name: its only readable strings are the usage line `slashsort memory.log > output` and the banner `Filename groupings:` — most likely post-processing for `TRACK_MEMORY_LEAKS` output; referenced by no script', 'unreferenced'),
    'tools/build_multiwinia.bat': ('MW packaging', 'Calls `build_multiwinia_data.bat` with **no arguments** (platform `plat`, retail exclusion list, and an **English-only** `language.dat`, because `build_language_dat.bat` honours the language only for platform `retail`); copies `darwinia_gl.exe` -> `build\\multiwinia.exe` and `contrib\\systemIV\\contrib\\XCrashReport\\dbghelp.dll`; `upx --best` on the exe', 'live (plain)'),
    'tools/build_multiwinia_crashreporter.bat': ('MW packaging', 'As `build_multiwinia.bat` (no-argument data build, English-only `language.dat`) plus `MultiwiniaSupport.ini` and `MultiwiniaSupport.exe` copied from the repository root (the exe is absent there); UPX lines commented out ("Cause problems with debugging tools")', 'live (refs absent file)'),
    'tools/build_multiwinia_gold.bat': ('MW packaging', 'Gold-master package: no-argument data build (English-only `language.dat`) + exe + dbghelp + `MultiwiniaSupport.ini` + `tools\\multiwiniasupport\\MultiwiniaSupport.exe` + `tools\\autopatcher\\{autopatch,copy_assist}.exe` + `tools\\profilelocatorwin32\\profilelocator.exe` + `docs\\multiwinia_manual.pdf` as `manual.pdf`; no UPX', 'live (autopatcher/profilelocator dirs absent)'),
    'tools/build_multiwinia_pc_preview.bat': ('MW packaging', 'PC preview SKU: data build `pcpreview %1` (beta-test exclusions + `multiwinia_inclusions_previewcopy.txt`; English-only `language.dat` because the platform is not `retail`), exe only, UPX commented out', 'variant (TARGET_PC_PREVIEW)'),
    'tools/InstallerScript_retail_crashreporter_euro.iss': ('installer', 'Retail v1.2.3 EFIGS: `[Languages]` en/fr/it/de/es with localised `[CustomMessages]` shortcut names, `ShowLanguageDialog=no`; **ships `ambx.dat`**; `branding.dat` line commented out; `origin` installed `onlyifdoesntexist nocompression dontverifychecksum`', 'live (release)'),
    'tools/multiwiniasupport/MultiwiniaSupport.exe': ('binary', 'Crash/support reporter launched by `RecordExceptionInfo`; built from `contrib/XCrashReport` (identical dialog/protocol strings): runs `dxdiag /t <profile>dxdiag_info.txt`, zips the files listed in `MultiwiniaSupport.ini` into `<profile>multiwinia.zip`, and **only when the user presses Send** uploads over TCP to `crashreports.introversion.co.uk:4537` (§6.4)', 'live'),
    'tools/traceblackbox.bat': ('crash tooling', 'Adds a developer-local Perl path, runs `perl trace.pl blackbox.txt darwinia.map > traceblackbox.txt` (the baseline Release link writes its map under the default name, not `darwinia.map` [I])', 'live helper'),
    'tools/MapCRC/MapCRC.exe': ('binary', 'Prebuilt MapCRC (Win32 GUI-subsystem exe, entry `WinMain`) invoked by the `Darwinia.vcproj` pre-build event `"../../tools/MapCRC/MapCRC.exe" ../../data/levels/ ../../code/mapcrc.h` in all four configurations', 'live'),
}
TARGETS_FIX = {
    'targets/macosx/Multiwinia/Multiwinia.xcodeproj/project.pbxproj': ('Multiwinia 1.3.1 Mac Xcode project: targets `Multiwinia` (app), `Generate Map CRCs` (builds `tools/MapCRC/main.cpp`, run as a build phase on `data/levels/` -> `code/mapcrc.h`) and `OggVorbis`; ppc+i386; defines in §7.8; 936 file references resolve, 26 do not (Ambrosia frameworks, `code/sound/macosx/*`, `pokey`, `command_buffer`, `debug_xbox`, `Crypticon`)', '[B:DEAD] on PC'),
    'targets/macosx/Multiwinia/data': ('80-byte Subversion "special" link file (`link <absolute developer-machine path>/data`) — **not** a working symlink in this checkout; the Xcode build phase instead runs `ln -sf "$SRCROOT/../../../data"` inside the bundle Resources', '[B:DEAD]'),
    'targets/linux/Makefile': ('Darwinia-era Linux makefile (`CXX=apg++`, `-g -Os`, `-DTARGET_OS_LINUX -DHAVE_SOCKET -DHAVE_SELECT -DHAVE_GETHOSTBYNAME -DHAVE_INET_NTOA`, SDL/GL/GLU/vorbisfile/vorbis/ogg, `TARGET_DEMOGAME`/`TARGET_FULLGAME` variants); 247 `code/`+`contrib/` sources listed, 23 absent (17 under `code/`, 6 under `contrib/netclass`/`contrib/binreloc`)', '[B:DEAD]'),
}
DOCS_FIX = {
    'docs/Network.txt': ('early design: server ticks 25 FPS, sends last 3 frames\' updates via UDP, 1 s watchdog', 'stale: code ticks the server at 10 Hz (`SERVER_ADVANCE_PERIOD`, see [22](22_determinism_and_networking.md))'),
}

def fmt(cells):
    return '| ' + ' | '.join(cells) + ' |'

problems = []
out = []

# root
root_rows = rows(section('## [→ 31_platform_build_and_tools] 31.15 Root files'))
out.append('### 14.1 Repository root (10 files)\n')
out.append('| # | File | Role | Evidence |')
out.append('|---|---|---|---|')
seen = set()
for n, r in enumerate(root_rows, 1):
    f = r[0].strip('`')
    seen.add(f)
    out.append(fmt([str(n), '`%s`' % f, r[1], r[2]]))
for p in manifest:
    if '/' not in p and p not in seen:
        problems.append('root file missing: ' + p)

# targets
t_rows = rows(section('## [→ 31_platform_build_and_tools] 31.16'))
out.append('\n### 14.2 `targets/` (64 files)\n')
out.append('| # | File | Role | Build tag |')
out.append('|---|---|---|---|')
seen = set()
for r in t_rows:
    f = r[1].strip('`')
    role, tag = r[2], r[3]
    if f in TARGETS_FIX:
        role, tag = TARGETS_FIX[f]
    seen.add(f)
    if f not in mset:
        problems.append('targets row not in manifest: ' + f)
    out.append(fmt([r[0], '`%s`' % f, role, tag]))
for p in manifest:
    if p.startswith('targets/') and p not in seen:
        problems.append('targets file missing: ' + p)

# tools
tl_rows = rows(section('## [→ 31_platform_build_and_tools] 31.17'))
out.append('\n### 14.3 `tools/` (174 files)\n')
out.append('Status column: *live* = used for the Multiwinia PC build or release; *variant* = non-baseline SKU; *legacy* = Darwinia-era; *third-party*; *authoring* = content tool; *dev audit*; *unreferenced*; *stale*.\n')
out.append('| # | File | Category | Role | Status |')
out.append('|---|---|---|---|---|')
seen = set()
for r in tl_rows:
    f = r[1].strip('`')
    cat, role, st = r[2], r[3], r[4]
    if f in TOOLS_FIX:
        cat, role, st = TOOLS_FIX[f]
    seen.add(f)
    if f not in mset:
        problems.append('tools row not in manifest: ' + f)
    out.append(fmt([r[0], '`%s`' % f, cat, role, st]))
for p in manifest:
    if p.startswith('tools/') and p not in seen:
        problems.append('tools file missing: ' + p)

# docs
d_rows = rows(section('## [→ 31_platform_build_and_tools] 31.18'))
out.append('\n### 14.4 `docs/` (70 files, supporting evidence only)\n')
out.append('| # | File | Content (summary) | Relevance / discrepancy note |')
out.append('|---|---|---|---|')
seen = set()
for r in d_rows:
    f = r[1].strip('`')
    c, note = r[2], r[3]
    if f in DOCS_FIX:
        c, note = DOCS_FIX[f]
    seen.add(f)
    if f not in mset:
        problems.append('docs row not in manifest: ' + f)
    out.append(fmt([r[0], '`%s`' % f, c, note]))
for p in manifest:
    if p.startswith('docs/') and p not in seen:
        problems.append('docs file missing: ' + p)

SECT = {'§20.T.2': '[20](20_architecture.md)', '§21.C': '[21](21_simulation_and_entity_model.md)',
        '§29.P.1': '[29](29_data_formats_reference.md)', '§31.1.2': '§7.2', '§31.1.4': '§7.5',
        '§31.13.2': '§6.4', '§31.4.2': '§5.3', '§31.1.6': '§7.8'}
AGENT = {'P01': '20', 'P03': '21', 'P05': '02', 'P06': '03', 'P08': '04', 'P09': '06', 'P10': '06',
         'P11': '23', 'P12': '24', 'P13': '24', 'P15': '22', 'P16': '27', 'P17b': '09', 'P19': '30',
         'P20': '09', 'X1a': '29', 'X3': '22'}
DOCNAME = {'20': '20_architecture', '21': '21_simulation_and_entity_model', '02': '02_units_and_entities',
           '03': '03_buildings_and_world_objects', '04': '04_crates_powerups_and_weapons', '06': '06_player_control_and_input',
           '23': '23_terrain_and_landscape', '24': '24_rendering', '22': '22_determinism_and_networking',
           '27': '27_audio_engine', '09': '09_meta_systems', '30': '30_scripting_tutorial_and_cutscenes',
           '29': '29_data_formats_reference'}
text = '\n'.join(out)
for k in sorted(SECT, key=len, reverse=True):
    text = text.replace(k, SECT[k])
def agent_sub(m):
    d = AGENT[m.group(0)]
    return '[%s](%s.md)' % (d, DOCNAME[d])
text = re.sub(r'\b(P17b|P0[1-9]|P1[0-9]|P20|X1a|X3)\b(?![_.])', lambda m: agent_sub(m) if m.group(0) in AGENT else m.group(0), text)
text = text.replace('grammar owned by X1a/[09](09_meta_systems.md)', 'grammar in [29](29_data_formats_reference.md) and [32](32_debug_test_and_performance.md)')
print(text)
for p in problems:
    print('PROBLEM:', p, file=sys.stderr)
print('# rows: root %d targets %d tools %d docs %d; problems %d' % (len(root_rows), len(t_rows), len(tl_rows), len(d_rows), len(problems)), file=sys.stderr)
