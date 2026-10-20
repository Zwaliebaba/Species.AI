#include "pch.h"

#include "CookedRecord.h"
#include "Development.h"
#include "Files.h"
#include "LooseContent.h"
#include "TextEncoding.h"

#include <algorithm>
#include <format>
#include <system_error>
#include <utility>

namespace Neuron
{

namespace
{

constexpr std::string_view LOOSE_INDEX_MAGIC = "SPCLOOSE";
constexpr std::uint8_t FLAG_SIMULATION_RELEVANT = 0x1;
constexpr std::uint8_t FLAG_PLACEHOLDER = 0x2;

// AST-107: an entry that is not simulation-relevant is checked against its hash in Debug builds only.
constexpr bool CHECK_EVERY_ENTRY = ENGINE_DEVELOPMENT != 0;

std::span<const std::byte> BytesOf(std::string_view _text) noexcept
{
  return std::as_bytes(std::span(_text.data(), _text.size()));
}

std::string TextOf(const std::filesystem::path& _path)
{
  return ToUtf8(_path.wstring());
}

std::filesystem::path PathOf(const std::filesystem::path& _folder, std::string_view _key)
{
  // A key is ASCII (AST-014), so its bytes are the same in every code page.
  return _folder / std::filesystem::path(std::string(_key)).make_preferred();
}

} // namespace

std::vector<std::byte> WriteLooseIndex(std::span<const ContentEntry> _entries)
{
  CookedWriter out;
  out.Append(BytesOf(LOOSE_INDEX_MAGIC));
  out.U16(LOOSE_INDEX_VERSION);
  out.U16(0);
  out.U32(static_cast<std::uint32_t>(_entries.size()));
  for (const ContentEntry& entry : _entries)
  {
    out.U16(static_cast<std::uint16_t>(entry.key.size()));
    out.Append(BytesOf(entry.key));
    out.U8(static_cast<std::uint8_t>(entry.type));
    out.U8(
      static_cast<std::uint8_t>((entry.simulationRelevant ? FLAG_SIMULATION_RELEVANT : 0) | (entry.placeholder ? FLAG_PLACEHOLDER : 0)));
    out.Text8(entry.schemaId);
    out.U16(entry.schemaVersion.major);
    out.U16(entry.schemaVersion.minor);
    out.U64(entry.size);
    out.Append(entry.hash);
  }
  const Sha256Digest hash = Sha256Of(out.View());
  out.Append(hash);
  return out.Take();
}

std::expected<std::vector<ContentEntry>, std::string> ReadLooseIndex(std::span<const std::byte> _index)
{
  if (_index.size() < LOOSE_INDEX_MAGIC.size() + 8 + sizeof(Sha256Digest))
  {
    return std::unexpected(std::string("the index is shorter than its header and hash"));
  }
  const std::span<const std::byte> body = _index.first(_index.size() - sizeof(Sha256Digest));
  if (!std::ranges::equal(Sha256Of(body), _index.last(sizeof(Sha256Digest))))
  {
    return std::unexpected(std::string("the index's hash does not match its bytes"));
  }
  CookedReader in(body);
  if (!std::ranges::equal(in.Bytes(LOOSE_INDEX_MAGIC.size()), BytesOf(LOOSE_INDEX_MAGIC)))
  {
    return std::unexpected(std::string("the index does not start with SPCLOOSE"));
  }
  const std::uint16_t version = in.U16();
  const std::uint16_t flags = in.U16();
  if (version != LOOSE_INDEX_VERSION || flags != 0)
  {
    return std::unexpected(std::format("the index is version {}; this build reads version {} only", version, LOOSE_INDEX_VERSION));
  }
  const std::uint32_t count = in.U32();
  if (count > in.Remaining())
  {
    return std::unexpected(std::format("the index states {} entries, more than its bytes hold", count));
  }
  std::vector<ContentEntry> entries(count);
  for (ContentEntry& entry : entries)
  {
    const std::uint16_t keyBytes = in.U16();
    const std::span<const std::byte> key = in.Bytes(keyBytes);
    entry.key.assign(reinterpret_cast<const char*>(key.data()), key.size());
    const std::uint8_t type = in.U8();
    const std::uint8_t entryFlags = in.U8();
    entry.schemaId = std::string(in.Text8());
    entry.schemaVersion.major = in.U16();
    entry.schemaVersion.minor = in.U16();
    entry.size = in.U64();
    const std::span<const std::byte> hash = in.Bytes(entry.hash.size());
    if (in.Failed())
    {
      return std::unexpected(std::string("the index ends inside an entry"));
    }
    std::ranges::copy(hash, entry.hash.begin());
    if (!IsCanonicalKey(entry.key) || type >= CONTENT_TYPE_COUNT || (entryFlags & ~(FLAG_SIMULATION_RELEVANT | FLAG_PLACEHOLDER)) != 0)
    {
      return std::unexpected(std::format("the entry {} has a key, a content type or flags that no content build writes", entry.key));
    }
    entry.type = static_cast<ContentType>(type);
    entry.simulationRelevant = (entryFlags & FLAG_SIMULATION_RELEVANT) != 0;
    entry.placeholder = (entryFlags & FLAG_PLACEHOLDER) != 0;
  }
  if (!in.Finished())
  {
    return std::unexpected(std::string("the index holds bytes after its last entry"));
  }
  const auto outOfOrder =
    std::ranges::adjacent_find(entries, [](const ContentEntry& _a, const ContentEntry& _b) { return _a.key >= _b.key; });
  if (outOfOrder != entries.end())
  {
    return std::unexpected(std::format("the entry {} is out of key order or listed twice", outOfOrder->key));
  }
  return entries;
}

Sha256Digest SimulationIdentity(std::span<const ContentEntry> _entries)
{
  Sha256 hasher;
  for (const ContentEntry& entry : _entries)
  {
    if (entry.simulationRelevant)
    {
      CookedWriter pair;
      pair.U16(static_cast<std::uint16_t>(entry.key.size()));
      pair.Append(BytesOf(entry.key));
      pair.Append(entry.hash);
      hasher.Update(pair.View());
    }
  }
  return hasher.Finish();
}

std::expected<std::vector<ContentEntry>, std::string> WriteLooseContent(const std::filesystem::path& _folder,
                                                                        std::vector<CookedEntry> _entries)
{
  std::ranges::sort(_entries, {}, [](const CookedEntry& _cooked) { return std::string_view{_cooked.entry.key}; });
  std::vector<ContentEntry> index;
  index.reserve(_entries.size());
  for (CookedEntry& cooked : _entries)
  {
    if (!IsCanonicalKey(cooked.entry.key) || (!index.empty() && index.back().key == cooked.entry.key))
    {
      return std::unexpected(std::format("the key {} is not canonical or is written twice", cooked.entry.key));
    }
    cooked.entry.size = cooked.bytes.size();
    cooked.entry.hash = Sha256Of(cooked.bytes);
    index.push_back(cooked.entry);
  }

  std::error_code error;
  if (std::filesystem::exists(_folder, error) && !std::filesystem::is_empty(_folder, error))
  {
    if (!std::filesystem::exists(_folder / LOOSE_INDEX_FILE, error))
    {
      return std::unexpected(
        std::format("{} holds files and no {}, so the content build did not write it", TextOf(_folder), LOOSE_INDEX_FILE));
    }
    (void)std::filesystem::remove_all(_folder, error);
    if (error)
    {
      return std::unexpected(std::format("{} cannot be emptied: {}", TextOf(_folder), error.message()));
    }
  }
  (void)std::filesystem::create_directories(_folder, error);
  if (error)
  {
    return std::unexpected(std::format("{} cannot be made: {}", TextOf(_folder), error.message()));
  }
  for (const CookedEntry& cooked : _entries)
  {
    const std::filesystem::path path = PathOf(_folder, cooked.entry.key);
    (void)std::filesystem::create_directories(path.parent_path(), error);
    if (error || !ReplaceFileAtomically(path, cooked.bytes))
    {
      return std::unexpected(std::format("{} cannot be written", TextOf(path)));
    }
  }
  const std::vector<std::byte> indexBytes = WriteLooseIndex(index);
  if (!ReplaceFileAtomically(_folder / LOOSE_INDEX_FILE, indexBytes))
  {
    return std::unexpected(std::format("{} cannot be written", TextOf(_folder / LOOSE_INDEX_FILE)));
  }
  return index;
}

std::expected<LooseContent, AssetError> LooseContent::Mount(const std::filesystem::path& _folder)
{
  const auto bytes = ReadWholeFile(_folder / LOOSE_INDEX_FILE);
  if (!bytes)
  {
    return std::unexpected(AssetError{AssetFault::Missing, {}, std::format("no {} in {}", LOOSE_INDEX_FILE, TextOf(_folder))});
  }
  auto entries = ReadLooseIndex(*bytes);
  if (!entries)
  {
    return std::unexpected(AssetError{AssetFault::Damaged, {}, std::format("{}: {}", LOOSE_INDEX_FILE, entries.error())});
  }
  LooseContent content;
  content.m_folder = _folder;
  content.m_entries = std::move(*entries);
  content.m_identity = SimulationIdentity(content.m_entries);
  return content;
}

const ContentEntry* LooseContent::Find(std::string_view _requestedKey) const
{
  const std::string key = CanonicalKey(_requestedKey);
  const auto found =
    std::ranges::lower_bound(m_entries, std::string_view{key}, {}, [](const ContentEntry& _entry) { return std::string_view{_entry.key}; });
  return found != m_entries.end() && found->key == key ? &*found : nullptr;
}

std::expected<std::vector<std::byte>, AssetError> LooseContent::Read(std::string_view _requestedKey) const
{
  const ContentEntry* entry = Find(_requestedKey);
  if (entry == nullptr)
  {
    return std::unexpected(AssetError{AssetFault::Missing, CanonicalKey(_requestedKey), "the content holds no entry of this key"});
  }
  auto bytes = ReadWholeFile(PathOf(m_folder, entry->key));
  if (!bytes)
  {
    return std::unexpected(AssetError{AssetFault::Missing, entry->key, "the index lists the entry and its file cannot be read"});
  }
  if (bytes->size() != entry->size)
  {
    return std::unexpected(AssetError{AssetFault::Damaged, entry->key,
                                      std::format("the file holds {} bytes and the index states {}", bytes->size(), entry->size)});
  }
  if ((entry->simulationRelevant || CHECK_EVERY_ENTRY) && Sha256Of(*bytes) != entry->hash)
  {
    return std::unexpected(AssetError{AssetFault::Damaged, entry->key, "the entry's SHA-256 is not the one its index states"});
  }
  return std::move(*bytes);
}

} // namespace Neuron
