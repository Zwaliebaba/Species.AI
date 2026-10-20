#include "pch.h"

#include "RowSpec.h"

#include <limits>

namespace ContentBuild
{

namespace
{

bool IsDigit(char _c) noexcept
{
  return _c >= '0' && _c <= '9';
}

// The digits at the start of the text and how many there are; none when there are none or they pass 64 bits.
std::optional<std::uint64_t> LeadingNumber(std::string_view _text, std::size_t& _length) noexcept
{
  _length = 0;
  std::uint64_t value = 0;
  while (_length < _text.size() && IsDigit(_text[_length]))
  {
    const auto digit = static_cast<std::uint64_t>(_text[_length] - '0');
    if (value > (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
    {
      return std::nullopt;
    }
    value = value * 10 + digit;
    ++_length;
  }
  return _length == 0 ? std::nullopt : std::optional<std::uint64_t>(value);
}

} // namespace

std::optional<PixelSize> PixelSizeAfter(std::string_view _text, std::string_view _phrase)
{
  std::size_t from = 0;
  if (!_phrase.empty())
  {
    from = _text.find(_phrase);
    if (from == std::string_view::npos)
    {
      return std::nullopt;
    }
    from += _phrase.size();
  }
  for (std::size_t at = from; at < _text.size(); ++at)
  {
    if (!IsDigit(_text[at]) || (at > 0 && IsDigit(_text[at - 1])))
    {
      continue;
    }
    std::size_t widthDigits = 0;
    const std::optional<std::uint64_t> width = LeadingNumber(_text.substr(at), widthDigits);
    const std::size_t times = at + widthDigits;
    if (!width || times >= _text.size() || _text[times] != 'x')
    {
      continue;
    }
    std::size_t heightDigits = 0;
    const std::optional<std::uint64_t> height = LeadingNumber(_text.substr(times + 1), heightDigits);
    const std::string_view after = _text.substr(times + 1 + heightDigits);
    const std::uint64_t limit = std::numeric_limits<std::uint32_t>::max();
    if (height && after.starts_with(" px") && width.value_or(0) <= limit && height.value_or(0) <= limit)
    {
      return PixelSize{static_cast<std::uint32_t>(width.value_or(0)), static_cast<std::uint32_t>(height.value_or(0))};
    }
  }
  return std::nullopt;
}

std::optional<std::uint64_t> NumberAfter(std::string_view _text, std::string_view _phrase)
{
  const std::size_t at = _text.find(_phrase);
  if (at == std::string_view::npos)
  {
    return std::nullopt;
  }
  std::size_t digits = 0;
  return LeadingNumber(_text.substr(at + _phrase.size()), digits);
}

std::optional<std::uint64_t> NumberBefore(std::string_view _text, std::string_view _phrase)
{
  const std::size_t at = _text.find(_phrase);
  if (at == std::string_view::npos || at == 0)
  {
    return std::nullopt;
  }
  std::size_t start = at;
  while (start > 0 && IsDigit(_text[start - 1]))
  {
    --start;
  }
  std::size_t digits = 0;
  const std::optional<std::uint64_t> value = LeadingNumber(_text.substr(start, at - start), digits);
  return digits == at - start ? value : std::nullopt;
}

} // namespace ContentBuild
