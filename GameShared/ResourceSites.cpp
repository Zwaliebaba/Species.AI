#include "pch.h"

#include "RegionTerrain.h"
#include "ResourceSites.h"

#include <algorithm>
#include <array>
#include <tuple>
#include <cmath>
#include <numbers>

namespace Species
{

namespace
{

// The horizontal unit vector from _from to _to, or +x when they stand on one point.
Neuron::Vec3 Towards(const Neuron::Vec3& _from, const Neuron::Vec3& _to) noexcept
{
  const double dx = _to.x - _from.x;
  const double dz = _to.z - _from.z;
  const double length = std::sqrt(dx * dx + dz * dz);
  return length > 0.0 ? Neuron::Vec3{dx / length, 0.0, dz / length} : Neuron::Vec3{1.0, 0.0, 0.0};
}

// _direction turned by _degrees about the vertical, from +x towards +z as the layout's rotation turns (WLD-040).
Neuron::Vec3 Turned(const Neuron::Vec3& _direction, double _degrees) noexcept
{
  const double radians = _degrees * (std::numbers::pi / 180.0);
  const double cosine = std::cos(radians);
  const double sine = std::sin(radians);
  return {_direction.x * cosine - _direction.z * sine, 0.0, _direction.x * sine + _direction.z * cosine};
}

// The distance from a disc's centre to the centroid of the segment that a chord _chordDistance from that centre cuts
// from a disc of radius _radius, on the far side of the chord; none when the chord misses the disc.
std::optional<double> SegmentCentroidDistance(double _radius, double _chordDistance) noexcept
{
  if (_chordDistance >= _radius)
  {
    return std::nullopt;
  }
  const double halfChordSquared = _radius * _radius - _chordDistance * _chordDistance;
  const double halfChord = std::sqrt(halfChordSquared);
  const double area = _radius * _radius * std::acos(_chordDistance / _radius) - _chordDistance * halfChord;
  return 2.0 * halfChordSquared * halfChord / (3.0 * area);
}

ResourceSitePlace PlaceAt(ResourceSiteClass _class, std::optional<std::uint8_t> _site, double _x, double _z, const Neuron::Vec3& _facing,
                          std::uint32_t _units, const Neuron::TerrainField& _field)
{
  ResourceSitePlace place;
  place.siteClass = _class;
  place.site = _site;
  place.position = {_x, _field.Height(_x, _z), _z};
  place.front = Towards(place.position, _facing);
  place.units = _units;
  return place;
}

} // namespace

std::vector<ResourceSitePlace> PlaceResourceSites(const RegionLayout& _layout, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  const auto& resource = _tuning.wld.resource;
  std::vector<ResourceSitePlace> places;
  for (std::size_t local = 0; local < _layout.sites.size(); ++local)
  {
    const Neuron::Vec3& center = _layout.sites[local];
    const auto site = static_cast<std::uint8_t>(local);
    // WLD-082: the rings, measured from the outward diagonal, and the ninth at its bearing (DEC-1000).
    const Neuron::Vec3 diagonal = Towards(_layout.valleyCenter, center);
    const auto placeHome = [&](double _degrees, double _radius)
    {
      const Neuron::Vec3 bearing = Turned(diagonal, _degrees);
      places.push_back(
        PlaceAt(ResourceSiteClass::Home, site, center.x + bearing.x * _radius, center.z + bearing.z * _radius, center, 1, _field));
    };
    const auto ring = [&](std::int64_t _count, double _radius)
    {
      const double step = 180.0 / static_cast<double>(std::max<std::int64_t>(_count, 1));
      for (std::int64_t k = 0; k < _count; ++k)
      {
        placeHome((static_cast<double>(k) - static_cast<double>(_count - 1) / 2.0) * step, _radius);
      }
    };
    ring(resource.homeInner, resource.innerRadius);
    ring(resource.homeOuter, resource.outerRadius);
    placeHome(resource.extraBearing, resource.extraRadius);
    // WLD-062, WLD-080: the centre of each of its own valley border areas, the neighbours across x and then across z.
    for (const std::size_t neighbor : {local ^ 1U, local ^ 2U})
    {
      const Neuron::Vec3& other = _layout.sites[neighbor];
      const double distance = Neuron::HorizontalDistance(center, other);
      const std::optional<double> centroid = SegmentCentroidDistance(_tuning.wld.territory.maxRadius, distance / 2.0);
      if (!centroid)
      {
        continue; // the two discs do not overlap: no border area
      }
      const Neuron::Vec3 along = Towards(center, other);
      const double from = distance - centroid.value_or(0.0);
      places.push_back(PlaceAt(ResourceSiteClass::Border, site, center.x + along.x * from, center.z + along.z * from, center,
                               static_cast<std::uint32_t>(resource.borderFactor), _field));
    }
  }
  // WLD-083: the central resource site at the valley's centre.
  places.push_back(PlaceAt(ResourceSiteClass::Central, std::nullopt, _layout.valleyCenter.x, _layout.valleyCenter.z, _layout.sites[0],
                           static_cast<std::uint32_t>(resource.centralFactor), _field));
  return places;
}

bool HomeYieldsAgree(std::span<const ResourceSitePlace> _places, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  std::array<std::uint64_t, std::tuple_size_v<decltype(RegionLayout::sites)>> yields{};
  for (const ResourceSitePlace& place : _places)
  {
    const Neuron::Vec3& p = place.position;
    const bool walkable =
      _field.Height(p.x, p.z) > _tuning.ref.unit.worker.shoreHeight && _field.Normal(p.x, p.z).y >= _tuning.ref.sim.walk.minNormalY;
    if (place.siteClass == ResourceSiteClass::Home && walkable && place.site && place.site.value_or(0) < yields.size())
    {
      yields[place.site.value_or(0)] += place.units;
    }
  }
  const auto [smallest, largest] = std::ranges::minmax(yields);
  return static_cast<double>(largest - smallest) <= _tuning.wld.resource.homeYieldTolerance / 100.0 * static_cast<double>(largest);
}

} // namespace Species
