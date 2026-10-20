#pragma once

#include "AuthoringParser.h"
#include "ContentDiagnostic.h"
#include "RecordReader.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace SchemaGen
{

// The logical field types of table 3.1-A that the generator's subset reads (ADR-098, ADR-108).
enum class FieldKind : std::uint8_t
{
  Object,
  Integer,
  Count,
  Ticks,
  SimDecimal,
  AssetRef,
  List
};

/// One field of a schema, as the generator writes it. A list's element is a field of its own, without a key.
struct FieldModel
{
  std::string key;        // snake_case, as authored
  std::string identifier; // camelCase, the generated field
  std::string typeName;   // PascalCase, an object's generated struct; <Key>Item for a list's object element
  std::string path;       // dotted from the record's root: for a tuning record, the tuning key
  std::string unit;
  FieldKind kind = FieldKind::Object;
  bool defaulted = false; // DAT-011: the field may be omitted, and then takes its declared default
  std::int64_t integerMinimum = 0;
  std::optional<std::int64_t> integerMaximum;
  std::int64_t integerDefault = 0;
  std::optional<double> minimum;
  std::optional<double> exclusiveMinimum;
  std::optional<double> maximum;
  Neuron::SimPrecision precision = Neuron::SimPrecision::Binary64;
  std::size_t maxChars = 0; // an asset reference's x-species-max-chars (DAT-009)
  std::string refKind;      // an asset reference's x-species-ref: the asset category it names
  std::size_t minItems = 0; // a list's
  std::optional<std::size_t> maxItems;
  bool orderMeaningful = false;   // a list's x-species-order (DAT-013)
  std::vector<FieldModel> fields; // an object's, in schema order
  std::vector<FieldModel> item;   // a list's element, alone
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
  std::string nameSpace; // R9: Neuron for an engine project, Species for a game project, <Name> for Tools/Species<Name>
  bool simulationRelevant = false;
  bool cooked = false;            // every field has a fixed size, so the header holds the cooked record's writer and reader (DAT-014)
  std::vector<FieldModel> fields; // the record's own fields, without `schema` and `version`
};

// Reads a schema document, refusing every keyword, annotation and form outside the subset, every identifier that R11
// would spell otherwise and every C++ keyword (DAT-015, DAT-054).
[[nodiscard]] std::expected<SchemaModel, std::vector<Neuron::ContentDiagnostic>> ReadSchema(const Neuron::JsonDocument& _document);

// "tuning.mvp" → "TuningMvp"; "loss_cap_store_share" → "lossCapStoreShare" (ADR-062: the generator only converts case).
[[nodiscard]] std::string PascalCase(std::string_view _name);
[[nodiscard]] std::string CamelCase(std::string_view _name);

} // namespace SchemaGen
