#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// The version of the cooked string table format; a reader accepts this version only (ADR-108, ADR-146).
inline constexpr std::uint32_t STRING_TABLE_ASSET_VERSION = 1;

/// One cooked string (DAT-217): its id, its expanded text in each input mode (DAT-209, table 3.5-B), and whether it can
/// only be embedded in another string (DAT-211), which a look-up never returns.
struct StringEntry
{
  std::string id;
  std::string keyboardMouse;
  std::string gamepad;
  bool embedOnly = false;
};

/// The cooked `text.string_table` entry of one language (DAT-217, ADR-146): its BCP 47 tag and its strings, sorted by id
/// in byte order, each id once.
struct StringTableAsset
{
  std::string language;
  std::vector<StringEntry> entries;
};

// The entry of an id, or null when the table does not hold it.
[[nodiscard]] const StringEntry* FindString(const StringTableAsset& _table, std::string_view _id) noexcept;

// The entry bytes of a cooked string table, in the layout of ADR-108.
[[nodiscard]] std::vector<std::byte> WriteStringTableAsset(const StringTableAsset& _table);
// A cooked string table, or why the bytes are not one of this version: an id out of order or given twice, or a length
// past the entry's end.
[[nodiscard]] std::expected<StringTableAsset, std::string> ReadStringTableAsset(std::span<const std::byte> _entry);

} // namespace Neuron
