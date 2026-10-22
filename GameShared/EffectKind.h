#pragma once

#include <cstdint>

namespace Species
{

// R-CRATE-400: the kinds of the region's effect pool that the MVP builds. Muzzle flashes, particles and the other
// presentation of the weapons are effect events, not pool objects (ADR-105); order markers and their trail marks are
// simulated objects (R-UNIT-187, ADR-106), and so are turret rounds (R-CRATE-450, ADR-133), which the client draws from
// their events as it draws laser bolts. The values are state, never renumbered. The client reads the kind of a
// replicated effect to draw it, so it is a shared type (ADR-111).
enum class EffectKind : std::uint8_t
{
  Grenade,
  ShockwaveRing,
  OrderMarker,
  OrderTrailMark,
  TurretRound
};

inline constexpr std::uint8_t EFFECT_KIND_MAX = static_cast<std::uint8_t>(EffectKind::TurretRound);

} // namespace Species
