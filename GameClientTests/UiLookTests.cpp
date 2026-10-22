#include "pch.h"

#include "FontAsset.h"
#include "MessageLocale.h"
#include "StringTable.h"
#include "TestSupport.h"
#include "UiLook.h"
#include "WorldMarkers.h"

#include <cstddef>
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

// A face as GameFont is drawn: 16 px cells, every glyph from U+0020 to U+007E the whole cell, '?' the missing glyph.
Neuron::FontAsset GameLikeFace()
{
  Neuron::FontAsset face;
  face.designSizePixels = 16;
  face.lineHeightPixels = 16;
  face.baselinePixels = 13;
  face.cellWidthPixels = 16;
  face.cellHeightPixels = 16;
  face.missingCodePoint = '?';
  face.pageWidth = 256;
  face.pageHeight = 128;
  for (std::uint32_t code = 0x20; code < 0x7F; ++code)
  {
    const std::uint32_t cell = code - 0x20;
    face.glyphs.push_back({.codePoint = code,
                           .x = static_cast<std::uint16_t>(cell % 16 * 16),
                           .y = static_cast<std::uint16_t>(cell / 16 * 16),
                           .widthPixels = 16,
                           .bearingPixels = 0,
                           .advancePixels = 16,
                           .offsetYPixels = 0});
  }
  face.page.assign(std::size_t{face.pageWidth} * face.pageHeight, 0);
  return face;
}

} // namespace

TEST_CLASS(UiLookTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui171TheDefaultTextDrawsAtTheScaleItsMinimumAllows)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-171")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui171TheDefaultTextDrawsAtTheScaleItsMinimumAllows)
  {
    // The record's 12 px default in a 16 px face: 16 px up to the height whose minimum passes 16 px, then 32 px.
    const Neuron::FontAsset face = GameLikeFace();
    const double size = CommittedPresentation().ui.text.defaultSize;
    Assert::AreEqual(1u, Species::UiTextScale(CommittedPresentation(), face, size, 720));
    Assert::AreEqual(1u, Species::UiTextScale(CommittedPresentation(), face, size, 1080));
    Assert::AreEqual(1u, Species::UiTextScale(CommittedPresentation(), face, size, 1440));
    Assert::AreEqual(2u, Species::UiTextScale(CommittedPresentation(), face, size, 2160), L"19 px is above 16 px");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl366TheBadgeCountIsAdditiveTextBesideTheBadge)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-366")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-190")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl366TheBadgeCountIsAdditiveTextBesideTheBadge)
  {
    // ADR-145: the count's line starts at the label's left edge, its cell's middle at the label's middle, on whole pixels.
    const Neuron::FontAsset face = GameLikeFace();
    std::vector<Neuron::UiBatch> batches;
    Species::AppendBadgeCount({"18", 100.4f, 50.0f}, face, CommittedPresentation(), 1080, batches);
    Assert::AreEqual(std::size_t{1}, batches.size(), L"one batch, no outline");
    Assert::IsTrue(batches[0].source == Neuron::UiSource::Glyphs && batches[0].blend == Neuron::UiBlend::Additive, L"additive glyphs");
    Assert::AreEqual(std::size_t{2}, batches[0].quads.size(), L"two digits");
    Assert::AreEqual(100.0f, batches[0].quads[0].rectangle[0]);
    Assert::AreEqual(42.0f, batches[0].quads[0].rectangle[1], L"its 16 px cell centred on the label's middle");
    Assert::AreEqual(58.0f, batches[0].quads[0].rectangle[3]);
    std::vector<Neuron::UiBatch> large;
    Species::AppendBadgeCount({"18", 100.0f, 50.0f}, face, CommittedPresentation(), 2160, large);
    Assert::AreEqual(34.0f, large[0].quads[0].rectangle[1], L"at 2160 the cell is 32 px");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui054TheTestPageDrawsEveryPipelineOfTheUiPass)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-054")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-180")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-184")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui054TheTestPageDrawsEveryPipelineOfTheUiPass)
  {
    const Neuron::FontAsset face = GameLikeFace();
    std::vector<Neuron::UiBatch> batches;
    const Neuron::StringTable strings;
    const Neuron::MessageLocale locale("en-GB");
    Species::AppendUiTestPage(face, CommittedPresentation(), strings, locale, {1280, 720}, batches);
    Assert::IsFalse(batches.empty(), L"a page");
    // The window body first: the accent strip, u to the repeat across and v once down, tinted and laid over.
    const Neuron::UiBatch& body = batches.front();
    Assert::IsTrue(body.source == Neuron::UiSource::Image && body.blend == Neuron::UiBlend::Alpha &&
                     body.image == static_cast<std::uint16_t>(Species::UiImage::PanelAccent),
                   L"the accent strip, laid over");
    const Species::TuningPresentation& presentation = CommittedPresentation();
    Assert::AreEqual(static_cast<float>(presentation.ui.chrome.bodyRepeat), body.quads[0].uv[2], L"repeated across");
    Assert::AreEqual(1.0f, body.quads[0].uv[3], L"once down");
    Assert::AreEqual(static_cast<float>(presentation.ui.palette.windowBodyTint.a), body.quads[0].color[3], L"tinted");
    std::set<std::pair<Neuron::UiSource, Neuron::UiBlend>> drawn;
    for (const Neuron::UiBatch& batch : batches)
    {
      drawn.emplace(batch.source, batch.blend);
      for (const Neuron::UiQuad& quad : batch.quads)
      {
        Assert::IsTrue(quad.rectangle[0] >= 0.0f && quad.rectangle[1] >= 0.0f && quad.rectangle[2] <= 1280.0f &&
                         quad.rectangle[3] <= 720.0f,
                       L"inside a 1280 × 720 client area");
      }
    }
    Assert::AreEqual(std::size_t{7}, drawn.size(), L"solid and image quads laid over and added, glyphs laid over, added and inked");
  }
};

} // namespace GameClientTests
