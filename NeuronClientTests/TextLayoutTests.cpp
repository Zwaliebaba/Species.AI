#include "pch.h"

#include "FontAsset.h"
#include "TestSupport.h"
#include "TextLayout.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// A face of 8 × 10 cells on a 64 × 32 page: a space, 'A' with a bearing of 1, 'B' with an offset of 2 and '?', the
// missing glyph.
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
    {.codePoint = ' ', .x = 0, .y = 0, .widthPixels = 0, .bearingPixels = 0, .advancePixels = 4, .offsetYPixels = 0},
    {.codePoint = '?', .x = 8, .y = 0, .widthPixels = 6, .bearingPixels = 0, .advancePixels = 7, .offsetYPixels = 0},
    {.codePoint = 'A', .x = 16, .y = 0, .widthPixels = 6, .bearingPixels = 1, .advancePixels = 8, .offsetYPixels = 0},
    {.codePoint = 'B', .x = 24, .y = 10, .widthPixels = 5, .bearingPixels = 0, .advancePixels = 6, .offsetYPixels = 2},
  };
  face.page.assign(std::size_t{face.pageWidth} * face.pageHeight, 0);
  return face;
}

} // namespace

TEST_CLASS(TextLayoutTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(O63TheTextScaleIsTheRoundedDisplayScaleAndAtLeastOne)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O63TheTextScaleIsTheRoundedDisplayScaleAndAtLeastOne)
  {
    Assert::AreEqual(1u, Neuron::TextScaleFor(1.0f));
    Assert::AreEqual(1u, Neuron::TextScaleFor(0.5f), L"never below one");
    Assert::AreEqual(1u, Neuron::TextScaleFor(1.25f));
    Assert::AreEqual(2u, Neuron::TextScaleFor(1.5f));
    Assert::AreEqual(2u, Neuron::TextScaleFor(2.0f));
    Assert::AreEqual(3u, Neuron::TextScaleFor(3.0f));
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg095TheTextIsReadAsUtf8AndAMalformedSequenceIsReplaced)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg095TheTextIsReadAsUtf8AndAMalformedSequenceIsReplaced)
  {
    const std::vector<std::uint32_t> dot = Neuron::CodePointsOf("a\xC2\xB7z");
    Assert::AreEqual(std::size_t{3}, dot.size());
    Assert::AreEqual(0xB7u, dot[1], L"U+00B7 in two bytes");
    const std::vector<std::uint32_t> wide = Neuron::CodePointsOf("\xE2\x80\x94\xF0\x9F\x98\x80");
    Assert::AreEqual(std::size_t{2}, wide.size());
    Assert::AreEqual(0x2014u, wide[0]);
    Assert::AreEqual(0x1F600u, wide[1]);
    const std::vector<std::uint32_t> broken = Neuron::CodePointsOf("\xC2\x61\xFF"); // a lead byte, 'a' and a byte no sequence starts
    Assert::AreEqual(std::size_t{3}, broken.size(), L"a lead byte without its continuation is replaced, and the next byte read");
    Assert::AreEqual(0xFFFDu, broken[0]);
    Assert::AreEqual(static_cast<std::uint32_t>('a'), broken[1]);
    Assert::AreEqual(0xFFFDu, broken[2]);
    Assert::AreEqual(std::size_t{2}, Neuron::CodePointsOf("\xE2\x80").size(), L"a sequence cut short at the end: each byte replaced");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg095ALineIsLaidOutInWholePixelsAtTheTextScale)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg095ALineIsLaidOutInWholePixelsAtTheTextScale)
  {
    const Neuron::FontAsset face = SmallFace();
    Assert::AreEqual(36.0f, Neuron::TextWidth(face, "A B", 2), L"(8 + 4 + 6) × 2");
    const std::vector<Neuron::GlyphQuad> quads = Neuron::LayoutText(face, "A B", 2, 100.4f, 50.6f);
    Assert::AreEqual(std::size_t{2}, quads.size(), L"the space draws nothing");
    // 'A': the pen at 100, its bearing of 1 scaled to 2, its cell 6 × 10 at 2.
    Assert::AreEqual(102.0f, quads[0].rectangle[0]);
    Assert::AreEqual(51.0f, quads[0].rectangle[1]);
    Assert::AreEqual(114.0f, quads[0].rectangle[2]);
    Assert::AreEqual(71.0f, quads[0].rectangle[3]);
    Assert::AreEqual(16.0f / 64.0f, quads[0].uv[0]);
    Assert::AreEqual(22.0f / 64.0f, quads[0].uv[2]);
    Assert::AreEqual(10.0f / 32.0f, quads[0].uv[3]);
    // 'B': the pen at 100 + (8 + 4) × 2, its offset of 2 scaled to 4.
    Assert::AreEqual(124.0f, quads[1].rectangle[0]);
    Assert::AreEqual(55.0f, quads[1].rectangle[1]);
    Assert::AreEqual(10.0f / 32.0f, quads[1].uv[1]);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg095ACharacterTheFaceDoesNotHoldIsDrawnAsItsMissingGlyph)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg095ACharacterTheFaceDoesNotHoldIsDrawnAsItsMissingGlyph)
  {
    const Neuron::FontAsset face = SmallFace();
    const std::vector<Neuron::GlyphQuad> quads = Neuron::LayoutText(face, "A\xC2\xB7", 1, 0.0f, 0.0f);
    Assert::AreEqual(std::size_t{2}, quads.size());
    Assert::AreEqual(8.0f / 64.0f, quads[1].uv[0], L"the page cell of '?'");
    Assert::AreEqual(15.0f, Neuron::TextWidth(face, "A\xC2\xB7", 1), L"its advance too");
    Neuron::FontAsset empty = face;
    empty.pageWidth = 0;
    Assert::IsTrue(Neuron::LayoutText(empty, "A", 1, 0.0f, 0.0f).empty(), L"a face with no page draws nothing");
  }
};

} // namespace NeuronClientTests
