#include "pch.h"

#include "Check.h"
#include "Log.h"
#include "Snapshot.h"

#include <algorithm>
#include <format>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace Neuron
{

namespace
{

void PutLittleEndian(std::vector<std::byte>& _out, std::uint64_t _value, std::size_t _bytes)
{
  for (std::size_t i = 0; i < _bytes; ++i)
  {
    _out.push_back(static_cast<std::byte>((_value >> (8 * i)) & 0xff));
  }
}

std::uint64_t GetLittleEndian(std::span<const std::byte> _bytes, std::size_t _offset, std::size_t _count) noexcept
{
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < _count; ++i)
  {
    value |= std::to_integer<std::uint64_t>(_bytes[_offset + i]) << (8 * i);
  }
  return value;
}

// world-<the tick in 20 digits>.snapshot, and nothing else: a temporary file of an unfinished write is not one.
bool IsSnapshotName(std::wstring_view _name) noexcept
{
  constexpr std::wstring_view PREFIX = L"world-";
  constexpr std::size_t TICK_DIGITS = 20;
  if (_name.size() != PREFIX.size() + TICK_DIGITS + SNAPSHOT_SUFFIX.size() || !_name.starts_with(PREFIX))
  {
    return false;
  }
  const std::wstring_view digits = _name.substr(PREFIX.size(), TICK_DIGITS);
  const std::wstring_view suffix = _name.substr(PREFIX.size() + TICK_DIGITS);
  return std::ranges::all_of(digits, [](wchar_t _c) { return _c >= L'0' && _c <= L'9'; }) &&
         std::ranges::equal(suffix, SNAPSHOT_SUFFIX, [](wchar_t _left, char _right) { return _left == static_cast<wchar_t>(_right); });
}

} // namespace

std::string_view SnapshotErrorText(SnapshotError _error) noexcept
{
  switch (_error)
  {
  case SnapshotError::Unreadable:
    return "the file cannot be read";
  case SnapshotError::NotASnapshot:
    return "the file is not a snapshot";
  case SnapshotError::Truncated:
    return "the file is shorter than the snapshot it states";
  case SnapshotError::Damaged:
    return "the file's check value does not match: it is torn or damaged";
  case SnapshotError::FormatVersion:
    return "the snapshot format is one this build does not read";
  case SnapshotError::StateVersion:
    return "the world state's version is one this build cannot migrate";
  case SnapshotError::MalformedState:
    return "the world state does not decode whole";
  }
  return "the snapshot is refused";
}

std::vector<std::byte> EncodeSnapshot(const SnapshotContents& _contents)
{
  std::vector<std::byte> file(SNAPSHOT_MAGIC.begin(), SNAPSHOT_MAGIC.end());
  file.reserve(SNAPSHOT_HEADER_BYTES + _contents.state.size() + SNAPSHOT_CHECK_BYTES);
  PutLittleEndian(file, SNAPSHOT_FORMAT_VERSION, 2);
  PutLittleEndian(file, 0, 2); // reserved, always 0
  PutLittleEndian(file, _contents.stateVersion, 4);
  PutLittleEndian(file, _contents.tick, 8);
  PutLittleEndian(file, _contents.state.size(), 8);
  ENGINE_ASSERT(file.size() == SNAPSHOT_HEADER_BYTES);
  file.insert(file.end(), _contents.state.begin(), _contents.state.end());
  PutLittleEndian(file, SipHash24(SNAPSHOT_CHECK_KEY, file), SNAPSHOT_CHECK_BYTES);
  return file;
}

std::expected<SnapshotContents, SnapshotError> DecodeSnapshot(std::span<const std::byte> _file)
{
  if (_file.size() < SNAPSHOT_MAGIC.size() || !std::ranges::equal(_file.first(SNAPSHOT_MAGIC.size()), SNAPSHOT_MAGIC))
  {
    return std::unexpected(SnapshotError::NotASnapshot);
  }
  if (_file.size() < SNAPSHOT_HEADER_BYTES + SNAPSHOT_CHECK_BYTES)
  {
    return std::unexpected(SnapshotError::Truncated);
  }
  // The format version decides the layout of everything after it, so it is read before anything else is.
  if (GetLittleEndian(_file, 8, 2) != SNAPSHOT_FORMAT_VERSION || GetLittleEndian(_file, 10, 2) != 0)
  {
    return std::unexpected(SnapshotError::FormatVersion);
  }
  const std::uint64_t stateBytes = GetLittleEndian(_file, 24, 8);
  const std::size_t presentBytes = _file.size() - SNAPSHOT_HEADER_BYTES - SNAPSHOT_CHECK_BYTES;
  if (stateBytes != presentBytes)
  {
    return std::unexpected(stateBytes > presentBytes ? SnapshotError::Truncated : SnapshotError::Damaged);
  }
  const std::size_t checked = _file.size() - SNAPSHOT_CHECK_BYTES;
  if (SipHash24(SNAPSHOT_CHECK_KEY, _file.first(checked)) != GetLittleEndian(_file, checked, SNAPSHOT_CHECK_BYTES))
  {
    return std::unexpected(SnapshotError::Damaged);
  }
  SnapshotContents contents;
  contents.stateVersion = static_cast<std::uint32_t>(GetLittleEndian(_file, 12, 4));
  contents.tick = GetLittleEndian(_file, 16, 8);
  const auto state = _file.subspan(SNAPSHOT_HEADER_BYTES, static_cast<std::size_t>(stateBytes));
  contents.state.assign(state.begin(), state.end());
  return contents;
}

SnapshotStore::SnapshotStore(std::filesystem::path _folder, std::size_t _kept)
  : m_folder(std::move(_folder)),
    m_kept(_kept)
{
  ENGINE_CHECK(_kept > 0);
}

std::expected<std::filesystem::path, FileError> SnapshotStore::Save(const SnapshotContents& _contents)
{
  std::error_code error;
  std::filesystem::create_directories(m_folder, error);
  // The tick in 20 digits, so the byte-wise order of the names is the order of the ticks.
  std::filesystem::path path = m_folder / std::format("world-{:020}{}", _contents.tick, SNAPSHOT_SUFFIX);
  const std::vector<std::byte> file = EncodeSnapshot(_contents);
  if (const std::expected<void, FileError> written = ReplaceFileAtomically(path, file); !written)
  {
    return std::unexpected(written.error());
  }
  const std::vector<std::filesystem::path> snapshots = Snapshots();
  for (std::size_t i = m_kept; i < snapshots.size(); ++i)
  {
    if (!RemoveFile(snapshots[i]))
    {
      Log(LogCategory::Simulation, LogLevel::Warning, "an old snapshot could not be removed: {}", snapshots[i].filename().string());
    }
  }
  return path;
}

std::vector<std::filesystem::path> SnapshotStore::Snapshots() const
{
  std::vector<std::filesystem::path> snapshots;
  for (const std::filesystem::path& file : ListFiles(m_folder, SNAPSHOT_SUFFIX))
  {
    if (IsSnapshotName(file.filename().native()))
    {
      snapshots.push_back(file);
    }
  }
  std::ranges::reverse(snapshots);
  return snapshots;
}

std::expected<SnapshotContents, SnapshotError> SnapshotStore::Load(const std::filesystem::path& _path)
{
  const std::expected<std::vector<std::byte>, FileError> file = ReadWholeFile(_path);
  if (!file)
  {
    return std::unexpected(SnapshotError::Unreadable);
  }
  return DecodeSnapshot(*file);
}

} // namespace Neuron
