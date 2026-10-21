#include "pch.h"

#include "CrowdLook.h"
#include "Geometry.h"
#include "Navigation.h"
#include "RoutePreview.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <optional>
#include <string_view>
#include <utility>

namespace Species
{

namespace
{

// The catalogue entries of pass 12 (RND-355, ADR-125).
constexpr std::string_view ROUTE_LINE = "route_line";
constexpr std::string_view ROUTE_ARROW = "route_arrow";
constexpr double COLOR_LEVELS = 255.0;
// The shortest step between two spots, wu, so that a spot at the camera does not stall the walk, and the most spots a
// preview draws.
constexpr double SPOT_STEP_MIN = 0.05;
constexpr std::size_t SPOTS_MAX = 4096;
// The nearest depth ahead of the camera, wu, at which a leg's end is projected to find its middle on the screen.
constexpr double SCREEN_DEPTH_MIN = 1e-3;

Neuron::SceneQuad Spot(const Neuron::Vec3& _at, double _halfSize, const std::array<float, 4>& _color) noexcept
{
  Neuron::SceneQuad quad;
  quad.position = _at;
  quad.halfWidth = static_cast<float>(_halfSize);
  quad.halfHeight = quad.halfWidth;
  quad.color = _color;
  quad.endColor = _color;
  return quad;
}

// ADR-127: the fraction along a leg whose projection is the middle of the leg's projection, from its ends' depths ahead
// of the camera, w0 ÷ (w0 + w1); the leg's middle while an end lies behind the camera.
double ScreenMiddle(const Neuron::Vec3& _from, const Neuron::Vec3& _to, const Neuron::Vec3& _camera,
                    const Neuron::Vec3& _cameraFront) noexcept
{
  const double fromDepth = Neuron::Dot(_from - _camera, _cameraFront);
  const double toDepth = Neuron::Dot(_to - _camera, _cameraFront);
  return fromDepth > SCREEN_DEPTH_MIN && toDepth > SCREEN_DEPTH_MIN ? fromDepth / (fromDepth + toDepth) : 0.5;
}

} // namespace

/// The batches of pass 12 in their order: the spots, the arrows and the destination marks.
struct RoutePreview::Batches
{
  Neuron::QuadBatch spots{Neuron::ScenePass::Routes, ROUTE_LINE, static_cast<std::uint16_t>(SceneTexture::ParticleDot), 0.0f, 0.0f, {}};
  Neuron::QuadBatch arrows{Neuron::ScenePass::Routes, ROUTE_ARROW, static_cast<std::uint16_t>(SceneTexture::RouteArrow), 0.0f, 0.0f, {}};
  Neuron::QuadBatch ends{Neuron::ScenePass::Routes, ROUTE_ARROW, static_cast<std::uint16_t>(SceneTexture::OrderTrail), 0.0f, 0.0f, {}};
};

RoutePreview::RoutePreview(const TuningPresentation& _presentation, const TuningMvp& _tuning) noexcept
  : m_presentation(&_presentation),
    m_tuning(&_tuning)
{
}

void RoutePreview::Clear() noexcept
{
  m_points.clear();
  m_walkable = false;
}

void RoutePreview::Reset() noexcept
{
  Clear();
  m_ordered.clear();
}

void RoutePreview::Commit()
{
  if (Shown() && m_walkable)
  {
    m_ordered.push_back({m_points, m_endNormal, 1.0});
  }
}

void RoutePreview::Advance(double _realStepSeconds)
{
  const double rate = m_presentation->rnd.route.fadeRate;
  for (Ordered& route : m_ordered)
  {
    route.fade -= _realStepSeconds * rate;
  }
  std::erase_if(m_ordered, [](const Ordered& _route) { return _route.fade <= 0.0; });
}

void RoutePreview::Update(const std::optional<Neuron::Vec3>& _from, const std::optional<Neuron::Vec3>& _to, bool _straight,
                          const RegionTerrain& _terrain)
{
  if (!_from.has_value() || !_to.has_value())
  {
    Clear();
    return;
  }
  const double rebuild = m_presentation->rnd.route.previewRebuildDistance;
  if (Shown() && _straight == m_straight && Neuron::Length(*_from - m_from) <= rebuild && Neuron::Length(*_to - m_to) <= rebuild)
  {
    return;
  }
  // R-SIM-358: the preview's own store; the simulation's route store never sees it.
  m_points = _terrain.routes.BuildRoute(*_from, *_to, _straight, false, false, _terrain.field, *m_tuning);
  m_walkable = m_points.size() > 1 && WalkingDistance(m_points, _terrain.field, *m_tuning) >= 0.0;
  if (!m_points.empty())
  {
    m_endNormal = _terrain.field.Normal(m_points.back().x, m_points.back().z);
  }
  m_from = *_from;
  m_to = *_to;
  m_straight = _straight;
}

void RoutePreview::AppendBatches(const Neuron::Vec3& _camera, const Neuron::Vec3& _cameraFront, double _realSeconds,
                                 const std::array<float, 4>& _teamColor, std::vector<Neuron::QuadBatch>& _batches) const
{
  Batches batches;
  for (const Ordered& route : m_ordered)
  {
    AppendRoute(route.points, route.endNormal, true, route.fade, _camera, _cameraFront, _realSeconds, _teamColor, batches);
  }
  if (Shown())
  {
    AppendRoute(m_points, m_endNormal, m_walkable, 1.0, _camera, _cameraFront, _realSeconds, _teamColor, batches);
  }
  for (Neuron::QuadBatch* batch : {&batches.spots, &batches.arrows, &batches.ends})
  {
    if (!batch->quads.empty())
    {
      _batches.push_back(std::move(*batch));
    }
  }
}

void RoutePreview::AppendRoute(const std::vector<Neuron::Vec3>& _points, const Neuron::Vec3& _endNormal, bool _walkable, double _fade,
                               const Neuron::Vec3& _camera, const Neuron::Vec3& _cameraFront, double _realSeconds,
                               const std::array<float, 4>& _teamColor, Batches& _batches) const
{
  if (_points.size() < 2)
  {
    return;
  }
  const TuningPresentation::Rnd::Route& route = m_presentation->rnd.route;
  // RND-353: the team colour while walkable, the blocked colour otherwise, each with the grey offset, capped; an ordered
  // route's alpha is × its fade value (RND-352).
  const double gray = route.grayOffset / COLOR_LEVELS;
  const auto& blocked = route.blockedColor;
  const std::array<double, 4> base = _walkable ? std::array<double, 4>{_teamColor[0], _teamColor[1], _teamColor[2], _teamColor[3]}
                                               : std::array<double, 4>{blocked.r / COLOR_LEVELS, blocked.g / COLOR_LEVELS,
                                                                       blocked.b / COLOR_LEVELS, blocked.a / COLOR_LEVELS};
  const double fade = std::clamp(_fade, 0.0, 1.0);
  const auto colorOf = [&](std::size_t _spot)
  {
    const double pulse =
      route.pulseBase + route.pulseAmplitude * std::sin(route.pulseRate * _realSeconds + route.pulsePhaseStep * static_cast<double>(_spot));
    return std::array<float, 4>{static_cast<float>(std::min(base[0] + gray, 1.0)), static_cast<float>(std::min(base[1] + gray, 1.0)),
                                static_cast<float>(std::min(base[2] + gray, 1.0)), static_cast<float>(base[3] * pulse * fade)};
  };
  const double divisor = route.spotSizeDivisor;
  std::size_t spot = 0;
  double into = 0.0; // the distance into the leg of its first spot
  for (std::size_t leg = 0; leg + 1 < _points.size() && spot < SPOTS_MAX; ++leg)
  {
    const Neuron::Vec3& from = _points[leg];
    const Neuron::Vec3& to = _points[leg + 1];
    const double length = Neuron::Length(to - from);
    // ADR-127: the arrow lies at the leg's middle on the screen.
    const double middleFraction = ScreenMiddle(from, to, _camera, _cameraFront);
    std::optional<std::size_t> middleSpot;
    // Spot i has the half-size of its camera distance ÷ the divisor, and the next lies one spot's size, twice its
    // half-size, further along the route.
    while (into <= length && spot < SPOTS_MAX)
    {
      const Neuron::Vec3 at = length > 0.0 ? from + (to - from) * (into / length) : from;
      const double halfSize = Neuron::Length(at - _camera) / divisor;
      _batches.spots.quads.push_back(Spot(at, halfSize, colorOf(spot)));
      _batches.spots.cullRadius = std::max(_batches.spots.cullRadius, static_cast<float>(halfSize));
      if (!middleSpot.has_value() && into >= length * middleFraction)
      {
        middleSpot = spot;
      }
      ++spot;
      into += std::max(2.0 * halfSize, SPOT_STEP_MIN);
    }
    into -= length;
    // DEC-959 item 4: an arrow pointing along the leg on the screen (its art points right), in the colour of the spot
    // there, on a leg long enough to hold it.
    const Neuron::Vec3 middle = from + (to - from) * middleFraction;
    const double arrowHalf = route.arrowSizeFactor * Neuron::Length(middle - _camera) / divisor;
    if (length >= 2.0 * arrowHalf)
    {
      const std::array<double, 2> along = ScreenDirection(from, to, _camera, _cameraFront);
      Neuron::SceneQuad arrow = Spot(middle, arrowHalf, colorOf(middleSpot.value_or(spot)));
      arrow.rotationRadians = static_cast<float>(std::atan2(along[1], along[0]));
      _batches.arrows.quads.push_back(arrow);
      _batches.arrows.cullRadius = std::max(_batches.arrows.cullRadius, static_cast<float>(arrowHalf * std::numbers::sqrt2));
    }
  }
  // ADR-127: marker.order_trail on the ground at the route's end, sized as R-UNIT-186's trail marks are, in the colour of
  // the last spot.
  const auto& leader = m_presentation->ref.unit.leader;
  const Neuron::Vec3& end = _points.back();
  const double endHalf = std::max(leader.trailSizeMin, leader.trailSizeScale * Neuron::Length(end - _camera) / leader.trailSizeDivisor);
  const Neuron::Vec3 normal = Neuron::NormalizedOr(_endNormal, {0.0, 1.0, 0.0});
  const Neuron::Vec3 right = Neuron::NormalizedOr(Neuron::Cross(normal, {0.0, 0.0, 1.0}), {1.0, 0.0, 0.0});
  Neuron::SceneQuad mark = Spot(end, endHalf, colorOf(spot > 0 ? spot - 1 : 0));
  mark.orientation = Neuron::QuadOrientation::WorldOriented;
  mark.axisRight = right;
  mark.axisUp = Neuron::Cross(right, normal);
  _batches.ends.quads.push_back(mark);
  _batches.ends.cullRadius = std::max(_batches.ends.cullRadius, static_cast<float>(endHalf * std::numbers::sqrt2));
}

} // namespace Species
