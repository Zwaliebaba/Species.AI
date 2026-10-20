#include "pch.h"

#include "OrderMarkers.h"
#include "RegionSimulation.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Species
{

void CreateOrderMarker(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, RouteHandle _route)
{
  Effect marker;
  marker.id = _region.IssueId();
  marker.kind = EffectKind::OrderMarker;
  marker.team = _team;
  marker.position = _position;
  // It starts at route point 1, point 0 being the Leader; a route of one point ends where it starts.
  const std::vector<Neuron::Vec3>* points = _region.Routes().Find(_route);
  if (points == nullptr || points->empty())
  {
    marker.arrived = true;
    marker.fuseTicks = static_cast<std::int32_t>(_region.Tuning().ref.unit.leader.markerFadeTicks);
  }
  else
  {
    marker.route = _route;
    marker.routePoint = std::min<std::int32_t>(1, static_cast<std::int32_t>(points->size()) - 1);
  }
  static_cast<void>(_region.Effects().Take(marker));
}

bool AdvanceOrderMarker(RegionSimulation& _region, Effect& _marker)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& leader = tuning.ref.unit.leader;
  const double dt = tuning.ref.sim.tickPeriod;
  if (!_marker.arrived)
  {
    // It steps to the next route point within the reach; past the last one it has arrived.
    const std::vector<Neuron::Vec3>* points = _region.Routes().Find(_marker.route);
    const auto last = points != nullptr ? static_cast<std::int32_t>(points->size()) - 1 : -1;
    while (points != nullptr && _marker.routePoint <= last &&
           Neuron::HorizontalDistance(_marker.position, (*points)[static_cast<std::size_t>(_marker.routePoint)]) <= leader.markerReach)
    {
      ++_marker.routePoint;
    }
    if (points == nullptr || _marker.routePoint > last)
    {
      _marker.arrived = true;
      _marker.routePoint = points != nullptr ? last : -1;
      _marker.route = points != nullptr ? _marker.route : 0;
      _marker.velocity = {};
      _marker.fuseTicks = static_cast<std::int32_t>(leader.markerFadeTicks);
    }
    else
    {
      const Neuron::Vec3& target = (*points)[static_cast<std::size_t>(_marker.routePoint)];
      const double speed = std::clamp(Neuron::Length(_marker.velocity) * (1.0 + dt), leader.markerSpeedMin, leader.markerSpeedMax);
      const Neuron::Vec3 direction = Neuron::Normalized({target.x - _marker.position.x, 0.0, target.z - _marker.position.z});
      _marker.velocity = direction * speed;
      _marker.position = _marker.position + _marker.velocity * dt;
      _marker.position.y = _region.Terrain()->field.Height(_marker.position.x, _marker.position.z);
      // Every trail period of its advances it drops a trail mark, for every team.
      ++_marker.activeTick;
      if (_marker.activeTick % static_cast<std::int32_t>(leader.trailPeriod) == 0)
      {
        Effect mark;
        mark.id = _region.IssueId();
        mark.kind = EffectKind::OrderTrailMark;
        mark.team = _marker.team;
        mark.position = _marker.position;
        mark.velocity = _marker.velocity;
        mark.createdTick = _region.Draws().Tick();
        static_cast<void>(_region.Effects().Take(mark));
      }
      return false;
    }
  }
  // It fades for the fade's ticks after it arrives, and is removed.
  --_marker.fuseTicks;
  return _marker.fuseTicks <= 0;
}

bool AdvanceTrailMark(const RegionSimulation& _region, const Effect& _mark) noexcept
{
  return _region.Draws().Tick() - _mark.createdTick >= static_cast<std::uint64_t>(_region.Tuning().ref.unit.leader.trailLifeTicks);
}

} // namespace Species
