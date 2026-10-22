#pragma once

#include "CppUnitTest.h"

#include "LooseContent.h"
#include "RegionTerrain.h"
#include "StructureRecords.h"
#include "TuningMvp.h"
#include "WorldSimulation.h"

#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace GameLogicTests
{

// The failed development checks since the last call, which the module's check hook collects (DBG-072).
[[nodiscard]] std::vector<std::string> TakeDevelopmentChecks();

/// A folder of its own under the temporary directory, removed with everything in it when the test ends.
class TemporaryFolder
{
public:
  TemporaryFolder();
  ~TemporaryFolder();

  TemporaryFolder(const TemporaryFolder&) = delete;
  TemporaryFolder& operator=(const TemporaryFolder&) = delete;

  [[nodiscard]] const std::filesystem::path& Path() const noexcept
  {
    return m_path;
  }

private:
  std::filesystem::path m_path;
};

// The repository's content root, found from this source file's own path, so a test reads the committed data
// wherever its runner is.
[[nodiscard]] std::filesystem::path RepositoryContentRoot();

// The committed tuning record (MVP-099), read once: the values every world of the tests runs on.
[[nodiscard]] const Species::TuningMvp& CommittedTuning();

// A terrain over the MVP region's square that is level at _height everywhere, its border included: a field with no
// tile and no flatten area, whose every sample is the outside height (TER-031).
[[nodiscard]] std::shared_ptr<const Species::RegionTerrain> FlatTerrain(double _height = 50.0);

// The valley of seed 1 (MVP-011), built once.
[[nodiscard]] std::shared_ptr<const Species::RegionTerrain> ValleyTerrain();

// Simulation data records for the MVP's structures written by the tests (M2 plan §4: T4 and T5 test on records the
// tests write): each a root node at the identity with its kind's ports on a ring of radius 20 wu, its entrance and door
// at 25 wu on -z and +z where its kind has them, and a bounding sphere of radius 15 wu. TestStructureRecords holds them;
// StructureRecordEntries cooks them under their keys (ADR-129).
[[nodiscard]] Species::StructureRecords TestStructureRecords();
[[nodiscard]] std::vector<Neuron::CookedEntry> StructureRecordEntries();
// TestStructureRecords made once, for the worlds that outlive a call.
[[nodiscard]] const Species::StructureRecords& TestRecords();

// The key of the settled worlds of the tests, and a new world on the valley of seed 1 with the steward's settlement at
// _site (MVP-012), on TestRecords.
inline constexpr Neuron::SipHashKey SETTLED_TEST_KEY{0x5e771e5e771e5e77, 0x0a0b0c0d0e0f1011};
[[nodiscard]] std::unique_ptr<Species::WorldSimulation> SettledWorld(std::uint8_t _site = 2);
// M3's valley (ADR-157): the steward's settlement at site 2 and three AI settlements at the other sites.
[[nodiscard]] std::unique_ptr<Species::WorldSimulation> ValleyWorld(const Neuron::SipHashKey& _key = SETTLED_TEST_KEY);

// Runs each job on a thread of its own and returns once all have ended, so that a test's independent runs share the
// runner's cores. A job asserts nothing: it computes, and the test asserts on what the jobs computed. An exception a
// job throws is thrown again here, the first job's first.
void RunConcurrently(const std::vector<std::function<void()>>& _jobs);

// The whole file as text, or an empty string; and a file written whole, its folders made.
[[nodiscard]] std::string ReadText(const std::filesystem::path& _path);
void WriteText(const std::filesystem::path& _path, std::string_view _text);

// The framework's AreEqual needs a ToString for every type it compares; these compare and describe the values
// themselves.
inline void ExpectEqual(std::int64_t _expected, std::int64_t _actual, std::wstring_view _what)
{
  if (_expected != _actual)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
      std::format(L"{}: expected {}, found {}", _what, _expected, _actual).c_str());
  }
}

inline void ExpectNoDevelopmentCheck()
{
  const std::vector<std::string> checks = TakeDevelopmentChecks();
  if (!checks.empty())
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
      std::format(L"{} unexpected development checks failed", checks.size()).c_str());
  }
}

} // namespace GameLogicTests
