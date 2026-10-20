#pragma once

#include "RouteStore.h"
#include "Vec3.h"
#include "Weapons.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// R-UNIT-163 step 5, R-UNIT-185: an order marker of _team at _position, which follows _route from its point 1 on the
// ground and drops trail marks; it holds the route while it lives (ADR-106).
void CreateOrderMarker(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, RouteHandle _route);

// R-UNIT-185: one advance of an order marker; true when it asks to be removed.
[[nodiscard]] bool AdvanceOrderMarker(RegionSimulation& _region, Effect& _marker);

// R-UNIT-186: one advance of an order trail mark; true once its life has passed since its creation tick, whether or not
// it advanced in that tick (DEC-455).
[[nodiscard]] bool AdvanceTrailMark(const RegionSimulation& _region, const Effect& _mark) noexcept;

} // namespace Species
