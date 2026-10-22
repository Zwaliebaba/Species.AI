#pragma once

#include "MessageLocale.h"
#include "MessagePattern.h"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <variant>

namespace Neuron
{

/// A named argument of a message (UI-227): its name and its value, a whole number, a number or a text. A text is
/// inserted literally, so none of its characters is read as message syntax (DEC-102).
struct FormatArgument
{
  std::string_view name;
  std::variant<std::int64_t, double, std::string_view> value;
};

// ADR-063 and ADR-146: _pattern with _arguments inserted, in _locale's number formats and plural rules.
// * A plain argument writes its text, or its number in the language's form.
// * A number argument writes its number in the language's form, rounded to a whole number in the integer style.
// * A plural or select-ordinal argument takes the case of its exact number (=n) if one is given, else the case of the
//   category of its number less the offset, else other; # in that case writes the number less the offset.
// * A select argument takes the case of its text, else other.
// An argument the call does not give is written as {name}, as ICU writes it.
[[nodiscard]] std::string FormatPattern(const MessagePattern& _pattern, std::span<const FormatArgument> _arguments,
                                        const MessageLocale& _locale);

} // namespace Neuron
