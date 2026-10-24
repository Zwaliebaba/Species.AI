#include "pch.h"

#include "AwayReport.h"
#include "Presence.h"
#include "RegionSimulation.h"
#include "Statistics.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <cstddef>

namespace Species
{

namespace
{

// The structures a report counts, in the order of AwayReport::structuresCompleted.
constexpr StructureKind COUNTED_KINDS[AWAY_STRUCTURE_KINDS]{StructureKind::Dwelling, StructureKind::Storehouse, StructureKind::Turret};

} // namespace

AwayReport AwayReportOf(const WorldSimulation& _world, std::uint8_t _team, std::uint64_t _tick, bool _onReturn)
{
  AwayReport report;
  report.tick = _tick;
  report.team = _team;
  report.onReturn = _onReturn;
  const RegionSimulation& region = _world.Region(0);
  const Absence* absence = LatestAbsence(region, _team);
  const auto settlement = std::ranges::find(region.Settlements(), _team, &Settlement::team);
  if (absence == nullptr || !absence->toTick || settlement == region.Settlements().end())
  {
    return report;
  }
  const std::uint8_t site = settlement->site;
  const std::uint64_t from = absence->fromTick;
  const std::uint64_t to = *absence->toTick;
  const auto within = [from, to](std::uint64_t _at) { return _at >= from && _at < to; };
  report.absence = true;
  report.fromTick = from;
  report.toTick = to;
  report.workersFrom = absence->workersFrom;
  report.workersTo = absence->workersTo;
  report.storesFromMilliRu = absence->storesFromMilliRu;
  report.storesToMilliRu = absence->storesToMilliRu;
  // MVP-048: each battle it was a side of that ended in the absence, in the order they ended.
  for (const BattleResult& result : region.BattleResults())
  {
    if ((result.attacker != site && result.defender != site) || !within(result.endTick) || report.battles.size() >= AWAY_BATTLES_MAX)
    {
      continue;
    }
    const bool defended = result.defender == site;
    report.battles.push_back({defended ? result.attacker : result.defender, defended, defended != result.attackerWon, result.end,
                              result.endTick, defended ? result.defenderLost : result.attackerLost,
                              defended ? -result.storesMovedMilliRu : result.storesMovedMilliRu, result.areaMoved});
  }
  // MVP-064: the structures it completed, and the central site won by its wait, from the statistics.
  for (const StatisticEntry& entry : region.Statistics())
  {
    if (!within(entry.tick))
    {
      continue;
    }
    if (entry.kind == StatisticKind::StructureCompleted && entry.site == site)
    {
      for (std::size_t kind = 0; kind < AWAY_STRUCTURE_KINDS; ++kind)
      {
        report.structuresCompleted[kind] += entry.detail == static_cast<std::uint32_t>(COUNTED_KINDS[kind]) ? 1 : 0;
      }
    }
    if (entry.kind == StatisticKind::ClaimWon && entry.detail == CLAIM_AREA_CENTRAL && report.claims.size() < AWAY_CLAIMS_MAX)
    {
      report.claims.push_back({entry.tick, CLAIM_AREA_CENTRAL, std::nullopt, entry.site});
    }
  }
  // MVP-053: each half of a border area that moved to or from it.
  for (const ContactEvent& event : region.ContactEvents())
  {
    if (event.kind == ContactEventKind::AreaMoved && within(event.tick) && (event.first == site || event.second == site) &&
        report.claims.size() < AWAY_CLAIMS_MAX)
    {
      report.claims.push_back({event.tick, event.area, event.first, event.second});
    }
  }
  std::ranges::stable_sort(report.claims, {}, &AwayClaimLine::tick);
  // MVP-051: the grievances recorded for or against it (ADR-183).
  for (const AbsenceGrievance& grievance : absence->grievances)
  {
    if (report.grievances.size() < AWAY_GRIEVANCES_MAX)
    {
      report.grievances.push_back({grievance.tick, grievance.holder, grievance.against, grievance.cause});
    }
  }
  report.grievancesTotal = static_cast<std::uint32_t>(absence->grievances.size()) + absence->grievancesDropped;
  // PIL-131: the world's freezes in it, which the world record keeps (ADR-137).
  for (const FreezeRecord& freeze : _world.Record().freezes)
  {
    if (freeze.tick >= from && freeze.tick <= to && report.freezes.size() < AWAY_FREEZES_MAX)
    {
      report.freezes.push_back({freeze.tick, freeze.cause, freeze.seconds});
    }
  }
  // PIL-026: every deputy action with its order.
  for (const DeputyAction& action : absence->actions)
  {
    if (report.actions.size() < AWAY_ACTIONS_MAX)
    {
      report.actions.push_back({action.tick, action.kind, action.order, action.leader, static_cast<std::uint8_t>(action.detail)});
    }
  }
  report.actionsTotal = static_cast<std::uint32_t>(absence->actions.size()) + absence->actionsDropped;
  return report;
}

} // namespace Species
