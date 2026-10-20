#pragma once

#include "AuthoringParser.h"
#include "ContentDiagnostic.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
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

// The bound that an integer field or a list without a declared maximum is read with.
inline constexpr std::int64_t INTEGER_NO_MAXIMUM = std::numeric_limits<std::int64_t>::max();
inline constexpr std::size_t LIST_NO_MAXIMUM = std::numeric_limits<std::size_t>::max();

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

/// A list field of a record being read: its value and its dotted path. A null value stands for a list that is absent or
/// not a list, which has been reported once, and for a defaulted list that the object omits, which is empty.
struct RecordList
{
  const JsonValue* value = nullptr;
  std::string path;

  [[nodiscard]] std::size_t Size() const noexcept
  {
    return value == nullptr ? 0 : value->items.size();
  }
};

/// What the readers generated from the schemas call (ADR-062, DAT-024 step 2): the checks of a record's header, its
/// fields, their JSON forms and their ranges. A required field that is missing is reported where it is read, a
/// defaulted one takes its declared default (DAT-011, DAT-012), and a member that no read took, other than `notes`, is
/// reported as unknown when its object closes. Every finding is collected, not only the first, and a value that failed
/// reads as 0, which no caller sees because the record is refused (DAT-012).
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
  [[nodiscard]] std::int64_t Integer(RecordObject& _object, std::string_view _key, std::int64_t _minimum, std::int64_t _maximum);
  // A defaulted integer: its declared default when the object omits it (DAT-012).
  [[nodiscard]] std::int64_t DefaultedInteger(RecordObject& _object, std::string_view _key, std::int64_t _minimum, std::int64_t _maximum,
                                              std::int64_t _default);
  // DAT-010: a simulation decimal, held at its precision.
  [[nodiscard]] double Decimal(RecordObject& _object, std::string_view _key, const DecimalRange& _range, SimPrecision _precision);
  // DAT-009: a text of at most _maxChars code points, refused rather than shortened when longer.
  [[nodiscard]] std::string Text(RecordObject& _object, std::string_view _key, std::size_t _maxChars);
  // A list of _minItems to _maxItems elements; a defaulted list is empty when the object omits it.
  [[nodiscard]] RecordList List(RecordObject& _object, std::string_view _key, std::size_t _minItems, std::size_t _maxItems);
  [[nodiscard]] RecordList DefaultedList(RecordObject& _object, std::string_view _key, std::size_t _minItems, std::size_t _maxItems);
  // A list's element as an object, and as an integer; the path of element n is "<list>[n]".
  [[nodiscard]] RecordObject Element(const RecordList& _list, std::size_t _index);
  [[nodiscard]] std::int64_t ElementInteger(const RecordList& _list, std::size_t _index, std::int64_t _minimum, std::int64_t _maximum);
  // The end of an object: every member no read took is unknown (DAT-026), and `notes` is checked (DAT-016).
  void Close(const RecordObject& _object);

  [[nodiscard]] bool Failed() const noexcept
  {
    return !m_diagnostics.empty();
  }
  [[nodiscard]] std::vector<ContentDiagnostic> TakeDiagnostics();

private:
  [[nodiscard]] const JsonValue* Take(RecordObject& _object, std::string_view _key);
  // Take for a defaulted field: an absent member is no finding.
  [[nodiscard]] const JsonValue* TakeIfPresent(RecordObject& _object, std::string_view _key);
  [[nodiscard]] std::int64_t IntegerValue(const JsonValue* _value, const std::string& _path, std::int64_t _minimum, std::int64_t _maximum);
  [[nodiscard]] RecordList ListValue(const JsonValue* _value, std::string _path, std::size_t _minItems, std::size_t _maxItems);
  [[nodiscard]] RecordObject Open(const JsonValue* _value, std::string _path);
  void Report(DiagnosticClass _class, std::uint32_t _line, std::uint32_t _column, std::string_view _path, std::string_view _message);

  std::string m_file;
  std::vector<ContentDiagnostic> m_diagnostics;
};

} // namespace Neuron
