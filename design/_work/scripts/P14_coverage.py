# Builds coverage/P14_platform_lib.csv
# Usage: python3 -I P14_coverage.py <partitions.json> <manifest.csv> <out.csv>
import sys, json, csv
parts, manifest, out = sys.argv[1], sys.argv[2], sys.argv[3]
files = json.load(open(parts))['P14_platform_lib']['files']
man = list(csv.DictReader(open(manifest)))
areas = {r['path']: r for r in man}
S = {
 'build': '31.1 Build system',
 'plat': '31.2 Platform abstraction map',
 'time': '31.3 Timing',
 'paths': '31.4 File locations',
 'dat': '31.5 Data archives',
 'pipe': '31.6 Data build pipeline',
 'pack': '31.7 Packaging scripts',
 'inst': '31.8 Installers',
 'mapcrc': '31.9 MapCRC',
 'shape': '31.10 ShapeExporter',
 'loc': '31.11 Localisation build',
 'audit': '31.12 Audit scripts',
 'crash': '31.13 Crash handling',
 'sysinfo': '31.14 System information',
 'root': '31.15 Root files',
 'targets': '31.16 targets inventory',
 'tools': '31.17 tools inventory',
 'docs': '31.18 docs inventory',
 'thr': '20.T Threading primitives',
 'tok': '29.T Text tokeniser',
 'prefs': '29.P Preferences file',
 'lang': '29.L Language files',
 'dir': '29.D Directory format',
 'bin': '29.B Binary readers CRC hash keys',
 'rng': '22.R RNG and float discipline',
 'prof': '32.P Profiler',
 'dbg': '32.D Debug output asserts',
 'cont': '21.C Core containers',
 'app': 'Appendix dead/legacy',
 'ip': 'IP register',
 'dvc': 'Docs-vs-code discrepancies',
}
code_map = {
 'code/keygen.cpp': ['bin', 'app'], 'code/keygen.h': ['bin', 'app'],
 'code/lib/2d_array.h': ['cont'], 'code/lib/2d_surface_map.h': ['cont'],
 'code/lib/GetWinVer.cpp': ['sysinfo', 'crash'], 'code/lib/GetWinVer.h': ['sysinfo', 'crash'],
 'code/lib/MiniVersion.cpp': ['sysinfo', 'crash'], 'code/lib/MiniVersion.h': ['sysinfo', 'crash'],
 'code/lib/auto_vector.h': ['cont', 'ip'],
 'code/lib/exception_handler.cpp': ['crash'], 'code/lib/exception_handler.h': ['crash'],
 'code/lib/hi_res_time.cpp': ['time', 'plat'], 'code/lib/hi_res_time.h': ['time'],
 'code/lib/language_table.cpp': ['lang', 'loc'], 'code/lib/language_table.h': ['lang'],
 'code/lib/preferences.cpp': ['prefs'], 'code/lib/preferences.h': ['prefs'],
 'code/lib/presized_array.h': ['cont'],
 'code/lib/runnable.cpp': ['thr'], 'code/lib/runnable.h': ['thr'],
 'code/lib/slice_darray.h': ['cont'],
 'code/lib/system_info.cpp': ['sysinfo'], 'code/lib/system_info.h': ['sysinfo'],
 'code/lib/universal_include.cpp': ['build'], 'code/lib/universal_include.h': ['build', 'rng', 'plat'],
 'code/lib/user_info.cpp': ['sysinfo', 'app'], 'code/lib/user_info.h': ['sysinfo', 'app'],
 'code/lib/work_queue.cpp': ['thr', 'rng'], 'code/lib/work_queue.h': ['thr'],
 'code/unicode/unicode_text_file_writer.cpp': ['tok', 'prefs'], 'code/unicode/unicode_text_file_writer.h': ['tok'],
 'contrib/systemiv/lib/debug_utils.cpp': ['dbg', 'crash', 'rng'], 'contrib/systemiv/lib/debug_utils.h': ['dbg'],
 'contrib/systemiv/lib/filesys/binary_stream_readers.cpp': ['bin'], 'contrib/systemiv/lib/filesys/binary_stream_readers.h': ['bin'],
 'contrib/systemiv/lib/filesys/file_system.cpp': ['dat', 'app'], 'contrib/systemiv/lib/filesys/file_system.h': ['dat'],
 'contrib/systemiv/lib/filesys/filesys_utils.cpp': ['plat', 'thr', 'dat'], 'contrib/systemiv/lib/filesys/filesys_utils.h': ['plat'],
 'contrib/systemiv/lib/filesys/text_file_writer.cpp': ['tok'], 'contrib/systemiv/lib/filesys/text_file_writer.h': ['tok'],
 'contrib/systemiv/lib/filesys/text_stream_readers.cpp': ['tok'], 'contrib/systemiv/lib/filesys/text_stream_readers.h': ['tok'],
 'contrib/systemiv/lib/math/filecrc.cpp': ['bin', 'mapcrc'], 'contrib/systemiv/lib/math/filecrc.h': ['bin'],
 'contrib/systemiv/lib/math/hash.cpp': ['bin'], 'contrib/systemiv/lib/math/hash.h': ['bin'],
 'contrib/systemiv/lib/math/math_stubs.cpp': ['rng'], 'contrib/systemiv/lib/math/math_stubs.h': ['rng'],
 'contrib/systemiv/lib/math/random_number.cpp': ['rng'], 'contrib/systemiv/lib/math/random_number.h': ['rng'],
 'contrib/systemiv/lib/memory_leak.cpp': ['dbg', 'app'], 'contrib/systemiv/lib/memory_leak.h': ['dbg'],
 'contrib/systemiv/lib/profiler.cpp': ['prof'], 'contrib/systemiv/lib/profiler.h': ['prof'],
 'contrib/systemiv/lib/string_utils.cpp': ['dbg'], 'contrib/systemiv/lib/string_utils.h': ['dbg'],
 'contrib/systemiv/lib/tosser/bounded_array.h': ['cont'], 'contrib/systemiv/lib/tosser/btree.h': ['cont'],
 'contrib/systemiv/lib/tosser/darray.h': ['cont'], 'contrib/systemiv/lib/tosser/directory.cpp': ['dir'],
 'contrib/systemiv/lib/tosser/directory.h': ['dir'], 'contrib/systemiv/lib/tosser/fast_darray.h': ['cont'],
 'contrib/systemiv/lib/tosser/hash_table.h': ['cont'], 'contrib/systemiv/lib/tosser/llist.h': ['cont', 'thr'],
 'contrib/systemiv/lib/tosser/sorting_hash_table.h': ['cont', 'prof'],
 'contrib/systemiv/lib/unicode/unicode_string.cpp': ['lang', 'tok'], 'contrib/systemiv/lib/unicode/unicode_string.h': ['lang'],
 'contrib/systemiv/lib/unicode/unicode_text_stream_reader.cpp': ['tok'], 'contrib/systemiv/lib/unicode/unicode_text_stream_reader.h': ['tok'],
 'targets/vs2005/resource.h': ['build', 'targets'],
}
rows = {}
def add(path, depth, reason, secs):
    rows[path] = (depth, reason, ';'.join(S[x] for x in secs))
for f in files:
    add(f, 'full', '', code_map[f])
binary_types = ('binary/', 'archive/', 'i18n/', 'image/', 'doc/pdf', 'doc/word', 'other/icns', 'other/nib', 'other/aps', 'other/dle', 'other/gtk', 'other/xpm')
for r in man:
    p, a, t = r['path'], r['area'], r['type']
    if p in rows:
        continue
    if a == 'tools':
        secs = ['tools']
        if p.endswith('.iss') or 'NSIS' in p or '/loki/' in p: secs.append('inst')
        if 'build_' in p or p.endswith(('exclusions.txt', '.list')) or 'inclusions' in p or p.endswith(('copyfiles.pl', 'listfiles.pl', 'sounds.txt')): secs.append('pipe')
        if 'build_multiwinia' in p and 'data' not in p: secs.append('pack')
        if '/MapCRC/' in p: secs.append('mapcrc')
        if 'ShapeExporter' in p: secs.append('shape')
        if '/language/' in p or 'language' in p or 'copystrings' in p or 'english.mk' in p: secs.append('loc')
        if p.endswith(('FindUnusedResources.sh', 'FindUnusedResourcesExceptions.txt', 'findAllOpts.sh', 'unused_samples.sh', 'unused_sample_groups.sh', 'FindErrors.sh')): secs.append('audit')
        if 'multiwiniasupport' in p or 'trace' in p: secs.append('crash')
        if 'readme-linux' in p: secs.append('dvc')
        secs = list(dict.fromkeys(secs))
        if t.startswith(binary_types) or p.endswith(('.exe', '.dll', '.cab', '.mo')) or '/bin/Linux/' in p:
            insp = p.endswith(('.exe', '.dll', '.dle')) and '/DXInstall/' not in p
            if insp:
                add(p, 'skimmed', 'binary: identified via file/strings and script references; role recorded', secs)
            else:
                add(p, 'not-read', 'binary/third-party payload; role recorded from name, file type and references', secs)
        else:
            add(p, 'full', '', secs)
    elif a == 'targets':
        secs = ['targets']
        if 'vs2005' in p: secs.append('build')
        if 'macosx' in p or 'linux' in p: secs.extend(['build', 'app'])
        if p.endswith(('.ico', '.icns', '.png')) or 'Credits' in p: secs.append('ip')
        secs = list(dict.fromkeys(secs))
        if p.endswith('project.pbxproj'):
            add(p, 'skimmed', 'large generated Xcode project: grepped defines/targets/file counts only', secs)
        elif t.startswith(('image/', 'other/nib', 'other/icns', 'other/aps', 'doc/pdf')) or p.endswith('.ico'):
            add(p, 'skimmed', 'binary/resource: type and dimensions inspected, role recorded', secs)
        elif p.endswith(('SDLMain.m', 'SDLMain.mm', 'Acknowledgements.rtf', 'Makefile')):
            add(p, 'skimmed', 'platform-only (not baseline): structure and relevant calls read', secs)
        else:
            add(p, 'full', '', secs)
    elif a == 'root':
        secs = ['root']
        if p in ('EULA.txt',): secs.append('ip')
        if p == 'README.TXT': secs.append('dvc')
        if p == 'MultiwiniaSupport.ini': secs.append('crash')
        if p in ('darwinia.gdf', 'darwinia.mcl', 'darwinia.png'): secs.append('inst')
        if p == 'animator.exe':
            add(p, 'skimmed', 'binary: file type only; role recorded', secs)
        elif p == 'animator.dat':
            add(p, 'skimmed', 'RAR archive: entry headers listed with scripts/P14_rar_list.py (not extracted)', secs)
        elif p == 'darwinia.png':
            add(p, 'skimmed', 'image: type/dimensions inspected', secs)
        else:
            add(p, 'full', '', secs)
    elif a == 'docs':
        secs = ['docs']
        if p in ('docs/RussianFontRequest.txt', 'docs/vista installer check list.txt', 'docs/input_preferences_syntax.txt', 'docs/cpu cycles.txt', 'docs/Network.txt'): secs.append('dvc')
        if p in ('docs/betatesters.txt', 'docs/sepulveda_interview.txt', 'docs/story material.txt', 'docs/retro music.txt', 'docs/IntroversionEULA.txt', 'docs/multiwinia names.txt', 'docs/darwin digital life.txt', 'docs/sepulvedas themepark.txt'): secs.append('ip')
        if p == 'docs/multiwinia_manual.pdf': secs.append('pack')
        if t.startswith(('image/',)):
            add(p, 'skimmed', 'docs image (supporting evidence): file type inspected only; diagrams belong to other subsystems', secs)
        elif t.startswith(('doc/',)):
            add(p, 'skimmed', 'docs office/pdf/rtf (supporting evidence): text extracted and skimmed', secs)
        else:
            add(p, 'skimmed', 'docs text (supporting evidence): skimmed for platform/build facts and discrepancies', secs)
extra = [
 ('contrib/systemiv/lib/tosser/llist.cpp', 'full', 'template body included by llist.h', ['cont', 'thr']),
 ('contrib/systemiv/lib/tosser/darray.cpp', 'full', 'template body included by darray.h', ['cont']),
 ('contrib/systemiv/lib/tosser/fast_darray.cpp', 'full', 'template body included by fast_darray.h', ['cont']),
 ('contrib/systemiv/lib/tosser/btree.cpp', 'full', 'template body included by btree.h', ['cont']),
 ('contrib/systemiv/lib/tosser/hash_table.cpp', 'full', 'template body included by hash_table.h', ['cont']),
 ('contrib/systemiv/lib/tosser/sorting_hash_table.cpp', 'full', 'template body included by sorting_hash_table.h', ['cont']),
 ('contrib/systemiv/lib/tosser/bounded_array.cpp', 'full', 'template body included by bounded_array.h', ['cont']),
 ('contrib/systemiv/lib/tosser/hash_map_darwin.h', 'full', 'outside list; Mac-only shim', ['app']),
 ('code/lib/2d_array.cpp', 'full', 'template body included by 2d_array.h', ['cont']),
 ('code/lib/2d_surface_map.cpp', 'full', 'template body included by 2d_surface_map.h', ['cont']),
 ('code/lib/slice_darray.cpp', 'full', 'template body included by slice_darray.h', ['cont']),
 ('code/lib/hi_res_time_gcc.cpp', 'full', 'not compiled; platform variant', ['time', 'app']),
 ('code/lib/hi_res_time_sdl.cpp', 'full', 'not compiled; platform variant', ['time', 'app']),
 ('code/lib/preference_names.h', 'full', 'header used by compiled files; screen pref key names', ['prefs']),
 ('code/lib/debug_logging.h', 'full', 'not compiled', ['dbg', 'app']),
 ('contrib/systemiv/lib/hi_res_time.h', 'full', 'shadowed header used by profiler.cpp', ['build', 'time']),
 ('contrib/systemiv/lib/network_stream.cpp', 'full', 'Directory wire helpers (owned by P15)', ['dir']),
 ('contrib/systemiv/lib/network_stream.h', 'full', 'Directory wire helpers (owned by P15)', ['dir']),
 ('contrib/systemiv/lib/network_stream_inlines.cpp', 'full', 'Directory wire helpers (owned by P15)', ['dir']),
 ('contrib/systemiv/lib/netlib/net_mutex_win32.cpp', 'skimmed', 'NetMutex/NetEvent semantics (owned by P15)', ['thr']),
 ('contrib/systemiv/lib/netlib/net_mutex.h', 'skimmed', 'NetMutex/NetEvent API (owned by P15)', ['thr']),
 ('contrib/systemiv/lib/netlib/net_thread_win32.cpp', 'skimmed', 'NetStartThread (owned by P15)', ['thr']),
 ('contrib/systemiv/lib/metaserver/authentication.cpp', 'skimmed', 'auth key format/hash (owned by P15)', ['bin']),
 ('contrib/systemiv/lib/metaserver/authentication.h', 'skimmed', 'AUTHENTICATION_KEYLEN (owned by P15)', ['bin']),
 ('contrib/XCrashReport/CrashFileNames.h', 'full', 'crash file name constants', ['crash']),
 ('contrib/unrar/crc.cpp', 'full', 'CRC32 used by FileCRC/MapCRC', ['bin', 'mapcrc']),
 ('contrib/unrar/unrar.cpp', 'skimmed', 'MemMappedFile name normalisation (archive semantics)', ['dat']),
 ('contrib/unrar/unrar.h', 'skimmed', 'MAX_FILES', ['dat']),
 ('code/lib/resource.cpp', 'skimmed', 'ParseArchive/GetTextReader/GetBinaryReader lookup order (owned by P12)', ['dat']),
 ('code/app.cpp', 'skimmed', 'profile dir, prefs path, archive order, language init, delayed jobs (owned by P01)', ['paths', 'dat', 'loc', 'thr']),
 ('code/main.cpp', 'skimmed', 'Initialise/AppMain/StandardiseCPUPrecision (owned by P01)', ['paths', 'crash', 'rng']),
 ('code/loading_screen.cpp', 'skimmed', 'job pump and worker queue (owned by P20/P01)', ['thr']),
 ('code/mapcrc.h', 'skimmed', 'generated map ID header', ['mapcrc']),
 ('code/level_file.cpp', 'skimmed', 'LevelFile::CalculeCRC only (owned by P11)', ['mapcrc']),
 ('code/interface/profilewindow.cpp', 'skimmed', 'profiler lifetime/output (owned by P17a)', ['prof']),
 ('code/network/server.cpp', 'skimmed', 'MakeRandom/GenSyncRandSeed only (owned by P15)', ['rng']),
 ('data/default_preferences.txt', 'full', 'first-run override file (format owned by X1a)', ['prefs']),
 ('data/language/english.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/french.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/german.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/italian.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/japanese.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/korean.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/portuguese.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/russian.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
 ('data/language/spanish.txt', 'skimmed', 'encoding/BOM inspected only (content owned by X1a)', ['loc', 'lang']),
]
for p, d, r_, secs in extra:
    if p not in rows:
        add(p, d, r_, secs)

override = {
 'tools/language/report.pl': ('skimmed', 'UTF-16 Perl script: configuration and all core subroutines read; report formatting skimmed'),
 'tools/language/test_translation.tcl': ('skimmed', 'Tcl QA script: lookup/markup emulation read; output helpers skimmed'),
 'tools/NSIS Installer/GESetup/GESetup/GESetup.cpp': ('skimmed', 'Darwinia Vista installer plugin: exported functions read; GUID/rich-save helpers skimmed'),
 'tools/loki/common/setup.data/setup.glade': ('not-read', 'third-party Loki Setup GTK UI definition (61 KB XML); role recorded'),
 'tools/loki/common/setup.sh': ('skimmed', 'third-party Loki bootstrap script: header and arch detection read'),
 'tools/loki/common/setup.data/splash.xpm': ('skimmed', 'image (XPM header only)'),
 'tools/NSIS Installer/Contrib/ExDLL/exdll.dpr': ('not-read', 'third-party NSIS SDK Delphi sample; role recorded'),
 'tools/NSIS Installer/Contrib/ExDLL/exdll_with_unit.dpr': ('not-read', 'third-party NSIS SDK Delphi sample; role recorded'),
 'tools/NSIS Installer/Contrib/ExDLL/nsis.pas': ('not-read', 'third-party NSIS SDK Delphi unit; role recorded'),
 'tools/NSIS Installer/Contrib/ExDLL/exdll.dsp': ('not-read', 'third-party NSIS SDK project file; role recorded'),
 'tools/NSIS Installer/Contrib/ExDLL/exdll.dsw': ('not-read', 'third-party NSIS SDK workspace; role recorded'),
 'tools/NSIS Installer/Contrib/ExDLL/exdll.h': ('skimmed', 'third-party NSIS plugin API header'),
 'tools/NSIS Installer/Contrib/ExDLL/extdll.inc': ('skimmed', 'third-party NSIS plugin API header'),
 'tools/NSIS Installer/GESetup/GESetup/GESetup.vcproj': ('skimmed', 'project file: configuration type and file list only'),
 'tools/NSIS Installer/GESetup/GESetup.sln': ('skimmed', 'solution file'),
 'tools/NSIS Installer/licence.txt': ('skimmed', 'EULA text (IP), first paragraphs read'),
 'tools/ShapeExporter/ShapeExporter.rc': ('skimmed', 'resource script: string table only'),
 'tools/ShapeExporter/resource.h': ('skimmed', 'resource IDs'),
 'tools/ShapeExporter/ShapeExporter.dsp': ('skimmed', 'VC6 project: include/link/source lines only'),
 'tools/ShapeExporter/ShapeExporterDLL.dsp': ('not-read', 'alternate VC6 project; role recorded'),
 'tools/ShapeExporter/ShapeExporter.dsw': ('not-read', 'VC6 workspace; role recorded'),
 'tools/3DGeometry/3DGeometry.dsp': ('skimmed', 'VC6 project: include/link/source lines only'),
 'tools/3DGeometry/3DGeometry.dsw': ('not-read', 'VC6 workspace; role recorded'),
 'tools/MapCRC/MapCRC.vcproj': ('skimmed', 'project: defines, include dirs and source list'),
 'targets/vs2005/Darwinia.vcproj': ('skimmed', '247 KB project: all 4 configurations read fully; per-file overrides extracted by scripts/P14_vcproj_overrides.py; file list grepped'),
 'targets/vs2005/GDF/DEU/Darwinia.gdf.xml': ('skimmed', 'UTF-16 GDF: key fields compared with ENU'),
 'targets/vs2005/GDF/ESP/Darwinia.gdf.xml': ('skimmed', 'UTF-16 GDF: key fields compared with ENU'),
 'targets/vs2005/GDF/FRA/Darwinia.gdf.xml': ('skimmed', 'UTF-16 GDF: key fields compared with ENU'),
 'targets/vs2005/GDF/ITA/Darwinia.gdf.xml': ('skimmed', 'UTF-16 GDF: key fields compared with ENU'),
 'targets/vs2005/GDF/NEU/Darwinia.gdf.xml': ('skimmed', 'UTF-16 GDF: key fields compared with ENU'),
 'targets/vs2005/resource.h': ('full', ''),
 'targets/macosx/Darwinia/SDLMain.m': ('not-read', 'Darwinia-era SDL Cocoa boilerplate; role recorded'),
 'targets/macosx/Darwinia/SDLMain.h': ('skimmed', 'Darwinia-era SDL Cocoa boilerplate'),
 'targets/macosx/Darwinia/English.lproj/InfoPlist.strings': ('skimmed', 'Darwinia bundle strings'),
 'targets/macosx/Multiwinia/SDLMain.h': ('skimmed', 'SDL Cocoa delegate interface'),
}
for p_, (d_, r2) in override.items():
    if p_ in rows:
        rows[p_] = (d_, r2, rows[p_][2])
with open(out, 'w', newline='') as fh:
    w = csv.writer(fh)
    w.writerow(['path', 'depth', 'reason', 'sections'])
    for p in rows:
        d, r_, s_ = rows[p]
        w.writerow([p, d, r_, s_])
cnt = {}
for r in man:
    if r['area'] in ('tools', 'targets', 'root', 'docs'):
        cnt[r['area']] = cnt.get(r['area'], 0) + (1 if r['path'] in rows else 0)
print('rows', len(rows), 'partition', sum(1 for f in files if f in rows), '/', len(files), cnt)
