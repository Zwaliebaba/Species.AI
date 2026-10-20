#include "pch.h"

#include "ResizeCoalescer.h"
#include "TestSupport.h"
#include "WindowGeometry.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

constexpr Neuron::RectPx MONITOR{0, 0, 1920, 1080};
constexpr Neuron::RectPx WORK_AREA{0, 0, 1920, 1040}; // a 40-pixel taskbar at the bottom
constexpr Neuron::FrameInsets FRAME{8, 31, 8, 8};

} // namespace

TEST_CLASS(WindowGeometryTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Plt059WindowedClientIsCenteredOnTheMonitor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-059")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt059WindowedClientIsCenteredOnTheMonitor)
  {
    // The window of a 1280 x 720 client is 1296 x 759 with its frame, centred on the 1920 x 1080 monitor.
    const Neuron::RectPx client = Neuron::WindowedClientRect({1280, 720}, MONITOR, WORK_AREA, FRAME);
    ExpectRect({320, 191, 1600, 911}, client, L"client rectangle");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt059DesktopSizedClientIsClampedToTheWorkArea)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-059")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt059DesktopSizedClientIsClampedToTheWorkArea)
  {
    // The first-run render size is the desktop size; the window with its frame fills the work area and no more.
    const Neuron::RectPx client = Neuron::WindowedClientRect({1920, 1080}, MONITOR, WORK_AREA, FRAME);
    ExpectRect({8, 31, 1912, 1032}, client, L"client rectangle");
    ExpectRect(WORK_AREA, Neuron::WindowRectFromClient(client, FRAME), L"window rectangle");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt059ClientIsNeverSmallerThanTheMinimum)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-059")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt059ClientIsNeverSmallerThanTheMinimum)
  {
    const Neuron::RectPx client = Neuron::WindowedClientRect({320, 200}, MONITOR, WORK_AREA, FRAME);
    ExpectSize({Neuron::MIN_RENDER_WIDTH_PIXELS, Neuron::MIN_RENDER_HEIGHT_PIXELS}, client.Size(), L"a small request");
    const Neuron::RectPx tiny{0, 0, 600, 400};
    const Neuron::RectPx cramped = Neuron::WindowedClientRect({800, 600}, tiny, tiny, FRAME);
    ExpectSize({640, 480}, cramped.Size(), L"a work area smaller than the minimum");
    ExpectEqual(8, cramped.left, L"the window starts at the work area's left");
    ExpectEqual(31, cramped.top, L"the window starts at the work area's top");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt059SecondMonitorKeepsItsCoordinates)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-059")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt059SecondMonitorKeepsItsCoordinates)
  {
    const Neuron::RectPx left{-2560, -360, 0, 1080};
    const Neuron::RectPx client = Neuron::WindowedClientRect({1280, 720}, left, left, FRAME);
    ExpectRect({-1920, 11, -640, 731}, client, L"client rectangle on a monitor left of the primary");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt060DpiScaleIsTheDpiOverTheBase)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt060DpiScaleIsTheDpiOverTheBase)
  {
    Assert::IsTrue(Neuron::ScaleForDpi(96) == 1.0f, L"96 dpi is 100 %");
    Assert::IsTrue(Neuron::ScaleForDpi(144) == 1.5f, L"144 dpi is 150 %");
    Assert::IsTrue(Neuron::ScaleForDpi(192) == 2.0f, L"192 dpi is 200 %");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt062OnePumpRaisesOneResizeWithTheLatestSize)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-062")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt062OnePumpRaisesOneResizeWithTheLatestSize)
  {
    // PLT T9: a drag delivers many sizes; the pump raises the last one once.
    Neuron::ResizeCoalescer sizes({1280, 720});
    Assert::IsFalse(sizes.TakeResize().has_value(), L"no size change, no resize");
    for (std::uint32_t width = 1281; width <= 1300; ++width)
    {
      Assert::IsTrue(sizes.OnSize(false, {width, 700}) == Neuron::ResizeCoalescer::Transition::None, L"a size is no transition");
    }
    const std::optional<Neuron::SizePx> resize = sizes.TakeResize();
    Assert::IsTrue(resize.has_value(), L"one resize");
    ExpectSize({1300, 700}, resize.value_or(Neuron::SizePx{}), L"the latest size");
    Assert::IsFalse(sizes.TakeResize().has_value(), L"and only one");
    sizes.OnSize(false, {1200, 600});
    sizes.OnSize(false, {1300, 700});
    Assert::IsFalse(sizes.TakeResize().has_value(), L"a size that ends where the last resize left it raises nothing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt063MinimizingRaisesNoResize)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt063MinimizingRaisesNoResize)
  {
    Neuron::ResizeCoalescer sizes({1280, 720});
    Assert::IsTrue(sizes.OnSize(true, {0, 0}) == Neuron::ResizeCoalescer::Transition::Minimized, L"minimised");
    Assert::IsTrue(sizes.OnSize(true, {0, 0}) == Neuron::ResizeCoalescer::Transition::None, L"still minimised");
    Assert::IsTrue(sizes.IsMinimized(), L"the state is minimised");
    Assert::IsFalse(sizes.TakeResize().has_value(), L"no resize while minimised");
    ExpectSize({1280, 720}, sizes.ClientSize(), L"the client size is kept while minimised");
    Assert::IsTrue(sizes.OnSize(false, {1280, 720}) == Neuron::ResizeCoalescer::Transition::Restored, L"restored");
    Assert::IsFalse(sizes.TakeResize().has_value(), L"restored at the same size: no resize");
    sizes.OnSize(true, {0, 0});
    sizes.OnSize(false, {1024, 768});
    const std::optional<Neuron::SizePx> resize = sizes.TakeResize();
    Assert::IsTrue(resize.has_value() && *resize == Neuron::SizePx{1024, 768}, L"restored at another size: one resize");
  }
};

} // namespace NeuronClientTests
