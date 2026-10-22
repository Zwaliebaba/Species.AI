#pragma once

#include "TextSystem.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Neuron
{

// UI-171: the accessibility minimum of a text's final size, in pixels, for a client area _clientHeight pixels tall:
// _minimum1080 × H ÷ 1080 up to 1080, linear from _minimum1080 to _minimum2160 between 1080 and 2160, and
// _minimum2160 × H ÷ 2160 above.
[[nodiscard]] double MinimumTextPixels(std::uint32_t _clientHeight, double _minimum1080, double _minimum2160) noexcept;

// UI-171 as O92 amends it (ADR-147): the final size of a text of layout size _layoutPixels at the text scale _textScale
// and the DPI scale _dpiScale (UI-170), never below _minimumPixels; DirectWrite rasterizes it at that size.
[[nodiscard]] float FinalTextPixels(double _layoutPixels, double _textScale, double _dpiScale, double _minimumPixels) noexcept;

/// How a text is drawn (UI-189 to UI-192, ADR-145): its fill pass's blend and color and, when it has one, its outline
/// pass's color, drawn first in the ink state at the outline's offset in whole pixels.
struct TextStyle
{
  UiBlend blend = UiBlend::Additive; // UI-190: normal text is additive
  std::array<float, 4> color{1.0f, 1.0f, 1.0f, 1.0f};
  std::optional<std::array<float, 4>> outline;
  std::int32_t outlineOffsetPixels = 1;
};

// UI-179: appends _quad to _batches, in the last batch when it has the same source, blend and image or glyph page and no
// scissor, and in a new batch otherwise, so the order is never changed.
void AppendQuad(std::vector<UiBatch>& _batches, UiSource _source, UiBlend _blend, std::uint16_t _image, const UiQuad& _quad);

// Appends _line with its top-left corner at (_left, _top), which are rounded to whole pixels: when the style has an
// outline, its pass of eight copies at the outline's offset to the neighboring positions in the ink state (UI-192),
// then the fill pass.
void AppendText(std::vector<UiBatch>& _batches, const TextLine& _line, float _left, float _top, const TextStyle& _style);

// DBG-095, DBG-096 (ADR-147): _lines of the development overlay in the development role at _sizePixels × _dpiScale,
// alpha-blended in their colors, each anchored by its corner to the target of _size at its insets × _dpiScale.
void AppendOverlayLines(std::vector<UiBatch>& _batches, TextSystem& _text, std::span<const OverlayLine> _lines, SizePx _size,
                        float _sizePixels, float _dpiScale);

} // namespace Neuron
