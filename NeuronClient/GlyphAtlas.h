#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <vector>

namespace Neuron
{

/// What a rasterized glyph is cached by (UI-198, ADR-147): its font face's index in the text system, its glyph index in
/// that face, its size in sixteenths of a pixel, and its sub-pixel x offset in steps of 1 ÷ `ui.glyph.subpixel_steps`.
struct GlyphKey
{
  std::uint32_t face = 0;
  std::uint32_t glyph = 0;
  std::uint32_t sizeSixteenths = 0;
  std::uint32_t subpixel = 0;

  friend constexpr auto operator<=>(const GlyphKey&, const GlyphKey&) noexcept = default;
};

/// Where a cached glyph's coverage lies (ADR-147): its page, its rectangle on the page in texels, and the offset of
/// that rectangle's top-left corner from the glyph's pen position on the baseline, in whole pixels. A glyph that draws
/// nothing, a space, has an empty rectangle.
struct GlyphSlot
{
  std::uint16_t page = 0;
  std::uint16_t x = 0;
  std::uint16_t y = 0;
  std::uint16_t width = 0;
  std::uint16_t height = 0;
  std::int16_t left = 0;
  std::int16_t top = 0;
};

/// The open shelf of a page: its top row, its next free column and the height of its tallest glyph, in texels.
struct GlyphShelf
{
  std::uint32_t top = 0;
  std::uint32_t left = 0;
  std::uint32_t height = 0;
};

/// One page of the glyph cache (UI-198): `R8_UNORM` coverage, rows from the top; its revision, raised on every change,
/// by which the renderer knows to upload it again; and the revision it took when it was last cleared, before which a
/// copy of it may hold other glyphs where this one's are.
struct GlyphPage
{
  std::vector<std::uint8_t> texels;
  std::uint64_t revision = 0;
  std::uint64_t clearedRevision = 0;
  std::uint64_t lastUseFrame = 0;
  GlyphShelf shelf;
};

/// The glyph cache's pages on the CPU (UI-198 as ADR-034 specifies DirectWrite's form, ADR-147): glyphs packed on
/// shelves with a texel of space around each, at most `ui.glyph.page_count` pages of `ui.glyph.page_size` texels a
/// side. When no page has room, the least recently used page is cleared and reused, and the glyphs it held are
/// rasterized again on their next use. Every call is made from the frame thread.
class GlyphAtlas
{
public:
  struct Desc
  {
    std::uint32_t pageSide = 1024; // ui.glyph.page_size
    std::uint32_t pageCount = 8;   // ui.glyph.page_count
  };

  explicit GlyphAtlas(Desc _desc);

  // The frame whose use the least-recently-used choice counts.
  void BeginFrame(std::uint64_t _frame) noexcept
  {
    m_frame = _frame;
  }
  // The slot of _key, its page marked as used this frame; none when the glyph is not cached.
  [[nodiscard]] std::optional<GlyphSlot> Find(const GlyphKey& _key);
  // Marks _page as used this frame, for a glyph whose slot was kept from an earlier look-up.
  void MarkUsed(std::uint16_t _page) noexcept
  {
    if (_page < m_pages.size())
    {
      m_pages[_page].lastUseFrame = m_frame;
    }
  }
  // Caches the coverage of _key, _width × _height bytes rows from the top, with its offset from the pen position. None
  // when the glyph is larger than a page.
  [[nodiscard]] std::optional<GlyphSlot> Insert(const GlyphKey& _key, std::uint32_t _width, std::uint32_t _height, std::int32_t _left,
                                                std::int32_t _top, std::span<const std::uint8_t> _coverage);

  [[nodiscard]] std::uint32_t PageSide() const noexcept
  {
    return m_desc.pageSide;
  }
  [[nodiscard]] std::span<const GlyphPage> Pages() const noexcept
  {
    return m_pages;
  }
  // How many times a full cache has cleared a page.
  [[nodiscard]] std::uint64_t Clears() const noexcept
  {
    return m_clears;
  }

private:
  /// Where a glyph fits on a page: its texel's top-left corner, and the page's shelf once it is placed.
  struct Fit
  {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    GlyphShelf shelf;
  };

  // Where a glyph of _width × _height texels with its border fits on _shelf's page, opening a new shelf when the open
  // one is full across; none when the page has no room.
  [[nodiscard]] std::optional<Fit> FitOn(const GlyphShelf& _shelf, std::uint32_t _width, std::uint32_t _height) const noexcept;
  void ClearPage(std::size_t _page);

  Desc m_desc;
  std::vector<GlyphPage> m_pages;
  std::map<GlyphKey, GlyphSlot> m_slots;
  std::uint64_t m_frame = 0;
  std::uint64_t m_clears = 0;
};

} // namespace Neuron
