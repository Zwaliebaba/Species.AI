#include "pch.h"

#include "ContentKey.h"

#include <array>

namespace Neuron
{

std::string_view ContentTypeName(ContentType _type) noexcept
{
  static constexpr std::array<std::string_view, CONTENT_TYPE_COUNT> NAMES{
    "texture",   "cpu_table",    "mesh", "collision",      "sim_record",   "record",           "map",
    "map_index", "string_table", "font", "resident_sound", "sound_stream", "explainer_script", "asset_index"};
  const auto index = static_cast<std::size_t>(_type);
  return index < NAMES.size() ? NAMES[index] : std::string_view{"unknown"};
}

bool IsCanonicalKey(std::string_view _key) noexcept
{
  if (_key.empty() || _key.size() > MAX_KEY_BYTES || _key.front() == '/' || _key.back() == '/' || _key.find("//") != std::string_view::npos)
  {
    return false;
  }
  for (const char c : _key)
  {
    const bool allowed = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.' || c == '/' || c == '@';
    if (!allowed)
    {
      return false;
    }
  }
  return true;
}

std::string CanonicalKey(std::string_view _requested)
{
  std::string key(_requested);
  for (char& c : key)
  {
    if (c >= 'A' && c <= 'Z')
    {
      c = static_cast<char>(c - 'A' + 'a');
    }
    else if (c == '\\')
    {
      c = '/';
    }
  }
  return key;
}

std::string KeyOfSource(std::string_view _relativePath)
{
  const std::size_t slash = _relativePath.find_last_of("/\\");
  const std::size_t dot = _relativePath.find_last_of('.');
  const bool hasExtension = dot != std::string_view::npos && (slash == std::string_view::npos || dot > slash);
  return CanonicalKey(hasExtension ? _relativePath.substr(0, dot) : _relativePath);
}

} // namespace Neuron
