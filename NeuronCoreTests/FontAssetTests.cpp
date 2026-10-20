#include "pch.h"

#include "FontAsset.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// A face of two 16 px glyphs on a 32 × 16 page, its missing glyph '?'.
Neuron::FontAsset TwoGlyphs()
{
  Neuron::FontAsset font;
  font.designSizePixels = 16;
  font.lineHeightPixels = 16;
  font.baselinePixels = 13;
  font.cellWidthPixels = 16;
  font.cellHeightPixels = 16;
  font.missingCodePoint = 63;
  font.pageWidth = 32;
  font.pageHeight = 16;
  font.glyphs.push_back(Neuron::FontGlyph{32, 0, 0, 16, 0, 16, 0});
  font.glyphs.push_back(Neuron::FontGlyph{63, 16, 0, 16, 0, 16, 0});
  font.page.assign(std::size_t{32} * 16, 0);
  font.page[40] = 255;
  return font;
}

} // namespace

TEST_CLASS(FontAssetTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dat219ACookedFaceReadsBackSortedByCodePoint)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-219")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat219ACookedFaceReadsBackSortedByCodePoint)
  {
    const std::vector<std::byte> bytes = Neuron::WriteFontAsset(TwoGlyphs());
    const auto font = Neuron::ReadFontAsset(bytes);
    Assert::IsTrue(font.has_value(), L"the entry reads");
    Assert::IsTrue(Neuron::WriteFontAsset(*font) == bytes, L"and writes the same bytes again (AST-091)");
    ExpectEqual(13, font->baselinePixels, L"the baseline");
    ExpectEqual(255, font->page[40], L"the page's coverage");
    const Neuron::FontGlyph* question = Neuron::FindGlyph(*font, 63);
    Assert::IsTrue(question != nullptr && question->x == 16, L"a glyph by its code point");
    Assert::IsTrue(Neuron::FindGlyph(*font, 65) == nullptr, L"a code point the face does not hold");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ast091ABrokenFaceIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-091")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast091ABrokenFaceIsRefused)
  {
    Neuron::FontAsset font = TwoGlyphs();
    std::swap(font.glyphs[0], font.glyphs[1]);
    Assert::IsFalse(Neuron::ReadFontAsset(Neuron::WriteFontAsset(font)).has_value(), L"glyphs out of order");
    font = TwoGlyphs();
    font.glyphs[1].codePoint = 32;
    Assert::IsFalse(Neuron::ReadFontAsset(Neuron::WriteFontAsset(font)).has_value(), L"a code point twice");
    font = TwoGlyphs();
    font.glyphs[1].x = 17;
    Assert::IsFalse(Neuron::ReadFontAsset(Neuron::WriteFontAsset(font)).has_value(), L"a cell off the page");
    font = TwoGlyphs();
    font.missingCodePoint = 65;
    Assert::IsFalse(Neuron::ReadFontAsset(Neuron::WriteFontAsset(font)).has_value(), L"a missing glyph the face does not hold");
    const std::vector<std::byte> bytes = Neuron::WriteFontAsset(TwoGlyphs());
    Assert::IsFalse(Neuron::ReadFontAsset(std::span(bytes).first(bytes.size() - 1)).has_value(), L"a page a byte short");
  }
};

} // namespace NeuronCoreTests
