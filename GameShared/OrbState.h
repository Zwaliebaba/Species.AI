#pragma once

#include <cstdint>

namespace Species
{

// R-UNIT-310: the states of an Essence orb that the MVP builds. An orb outside every territory follows the reference's
// timed states and ascends; from M2 task T6 one inside a settlement's territory drifts to its Wellspring and may wait
// there (MVP-023, MVP-025, ADR-134). The values are state, never renumbered. The client reads a replicated orb's state
// to draw its glow (ADR-111).
enum class OrbState : std::uint8_t
{
  Birth,
  Floating,
  Ascent,
  Drifting, // MVP-023: to its settlement's Wellspring
  Waiting   // MVP-025: at its Wellspring, for room under the population limit
};

inline constexpr std::uint8_t ORB_STATE_MAX = static_cast<std::uint8_t>(OrbState::Waiting);
// The last state of a snapshot before M2 task T6.
inline constexpr std::uint8_t ORB_STATE_MAX_BEFORE_DRIFT = static_cast<std::uint8_t>(OrbState::Ascent);

} // namespace Species
