#pragma once

#include "CppUnitTest.h"
#include "PlatformClock.h"

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace NeuronServerTests
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

/// A platform clock on a manual time source, which only the test advances (PLT-048, DBG-141).
class ManualClock
{
public:
  ManualClock()
  {
    auto source = std::make_unique<Neuron::ManualTimeSource>();
    m_source = source.get();
    m_clock = std::make_unique<Neuron::PlatformClock>(std::move(source));
  }

  [[nodiscard]] const Neuron::PlatformClock& Clock() const noexcept
  {
    return *m_clock;
  }
  [[nodiscard]] Neuron::Nanoseconds Now() const noexcept
  {
    return m_clock->Now();
  }
  void Advance(Neuron::Nanoseconds _step) noexcept
  {
    m_source->Advance(_step);
  }
  // What a sleep to the deadline does on this clock: time moves on to it, and no further.
  void AdvanceTo(Neuron::Nanoseconds _deadline) noexcept
  {
    const Neuron::Nanoseconds now = Now();
    if (_deadline > now)
    {
      m_source->Advance(_deadline - now);
    }
  }

private:
  Neuron::ManualTimeSource* m_source = nullptr;
  std::unique_ptr<Neuron::PlatformClock> m_clock;
};

inline constexpr Neuron::Nanoseconds TEST_TICK_PERIOD{100'000'000}; // `ref.sim.tick_period`, 0.1 s

} // namespace NeuronServerTests
