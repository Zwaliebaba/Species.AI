#pragma once

#include "RelationMessages.h"
#include "Settlement.h"
#include "SettlementMessages.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// ADR-136: registers the settlement's commands of M2 task T8 on the region's command queue (ADR-087 item 1). On arrival
// each is permitted when its sender, a steward or an AI, commands the team it names (MVP-094, DEC-122); on execution a
// team with no settlement, or a placement or rebuild that a check refuses, does nothing, and the outcome's reason is
// the PlacementRefusal (MVP-101). From M3 task T5 (ADR-160) the claims, their withdrawals and tribute are commands
// of the same path, whose refusals give the ClaimRefusal or the TributeRefusal, and from task T6 (ADR-161) the attacks
// and their withdrawals, whose refusals give the AttackRefusal.
void RegisterSettlementCommands(RegionSimulation& _region);

// MVP-098 (ADR-136): the settlement's status after the tick _tick, as the server sends it to the sender of a
// ReadSettlementStatusCommand; its construction sites are listed in build order, at most STATUS_QUEUE_MAX of them.
[[nodiscard]] SettlementStatus StatusOf(const RegionSimulation& _region, const Settlement& _settlement, std::uint64_t _tick);

struct BattleResult;

// MVP-098 and MVP-102 (ADR-165): the valley as the settlement sees it after the tick _tick, as the server sends it to the
// sender of a ReadValleyStatusCommand: every claim, area, battle, respite and settlement's renown, only the grievances held
// by or against the settlement and the expeditions it sent or that are declared on it, and only the figures of a battle
// it is a side of; at most VALLEY_CLAIMS_MAX claims and VALLEY_GRIEVANCES_MAX grievances. _runEnded is the world's.
[[nodiscard]] ValleyStatus ValleyStatusOf(const RegionSimulation& _region, const Settlement& _settlement, std::uint64_t _tick,
                                          bool _runEnded);
// MVP-048 (ADR-165): the result as a steward sees it: whole for one whose settlement was a side (_side), its public line
// for another; _area is the claimed area of a claim battle, which the claim's end no longer holds.
[[nodiscard]] BattleResultNotice ResultNoticeOf(const RegionSimulation& _region, const BattleResult& _result,
                                                std::optional<std::uint8_t> _area, bool _side);

} // namespace Species
