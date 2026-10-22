#include "pch.h"

#include "RegionSimulation.h"
#include "SeenLayer.h"

#include <algorithm>
#include <numbers>

namespace Species
{

CellRaster ComputeSeenLayer(const RegionSimulation& _region, const Settlement& _settlement)
{
  const TuningMvp& tuning = _region.Tuning();
  const RegionLayout& layout = _region.Terrain()->layout;
  CellRaster seen(layout.originX, layout.originZ, layout.side, tuning.mvp.vision.cellSize);
  if (const CellRaster* territory = _region.TerritoryOf(_settlement.site); territory != nullptr)
  {
    for (std::int32_t z = 0; z < seen.CellsPerAxis(); ++z)
    {
      for (std::int32_t x = 0; x < seen.CellsPerAxis(); ++x)
      {
        const Neuron::Vec3 center = seen.CellCenter(x, z);
        if (territory->Holds(center.x, center.z))
        {
          seen.Hold(x, z);
        }
      }
    }
  }
  const double radius = tuning.mvp.vision.unitRadius;
  // ADR-161: and about the figures of its expedition.
  for (const std::uint8_t team : {_settlement.team, ExpeditionTeamOf(_settlement.site)})
  {
    if (team >= _region.TeamCount())
    {
      continue;
    }
    const SlicedPool<Figure>& figures = _region.TeamAt(team).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (figures.Used(slot) && !figures.At(slot).dead)
      {
        seen.HoldDisc(figures.At(slot).position.x, figures.At(slot).position.z, radius);
      }
    }
  }
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && structures.At(slot).owner == std::optional<std::uint8_t>(_settlement.team))
    {
      seen.HoldDisc(structures.At(slot).position.x, structures.At(slot).position.z, radius);
    }
  }
  return seen;
}

Sight Sight::OfTeam(const RegionSimulation& _region, std::uint8_t _team)
{
  Sight sight;
  for (const Settlement& settlement : _region.Settlements())
  {
    if (settlement.team == _team || ExpeditionTeamOf(settlement.site) == _team)
    {
      sight.m_narrowed = true;
      if (const CellRaster* seen = _region.SeenLayerOf(settlement.site); seen != nullptr)
      {
        sight.m_layers.push_back(seen);
      }
    }
  }
  return sight;
}

Sight Sight::OfSteward(const RegionSimulation& _region, StableId _steward)
{
  Sight sight;
  for (const Settlement& settlement : _region.Settlements())
  {
    if (_region.CommanderOf(settlement.team).sender == _steward)
    {
      sight.m_narrowed = true;
      if (const CellRaster* seen = _region.SeenLayerOf(settlement.site); seen != nullptr)
      {
        sight.m_layers.push_back(seen);
      }
    }
  }
  return sight;
}

bool Sight::Sees(double _x, double _z) const noexcept
{
  return !m_narrowed || std::ranges::any_of(m_layers, [_x, _z](const CellRaster* _layer) { return _layer->Holds(_x, _z); });
}

bool Sight::SeesDisc(double _x, double _z, double _radius) const noexcept
{
  // A cell a point of the disc lies in has its center within half the cell's diagonal more.
  return !m_narrowed ||
         std::ranges::any_of(m_layers, [_x, _z, _radius](const CellRaster* _layer)
                             { return _layer->HoldsDisc(_x, _z, _radius + _layer->CellSize() * std::numbers::sqrt2 / 2.0); });
}

} // namespace Species
