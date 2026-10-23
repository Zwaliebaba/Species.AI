#pragma once

#include "UiLayout.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <vector>

namespace Neuron
{

/// A framed image drawn as nine slices (ADR-175): the image's size and the widths, in texels, of its left, top, right
/// and bottom slices. Drawn on a box, the corners keep their texels' size; the top and bottom edges and the middle
/// column stretch across; the left and right edges and the middle column are cut down the box at the texels' scale, the
/// middle rows repeating when the box is taller than the image holds.
struct NineSlice
{
  SizePx image;                  // texels; none: the image is drawn whole over the box
  std::array<float, 4> slices{}; // left, top, right and bottom, in texels
};

// The scale, in pixels per texel, at which _slice draws on _box: _texelPixels, or less where the box is too small to
// hold the slices whole.
[[nodiscard]] float NineSliceScale(const NineSlice& _slice, const UiRect& _box, float _texelPixels) noexcept;

// The part of _box inside _slice's slices drawn at _scale pixels per texel, its edges on whole pixels.
[[nodiscard]] UiRect NineSliceInner(const NineSlice& _slice, const UiRect& _box, float _scale) noexcept;

// Appends the quads of image _image drawn as _slice on _box at _scale pixels per texel, colored _color and alpha
// blended (UI-054 as ADR-175 amends it).
void AppendNineSlice(std::vector<UiBatch>& _batches, std::uint16_t _image, const NineSlice& _slice, const UiRect& _box, float _scale,
                     const std::array<float, 4>& _color);

} // namespace Neuron
