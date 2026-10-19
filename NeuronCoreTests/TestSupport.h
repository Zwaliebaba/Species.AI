#pragma once

#include "CppUnitTest.h"

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace NeuronCoreTests
{

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

// The failed development checks since the last call, which the module's check hook collects (DBG-072).
[[nodiscard]] std::vector<std::string> TakeDevelopmentChecks();

// The whole file as text, or an empty string when it cannot be read.
[[nodiscard]] std::string ReadText(const std::filesystem::path& _path);

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

inline void ExpectContains(std::string_view _text, std::string_view _part, std::wstring_view _what)
{
  if (_text.find(_part) == std::string_view::npos)
  {
    Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(std::format(L"{}: the text lacks a part", _what).c_str());
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

} // namespace NeuronCoreTests
