#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "FigureKind.h"
#include "Picking.h"
#include "WorldMarkers.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iterator>
#include <limits>
#include <string>
#include <utility>

namespace Species
{

namespace
{

constexpr Neuron::Vec3 UP{0.0, 1.0, 0.0};
constexpr std::array<float, 4> WHITE{1.0f, 1.0f, 1.0f, 1.0f};
// R-CTRL-366: the badge lies along Normalize(0.85, 1) from the pointer, down and to the right.
constexpr double BADGE_DIRECTION_X = 0.85;
constexpr double BADGE_DIRECTION_Y = 1.0;
// The shortest ray from the screen's centre that finds an edge point, px.
constexpr double EDGE_RAY_MIN = 1e-6;

Neuron::OverlayQuad QuadOf(SceneTexture _texture, Neuron::OverlayBlend _blend, double _x, double _y, double _side, double _rotation,
                           const std::array<float, 4>& _color) noexcept
{
  return {static_cast<std::uint16_t>(_texture), _blend, static_cast<float>(_x), static_cast<float>(_y), static_cast<float>(_side * 0.5),
          static_cast<float>(_rotation),        _color};
}

std::array<float, 4> Shadow(double _alpha) noexcept
{
  return {0.0f, 0.0f, 0.0f, static_cast<float>(_alpha)};
}

} // namespace

WorldMarkers::WorldMarkers(const TuningPresentation& _presentation) noexcept
  : m_presentation(&_presentation)
{
}

void WorldMarkers::Clear() noexcept
{
  m_markers.clear();
  m_quads.clear();
  m_underPointer.reset();
}

void WorldMarkers::Update(std::span<const InterpolatedFigure> _figures, std::optional<std::uint8_t> _team,
                          std::optional<StableId> _selected, const MarkerView& _view)
{
  m_quads.clear();
  m_underPointer.reset();
  if (!_team.has_value())
  {
    m_markers.clear();
    return;
  }
  const auto& marker = m_presentation->ref.ctrl.marker;
  const double dt = std::max(_view.realStepSeconds, 0.0);
  for (Marker& kept : m_markers)
  {
    kept.tracked = false;
  }
  // R-CTRL-381: a Leader marker for each living Leader of the local team, made when it is first seen.
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.kind != FigureKind::Leader || figure.state.team != *_team || figure.state.dead)
    {
      continue;
    }
    auto found = std::ranges::find(m_markers, figure.id, &Marker::leader);
    if (found == m_markers.end())
    {
      m_markers.push_back({figure.id, figure.state.team, {}, false, 0.0, 0.0, false});
      found = std::prev(m_markers.end());
    }
    // R-CTRL-382 step 2: the tracked position, smoothed, or taken at once by a new marker.
    const Neuron::Vec3 target = figure.position + UP * marker.lift;
    const double k = std::min(marker.smoothing * dt, 1.0);
    found->position = found->placed ? found->position * (1.0 - k) + target * k : target;
    found->placed = true;
    found->ageSeconds += dt;
    found->tracked = true;
  }
  std::erase_if(m_markers, [](const Marker& _marker) { return !_marker.tracked; });
  // R-CTRL-382 step 1: one bubble-sort pass, farthest first, so the nearest is drawn last; a marker younger than the new
  // age takes its age as its key, so the newest are drawn over the rest. Every marker is the local team's.
  const auto key = [&](const Marker& _marker)
  {
    return _marker.ageSeconds < marker.newAge ? _marker.ageSeconds
                                              : Neuron::LengthSquared(_marker.position - _view.camera) - marker.ownSortOffset;
  };
  for (std::size_t n = 0; n + 1 < m_markers.size(); ++n)
  {
    if (key(m_markers[n]) < key(m_markers[n + 1]))
    {
      std::swap(m_markers[n], m_markers[n + 1]);
    }
  }
  for (Marker& kept : m_markers)
  {
    const std::optional<Neuron::ScreenPoint> screen = Neuron::ProjectToScreen(kept.position, _view.frame, _view.size);
    const bool onScreen = screen.has_value() && screen->x >= 0.0 && screen->y >= 0.0 &&
                          screen->x <= static_cast<double>(_view.size.width) && screen->y <= static_cast<double>(_view.size.height);
    if (onScreen)
    {
      kept.onScreenSeconds += dt; // R-CTRL-384 rule 1
    }
    else
    {
      Lay(kept, kept.leader == _selected, _view);
    }
  }
}

void WorldMarkers::Lay(const Marker& _marker, bool _selected, const MarkerView& _view)
{
  const auto& marker = m_presentation->ref.ctrl.marker;
  // R-CTRL-384 rule 5: a Leader marker shows off screen only until it has been seen.
  if (_marker.onScreenSeconds >= marker.alpha.seenAfterS)
  {
    return;
  }
  // R-CTRL-383: the probe point from ahead of the camera towards the marker, and the ray from the screen's centre
  // through its projection to the screen's edge; the icon lies inset from there towards the centre.
  const Neuron::Vec3 ahead = _view.camera + _view.cameraFront * marker.edgeProbeAhead;
  const Neuron::Vec3 probe = ahead + Neuron::NormalizedOr(_marker.position - ahead, _view.cameraFront) * marker.edgeProbe;
  const std::optional<Neuron::ScreenPoint> projected = Neuron::ProjectToScreen(probe, _view.frame, _view.size);
  if (!projected.has_value())
  {
    return;
  }
  const double width = static_cast<double>(_view.size.width);
  const double height = static_cast<double>(_view.size.height);
  const double centerX = width * 0.5;
  const double centerY = height * 0.5;
  const double rayX = projected->x - centerX;
  const double rayY = projected->y - centerY;
  const double ray = std::hypot(rayX, rayY);
  if (ray < EDGE_RAY_MIN)
  {
    return;
  }
  double reach = std::numeric_limits<double>::infinity();
  if (rayX != 0.0)
  {
    reach = std::min(reach, ((rayX > 0.0 ? width : 0.0) - centerX) / rayX);
  }
  if (rayY != 0.0)
  {
    reach = std::min(reach, ((rayY > 0.0 ? height : 0.0) - centerY) / rayY);
  }
  const double edgeX = centerX + rayX * reach;
  const double edgeY = centerY + rayY * reach;
  const double inwardX = -rayX / ray;
  const double inwardY = -rayY / ray;
  const double iconX = edgeX + inwardX * marker.edgeInset;
  const double iconY = edgeY + inwardY * marker.edgeInset;
  // R-CTRL-385: the sizes from the target's height, larger while the camera is near, and while hovered or selected.
  const auto& size = marker.size;
  double scale = 1.0;
  const double distance = Neuron::Length(_marker.position - _view.camera);
  if (distance < size.nearDistance)
  {
    scale *= 2.0 - distance / size.nearDistance;
  }
  double icon = height / size.iconDivisor * scale;
  const bool hovered = std::abs(static_cast<double>(_view.pointer.x) - iconX) <= icon * 0.5 &&
                       std::abs(static_cast<double>(_view.pointer.y) - iconY) <= icon * 0.5;
  if (hovered || _selected)
  {
    scale *= size.hoverScale;
    icon = height / size.iconDivisor * scale;
  }
  if (hovered)
  {
    m_underPointer = _marker.leader; // the last drawn, on top, wins
  }
  const double shadow = icon + size.shadowPad;
  const double compass = icon + height / size.compassDivisor * scale;
  // The compass's arrow points up in its art; it is turned to point out from the centre, towards the Leader.
  const double turn = std::atan2(-inwardX, inwardY);
  const std::array<float, 4> team = TeamColor(_marker.team);
  m_quads.push_back(
    QuadOf(SceneTexture::TeamDiscShadow, Neuron::OverlayBlend::Alpha, iconX, iconY, shadow, 0.0, Shadow(marker.shadowAlpha)));
  m_quads.push_back(
    QuadOf(SceneTexture::EdgeCompassShadow, Neuron::OverlayBlend::Alpha, iconX, iconY, compass, turn, Shadow(marker.shadowAlpha)));
  m_quads.push_back(QuadOf(SceneTexture::EdgeCompass, Neuron::OverlayBlend::Additive, iconX, iconY, compass, turn, team));
  m_quads.push_back(QuadOf(SceneTexture::TeamDisc, Neuron::OverlayBlend::Additive, iconX, iconY, icon, 0.0, team));
  m_quads.push_back(QuadOf(SceneTexture::IconLeader, Neuron::OverlayBlend::Additive, iconX, iconY, icon, 0.0, WHITE));
}

std::vector<Neuron::OverlayQuad> CursorBadge(bool _leaderSelected, bool _workersSelected, std::uint8_t _team, Neuron::PointPx _pointer,
                                             Neuron::SizePx _size, const TuningPresentation& _presentation)
{
  std::vector<Neuron::OverlayQuad> quads;
  if (!_leaderSelected && !_workersSelected)
  {
    return quads;
  }
  // R-CTRL-366: the icon's centre lies the offset and the centre offset along n from the pointer; its size is the
  // target's height ÷ the divisor, drawn in the world markers' layers (R-CTRL-386).
  const auto& cursor = _presentation.ref.ctrl.cursor;
  const double length = std::hypot(BADGE_DIRECTION_X, BADGE_DIRECTION_Y);
  const double along = cursor.iconOffset + cursor.iconCenterOffset;
  const double x = static_cast<double>(_pointer.x) + BADGE_DIRECTION_X / length * along;
  const double y = static_cast<double>(_pointer.y) + BADGE_DIRECTION_Y / length * along;
  const double icon = static_cast<double>(_size.height) / cursor.iconDivisor;
  const auto& marker = _presentation.ref.ctrl.marker;
  quads.push_back(
    QuadOf(SceneTexture::TeamDiscShadow, Neuron::OverlayBlend::Alpha, x, y, icon + marker.size.shadowPad, 0.0, Shadow(marker.shadowAlpha)));
  quads.push_back(QuadOf(SceneTexture::TeamDisc, Neuron::OverlayBlend::Additive, x, y, icon, 0.0, TeamColor(_team)));
  // Row 2 before row 3: a selected Leader, then selected Workers.
  quads.push_back(
    QuadOf(_leaderSelected ? SceneTexture::IconLeader : SceneTexture::IconWorkers, Neuron::OverlayBlend::Additive, x, y, icon, 0.0, WHITE));
  return quads;
}

std::optional<BadgeLabel> BadgeCount(std::span<const Neuron::OverlayQuad> _badge, std::size_t _workers)
{
  if (_badge.empty() || _workers == 0)
  {
    return std::nullopt;
  }
  const Neuron::OverlayQuad& shadow = _badge.front();
  return BadgeLabel{std::to_string(_workers), shadow.centerX + shadow.halfSize, shadow.centerY};
}

} // namespace Species
