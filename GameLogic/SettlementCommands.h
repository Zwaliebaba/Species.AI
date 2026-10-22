#pragma once

#include "Settlement.h"
#include "SettlementMessages.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// ADR-136: registers the settlement's commands of M2 task T8 on the region's command queue (ADR-087 item 1). On arrival
// each is permitted when its sender, a steward or an AI, commands the team it names (MVP-094, DEC-122); on execution a
// team with no settlement, or a placement or rebuild that a check refuses, does nothing, and the outcome's reason is
// the PlacementRefusal (MVP-101).
void RegisterSettlementCommands(RegionSimulation& _region);

// MVP-098 (ADR-136): the settlement's status after the tick _tick, as the server sends it to the sender of a
// ReadSettlementStatusCommand; its construction sites are listed in build order, at most STATUS_QUEUE_MAX of them.
[[nodiscard]] SettlementStatus StatusOf(const RegionSimulation& _region, const Settlement& _settlement, std::uint64_t _tick);

} // namespace Species
