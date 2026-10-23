#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "Construction.h"
#include "RegionSimulation.h"
#include "Relations.h"
#include "Renown.h"
#include "SettlementCommands.h"
#include "SettlementTasks.h"
#include "Statistics.h"
#include "Upkeep.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <optional>
#include <vector>

namespace Species
{

namespace
{

constexpr double SECONDS_PER_MINUTE = 60.0;

using Neuron::CommandEffect;
using Neuron::CommandExecution;

CommandExecution Refused(PlacementRefusal _refusal) noexcept
{
  return {CommandEffect::NoEffect, static_cast<std::uint8_t>(_refusal)};
}

// MVP-032: a placement by the team's settlement; a team with no settlement places nothing.
CommandExecution PlaceStructureCommanded(RegionSimulation& _region, const PlaceStructureCommand& _command)
{
  const Settlement* settlement = SettlementOfTeam(_region, _command.team);
  if (settlement == nullptr)
  {
    return Refused(PlacementRefusal::NotBuildable);
  }
  const auto placed = PlaceStructure(_region, settlement->site, _command.kind, _command.position.x, _command.position.z);
  return placed ? CommandExecution{CommandEffect::Applied, 0} : Refused(placed.error());
}

// MVP-034: the rebuild of the team's destroyed turret that the id names, wherever its slot is.
CommandExecution RebuildTurretCommanded(RegionSimulation& _region, const RebuildTurretCommand& _command)
{
  const Settlement* settlement = SettlementOfTeam(_region, _command.team);
  std::optional<std::uint32_t> slot;
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t at = 0; at < structures.Capacity() && !slot; ++at)
  {
    if (structures.Used(at) && structures.At(at).id == _command.turret)
    {
      slot = at;
    }
  }
  if (settlement == nullptr || !slot)
  {
    return Refused(PlacementRefusal::NotBuildable);
  }
  const auto rebuilt = RebuildTurret(_region, settlement->site, slot.value_or(0), _command.turret);
  return rebuilt ? CommandExecution{CommandEffect::Applied, 0} : Refused(rebuilt.error());
}

// MVP-036: the shares, which the task search keeps from the next search on (ADR-135).
CommandEffect SetPrioritiesCommanded(RegionSimulation& _region, const SetPrioritiesCommand& _command)
{
  Settlement* settlement = SettlementOfTeam(_region, _command.team);
  if (settlement == nullptr)
  {
    return CommandEffect::NoEffect;
  }
  settlement->priorities = _command.shares;
  return CommandEffect::Applied;
}

// MVP-053 (ADR-160): a claim by the team's settlement.
CommandExecution MakeClaimCommanded(RegionSimulation& _region, const MakeClaimCommand& _command)
{
  const Settlement* settlement = SettlementOfTeam(_region, _command.team);
  if (settlement == nullptr)
  {
    return {CommandEffect::NoEffect, static_cast<std::uint8_t>(ClaimRefusal::NoSettlement)};
  }
  const auto made = MakeClaim(_region, settlement->site, _command.area);
  return made ? CommandExecution{CommandEffect::Applied, 0}
              : CommandExecution{CommandEffect::NoEffect, static_cast<std::uint8_t>(made.error())};
}

// MVP-053 (ADR-160): the team's settlement withdraws its claim.
CommandExecution WithdrawClaimCommanded(RegionSimulation& _region, const WithdrawClaimCommand& _command)
{
  const Settlement* settlement = SettlementOfTeam(_region, _command.team);
  const std::optional<ClaimRefusal> refusal = settlement != nullptr ? WithdrawClaim(_region, settlement->site, _command.claim)
                                                                    : std::optional<ClaimRefusal>(ClaimRefusal::NoSettlement);
  return refusal ? CommandExecution{CommandEffect::NoEffect, static_cast<std::uint8_t>(*refusal)}
                 : CommandExecution{CommandEffect::Applied, 0};
}

// MVP-054 (ADR-160): the team's settlement sends tribute to ease a grievance held against it.
CommandExecution SendTributeCommanded(RegionSimulation& _region, const SendTributeCommand& _command)
{
  const Settlement* settlement = SettlementOfTeam(_region, _command.team);
  if (settlement == nullptr)
  {
    return {CommandEffect::NoEffect, static_cast<std::uint8_t>(TributeRefusal::NoSettlement)};
  }
  const auto paid = SendTribute(_region, settlement->site, _command.to, static_cast<GrievanceCause>(_command.cause), _command.subject,
                                _command.amountMilliRu);
  return paid ? CommandExecution{CommandEffect::Applied, 0}
              : CommandExecution{CommandEffect::NoEffect, static_cast<std::uint8_t>(paid.error())};
}

// MVP-041 and MVP-042 (ADR-161): the team's settlement declares an attack.
CommandExecution DeclareAttackCommanded(RegionSimulation& _region, const DeclareAttackCommand& _command)
{
  const Settlement* settlement = SettlementOfTeam(_region, _command.team);
  if (settlement == nullptr)
  {
    return {CommandEffect::NoEffect, static_cast<std::uint8_t>(AttackRefusal::NoSettlement)};
  }
  const auto declared = DeclareAttack(_region, settlement->site, _command.leaders, _command.target, _command.claim);
  return declared ? CommandExecution{CommandEffect::Applied, 0}
                  : CommandExecution{CommandEffect::NoEffect, static_cast<std::uint8_t>(declared.error())};
}

// MVP-045 (ADR-161): the team's settlement withdraws its expedition.
CommandExecution WithdrawAttackCommanded(RegionSimulation& _region, const WithdrawAttackCommand& _command)
{
  const Settlement* settlement = SettlementOfTeam(_region, _command.team);
  const std::optional<AttackRefusal> refusal =
    settlement != nullptr ? WithdrawAttack(_region, settlement->site) : std::optional<AttackRefusal>(AttackRefusal::NoSettlement);
  return refusal ? CommandExecution{CommandEffect::NoEffect, static_cast<std::uint8_t>(*refusal)}
                 : CommandExecution{CommandEffect::Applied, 0};
}

// One command type, permitted when its sender commands the team it names, a steward or an AI alike (DEC-122).
template <class T, class R> void Add(RegionSimulation& _region, R (*_execute)(RegionSimulation&, const T&))
{
  _region.Commands().Register<T>(
    [&_region](Neuron::SenderId _sender, const T& _command) { return _region.MayCommand(_sender, _command.team, false); },
    std::function<R(Neuron::SenderId, const T&, std::uint64_t)>([&_region, _execute](Neuron::SenderId, const T& _command, std::uint64_t)
                                                                { return _execute(_region, _command); }));
}

} // namespace

void RegisterSettlementCommands(RegionSimulation& _region)
{
  Add<PlaceStructureCommand>(_region, &PlaceStructureCommanded);
  Add<RebuildTurretCommand>(_region, &RebuildTurretCommanded);
  Add<SetPrioritiesCommand>(_region, &SetPrioritiesCommanded);
  Add<MakeClaimCommand>(_region, &MakeClaimCommanded);
  Add<WithdrawClaimCommand>(_region, &WithdrawClaimCommanded);
  Add<SendTributeCommand>(_region, &SendTributeCommanded);
  Add<DeclareAttackCommand>(_region, &DeclareAttackCommanded);
  Add<WithdrawAttackCommand>(_region, &WithdrawAttackCommanded);
}

SettlementStatus StatusOf(const RegionSimulation& _region, const Settlement& _settlement, std::uint64_t _tick)
{
  const TuningMvp& tuning = _region.Tuning();
  SettlementStatus status;
  status.tick = _tick;
  status.team = _settlement.team;
  status.storesMilliRu = _settlement.storesMilliRu;
  status.capacityMilliRu = StoreCapacityMilliRu(tuning, ActiveCount(_region, _settlement.team, StructureKind::Storehouse));
  status.population = Population(_region, _settlement);
  status.populationLimit = PopulationLimitOf(_region, _settlement);
  // MVP-024: the order of AdvanceSettlement's own conditions.
  if (status.population >= status.populationLimit)
  {
    status.growth = GrowthState::AtLimit;
  }
  else if (_settlement.storesMilliRu < MilliRuOf(tuning.mvp.essence.orbCostRu))
  {
    status.growth = GrowthState::StoresShort;
  }
  status.growthTicks = static_cast<std::uint32_t>(std::max<std::int32_t>(0, _settlement.growthTicks));
  status.priorities = _settlement.priorities;
  status.tasks = CountTasks(_region, _settlement, 0).tasks;
  if (_settlement.team < _region.TeamCount())
  {
    // MVP-037: the reason of each Worker that follows no Leader.
    const SlicedPool<Figure>& figures = _region.TeamAt(_settlement.team).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (!figures.Used(slot))
      {
        continue;
      }
      const Figure& figure = figures.At(slot);
      if (figure.kind == FigureKind::Worker && !figure.dead && figure.leaderId == 0 && figure.idleReason != IdleReason::None)
      {
        ++status.idle[static_cast<std::size_t>(figure.idleReason) - 1];
      }
    }
  }
  status.inactiveStructures = InactiveStructures(_region, _settlement);
  status.upkeepMilliRuPerHour = UpkeepMilliRuPerHour(_region, _settlement);
  status.strengthMilliRu = StrengthMilliRu(_region, _settlement);
  for (const StructureRef& queued : _settlement.queue)
  {
    const Structure* site = _region.ResolveStructure(queued.slot, queued.id);
    if (site == nullptr || status.queue.size() >= STATUS_QUEUE_MAX)
    {
      continue;
    }
    const std::int64_t required = std::max<std::int64_t>(1, RequiredWorkTicks(*site, tuning));
    const std::int64_t permille = std::min<std::int64_t>(1'000, site->workTicks * 1'000 / required);
    status.queue.push_back({site->id, site->kind, static_cast<std::uint16_t>(permille)});
  }
  return status;
}

Epilogue EpilogueOf(const RegionSimulation& _region, const Settlement& _settlement, std::uint64_t _tick, bool _runEnded)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto points = [](std::int64_t _points)
  { return static_cast<std::uint32_t>(std::clamp<std::int64_t>(_points, 0, std::numeric_limits<std::uint32_t>::max())); };
  Epilogue epilogue;
  epilogue.tick = _tick;
  epilogue.team = _settlement.team;
  epilogue.runEndTick = _region.RunEndTick();
  epilogue.runEnded = _runEnded;
  for (const std::uint8_t site : Standing(_region))
  {
    EpilogueLine line;
    line.site = site;
    line.renown.site = site;
    if (const auto renown = std::ranges::find(_region.Renown(), site, &SettlementRenown::site); renown != _region.Renown().end())
    {
      const RenownPoints renowned = RenownOf(tuning, *renown);
      line.renown = {site, points(renowned.defense), points(renowned.claims), points(renowned.easing), points(renowned.structures)};
    }
    // MVP-064 (ADR-163): the statistics are in tick order, so the last entry holds the Workers at the last record.
    std::vector<std::int64_t> samples;
    std::optional<std::uint64_t> lastGrowth;
    for (const StatisticEntry& entry : _region.Statistics())
    {
      if (entry.site != site)
      {
        continue;
      }
      line.workersMost = std::max(line.workersMost, entry.workers);
      line.workersLast = entry.workers;
      switch (entry.kind)
      {
      case StatisticKind::Sample:
        samples.push_back(entry.strengthMilliRu);
        break;
      case StatisticKind::StructureCompleted:
        ++line.structures;
        break;
      case StatisticKind::GrievanceAdded:
        ++line.grievances;
        break;
      case StatisticKind::ClaimMade:
        ++line.claimsMade;
        break;
      case StatisticKind::ClaimWithdrawn:
        ++line.claimsWithdrawn;
        break;
      case StatisticKind::ClaimLost:
        ++line.claimsLost;
        break;
      case StatisticKind::ClaimWon:
        ++line.claimsWon;
        break;
      case StatisticKind::BattleEnded:
        if (entry.detail < _region.BattleResults().size())
        {
          const BattleResult& result = _region.BattleResults()[entry.detail];
          ++((result.attacker == site) == result.attackerWon ? line.battlesWon : line.battlesLost);
        }
        break;
      case StatisticKind::HalfwayReached:
        if (site == _settlement.site && epilogue.firstHalfwayTick == 0)
        {
          epilogue.firstHalfwayTick = entry.tick;
        }
        break;
      case StatisticKind::PopulationStep:
      case StatisticKind::ResourceSiteTaken:
        break;
      }
      // The M2 plan's growth steps of the steward's settlement, and the largest gap between two.
      if (site == _settlement.site && IsGrowthStep(entry.kind))
      {
        ++epilogue.growthSteps;
        if (lastGrowth.has_value())
        {
          epilogue.largestGapTicks = std::max(epilogue.largestGapTicks, entry.tick - *lastGrowth);
        }
        lastGrowth = entry.tick;
      }
    }
    // At most EPILOGUE_SAMPLES_MAX of the samples, evenly taken, the first and the last among them.
    const std::size_t kept = std::min<std::size_t>(samples.size(), EPILOGUE_SAMPLES_MAX);
    for (std::size_t index = 0; index < kept; ++index)
    {
      const std::size_t from = kept > 1 ? index * (samples.size() - 1) / (kept - 1) : 0;
      line.strengthMilliRu.push_back(std::max<std::int64_t>(samples[from], 0));
    }
    epilogue.settlements.push_back(std::move(line));
  }
  return epilogue;
}

ValleyStatus ValleyStatusOf(const RegionSimulation& _region, const Settlement& _settlement, std::uint64_t _tick, bool _runEnded)
{
  const TuningMvp& tuning = _region.Tuning();
  const std::uint8_t site = _settlement.site;
  const auto points = [](std::int64_t _points)
  { return static_cast<std::uint32_t>(std::clamp<std::int64_t>(_points, 0, std::numeric_limits<std::uint32_t>::max())); };
  ValleyStatus status;
  status.tick = _tick;
  status.team = _settlement.team;
  // MVP-060: the run's end and its quiet minutes, as InQuietMinutes reads them; a region of no run has neither.
  status.runEndTick = _region.RunEndTick();
  const auto quiet = static_cast<std::uint64_t>(
    std::max<std::int64_t>(0, std::llround(tuning.mvp.run.quietMinutes * SECONDS_PER_MINUTE / tuning.ref.sim.tickPeriod)));
  status.quietTick = status.runEndTick == std::numeric_limits<std::uint64_t>::max()
                       ? status.runEndTick
                       : status.runEndTick - std::min(quiet, status.runEndTick);
  status.runEnded = _runEnded;
  for (const SettlementRenown& renown : _region.Renown())
  {
    const RenownPoints renowned = RenownOf(tuning, renown);
    status.renown.push_back(
      {renown.site, points(renowned.defense), points(renowned.claims), points(renowned.easing), points(renowned.structures)});
  }
  for (std::uint8_t area = 0; area < VALLEY_AREAS; ++area)
  {
    status.areas[area] = {HolderOf(_region, area), CooldownEndOf(_region, area).value_or(0)};
  }
  for (const Claim& claim : _region.Claims())
  {
    if (status.claims.size() < VALLEY_CLAIMS_MAX)
    {
      status.claims.push_back({claim.id, claim.claimant, claim.area, claim.madeTick, claim.state});
    }
  }
  for (const Grievance& grievance : _region.Grievances())
  {
    if ((grievance.holder == site || grievance.against == site) && status.grievances.size() < VALLEY_GRIEVANCES_MAX)
    {
      status.grievances.push_back({grievance.holder, grievance.against, grievance.cause, grievance.subject, grievance.unprovoked,
                                   grievance.causeRemoved, std::max<std::int64_t>(0, grievance.weightMicro)});
    }
  }
  for (const Expedition& expedition : _region.Expeditions())
  {
    if (expedition.attacker == site || expedition.target == site)
    {
      status.expeditions.push_back({expedition.id, expedition.attacker, expedition.target, expedition.claim, expedition.declaredTick,
                                    expedition.state, expedition.unprovoked, expedition.sent});
    }
  }
  for (const Battle& battle : _region.Battles())
  {
    BattleLine line{battle.expedition, battle.attacker, battle.defender, battle.claim, battle.startTick};
    if (battle.attacker == site || battle.defender == site)
    {
      // WLD-244: the battlefield of a battle the settlement is a side of joins its steward's areas.
      constexpr double RADIUS_LIMIT = static_cast<double>((std::uint32_t{1} << SETTLEMENT_RADIUS_BITS) - 1);
      line.side = true;
      line.objective = QuantizePosition(battle.objective);
      line.fieldCenter = QuantizePosition(battle.fieldCenter);
      line.fieldRadius =
        static_cast<std::uint32_t>(std::clamp(std::round(battle.fieldRadius * POSITION_STEPS_PER_UNIT), 0.0, RADIUS_LIMIT));
      line.attackerLost = battle.attackerLost;
      line.defenderLost = battle.defenderLost;
      line.lossCap = LossCapOf(_region, battle);
    }
    status.battles.push_back(line);
  }
  for (const Respite& respite : _region.Respites())
  {
    status.respites.push_back({respite.site, respite.until});
  }
  return status;
}

BattleResultNotice ResultNoticeOf(const RegionSimulation& _region, const BattleResult& _result, std::optional<std::uint8_t> _area,
                                  bool _side)
{
  BattleResultNotice notice;
  notice.expedition = _result.expedition;
  notice.attacker = _result.attacker;
  notice.defender = _result.defender;
  notice.area = _result.claim != 0 ? _area : std::nullopt;
  notice.end = _result.end;
  notice.attackerWon = _result.attackerWon;
  notice.startTick = _result.startTick;
  notice.endTick = _result.endTick;
  notice.areaMoved = _result.areaMoved && notice.area.has_value();
  notice.side = _side;
  if (_side)
  {
    // MVP-061: a defense held, or a claim battle won, gives its winner renown (ADR-163).
    const auto& renown = _region.Tuning().mvp.renown;
    notice.attackerLost = _result.attackerLost;
    notice.defenderLost = _result.defenderLost;
    notice.storesMovedMilliRu = std::max<std::int64_t>(0, _result.storesMovedMilliRu);
    notice.essenceMoved = _result.essenceMoved;
    notice.grievanceMicro = std::max<std::int64_t>(0, _result.grievanceMicro);
    notice.respiteUntil = _result.respiteUntil;
    const std::int64_t given = !_result.attackerWon ? renown.defenseHeld : _result.claim != 0 ? renown.claimWon : 0;
    notice.renown = static_cast<std::uint32_t>(std::max<std::int64_t>(0, given));
  }
  return notice;
}

} // namespace Species
