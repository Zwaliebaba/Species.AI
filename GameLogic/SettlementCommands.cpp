#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "Construction.h"
#include "RegionSimulation.h"
#include "SettlementCommands.h"
#include "SettlementTasks.h"
#include "Upkeep.h"

#include <algorithm>
#include <functional>
#include <optional>

namespace Species
{

namespace
{

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

} // namespace Species
