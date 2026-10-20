#pragma once

#include "CppUnitTest.h"

#include "RegionTerrain.h"
#include "TuningMvp.h"

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
