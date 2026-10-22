#pragma once

#include "Settlement.h"
#include "Territory.h"

namespace Species
{

class RegionSimulation;

// MVP-016 (ADR-134): the settlement's seen layer, in cells of mvp.vision.cell_size over its region's square: every cell
// whose centre its territory holds, and every cell whose centre lies within mvp.vision.unit_radius of one of its living
// figures or of a structure it owns, built or not. The public layer of WLD-242 is the world's own data, which every
// settlement reads as it is.
[[nodiscard]] CellRaster ComputeSeenLayer(const RegionSimulation& _region, const Settlement& _settlement);

} // namespace Species
