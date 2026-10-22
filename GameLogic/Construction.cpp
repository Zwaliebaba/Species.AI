#include "pch.h"

#include "Construction.h"
#include "RegionSimulation.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Species
{

namespace
{

// Two horizontal discs meet: their centres are strictly closer than _reach.
bool Meet(const Neuron::Vec3& _a, const Neuron::Vec3& _b, double _reach) noexcept
{
  const double dx = _a.x - _b.x;
  const double dz = _a.z - _b.z;
  return dx * dx + dz * dz < _reach * _reach;
}

// MVP-032 for two structures: their footprints, the horizontal discs of their bounding spheres, meet, or a port of
// either and the other's footprint come within the port's radius (ADR-132).
bool Overlap(const Structure& _a, const Structure& _b, double _portRadius) noexcept
{
  if (Meet(_a.center, _b.center, _a.radius + _b.radius))
  {
    return true;
  }
  const auto portIn = [&](const Structure& _ported, const Structure& _other)
  {
    return std::ranges::any_of(_ported.ports, [&](const StructurePort& _port)
                               { return Meet(_port.position, _other.center, _other.radius + _portRadius); });
  };
  return portIn(_a, _b) || portIn(_b, _a);
}

// R-SIM-335 and WLD-163 check 6's walkable ground: above the shore height, and no steeper than walkable.
bool Walkable(const Neuron::TerrainField& _field, const Neuron::Vec3& _point, const TuningMvp& _tuning) noexcept
{
  return _field.Height(_point.x, _point.z) > _tuning.ref.unit.worker.shoreHeight &&
         _field.Normal(_point.x, _point.z).y >= _tuning.ref.sim.walk.minNormalY;
}

} // namespace

std::int64_t CostMilliRu(const RegionSimulation& _region, const Settlement& _settlement, StructureKind _kind)
{
  const auto& cost = _region.Tuning().mvp.cost;
  if (_kind != StructureKind::Dwelling)
  {
    return MilliRuOf(_kind == StructureKind::Storehouse ? cost.storehouseRu : _kind == StructureKind::Turret ? cost.turretRu : 0.0);
  }
  std::int64_t dwellings = 0;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && structures.At(slot).kind == StructureKind::Dwelling &&
        structures.At(slot).owner == std::optional<std::uint8_t>(_settlement.team))
    {
      ++dwellings;
    }
  }
  return MilliRuOf(cost.dwellingRu * std::pow(cost.dwellingEscalation, static_cast<double>(dwellings)));
}

std::int64_t RequiredWorkTicks(const Structure& _structure, const TuningMvp& _tuning) noexcept
{
  const double costRu = static_cast<double>(_structure.costMilliRu) / 1000.0;
  return std::llround(costRu * _tuning.mvp.construction.workPerRu / _tuning.ref.sim.tickPeriod);
}

std::uint32_t ActiveCount(const RegionSimulation& _region, std::uint8_t _team, StructureKind _kind)
{
  std::uint32_t count = 0;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot))
    {
      continue;
    }
    const Structure& structure = structures.At(slot);
    if (structure.kind == _kind && structure.owner == std::optional<std::uint8_t>(_team) && structure.built && structure.active)
    {
      ++count;
    }
  }
  return count;
}

std::uint32_t Population(const RegionSimulation& _region, const Settlement& _settlement)
{
  if (_settlement.team >= _region.TeamCount())
  {
    return 0;
  }
  std::uint32_t population = 0;
  const SlicedPool<Figure>& figures = _region.TeamAt(_settlement.team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    population += figures.Used(slot) && !figures.At(slot).dead ? 1U : 0U;
  }
  return population;
}

std::uint32_t PopulationLimitOf(const RegionSimulation& _region, const Settlement& _settlement)
{
  return PopulationLimit(_region.Tuning(), ActiveCount(_region, _settlement.team, StructureKind::Dwelling));
}

bool InTerritory(const RegionSimulation& _region, const Settlement& _settlement, const Neuron::Vec3& _point, double _radius)
{
  const CellRaster* territory = _region.TerritoryOf(_settlement.site);
  return territory != nullptr && territory->HoldsDisc(_point.x, _point.z, _radius);
}

bool Queued(const RegionSimulation& _region, const Structure& _structure)
{
  return std::ranges::any_of(_region.Settlements(), [&](const Settlement& _settlement)
                             { return std::ranges::find(_settlement.queue, _structure.id, &StructureRef::id) != _settlement.queue.end(); });
}

std::expected<void, PlacementRefusal> RebuildTurret(RegionSimulation& _region, std::uint8_t _site, std::uint32_t _slot, StableId _id)
{
  const auto found = std::ranges::find(_region.Settlements(), _site, &Settlement::site);
  Structure* site = _region.ResolveStructure(_slot, _id);
  if (found == _region.Settlements().end() || site == nullptr || site->kind != StructureKind::Turret || site->built ||
      site->owner != std::optional<std::uint8_t>(found->team) || Queued(_region, *site))
  {
    return std::unexpected(PlacementRefusal::NotBuildable);
  }
  Settlement& settlement = *found;
  if (std::cmp_greater_equal(settlement.queue.size(), _region.Tuning().mvp.construction.queueMax))
  {
    return std::unexpected(PlacementRefusal::QueueFull);
  }
  if (!Withdraw(settlement, site->costMilliRu))
  {
    return std::unexpected(PlacementRefusal::CannotAfford);
  }
  settlement.queue.push_back({_id, _slot});
  return {};
}

bool PortsMannable(const Structure& _structure) noexcept
{
  return _structure.built ? _structure.kind != StructureKind::Dwelling && _structure.kind != StructureKind::Storehouse : true;
}

std::expected<std::uint32_t, PlacementRefusal> PlaceStructure(RegionSimulation& _region, std::uint8_t _site, StructureKind _kind, double _x,
                                                              double _z)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto found = std::ranges::find(_region.Settlements(), _site, &Settlement::site);
  const bool kind = _kind == StructureKind::Dwelling || _kind == StructureKind::Storehouse || _kind == StructureKind::Turret;
  if (found == _region.Settlements().end() || !kind || _region.Terrain() == nullptr || _region.Records() == nullptr)
  {
    return std::unexpected(PlacementRefusal::NotBuildable);
  }
  Settlement& settlement = *found;
  const Structure* wellspring = _region.ResolveStructure(settlement.wellspringSlot, settlement.wellspringId);
  if (wellspring == nullptr)
  {
    return std::unexpected(PlacementRefusal::NotBuildable);
  }
  if (std::cmp_greater_equal(settlement.queue.size(), tuning.mvp.construction.queueMax))
  {
    return std::unexpected(PlacementRefusal::QueueFull);
  }
  const Neuron::TerrainField& field = _region.Terrain()->field;
  const Neuron::Vec3 away{_x - wellspring->position.x, 0.0, _z - wellspring->position.z};
  Structure candidate = MakeStructure(_kind, settlement.team, std::nullopt, _x, _z, away, RecordOf(*_region.Records(), _kind),
                                      CollisionOf(*_region.Records(), _kind), field, tuning);
  candidate.built = false;
  candidate.costMilliRu = CostMilliRu(_region, settlement, _kind);
  if (!InTerritory(_region, settlement, candidate.center, candidate.radius))
  {
    return std::unexpected(PlacementRefusal::OutsideTerritory);
  }
  if (Neuron::HorizontalDistance(candidate.center, wellspring->position) - candidate.radius < tuning.mvp.construction.centerRingRadius)
  {
    return std::unexpected(PlacementRefusal::InCenterRing);
  }
  const bool walkable =
    Walkable(field, candidate.position, tuning) &&
    std::ranges::all_of(candidate.ports, [&](const StructurePort& _port) { return Walkable(field, _port.position, tuning); });
  if (!walkable)
  {
    return std::unexpected(PlacementRefusal::NotWalkable);
  }
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && Overlap(candidate, structures.At(slot), tuning.ref.bldg.port.radiusWu))
    {
      return std::unexpected(PlacementRefusal::Overlaps);
    }
  }
  if (!Withdraw(settlement, candidate.costMilliRu))
  {
    return std::unexpected(PlacementRefusal::CannotAfford);
  }
  const std::uint32_t slot = _region.InsertStructure(std::move(candidate));
  settlement.queue.push_back({_region.Structures().At(slot).id, slot});
  return slot;
}

} // namespace Species
