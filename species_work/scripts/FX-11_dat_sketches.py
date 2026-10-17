"""FX-11: rewrite the C++ sketches of the data-schema files to AGENTS.md naming and layout.

Usage: python3 -I FX-11_dat_sketches.py <data_schemas dir> <clang-format binary> <.clang-format dir>

For each file: exact replacements inside its first ```cpp block, then the block is formatted
with clang-format (the repository's .clang-format, copied into the given directory).
"""
import os
import re
import subprocess
import sys

D, CF, STYLE_DIR = sys.argv[1], sys.argv[2], sys.argv[3]

EDITS = {
    'README.md': [
        ('namespace species::data {', 'namespace Engine\n{'),
        ('}  // namespace species::data', '} // namespace Engine'),
        ('Enum, EnumSet, Vec3Value, Direction, ColourRgba8, ColourMultiplier, Limit,',
         'Enum, EnumSet, Vec3Value, Direction, ColorRgba8, ColorMultiplier, Limit,'),
        ('  std::optional<double> minimum, maximum; // inclusive unless the field says otherwise',
         '  std::optional<double> minimum;          // inclusive unless the field says otherwise\n  std::optional<double> maximum;'),
        ('class SchemaRegistry {                    // generated, compiled into every product',
         '// Generated, compiled into every program.\nclass SchemaRegistry {'),
        ('  const RecordSchema* Find(std::string_view schemaId) const noexcept;',
         '  const RecordSchema* Find(std::string_view _schemaId) const noexcept;'),
        ('struct SourceLocation { std::string file; std::uint32_t line; std::uint32_t column; std::string fieldPath; };',
         'struct SourceLocation { std::string file; std::uint32_t line; std::uint32_t column; std::string fieldPath; };'),
        ('  DiagnosticClass cls;                    // table 6-A', '  DiagnosticClass diagnosticClass;        // table 6-A'),
        ('// ---- content build (tools product) -----------------------------------------------',
         '// ---- content build tool (tools solution) ----------------------------------------'),
        ('class AuthoringParser {                   // DAT-019 to DAT-023', '// DAT-019 to DAT-023\nclass AuthoringParser {'),
        ('    Parse(std::span<const std::byte> bytes, std::string_view file) const;',
         '    Parse(std::span<const std::byte> _bytes, std::string_view _file) const;'),
        ('class RecordValidator {                   // DAT-024 to DAT-032', '// DAT-024 to DAT-032\nclass RecordValidator {'),
        ('  std::vector<Diagnostic> Validate(const JsonDocument& doc, const RecordSchema& schema,\n                                   const ReferenceIndex& refs) const;',
         '  std::vector<Diagnostic> Validate(const JsonDocument& _document, const RecordSchema& _schema, const ReferenceIndex& _references) const;'),
        ('class RecordCooker {                      // DAT-033 to DAT-035', '// DAT-033 to DAT-035\nclass RecordCooker {'),
        ('  CookedRecord Cook(const JsonDocument& validated, const RecordSchema& schema) const;',
         '  CookedRecord Cook(const JsonDocument& _validated, const RecordSchema& _schema) const;'),
        ('class CanonicalWriter {                   // DAT-040 to DAT-044, DEC-800', '// DAT-040 to DAT-044, DEC-800\nclass CanonicalWriter {'),
        ('  std::string Write(const RecordValue& value, const RecordSchema& schema) const;',
         '  std::string Write(const RecordValue& _value, const RecordSchema& _schema) const;'),
        ('  std::expected<void, Diagnostic> ReplaceFileAtomically(const std::filesystem::path& target,\n                                                        std::string_view content) const;',
         '  std::expected<void, Diagnostic> ReplaceFileAtomically(const std::filesystem::path& _target, std::string_view _content) const;'),
        ('class SchemaMigration {                   // DAT-045, DAT-046', '// DAT-045, DAT-046\nclass SchemaMigration {'),
        ('    Migrate(const JsonDocument& older, SchemaVersion from, const RecordSchema& to) const;',
         '    Migrate(const JsonDocument& _older, SchemaVersion _from, const RecordSchema& _to) const;'),
        ('// ---- runtime (client, server and tools) ------------------------------------------',
         '// ---- runtime (client, server and content build tool) ----------------------------'),
        ('class RecordLoader {                      // DAT-036 to DAT-039', '// DAT-036 to DAT-039\nclass RecordLoader {'),
        ('  std::expected<RecordView<T>, LoadError> Load(const PackEntry& entry) const;',
         '  std::expected<RecordView<T>, LoadError> Load(const PackEntry& _entry) const;'),
        ('struct SchemaVersion { std::uint16_t major; std::uint16_t minor; };       // DAT-017',
         '// DAT-017\nstruct SchemaVersion { std::uint16_t major; std::uint16_t minor; };'),
    ],
    'config_input_and_strings.md': [
        ('namespace species::data::settings {', 'namespace Engine\n{'),
        ('}  // namespace species::data::settings', '} // namespace Engine'),
        ('namespace species::data::strings {', 'namespace Engine\n{'),
        ('}  // namespace species::data::strings', '} // namespace Engine'),
        ('struct SettingSpec {                                   // one row of table 3.1-A',
         '// One row of table 3.1-A.\nstruct SettingSpec {'),
        ('  std::optional<double> minimum, maximum;', '  std::optional<double> minimum;\n  std::optional<double> maximum;'),
        ('class SettingsDefaults {                               // generated; available before any pack is mounted (DEC-100)',
         '// Generated; available before any pack is mounted (DEC-100).\nclass SettingsDefaults {'),
        ('  static const SettingSpec* Find(std::string_view key) noexcept;   // exact; user binding keys by prefix',
         '  static const SettingSpec* Find(std::string_view _key) noexcept;   // exact; user binding keys by prefix'),
        ('class SettingsFile {                                   // used by the settings store (platform layer)',
         '// Used by the settings store (platform layer).\nclass SettingsFile {'),
        ('  static SettingsFileContent Read(std::u8string_view text, std::vector<Diagnostic>& log);    // never fails',
         '  static SettingsFileContent Read(std::u8string_view _text, std::vector<Diagnostic>& _log);    // never fails'),
        ('  static std::u8string Write(const SettingsFileContent& loaded, const ValueMap& current);   // DAT-201',
         '  static std::u8string Write(const SettingsFileContent& _loaded, const ValueMap& _current);   // DAT-201'),
        ('class StringTableBuilder {                             // content build: merge, validate, expand (DAT-217)',
         '// Content build: merge, validate, expand (DAT-217).\nclass StringTableBuilder {'),
        ('  std::vector<Diagnostic> Add(const JsonDocument& source);', '  std::vector<Diagnostic> Add(const JsonDocument& _source);'),
        ('  std::expected<CookedStringTable, std::vector<Diagnostic>> Build(const BindingDescriptions& bindings) const;',
         '  std::expected<CookedStringTable, std::vector<Diagnostic>> Build(const BindingDescriptions& _bindings) const;'),
        ('class StringTableView {                                // runtime, read-only (UI-221)',
         '// Run time, read-only (UI-221).\nclass StringTableView {'),
        ('  std::u8string_view Lookup(std::string_view id, InputMode mode) const noexcept;',
         '  std::u8string_view Lookup(std::string_view _id, InputMode _mode) const noexcept;'),
        ('  bool Exists(std::string_view id) const noexcept;', '  bool Exists(std::string_view _id) const noexcept;'),
    ],
    'entity_audio_geometry_and_panel_schemas.md': [
        ('namespace species::data {\n\nstruct ActorStats { ActorKind kind; double health, speed, rate; std::optional<double> actorRate; };   // table 3.1-A\n\nstruct CrateOutcomeEntry {                      // table 3.2-A\n  CrateOutcome outcome; OutcomeClass cls; bool good, bad, start; std::uint8_t comebackWeight; Delivery delivery;\n};\n',
         'namespace Species\n{\n\n// Table 3.1-A.\nstruct ActorStats { ActorKind kind; double health; double speed; double rate; std::optional<double> actorRate; };\n\n// Table 3.2-A.\nstruct CrateOutcomeEntry {\n  CrateOutcome outcome; OutcomeClass outcomeClass; bool good; bool bad; bool start; std::uint8_t comebackWeight; Delivery delivery;\n};\n\n// Table 3.7-B.\nstruct ExplainerKey;\nstruct ExplainerAnimation { MatchMode mode; std::uint32_t width; std::uint32_t height; double loopStart; double loopEnd;\n                            std::span<const std::span<const ExplainerKey>> objects; };\n\n} // namespace Species\n\nnamespace Engine\n{\n'),
        ('enum class ParamLink : std::uint8_t {           // R-AUD-052', '// R-AUD-052\nenum class ParamLink : std::uint8_t {'),
        ('struct SoundParam {                             // one modulatable value (DAT-268)\n  ParamKind kind; double value, lo, hi, inLo, inHi, outLo, outHi, smoothing; ParamLink link; ParamUpdate update;\n};',
         '// One modulatable value (DAT-268).\nstruct SoundParam {\n  ParamKind kind; double value; double lo; double hi; double inLo; double inHi; double outLo; double outHi; double smoothing;\n  ParamLink link; ParamUpdate update;\n};'),
        ('enum class EffectKind : std::uint8_t {          // R-AUD-077 order; the order carries meaning\n  Chorus, Compressor, Distortion, StereoEcho, Flanger, StandardGargle, RoomReverb,\n  ParametricEqualiser,',
         '// The order of R-AUD-077, which carries meaning.\nenum class EffectKind : std::uint8_t {\n  Chorus, Compressor, Distortion, StereoEcho, Flanger, StandardGargle, RoomReverb,\n  ParametricEqualizer,'),
        ('struct EffectBinding { EffectKind kind; std::array<SoundParam, 13> params; std::uint8_t count; };  // kind order',
         '// Parameters in the kind\'s order.\nstruct EffectBinding { EffectKind kind; std::array<SoundParam, 13> params; std::uint8_t count; };'),
        ('struct SoundLayerDef;                           // table 3.3-B', '// Table 3.3-B.\nstruct SoundLayerDef;'),
        ('struct DspParamDesc { std::string_view name; double minimum, maximum, declaredDefault, schemaDefault; bool integer; };',
         'struct DspParamDesc { std::string_view name; double minimum; double maximum; double declaredDefault; double schemaDefault; bool integer; };'),
        ('struct DspFilterDef { EffectKind kind; std::span<const DspParamDesc> params; };   // generated, compiled in',
         '// Generated, compiled in.\nstruct DspFilterDef { EffectKind kind; std::span<const DspParamDesc> params; };'),
        ('\nstruct ExplainerKey;                            // table 3.7-B\nstruct ExplainerAnimation { MatchMode mode; std::uint32_t width, height; double loopStart, loopEnd;\n                            std::span<const std::span<const ExplainerKey>> objects; };\n\n}  // namespace species::data',
         '\n} // namespace Engine'),
    ],
    'import_and_build_records.md': [
        ('namespace species::data::build {', 'namespace Engine\n{'),
        ('}  // namespace species::data::build', '} // namespace Engine'),
        ('enum class TextureKind : std::uint8_t { Colour, Ui, Normal, Mask, ExactData, CpuTable };   // AST table 3.3-A',
         '// AST table 3.3-A.\nenum class TextureKind : std::uint8_t { Color, Ui, Normal, Mask, ExactData, CpuTable };'),
        ('enum class ColourSpace : std::uint8_t { Srgb, Linear };', 'enum class ColorSpace : std::uint8_t { Srgb, Linear };'),
        ('enum class SamplerKind : std::uint8_t { LinearClamp, LinearWrap, PointClamp };             // ADR-018',
         '// The closed set of ADR-018.\nenum class SamplerKind : std::uint8_t { LinearClamp, LinearWrap, PointClamp, PointMagnifyClamp };'),
        ('enum class TintMarking : std::uint8_t { None, VertexWeight, MaterialFlag };                // AST-034',
         '// AST-034.\nenum class TintMarking : std::uint8_t { None, VertexWeight, MaterialFlag };'),
        ('struct EngineAssetTable;                        // table 3.4-A, generated from the code',
         'struct EngineAssetTable;                        // table 3.4-A, generated from the code'),
        ('class ImportRecordReader {                      // content build only', '// The content build tool only.\nclass ImportRecordReader {'),
        ('ReadTexture(const JsonDocument&) const;', 'ReadTexture(const JsonDocument& _document) const;'),
        ('ReadMesh(const JsonDocument&) const;', 'ReadMesh(const JsonDocument& _document) const;'),
        ('ReadAudio(const JsonDocument&) const;', 'ReadAudio(const JsonDocument& _document) const;'),
    ],
    'map_schema.md': [
        ('namespace species::data::map {\n', 'namespace Engine\n{\n\n// README DAT-008, DEC-120.\nstruct Limit {\n  enum class Form : std::uint8_t { Absent, None, Value, Unlimited } form;\n  std::uint32_t value;                          // meaningful only for Form::Value\n};\n\n} // namespace Engine\n\nnamespace Species\n{\n'),
        ('}  // namespace species::data::map', '} // namespace Species'),
        ('enum class MatchMode : std::uint8_t;            // the six modes of R-MODE-029, in their order',
         '// The six modes of R-MODE-029, in their order.\nenum class MatchMode : std::uint8_t;'),
        ('enum class MapOption : std::uint8_t {           // canonical order of R-MAP-070', '// The canonical order of R-MAP-070.\nenum class MapOption : std::uint8_t {'),
        ('enum class MapEffect : std::uint8_t { Snow, Lightning, DustVortices, EssenceOrbRain, SpecialLightRig };  // R-MAP-073',
         '// R-MAP-073.\nenum class MapEffect : std::uint8_t { Snow, Lightning, DustVortices, EssenceOrbRain, SpecialLightRig };'),
        ('struct Limit {                                  // README DAT-008, DEC-120\n  enum class Form : std::uint8_t { Absent, None, Value, Unlimited } form;\n  std::uint32_t value;                          // meaningful only for Form::Value\n};\n\n', ''),
        ('struct MatchOptions;                            // the map header, table 3.2-A', 'struct MatchOptions;                            // the map header, table 3.2-A'),
        ('struct MapLight { Vec3 front; Vec3 colour; };   // front normalised at cook time',
         '// The front is normalised at cook time.\nstruct MapLight { Engine::Vec3 front; Engine::Vec3 color; };'),
        ('struct CameraViewRecord { std::string name; Vec3 position, front, up; };',
         'struct CameraViewRecord { std::string name; Engine::Vec3 position; Engine::Vec3 front; Engine::Vec3 up; };'),
        ('  std::expected<MapHeaderView, LoadError> ReadHeader(const PackEntry& entry) const;     // the catalogue',
         '  std::expected<MapHeaderView, Engine::LoadError> ReadHeader(const Engine::PackEntry& _entry) const;     // the catalogue'),
        ('  std::expected<MapDefinitionView, LoadError> Load(const PackEntry& entry) const;       // match entry',
         '  std::expected<MapDefinitionView, Engine::LoadError> Load(const Engine::PackEntry& _entry) const;       // match entry'),
    ],
    'map_structure_records.md': [
        ('namespace species::data::map {', 'namespace Species\n{'),
        ('}  // namespace species::data::map', '} // namespace Species'),
        ('enum class StructureKind : std::uint8_t;        // reference/03 table 2.4-A, without Relic and Supply crate in map records',
         '// Reference/03 table 2.4-A, without Relic and Supply crate in map records.\nenum class StructureKind : std::uint8_t;'),
        ('struct StructureRecord {                        // the base fields, table 3.1-A', '// The base fields, table 3.1-A.\nstruct StructureRecord {'),
        ('  double x, z;                                  // wu, binary32-rounded (README DAT-010)',
         '  double x;                                     // wu, binary32-rounded (README DAT-010)\n  double z;'),
        ('  KindFields fields;                            // one alternative per kind, table 3.2-A',
         '  std::optional<std::string> roleTag;           // DAT-162\n  KindFields fields;                            // one alternative per kind, table 3.2-A'),
        ('struct StructureLink {                          // every link field of every kind (DAT-147)', '// Every link field of every kind (DAT-147).\nstruct StructureLink {'),
    ],
    'test_and_telemetry_records.md': [
        ('namespace species::data::diag {', 'namespace Engine\n{'),
        ('}  // namespace species::data::diag', '} // namespace Engine'),
        ('class SoakMatchTableWriter {       // owned by the SoakRunner (DBG-161)', '// Owned by the SoakRunner (DBG-161).\nclass SoakMatchTableWriter {'),
        ('  std::expected<void, Diagnostic> AppendRow(const SoakMatchRow&); // one write, then flush (DAT-383)',
         '  std::expected<void, Diagnostic> AppendRow(const SoakMatchRow& _row); // one write, then flush (DAT-383)'),
        ('std::expected<std::string, std::vector<Diagnostic>> WriteTelemetrySession(const TelemetrySession&);  // DAT-380',
         'std::expected<std::string, std::vector<Diagnostic>> WriteTelemetrySession(const TelemetrySession& _session);  // DAT-380'),
        ('std::expected<TelemetrySession, std::vector<Diagnostic>> ReadQueuedTelemetrySession(std::string_view); // DAT-385',
         'std::expected<TelemetrySession, std::vector<Diagnostic>> ReadQueuedTelemetrySession(std::string_view _text); // DAT-385'),
    ],
}

ok = True
for name, edits in EDITS.items():
    path = os.path.join(D, name)
    text = open(path, encoding='utf-8').read()
    a = text.index('```cpp\n') + len('```cpp\n')
    b = text.index('\n```', a)
    block = text[a:b]
    for old, new in edits:
        n = block.count(old)
        if n != 1:
            print(f'MISS {name}: {n} x {old[:70]!r}')
            ok = False
            continue
        block = block.replace(old, new)
    block = block.replace(' public:\n', 'public:\n')
    src = os.path.join(STYLE_DIR, 'sketch.h')
    open(src, 'w', encoding='utf-8').write(block + '\n')
    fmt = subprocess.run([CF, '--style=file', src], capture_output=True, text=True, cwd=STYLE_DIR, check=True).stdout
    text = text[:a] + fmt.rstrip('\n') + text[b:]
    if ok:
        open(path, 'w', encoding='utf-8').write(text)
    print(f'{name}: {len(text.encode())} bytes')
sys.exit(0 if ok else 1)
