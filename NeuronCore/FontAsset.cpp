#include "pch.h"

#include "CookedRecord.h"
#include "FontAsset.h"

#include <algorithm>
#include <format>

namespace Neuron
{

const FontGlyph* FindGlyph(const FontAsset& _font, std::uint32_t _codePoint) noexcept
{
  const auto found = std::ranges::lower_bound(_font.glyphs, _codePoint, {}, &FontGlyph::codePoint);
  return found != _font.glyphs.end() && found->codePoint == _codePoint ? &*found : nullptr;
}

std::vector<std::byte> WriteFontAsset(const FontAsset& _font)
{
  CookedWriter out;
  out.U32(FONT_ASSET_VERSION);
  out.U16(_font.designSizePixels);
  out.U16(_font.lineHeightPixels);
  out.U16(_font.baselinePixels);
  out.U16(_font.cellWidthPixels);
  out.U16(_font.cellHeightPixels);
  out.U32(_font.missingCodePoint);
  out.U16(_font.pageWidth);
  out.U16(_font.pageHeight);
  out.U32(static_cast<std::uint32_t>(_font.glyphs.size()));
  for (const FontGlyph& glyph : _font.glyphs)
  {
    out.U32(glyph.codePoint);
    out.U16(glyph.x);
    out.U16(glyph.y);
    out.U16(glyph.widthPixels);
    out.I16(glyph.bearingPixels);
    out.U16(glyph.advancePixels);
    out.I16(glyph.offsetYPixels);
  }
  out.Append(std::as_bytes(std::span(_font.page)));
  return out.Take();
}

std::expected<FontAsset, std::string> ReadFontAsset(std::span<const std::byte> _entry)
{
  CookedReader in(_entry);
  if (const std::uint32_t version = in.U32(); version != FONT_ASSET_VERSION)
  {
    return std::unexpected(std::format("the cooked font is version {}; this build reads version {} only", version, FONT_ASSET_VERSION));
  }
  FontAsset font;
  font.designSizePixels = in.U16();
  font.lineHeightPixels = in.U16();
  font.baselinePixels = in.U16();
  font.cellWidthPixels = in.U16();
  font.cellHeightPixels = in.U16();
  font.missingCodePoint = in.U32();
  font.pageWidth = in.U16();
  font.pageHeight = in.U16();
  const std::uint32_t glyphs = in.U32();
  if (in.Failed() || glyphs > in.Remaining())
  {
    return std::unexpected(std::string("the cooked font is shorter than its header and glyphs"));
  }
  font.glyphs.resize(glyphs);
  for (FontGlyph& glyph : font.glyphs)
  {
    glyph.codePoint = in.U32();
    glyph.x = in.U16();
    glyph.y = in.U16();
    glyph.widthPixels = in.U16();
    glyph.bearingPixels = in.I16();
    glyph.advancePixels = in.U16();
    glyph.offsetYPixels = in.I16();
    const std::uint32_t right = std::uint32_t{glyph.x} + font.cellWidthPixels;
    const std::uint32_t bottom = std::uint32_t{glyph.y} + font.cellHeightPixels;
    if (right > std::uint32_t{font.pageWidth} || bottom > std::uint32_t{font.pageHeight})
    {
      return std::unexpected(std::format("the glyph of U+{:04X} lies off the page", glyph.codePoint));
    }
  }
  const auto outOfOrder =
    std::ranges::adjacent_find(font.glyphs, [](const FontGlyph& _a, const FontGlyph& _b) { return _a.codePoint >= _b.codePoint; });
  if (outOfOrder != font.glyphs.end())
  {
    return std::unexpected(std::string("the glyphs are not sorted by code point, each once"));
  }
  const std::span<const std::byte> page = in.Bytes(std::size_t{font.pageWidth} * font.pageHeight);
  if (!in.Finished())
  {
    return std::unexpected(std::string("the cooked font's page is not its width times its height in bytes"));
  }
  font.page.resize(page.size());
  std::ranges::transform(page, font.page.begin(), [](std::byte _value) { return static_cast<std::uint8_t>(_value); });
  if (FindGlyph(font, font.missingCodePoint) == nullptr)
  {
    return std::unexpected(std::format("the missing glyph U+{:04X} is not a glyph of the face", font.missingCodePoint));
  }
  return font;
}

} // namespace Neuron
