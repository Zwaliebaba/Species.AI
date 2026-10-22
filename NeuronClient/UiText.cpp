#include "pch.h"

#include "TextLayout.h"
#include "UiText.h"

#include <algorithm>
#include <cmath>

namespace Neuron
{

namespace
{

constexpr double HEIGHT_1080 = 1080.0;
constexpr double HEIGHT_2160 = 2160.0;

// UI-192: the eight neighbouring positions of the outline pass.
constexpr std::array<std::array<std::int32_t, 2>, 8> OUTLINE_POSITIONS{
  {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, -1}, {1, -1}, {-1, 1}, {1, 1}}};

void AppendGlyphs(std::vector<UiBatch>& _batches, const std::vector<GlyphQuad>& _glyphs, UiBlend _blend, const std::array<float, 4>& _color,
                  float _dx, float _dy)
{
  for (const GlyphQuad& glyph : _glyphs)
  {
    UiQuad quad;
    quad.rectangle = {glyph.rectangle[0] + _dx, glyph.rectangle[1] + _dy, glyph.rectangle[2] + _dx, glyph.rectangle[3] + _dy};
    quad.uv = glyph.uv;
    quad.color = _color;
    AppendQuad(_batches, UiSource::Glyphs, _blend, 0, quad);
  }
}

} // namespace

double MinimumTextPixels(std::uint32_t _clientHeight, double _minimum1080, double _minimum2160) noexcept
{
  const auto height = static_cast<double>(_clientHeight);
  if (height <= HEIGHT_1080)
  {
    return _minimum1080 * height / HEIGHT_1080;
  }
  if (height <= HEIGHT_2160)
  {
    return _minimum1080 + (_minimum2160 - _minimum1080) * (height - HEIGHT_1080) / (HEIGHT_2160 - HEIGHT_1080);
  }
  return _minimum2160 * height / HEIGHT_2160;
}

std::uint32_t TextScaleForSize(double _sizePixels, double _designPixels, double _minimumPixels) noexcept
{
  if (_designPixels <= 0.0)
  {
    return 1;
  }
  auto scale = static_cast<std::uint32_t>(std::max(1.0, std::floor(_sizePixels / _designPixels + 0.5)));
  // A scale that no minimum could need bounds the raise against a minimum that is not a number.
  constexpr std::uint32_t SCALE_MAX = 64;
  while (static_cast<double>(scale) * _designPixels < _minimumPixels && scale < SCALE_MAX)
  {
    ++scale;
  }
  return scale;
}

void AppendQuad(std::vector<UiBatch>& _batches, UiSource _source, UiBlend _blend, std::uint16_t _image, const UiQuad& _quad)
{
  if (_batches.empty() || _batches.back().source != _source || _batches.back().blend != _blend ||
      (_source == UiSource::Image && _batches.back().image != _image) || _batches.back().scissor.has_value())
  {
    UiBatch batch;
    batch.source = _source;
    batch.blend = _blend;
    batch.image = _source == UiSource::Image ? _image : std::uint16_t{0};
    _batches.push_back(std::move(batch));
  }
  _batches.back().quads.push_back(_quad);
}

void AppendText(std::vector<UiBatch>& _batches, const FontAsset& _face, std::string_view _utf8, float _left, float _top,
                const TextStyle& _style)
{
  const std::vector<GlyphQuad> glyphs = LayoutText(_face, _utf8, std::max(_style.scale, 1u), std::round(_left), std::round(_top));
  if (glyphs.empty())
  {
    return;
  }
  if (_style.outline.has_value())
  {
    for (const auto& [x, y] : OUTLINE_POSITIONS)
    {
      AppendGlyphs(_batches, glyphs, UiBlend::Ink, *_style.outline, static_cast<float>(x * _style.outlineOffsetPixels),
                   static_cast<float>(y * _style.outlineOffsetPixels));
    }
  }
  AppendGlyphs(_batches, glyphs, _style.blend, _style.color, 0.0f, 0.0f);
}

} // namespace Neuron
