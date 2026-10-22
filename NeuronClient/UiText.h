#pragma once

#include "FontAsset.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace Neuron
{

// UI-171: the accessibility minimum of a text's final size, in pixels, for a client area _clientHeight pixels tall:
// _minimum1080 × H ÷ 1080 up to 1080, linear from _minimum1080 to _minimum2160 between 1080 and 2160, and
// _minimum2160 × H ÷ 2160 above.
[[nodiscard]] double MinimumTextPixels(std::uint32_t _clientHeight, double _minimum1080, double _minimum2160) noexcept;

// UI-198 as O63 amends it: the whole-number scale at which text of final size _sizePixels is drawn in a face of design
// size _designPixels, ⌊s ÷ d + 0.5⌋, at least 1, raised until k × d reaches _minimumPixels.
[[nodiscard]] std::uint32_t TextScaleForSize(double _sizePixels, double _designPixels, double _minimumPixels) noexcept;

/// How a line of text is drawn (UI-189 to UI-192, ADR-145): its whole-number scale, its fill pass's blend and colour,
/// and, when it has one, its outline pass's colour, drawn first in the ink state at the outline's offset in whole pixels.
struct TextStyle
{
  std::uint32_t scale = 1;
  UiBlend blend = UiBlend::Additive; // UI-190: normal text is additive
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
  std::optional<std::array<float, 4>> outline;
  std::int32_t outlineOffsetPixels = 1;
};

// UI-179: appends _quad to _batches, in the last batch when it has the same source, blend and image and no scissor,
// and in a new batch otherwise, so the order is never changed.
void AppendQuad(std::vector<UiBatch>& _batches, UiSource _source, UiBlend _blend, std::uint16_t _image, const UiQuad& _quad);

// Appends one line of _utf8 in _face with its first cell's top-left corner at (_left, _top), which are rounded to whole
// pixels: when the style has an outline, its pass of eight copies at the outline's offset to the neighbouring positions
// in the ink state (UI-192), then the fill pass. A character the face does not hold draws its missing glyph.
void AppendText(std::vector<UiBatch>& _batches, const FontAsset& _face, std::string_view _utf8, float _left, float _top,
                const TextStyle& _style);

} // namespace Neuron
