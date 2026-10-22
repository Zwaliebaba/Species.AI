#include "pch.h"

#include "MessageFormat.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <format>
#include <optional>
#include <vector>

namespace Neuron
{

namespace
{

const MessageArgument* Find(std::span<const MessageArgument> _arguments, std::string_view _name) noexcept
{
  const auto found = std::ranges::find(_arguments, _name, &MessageArgument::name);
  return found == _arguments.end() ? nullptr : &*found;
}

// An argument's number; none for a text.
std::optional<double> NumberOf(const MessageArgument& _argument) noexcept
{
  if (const auto* whole = std::get_if<std::int64_t>(&_argument.value))
  {
    return static_cast<double>(*whole);
  }
  if (const auto* number = std::get_if<double>(&_argument.value))
  {
    return *number;
  }
  return std::nullopt;
}

// A number in the language's form: a whole number as one, anything else with its fraction.
std::string NumberText(double _value, const MessageLocale& _locale)
{
  constexpr double INT64_EDGE = 9.2e18;
  if (std::isfinite(_value) && std::trunc(_value) == _value && std::abs(_value) < INT64_EDGE)
  {
    return _locale.FormatInteger(static_cast<std::int64_t>(_value));
  }
  return _locale.FormatDecimal(_value);
}

std::string ValueText(const MessageArgument& _argument, const MessageLocale& _locale)
{
  if (const auto* whole = std::get_if<std::int64_t>(&_argument.value))
  {
    return _locale.FormatInteger(*whole);
  }
  if (const auto* number = std::get_if<double>(&_argument.value))
  {
    return _locale.FormatDecimal(*number);
  }
  return std::string(std::get<std::string_view>(_argument.value));
}

const MessageCase* CaseOf(const MessagePart& _part, std::string_view _selector) noexcept
{
  const auto found = std::ranges::find(_part.cases, _selector, &MessageCase::selector);
  return found == _part.cases.end() ? nullptr : &*found;
}

const MessageCase* PluralCase(const MessagePart& _part, std::optional<double> _number, const MessageLocale& _locale)
{
  if (!_number.has_value())
  {
    return CaseOf(_part, "other");
  }
  for (const MessageCase& entry : _part.cases)
  {
    double exact = 0.0;
    const std::string_view selector = entry.selector;
    if (selector.starts_with('=') && std::from_chars(selector.data() + 1, selector.data() + selector.size(), exact).ec == std::errc{} &&
        exact == *_number)
    {
      return &entry;
    }
  }
  const std::string category = _locale.PluralCategory(*_number - _part.offset, _part.kind == MessagePartKind::SelectOrdinal);
  const MessageCase* chosen = CaseOf(_part, category);
  return chosen != nullptr ? chosen : CaseOf(_part, "other");
}

void Append(std::string& _out, const std::vector<MessagePart>& _parts, std::span<const MessageArgument> _arguments,
            const MessageLocale& _locale, std::optional<double> _pound)
{
  for (const MessagePart& part : _parts)
  {
    if (part.kind == MessagePartKind::Text)
    {
      _out += part.text;
      continue;
    }
    if (part.kind == MessagePartKind::Pound)
    {
      _out += _pound.has_value() ? NumberText(*_pound, _locale) : std::string("#");
      continue;
    }
    const MessageArgument* argument = Find(_arguments, part.text);
    if (argument == nullptr)
    {
      _out += std::format("{{{}}}", part.text);
      continue;
    }
    switch (part.kind)
    {
    case MessagePartKind::Argument:
      _out += ValueText(*argument, _locale);
      break;
    case MessagePartKind::Number:
      if (const std::optional<double> number = NumberOf(*argument); number.has_value() && part.integer)
      {
        _out += NumberText(std::round(*number), _locale);
      }
      else
      {
        _out += ValueText(*argument, _locale);
      }
      break;
    case MessagePartKind::Plural:
    case MessagePartKind::SelectOrdinal:
    {
      const std::optional<double> number = NumberOf(*argument);
      if (const MessageCase* chosen = PluralCase(part, number, _locale); chosen != nullptr)
      {
        const std::optional<double> pound = number.has_value() ? std::optional<double>(*number - part.offset) : std::nullopt;
        Append(_out, chosen->parts, _arguments, _locale, pound);
      }
      break;
    }
    case MessagePartKind::Select:
    {
      const std::string key = std::holds_alternative<std::string_view>(argument->value)
                                ? std::string(std::get<std::string_view>(argument->value))
                                : std::format("{}", NumberOf(*argument).value_or(0.0));
      const MessageCase* chosen = CaseOf(part, key);
      if (chosen == nullptr)
      {
        chosen = CaseOf(part, "other");
      }
      if (chosen != nullptr)
      {
        Append(_out, chosen->parts, _arguments, _locale, std::nullopt);
      }
      break;
    }
    case MessagePartKind::Text:
    case MessagePartKind::Pound:
      break;
    }
  }
}

} // namespace

std::string FormatPattern(const MessagePattern& _pattern, std::span<const MessageArgument> _arguments, const MessageLocale& _locale)
{
  std::string out;
  Append(out, _pattern.parts, _arguments, _locale, std::nullopt);
  return out;
}

} // namespace Neuron
