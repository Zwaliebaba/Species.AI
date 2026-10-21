#pragma once

#include "FrameView.h"
#include "ReplicaTiming.h"
#include "StableId.h"
#include "TuningPresentation.h"
#include "Vec3.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

/// What the world markers read of a frame (ADR-125): the camera's view, the overlay's target in physical pixels, the
/// pointer on it, and the real frame step.
struct MarkerView
{
  Neuron::FrameView frame;
  Neuron::Vec3 camera;
  Neuron::Vec3 cameraFront{0.0, 0.0, -1.0}; // a unit vector
  Neuron::SizePx size;
  Neuron::PointPx pointer;
  double realStepSeconds = 0.0;
};

/// The world markers of M1 (M1 task T17b, ADR-125; R-CTRL-380 to R-CTRL-388, RND-356, RND-357): the Leader markers, one
/// for each of the local team's living Leaders, made when the Leader is first seen and removed with it, the only type
/// whose source M1 has. Each tracks its Leader lifted by ref.ctrl.marker.lift, smoothed (R-CTRL-382), and counts its time
/// on screen. On screen its icon is the Leader's banner, which pass 7b draws (R-CTRL-388 as amended); off screen, until
/// it has been seen (R-CTRL-384), it is held at the screen's edge (R-CTRL-383) and drawn in pass 16 as the shadow, the
/// compass pointing towards its Leader, the team disc and icon.power.leader in place of the banner (R-CTRL-386). A
/// marker whose icon box holds the pointer is the marker under the pointer, which selects as the Leader does
/// (R-CTRL-387). Frame thread only.
class WorldMarkers
{
public:
  // _presentation outlives the markers.
  explicit WorldMarkers(const TuningPresentation& _presentation) noexcept;

  // Once per frame: _figures' Leaders of _team (none: no markers), _selected the team's selected Leader.
  void Update(std::span<const InterpolatedFigure> _figures, std::optional<std::uint8_t> _team, std::optional<StableId> _selected,
              const MarkerView& _view);
  // A new world: no markers (R-CTRL-380).
  void Clear() noexcept;

  // Pass 16's quads, nearest marker last (R-CTRL-382).
  [[nodiscard]] const std::vector<Neuron::OverlayQuad>& Quads() const noexcept
  {
    return m_quads;
  }
  // R-CTRL-387: the Leader of the marker under the pointer at the end of the frame.
  [[nodiscard]] std::optional<StableId> UnderPointer() const noexcept
  {
    return m_underPointer;
  }
  [[nodiscard]] std::size_t Count() const noexcept
  {
    return m_markers.size();
  }

private:
  struct Marker
  {
    StableId leader = 0;
    std::uint8_t team = 0;
    Neuron::Vec3 position;
    bool placed = false; // its position has been set once, so it smooths from there
    double ageSeconds = 0.0;
    double onScreenSeconds = 0.0;
    bool tracked = false; // its Leader was among this frame's figures
  };

  void Lay(const Marker& _marker, bool _selected, const MarkerView& _view);

  const TuningPresentation* m_presentation;
  std::vector<Marker> m_markers; // in drawing order
  std::vector<Neuron::OverlayQuad> m_quads;
  std::optional<StableId> m_underPointer;
};

// R-CTRL-366 rows 2 and 3 (ADR-125): the badge near the pointer while a Leader or Workers are selected, icon.power.leader
// or icon.power.workers on the team's disc over its shadow, the icon's centre down and to the right of the pointer;
// none while nothing is selected.
[[nodiscard]] std::vector<Neuron::OverlayQuad> CursorBadge(bool _leaderSelected, bool _workersSelected, std::uint8_t _team,
                                                           Neuron::PointPx _pointer, Neuron::SizePx _size,
                                                           const TuningPresentation& _presentation);

// R-CTRL-366's count (ADR-127): the selected Workers' count in white, its cell's left edge at the right edge of the
// badge's shadow, its first layer, and its middle level with the badge's centre; none for no badge or no Worker.
[[nodiscard]] std::optional<Neuron::OverlayLine> BadgeCount(std::span<const Neuron::OverlayQuad> _badge, std::size_t _workers);

} // namespace Species
