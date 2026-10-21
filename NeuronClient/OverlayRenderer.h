#pragma once

#include "DeviceGpuResourceFactory.h"
#include "FrameView.h"
#include "PipelineCatalog.h"
#include "UploadRing.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace Neuron
{

/// The development overlay's drawing (DBG-004, DBG-095; M1 tasks T15a and T15b): its face's page on the GPU, its
/// rectangles and lines in pass 29 over the final image, and the development dots in the scene. A second one draws the
/// cursor's badge count in GameFont in pass 20 (ADR-127). Every call is made from the frame thread.
class OverlayRenderer
{
public:
  // _pageName names the face's page in the allocation record and PIX: an asset id that outlives the renderer.
  explicit OverlayRenderer(DeviceGpuResourceFactory& _factory, std::string_view _pageName = "font.dev.editor") noexcept;
  ~OverlayRenderer();

  OverlayRenderer(const OverlayRenderer&) = delete;
  OverlayRenderer& operator=(const OverlayRenderer&) = delete;

  // Before the factory's frame: uploads _font's page when it is not the face held.
  void Show(const std::shared_ptr<const FontAsset>& _font);
  // After a device rebuild: the page again, into the resource the factory kept (RND-035).
  void UploadAgain();
  [[nodiscard]] bool Ready() const;

  // Pass 29: _rects, then _lines over them, over the target of _size, whose render target, viewport and scissor are set,
  // at _scale (O63).
  void RecordText(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
                  std::uint32_t _scale, std::span<const OverlayLine> _lines, std::span<const OverlayRect> _rects);
  // The dots in the scene, depth-tested; the frame constants are bound.
  void RecordDots(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, const FrameView& _frame,
                  std::uint32_t _scale, std::span<const DevelopmentDot> _dots);

private:
  void Release();
  void Schedule();

  DeviceGpuResourceFactory& m_factory;
  std::string_view m_pageName;
  std::shared_ptr<const FontAsset> m_font;
  GpuResourceId m_page;
  std::uint64_t m_fence = 0;
  bool m_pending = false;
};

} // namespace Neuron
