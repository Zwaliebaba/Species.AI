#include "pch.h"

#include "Construction.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "ScriptedSteward.h"
#include "SettlementMessages.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Species
{

namespace
{

// The eight bearings of a ring of places about a storehouse spot.
constexpr int SEARCH_BEARINGS = 8;

// What a list of places gives a kind: the first place every check passes; a place that only the stores refuse, or a
// refusal that no place changes, which waits for a later look; or no place at all.
enum class Choice : std::uint8_t
{
  Place,
  Wait,
  None
};

struct Chosen
{
  Choice choice = Choice::None;
  Neuron::Vec3 place;
};

Chosen Choose(const RegionSimulation& _region, std::uint8_t _site, StructureKind _kind, const std::vector<Neuron::Vec3>& _places)
{
  for (const Neuron::Vec3& place : _places)
  {
    const std::optional<PlacementRefusal> refusal = CheckPlacement(_region, _site, _kind, place.x, place.z);
    if (!refusal)
    {
      return {Choice::Place, place};
    }
    if (*refusal == PlacementRefusal::CannotAfford || *refusal == PlacementRefusal::QueueFull || *refusal == PlacementRefusal::NotBuildable)
    {
      return {Choice::Wait, place};
    }
  }
  return {};
}

const Settlement* SettlementOf(const RegionSimulation& _region, std::uint8_t _team)
{
  const auto found = std::ranges::find(_region.Settlements(), _team, &Settlement::team);
  return found == _region.Settlements().end() ? nullptr : &*found;
}

// The home resource sites of the settlement's site, in the generator's order: the inner ring, the outer ring, then the
// ninth (WLD-082); public world data (WLD-084).
std::vector<Neuron::Vec3> HomeSites(const RegionSimulation& _region, const Settlement& _settlement)
{
  std::vector<Neuron::Vec3> sites;
  if (_region.Terrain() == nullptr)
  {
    return sites;
  }
  for (const ResourceSitePlace& place : _region.Terrain()->resourceSites)
  {
    if (place.siteClass == ResourceSiteClass::Home && place.site == std::optional<std::uint8_t>(_settlement.site))
    {
      sites.push_back(place.position);
    }
  }
  return sites;
}

// The horizontal distance from _point to the segment from _from to _to.
double DistanceToSegment(const Neuron::Vec3& _point, const Neuron::Vec3& _from, const Neuron::Vec3& _to) noexcept
{
  const double dx = _to.x - _from.x;
  const double dz = _to.z - _from.z;
  const double lengthSquared = dx * dx + dz * dz;
  const double along =
    lengthSquared > 0.0 ? std::clamp(((_point.x - _from.x) * dx + (_point.z - _from.z) * dz) / lengthSquared, 0.0, 1.0) : 0.0;
  return std::hypot(_point.x - (_from.x + along * dx), _point.z - (_from.z + along * dz));
}

// Whether a storehouse of the settlement's own, built or a site, stands within the search's reach of the spot.
bool Served(const RegionSimulation& _region, const Settlement& _settlement, const Neuron::Vec3& _spot)
{
  const auto& steward = _region.Tuning().mvp.steward;
  const double reach = steward.storehouseSearchStep * static_cast<double>(steward.storehouseSearchRings + 1);
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot))
    {
      const Structure& structure = structures.At(slot);
      if (structure.kind == StructureKind::Storehouse && structure.owner == std::optional<std::uint8_t>(_settlement.team) &&
          Neuron::HorizontalDistance(structure.position, _spot) <= reach)
      {
        return true;
      }
    }
  }
  return false;
}

// The spot, then rings of eight places about it, mvp.steward.storehouse_search_step apart.
std::vector<Neuron::Vec3> PlacesAbout(const TuningMvp& _tuning, const Neuron::Vec3& _spot)
{
  const auto& steward = _tuning.mvp.steward;
  std::vector<Neuron::Vec3> places{_spot};
  for (std::int64_t ring = 1; ring <= steward.storehouseSearchRings; ++ring)
  {
    const double radius = steward.storehouseSearchStep * static_cast<double>(ring);
    for (int bearing = 0; bearing < SEARCH_BEARINGS; ++bearing)
    {
      const double angle = 2.0 * std::numbers::pi * bearing / SEARCH_BEARINGS;
      places.push_back({_spot.x + radius * std::cos(angle), 0.0, _spot.z + radius * std::sin(angle)});
    }
  }
  return places;
}

template <class T> void Send(RegionSimulation& _region, ScriptedSteward& _steward, const T& _command)
{
  static_cast<void>(_region.Commands().Submit(_steward.id, _steward.nextSequence++, Neuron::EncodeMessage(_command)));
}

} // namespace

std::vector<Neuron::Vec3> DwellingLayout(const RegionSimulation& _region, const Settlement& _settlement)
{
  std::vector<Neuron::Vec3> places;
  const Structure* wellspring = _region.ResolveStructure(_settlement.wellspringSlot, _settlement.wellspringId);
  if (_region.Terrain() == nullptr || wellspring == nullptr)
  {
    return places;
  }
  const auto& steward = _region.Tuning().mvp.steward;
  const Neuron::Vec3 center = wellspring->position;
  const Neuron::Vec3& valley = _region.Terrain()->layout.valleyCenter;
  const double outward = std::atan2(center.z - valley.z, center.x - valley.x);
  const std::vector<Neuron::Vec3> sites = HomeSites(_region, _settlement);
  for (std::int64_t ring = 0; ring < steward.ringsMax; ++ring)
  {
    const double radius = steward.ringFirst + steward.ringStep * static_cast<double>(ring);
    const auto count =
      std::max<std::int64_t>(1, static_cast<std::int64_t>(std::floor(2.0 * std::numbers::pi * radius / steward.slotSpacing)));
    for (std::int64_t slot = 0; slot < count; ++slot)
    {
      const double angle = outward + 2.0 * std::numbers::pi * (static_cast<double>(slot) + 0.5) / static_cast<double>(count);
      const Neuron::Vec3 place{center.x + radius * std::cos(angle), 0.0, center.z + radius * std::sin(angle)};
      if (std::ranges::none_of(sites,
                               [&](const Neuron::Vec3& _site) { return DistanceToSegment(place, center, _site) < steward.laneHalfWidth; }))
      {
        places.push_back(place);
      }
    }
  }
  return places;
}

std::vector<StorehouseSpot> StorehouseSpots(const RegionSimulation& _region, const Settlement& _settlement)
{
  std::vector<StorehouseSpot> spots;
  const std::vector<Neuron::Vec3> sites = HomeSites(_region, _settlement);
  const auto& resource = _region.Tuning().wld.resource;
  const auto first = static_cast<std::size_t>(std::max<std::int64_t>(0, resource.homeInner));
  const std::size_t end = std::min(sites.size(), first + static_cast<std::size_t>(std::max<std::int64_t>(0, resource.homeOuter)));
  const CellRaster* territory = _region.TerritoryOf(_settlement.site);
  for (std::size_t outer = first; outer + 1 < end; ++outer)
  {
    const Neuron::Vec3& a = sites[outer];
    const Neuron::Vec3& b = sites[outer + 1];
    const bool held = territory != nullptr && territory->Holds(a.x, a.z) && territory->Holds(b.x, b.z);
    spots.push_back({{(a.x + b.x) / 2.0, 0.0, (a.z + b.z) / 2.0}, held});
  }
  return spots;
}

void StepScriptedSteward(RegionSimulation& _region, ScriptedSteward& _steward)
{
  if (--_steward.thinkTicks > 0)
  {
    return;
  }
  const TuningMvp& tuning = _region.Tuning();
  _steward.thinkTicks = std::max(1, static_cast<std::int32_t>(std::llround(tuning.mvp.steward.thinkS / tuning.ref.sim.tickPeriod)));
  const Settlement* settlement = SettlementOf(_region, _steward.team);
  if (settlement == nullptr)
  {
    return;
  }
  bool storehouseQueued = false;
  bool dwellingQueued = false;
  for (const StructureRef& queued : settlement->queue)
  {
    if (const Structure* site = _region.ResolveStructure(queued.slot, queued.id))
    {
      storehouseQueued = storehouseQueued || site->kind == StructureKind::Storehouse;
      dwellingQueued = dwellingQueued || site->kind == StructureKind::Dwelling;
    }
  }
  // MVP-034: a storehouse at the first spot held and not yet served that has a place.
  if (!storehouseQueued)
  {
    for (const StorehouseSpot& spot : StorehouseSpots(_region, *settlement))
    {
      if (!spot.held || Served(_region, *settlement, spot.position))
      {
        continue;
      }
      const Chosen chosen = Choose(_region, settlement->site, StructureKind::Storehouse, PlacesAbout(tuning, spot.position));
      if (chosen.choice == Choice::Place)
      {
        Send(_region, _steward,
             PlaceStructureCommand{_steward.team, StructureKind::Storehouse, GroundPoint{chosen.place.x, chosen.place.z}});
      }
      if (chosen.choice != Choice::None)
      {
        break;
      }
    }
  }
  // MVP-022: a dwelling once the population nears a limit that a dwelling still raises.
  const std::uint32_t population = Population(_region, *settlement);
  const std::uint32_t limit = PopulationLimitOf(_region, *settlement);
  if (!dwellingQueued && std::int64_t{population} + tuning.mvp.steward.dwellingMargin >= std::int64_t{limit} &&
      std::int64_t{limit} < tuning.mvp.settlement.populationMax)
  {
    const Chosen chosen = Choose(_region, settlement->site, StructureKind::Dwelling, DwellingLayout(_region, *settlement));
    if (chosen.choice == Choice::Place)
    {
      Send(_region, _steward, PlaceStructureCommand{_steward.team, StructureKind::Dwelling, GroundPoint{chosen.place.x, chosen.place.z}});
    }
  }
}

void WriteScriptedSteward(Neuron::BitWriter& _writer, const ScriptedSteward& _steward)
{
  _writer.WriteBits(_steward.id, 64);
  _writer.WriteBits(_steward.team, 8);
  _writer.WriteSigned(_steward.thinkTicks, 32);
  _writer.WriteBits(_steward.nextSequence, 64);
}

bool ReadScriptedSteward(Neuron::BitReader& _reader, ScriptedSteward& _steward)
{
  _steward.id = _reader.ReadBits(64);
  _steward.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _steward.thinkTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _steward.nextSequence = _reader.ReadBits(64);
  return !_reader.Failed() && _steward.id != 0;
}

} // namespace Species
