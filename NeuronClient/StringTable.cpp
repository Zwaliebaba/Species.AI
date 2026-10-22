#include "pch.h"

#include "AssetError.h"
#include "Development.h"
#include "Log.h"
#include "StringTable.h"

#include <utility>

namespace Neuron
{

namespace
{

// A text the content build validated parses; should one not, its text is shown as written.
MessagePattern ParsedOrLiteral(const std::string& _id, const std::string& _text)
{
  auto pattern = ParseMessage(_text);
  if (!pattern)
  {
    Log(LogCategory::Ui, LogLevel::Error, "the string {} is not a message ({}); it is shown as written", _id, pattern.error().what);
    return MessagePattern{{MessagePart{MessagePartKind::Text, _text, false, 0.0, {}}}};
  }
  return std::move(*pattern);
}

} // namespace

StringTable::StringTable(StringTableAsset _asset)
  : m_asset(std::move(_asset))
{
  m_messages.reserve(m_asset.entries.size());
  for (const StringEntry& entry : m_asset.entries)
  {
    m_messages.push_back({ParsedOrLiteral(entry.id, entry.keyboardMouse), ParsedOrLiteral(entry.id, entry.gamepad)});
  }
}

std::size_t StringTable::IndexOf(std::string_view _id) const noexcept
{
  const StringEntry* entry = FindString(m_asset, _id);
  if (entry == nullptr || entry->embedOnly)
  {
    return m_asset.entries.size();
  }
  return static_cast<std::size_t>(entry - m_asset.entries.data());
}

std::string_view StringTable::Miss(std::string_view _id) const
{
  bool first = false;
  {
    const std::scoped_lock lock(m_missLock);
    first = m_missed.emplace(_id).second;
  }
  if (first)
  {
    Log(LogCategory::Ui, LogLevel::Warning, "the string {} is not in the {} table (UI-219)", _id,
        m_asset.language.empty() ? std::string_view("empty") : std::string_view(m_asset.language));
  }
#if ENGINE_DEVELOPMENT
  return _id;
#else
  return {};
#endif
}

std::string_view StringTable::Lookup(std::string_view _id, InputMode _mode) const
{
  const std::size_t index = IndexOf(_id);
  if (index == m_asset.entries.size())
  {
    return Miss(_id);
  }
  const StringEntry& entry = m_asset.entries[index];
  return _mode == InputMode::KeyboardMouse ? entry.keyboardMouse : entry.gamepad;
}

bool StringTable::Exists(std::string_view _id) const noexcept
{
  return IndexOf(_id) != m_asset.entries.size();
}

std::string StringTable::Format(std::string_view _id, std::span<const FormatArgument> _arguments, const MessageLocale& _locale,
                                InputMode _mode) const
{
  const std::size_t index = IndexOf(_id);
  if (index == m_asset.entries.size())
  {
    return std::string(Miss(_id));
  }
  const Messages& messages = m_messages[index];
  return FormatPattern(_mode == InputMode::KeyboardMouse ? messages.keyboardMouse : messages.gamepad, _arguments, _locale);
}

std::string StringTableKey(std::string_view _languageTag)
{
  std::string key = "strings/";
  for (const char c : _languageTag)
  {
    if (c == '-')
    {
      key += '_';
    }
    else
    {
      key += c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
    }
  }
  return key;
}

std::shared_ptr<const StringTable> LoadStringTable(const LooseContent& _content, std::string_view _languageTag)
{
  const std::string key = StringTableKey(_languageTag);
  const auto bytes = _content.Read(key);
  if (!bytes)
  {
    Log(LogCategory::Content, LogLevel::Error, "the {} string table: {}", _languageTag, FormatAssetError(bytes.error()));
    return std::make_shared<const StringTable>();
  }
  auto asset = ReadStringTableAsset(*bytes);
  if (!asset)
  {
    Log(LogCategory::Content, LogLevel::Error, "the string table {} is damaged: {}", key, asset.error());
    return std::make_shared<const StringTable>();
  }
  return std::make_shared<const StringTable>(std::move(*asset));
}

} // namespace Neuron
