#pragma once

#include "FontAsset.h"

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace Neuron
{

/// One glyph of a laid-out line: its rectangle on the target in whole pixels and its rectangle on the face's page, both
/// as (left, top, right, bottom); the texture coordinates run from 0 to 1 across the page.
struct GlyphQuad
{
  std::array<float, 4> rectangle;
  std::array<float, 4> uv;
};

// O63: the whole-number scale at which a face is drawn for a display scale of _dpiScale, at least 1.
[[nodiscard]] std::uint32_t TextScaleFor(float _dpiScale) noexcept;

// The code points of UTF-8 text; a malformed sequence gives U+FFFD, which a face draws as its missing glyph.
[[nodiscard]] std::vector<std::uint32_t> CodePointsOf(std::string_view _utf8);

// DAT-219 and O63: the line's width in pixels at _scale, the sum of its glyphs' advances.
[[nodiscard]] float TextWidth(const FontAsset& _font, std::string_view _utf8, std::uint32_t _scale);

// The glyphs of one line at _scale, its cell's top-left at (_left, _top), each pen step a whole number of pixels. A
// character the face does not hold is drawn as the face's missing glyph; a space draws nothing.
[[nodiscard]] std::vector<GlyphQuad> LayoutText(const FontAsset& _font, std::string_view _utf8, std::uint32_t _scale, float _left,
                                                float _top);

} // namespace Neuron
