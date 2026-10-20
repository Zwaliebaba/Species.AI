#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Neuron
{

// DAT-017: a schema's version.
struct SchemaVersion
{
  std::uint16_t major = 1;
  std::uint16_t minor = 0;

  friend bool operator==(const SchemaVersion&, const SchemaVersion&) = default;
};

// DAT-020: where a value is: its file, line and column (both from 1, the column in Unicode code points), and the
// dotted path of its field from the record's root.
struct SourceLocation
{
  std::string file;
  std::uint32_t line = 0;
  std::uint32_t column = 0;
  std::string fieldPath;
};

enum class Severity : std::uint8_t
{
  Error,
  Warning,
  Information
};

// The classes of table 6-A, in its order.
enum class DiagnosticClass : std::uint8_t
{
  Syntax,
  Encoding,
  Depth,
  DuplicateKey,
  DuplicateId,
  UnknownField,
  UnknownValue,
  MissingField,
  DerivedField,
  WrongType,
  OutOfRange,
  TooLong,
  ZeroDirection,
  PositionalTable,
  MissingAsset,
  MissingRecord,
  MissingString,
  ReferenceCycle,
  FamilyRule,
  Version,
  ContentWarning
};

// The class's name in table 6-A, as diagnostics print it.
[[nodiscard]] std::string_view DiagnosticClassName(DiagnosticClass _class) noexcept;

/// One finding about authored content (DAT-051): its class, severity and location, and for a duplicate the other
/// occurrence. The message is literal text, never a format string (DEC-800).
struct ContentDiagnostic
{
  DiagnosticClass diagnosticClass = DiagnosticClass::Syntax;
  Severity severity = Severity::Error;
  SourceLocation at;
  std::optional<SourceLocation> also;
  std::string message;
};

// "<file>:<line>:<column>: <class>: <field path>: <message>", one line, for the log and the build report.
[[nodiscard]] std::string FormatDiagnostic(const ContentDiagnostic& _diagnostic);

} // namespace Neuron
