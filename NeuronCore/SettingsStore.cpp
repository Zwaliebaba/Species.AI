#include "pch.h"

#include "SettingsStore.h"

#include "Check.h"
#include "Log.h"
#include "TextEncoding.h"

#include <algorithm>
#include <set>
#include <utility>

namespace Neuron
{

namespace
{

std::string_view FileErrorName(FileError _error) noexcept
{
  switch (_error)
  {
  case FileError::NotFound:
    return "not found";
  case FileError::AccessDenied:
    return "access denied";
  case FileError::DiskFull:
    return "disk full";
  case FileError::Other:
    break;
  }
  return "other";
}

} // namespace

SettingsStore::SettingsStore(std::vector<SettingSpec> _specs, std::optional<std::filesystem::path> _file)
  : m_specs(std::move(_specs)),
    m_file(std::move(_file))
{
  for (const SettingSpec& spec : m_specs)
  {
    ENGINE_ASSERT(spec.key.ends_with('.') || IsSettingKey(spec.key));
    ENGINE_ASSERT(!spec.defaultValue.has_value() || SettingValueFits(spec, *spec.defaultValue));
    if (!spec.key.ends_with('.'))
    {
      m_values[spec.key].value = spec.defaultValue;
    }
  }
  Load();
}

const SettingSpec* SettingsStore::SpecOf(std::string_view _key) const noexcept
{
  return FindSettingSpec(m_specs, _key);
}

void SettingsStore::Load()
{
  if (!m_file.has_value())
  {
    return;
  }
  const std::expected<std::vector<std::byte>, FileError> bytes = ReadWholeFile(*m_file);
  if (!bytes.has_value())
  {
    // PLT-080: no file is a first run; a file that exists and cannot be read starts from the defaults too (DAT-200).
    if (bytes.error() != FileError::NotFound)
    {
      Log(LogCategory::Platform, LogLevel::Warning, "settings: {} cannot be read ({}); the defaults apply", ToUtf8(m_file->wstring()),
          FileErrorName(bytes.error()));
    }
    return;
  }
  m_loadedFromFile = true;
  m_loadedLines = ReadSettingsLines(*bytes);
  std::set<std::string, std::less<>> seen;
  std::size_t lineNumber = 0;
  for (const SettingsLine& line : m_loadedLines)
  {
    ++lineNumber;
    if (line.kind == SettingsLine::Kind::Other)
    {
      Log(LogCategory::Platform, LogLevel::Warning, "settings: line {} is not an entry, a comment or a blank line, and is kept",
          lineNumber);
      continue;
    }
    if (line.kind != SettingsLine::Kind::Entry)
    {
      continue;
    }
    if (!seen.insert(line.key).second)
    {
      // DAT-199: the first occurrence wins.
      Log(LogCategory::Platform, LogLevel::Warning, "settings: line {} repeats the key {} and is ignored", lineNumber, line.key);
      continue;
    }
    const SettingSpec* spec = SpecOf(line.key);
    if (spec == nullptr)
    {
      Log(LogCategory::Platform, LogLevel::Info, "settings: line {} holds the key {}, which this build does not know; it is kept",
          lineNumber, line.key);
      Stored& stored = m_values[line.key];
      stored.known = false;
      stored.unknownText = line.value;
      continue;
    }
    if (std::optional<SettingValue> value = ParseSettingValue(*spec, line.value))
    {
      m_values[line.key].value = std::move(*value);
    }
    else
    {
      // DAT-198: a malformed value leaves the setting at its default.
      Log(LogCategory::Platform, LogLevel::Warning, "settings: line {} gives {} a value that is not valid; its default applies", lineNumber,
          line.key);
    }
  }
}

std::optional<SettingValue> SettingsStore::Get(std::string_view _key) const
{
  const auto found = m_values.find(_key);
  if (found == m_values.end() || !found->second.known)
  {
    return std::nullopt;
  }
  return found->second.value;
}

const SettingValue* SettingsStore::ValueOf(std::string_view _key, SettingType _type) const
{
  const SettingSpec* spec = SpecOf(_key);
  const bool textual =
    _type == SettingType::Text && spec != nullptr && (spec->type == SettingType::Enumeration || spec->type == SettingType::Binding);
  ENGINE_ASSERT(spec != nullptr && (spec->type == _type || textual));
  const auto found = m_values.find(_key);
  if (spec == nullptr || found == m_values.end())
  {
    return nullptr;
  }
  const std::optional<SettingValue>& value = found->second.value;
  return value.has_value() ? &value.value() : nullptr;
}

bool SettingsStore::GetBool(std::string_view _key) const
{
  const SettingValue* value = ValueOf(_key, SettingType::Boolean);
  const auto* flag = value != nullptr ? std::get_if<bool>(value) : nullptr;
  return flag != nullptr && *flag;
}

std::int64_t SettingsStore::GetInteger(std::string_view _key) const
{
  const SettingValue* value = ValueOf(_key, SettingType::Integer);
  const auto* integer = value != nullptr ? std::get_if<std::int64_t>(value) : nullptr;
  return integer != nullptr ? *integer : 0;
}

double SettingsStore::GetDecimal(std::string_view _key) const
{
  const SettingValue* value = ValueOf(_key, SettingType::Decimal);
  const auto* decimal = value != nullptr ? std::get_if<double>(value) : nullptr;
  return decimal != nullptr ? *decimal : 0.0;
}

std::string SettingsStore::GetText(std::string_view _key) const
{
  const SettingValue* value = ValueOf(_key, SettingType::Text);
  const auto* text = value != nullptr ? std::get_if<std::string>(value) : nullptr;
  return text != nullptr ? *text : std::string{};
}

bool SettingsStore::Set(std::string_view _key, const SettingValue& _value)
{
  const SettingSpec* spec = SpecOf(_key);
  if (spec == nullptr || !SettingValueFits(*spec, _value))
  {
    Log(LogCategory::Platform, LogLevel::Warning, "settings: a value for {} is refused", _key);
    return false;
  }
  Stored& stored = m_values[std::string(_key)];
  if (stored.known && stored.value.has_value() && *stored.value == _value)
  {
    return true;
  }
  stored.known = true;
  stored.unknownText.clear();
  stored.value = _value;
  Notify(_key);
  return true;
}

bool SettingsStore::Clear(std::string_view _key)
{
  if (SpecOf(_key) == nullptr)
  {
    return false;
  }
  const auto found = m_values.find(_key);
  if (found == m_values.end() || !found->second.value.has_value())
  {
    return true;
  }
  found->second.value.reset();
  Notify(_key);
  return true;
}

std::vector<std::string> SettingsStore::KeysWithPrefix(std::string_view _prefix) const
{
  std::vector<std::string> keys;
  for (const auto& [key, stored] : m_values)
  {
    if (stored.known && stored.value.has_value() && key.starts_with(_prefix))
    {
      keys.push_back(key);
    }
  }
  return keys;
}

SettingsStore::SubscriptionId SettingsStore::Subscribe(std::string _prefix, Subscriber _subscriber)
{
  const SubscriptionId id = m_nextSubscription++;
  m_subscriptions.push_back({id, std::move(_prefix), std::move(_subscriber)});
  return id;
}

void SettingsStore::Unsubscribe(SubscriptionId _id)
{
  std::erase_if(m_subscriptions, [_id](const Subscription& _subscription) { return _subscription.id == _id; });
}

void SettingsStore::Notify(std::string_view _key)
{
  // A subscriber may subscribe or unsubscribe; the ones of this call are those subscribed when it began.
  const std::vector<Subscription> subscriptions = m_subscriptions;
  for (const Subscription& subscription : subscriptions)
  {
    if (_key.starts_with(subscription.prefix))
    {
      subscription.subscriber(_key);
    }
  }
}

std::expected<void, FileError> SettingsStore::Save()
{
  if (!m_file.has_value())
  {
    Log(LogCategory::Platform, LogLevel::Warning, "settings: not saved, because the user-data root is unavailable (PLT-018)");
    return std::unexpected(FileError::Other);
  }
  // DAT-201: a key the loaded file held keeps its line whatever its value; any other is written only when it differs
  // from its default.
  std::vector<SettingsEntryText> current;
  for (const auto& [key, stored] : m_values)
  {
    if (!stored.known)
    {
      current.push_back({key, stored.unknownText});
      continue;
    }
    if (!stored.value.has_value())
    {
      continue;
    }
    const SettingSpec* spec = SpecOf(key);
    const bool loaded = std::ranges::any_of(m_loadedLines, [&](const SettingsLine& _line) { return _line.key == key; });
    if (!loaded && spec != nullptr && spec->defaultValue.has_value() && *spec->defaultValue == *stored.value)
    {
      continue;
    }
    current.push_back({key, FormatSettingValue(*stored.value)});
  }
  const std::string text = WriteSettingsText(m_loadedLines, current);
  std::error_code error;
  std::filesystem::create_directories(m_file->parent_path(), error);
  const std::expected<void, FileError> written =
    ReplaceFileAtomically(*m_file, std::span(reinterpret_cast<const std::byte*>(text.data()), text.size()));
  if (!written.has_value())
  {
    Log(LogCategory::Platform, LogLevel::Error, "settings: saving {} failed ({}); the previous file is kept", ToUtf8(m_file->wstring()),
        FileErrorName(written.error()));
    return written;
  }
  // The file now holds these lines, so a later save keeps each key at the line it has now.
  m_loadedLines = ReadSettingsLines(std::span(reinterpret_cast<const std::byte*>(text.data()), text.size()));
  m_loadedFromFile = true;
  return {};
}

} // namespace Neuron
