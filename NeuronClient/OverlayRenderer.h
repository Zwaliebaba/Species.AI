#pragma once

#include "FrameView.h"
#include "PipelineCatalog.h"
#include "UploadRing.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <span>

namespace Neuron
{

/// The development overlay's drawing (DBG-004, DBG-095; M1 tasks T15a and T15b): its rectangles in pass 29 over the
/// final image, under its text, which the UI renderer draws (ADR-147), and the development dots in the scene. It holds
/// no GPU resource. Every call is made from the frame thread.
class OverlayRenderer
{
public:
  // Pass 29: _rects over the target of _size, whose render target, viewport and scissor are set, at the DPI scale
  // _scale, on whole pixels.
  void RecordRects(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size, float _scale,
                   std::span<const OverlayRect> _rects);
  // The dots in the scene, depth-tested, their sides at the DPI scale _scale; the frame constants are bound.
  void RecordDots(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, const FrameView& _frame,
                  float _scale, std::span<const DevelopmentDot> _dots);
};

} // namespace Neuron
