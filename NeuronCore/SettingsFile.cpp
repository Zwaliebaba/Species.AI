#include "pch.h"

#include "SettingsFile.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <memory>
#include <set>
#include <system_error>

namespace Neuron
{

namespace
{

constexpr std::string_view UTF8_BYTE_ORDER_MARK = "\xEF\xBB\xBF";

bool IsBlank(char _c) noexcept
{
  return _c == ' ' || _c == '\t';
}

std::string_view TrimBlanks(std::string_view _text) noexcept
{
  while (!_text.empty() && IsBlank(_text.front()))
  {
    _text.remove_prefix(1);
  }
  while (!_text.empty() && IsBlank(_text.back()))
  {
    _text.remove_suffix(1);
  }
  return _text;
}

// Whether the bytes are well-formed UTF-8: no overlong form, no surrogate and nothing above U+10FFFF.
bool IsUtf8(std::string_view _text) noexcept
{
  std::size_t i = 0;
  while (i < _text.size())
  {
    const auto lead = static_cast<unsigned char>(_text[i]);
    std::size_t length = 0;
    char32_t point = 0;
    char32_t least = 0;
    if (lead < 0x80)
    {
      ++i;
      continue;
    }
    if ((lead & 0xE0) == 0xC0)
    {
      length = 2;
      point = lead & 0x1Fu;
      least = 0x80;
    }
    else if ((lead & 0xF0) == 0xE0)
    {
      length = 3;
      point = lead & 0x0Fu;
      least = 0x800;
    }
    else if ((lead & 0xF8) == 0xF0)
    {
      length = 4;
      point = lead & 0x07u;
      least = 0x10000;
    }
    else
    {
      return false;
    }
    if (i + length > _text.size())
    {
      return false;
    }
    for (std::size_t k = 1; k < length; ++k)
    {
      const auto next = static_cast<unsigned char>(_text[i + k]);
      if ((next & 0xC0) != 0x80)
      {
        return false;
      }
      point = (point << 6) | (next & 0x3Fu);
    }
    if (point < least || point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF))
    {
      return false;
    }
    i += length;
  }
  return true;
}

SettingsLine ClassifyLine(std::string_view _line)
{
  SettingsLine line;
  line.text = std::string(_line);
  if (_line.size() > SETTINGS_LINE_BYTES_MAX || !IsUtf8(_line))
  {
    return line;
  }
  const std::string_view trimmed = TrimBlanks(_line);
  if (trimmed.empty())
  {
    line.kind = SettingsLine::Kind::Blank;
    return line;
  }
  if (trimmed.front() == '#')
  {
    line.kind = SettingsLine::Kind::Comment;
    return line;
  }
  const std::size_t equals = trimmed.find('=');
  if (equals == std::string_view::npos)
  {
    return line;
  }
  const std::string_view key = TrimBlanks(trimmed.substr(0, equals));
  if (!IsSettingKey(key))
  {
    return line;
  }
  line.kind = SettingsLine::Kind::Entry;
  line.key = std::string(key);
  line.value = std::string(TrimBlanks(trimmed.substr(equals + 1)));
  return line;
}

std::optional<std::int64_t> IntegerOf(std::string_view _text) noexcept
{
  if (_text.empty())
  {
    return std::nullopt;
  }
  std::int64_t value = 0;
  const char* first = std::to_address(_text.begin());
  const char* end = first + _text.size();
  const auto [last, error] = std::from_chars(first, end, value);
  if (error != std::errc{} || last != end)
  {
    return std::nullopt;
  }
  return value;
}

std::optional<double> DecimalOf(std::string_view _text) noexcept
{
  if (_text.empty())
  {
    return std::nullopt;
  }
  double value = 0.0;
  const char* first = std::to_address(_text.begin());
  const char* end = first + _text.size();
  const auto [last, error] = std::from_chars(first, end, value, std::chars_format::general);
  if (error != std::errc{} || last != end || !std::isfinite(value))
  {
    return std::nullopt;
  }
  return value;
}

} // namespace

bool IsSettingKey(std::string_view _key) noexcept
{
  if (_key.empty() || _key.size() > SETTING_KEY_CHARS_MAX)
  {
    return false;
  }
  return std::ranges::all_of(_key, [](char _c) { return (_c >= 'a' && _c <= 'z') || (_c >= '0' && _c <= '9') || _c == '_' || _c == '.'; });
}

const SettingSpec* FindSettingSpec(std::span<const SettingSpec> _specs, std::string_view _key) noexcept
{
  for (const SettingSpec& spec : _specs)
  {
    const bool family = spec.key.ends_with('.');
    if (family ? (_key.size() > spec.key.size() && _key.starts_with(spec.key)) : _key == spec.key)
    {
      return &spec;
    }
  }
  return nullptr;
}

bool SettingValueFits(const SettingSpec& _spec, const SettingValue& _value)
{
  switch (_spec.type)
  {
  case SettingType::Boolean:
    return std::holds_alternative<bool>(_value);
  case SettingType::Integer:
  {
    const auto* value = std::get_if<std::int64_t>(&_value);
    return value != nullptr && static_cast<double>(*value) >= _spec.minimum && static_cast<double>(*value) <= _spec.maximum;
  }
  case SettingType::Decimal:
  {
    const auto* value = std::get_if<double>(&_value);
    return value != nullptr && std::isfinite(*value) && *value >= _spec.minimum && *value <= _spec.maximum;
  }
  case SettingType::Enumeration:
  {
    const auto* value = std::get_if<std::string>(&_value);
    return value != nullptr && std::ranges::find(_spec.enumerators, *value) != _spec.enumerators.end();
  }
  case SettingType::Text:
  case SettingType::Binding:
  {
    // DAT-197: a text cannot hold a line break.
    const auto* value = std::get_if<std::string>(&_value);
    return value != nullptr && value->find_first_of("\r\n") == std::string::npos && value->size() <= SETTINGS_LINE_BYTES_MAX &&
           IsUtf8(*value) && (_spec.type == SettingType::Text || !value->empty());
  }
  }
  return false;
}

std::optional<SettingValue> ParseSettingValue(const SettingSpec& _spec, std::string_view _text)
{
  std::optional<SettingValue> value;
  switch (_spec.type)
  {
  case SettingType::Boolean:
    if (_text == "true" || _text == "false")
    {
      value = _text == "true";
    }
    break;
  case SettingType::Integer:
    if (const std::optional<std::int64_t> integer = IntegerOf(_text))
    {
      value = *integer;
    }
    break;
  case SettingType::Decimal:
    if (const std::optional<double> decimal = DecimalOf(_text))
    {
      value = *decimal;
    }
    break;
  case SettingType::Text:
  case SettingType::Enumeration:
  case SettingType::Binding:
    value = std::string(_text);
    break;
  }
  if (value.has_value() && !SettingValueFits(_spec, *value))
  {
    value.reset();
  }
  return value;
}

std::string FormatSettingValue(const SettingValue& _value)
{
  if (const auto* flag = std::get_if<bool>(&_value))
  {
    return *flag ? "true" : "false";
  }
  if (const auto* integer = std::get_if<std::int64_t>(&_value))
  {
    return std::to_string(*integer);
  }
  if (const auto* decimal = std::get_if<double>(&_value))
  {
    std::array<char, 32> buffer{};
    const auto [last, error] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), *decimal);
    return error == std::errc{} ? std::string(buffer.data(), last) : std::string("0");
  }
  return std::get<std::string>(_value);
}

std::vector<SettingsLine> ReadSettingsLines(std::span<const std::byte> _bytes)
{
  std::string_view text(reinterpret_cast<const char*>(_bytes.data()), _bytes.size());
  if (text.starts_with(UTF8_BYTE_ORDER_MARK))
  {
    text.remove_prefix(UTF8_BYTE_ORDER_MARK.size());
  }
  std::vector<SettingsLine> lines;
  while (!text.empty())
  {
    const std::size_t end = text.find('\n');
    std::string_view line = text.substr(0, end);
    text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
    if (line.ends_with('\r'))
    {
      line.remove_suffix(1);
    }
    lines.push_back(ClassifyLine(line));
  }
  return lines;
}

std::string WriteSettingsText(std::span<const SettingsLine> _loaded, std::span<const SettingsEntryText> _current)
{
  const auto valueOf = [&](std::string_view _key) -> const std::string*
  {
    const auto found = std::ranges::find(_current, _key, &SettingsEntryText::key);
    return found == _current.end() ? nullptr : &found->value;
  };
  std::string out;
  std::set<std::string, std::less<>> written;
  for (const SettingsLine& line : _loaded)
  {
    if (line.kind != SettingsLine::Kind::Entry)
    {
      out += line.text;
      out += '\n';
      continue;
    }
    if (!written.insert(line.key).second)
    {
      continue; // DAT-201: a repeated key's later lines are not written
    }
    if (const std::string* value = valueOf(line.key))
    {
      out += line.key + " = " + *value + '\n';
    }
  }
  std::vector<const SettingsEntryText*> added;
  for (const SettingsEntryText& entry : _current)
  {
    if (!written.contains(entry.key))
    {
      added.push_back(&entry);
    }
  }
  std::ranges::sort(added, {}, [](const SettingsEntryText* _entry) -> const std::string& { return _entry->key; });
  for (const SettingsEntryText* entry : added)
  {
    out += entry->key + " = " + entry->value + '\n';
  }
  return out;
}

} // namespace Neuron
