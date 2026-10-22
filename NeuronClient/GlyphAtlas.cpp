#include "pch.h"

#include "GlyphAtlas.h"

#include <algorithm>
#include <cstring>
#include <iterator>

namespace Neuron
{

namespace
{

// The empty texels kept around every glyph, so that a filtered read never takes a neighbor's coverage.
constexpr std::uint32_t GLYPH_BORDER = 1;

} // namespace

GlyphAtlas::GlyphAtlas(Desc _desc)
  : m_desc(_desc)
{
  m_desc.pageSide = std::max(m_desc.pageSide, 16u);
  m_desc.pageCount = std::max(m_desc.pageCount, 1u);
}

std::optional<GlyphSlot> GlyphAtlas::Find(const GlyphKey& _key)
{
  const auto found = m_slots.find(_key);
  if (found == m_slots.end())
  {
    return std::nullopt;
  }
  if (found->second.page < m_pages.size())
  {
    m_pages[found->second.page].lastUseFrame = m_frame;
  }
  return found->second;
}

std::optional<GlyphAtlas::Fit> GlyphAtlas::FitOn(const GlyphShelf& _shelf, std::uint32_t _width, std::uint32_t _height) const noexcept
{
  const std::uint32_t side = m_desc.pageSide;
  const std::uint32_t width = _width + 2 * GLYPH_BORDER;
  const std::uint32_t height = _height + 2 * GLYPH_BORDER;
  GlyphShelf shelf = _shelf;
  if (shelf.left + width > side)
  {
    // The open shelf is full across: the next one starts under it.
    shelf = GlyphShelf{shelf.top + shelf.height, 0, 0};
  }
  if (shelf.top + height > side || width > side)
  {
    return std::nullopt;
  }
  const Fit fit{shelf.left + GLYPH_BORDER, shelf.top + GLYPH_BORDER,
                GlyphShelf{shelf.top, shelf.left + width, std::max(shelf.height, height)}};
  return fit;
}

void GlyphAtlas::ClearPage(std::size_t _page)
{
  GlyphPage& page = m_pages[_page];
  std::ranges::fill(page.texels, std::uint8_t{0});
  page.shelf = {};
  ++page.revision;
  page.clearedRevision = page.revision;
  std::erase_if(m_slots, [&](const auto& _entry) { return _entry.second.page == _page && _entry.second.width > 0; });
  ++m_clears;
}

std::optional<GlyphSlot> GlyphAtlas::Insert(const GlyphKey& _key, std::uint32_t _width, std::uint32_t _height, std::int32_t _left,
                                            std::int32_t _top, std::span<const std::uint8_t> _coverage)
{
  GlyphSlot slot;
  slot.left = static_cast<std::int16_t>(std::clamp(_left, -32768, 32767));
  slot.top = static_cast<std::int16_t>(std::clamp(_top, -32768, 32767));
  if (_width == 0 || _height == 0)
  {
    m_slots[_key] = slot;
    return slot;
  }
  if (_width + 2 * GLYPH_BORDER > m_desc.pageSide || _height + 2 * GLYPH_BORDER > m_desc.pageSide ||
      _coverage.size() < std::size_t{_width} * _height)
  {
    return std::nullopt;
  }
  // The pages with room first, in order; then a new page; then the least recently used page, cleared.
  std::optional<std::size_t> chosen;
  std::optional<Fit> fit;
  for (std::size_t n = 0; n < m_pages.size() && !chosen; ++n)
  {
    fit = FitOn(m_pages[n].shelf, _width, _height);
    if (fit)
    {
      chosen = n;
    }
  }
  if (!chosen && m_pages.size() < m_desc.pageCount)
  {
    GlyphPage page;
    page.texels.assign(std::size_t{m_desc.pageSide} * m_desc.pageSide, std::uint8_t{0});
    m_pages.push_back(std::move(page));
    chosen = m_pages.size() - 1;
    fit = FitOn(m_pages.back().shelf, _width, _height);
  }
  if (!chosen)
  {
    const auto oldest = std::ranges::min_element(m_pages, {}, &GlyphPage::lastUseFrame);
    const auto index = static_cast<std::size_t>(std::distance(m_pages.begin(), oldest));
    ClearPage(index);
    chosen = index;
    fit = FitOn(m_pages[index].shelf, _width, _height);
  }
  if (!fit)
  {
    return std::nullopt; // not reached: a glyph that fits an empty page was checked above
  }
  GlyphPage& page = m_pages[*chosen];
  for (std::uint32_t row = 0; row < _height; ++row)
  {
    std::memcpy(page.texels.data() + (std::size_t{fit->y} + row) * m_desc.pageSide + fit->x, _coverage.data() + std::size_t{row} * _width,
                _width);
  }
  page.shelf = fit->shelf;
  ++page.revision;
  page.lastUseFrame = m_frame;
  slot.page = static_cast<std::uint16_t>(*chosen);
  slot.x = static_cast<std::uint16_t>(fit->x);
  slot.y = static_cast<std::uint16_t>(fit->y);
  slot.width = static_cast<std::uint16_t>(_width);
  slot.height = static_cast<std::uint16_t>(_height);
  m_slots[_key] = slot;
  return slot;
}

} // namespace Neuron
