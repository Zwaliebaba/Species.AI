#include "pch.h"

#include "StructureCost.h"

#include <cmath>

namespace Species
{

std::int64_t StructureCostMilliRu(const TuningMvp& _tuning, StructureKind _kind, std::int64_t _dwellings) noexcept
{
  const auto& cost = _tuning.mvp.cost;
  double resourceUnits = 0.0;
  switch (_kind)
  {
  case StructureKind::Dwelling:
    resourceUnits = cost.dwellingRu * std::pow(cost.dwellingEscalation, static_cast<double>(_dwellings));
    break;
  case StructureKind::Storehouse:
    resourceUnits = cost.storehouseRu;
    break;
  case StructureKind::Turret:
    resourceUnits = cost.turretRu;
    break;
  case StructureKind::Wellspring:
  case StructureKind::ResourceSite:
  case StructureKind::ResourceSiteCentral:
    break;
  }
  return std::llround(resourceUnits * 1000.0);
}

} // namespace Species
