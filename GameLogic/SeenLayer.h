#pragma once

#include "Settlement.h"
#include "StableId.h"
#include "Territory.h"

#include <cstdint>
#include <vector>

namespace Species
{

class RegionSimulation;

// MVP-016 (ADR-134): the settlement's seen layer, in cells of mvp.vision.cell_size over its region's square: every cell
// whose centre its territory holds, and every cell whose centre lies within mvp.vision.unit_radius of one of its living
// figures or of a structure it owns, built or not. The public layer of WLD-242 is the world's own data, which every
// settlement reads as it is.
[[nodiscard]] CellRaster ComputeSeenLayer(const RegionSimulation& _region, const Settlement& _settlement);

/// MVP-016, MVP-058 and MVP-097 (ADR-158): the places one reader of the seen layer may see of a region at a step's end,
/// the union of the seen layers of its settlements. A reader with no settlement sees every place: the sandbox's teams,
/// and the client of a steward who commands none, as a watched world's has none. A reader whose settlement has no seen
/// layer yet sees none. It points at the region's seen layers, so it lives no longer than the step it was made in.
class Sight
{
public:
  // Every place.
  Sight() = default;
  // The AI of _team: the seen layer of the settlement whose team it is.
  [[nodiscard]] static Sight OfTeam(const RegionSimulation& _region, std::uint8_t _team);
  // The client of steward _steward: the seen layers of the settlements it commands.
  [[nodiscard]] static Sight OfSteward(const RegionSimulation& _region, StableId _steward);

  // Narrowed to the seen layers of settlements, rather than every place.
  [[nodiscard]] bool Narrowed() const noexcept
  {
    return m_narrowed;
  }
  [[nodiscard]] bool Sees(double _x, double _z) const noexcept;
  // Every point within _radius of (_x, _z) is seen: one of the seen layers holds every cell such a point can lie in.
  [[nodiscard]] bool SeesDisc(double _x, double _z, double _radius) const noexcept;

private:
  std::vector<const CellRaster*> m_layers;
  bool m_narrowed = false;
};

} // namespace Species
