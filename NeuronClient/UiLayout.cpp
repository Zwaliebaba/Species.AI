#include "pch.h"

#include "UiLayout.h"

#include <algorithm>
#include <cmath>

namespace Neuron
{

PageGrid ComputePageGrid(SizePx _client, const PageGridKeys& _keys) noexcept
{
  const auto width = static_cast<double>(_client.width);
  const auto height = static_cast<double>(_client.height);
  const double horizontalSplit = _keys.horizontalSplit * height;
  const double verticalSplit = _keys.verticalSplit * width;
  const double gap = _keys.gap * verticalSplit;
  double left = gap;
  const double top = gap;
  double right = width - gap;
  const double bottom = height - gap;
  PageGrid grid;
  grid.wide = height > 0.0 && width / height > _keys.widescreenAspect;
  if (grid.wide)
  {
    left += _keys.widescreenInset * width;
    right -= _keys.widescreenInset * width;
  }
  const auto rect = [](double _x, double _y, double _w, double _h)
  { return UiRect::FromSize(static_cast<float>(_x), static_cast<float>(_y), static_cast<float>(_w), static_cast<float>(_h)); };
  grid.titleBar = rect(left, top, right - left, horizontalSplit - gap * 0.5 - top);
  const double leftY = horizontalSplit + gap * 0.5;
  grid.leftBox = rect(left, leftY, verticalSplit - gap * 0.5 - left, bottom - leftY);
  const double rightX = verticalSplit + gap * 0.5;
  grid.rightBox = rect(rightX, leftY, right - rightX, bottom - leftY);
  grid.fontLarge = static_cast<float>(_keys.fontLarge * height);
  grid.fontMedium = static_cast<float>(_keys.fontMedium * height);
  grid.fontSmall = static_cast<float>(_keys.fontSmall * height);
  grid.buttonWidth = static_cast<float>(_keys.buttonWidth * static_cast<double>(grid.leftBox.Width()));
  grid.buttonHeight = static_cast<float>(_keys.buttonHeight * static_cast<double>(grid.fontMedium));
  grid.buttonGap = static_cast<float>(_keys.buttonGap * static_cast<double>(grid.fontMedium));
  return grid;
}

float UiMetrics::HudUnitPixels() const noexcept
{
  return hudCanvasHeight > 0.0 ? static_cast<float>(static_cast<double>(client.height) / hudCanvasHeight) : 0.0f;
}

float UiMetrics::TextPixels(double _layoutPixels, bool _scalesWithUi) const noexcept
{
  const double scaled = _layoutPixels * static_cast<double>(textScale) * (_scalesWithUi ? static_cast<double>(uiScale) : 1.0);
  return static_cast<float>(std::max(scaled, minimumTextPixels));
}

float UiMetrics::LinePixels(double _pixels) const noexcept
{
  return static_cast<float>(std::max(std::round(_pixels * static_cast<double>(dpiScale)), 1.0));
}

UiRect ScaleAbout(const UiRect& _rect, float _anchorX, float _anchorY, float _scale) noexcept
{
  return {_anchorX + (_rect.left - _anchorX) * _scale, _anchorY + (_rect.top - _anchorY) * _scale,
          _anchorX + (_rect.right - _anchorX) * _scale, _anchorY + (_rect.bottom - _anchorY) * _scale};
}

UiRect SnapRect(const UiRect& _rect) noexcept
{
  return {std::round(_rect.left), std::round(_rect.top), std::round(_rect.right), std::round(_rect.bottom)};
}

UiRect HudRect(float _x, float _y, float _width, float _height, const UiMetrics& _metrics) noexcept
{
  const float unit = _metrics.HudUnitPixels();
  return UiRect::FromSize(_x * unit, _y * unit, _width * unit, _height * unit);
}

float RowTextTop(float _rowTop, float _rowHeight, float _lineHeight) noexcept
{
  return std::round(_rowTop + (_rowHeight - _lineHeight) * 0.5f);
}

} // namespace Neuron
