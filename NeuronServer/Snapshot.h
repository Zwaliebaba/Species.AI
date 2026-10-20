#pragma once

#include "Files.h"
#include "SipHash.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace Neuron
{

// ADR-099: a snapshot file starts with these eight bytes, then the format version, the world's state version, its tick
// and the length of its state, and ends with a SipHash-2-4 of everything before it under SNAPSHOT_CHECK_KEY.
inline constexpr std::array<std::byte, 8> SNAPSHOT_MAGIC{std::byte{'S'}, std::byte{'P'}, std::byte{'C'}, std::byte{'S'},
                                                         std::byte{'N'}, std::byte{'A'}, std::byte{'P'}, std::byte{0x1A}};
inline constexpr std::uint16_t SNAPSHOT_FORMAT_VERSION = 1;
inline constexpr std::size_t SNAPSHOT_HEADER_BYTES = 32;
inline constexpr std::size_t SNAPSHOT_CHECK_BYTES = 8;
// A public key: the check finds a torn or damaged file, it authenticates nothing.
inline constexpr SipHashKey SNAPSHOT_CHECK_KEY{0x0053656963657053, 0x746f687370616e53};
inline constexpr std::string_view SNAPSHOT_SUFFIX = ".snapshot";

// Why a snapshot was refused; a refused snapshot is never loaded in part (MVP-095).
enum class SnapshotError : std::uint8_t
{
  Unreadable,    // the file cannot be read
  NotASnapshot,  // its first bytes are not SNAPSHOT_MAGIC
  Truncated,     // shorter than its header and stated length
  Damaged,       // its check value does not match: torn or changed
  FormatVersion, // a format version this build does not read
  StateVersion,  // a world state version the world's loader cannot migrate (MVP-095)
  MalformedState // the state does not decode whole
};

[[nodiscard]] std::string_view SnapshotErrorText(SnapshotError _error) noexcept;

/// One snapshot's contents: the world's state in its own versioned encoding, which the engine does not read.
struct SnapshotContents
{
  std::uint32_t stateVersion = 0;
  std::uint64_t tick = 0; // the first tick the restored world steps
  std::vector<std::byte> state;
};

[[nodiscard]] std::vector<std::byte> EncodeSnapshot(const SnapshotContents& _contents);
[[nodiscard]] std::expected<SnapshotContents, SnapshotError> DecodeSnapshot(std::span<const std::byte> _file);

/// The snapshots of one world in one folder (MVP-075, ADR-099): world-<tick>.snapshot, each written whole beside the
/// previous one and put in place by an atomic replace (PLT-089), the newest `kept` kept. Used from one thread.
class SnapshotStore
{
public:
  SnapshotStore(std::filesystem::path _folder, std::size_t _kept);

  // Writes the snapshot, then removes all but the newest `kept`. A failed write leaves every earlier snapshot.
  [[nodiscard]] std::expected<std::filesystem::path, FileError> Save(const SnapshotContents& _contents);
  // The snapshots in the folder, newest first.
  [[nodiscard]] std::vector<std::filesystem::path> Snapshots() const;
  [[nodiscard]] static std::expected<SnapshotContents, SnapshotError> Load(const std::filesystem::path& _path);

  [[nodiscard]] const std::filesystem::path& Folder() const noexcept
  {
    return m_folder;
  }

private:
  std::filesystem::path m_folder;
  std::size_t m_kept;
};

} // namespace Neuron
