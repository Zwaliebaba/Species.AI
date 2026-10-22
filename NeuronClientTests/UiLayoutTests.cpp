#include "pch.h"

#include "UiLayout.h"

#include <array>
#include <cstdint>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

constexpr float TOLERANCE = 0.06f; // the worked values are given to one decimal place

void AreEqual(const Neuron::UiRect& _rect, float _x, float _y, float _width, float _height, const wchar_t* _what)
{
  Assert::AreEqual(_x, _rect.left, TOLERANCE, _what);
  Assert::AreEqual(_y, _rect.top, TOLERANCE, _what);
  Assert::AreEqual(_width, _rect.Width(), TOLERANCE, _what);
  Assert::AreEqual(_height, _rect.Height(), TOLERANCE, _what);
}

} // namespace

TEST_CLASS(UiLayoutTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui163ThePageGridGivesTheWorkedValues)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-163")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui163ThePageGridGivesTheWorkedValues)
  {
    const Neuron::PageGridKeys keys;
    const Neuron::PageGrid standard = Neuron::ComputePageGrid({800, 600}, keys);
    Assert::IsFalse(standard.wide, L"4:3 is not wide");
    AreEqual(standard.titleBar, 24.0f, 24.0f, 752.0f, 120.0f, L"800 x 600 title bar");
    AreEqual(standard.leftBox, 24.0f, 168.0f, 444.0f, 408.0f, L"800 x 600 left box");
    AreEqual(standard.rightBox, 492.0f, 168.0f, 284.0f, 408.0f, L"800 x 600 right box");
    Assert::AreEqual(57.0f, standard.fontLarge, TOLERANCE);
    Assert::AreEqual(24.0f, standard.fontMedium, TOLERANCE);
    Assert::AreEqual(13.2f, standard.fontSmall, TOLERANCE);
    Assert::AreEqual(399.6f, standard.buttonWidth, TOLERANCE);
    Assert::AreEqual(31.2f, standard.buttonHeight, TOLERANCE);
    Assert::AreEqual(2.4f, standard.buttonGap, TOLERANCE);

    const Neuron::PageGrid older = Neuron::ComputePageGrid({1024, 768}, keys);
    AreEqual(older.titleBar, 30.7f, 30.7f, 962.6f, 153.6f, L"1024 x 768 title bar");
    AreEqual(older.leftBox, 30.7f, 215.0f, 568.3f, 522.2f, L"1024 x 768 left box");
    AreEqual(older.rightBox, 629.8f, 215.0f, 363.5f, 522.2f, L"1024 x 768 right box");

    const Neuron::PageGrid wide = Neuron::ComputePageGrid({1280, 720}, keys);
    Assert::IsTrue(wide.wide, L"16:9 is wide");
    AreEqual(wide.titleBar, 140.8f, 38.4f, 998.4f, 129.6f, L"1280 x 720 title bar");
    AreEqual(wide.leftBox, 140.8f, 206.4f, 608.0f, 475.2f, L"1280 x 720 left box");
    AreEqual(wide.rightBox, 787.2f, 206.4f, 352.0f, 475.2f, L"1280 x 720 right box");

    const Neuron::PageGrid full = Neuron::ComputePageGrid({1920, 1080}, keys);
    AreEqual(full.titleBar, 211.2f, 57.6f, 1497.6f, 194.4f, L"1920 x 1080 title bar");
    AreEqual(full.leftBox, 211.2f, 309.6f, 912.0f, 712.8f, L"1920 x 1080 left box");
    AreEqual(full.rightBox, 1180.8f, 309.6f, 528.0f, 712.8f, L"1920 x 1080 right box");
    Assert::AreEqual(102.6f, full.fontLarge, TOLERANCE);
    Assert::AreEqual(43.2f, full.fontMedium, TOLERANCE);
    Assert::AreEqual(23.76f, full.fontSmall, TOLERANCE);
    Assert::AreEqual(820.8f, full.buttonWidth, TOLERANCE);
    Assert::AreEqual(56.16f, full.buttonHeight, TOLERANCE);
    Assert::AreEqual(4.32f, full.buttonGap, TOLERANCE);
    // UI-164 item 1: the button column is centered in the left box.
    Assert::AreEqual(211.2f + (912.0f - 820.8f) * 0.5f, full.ButtonColumnLeft(), TOLERANCE);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui169LinesScaleWithTheDpiInWholePixels)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-169")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui169LinesScaleWithTheDpiInWholePixels)
  {
    Neuron::UiMetrics metrics;
    metrics.dpiScale = 2.0f;
    Assert::AreEqual(2.0f, metrics.LinePixels(1.0), 0.0f, L"at DPI 2 a 1 px border is 2 px");
    metrics.dpiScale = 1.25f;
    Assert::AreEqual(1.0f, metrics.LinePixels(1.0), 0.0f, L"at DPI 1.25 a 1 px line stays 1 px");
    metrics.dpiScale = 0.5f;
    Assert::AreEqual(1.0f, metrics.LinePixels(1.0), 0.0f, L"never below 1 px");
    const Neuron::UiRect snapped = Neuron::SnapRect({10.4f, 10.6f, 20.5f, 30.49f});
    Assert::IsTrue(snapped == Neuron::UiRect{10.0f, 11.0f, 21.0f, 30.0f}, L"every edge on its nearest whole pixel");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui171TextScalesAndIsClampedToTheMinimum)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-171")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui171TextScalesAndIsClampedToTheMinimum)
  {
    Neuron::UiMetrics metrics;
    metrics.textScale = 1.5f;
    metrics.uiScale = 2.0f;
    metrics.minimumTextPixels = 6.0;
    Assert::AreEqual(60.0f, metrics.TextPixels(20.0), 0.001f, L"the text scale and the UI scale");
    Assert::AreEqual(30.0f, metrics.TextPixels(20.0, false), 0.001f, L"the text scale alone where the element does not scale");
    Assert::AreEqual(6.0f, metrics.TextPixels(2.0, false), 0.001f, L"never below the minimum");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd377TheHudSpaceIsSixHundredUnitsHigh)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-377")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd377TheHudSpaceIsSixHundredUnitsHigh)
  {
    Neuron::UiMetrics metrics;
    metrics.client = {1920, 1080};
    Assert::AreEqual(1.8f, metrics.HudUnitPixels(), 0.0001f);
    const Neuron::UiRect rect = Neuron::HudRect(10.0f, 20.0f, 100.0f, 50.0f, metrics);
    Assert::IsTrue(rect == Neuron::UiRect{18.0f, 36.0f, 198.0f, 126.0f}, L"units times the unit's pixels, from the top-left");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui168TheUiScaleScalesAboutTheAnchor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-168")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui168TheUiScaleScalesAboutTheAnchor)
  {
    const Neuron::UiRect scaled = Neuron::ScaleAbout({100.0f, 100.0f, 200.0f, 150.0f}, 100.0f, 150.0f, 2.0f);
    Assert::IsTrue(scaled == Neuron::UiRect{100.0f, 50.0f, 300.0f, 150.0f}, L"the anchor, the bottom-left corner, stays");
    Assert::AreEqual(14.0f, Neuron::RowTextTop(10.0f, 20.0f, 12.0f), 0.0f, L"UI-165: a line centered in its row");
  }
};

} // namespace NeuronClientTests
