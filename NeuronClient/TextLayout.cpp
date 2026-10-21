#include "pch.h"

#include "TextLayout.h"

#include <algorithm>
#include <cmath>

namespace Neuron
{

namespace
{

constexpr std::uint32_t REPLACEMENT_CHARACTER = 0xFFFD;

const FontGlyph* GlyphOf(const FontAsset& _font, std::uint32_t _codePoint) noexcept
{
  const FontGlyph* glyph = FindGlyph(_font, _codePoint);
  return glyph != nullptr ? glyph : FindGlyph(_font, _font.missingCodePoint);
}

} // namespace

std::uint32_t TextScaleFor(float _dpiScale) noexcept
{
  return static_cast<std::uint32_t>(std::max(1.0f, std::round(_dpiScale)));
}

std::vector<std::uint32_t> CodePointsOf(std::string_view _utf8)
{
  std::vector<std::uint32_t> codePoints;
  codePoints.reserve(_utf8.size());
  for (std::size_t i = 0; i < _utf8.size();)
  {
    const auto lead = static_cast<std::uint8_t>(_utf8[i]);
    const std::size_t length = lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 0;
    if (length == 0 || i + length > _utf8.size())
    {
      codePoints.push_back(REPLACEMENT_CHARACTER);
      ++i;
      continue;
    }
    std::uint32_t codePoint = length == 1 ? lead : lead & (0x7Fu >> length);
    bool valid = true;
    for (std::size_t k = 1; k < length; ++k)
    {
      const auto next = static_cast<std::uint8_t>(_utf8[i + k]);
      valid = valid && (next & 0xC0) == 0x80;
      codePoint = (codePoint << 6) | (next & 0x3Fu);
    }
    codePoints.push_back(valid ? codePoint : REPLACEMENT_CHARACTER);
    i += valid ? length : 1;
  }
  return codePoints;
}

float TextWidth(const FontAsset& _font, std::string_view _utf8, std::uint32_t _scale)
{
  float width = 0.0f;
  for (const std::uint32_t codePoint : CodePointsOf(_utf8))
  {
    if (const FontGlyph* glyph = GlyphOf(_font, codePoint); glyph != nullptr)
    {
      width += static_cast<float>(glyph->advancePixels * _scale);
    }
  }
  return width;
}

std::vector<GlyphQuad> LayoutText(const FontAsset& _font, std::string_view _utf8, std::uint32_t _scale, float _left, float _top)
{
  std::vector<GlyphQuad> quads;
  if (_font.pageWidth == 0 || _font.pageHeight == 0)
  {
    return quads;
  }
  const auto scale = static_cast<float>(_scale);
  const float pageWidth = _font.pageWidth;
  const float pageHeight = _font.pageHeight;
  const float cellHeight = _font.cellHeightPixels;
  float pen = std::round(_left);
  const float top = std::round(_top);
  for (const std::uint32_t codePoint : CodePointsOf(_utf8))
  {
    const FontGlyph* glyph = GlyphOf(_font, codePoint);
    if (glyph == nullptr)
    {
      continue;
    }
    if (glyph->widthPixels > 0)
    {
      const float left = pen + static_cast<float>(glyph->bearingPixels) * scale;
      const float glyphTop = top + static_cast<float>(glyph->offsetYPixels) * scale;
      const float width = static_cast<float>(glyph->widthPixels);
      quads.push_back({{left, glyphTop, left + width * scale, glyphTop + cellHeight * scale},
                       {static_cast<float>(glyph->x) / pageWidth, static_cast<float>(glyph->y) / pageHeight,
                        (static_cast<float>(glyph->x) + width) / pageWidth, (static_cast<float>(glyph->y) + cellHeight) / pageHeight}});
    }
    pen += static_cast<float>(glyph->advancePixels) * scale;
  }
  return quads;
}

} // namespace Neuron
