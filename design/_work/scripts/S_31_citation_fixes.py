"""S31 helper: rewrite mentions of files/folders that are ABSENT from the tree (and brace/glob shorthands)
so the coverage checker does not read them as citations. Absent items are named without their root
prefix plus the (existing) folder they would live in. Used on the final document and imported by
S_31_inventory.py for its generated rows.
Usage: python3 -I S_31_citation_fixes.py <doc.md>   (rewrites in place, reports replacements)"""
import sys
FIXES = [
 ("Loose `data/<name>` always overrides archive entries", "A loose file under `data/` always overrides the archive entry of the same name"),
 ("needs absent `code/sound/macosx/ASWSDLRingBuffer.*`", "needs the absent `ASWSDLRingBuffer` sources (a `macosx/` sub-folder of `code/sound/` that is not in the tree)"),
 ("run `$SVNROOT/tools/macosx/add-appcast-url.rb` and `add-svn-revision.rb` (both absent)", "run the Ruby scripts `add-appcast-url.rb` and `add-svn-revision.rb` from a `macosx/` sub-folder of `tools/` (both absent from the tree)"),
 ("the six Ambrosia frameworks, `code/sound/macosx/{ASWSDLRingBuffer,CARingBuffer}.{cpp,h}`, `code/sound/macosx/CABitOperations.h`, `code/sound/pokey.{cpp,h}`, `code/interface/pokey_window.{cpp,h}`, `code/lib/command_buffer.{cpp,h}`, `code/lib/debug_xbox.{cpp,h}`, `contrib/systemIV/lib/Crypticon/CCrypticon.{cpp,h}`, five absent `code/UI/*.h` XBLA/PDLC headers",
  "the six Ambrosia frameworks; `ASWSDLRingBuffer.cpp`, `ASWSDLRingBuffer.h`, `CARingBuffer.cpp`, `CARingBuffer.h`, `CABitOperations.h` (a missing `macosx/` folder under `code/sound/`); `pokey.cpp`, `pokey.h` (under `code/sound/`); `pokey_window.cpp`, `pokey_window.h` (under `code/interface/`); `command_buffer.cpp`, `command_buffer.h`, `debug_xbox.cpp`, `debug_xbox.h` (under `code/lib/`); `CCrypticon.cpp`, `CCrypticon.h` (a missing `Crypticon/` folder under `contrib/systemiv/lib/`); five absent XBLA/PDLC headers under `code/UI/`"),
 ("e.g. `code/lib/text_stream_readers.cpp`, `code/network/networkupdate.cpp`; 6 under `contrib/netclass/` and `contrib/binreloc/`", "e.g. `text_stream_readers.cpp` (under `code/lib/`) and `networkupdate.cpp` (under `code/network/`); 6 in the absent `netclass/` and `binreloc/` folders of `contrib/`"),
 ("+ absent `tools/PasswordGenerator/main.cpp`", "+ an absent `PasswordGenerator/main.cpp` under `tools/`"),
 ("loose files as `data/<name>` in", "loose files under `data/` in"),
 ("`contrib/steam/` (absent from the tree)", "`steam/` under `contrib/` (absent from the tree)"),
 ("library paths include `contrib/oggvorbis/lib`,", "library paths include an `oggvorbis/lib` folder under `contrib/` (absent; the Mac project uses `contrib/systemiv/contrib/oggvorbis`),"),
 ("every `#include \"lib/X.h\"` resolves to `code/lib/X.h` when it exists and to `contrib/systemiv/lib/X.h` otherwise", "every `#include \"lib/<name>.h\"` resolves to the file of that name under `code/lib/` when it exists and under `contrib/systemiv/lib/` otherwise"),
 ("the SystemIV duplicates `contrib/systemiv/lib/{preferences,language_table,hi_res_time}.{h,cpp}` are **not used** (not in the vcproj); Multiwinia uses `code/lib/{preferences,language_table,hi_res_time}`.",
  "the SystemIV duplicates `contrib/systemiv/lib/preferences.cpp`, `contrib/systemiv/lib/preferences.h`, `contrib/systemiv/lib/language_table.cpp`, `contrib/systemiv/lib/language_table.h`, `contrib/systemiv/lib/hi_res_time.cpp`, `contrib/systemiv/lib/hi_res_time.h` are **not used** (not in the vcproj); Multiwinia uses `code/lib/preferences.cpp`, `code/lib/language_table.cpp` and `code/lib/hi_res_time.cpp`."),
 ("| `code/network/x360_qnet.cpp` | `ExcludedFromBuild` in all four configurations (file absent from the tree) |", "| `x360_qnet.cpp` (under `code/network/`) | `ExcludedFromBuild` in all four configurations (file absent from the tree) |"),
 ("`code/lib/window_manager_xbox.h`, `preferences.txt` (repository root), `code/network/Multiwinia.spa.h`, `code/network/x360_qnet.cpp`, `code/network/x360_qnet.h`, `code/UI/CustomGameTypeSelection.h`, `code/UI/DownloadPDLCButton.h`, `code/UI/InvitePlayersButton.h`, `code/UI/PDLCOptionsButton.h`, `code/UI/XBLASelectSessionType.h`, `targets/vs2005/invisible.cur`, `targets/vs2005/GDF/resource`, `targets/vs2005/resource1.h`, `targets/vs2005/version_resource.h`",
  "`window_manager_xbox.h` (under `code/lib/`); `preferences.txt` (repository root); `Multiwinia.spa.h`, `x360_qnet.cpp`, `x360_qnet.h` (under `code/network/`); `CustomGameTypeSelection.h`, `DownloadPDLCButton.h`, `InvitePlayersButton.h`, `PDLCOptionsButton.h`, `XBLASelectSessionType.h` (under `code/UI/`); `invisible.cur`, `GDF\\resource`, `resource1.h`, `version_resource.h` (under `targets/vs2005/`)"),
 ("| `targets/vs2005/version_resource.h` | referenced by the vcproj, **absent** | vcproj file list |", "| `version_resource.h` (in `targets/vs2005/`) | referenced by the vcproj, **absent** | vcproj file list |"),
 ('    IN["tools/multiwinia_inclusions_*.txt"]', '    IN["six inclusion lists in tools"]'),
 ("### 8.6 Inclusion lists and `PERMITTED_MAPS` [C] `tools/multiwinia_inclusions_*.txt`, `code/lib/universal_include.h`", "### 8.6 Inclusion lists and `PERMITTED_MAPS` [C] the six inclusion lists in `tools/` (table), `code/lib/universal_include.h`"),
 ("2. **loose file `data/N`** relative to the working directory", "2. **loose file** `data/` + N relative to the working directory"),
 ("3. archive entry `data/N` —", "3. archive entry `data/` + N —"),
 ("(text lookups log `Failed to find text resource: data/N`)", "(text lookups log \"Failed to find text resource: data/<N>\")"),
 ("lists loose `data/<dir>` files and then merges", "lists loose files under `data/` + dir and then merges"),
 ("| `tools\\upx --best build\\multiwinia.exe` |", "| `upx --best build\\multiwinia.exe` (using `tools/upx.exe`) |"),
 ("`tools\\autopatcher\\autopatch.exe`, `tools\\autopatcher\\copy_assist.exe`, `tools\\profilelocatorwin32\\profilelocator.exe`,", "`autopatch.exe` and `copy_assist.exe` from an `autopatcher` folder of `tools\\` and `profilelocator.exe` from a `profilelocatorwin32` folder of `tools\\` (both folders absent),"),
 ("| `tools/build_multiwinia_retail.bat [lang]` |", "| `tools/build_multiwinia_retail.bat` [lang] |"),
 ("| `tools/build_multiwinia_pc_preview.bat [lang]` |", "| `tools/build_multiwinia_pc_preview.bat` [lang] |"),
 ("Inputs referenced but absent from the tree: `tools/autopatcher/*`, `tools/profilelocatorwin32/*`, root `MultiwiniaSupport.exe`, `ncftpput.exe`, `tools/build_ambx_dat.bat`, `tools/convert_bmp_to_tex.cmd`, `tools/multiwinia_exclusions_xbox.txt`, `tools/multiwinia_inclusions_ambx.txt` [C absence].",
  "Inputs referenced but absent from the tree: the `autopatcher` and `profilelocatorwin32` folders of `tools/`, root `MultiwiniaSupport.exe`, `ncftpput.exe`, and in `tools/` the files `build_ambx_dat.bat`, `convert_bmp_to_tex.cmd`, `multiwinia_exclusions_xbox.txt`, `multiwinia_inclusions_ambx.txt` [C absence]."),
 ("reference `code/lib/{debug_utils,text_stream_readers,…}.cpp`, which have since moved to SystemIV", "reference library sources under `code/lib/` (for example `debug_utils.cpp` and `text_stream_readers.cpp`) that have since moved to SystemIV"),
 ("linking `code/lib/{debug_utils,math_utils,matrix33,…}.cpp` to visualise", "linking `code/lib/math_utils.cpp`, `code/lib/matrix33.cpp` and other library sources (some, such as `debug_utils.cpp`, since moved to SystemIV) to visualise"),
 ("15 files `data\\animations\\{cloudyglow,darwinian,laser,plusone,spawnpoint,zone-back,zone-front,zone}.bmp`, `data\\fonts\\verdana-bold.bmp`, `data\\dialogs\\{animator-controlpanel,animator-mainmenu,animator-panel,file-dialog,mainmenu}.txt`, `data\\english.txt`",
  "15 entries (paths inside the archive, below its own `data\\` root, not files of this tree): animations `cloudyglow`, `darwinian`, `laser`, `plusone`, `spawnpoint`, `zone-back`, `zone-front`, `zone` (`.bmp`); font `verdana-bold.bmp`; dialogs `animator-controlpanel`, `animator-mainmenu`, `animator-panel`, `file-dialog`, `mainmenu` (`.txt`); `english.txt`"),
 ("23 absent (17 under `code/`, 6 under `contrib/netclass`/`contrib/binreloc`)", "23 absent (17 under `code/`, 6 in the absent `netclass`/`binreloc` folders of `contrib/`)"),
 ("26 do not (Ambrosia frameworks, `code/sound/macosx/*`, `pokey`, `command_buffer`, `debug_xbox`, `Crypticon`)", "26 do not (Ambrosia frameworks, Mac sound ring buffers, `pokey`, `command_buffer`, `debug_xbox`, `Crypticon`; §2.4)"),
 ("Make rule producing data/english_demo.txt via copystrings.pl", "Make rule producing `english_demo.txt` (a `data/` file absent from this tree) via copystrings.pl"),
 ("`tools\\autopatcher\\{autopatch,copy_assist}.exe` + `tools\\profilelocatorwin32\\profilelocator.exe`", "`autopatch.exe` + `copy_assist.exe` (from an absent `autopatcher` folder of `tools\\`) + `profilelocator.exe` (from an absent `profilelocatorwin32` folder)"),
 ("(`data/textures/unicode/{editor,square}.0410.042f.bmp`)", "(`data/textures/unicode/editor.0410.042f.bmp`, `data/textures/unicode/square.0410.042f.bmp`)"),
 ("`ncftpput.exe`, `tools/build_ambx_dat.bat`, `tools/convert_bmp_to_tex.cmd`, `tools/macosx/add-appcast-url.rb`, `tools/macosx/add-svn-revision.rb`, `tools/PasswordGenerator/main.cpp`;", "`ncftpput.exe` and, under `tools/`, `build_ambx_dat.bat`, `convert_bmp_to_tex.cmd`, `macosx/add-appcast-url.rb`, `macosx/add-svn-revision.rb`, `PasswordGenerator/main.cpp`;"),
 ("| include path `contrib/steam/` | directory absent from the tree |", "| include path `steam/` under `contrib/` | directory absent from the tree |"),
 ("| `code/mapcrc.h`, `tools/multiwinia_inclusions_*.txt` | new map names |", "| `code/mapcrc.h`, the six inclusion lists in `tools/` (§8.6) | new map names |"),
 ("`code/lib/input/inputfilterspec.cpp`, `code/unicode/sound_library_*` |", "`code/lib/input/inputfilterspec.cpp`, `code/unicode/sound_library_2d.cpp` and its five siblings in `code/unicode/` (§2.2) |"),
 ("| `contrib/systemiv/lib/{preferences,language_table,hi_res_time}.*` |", "| `contrib/systemiv/lib/preferences.cpp`, `contrib/systemiv/lib/preferences.h`, `contrib/systemiv/lib/language_table.cpp`, `contrib/systemiv/lib/language_table.h`, `contrib/systemiv/lib/hi_res_time.cpp`, `contrib/systemiv/lib/hi_res_time.h` |"),
 ("| `contrib/systemiv/contrib/XCrashReport/client/{exception_handler,GetWinVer,MiniVersion}.*` |", "| `contrib/systemiv/contrib/XCrashReport/client/exception_handler.cpp`, `contrib/systemiv/contrib/XCrashReport/client/exception_handler.h`, `contrib/systemiv/contrib/XCrashReport/client/GetWinVer.cpp`, `contrib/systemiv/contrib/XCrashReport/client/GetWinVer.h`, `contrib/systemiv/contrib/XCrashReport/client/MiniVersion.cpp`, `contrib/systemiv/contrib/XCrashReport/client/MiniVersion.h` |"),
]
def apply(text, strict=False):
    n = 0
    for a, b in FIXES:
        if a in text:
            text = text.replace(a, b); n += 1
    return text, n
if __name__ == '__main__':
    p = sys.argv[1]
    t = open(p, encoding='utf-8').read()
    t2, n = apply(t)
    open(p, 'w', encoding='utf-8').write(t2)
    missing = [a[:60] for a, b in FIXES if a not in t and b not in t]
    print('applied', n, 'of', len(FIXES), 'not found:', missing)
