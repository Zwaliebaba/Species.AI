#include "pch.h"

#include "NineSlice.h"

#include "UiText.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace Neuron
{

namespace
{

// The edges of a box's three columns, or rows, on whole pixels: its start, the end of its first slice, the start of its
// last slice, and its end.
std::array<float, 4> Edges(float _start, float _end, float _first, float _last, float _scale) noexcept
{
  const float first = std::min(std::round(_start + _first * _scale), _end);
  const float last = std::max(std::round(_end - _last * _scale), first);
  return {_start, first, last, _end};
}

} // namespace

float NineSliceScale(const NineSlice& _slice, const UiRect& _box, float _texelPixels) noexcept
{
  float scale = _texelPixels;
  const float across = _slice.slices[0] + _slice.slices[2];
  const float down = _slice.slices[1] + _slice.slices[3];
  if (across > 0.0f)
  {
    scale = std::min(scale, _box.Width() / across);
  }
  if (down > 0.0f)
  {
    scale = std::min(scale, _box.Height() / down);
  }
  return std::max(scale, 0.0f);
}

UiRect NineSliceInner(const NineSlice& _slice, const UiRect& _box, float _scale) noexcept
{
  const std::array<float, 4> x = Edges(_box.left, _box.right, _slice.slices[0], _slice.slices[2], _scale);
  const std::array<float, 4> y = Edges(_box.top, _box.bottom, _slice.slices[1], _slice.slices[3], _scale);
  return {x[1], y[1], x[2], y[2]};
}

void AppendNineSlice(std::vector<UiBatch>& _batches, std::uint16_t _image, const NineSlice& _slice, const UiRect& _box, float _scale,
                     const std::array<float, 4>& _color)
{
  const auto quad = [&](float _left, float _top, float _right, float _bottom, const std::array<float, 4>& _uv)
  {
    if (_right <= _left || _bottom <= _top)
    {
      return;
    }
    UiQuad drawn;
    drawn.rectangle = {_left, _top, _right, _bottom};
    drawn.uv = _uv;
    drawn.color = _color;
    AppendQuad(_batches, UiSource::Image, UiBlend::Alpha, _image, drawn);
  };
  if (_slice.image.width == 0 || _slice.image.height == 0 || _scale <= 0.0f)
  {
    quad(_box.left, _box.top, _box.right, _box.bottom, {0.0f, 0.0f, 1.0f, 1.0f});
    return;
  }
  const auto width = static_cast<float>(_slice.image.width);
  const auto height = static_cast<float>(_slice.image.height);
  const std::array<float, 4> x = Edges(_box.left, _box.right, _slice.slices[0], _slice.slices[2], _scale);
  const std::array<float, 4> y = Edges(_box.top, _box.bottom, _slice.slices[1], _slice.slices[3], _scale);
  const std::array<float, 4> u{0.0f, _slice.slices[0] / width, 1.0f - _slice.slices[2] / width, 1.0f};
  const float vTop = _slice.slices[1] / height;
  const float vBottom = 1.0f - _slice.slices[3] / height;
  const auto row = [&](float _top, float _bottom, float _v0, float _v1)
  {
    for (std::size_t column = 0; column + 1 < x.size(); ++column)
    {
      quad(x[column], _top, x[column + 1], _bottom, {u[column], _v0, u[column + 1], _v1});
    }
  };
  row(y[0], y[1], 0.0f, vTop);
  // The middle rows at the texels' scale, cut at the box's middle and repeated down a box taller than the image holds;
  // stretched once when they are less than a pixel tall.
  const float middlePixels = (vBottom - vTop) * height * _scale;
  if (middlePixels < 1.0f)
  {
    row(y[1], y[2], vTop, vBottom);
  }
  else
  {
    for (float top = y[1]; top < y[2];)
    {
      const float bottom = std::min(y[2], std::max(top + 1.0f, std::round(top + middlePixels)));
      row(top, bottom, vTop, vTop + (bottom - top) / (height * _scale));
      top = bottom;
    }
  }
  row(y[2], y[3], vBottom, 1.0f);
}

} // namespace Neuron
