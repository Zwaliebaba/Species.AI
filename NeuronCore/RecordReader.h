#pragma once

#include "AuthoringParser.h"
#include "ContentDiagnostic.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// DAT-010, ADR-081: the precision a simulation decimal is held at.
enum class SimPrecision : std::uint8_t
{
  Binary32Rounded,
  Binary64
};

// dat.notes.max_chars: the longest note an object may carry (DAT-016).
inline constexpr std::size_t NOTES_MAX_CHARS = 2000;

// A record read whole, or every finding about it (DAT-024).
template <class T> using RecordResult = std::expected<T, std::vector<ContentDiagnostic>>;

// A decimal field's declared range; an absent bound does not apply.
struct DecimalRange
{
  std::optional<double> minimum = std::nullopt;          // inclusive
  std::optional<double> exclusiveMinimum = std::nullopt; // exclusive
  std::optional<double> maximum = std::nullopt;          // inclusive
};

/// An object of a record being read: its value, its dotted path, and which of its members the reader has taken. A
/// null value stands for an object that is absent or not an object, which has been reported once.
struct RecordObject
{
  const JsonValue* value = nullptr;
  std::string path;
  std::vector<bool> taken;
};

/// What the readers generated from the schemas call (ADR-062, DAT-024 step 2): the checks of a record's header, its
/// fields, their JSON forms and their ranges. Every field is required at M0; a missing one is reported where it is
/// read, and a member that no read took, other than `notes`, is reported as unknown when its object closes. Every
/// finding is collected, not only the first, and a value that failed reads as 0, which no caller sees because the
/// record is refused (DAT-012).
class RecordReader
{
public:
  explicit RecordReader(std::string_view _file);

  // DAT-016: the record's top-level object, whose `schema` and `version` must name this schema at this version.
  [[nodiscard]] RecordObject Record(const JsonValue& _root, std::string_view _schemaId, SchemaVersion _version);
  // A nested object field.
  [[nodiscard]] RecordObject Object(RecordObject& _parent, std::string_view _key);
  // DAT-023: an integer takes its decimal digits exactly; a fraction, an exponent or a value beyond 64 bits is refused.
  [[nodiscard]] std::int64_t Integer(RecordObject& _object, std::string_view _key, std::int64_t _minimum);
  // DAT-010: a simulation decimal, held at its precision.
  [[nodiscard]] double Decimal(RecordObject& _object, std::string_view _key, const DecimalRange& _range, SimPrecision _precision);
  // The end of an object: every member no read took is unknown (DAT-026), and `notes` is checked (DAT-016).
  void Close(const RecordObject& _object);

  [[nodiscard]] bool Failed() const noexcept
  {
    return !m_diagnostics.empty();
  }
  [[nodiscard]] std::vector<ContentDiagnostic> TakeDiagnostics();

private:
  [[nodiscard]] const JsonValue* Take(RecordObject& _object, std::string_view _key);
  [[nodiscard]] RecordObject Open(const JsonValue* _value, std::string _path);
  void Report(DiagnosticClass _class, std::uint32_t _line, std::uint32_t _column, std::string_view _path, std::string_view _message);

  std::string m_file;
  std::vector<ContentDiagnostic> m_diagnostics;
};

} // namespace Neuron
