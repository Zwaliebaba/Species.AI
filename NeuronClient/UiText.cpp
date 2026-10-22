#include "pch.h"

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

void AppendGlyphs(std::vector<UiBatch>& _batches, const TextLine& _line, UiBlend _blend, const std::array<float, 4>& _color, float _dx,
                  float _dy)
{
  for (const PlacedGlyph& glyph : _line.glyphs)
  {
    UiQuad quad;
    quad.rectangle = {glyph.rectangle[0] + _dx, glyph.rectangle[1] + _dy, glyph.rectangle[2] + _dx, glyph.rectangle[3] + _dy};
    quad.uv = glyph.uv;
    quad.color = _color;
    AppendQuad(_batches, UiSource::Glyphs, _blend, glyph.page, quad);
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

float FinalTextPixels(double _layoutPixels, double _textScale, double _dpiScale, double _minimumPixels) noexcept
{
  const double scaled = _layoutPixels * _textScale * _dpiScale;
  // A minimum that is not a number leaves the size as it is.
  return static_cast<float>(scaled < _minimumPixels ? _minimumPixels : scaled);
}

void AppendQuad(std::vector<UiBatch>& _batches, UiSource _source, UiBlend _blend, std::uint16_t _image, const UiQuad& _quad)
{
  if (_batches.empty() || _batches.back().source != _source || _batches.back().blend != _blend ||
      (_source != UiSource::Solid && _batches.back().image != _image) || _batches.back().scissor.has_value())
  {
    UiBatch batch;
    batch.source = _source;
    batch.blend = _blend;
    batch.image = _source == UiSource::Solid ? std::uint16_t{0} : _image;
    _batches.push_back(std::move(batch));
  }
  _batches.back().quads.push_back(_quad);
}

void AppendText(std::vector<UiBatch>& _batches, const TextLine& _line, float _left, float _top, const TextStyle& _style)
{
  if (_line.glyphs.empty())
  {
    return;
  }
  const float left = std::round(_left);
  const float top = std::round(_top);
  if (_style.outline.has_value())
  {
    for (const auto& [x, y] : OUTLINE_POSITIONS)
    {
      AppendGlyphs(_batches, _line, UiBlend::Ink, *_style.outline, left + static_cast<float>(x * _style.outlineOffsetPixels),
                   top + static_cast<float>(y * _style.outlineOffsetPixels));
    }
  }
  AppendGlyphs(_batches, _line, _style.blend, _style.color, left, top);
}

void AppendOverlayLines(std::vector<UiBatch>& _batches, TextSystem& _text, std::span<const OverlayLine> _lines, SizePx _size,
                        float _sizePixels, float _dpiScale)
{
  const auto width = static_cast<float>(_size.width);
  const auto height = static_cast<float>(_size.height);
  for (const OverlayLine& line : _lines)
  {
    const TextLine laid = _text.Layout(line.text, FontRole::Development, _sizePixels * _dpiScale);
    // DBG-095: a bottom-right line is right-aligned; DBG-096: a page's line starts at its top-left corner. The anchor's
    // distances scale with the text.
    float left = std::round(line.insetX * _dpiScale);
    float top = std::round(line.insetY * _dpiScale);
    if (line.corner == OverlayCorner::BottomRight)
    {
      left = width - left - std::ceil(laid.widthPixels);
      top = height - top - std::ceil(laid.heightPixels);
    }
    TextStyle style;
    style.blend = UiBlend::Alpha;
    style.color = line.color;
    AppendText(_batches, laid, left, top, style);
  }
}

} // namespace Neuron
