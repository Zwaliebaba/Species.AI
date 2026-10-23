#pragma once

#include "Files.h"
#include "SettingsFile.h"

#include <cstdint>
#include <expected>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

/// The client's settings store (PLT-030 to PLT-034, PLT-079 to PLT-082; ADR-150): named, typed values from the defaults
/// table, backed by the settings file of DAT-196. It is created after the start-up options and the user-data root and
/// before the window, and it is used on the main thread only (PLT-082). Without a file (an unavailable user-data root,
/// PLT-018) it keeps its values in memory and every save reports its failure.
class SettingsStore
{
public:
  using SubscriptionId = std::uint32_t;
  using Subscriber = std::function<void(std::string_view)>; // called with the key that changed

  // _file: the settings file, read at once; none keeps the settings in memory. A missing or unreadable file is a first
  // run (PLT-080), and no content of the file stops the reader (DAT-200).
  SettingsStore(std::vector<SettingSpec> _specs, std::optional<std::filesystem::path> _file);

  SettingsStore(const SettingsStore&) = delete;
  SettingsStore& operator=(const SettingsStore&) = delete;

  // PLT-080: whether the file existed and was read; false on a first run.
  [[nodiscard]] bool LoadedFromFile() const noexcept
  {
    return m_loadedFromFile;
  }
  [[nodiscard]] const std::optional<std::filesystem::path>& File() const noexcept
  {
    return m_file;
  }

  // Whether the schema holds the key, by its own entry or its family (INP-142: a setting a binding may name).
  [[nodiscard]] bool Knows(std::string_view _key) const noexcept
  {
    return SpecOf(_key) != nullptr;
  }
  // The value of a key the schema holds; none while the setting is absent.
  [[nodiscard]] std::optional<SettingValue> Get(std::string_view _key) const;
  // PLT-033: the value with the schema's type. A key the schema does not hold, or another type, is a development check
  // (DBG-070) and reads as the type's zero.
  [[nodiscard]] bool GetBool(std::string_view _key) const;
  [[nodiscard]] std::int64_t GetInteger(std::string_view _key) const;
  [[nodiscard]] double GetDecimal(std::string_view _key) const;
  [[nodiscard]] std::string GetText(std::string_view _key) const; // Text, Enumeration and Binding

  // PLT-034: changes the value at once and then calls every subscriber whose prefix matches the key, in subscription
  // order; a value equal to the stored one notifies nobody. A key the schema does not hold, or a value that does not
  // fit its type, range or enumerators, is refused (DAT-197) and returns false.
  bool Set(std::string_view _key, const SettingValue& _value);
  // Makes the setting absent, as an unset binding is (INP-070); false for a key the schema does not hold.
  bool Clear(std::string_view _key);
  // The keys the store holds a value for that begin with _prefix, in byte-wise order.
  [[nodiscard]] std::vector<std::string> KeysWithPrefix(std::string_view _prefix) const;

  SubscriptionId Subscribe(std::string _prefix, Subscriber _subscriber);
  void Unsubscribe(SubscriptionId _id);

  // PLT-081: writes the file losslessly and atomically with the writer of DAT-201 (PLT-089). At M2 the write runs on
  // the main thread (ADR-150). A failed save keeps the previous file and is logged; without a file every save fails.
  [[nodiscard]] std::expected<void, FileError> Save();

private:
  struct Stored
  {
    std::optional<SettingValue> value; // a key of the schema
    std::string unknownText;           // DAT-199: a key the schema does not hold, kept as its text
    bool known = true;
  };
  struct Subscription
  {
    SubscriptionId id;
    std::string prefix;
    Subscriber subscriber;
  };

  [[nodiscard]] const SettingSpec* SpecOf(std::string_view _key) const noexcept;
  [[nodiscard]] const SettingValue* ValueOf(std::string_view _key, SettingType _type) const;
  void Load();
  void Notify(std::string_view _key);

  std::vector<SettingSpec> m_specs;
  std::optional<std::filesystem::path> m_file;
  std::map<std::string, Stored, std::less<>> m_values;
  std::vector<SettingsLine> m_loadedLines;
  std::vector<Subscription> m_subscriptions;
  SubscriptionId m_nextSubscription = 1;
  bool m_loadedFromFile = false;
};

} // namespace Neuron
