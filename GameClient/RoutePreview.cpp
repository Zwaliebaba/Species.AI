#include "pch.h"

#include "CrowdLook.h"
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

} // namespace

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
  m_from = *_from;
  m_to = *_to;
  m_straight = _straight;
}

void RoutePreview::AppendBatches(const Neuron::Vec3& _camera, const Neuron::Vec3& _cameraFront, double _realSeconds,
                                 const std::array<float, 4>& _teamColor, std::vector<Neuron::QuadBatch>& _batches) const
{
  if (!Shown())
  {
    return;
  }
  const TuningPresentation::Rnd::Route& route = m_presentation->rnd.route;
  // RND-353: the team colour while walkable, the blocked colour otherwise, each with the grey offset, capped.
  const double gray = route.grayOffset / COLOR_LEVELS;
  const auto& blocked = route.blockedColor;
  const std::array<double, 4> base = m_walkable ? std::array<double, 4>{_teamColor[0], _teamColor[1], _teamColor[2], _teamColor[3]}
                                                : std::array<double, 4>{blocked.r / COLOR_LEVELS, blocked.g / COLOR_LEVELS,
                                                                        blocked.b / COLOR_LEVELS, blocked.a / COLOR_LEVELS};
  const auto colorOf = [&](std::size_t _spot)
  {
    const double pulse =
      route.pulseBase + route.pulseAmplitude * std::sin(route.pulseRate * _realSeconds + route.pulsePhaseStep * static_cast<double>(_spot));
    return std::array<float, 4>{static_cast<float>(std::min(base[0] + gray, 1.0)), static_cast<float>(std::min(base[1] + gray, 1.0)),
                                static_cast<float>(std::min(base[2] + gray, 1.0)), static_cast<float>(base[3] * pulse)};
  };
  const double divisor = route.spotSizeDivisor;
  Neuron::QuadBatch spots{Neuron::ScenePass::Routes, ROUTE_LINE, static_cast<std::uint16_t>(SceneTexture::ParticleDot), 0.0f, 0.0f, {}};
  Neuron::QuadBatch arrows{Neuron::ScenePass::Routes, ROUTE_ARROW, static_cast<std::uint16_t>(SceneTexture::RouteArrow), 0.0f, 0.0f, {}};
  std::size_t spot = 0;
  double into = 0.0; // the distance into the leg of its first spot
  for (std::size_t leg = 0; leg + 1 < m_points.size() && spot < SPOTS_MAX; ++leg)
  {
    const Neuron::Vec3& from = m_points[leg];
    const Neuron::Vec3& to = m_points[leg + 1];
    const double length = Neuron::Length(to - from);
    std::optional<std::size_t> middleSpot;
    // Spot i has the half-size of its camera distance ÷ the divisor, and the next lies one spot's size, twice its
    // half-size, further along the route.
    while (into <= length && spot < SPOTS_MAX)
    {
      const Neuron::Vec3 at = length > 0.0 ? from + (to - from) * (into / length) : from;
      const double halfSize = Neuron::Length(at - _camera) / divisor;
      spots.quads.push_back(Spot(at, halfSize, colorOf(spot)));
      spots.cullRadius = std::max(spots.cullRadius, static_cast<float>(halfSize));
      if (!middleSpot.has_value() && into >= length * 0.5)
      {
        middleSpot = spot;
      }
      ++spot;
      into += std::max(2.0 * halfSize, SPOT_STEP_MIN);
    }
    into -= length;
    // DEC-959 item 4: an arrow at the leg's middle, pointing along it on the screen (its art points right), in the colour
    // of the spot there, on a leg long enough to hold it.
    const Neuron::Vec3 middle = (from + to) * 0.5;
    const double arrowHalf = route.arrowSizeFactor * Neuron::Length(middle - _camera) / divisor;
    if (length >= 2.0 * arrowHalf)
    {
      const std::array<double, 2> along = ScreenDirection(from, to, _camera, _cameraFront);
      Neuron::SceneQuad arrow = Spot(middle, arrowHalf, colorOf(middleSpot.value_or(spot)));
      arrow.rotationRadians = static_cast<float>(std::atan2(along[1], along[0]));
      arrows.quads.push_back(arrow);
      arrows.cullRadius = std::max(arrows.cullRadius, static_cast<float>(arrowHalf * std::numbers::sqrt2));
    }
  }
  _batches.push_back(std::move(spots));
  if (!arrows.quads.empty())
  {
    _batches.push_back(std::move(arrows));
  }
}

} // namespace Species
