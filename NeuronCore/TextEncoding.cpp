#include "pch.h"

#include "TextEncoding.h"

#include <climits>

namespace Neuron
{

std::wstring ToUtf16(std::string_view _utf8)
{
  std::wstring text;
  // The conversion takes an int length; a longer string is converted in parts that end on a sequence boundary.
  while (!_utf8.empty())
  {
    std::size_t partBytes = _utf8.size() < static_cast<std::size_t>(INT_MAX / 2) ? _utf8.size() : INT_MAX / 2;
    while (partBytes < _utf8.size() && partBytes > 0 && (static_cast<unsigned char>(_utf8[partBytes]) & 0xC0) == 0x80)
    {
      --partBytes;
    }
    const std::string_view part = _utf8.substr(0, partBytes);
    const int units = MultiByteToWideChar(CP_UTF8, 0, part.data(), static_cast<int>(part.size()), nullptr, 0);
    if (units <= 0)
    {
      break;
    }
    const std::size_t start = text.size();
    text.resize(start + static_cast<std::size_t>(units));
    MultiByteToWideChar(CP_UTF8, 0, part.data(), static_cast<int>(part.size()), text.data() + start, units);
    _utf8.remove_prefix(partBytes);
  }
  return text;
}

std::string ToUtf8(std::wstring_view _utf16)
{
  std::string text;
  while (!_utf16.empty())
  {
    std::size_t partUnits = _utf16.size() < static_cast<std::size_t>(INT_MAX / 4) ? _utf16.size() : INT_MAX / 4;
    // Never split a surrogate pair.
    if (partUnits < _utf16.size() && _utf16[partUnits - 1] >= 0xD800 && _utf16[partUnits - 1] <= 0xDBFF)
    {
      --partUnits;
    }
    const std::wstring_view part = _utf16.substr(0, partUnits);
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, part.data(), static_cast<int>(part.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0)
    {
      break;
    }
    const std::size_t start = text.size();
    text.resize(start + static_cast<std::size_t>(bytes));
    WideCharToMultiByte(CP_UTF8, 0, part.data(), static_cast<int>(part.size()), text.data() + start, bytes, nullptr, nullptr);
    _utf16.remove_prefix(partUnits);
  }
  return text;
}

} // namespace Neuron
