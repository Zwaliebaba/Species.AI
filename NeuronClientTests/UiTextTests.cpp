#include "pch.h"

#include "TestSupport.h"
#include "UiText.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// A laid-out line of two glyphs on page 0 of a 64-texel page, as the text system gives one (ADR-147): 'A' 6 × 10 px
// from (0, 2), 'B' 5 × 10 px from (8, 2).
Neuron::TextLine TwoGlyphLine()
{
  Neuron::TextLine line;
  line.widthPixels = 14.0f;
  line.heightPixels = 13.0f;
  line.baselinePixels = 11.0f;
  line.glyphs.push_back({0, {0.0f, 2.0f, 6.0f, 12.0f}, {0.0f, 0.0f, 6.0f / 64.0f, 10.0f / 64.0f}});
  line.glyphs.push_back({0, {8.0f, 2.0f, 13.0f, 12.0f}, {8.0f / 64.0f, 0.0f, 13.0f / 64.0f, 10.0f / 64.0f}});
  return line;
}

Neuron::UiQuad QuadAt(float _left)
{
  Neuron::UiQuad quad;
  quad.rectangle = {_left, 0.0f, _left + 1.0f, 1.0f};
  return quad;
}

} // namespace

TEST_CLASS(UiTextTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui171TheMinimumTextSizeFollowsTheClientHeight)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-171")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui171TheMinimumTextSizeFollowsTheClientHeight)
  {
    Assert::AreEqual(9.0, Neuron::MinimumTextPixels(1080, 9.0, 19.0), 1e-9, L"9 px at 1080");
    Assert::AreEqual(4.5, Neuron::MinimumTextPixels(540, 9.0, 19.0), 1e-9, L"proportional below 1080");
    Assert::AreEqual(14.0, Neuron::MinimumTextPixels(1620, 9.0, 19.0), 1e-9, L"linear between 1080 and 2160");
    Assert::AreEqual(19.0, Neuron::MinimumTextPixels(2160, 9.0, 19.0), 1e-9, L"19 px at 2160");
    Assert::AreEqual(38.0, Neuron::MinimumTextPixels(4320, 9.0, 19.0), 1e-9, L"proportional above 2160");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui171TheFinalSizeTakesTheDpiScaleAndTheMinimum)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-171")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-170")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui171TheFinalSizeTakesTheDpiScaleAndTheMinimum)
  {
    // O92: DirectWrite rasterizes at the final size itself, with no whole-number scale.
    Assert::AreEqual(12.0f, Neuron::FinalTextPixels(12.0, 1.0, 1.0, 9.0), L"the layout size at a scale of 1");
    Assert::AreEqual(18.0f, Neuron::FinalTextPixels(12.0, 1.0, 1.5, 9.0), L"times the DPI scale");
    Assert::AreEqual(15.0f, Neuron::FinalTextPixels(12.0, 1.25, 1.0, 9.0), L"times the text scale");
    Assert::AreEqual(19.0f, Neuron::FinalTextPixels(12.0, 1.0, 1.0, 19.0), L"raised to the minimum");
    Assert::AreEqual(12.0f, Neuron::FinalTextPixels(12.0, 1.0, 1.0, std::numeric_limits<double>::quiet_NaN()),
                     L"a minimum that is no number leaves it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui179QuadsJoinTheLastBatchOnlyWhenNothingChanges)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-179")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui179QuadsJoinTheLastBatchOnlyWhenNothingChanges)
  {
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendQuad(batches, Neuron::UiSource::Solid, Neuron::UiBlend::Alpha, 0, QuadAt(0.0f));
    Neuron::AppendQuad(batches, Neuron::UiSource::Solid, Neuron::UiBlend::Alpha, 3, QuadAt(1.0f)); // a solid quad reads no image
    Assert::AreEqual(std::size_t{1}, batches.size(), L"the same source and blend: one batch");
    Neuron::AppendQuad(batches, Neuron::UiSource::Solid, Neuron::UiBlend::Additive, 0, QuadAt(2.0f));
    Neuron::AppendQuad(batches, Neuron::UiSource::Image, Neuron::UiBlend::Additive, 0, QuadAt(3.0f));
    Neuron::AppendQuad(batches, Neuron::UiSource::Image, Neuron::UiBlend::Additive, 1, QuadAt(4.0f));
    Neuron::AppendQuad(batches, Neuron::UiSource::Solid, Neuron::UiBlend::Alpha, 0, QuadAt(5.0f));
    Assert::AreEqual(std::size_t{5}, batches.size(), L"a new batch for each change of blend, source or image");
    Assert::AreEqual(std::size_t{2}, batches[0].quads.size());
    Assert::IsTrue(batches[3].image == 1 && batches[4].source == Neuron::UiSource::Solid, L"in painter's order, never reordered");
    batches.back().scissor = Neuron::UiScissor{0, 0, 10, 10};
    Neuron::AppendQuad(batches, Neuron::UiSource::Solid, Neuron::UiBlend::Alpha, 0, QuadAt(6.0f));
    Assert::AreEqual(std::size_t{6}, batches.size(), L"a batch with its own scissor is not extended");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui179GlyphsOnAnotherPageStartABatch)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-179")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui179GlyphsOnAnotherPageStartABatch)
  {
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendQuad(batches, Neuron::UiSource::Glyphs, Neuron::UiBlend::Additive, 0, QuadAt(0.0f));
    Neuron::AppendQuad(batches, Neuron::UiSource::Glyphs, Neuron::UiBlend::Additive, 0, QuadAt(1.0f));
    Neuron::AppendQuad(batches, Neuron::UiSource::Glyphs, Neuron::UiBlend::Additive, 2, QuadAt(2.0f));
    Assert::AreEqual(std::size_t{2}, batches.size(), L"one batch for each run of a page");
    Assert::IsTrue(batches[0].image == 0 && batches[1].image == 2, L"each names its page");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui192AnOutlinedLineDrawsEightInkCopiesThenItsFill)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-192")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-190")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-180")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui192AnOutlinedLineDrawsEightInkCopiesThenItsFill)
  {
    Neuron::TextStyle style;
    style.outline = std::array<float, 4>{1.0f, 1.0f, 1.0f, 0.0f};
    style.outlineOffsetPixels = 1;
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendText(batches, TwoGlyphLine(), 10.4f, 20.6f, style);
    Assert::AreEqual(std::size_t{2}, batches.size(), L"the outline pass, then the fill pass");
    Assert::IsTrue(batches[0].source == Neuron::UiSource::Glyphs && batches[0].blend == Neuron::UiBlend::Ink, L"the outline inks");
    Assert::AreEqual(std::size_t{16}, batches[0].quads.size(), L"eight copies of two glyphs");
    Assert::IsTrue(batches[1].blend == Neuron::UiBlend::Additive && batches[1].quads.size() == 2, L"normal text is additive");
    const Neuron::UiQuad& fill = batches[1].quads[0];
    Assert::AreEqual(10.0f, fill.rectangle[0], L"the line's corner on a whole pixel");
    Assert::AreEqual(23.0f, fill.rectangle[1], L"with the glyph's place in its line");
    Assert::AreEqual(6.0f, fill.rectangle[2] - fill.rectangle[0], L"one texel a pixel");
    const Neuron::UiQuad& first = batches[0].quads[0];
    Assert::AreEqual(fill.rectangle[0] - 1.0f, first.rectangle[0], L"the first copy one pixel to the left");
    Assert::AreEqual(fill.rectangle[1], first.rectangle[1]);
    Assert::AreEqual(0.0f, first.color[3], L"in the outline's color");
    const Neuron::UiQuad& last = batches[0].quads[14];
    Assert::AreEqual(fill.rectangle[0] + 1.0f, last.rectangle[0], L"the last copy down and to the right");
    Assert::AreEqual(fill.rectangle[1] + 1.0f, last.rectangle[1]);
    std::vector<Neuron::UiBatch> none;
    Neuron::AppendText(none, Neuron::TextLine{}, 0.0f, 0.0f, {});
    Assert::IsTrue(none.empty(), L"a line without glyphs adds no batch");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg095OverlayLinesAreAnchoredByTheirCornersAtTheDpiScale)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg095OverlayLinesAreAnchoredByTheirCornersAtTheDpiScale)
  {
    // ADR-147: the development role lays the lines out; the anchors scale with the DPI scale.
    Neuron::TextSystem::Desc desc;
    desc.families[static_cast<std::size_t>(Neuron::FontRole::Development)] = {"Cascadia Mono", "Consolas"};
    Neuron::TextSystem text(desc);
    Neuron::OverlayLine corner;
    corner.text = "60 fps";
    corner.insetX = 10.0f;
    corner.insetY = 40.0f;
    corner.corner = Neuron::OverlayCorner::BottomRight;
    corner.color = {0.5f, 1.0f, 0.5f, 1.0f};
    Neuron::OverlayLine page;
    page.text = "frame";
    page.insetX = 12.0f;
    page.insetY = 12.0f;
    page.corner = Neuron::OverlayCorner::TopLeft;
    const std::vector<Neuron::OverlayLine> lines{corner, page};
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendOverlayLines(batches, text, lines, {800, 600}, 14.0f, 1.5f);
    Assert::IsFalse(batches.empty(), L"the lines are drawn");
    std::size_t right = 0;
    for (const Neuron::UiBatch& batch : batches)
    {
      Assert::IsTrue(batch.source == Neuron::UiSource::Glyphs && batch.blend == Neuron::UiBlend::Alpha, L"laid over");
      for (const Neuron::UiQuad& quad : batch.quads)
      {
        if (quad.color == corner.color)
        {
          ++right;
          Assert::IsTrue(quad.rectangle[2] <= 800.0f - 15.0f + 1.0f && quad.rectangle[3] <= 600.0f - 60.0f + 1.0f,
                         L"the corner line ends 15 px from the right and 60 px from the bottom");
        }
        else
        {
          Assert::IsTrue(quad.rectangle[0] >= 16.0f && quad.rectangle[1] >= 17.0f, L"the page line starts at 18 px");
        }
      }
    }
    Assert::IsTrue(right >= 4, L"the corner line's glyphs, its space drawing nothing");
  }
};

} // namespace NeuronClientTests
