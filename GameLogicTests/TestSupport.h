#pragma once

#include "CppUnitTest.h"

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

namespace GameLogicTests
{

// The failed development checks since the last call, which the module's check hook collects (DBG-072).
[[nodiscard]] std::vector<std::string> TakeDevelopmentChecks();

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
