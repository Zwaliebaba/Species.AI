#include "pch.h"

#include "ContentDiagnostic.h"

#include <array>
#include <format>

namespace Neuron
{

std::string_view DiagnosticClassName(DiagnosticClass _class) noexcept
{
  static constexpr std::array<std::string_view, 21> NAMES{
    "syntax",        "encoding",       "depth",          "duplicate_key",   "duplicate_id", "unknown_field",  "unknown_value",
    "missing_field", "derived_field",  "wrong_type",     "out_of_range",    "too_long",     "zero_direction", "positional_table",
    "missing_asset", "missing_record", "missing_string", "reference_cycle", "family_rule",  "version",        "content_warning"};
  const auto index = static_cast<std::size_t>(_class);
  return index < NAMES.size() ? NAMES[index] : std::string_view{"unknown"};
}

std::string FormatDiagnostic(const ContentDiagnostic& _diagnostic)
{
  std::string text = std::format("{}:{}:{}: {}", _diagnostic.at.file, _diagnostic.at.line, _diagnostic.at.column,
                                 DiagnosticClassName(_diagnostic.diagnosticClass));
  if (!_diagnostic.at.fieldPath.empty())
  {
    text += std::format(": {}", _diagnostic.at.fieldPath);
  }
  text += std::format(": {}", _diagnostic.message);
  if (_diagnostic.also)
  {
    text += std::format(" (also at {}:{})", _diagnostic.also->line, _diagnostic.also->column);
  }
  return text;
}

} // namespace Neuron
