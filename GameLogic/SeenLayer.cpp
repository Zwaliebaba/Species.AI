#include "pch.h"

#include "RegionSimulation.h"
#include "SeenLayer.h"

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
  if (_settlement.team < _region.TeamCount())
  {
    const SlicedPool<Figure>& figures = _region.TeamAt(_settlement.team).figures;
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

} // namespace Species
