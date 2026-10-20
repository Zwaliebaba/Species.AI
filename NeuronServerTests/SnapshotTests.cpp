#include "pch.h"

#include "Files.h"
#include "Snapshot.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronServerTests
{

namespace
{

Neuron::SnapshotContents Contents(std::uint64_t _tick)
{
  Neuron::SnapshotContents contents;
  contents.stateVersion = 3;
  contents.tick = _tick;
  for (std::size_t i = 0; i < 37; ++i)
  {
    contents.state.push_back(static_cast<std::byte>((i * 7 + _tick) & 0xff));
  }
  return contents;
}

bool Same(const Neuron::SnapshotContents& _left, const Neuron::SnapshotContents& _right)
{
  return _left.stateVersion == _right.stateVersion && _left.tick == _right.tick && _left.state == _right.state;
}

bool RefusedAs(const std::vector<std::byte>& _file, Neuron::SnapshotError _error)
{
  const auto decoded = Neuron::DecodeSnapshot(_file);
  return !decoded.has_value() && decoded.error() == _error;
}

void WriteBytes(const std::filesystem::path& _path, const std::vector<std::byte>& _bytes)
{
  std::ofstream file(_path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char*>(_bytes.data()), static_cast<std::streamsize>(_bytes.size()));
}

} // namespace

TEST_CLASS(SnapshotTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075ASnapshotRoundTrips)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075ASnapshotRoundTrips)
  {
    const Neuron::SnapshotContents contents = Contents(600);
    const std::vector<std::byte> file = Neuron::EncodeSnapshot(contents);
    ExpectEqual(static_cast<std::int64_t>(Neuron::SNAPSHOT_HEADER_BYTES + 37 + Neuron::SNAPSHOT_CHECK_BYTES),
                static_cast<std::int64_t>(file.size()), L"the header, the state and the check value");
    const auto decoded = Neuron::DecodeSnapshot(file);
    Assert::IsTrue(decoded.has_value() && Same(*decoded, contents), L"the same contents");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095ATornOrChangedSnapshotIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095ATornOrChangedSnapshotIsRefused)
  {
    // Never loaded in part: every shorter file and every flipped bit is refused with a reason.
    const std::vector<std::byte> file = Neuron::EncodeSnapshot(Contents(42));
    for (std::size_t length = 0; length < file.size(); ++length)
    {
      const std::vector<std::byte> torn(file.begin(), file.begin() + static_cast<std::ptrdiff_t>(length));
      Assert::IsFalse(Neuron::DecodeSnapshot(torn).has_value(), L"a torn snapshot is refused");
    }
    for (std::size_t bit = 0; bit < file.size() * 8; ++bit)
    {
      std::vector<std::byte> changed = file;
      changed[bit / 8] ^= std::byte{static_cast<unsigned char>(1U << (bit % 8))};
      Assert::IsFalse(Neuron::DecodeSnapshot(changed).has_value(), L"a changed snapshot is refused");
    }
    std::vector<std::byte> longer = file;
    longer.push_back(std::byte{0});
    Assert::IsTrue(RefusedAs(longer, Neuron::SnapshotError::Damaged), L"a byte after the check value");
    Assert::IsTrue(RefusedAs(std::vector<std::byte>(file.begin(), file.begin() + 20), Neuron::SnapshotError::Truncated), L"a torn header");
    std::vector<std::byte> other = file;
    other[0] = std::byte{'X'};
    Assert::IsTrue(RefusedAs(other, Neuron::SnapshotError::NotASnapshot), L"another file");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095AnUnknownFormatVersionIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095AnUnknownFormatVersionIsRefused)
  {
    // A later build's file, whole and with its check value right, is still refused by this one.
    std::vector<std::byte> file = Neuron::EncodeSnapshot(Contents(7));
    file[8] = std::byte{Neuron::SNAPSHOT_FORMAT_VERSION + 1};
    file.resize(file.size() - Neuron::SNAPSHOT_CHECK_BYTES);
    std::uint64_t check = Neuron::SipHash24(Neuron::SNAPSHOT_CHECK_KEY, file);
    for (std::size_t i = 0; i < Neuron::SNAPSHOT_CHECK_BYTES; ++i, check >>= 8)
    {
      file.push_back(static_cast<std::byte>(check & 0xff));
    }
    Assert::IsTrue(RefusedAs(file, Neuron::SnapshotError::FormatVersion), L"format version 2");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp075TheStoreKeepsTheNewestAndOutlivesATornWrite)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-075")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp075TheStoreKeepsTheNewestAndOutlivesATornWrite)
  {
    const TemporaryFolder folder;
    Neuron::SnapshotStore store(folder.Path() / L"saves", 3);
    for (const std::uint64_t tick : {std::uint64_t{10}, std::uint64_t{600}, std::uint64_t{20}, std::uint64_t{1200}})
    {
      Assert::IsTrue(store.Save(Contents(tick)).has_value(), L"saved");
    }
    std::vector<std::filesystem::path> snapshots = store.Snapshots();
    ExpectEqual(3, static_cast<std::int64_t>(snapshots.size()), L"mvp.persist.snapshots_kept are kept");
    Assert::IsTrue(snapshots[0].filename() == L"world-00000000000000001200.snapshot", L"the newest first, by tick");
    Assert::IsTrue(snapshots[2].filename() == L"world-00000000000000000020.snapshot", L"the oldest of the three kept");

    // A write that never finished leaves a temporary file, which is no snapshot; a newest file torn by other means is
    // refused, and the next older one loads.
    WriteBytes(folder.Path() / L"saves" / L"world-00000000000000001800.snapshot.1.0.tmp", Neuron::EncodeSnapshot(Contents(1800)));
    std::vector<std::byte> torn = Neuron::EncodeSnapshot(Contents(1200));
    torn.resize(torn.size() / 2);
    WriteBytes(snapshots[0], torn);
    snapshots = store.Snapshots();
    ExpectEqual(3, static_cast<std::int64_t>(snapshots.size()), L"the temporary file is not listed");
    Assert::IsTrue(Neuron::SnapshotStore::Load(snapshots[0]).error_or(Neuron::SnapshotError::Unreadable) ==
                     Neuron::SnapshotError::Truncated,
                   L"the torn newest is refused");
    const auto next = Neuron::SnapshotStore::Load(snapshots[1]);
    Assert::IsTrue(next.has_value() && Same(*next, Contents(600)), L"the next older loads whole");
  }
};

} // namespace NeuronServerTests
