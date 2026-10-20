#pragma once

#include "AuthoringParser.h"
#include "ContentDiagnostic.h"
#include "RecordReader.h"

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace SchemaGen
{

// The logical field types of table 3.1-A that M0's subset reads (ADR-098).
enum class FieldKind : std::uint8_t
{
  Object,
  Integer,
  Count,
  Ticks,
  SimDecimal
};

/// One field of a schema, as the generator writes it.
struct FieldModel
{
  std::string key;        // snake_case, as authored
  std::string identifier; // camelCase, the generated field
  std::string typeName;   // PascalCase, an object's generated struct
  std::string path;       // dotted from the record's root: for a tuning record, the tuning key
  std::string unit;
  FieldKind kind = FieldKind::Object;
  std::int64_t integerMinimum = 0;
  std::optional<double> minimum;
  std::optional<double> exclusiveMinimum;
  std::optional<double> maximum;
  Neuron::SimPrecision precision = Neuron::SimPrecision::Binary64;
  std::vector<FieldModel> fields; // an object's, in schema order
};

/// One schema document of the restricted subset of ADR-062 and DAT-015.
struct SchemaModel
{
  std::string file; // relative to the repository root, with forward slashes
  std::string id;   // the schema id of table 1-A
  Neuron::SchemaVersion version;
  std::string typeName;     // PascalCase of the id's segments
  std::string constantName; // UPPER_CASE of the id's segments
  std::string title;
  std::string project;   // x-species-project: the only folder its header is written to
  std::string nameSpace; // R9: Neuron for an engine project, Species for a game project
  bool simulationRelevant = false;
  std::vector<FieldModel> fields; // the record's own fields, without `schema` and `version`
};

// Reads a schema document, refusing every keyword, annotation and form outside the subset, every identifier that R11
// would spell otherwise and every C++ keyword (DAT-015, DAT-054).
[[nodiscard]] std::expected<SchemaModel, std::vector<Neuron::ContentDiagnostic>> ReadSchema(const Neuron::JsonDocument& _document);

// "tuning.mvp" → "TuningMvp"; "loss_cap_store_share" → "lossCapStoreShare" (ADR-062: the generator only converts case).
[[nodiscard]] std::string PascalCase(std::string_view _name);
[[nodiscard]] std::string CamelCase(std::string_view _name);

} // namespace SchemaGen
