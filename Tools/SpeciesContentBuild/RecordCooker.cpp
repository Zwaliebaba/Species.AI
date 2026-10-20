#include "pch.h"

#include "AuthoringParser.h"
#include "RecordCooker.h"
#include "TuningMvp.h"

#include <algorithm>
#include <array>
#include <expected>
#include <format>
#include <utility>
#include <vector>

namespace ContentBuild
{

namespace
{

using CookedBytes = std::expected<std::vector<std::byte>, std::vector<Neuron::ContentDiagnostic>>;

/// A record type the content build cooks: its schema id and version, the folder under GameData/ its records live in
/// (DAT table 1-A), whether it is simulation-relevant (AST-098), and its generated reader and cooker.
struct RecordType
{
  std::string_view schemaId;
  Neuron::SchemaVersion version;
  std::string_view folder;
  bool simulationRelevant = false;
  CookedBytes (*cook)(const Neuron::JsonDocument&) = nullptr;
  bool (*readsBack)(std::span<const std::byte>) = nullptr;
};

CookedBytes CookTuning(const Neuron::JsonDocument& _document)
{
  auto record = Species::ReadTuningMvp(_document);
  if (!record)
  {
    return std::unexpected(std::move(record.error()));
  }
  return Species::CookTuningMvp(*record);
}

// AST-091: the cooked entry reads back through the server's loader as the record the cooker wrote.
bool TuningReadsBack(std::span<const std::byte> _entry)
{
  const auto record = Species::ReadCookedTuningMvp(_entry);
  return record && std::ranges::equal(Species::CookTuningMvp(*record), _entry);
}

constexpr std::array<RecordType, 1> RECORD_TYPES{{
  {Species::TUNING_MVP_SCHEMA_ID, Species::TUNING_MVP_VERSION, "Records/tuning/", Species::TUNING_MVP_SIMULATION_RELEVANT, &CookTuning,
   &TuningReadsBack},
}};

} // namespace

std::optional<Neuron::CookedEntry> CookRecord(std::string_view _file, std::string_view _contentPath, std::span<const std::byte> _bytes,
                                              BuildReport& _report)
{
  const auto document = Neuron::AuthoringParser{}.Parse(_bytes, _file);
  if (!document)
  {
    _report.Fail(document.error());
    return std::nullopt;
  }
  const auto schema =
    std::ranges::find_if(document->root.members, [](const Neuron::JsonMember& _member) { return _member.key == "schema"; });
  const std::string_view schemaId = schema == document->root.members.end() ? std::string_view{} : std::string_view{schema->value.text};
  const auto type = std::ranges::find(RECORD_TYPES, schemaId, &RecordType::schemaId);
  if (type == RECORD_TYPES.end())
  {
    _report.Fail(_file, std::format("the schema {} is no record type that the content build cooks (DAT-016, ADR-108)",
                                    schemaId.empty() ? std::string_view{"field is missing; it"} : schemaId));
    return std::nullopt;
  }
  if (!_contentPath.starts_with(type->folder))
  {
    _report.Fail(_file, std::format("a {} record lives in GameData/{} (DAT-016)", type->schemaId, type->folder));
    return std::nullopt;
  }
  CookedBytes bytes = type->cook(*document);
  if (!bytes)
  {
    for (const Neuron::ContentDiagnostic& diagnostic : bytes.error())
    {
      _report.Fail(diagnostic);
    }
    return std::nullopt;
  }
  if (!type->readsBack(*bytes))
  {
    _report.Fail(_file, "the cooked record does not read back as the record the cooker wrote (AST-091)");
    return std::nullopt;
  }
  Neuron::CookedEntry entry;
  entry.entry.type = Neuron::ContentType::Record;
  entry.entry.schemaId = std::string(type->schemaId);
  entry.entry.schemaVersion = type->version;
  entry.entry.simulationRelevant = type->simulationRelevant;
  entry.bytes = std::move(*bytes);
  return entry;
}

} // namespace ContentBuild
