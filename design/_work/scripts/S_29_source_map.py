"""S29: build the 'Source map' table of 29_data_formats_reference.md from the paths the document cites.

Usage: python3 -I S_29_source_map.py DOC_MD MANIFEST_CSV OUT_MD

For every code/data/contrib/tools/targets/docs path (or root file) cited in DOC_MD that exists in MANIFEST_CSV,
emit one table row: path, role (hand-written for key files, else derived), and the fmt-* sections citing it.
Glob citations (data/levels/*.txt ...) are emitted as one row with the number of manifest files they match.
"""
import csv
import fnmatch
import re
import sys

ROOTS = ('code/', 'data/', 'contrib/', 'tools/', 'targets/', 'docs/')

ROLES = {
    'contrib/systemiv/lib/filesys/text_stream_readers.cpp': 'TextReader / TextFileReader / TextDataReader: line acquisition, cleaning, tokenising, redshirt2 decoding',
    'contrib/systemiv/lib/filesys/text_stream_readers.h': 'TextReader class declarations (virtual / non-virtual split)',
    'contrib/systemiv/lib/unicode/unicode_text_stream_reader.cpp': 'UTF-16 readers: BOM sniffing, narrow mirror, wide tokenising, redshirt2 offset bug',
    'contrib/systemiv/lib/unicode/unicode_string.cpp': 'UnicodeString: Latin-1 widening, narrow mirror, StrOffsetCmp, ReplaceStringFlag',
    'contrib/systemiv/lib/filesys/text_file_writer.cpp': 'TextFileWriter: redshirt2 encoder',
    'code/unicode/unicode_text_file_writer.cpp': 'UnicodeTextFileWriter: UTF-16LE + BOM writer (preferences)',
    'contrib/systemiv/lib/filesys/binary_stream_readers.cpp': 'BinaryFileReader / BinaryDataReader conventions',
    'contrib/systemiv/lib/math/math_stubs.cpp': 'iv_atof (F32 rounding of parsed numbers)',
    'contrib/systemiv/lib/tosser/directory.cpp': 'Directory::Read/Write - network wire container',
    'contrib/systemiv/lib/tosser/directory.h': 'Directory type codes and safe values',
    'contrib/systemiv/lib/network_stream.cpp': 'packed int, dynamic / unicode strings, void blobs',
    'contrib/systemiv/lib/network_stream_inlines.cpp': 'WriteNetworkValue / ReadNumbersData primitives',
    'contrib/systemiv/lib/tosser/hash_table.cpp': 'case-insensitive hash table (archive keys, level_enum)',
    'code/lib/resource.cpp': 'Resource: reader selection, archive mounting, texture/shape/sound look-up, ListResources',
    'code/level_file.cpp': 'LevelFile parser and writer for map / mission files; map CRC',
    'code/level_file.h': 'LevelFile, CameraMount, CamAnimNode, InstantUnit declarations, option and effect enums',
    'code/worldobject/building.cpp': 'Building::CreateBuilding, Building::Read/Write base columns, BuildingIsBlocked',
    'code/game_menu.cpp': 'GameMenu::AddMap menu scanner, CreateMapList',
    'code/achievement_tracker.cpp': 'level_enum.txt parser, achievement level ids',
    'code/multiwinia.cpp': 'Multiwinia::LoadBlueprints (multiwinia.txt)',
    'code/global_world.cpp': 'game.txt / locations.txt parser and writer, GlobalEventCondition',
    'contrib/systemiv/lib/math/filecrc.cpp': 'FileCRC map identity',
    'contrib/unrar/crc.cpp': 'CRC-32 table used by FileCRC',
    'tools/MapCRC/main.cpp': 'generates code/mapcrc.h',
    'code/mapcrc.h': 'generated MAPID_* table',
    'code/lib/preferences.cpp': 'PrefsManager / PrefsItem: preferences reader, writer, defaults',
    'code/lib/input/input.cpp': 'InputManager::parseInputPrefs, spec parsing',
    'code/lib/input/inputdriver.cpp': 'condition table, getDefaultConditionID fallback',
    'code/lib/input/file_paths.cpp': 'binding file paths, keyboard layout, locales.txt reader',
    'code/main.cpp': 'input driver registration, binding layering, --write-version, RecordDemo',
    'code/lib/language_table.cpp': 'LangTable: language parser, mode tables, caption markup, QA report',
    'code/app.cpp': 'archive load order, preferences construction, language selection, auth key read, origin',
    'code/script.cpp': 'Script: cut-scene script opcodes and execution',
    'code/worldobject/entity.cpp': 'EntityBlueprint::Initialise (stats.txt), entity type names',
    'code/gesture.cpp': 'gestures.txt reader and writer',
    'code/gesture_demo.cpp': 'gesture demo reader [B:DEAD]',
    'code/sound/soundsystem.cpp': 'sounds.txt and effects.txt parsers and writers',
    'code/sound/sound_parameter.cpp': 'SoundParameter::Read / Write',
    'code/sound/sample_cache.cpp': 'CachedSample: sample resolution and Ogg validity checks',
    'code/sound/sound_stream_decoder.cpp': 'Ogg Vorbis (and unreachable WAV) decoder',
    'code/lib/shape.cpp': 'Shape / ShapeFragment / ShapeMarker parser and writer',
    'tools/ShapeExporter/ShapeExporter.cpp': '3ds Max exporter producing .shp',
    'code/lib/bitmap.cpp': 'BMP decoder, PNG/BMP writers, colour-key conversions',
    'code/lib/poster_maker.cpp': 'screenshot PNG writer',
    'code/landscape_renderer.cpp': 'landscape colour-ramp look-up',
    'code/water.cpp': 'water texture and wave ramp use',
    'code/animatedpanel.cpp': '.anim parser and interpolation',
    'code/multiwiniahelp.cpp': 'animated panel selection; help icon names',
    'code/lib/shader.cpp': 'HLSL shader loader [B:D3D]',
    'contrib/unrar/unrar.cpp': 'in-memory RAR extraction',
    'tools/build_multiwinia_data.bat': 'archive build script',
    'tools/listfiles.pl': 'exclusion list interpreter',
    'tools/copyfiles.pl': 'inclusion list interpreter',
    'tools/multiwinia_exclusions.txt': 'retail exclusion list',
    'tools/sounds.txt': 'sounds.dat list',
    'tools/menu_sounds.txt': 'menusounds.dat list',
    'tools/init.list': 'init.dat list',
    'code/network/server.cpp': 'serverHistory.dat writer / reader',
    'code/team.cpp': 'TeamControls::Pack / Unpack',
    'code/mapfile.cpp': '.mwm container reader / writer',
    'code/network/clienttoserver.cpp': 'sync dump file naming',
    'code/location.cpp': 'DumpWorldToSyncLog; level loading entry',
    'contrib/systemiv/lib/metaserver/authentication.cpp': 'authentication key file reader / writer',
    'contrib/systemiv/lib/debug_utils.cpp': 'black box, debug log, HashDouble',
    'code/lib/exception_handler.cpp': 'ERRORLOG.TXT / CRASH.DMP writer',
    'contrib/XCrashReport/Dlg.cpp': 'MultiwiniaSupport.ini reader',
    'code/UI/GameMenuWindow.cpp': 'credits / beta testers, testbedconfig reader, auth key paste',
    'code/keygen.cpp': 'legacy key generator (user/system info texts)',
}


def section_of(doc):
    """Return list of (start_index, anchor) for fmt anchors."""
    return [(m.start(), m.group(1)) for m in re.finditer(r'<a id="(fmt-[a-z0-9-]+)"></a>', doc)]


def main():
    doc_path, man_path, out_path = sys.argv[1], sys.argv[2], sys.argv[3]
    doc = open(doc_path, encoding='utf-8').read()
    cut = doc.find('\n## Source map')
    if cut > 0:
        doc = doc[:cut]
    manifest = [r['path'] for r in csv.DictReader(open(man_path, encoding='utf-8'))]
    by_lower = {p.lower(): p for p in manifest}
    root_files = {p.lower() for p in manifest if '/' not in p}
    anchors = section_of(doc)
    closing = doc.find('\n## 12 ')

    def anchor_at(pos):
        if closing > 0 and pos > closing:
            return 'closing'
        cur = 'index'
        for start, a in anchors:
            if start <= pos:
                cur = a
            else:
                break
        return cur

    hits = {}
    for m in re.finditer(r'`([^`\n]+)`', doc):
        span = m.group(1).strip().strip('"\'').replace('\\', '/')
        low = span.lower()
        if not (low.startswith(ROOTS) or low.split(':')[0] in root_files):
            continue
        base = re.split(r'[:#](?![/\\])', span, maxsplit=1)[0].rstrip('.,;)')
        # a backticked span may list several paths separated by ', '
        for piece in base.split(', '):
            piece = piece.strip()
            pl = piece.lower()
            if not (pl.startswith(ROOTS) or pl in root_files):
                continue
            if any(ch in pl for ch in '*?['):
                n = sum(1 for p in manifest if fnmatch.fnmatchcase(p.lower(), pl))
                if n == 0:
                    continue
                key = ('glob', piece)
            elif pl in by_lower:
                key = ('file', by_lower[pl])
            else:
                continue
            hits.setdefault(key, set()).add(anchor_at(m.start()))

    rows = []
    for (kind, path), secs in hits.items():
        secs = sorted(s for s in secs if s not in ('index', 'closing')) or sorted(secs)
        if kind == 'glob':
            n = sum(1 for p in manifest if fnmatch.fnmatchcase(p.lower(), path.lower()))
            role = f'file set ({n} manifest files)'
        else:
            role = ROLES.get(path, 'consumer, producer or sample of the formats listed')
        rows.append((path, role, ', '.join(f'[{s}](#{s})' if s.startswith('fmt-') else ('format index' if s == 'index' else 'guidance / closing sections') for s in secs)))
    rows.sort(key=lambda r: (r[0].split('/')[0] != 'code', r[0].lower()))

    with open(out_path, 'w', encoding='utf-8') as f:
        f.write('## Source map\n\n')
        f.write('Generated by `_work/scripts/S_29_source_map.py` from the paths cited above (every path exists in `_work/manifest.csv`; glob rows stand for the file sets catalogued per file in [25](25_textures_and_assets.md), [10](10_audio_design.md) and [08](08_levels_and_level_design.md)).\n\n')
        f.write('| File | Role | Formats citing it |\n|---|---|---|\n')
        for path, role, secs in rows:
            f.write(f'| `{path}` | {role} | {secs} |\n')
        f.write('\n')
    print(len(rows), 'rows')


if __name__ == '__main__':
    main()
