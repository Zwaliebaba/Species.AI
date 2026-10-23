#include "pch.h"

#include "MessageLocale.h"
#include "StringTable.h"
#include "TestSupport.h"
#include "TextSystem.h"
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

TEST_CLASS(UiLookTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui171TheDefaultTextTakesTheDpiScaleAndItsMinimum)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-171")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-170")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui171TheDefaultTextTakesTheDpiScaleAndItsMinimum)
  {
    // The record's 12 px default: as it is at 720 and 1080, raised to 19 px at 2160, and 18 px at a DPI scale of 1.5.
    const Species::TuningPresentation& presentation = CommittedPresentation();
    const double size = presentation.ui.text.defaultSize;
    Assert::AreEqual(12.0f, Species::UiTextPixels(presentation, size, 720, 1.0f, 1.0));
    Assert::AreEqual(12.0f, Species::UiTextPixels(presentation, size, 1080, 1.0f, 1.0));
    Assert::AreEqual(19.0f, Species::UiTextPixels(presentation, size, 2160, 1.0f, 1.0), L"the minimum at 2160");
    Assert::AreEqual(18.0f, Species::UiTextPixels(presentation, size, 1080, 1.5f, 1.0), L"times the DPI scale");
    Assert::AreEqual(18.0f, Species::UiTextPixels(presentation, size, 1080, 1.0f, 1.5), L"and the text scale (ADR-150)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl366TheBadgeCountIsAdditiveTextBesideTheBadge)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-366")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-190")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl366TheBadgeCountIsAdditiveTextBesideTheBadge)
  {
    // ADR-145, ADR-147: the count's line starts at the label's left edge, its middle at the label's middle.
    Neuron::TextSystem text(Species::TextSystemDesc(CommittedPresentation()));
    std::vector<Neuron::UiBatch> batches;
    Species::AppendBadgeCount({"18", 100.4f, 50.0f}, text, CommittedPresentation(), 1080, 1.0f, 1.0, batches);
    Assert::AreEqual(std::size_t{1}, batches.size(), L"one batch, no outline");
    Assert::IsTrue(batches[0].source == Neuron::UiSource::Glyphs && batches[0].blend == Neuron::UiBlend::Additive, L"additive glyphs");
    Assert::AreEqual(std::size_t{2}, batches[0].quads.size(), L"two digits");
    const float height = text.Metrics(Neuron::FontRole::Body, 12.0f).heightPixels;
    for (const Neuron::UiQuad& quad : batches[0].quads)
    {
      Assert::IsTrue(quad.rectangle[0] >= 99.0f, L"from the label's left edge");
      Assert::IsTrue(quad.rectangle[1] >= 50.0f - height / 2.0f - 1.0f && quad.rectangle[3] <= 50.0f + height / 2.0f + 1.0f,
                     L"inside its line, centered on the label's middle");
    }
    std::vector<Neuron::UiBatch> large;
    Species::AppendBadgeCount({"18", 100.0f, 50.0f}, text, CommittedPresentation(), 2160, 1.0f, 1.0, large);
    const float smallHeight = batches[0].quads[0].rectangle[3] - batches[0].quads[0].rectangle[1];
    const float largeHeight = large[0].quads[0].rectangle[3] - large[0].quads[0].rectangle[1];
    Assert::IsTrue(largeHeight > smallHeight * 1.3f, L"at 2160 the digits are drawn at 19 px");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui054TheTestPageDrawsEveryPipelineOfTheUiPass)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-054")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-180")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-184")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui054TheTestPageDrawsEveryPipelineOfTheUiPass)
  {
    Neuron::TextSystem text(Species::TextSystemDesc(CommittedPresentation()));
    std::vector<Neuron::UiBatch> batches;
    const Neuron::StringTable strings;
    const Neuron::MessageLocale locale("en-US");
    Species::AppendUiTestPage(text, CommittedPresentation(), strings, locale, {0.0f, 0.0f, 1280.0f, 720.0f}, 1.0f, batches);
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
