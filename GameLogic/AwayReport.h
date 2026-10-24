#pragma once

#include "AwayMessages.h"

#include <cstdint>

namespace Species
{

class WorldSimulation;

/// MVP-074 and PIL-073 (ADR-186): the away report of the settlement of _team at _tick, built by the server from the
/// region's records of its steward's latest closed absence (ADR-184): the battles the settlement was a side of that ended
/// in it, its Workers and stores at either end, the structures it completed, the grievances recorded for or against it,
/// the areas that moved to or from it and the central site's holder, the world's freezes, and every deputy action with
/// its standing order. With no closed absence the report says so and holds nothing else.
[[nodiscard]] AwayReport AwayReportOf(const WorldSimulation& _world, std::uint8_t _team, std::uint64_t _tick, bool _onReturn);

} // namespace Species
