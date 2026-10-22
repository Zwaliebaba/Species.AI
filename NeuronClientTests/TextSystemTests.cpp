#include "pch.h"

#include "TestSupport.h"
#include "TextSystem.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// The chains of the game's roles (GameShared/GameFonts.h), which every supported Windows can draw from: Segoe UI is
// installed wherever Bahnschrift is not.
Neuron::TextSystem::Desc GameDesc()
{
  Neuron::TextSystem::Desc desc;
  for (std::vector<std::string>& chain : desc.families)
  {
    chain = {"Bahnschrift", "Segoe UI"};
  }
  desc.families[static_cast<std::size_t>(Neuron::FontRole::Development)] = {"Cascadia Mono", "Consolas"};
  return desc;
}

bool OnWholePixels(const Neuron::PlacedGlyph& _glyph) noexcept
{
  for (const float edge : _glyph.rectangle)
  {
    if (edge != std::floor(edge))
    {
      return false;
    }
  }
  return true;
}

} // namespace

TEST_CLASS(TextSystemTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui198ATextIsLaidOutInItsRoleAndItsGlyphsCached)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-200")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui198ATextIsLaidOutInItsRoleAndItsGlyphsCached)
  {
    Neuron::TextSystem text(GameDesc());
    Assert::IsTrue(text.Available(), L"DirectWrite opened");
    const std::string family = text.FaceFamily(Neuron::FontRole::Body);
    Assert::IsTrue(family == "Bahnschrift" || family == "Segoe UI", L"the first family of the chain the system holds");
    text.BeginFrame(1);
    const Neuron::TextLine line = text.Layout("Species", Neuron::FontRole::Body, 16.0f);
    Assert::AreEqual(std::size_t{7}, line.glyphs.size(), L"a glyph a letter");
    Assert::IsTrue(line.widthPixels > 0.0f && line.heightPixels >= 16.0f, L"measured (UI-200)");
    Assert::IsTrue(line.baselinePixels > 0.0f && line.baselinePixels <= line.heightPixels, L"its baseline inside the line");
    Assert::AreEqual(line.baselinePixels, std::floor(line.baselinePixels), L"on a whole pixel");
    const float side = static_cast<float>(text.Atlas()->PageSide());
    for (const Neuron::PlacedGlyph& glyph : line.glyphs)
    {
      Assert::IsTrue(OnWholePixels(glyph), L"every glyph one texel a pixel");
      Assert::AreEqual(glyph.rectangle[2] - glyph.rectangle[0], (glyph.uv[2] - glyph.uv[0]) * side, 1e-3f, L"its width in texels");
      Assert::IsTrue(glyph.rectangle[0] >= -2.0f && glyph.rectangle[2] <= line.widthPixels + 2.0f, L"inside its line");
    }
    const std::uint64_t rasterized = text.GlyphsRasterized();
    Assert::IsTrue(rasterized >= 6 && rasterized <= 7, L"each glyph rasterized once at its offset");
    static_cast<void>(text.Layout("Species", Neuron::FontRole::Body, 16.0f));
    text.BeginFrame(1000); // the kept layouts are dropped; the atlas keeps the glyphs
    const Neuron::TextLine again = text.Layout("Species", Neuron::FontRole::Body, 16.0f);
    Assert::AreEqual(rasterized, text.GlyphsRasterized(), L"none rasterized again (UI-241)");
    Assert::IsTrue(again.glyphs.size() == line.glyphs.size() && again.glyphs[0].uv == line.glyphs[0].uv, L"from the same slots");
    const Neuron::TextLine larger = text.Layout("Species", Neuron::FontRole::Body, 32.0f);
    Assert::IsTrue(larger.widthPixels > 1.8f * line.widthPixels, L"rasterized at its final size, not scaled");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui198TheMetricsOfALineHoldNoGlyphs)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui198TheMetricsOfALineHoldNoGlyphs)
  {
    Neuron::TextSystem text(GameDesc());
    const Neuron::TextLine metrics = text.Metrics(Neuron::FontRole::Development, 14.0f);
    Assert::IsTrue(metrics.glyphs.empty() && metrics.widthPixels == 0.0f, L"no glyphs and no width");
    Assert::IsTrue(metrics.heightPixels >= 14.0f && metrics.baselinePixels > 0.0f, L"a line's height and baseline");
    const Neuron::TextLine empty = text.Layout("", Neuron::FontRole::Development, 14.0f);
    Assert::AreEqual(metrics.heightPixels, empty.heightPixels, L"an empty text is the metrics");
    Assert::IsTrue(text.Layout("x", Neuron::FontRole::Body, 0.0f).glyphs.empty(), L"no size draws nothing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui199ACharacterTheFaceLacksIsDrawnByTheChain)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-199")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-234")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui199ACharacterTheFaceLacksIsDrawnByTheChain)
  {
    // Ж is Segoe UI's when Bahnschrift lacks it; either way it draws a glyph of its own, not an empty one.
    Neuron::TextSystem text(GameDesc());
    const Neuron::TextLine line = text.Layout("\xD0\x96", Neuron::FontRole::Body, 20.0f);
    Assert::AreEqual(std::size_t{1}, line.glyphs.size(), L"a glyph");
    Assert::IsTrue(line.glyphs[0].rectangle[2] - line.glyphs[0].rectangle[0] >= 8.0f, L"of a letter's width");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui198ARoleWithoutAFamilyDrawsNothing)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui198ARoleWithoutAFamilyDrawsNothing)
  {
    Neuron::TextSystem::Desc desc = GameDesc();
    desc.families[static_cast<std::size_t>(Neuron::FontRole::Caption)].clear();
    Neuron::TextSystem text(desc);
    Assert::IsTrue(text.Layout("caption", Neuron::FontRole::Caption, 16.0f).glyphs.empty(), L"logged, and nothing drawn");
    Assert::IsFalse(text.Layout("body", Neuron::FontRole::Body, 16.0f).glyphs.empty(), L"the other roles draw");
  }
};

} // namespace NeuronClientTests
