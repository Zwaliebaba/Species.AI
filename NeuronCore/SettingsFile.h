#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Neuron
{

// DAT-195: a setting key's most characters, `dat.id.max_chars`, compiled in because the store exists before any
// content is mounted (DEC-100).
inline constexpr std::size_t SETTING_KEY_CHARS_MAX = 64;
// DAT-200: a line longer than this is malformed, and kept as it is.
inline constexpr std::size_t SETTINGS_LINE_BYTES_MAX = 4096;

// The value types of DAT-197 that the client stores so far (ADR-150). An enumeration holds its enumerator's name, and a
// binding the canonical JSON of its node (INP-072), which the input router reads.
enum class SettingType : std::uint8_t
{
  Boolean,
  Integer,
  Decimal,
  Text,
  Enumeration,
  Binding
};

using SettingValue = std::variant<bool, std::int64_t, double, std::string>;

/// One entry of the defaults table (DAT-193, DEC-100). A family's key ends with `.` and gives every key under it its
/// type, as `binding.` does for the user layer's entries (INP-070).
struct SettingSpec
{
  std::string key;
  SettingType type = SettingType::Boolean;
  std::optional<SettingValue> defaultValue; // none: the setting is absent until it is set
  double minimum = 0.0;                     // Integer and Decimal, inclusive
  double maximum = 0.0;
  std::vector<std::string> enumerators; // Enumeration
};

// DAT-195: lower-case ASCII letters, digits, `_` and `.`, from 1 to SETTING_KEY_CHARS_MAX characters.
[[nodiscard]] bool IsSettingKey(std::string_view _key) noexcept;
// The spec that types _key: its own entry, or the family it belongs to; none for a key the schema does not hold.
[[nodiscard]] const SettingSpec* FindSettingSpec(std::span<const SettingSpec> _specs, std::string_view _key) noexcept;

// DAT-197 and DAT-198: the stored text as a value of the spec's type within its range or enumerators, or none.
[[nodiscard]] std::optional<SettingValue> ParseSettingValue(const SettingSpec& _spec, std::string_view _text);
// DAT-197: the value's text as the file holds it; a decimal in its shortest round-trip form.
[[nodiscard]] std::string FormatSettingValue(const SettingValue& _value);
// Whether the value has the spec's type and lies within its range or enumerators.
[[nodiscard]] bool SettingValueFits(const SettingSpec& _spec, const SettingValue& _value);

/// One line of a settings file as read (DAT-196, table 3.2-A). An entry's key and value text are views into the line.
struct SettingsLine
{
  enum class Kind : std::uint8_t
  {
    Entry,
    Comment,
    Blank,
    Other // neither of the three: logged and kept as it is (DAT-199)
  };

  Kind kind = Kind::Other;
  std::string text; // the line without its line end
  std::string key;  // an entry's key
  std::string value;
};

// DAT-196 and DAT-200: the file's lines. Nothing in the bytes makes the reader fail: a byte sequence that is not UTF-8,
// a missing `=`, a key that is not one and an over-long line each make their own line Other. A carriage return before
// a line feed, a byte-order mark and blanks around `=` are accepted, and the writer spells them as table 3.2-A does.
[[nodiscard]] std::vector<SettingsLine> ReadSettingsLines(std::span<const std::byte> _bytes);

// DAT-201: the file's bytes for the current values. Every line of _loaded that is not an entry stays in its place; an
// entry's key is written once, at its first line, with its current value, or dropped if it has none; then every key of
// _current that _loaded did not hold, in byte-wise order. _current holds each key's text in the form of DAT-197.
struct SettingsEntryText
{
  std::string key;
  std::string value;
};
[[nodiscard]] std::string WriteSettingsText(std::span<const SettingsLine> _loaded, std::span<const SettingsEntryText> _current);

} // namespace Neuron
