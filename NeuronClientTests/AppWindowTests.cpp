#include "pch.h"

#include "AppWindow.h"
#include "DisplayAttempt.h"
#include "PlatformClock.h"

#include <expected>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

std::expected<Neuron::AppWindow, Neuron::PlatformError> CreateWindowIn(const Neuron::PlatformClock& _clock, Neuron::WindowMode _mode)
{
  Neuron::AppWindow::Desc desc;
  desc.clock = &_clock;
  desc.title = "NeuronClientTests";
  desc.placement.mode = _mode;
  desc.renderSize = Neuron::FALLBACK_WINDOW_SIZE;
  return Neuron::AppWindow::Create(desc);
}

} // namespace

TEST_CLASS(AppWindowTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Plt057WindowedWindowIsCreatedHidden)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-057")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt057WindowedWindowIsCreatedHidden)
  {
    // The window procedure answers the creation messages before CreateWindowExW has returned the handle.
    const Neuron::PlatformClock clock;
    const auto window = CreateWindowIn(clock, Neuron::WindowMode::Windowed);
    Assert::IsTrue(window.has_value(), L"the window is created");
    Assert::IsNotNull(window->Native(), L"the window has a handle");
    Assert::IsFalse(IsWindowVisible(window->Native()) != FALSE, L"the window is hidden until the swap chain exists");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt057BorderlessWindowIsCreatedHidden)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-057")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt057BorderlessWindowIsCreatedHidden)
  {
    const Neuron::PlatformClock clock;
    const auto window = CreateWindowIn(clock, Neuron::WindowMode::BorderlessFullScreen);
    Assert::IsTrue(window.has_value(), L"the window is created");
    Assert::IsNotNull(window->Native(), L"the window has a handle");
    Assert::IsFalse(IsWindowVisible(window->Native()) != FALSE, L"the window is hidden until the swap chain exists");
  }
};

} // namespace NeuronClientTests
