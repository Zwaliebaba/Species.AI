#include "pch.h"

#include "GlyphAtlas.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

Neuron::GlyphKey KeyOf(std::uint32_t _glyph) noexcept
{
  return Neuron::GlyphKey{0, _glyph, 16 * 16, 0};
}

std::vector<std::uint8_t> Coverage(std::uint32_t _width, std::uint32_t _height, std::uint8_t _value)
{
  return std::vector<std::uint8_t>(std::size_t{_width} * _height, _value);
}

// The slot of a glyph inserted, after the assertion that it was; an empty slot when it was not.
Neuron::GlyphSlot Inserted(Neuron::GlyphAtlas& _atlas, std::uint32_t _glyph, std::uint32_t _width, std::uint32_t _height,
                           std::int32_t _left = 0, std::int32_t _top = 0)
{
  const std::optional<Neuron::GlyphSlot> slot = _atlas.Insert(KeyOf(_glyph), _width, _height, _left, _top, Coverage(_width, _height, 200));
  Assert::IsTrue(slot.has_value(), L"cached");
  return slot.value_or(Neuron::GlyphSlot{});
}

} // namespace

TEST_CLASS(GlyphAtlasTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ui198AGlyphIsCachedWithABorderOfEmptyTexels)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui198AGlyphIsCachedWithABorderOfEmptyTexels)
  {
    Neuron::GlyphAtlas atlas({64, 2});
    Assert::IsFalse(atlas.Find(KeyOf(1)).has_value(), L"nothing cached");
    const Neuron::GlyphSlot first = Inserted(atlas, 1, 4, 6, -1, -5);
    Assert::IsTrue(first.x == 1 && first.y == 1 && first.width == 4 && first.height == 6, L"one texel in from the corner");
    Assert::IsTrue(first.left == -1 && first.top == -5, L"its offset from the pen");
    const Neuron::GlyphSlot second = Inserted(atlas, 2, 3, 3, 0, -3);
    Assert::AreEqual(std::uint16_t{7}, second.x, L"the next on the shelf, one texel of space between");
    const std::span<const Neuron::GlyphPage> pages = atlas.Pages();
    Assert::AreEqual(std::size_t{1}, pages.size());
    Assert::AreEqual(std::uint8_t{200}, pages[0].texels[std::size_t{1} * 64 + 1], L"the coverage on the page");
    Assert::AreEqual(std::uint8_t{0}, pages[0].texels[std::size_t{1} * 64 + 5], L"and the border empty");
    Assert::AreEqual(std::uint64_t{2}, pages[0].revision, L"each change raises the revision");
    const Neuron::GlyphSlot found = atlas.Find(KeyOf(1)).value_or(Neuron::GlyphSlot{});
    Assert::IsTrue(found.x == first.x && found.y == first.y && found.width == 4, L"found again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui198AFullShelfOpensTheNextAndAFullPageTheNext)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui198AFullShelfOpensTheNextAndAFullPageTheNext)
  {
    // 30 texels with their border: two to a 64-texel shelf, two shelves to a page.
    Neuron::GlyphAtlas atlas({64, 4});
    std::vector<Neuron::GlyphSlot> slots;
    slots.reserve(5);
    for (std::uint32_t n = 0; n < 5; ++n)
    {
      slots.push_back(Inserted(atlas, n, 28, 28));
    }
    Assert::IsTrue(slots[1].y == slots[0].y && slots[1].x > slots[0].x, L"the second beside the first");
    Assert::IsTrue(slots[2].y > slots[0].y && slots[2].x == 1, L"the third on the next shelf");
    Assert::IsTrue(slots[4].page == 1 && slots[4].x == 1 && slots[4].y == 1, L"the fifth on a new page");
    Assert::AreEqual(std::size_t{2}, atlas.Pages().size());
    Assert::IsFalse(atlas.Insert(KeyOf(9), 63, 10, 0, 0, Coverage(63, 10, 255)).has_value(), L"wider than a page with its border");
    const std::optional<Neuron::GlyphSlot> empty = atlas.Insert(KeyOf(10), 0, 0, 0, 0, {});
    Assert::IsTrue(empty.has_value() && empty.value_or(Neuron::GlyphSlot{1, 1, 1, 1, 1}).width == 0,
                   L"a glyph that draws nothing takes no texels");
    Assert::AreEqual(std::size_t{2}, atlas.Pages().size());
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Ui198AFullCacheClearsItsLeastRecentlyUsedPage)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-198")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ui198AFullCacheClearsItsLeastRecentlyUsedPage)
  {
    Neuron::GlyphAtlas atlas({32, 2});
    atlas.BeginFrame(1);
    const Neuron::GlyphSlot onFirst = Inserted(atlas, 1, 30, 30);
    atlas.BeginFrame(2);
    const Neuron::GlyphSlot onSecond = Inserted(atlas, 2, 30, 30);
    Assert::IsTrue(onFirst.page == 0 && onSecond.page == 1, L"a page each");
    atlas.BeginFrame(3);
    atlas.MarkUsed(0); // page 0 drawn this frame from a kept layout
    const Neuron::GlyphSlot third = Inserted(atlas, 3, 30, 30);
    Assert::AreEqual(std::uint16_t{1}, third.page, L"page 1 was used least recently");
    Assert::AreEqual(std::uint64_t{1}, atlas.Clears());
    Assert::IsFalse(atlas.Find(KeyOf(2)).has_value(), L"its glyphs are gone");
    Assert::IsTrue(atlas.Find(KeyOf(1)).has_value(), L"the other page's stay");
    const Neuron::GlyphPage& cleared = atlas.Pages()[1];
    Assert::IsTrue(cleared.clearedRevision > 0 && cleared.revision == cleared.clearedRevision + 1,
                   L"its revision when cleared is kept, so that an older copy of it is not drawn");
  }
};

} // namespace NeuronClientTests
