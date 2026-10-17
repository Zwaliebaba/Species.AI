"""S31 helper: emit the Source map table of 31_platform_build_and_tools.md (code/, contrib/, data/
files; tools/, targets/, docs/ and root files are covered by the §14 inventories) and verify that every
path exists in _work/manifest.csv.

Usage: python3 -I S_31_source_map.py <OUT_WORK_DIR>
"""
import sys, os, csv, glob

work = sys.argv[1]
manifest = {r['path']: r for r in csv.DictReader(open(os.path.join(work, 'manifest.csv'), encoding='utf-8'))}

ROWS = [
    # --- entry, window management, renderer bring-up
    ('code/lib/window_manager.h', 'abstract `WindowManager`, `Resolution`, `PlatformWindow` (§3.1)'),
    ('code/lib/window_manager.cpp', 'base class: display-mode helpers, closest resolution, mouse-capture helpers, `g_windowManager` (§3.1)'),
    ('code/lib/window_manager_win32.h', 'OpenGL window manager declaration (§3.3)'),
    ('code/lib/window_manager_win32.cpp', 'baseline `WinMain`, `CreateWin`, `EnableOpenGL`, display modes, mouse, `OpenWebsite` (§3.3)'),
    ('code/lib/window_manager_directx.h', 'D3D window manager declaration; `const char*` title (§3.4, §7.9)'),
    ('code/lib/window_manager_directx.cpp', 'D3D `WinMain`, `CreateWin`, Vista command line [B:D3D] (§3.4)'),
    ('code/lib/window_manager_sdl.h', 'SDL window manager declaration [B:DEAD] (§2.2)'),
    ('code/lib/window_manager_sdl.cpp', 'SDL window manager (Mac target) [B:DEAD] (§2.2, §3.11)'),
    ('code/lib/opengl_directx.cpp', '`Direct3DInit`, `Direct3DSwapBuffers`, `WaitAndResetDevice`, `glLineWidth` breakage [B:D3D] (§3.5, §3.6, §7.9)'),
    ('code/lib/opengl_directx.h', 'D3D emulation API [B:D3D] (§2.2)'),
    ('code/lib/opengl_directx_internals.h', '`LPDIRECT3DDEVICE9 g_pd3dDevice` declaration (§7.9)'),
    ('code/lib/ogl_extensions.cpp', 'WGL extension loading, `wglChoosePixelFormatARB` loaded but unused (§3.11)'),
    ('code/lib/ogl_extensions_directx.cpp', 'D3D extension stand-ins [B:D3D] (§2.2)'),
    ('code/lib/ogl_extensions_sdl.cpp', 'SDL extension loading [B:DEAD] (§2.2)'),
    ('code/lib/safegl.cpp', 'display-list mutex wrapper, inactive in baseline (§2.2)'),
    ('code/lib/safegl.h', 'display-list macro redirection (D3D/Mac only) (§2.2)'),
    ('code/renderer.cpp', '`Renderer::Initialise` retry chain, prefs write-back, `SetScreenGamma` (§3.7, §3.11)'),
    ('code/lib/preference_names.h', 'screen preference key names (§3.8)'),
    ('code/interface/prefs_screen_window.cpp', 'screen options dialog, `RestartWindowManagerAndRenderer`, `SetWindowed`, `HAVE_REFRESH_RATES` (§3.2, §3.9)'),
    ('code/lib/input/win32_eventhandler.cpp', 'window message handling, focus, Alt-Tab hot key (§3.10)'),
    ('code/lib/input/win32_eventhandler.h', 'Win32 event handler declaration (§3.10)'),
    ('code/interface/prefs_keybindings_window.cpp', 'commented-out Alt-Tab binding (§3.10)'),
    ('code/interface/mainmenus.cpp', '`ResetWindowHandle` call (§3.10)'),
    ('code/lib/avi_generator.cpp', 'AVI capture [B:DEBUG] (§3.12)'),
    ('code/lib/avi_generator.h', 'AVI capture declaration (§3.12)'),
    ('code/lib/avi_player.cpp', 'AVI playback for the eGames loader [B:DEAD] (appendix)'),
    ('code/lib/avi_player.h', 'AVI playback declaration [B:DEAD] (appendix)'),
    ('code/interface/grabber_window.cpp', 'debug Grabber window: fake time + AVI generator (§3.12, §4.2)'),
    ('code/interface/debugmenu.cpp', 'opens the Grabber window (§3.12)'),
    # --- timing
    ('code/lib/hi_res_time.h', 'timer API (§4.1)'),
    ('code/lib/hi_res_time.cpp', 'QPC clock, fake time, dead RDTSC path (§4)'),
    ('code/lib/hi_res_time_gcc.cpp', 'QPC variant for GCC [B:DEAD] (§4.3)'),
    ('code/lib/hi_res_time_sdl.cpp', '`gettimeofday` variant (Mac target) [B:DEAD] (§4.3)'),
    ('code/main.cpp', '`AppMain` SEH wrapper, `--write-version`, `Initialise` (`--NoProfile`, log redirect, `StandardiseCPUPrecision`), `UpdateAdvanceTime`, OS checks, `HandleCommonConditions` (§3.10, §4, §5, §6)'),
    ('code/loading_screen.cpp', 'loading-screen frame pacing (1/60 s) and archive-load serialisation (§3.11, §8.7)'),
    # --- paths, app
    ('code/app.cpp', '`App::App` (init.dat, prefs, renderer), `Initialise` (origin, archives, language), `LoadSounds`, `GetProfileDirectory`, `GetPreferencesPath`, `GetScreenshotDirectory`, `GetMapDirectory`, `InitMetaServer` (`authkey.dev`), `InitLanguage`, `IsMapPermitted`, `GetMapID`, `GamePaused` (§5, §8, §10, §12)'),
    ('code/app.h', '`App` members referenced (`m_lostFocusPause`, `m_originVersion`) (§3.10, §5.3)'),
    ('code/UI/AuthInputField.h', 'typed authentication key saved to `<P>authkey` (§5.3)'),
    ('code/UI/PlayDemoAuthkeyButton.h', 'generated demo key saved to `<P>authkey.dev` (§5.3)'),
    ('code/UI/GameMenuWindow.cpp', '`PasteFromClipboard` (`authkey.dev`), map-ID uses in tutorials/lobby (§2.3, §5.3, §10.3)'),
    ('code/lib/input/file_paths.cpp', 'input-preference file names and user path (§5.3)'),
    ('code/lib/input/input.cpp', '`inputprefs_debug.txt` (§5.3)'),
    ('code/network/clienttoserver.cpp', '`GetSyncFileName` desync dump naming (§5.3)'),
    ('code/benchmark.cpp', '`dxdiag.txt` generation and parsing (§5.3)'),
    ('code/lib/poster_maker.cpp', 'screenshot directory consumer (§5.3)'),
    ('code/lib/preferences.cpp', '`PrefsManager` load, `CreateDefaultValues` + `default_preferences.txt` overlay (§8.9)'),
    # --- crash, debug, sysinfo
    ('code/lib/exception_handler.cpp', '`RecordExceptionInfo`, `GetExceptionDescription`, minidump, support-tool launch (§6.2, §6.3)'),
    ('code/lib/exception_handler.h', 'crash handler declaration (§6.2)'),
    ('code/lib/GetWinVer.cpp', 'Windows version naming for crash logs (§6.6)'),
    ('code/lib/GetWinVer.h', 'version codes (§6.6)'),
    ('code/lib/MiniVersion.cpp', 'version-resource reader for crash logs (§6.6)'),
    ('code/lib/MiniVersion.h', '`CMiniVersion` declaration (§6.6)'),
    ('code/lib/system_info.cpp', 'locale, audio devices, DirectX version (§6.6)'),
    ('code/lib/system_info.h', '`SystemInfo` declaration (§6.6)'),
    ('code/lib/system_info_macosx.cpp', 'Mac system info [B:DEAD] (§2.2)'),
    ('code/lib/macosx_hardware_detect.cpp', 'Mac hardware detection [B:DEAD] (§2.2)'),
    ('code/lib/user_info.cpp', 'unused user-info file parser [B:LEGACY] (§6.6)'),
    ('code/lib/user_info.h', 'user-info declaration [B:LEGACY] (§6.6)'),
    ('code/sound/sound_library_3d_dsound.cpp', 'DirectX ≥ 9 assertion (§6.6)'),
    ('contrib/systemiv/lib/debug_utils.cpp', '`AppDebugOut`, redirect, `AppReleaseAssertFailed`, `AppReleaseExit`, `AppGenerateBlackBox`, log paths (§5.3, §6.5)'),
    ('contrib/systemiv/lib/debug_utils.h', 'assert macros (§6.5)'),
    ('contrib/XCrashReport/CrashFileNames.h', 'crash file names and support-tool exe name (§6.2, §6.4)'),
    ('contrib/XCrashReport/XCrashReport.cpp', 'support tool argument parsing (§6.4)'),
    ('contrib/XCrashReport/Dlg.cpp', 'support tool dialog, INI file list, zip, Send (§6.4)'),
    ('contrib/XCrashReport/SendEmail.cpp', 'TCP upload protocol to port 4537 (§6.4)'),
    ('contrib/XCrashReport/SystemInfoGatering.cpp', '`dxdiag /t` collection (§6.4)'),
    ('contrib/XCrashReport/IniDefines.h', 'INI name and section names (§6.4)'),
    ('contrib/XCrashReport/EmailDefines.h', 'fallback e-mail address (§6.4)'),
    ('contrib/XCrashReport/BackupFile.cpp', 'backs up an existing zip (`.zip.0001`…) (§6.4)'),
    ('contrib/XCrashReport/XZip.cpp', 'zip writer (§6.4)'),
    ('contrib/XCrashReport/XCrashReport.ini', 'sample INI (§6.4)'),
    ('contrib/XCrashReport/XCrashReport.vcproj', 'support tool project (§6.4)'),
    ('contrib/XCrashReport/XCrashReport.sln', 'support tool solution (§6.4)'),
    ('contrib/systemiv/contrib/XCrashReport/dbghelp.dll', '`dbghelp.dll` shipped next to the exe (§9.1)'),
    ('contrib/systemiv/contrib/XCrashReport/client/exception_handler.cpp', 'SystemIV copy of the crash handler, not compiled (appendix)'),
    # --- build
    ('code/lib/universal_include.h', 'product targets, feature macros, platform blocks, version strings (§7.6, §7.7)'),
    ('code/lib/universal_include.cpp', 'precompiled-header creator (§7.4)'),
    ('code/deform.cpp', 'D3D breakage 3 (`Resolve`) [B:D3D] (§7.9)'),
    ('code/water.cpp', 'D3D breakage 4 (`bmp.GetPixel`) (§7.9)'),
    ('code/worldobject/crate.h', '`INCLUDE_THEBEAST` under `TARGET_DEBUG` (§7.6)'),
    ('contrib/unrar/Unrar.vcproj', 'unrar sub-project (§7.1)'),
    ('contrib/eclipse/eclipse.vcproj', 'Eclipse UI sub-project (§7.1)'),
    ('contrib/libvorbis-1.1.2/win32/vorbis_static.vcproj', 'vorbis sub-project (§7.1)'),
    ('contrib/libvorbis-1.1.2/win32/vorbisfile_static.vcproj', 'vorbisfile sub-project (§7.1)'),
    ('contrib/libogg-1.1.3/win32/ogg_static.vcproj', 'ogg sub-project (§7.1)'),
    ('contrib/lpng1215/projects/visualc71/libpng.vcproj', 'libpng sub-project (§7.1)'),
    ('contrib/lpng1215/projects/visualc71/zlib.vcproj', 'zlib sub-project (§7.1)'),
    # --- archives, resources
    ('code/lib/resource.cpp', '`ParseArchive`, `GetTextReader`, `GetBinaryReader`, `GetUncompressedFile`, `FileExists`, `ListResources` (§8.8)'),
    ('code/lib/resource.h', '`HashTable<MemMappedFile*> m_resourceFiles` (§8.8)'),
    ('contrib/unrar/unrar.cpp', '`UncompressedArchive`, `MemMappedFile` (§8.8)'),
    ('contrib/unrar/unrar.h', '`MAX_FILES = 16384` (§8.8)'),
    ('contrib/unrar/extinfo.cpp', 'includes `win32acl.cpp`/`win32stm.cpp` (§2.2)'),
    ('contrib/unrar/win32acl.cpp', 'NTFS ACL extraction (compiled via include) (§2.2)'),
    ('contrib/unrar/win32stm.cpp', 'NTFS stream extraction (compiled via include) (§2.2)'),
    ('contrib/unrar/crc.cpp', 'CRC-32 table and `CRC()` used by `FileCRC` (§10.2)'),
    ('contrib/unrar/crc.h', 'CRC declaration (§10.2)'),
    ('contrib/systemiv/lib/tosser/hash_table.cpp', 'case-insensitive hash table used for archive entries (§8.8)'),
    ('contrib/systemiv/lib/filesys/file_system.cpp', 'unused SystemIV `FileSystem`, `OrderedInsert` (§8.10)'),
    ('contrib/systemiv/lib/filesys/filesys_utils.cpp', '`ListDirectory`, `FindCaseInsensitive`, `CreateDirectoryRecursively` (§2.3, §10.1)'),
    ('contrib/systemiv/lib/filesys/binary_stream_readers.cpp', '`BinaryReader::ReadU8` EOF behaviour for `FileCRC` (§10.2)'),
    ('contrib/systemiv/lib/math/filecrc.cpp', '`FileCRC`, `ReadFileData` (§10.2)'),
    ('contrib/systemiv/lib/math/filecrc.h', '`FileCRC` declaration (§10.2)'),
    ('contrib/systemiv/lib/netlib/net_thread_win32.cpp', '`CreateThread` wrapper, no SEH (§2.3, §6.2)'),
    ('contrib/systemiv/lib/netlib/net_mutex_win32.cpp', 'Win32 mutex/event (§2.3)'),
    ('contrib/systemiv/lib/netlib/net_lib_win32.h', 'Winsock platform header (§2.2)'),
    ('contrib/systemiv/lib/netlib/net_thread_linux.cpp', 'pthreads variant [B:DEAD] (§2.2)'),
    ('contrib/systemiv/lib/netlib/net_mutex_linux.cpp', 'pthreads variant [B:DEAD] (§2.2)'),
    ('contrib/systemiv/lib/network_stream.cpp', '`wchar_t` size handling on the wire (§2.3)'),
    ('code/unicode/unicode_text_file_writer.cpp', 'UTF-16/UTF-32 conversion per platform (§2.3)'),
    ('code/lib/work_queue.cpp', 'worker threads call `StandardiseCPUPrecision`; no SEH (§2.3, §6.2)'),
    # --- map CRC
    ('code/mapcrc.h', 'generated 87 `MAPID_*` defines (§10)'),
    ('code/level_file.cpp', '`LevelFile::CalculeCRC` (§10.3)'),
    ('code/UI/MapData.cpp', '`m_mapId` from CRC (§10.3)'),
    ('code/game_menu.cpp', 'custom-map flag when CRC = −1 (§10.3)'),
    ('code/network/demo_limitations.cpp', 'default demo map CRC list (§10.3)'),
    ('code/network/server.cpp', 'metaserver map CRC advertisement (§10.3)'),
    # --- localisation
    ('code/lib/language_table.cpp', '`LangTable`, `TestAgainstEnglish` → `language_errors.txt` (§12.5)'),
    ('contrib/systemiv/lib/unicode/unicode_text_stream_reader.cpp', 'UTF-16 BE/LE detection (§12.3)'),
    ('code/sepulveda.cpp', 'Darwinia `language_errors.txt` append path [B:DEAD] (§12.5)'),
    ('code/lib/shape.cpp', '`Shape::WriteToFile` used by the exporter (§11.1)'),
    ('code/lib/math_utils.cpp', 'intersection maths visualised by 3DGeometry (§11.2)'),
    ('data/default_preferences.txt', 'first-run default overlay, not reachable before `main.dat` (§8.9)'),
]
DATA_GLOBS = [
    ('data/levels/*.txt', 'MapCRC input; `MAPID_*` source (§10)'),
    ('data/language/*.txt', '`language.dat` member; encoding in §12.3'),
    ('data/textures/unicode/*.bmp', 'font page named by `tools/*language.list` (§12.2)'),
]
INIT = ['shaders/deform.vs', 'shaders/deformDiamond.ps', 'shaders/deformFullscreen.ps', 'shaders/deformSphere.ps',
        'shaders/FixedFunc0.vs', 'shaders/FixedFunc0Maps.ps', 'shaders/FixedFunc1.vs', 'shaders/FixedFunc1Maps.ps',
        'shaders/FixedFunc2.vs', 'shaders/FixedFunc2Maps.ps', 'shaders/globals.h', 'shaders/water-meshwaves.ps',
        'shaders/water-meshwaves.ps3', 'shaders/water-meshwaves.vs', 'sprites/darwinian.bmp',
        'textures/xboxlivearcade.bmp', 'textures/ivlogo.bmp']

out = ['| File | Role in this document |', '|---|---|']
missing = []
seen = set()
def add(p, role):
    if p in seen:
        return
    seen.add(p)
    if p not in manifest:
        missing.append(p)
    out.append('| `%s` | %s |' % (p, role))

for p, r in ROWS:
    add(p, r)
for p in INIT:
    role = '`init.dat` member (`tools/init.list`)'
    if p == 'textures/xboxlivearcade.bmp':
        role = 'listed in `tools/init.list`, deleted before packing on PC'
    elif p.startswith('shaders/') and not p.startswith('shaders/FixedFunc'):
        role += '; also packed in `main.dat`'
    add('data/' + p, role + ' (§8.7)')
for g, role in DATA_GLOBS:
    for p in sorted(k for k in manifest if glob.fnmatch.fnmatch(k, g) and k.count('/') == g.count('/')):
        r = role
        if '20a0.20af' in p:
            r = 'Euro-sign font page in no `tools/*language.list` — never shipped (§12.2)'
        add(p, r)
print('\n'.join(out))
print('# rows %d, missing from manifest %d %s' % (len(out) - 2, len(missing), missing), file=sys.stderr)
