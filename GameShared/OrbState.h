#pragma once

#include <cstdint>

namespace Species
{

// R-UNIT-310: the states of an Essence orb that the MVP builds. M1 has no taker and no territory, so every orb follows
// the reference's timed states and ascends (MVP-023). The values are state, never renumbered. The client reads a
// replicated orb's state to draw its glow (ADR-111).
enum class OrbState : std::uint8_t
{
  Birth,
  Floating,
  Ascent
};

inline constexpr std::uint8_t ORB_STATE_MAX = static_cast<std::uint8_t>(OrbState::Ascent);

} // namespace Species
