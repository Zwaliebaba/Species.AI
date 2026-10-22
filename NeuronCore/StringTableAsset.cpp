#include "pch.h"

#include "CookedRecord.h"
#include "StringTableAsset.h"

#include <algorithm>
#include <format>

namespace Neuron
{

namespace
{

void WriteText(CookedWriter& _out, std::string_view _text)
{
  _out.U32(static_cast<std::uint32_t>(_text.size()));
  _out.Append(std::as_bytes(std::span(_text.data(), _text.size())));
}

std::string ReadText(CookedReader& _in)
{
  const std::uint32_t bytes = _in.U32();
  if (bytes > _in.Remaining())
  {
    static_cast<void>(_in.Bytes(_in.Remaining() + 1)); // fails the reader
    return {};
  }
  const std::span<const std::byte> text = _in.Bytes(bytes);
  return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

} // namespace

const StringEntry* FindString(const StringTableAsset& _table, std::string_view _id) noexcept
{
  const auto found =
    std::ranges::lower_bound(_table.entries, _id, {}, [](const StringEntry& _entry) { return std::string_view(_entry.id); });
  return found != _table.entries.end() && found->id == _id ? &*found : nullptr;
}

std::vector<std::byte> WriteStringTableAsset(const StringTableAsset& _table)
{
  CookedWriter out;
  out.U32(STRING_TABLE_ASSET_VERSION);
  out.Text8(_table.language);
  out.U32(static_cast<std::uint32_t>(_table.entries.size()));
  for (const StringEntry& entry : _table.entries)
  {
    out.Text8(entry.id);
    out.U8(entry.embedOnly ? std::uint8_t{1} : std::uint8_t{0});
    WriteText(out, entry.keyboardMouse);
    WriteText(out, entry.gamepad);
  }
  return out.Take();
}

std::expected<StringTableAsset, std::string> ReadStringTableAsset(std::span<const std::byte> _entry)
{
  CookedReader in(_entry);
  if (const std::uint32_t version = in.U32(); version != STRING_TABLE_ASSET_VERSION)
  {
    return std::unexpected(
      std::format("the cooked string table is version {}; this build reads version {} only", version, STRING_TABLE_ASSET_VERSION));
  }
  StringTableAsset table;
  table.language = std::string(in.Text8());
  const std::uint32_t entries = in.U32();
  if (in.Failed() || entries > in.Remaining())
  {
    return std::unexpected(std::string("the cooked string table is shorter than its header"));
  }
  table.entries.resize(entries);
  for (StringEntry& entry : table.entries)
  {
    entry.id = std::string(in.Text8());
    const std::uint8_t flags = in.U8();
    entry.embedOnly = (flags & 1u) != 0;
    entry.keyboardMouse = ReadText(in);
    entry.gamepad = ReadText(in);
    if (in.Failed())
    {
      return std::unexpected(std::string("a cooked string runs past the entry's end"));
    }
  }
  if (!in.Finished())
  {
    return std::unexpected(std::string("the cooked string table has bytes after its last string"));
  }
  const auto outOfOrder =
    std::ranges::adjacent_find(table.entries, [](const StringEntry& _a, const StringEntry& _b) { return _a.id >= _b.id; });
  if (outOfOrder != table.entries.end())
  {
    return std::unexpected(std::string("the strings are not sorted by id, each once"));
  }
  return table;
}

} // namespace Neuron
