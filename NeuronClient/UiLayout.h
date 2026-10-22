#pragma once

#include "WindowGeometry.h"

#include <array>
#include <cstdint>

namespace Neuron
{

/// A rectangle of the client area in pixels, from its top-left corner (left, top) to its bottom-right one (right,
/// bottom). Hit tests take both edges as inside (UI-026).
struct UiRect
{
  float left = 0.0f;
  float top = 0.0f;
  float right = 0.0f;
  float bottom = 0.0f;

  [[nodiscard]] static UiRect FromSize(float _x, float _y, float _width, float _height) noexcept
  {
    return {_x, _y, _x + _width, _y + _height};
  }
  [[nodiscard]] float Width() const noexcept
  {
    return right - left;
  }
  [[nodiscard]] float Height() const noexcept
  {
    return bottom - top;
  }
  [[nodiscard]] bool Contains(float _x, float _y) const noexcept
  {
    return _x >= left && _x <= right && _y >= top && _y <= bottom;
  }

  bool operator==(const UiRect& _other) const = default;
};

/// The page grid's keys (UI-163; reference/09 R-META-200, R-META-201), each a fraction of the quantity its key names.
struct PageGridKeys
{
  double horizontalSplit = 0.26; // ref.meta.layout.horizontal_split, × H
  double verticalSplit = 0.6;    // ref.meta.layout.vertical_split, × W
  double gap = 0.05;             // ref.meta.layout.gap, × the vertical split
  double widescreenAspect = 1.4; // ref.meta.layout.widescreen_aspect, W ÷ H above which the screen is wide
  double widescreenInset = 0.08; // ref.meta.layout.widescreen_inset, × W
  double fontLarge = 0.095;      // ref.meta.layout.font_large, × H
  double fontMedium = 0.04;      // ref.meta.layout.font_medium, × H
  double fontSmall = 0.022;      // ref.meta.layout.font_small, × H
  double buttonWidth = 0.9;      // ref.meta.layout.button_width, × the left box's width
  double buttonHeight = 1.3;     // ref.meta.layout.button_height, × the medium font
  double buttonGap = 0.1;        // ref.meta.layout.button_gap, × the medium font
};

/// The page grid of a client area (UI-163): the title bar, the left and right boxes, the three font sizes and the
/// standard button, in pixels before snapping.
struct PageGrid
{
  UiRect titleBar;
  UiRect leftBox;
  UiRect rightBox;
  float fontLarge = 0.0f;
  float fontMedium = 0.0f;
  float fontSmall = 0.0f;
  float buttonWidth = 0.0f;
  float buttonHeight = 0.0f;
  float buttonGap = 0.0f;
  bool wide = false;

  // UI-164 item 1: the left edge of the button column, centered in the left box.
  [[nodiscard]] float ButtonColumnLeft() const noexcept
  {
    return leftBox.left + (leftBox.Width() - buttonWidth) * 0.5f;
  }
  // UI-164 item 3: one button step, a button and its gap.
  [[nodiscard]] float ButtonStep() const noexcept
  {
    return buttonHeight + buttonGap;
  }
};

// UI-163: `ComputePageGrid(W, H)` from its keys.
[[nodiscard]] PageGrid ComputePageGrid(SizePx _client, const PageGridKeys& _keys) noexcept;

/// The figures that convert every layout of a frame to pixels (ADR-033, UI-168 to UI-171).
struct UiMetrics
{
  SizePx client;
  float dpiScale = 1.0f;          // the window's (ADR-075)
  float uiScale = 1.0f;           // interface.ui_scale (UI-168)
  float textScale = 1.0f;         // interface.text_scale (UI-168)
  double minimumTextPixels = 0.0; // UI-171's minimum for the client height
  double hudCanvasHeight = 600.0; // ref.ctrl.panel.canvas_height: the HUD space's height in units (RND-377)

  bool operator==(const UiMetrics& _other) const = default;

  // RND-377: the pixels of one HUD unit; the HUD space is hudCanvasHeight units high, independent of the render size.
  [[nodiscard]] float HudUnitPixels() const noexcept;
  // UI-171: the final size of a text of layout size _layoutPixels, × the text scale (and the UI scale where the element
  // scales), never below the minimum. DirectWrite rasterizes it at that size (ADR-147).
  [[nodiscard]] float TextPixels(double _layoutPixels, bool _scalesWithUi = true) const noexcept;
  // UI-169: a line width, border, focus outline or text-outline offset of _pixels × the DPI scale, rounded to whole
  // pixels and at least 1 px.
  [[nodiscard]] float LinePixels(double _pixels) const noexcept;
};

// UI-168, UI-170: _rect scaled by _scale about the point (_anchorX, _anchorY), so that the point stays where it is.
[[nodiscard]] UiRect ScaleAbout(const UiRect& _rect, float _anchorX, float _anchorY, float _scale) noexcept;

// UI-169: _rect snapped to whole pixels, each edge to its nearest.
[[nodiscard]] UiRect SnapRect(const UiRect& _rect) noexcept;

// RND-377: a rectangle of HUD units (x, y from the top-left corner, w, h) in pixels.
[[nodiscard]] UiRect HudRect(float _x, float _y, float _width, float _height, const UiMetrics& _metrics) noexcept;

// UI-165: the top of a line _lineHeight pixels tall that is centered vertically in the row from _rowTop, _rowHeight
// pixels tall, on a whole pixel.
[[nodiscard]] float RowTextTop(float _rowTop, float _rowHeight, float _lineHeight) noexcept;

} // namespace Neuron
