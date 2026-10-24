#pragma once

#include "TuningMvp.h"

#include <cstdint>
#include <optional>
#include <string_view>

namespace Species
{

/// PIL-100, PIL-104: a world's pace, chosen when the world is created and fixed for its life. A saved value that is never
/// renumbered.
enum class WorldPace : std::uint8_t
{
  Session = 0, // the world runs while its steward plays, in the client's process (MVP-070)
  AlwaysOn = 1 // the world runs in the headless server whether or not anyone plays (MVP-071)
};

inline constexpr std::uint64_t WORLD_PACE_MAX = 1;

/// PIL-101 and PIL-103 for the MVP (the scope's §1.3 and table 3): the tuning record as a world of _pace reads it. The
/// record states its values at the session pace, so a session-paced world reads it unchanged. At the always-on pace
/// each value that table 3 marks `scaled` is converted by mvp.pace.session_factor: a duration is multiplied by it, and an
/// amount per event or per hour is divided by it. Every other value, the battle's and the movement's among them, is the
/// same at both paces.
[[nodiscard]] TuningMvp PacedTuning(const TuningMvp& _tuning, WorldPace _pace);

// The pace's name in a world's description (`world.pace`, ADR-155): "session" or "always_on".
[[nodiscard]] std::string_view PaceName(WorldPace _pace) noexcept;
// The pace a name gives; none for a name that is no pace's.
[[nodiscard]] std::optional<WorldPace> PaceNamed(std::string_view _name) noexcept;

} // namespace Species
