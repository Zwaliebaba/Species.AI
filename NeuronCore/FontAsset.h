#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace Neuron
{

// The version of the cooked font format; a reader accepts this version only (ADR-034, ADR-108).
inline constexpr std::uint32_t FONT_ASSET_VERSION = 1;

/// One glyph of a cooked face (DAT-219): its code point, the top-left texel of its cell on the page, and its ink
/// width, bearing, advance and vertical offset, every length in pixels of the face's one sheet (O63).
struct FontGlyph
{
  std::uint32_t codePoint = 0;
  std::uint16_t x = 0;
  std::uint16_t y = 0;
  std::uint16_t widthPixels = 0;
  std::int16_t bearingPixels = 0;
  std::uint16_t advancePixels = 0;
  std::int16_t offsetYPixels = 0;
};

/// The cooked `Font` entry of one drawn face (ADR-034 as amended by O56 and O63, DAT-219): the face's metrics, its
/// glyphs sorted by code point, and its one R8_UNORM coverage page, rows from the top, never block-compressed
/// (ADR-017). The UI fills its glyph page from it; nothing is rasterised at run time.
struct FontAsset
{
  std::uint16_t designSizePixels = 0;
  std::uint16_t lineHeightPixels = 0;
  std::uint16_t baselinePixels = 0;
  std::uint16_t cellWidthPixels = 0;
  std::uint16_t cellHeightPixels = 0;
  std::uint32_t missingCodePoint = 0; // the glyph drawn for a character the face does not hold
  std::uint16_t pageWidth = 0;
  std::uint16_t pageHeight = 0;
  std::vector<FontGlyph> glyphs;
  std::vector<std::uint8_t> page; // pageWidth × pageHeight coverage values
};

// The glyph of a code point, or null when the face does not hold it.
[[nodiscard]] const FontGlyph* FindGlyph(const FontAsset& _font, std::uint32_t _codePoint) noexcept;

// The entry bytes of a cooked face, in the layout of ADR-108.
[[nodiscard]] std::vector<std::byte> WriteFontAsset(const FontAsset& _font);
// A cooked face, or why the bytes are not one of this version: a glyph out of order or off the page, a page of the
// wrong size, or a missing glyph the face does not hold.
[[nodiscard]] std::expected<FontAsset, std::string> ReadFontAsset(std::span<const std::byte> _entry);

} // namespace Neuron
