#pragma once

#include "CppUnitTest.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace GameClientTests
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

// The committed tuning record (MVP-099), read once: the client's own data in every test.
[[nodiscard]] const Species::TuningMvp& CommittedTuning();
// The committed presentation record (ADR-116), read once.
[[nodiscard]] const Species::TuningPresentation& CommittedPresentation();

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

inline void ExpectText(std::string_view _expected, std::string_view _actual, std::wstring_view _what)
{
  if (_expected != _actual)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(
      std::format(L"{}: expected {} bytes, found {} bytes", _what, _expected.size(), _actual.size()).c_str());
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

} // namespace GameClientTests
