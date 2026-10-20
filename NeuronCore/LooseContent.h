#pragma once

#include "AssetError.h"
#include "ContentDiagnostic.h"
#include "ContentKey.h"
#include "Sha256.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// The file of a loose-content folder that lists its entries. Its name holds `-`, which no key can (AST-014), so it
// never collides with an entry.
inline constexpr std::string_view LOOSE_INDEX_FILE = "content-index.bin";
inline constexpr std::uint16_t LOOSE_INDEX_VERSION = 1;

/// One cooked entry as an index lists it: the fields of a pack's entry record without its chunk (AST-054, ADR-108).
struct ContentEntry
{
  std::string key;
  ContentType type = ContentType::Texture;
  std::string schemaId;              // a data record's schema id (DAT table 1-A); empty for every other entry
  SchemaVersion schemaVersion{0, 0}; // a data record's schema version; 0.0 for every other entry
  std::uint64_t size = 0;
  bool simulationRelevant = false; // AST-098
  bool placeholder = false;        // AST-083
  Sha256Digest hash{};             // of the entry's bytes (AST-097)
};

// An entry and its bytes, as a cooker hands it to the writer, which sets its size and hash.
struct CookedEntry
{
  ContentEntry entry;
  std::vector<std::byte> bytes;
};

// The index of a loose-content folder, in the layout of ADR-108: its entries sorted by key, then the SHA-256 of every
// byte before it.
[[nodiscard]] std::vector<std::byte> WriteLooseIndex(std::span<const ContentEntry> _entries);
// The entries of an index, or why the bytes are not one: a wrong magic, version or hash, an unknown content type, a
// key that is not canonical or out of order.
[[nodiscard]] std::expected<std::vector<ContentEntry>, std::string> ReadLooseIndex(std::span<const std::byte> _index);

// AST-100: the SHA-256 over the simulation-relevant entries in key order, each as its key's length (16 bits), its key
// and its hash; M1's edition has no runtime feature flag to append.
[[nodiscard]] Sha256Digest SimulationIdentity(std::span<const ContentEntry> _entries);

// Writes a loose-content folder (AST-105): each entry's bytes in the file at its key, and the index last, so a folder
// with an index is whole. The folder is emptied first, but only when it holds an index or nothing, so a mistaken path
// never loses files that the content build did not write. The index's entries, or why the folder was not written.
[[nodiscard]] std::expected<std::vector<ContentEntry>, std::string> WriteLooseContent(const std::filesystem::path& _folder,
                                                                                      std::vector<CookedEntry> _entries);

/// A mounted loose-content folder (AST-105, ADR-108): the cooked entries the content build wrote, laid out by key, with
/// the index that lists them. Until packs exist every configuration mounts one (owner decision O73), so every entry is
/// loose and the session's content identity is reported as modified. It is immutable once mounted, so any thread may
/// look up and read entries at once (AST-010).
class LooseContent
{
public:
  // Reads the folder's index and checks it. Missing when the folder or its index is not there; damaged when the index
  // is not one.
  [[nodiscard]] static std::expected<LooseContent, AssetError> Mount(const std::filesystem::path& _folder);

  // The entry of a key, canonicalised first (AST-015); null when the folder holds none.
  [[nodiscard]] const ContentEntry* Find(std::string_view _requestedKey) const;
  // An entry's bytes, checked against its size, and against its hash when it is simulation-relevant and, in Debug
  // builds, always (AST-107). Missing when the index lists no such key or its file is gone; damaged otherwise.
  [[nodiscard]] std::expected<std::vector<std::byte>, AssetError> Read(std::string_view _requestedKey) const;

  [[nodiscard]] std::span<const ContentEntry> Entries() const noexcept
  {
    return m_entries;
  }
  [[nodiscard]] const std::filesystem::path& Folder() const noexcept
  {
    return m_folder;
  }
  [[nodiscard]] const Sha256Digest& Identity() const noexcept
  {
    return m_identity;
  }

private:
  LooseContent() = default;

  std::filesystem::path m_folder;
  std::vector<ContentEntry> m_entries; // in key order
  Sha256Digest m_identity{};
};

} // namespace Neuron
