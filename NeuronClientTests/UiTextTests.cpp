#include "pch.h"

#include "FontAsset.h"
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

// A face of 8 × 10 cells on a 64 × 32 page, its design size 10: 'A', 'B' and '?', the missing glyph.
Neuron::FontAsset SmallFace()
{
  Neuron::FontAsset face;
  face.designSizePixels = 10;
  face.lineHeightPixels = 10;
  face.cellWidthPixels = 8;
  face.cellHeightPixels = 10;
  face.missingCodePoint = '?';
  face.pageWidth = 64;
  face.pageHeight = 32;
  face.glyphs = {
    {.codePoint = '?', .x = 8, .y = 0, .widthPixels = 6, .bearingPixels = 0, .advancePixels = 7, .offsetYPixels = 0},
    {.codePoint = 'A', .x = 16, .y = 0, .widthPixels = 6, .bearingPixels = 0, .advancePixels = 8, .offsetYPixels = 0},
    {.codePoint = 'B', .x = 24, .y = 10, .widthPixels = 5, .bearingPixels = 0, .advancePixels = 6, .offsetYPixels = 0},
  };
  face.page.assign(std::size_t{face.pageWidth} * face.pageHeight, 0);
  return face;
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

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui198TextIsDrawnAtAWholeNumberScaleRaisedToTheMinimum)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-171")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui198TextIsDrawnAtAWholeNumberScaleRaisedToTheMinimum)
  {
    // O63: k = ⌊s ÷ d + 0.5⌋, at least 1, raised until k × d reaches the minimum.
    Assert::AreEqual(1u, Neuron::TextScaleForSize(12.0, 16.0, 9.0), L"the 12 px default draws at 16 px");
    Assert::AreEqual(2u, Neuron::TextScaleForSize(24.0, 16.0, 9.0), L"24 px titles at 32 px");
    Assert::AreEqual(1u, Neuron::TextScaleForSize(19.9, 16.0, 9.0), L"rounded half up: 1.24 is 1");
    Assert::AreEqual(1u, Neuron::TextScaleForSize(0.0, 16.0, 0.0), L"never below one");
    Assert::AreEqual(2u, Neuron::TextScaleForSize(12.0, 16.0, 19.0), L"raised to the minimum at 2160");
    Assert::AreEqual(3u, Neuron::TextScaleForSize(12.0, 16.0, 38.0), L"and above it");
    Assert::AreEqual(1u, Neuron::TextScaleForSize(12.0, 0.0, 9.0), L"a face without a design size draws at 1");
    const std::uint32_t bounded = Neuron::TextScaleForSize(12.0, 16.0, std::numeric_limits<double>::infinity());
    Assert::IsTrue(bounded >= 1 && bounded <= 64, L"an unreachable minimum is bounded");
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

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui192AnOutlinedLineDrawsEightInkCopiesThenItsFill)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-192")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-190")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-180")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui192AnOutlinedLineDrawsEightInkCopiesThenItsFill)
  {
    const Neuron::FontAsset face = SmallFace();
    Neuron::TextStyle style;
    style.scale = 2;
    style.outline = std::array<float, 4>{1.0f, 1.0f, 1.0f, 0.0f};
    style.outlineOffsetPixels = 1;
    std::vector<Neuron::UiBatch> batches;
    Neuron::AppendText(batches, face, "AB", 10.4f, 20.6f, style);
    Assert::AreEqual(std::size_t{2}, batches.size(), L"the outline pass, then the fill pass");
    Assert::IsTrue(batches[0].source == Neuron::UiSource::Glyphs && batches[0].blend == Neuron::UiBlend::Ink, L"the outline inks");
    Assert::AreEqual(std::size_t{16}, batches[0].quads.size(), L"eight copies of two glyphs");
    Assert::IsTrue(batches[1].blend == Neuron::UiBlend::Additive && batches[1].quads.size() == 2, L"normal text is additive");
    const Neuron::UiQuad& fill = batches[1].quads[0];
    Assert::AreEqual(10.0f, fill.rectangle[0], L"the line's corner on a whole pixel");
    Assert::AreEqual(21.0f, fill.rectangle[1]);
    Assert::AreEqual(fill.rectangle[2] - fill.rectangle[0], 12.0f, L"the glyph's 6 px at the scale 2");
    const Neuron::UiQuad& first = batches[0].quads[0];
    Assert::AreEqual(fill.rectangle[0] - 1.0f, first.rectangle[0], L"the first copy one pixel to the left");
    Assert::AreEqual(fill.rectangle[1], first.rectangle[1]);
    Assert::AreEqual(0.0f, first.color[3], L"in the outline's colour");
    const Neuron::UiQuad& last = batches[0].quads[14];
    Assert::AreEqual(fill.rectangle[0] + 1.0f, last.rectangle[0], L"the last copy down and to the right");
    Assert::AreEqual(fill.rectangle[1] + 1.0f, last.rectangle[1]);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O63ACharacterTheFaceLacksDrawsItsMissingGlyph)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O63ACharacterTheFaceLacksDrawsItsMissingGlyph)
  {
    const Neuron::FontAsset face = SmallFace();
    std::vector<Neuron::UiBatch> missing;
    Neuron::AppendText(missing, face, "\xD0\x96", 0.0f, 0.0f, {}); // U+0416
    std::vector<Neuron::UiBatch> question;
    Neuron::AppendText(question, face, "?", 0.0f, 0.0f, {});
    Assert::IsTrue(missing.size() == 1 && question.size() == 1 && missing[0].quads.size() == 1, L"one glyph");
    Assert::IsTrue(missing[0].quads[0].uv == question[0].quads[0].uv, L"the missing glyph's cell");
    std::vector<Neuron::UiBatch> none;
    Neuron::AppendText(none, face, "", 0.0f, 0.0f, {});
    Assert::IsTrue(none.empty(), L"an empty line adds no batch");
  }
};

} // namespace NeuronClientTests
