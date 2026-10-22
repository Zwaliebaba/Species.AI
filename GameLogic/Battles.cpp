#include "pch.h"

#include "Battles.h"
#include "Claims.h"
#include "Construction.h"
#include "Formation.h"
#include "LeaderOrders.h"
#include "RegionSimulation.h"
#include "Relations.h"
#include "SettlementTasks.h"
#include "StateValues.h"
#include "Territory.h"
#include "Upkeep.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>
#include <vector>

namespace Species
{

namespace
{

constexpr double SECONDS_PER_MINUTE = 60.0;
// A region's local sites are 0 to 3 (WLD-040).
constexpr std::uint64_t LOCAL_SITE_MAX = 3;

std::uint64_t TicksOf(const TuningMvp& _tuning, double _seconds) noexcept
{
  return static_cast<std::uint64_t>(std::max<std::int64_t>(0, std::llround(_seconds / _tuning.ref.sim.tickPeriod)));
}

Settlement* MutableSettlementAt(RegionSimulation& _region, std::uint8_t _site) noexcept
{
  const auto found = std::ranges::find(_region.Settlements(), _site, &Settlement::site);
  return found != _region.Settlements().end() ? &*found : nullptr;
}

// The living Workers and figures of a team.
struct Living
{
  std::uint32_t workers = 0;
  std::uint32_t figures = 0;
};

Living LivingOf(const RegionSimulation& _region, std::uint8_t _team)
{
  Living living;
  if (_team >= _region.TeamCount())
  {
    return living;
  }
  const SlicedPool<Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot) && !figures.At(slot).dead)
    {
      ++living.figures;
      living.workers += figures.At(slot).kind == FigureKind::Worker ? 1U : 0U;
    }
  }
  return living;
}

// The living figures of _team, or only its Workers, within _radius of _point, across the ground.
std::uint32_t LivingNear(const RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _point, double _radius, bool _workersOnly)
{
  if (_team >= _region.TeamCount())
  {
    return 0;
  }
  std::uint32_t near = 0;
  const SlicedPool<Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    const bool counted = figures.Used(slot) && !figures.At(slot).dead && (!_workersOnly || figures.At(slot).kind == FigureKind::Worker);
    near += counted && Neuron::HorizontalDistance(figures.At(slot).position, _point) <= _radius ? 1U : 0U;
  }
  return near;
}

const Claim* ClaimNamed(const RegionSimulation& _region, StableId _claim) noexcept
{
  const auto found = std::ranges::find(_region.Claims(), _claim, &Claim::id);
  return found != _region.Claims().end() ? &*found : nullptr;
}

// The claimed area's resource site: the one in the half, or the central resource site.
const Structure* SiteOfArea(const RegionSimulation& _region, std::uint8_t _area)
{
  const SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot))
    {
      continue;
    }
    const Structure& site = structures.At(slot);
    if (_area == CLAIM_AREA_CENTRAL
          ? site.kind == StructureKind::ResourceSiteCentral
          : site.kind == StructureKind::ResourceSite && InBorderHalf(_region, _area, site.position.x, site.position.z))
    {
      return &site;
    }
  }
  return nullptr;
}

Neuron::Vec3 WellspringOf(const RegionSimulation& _region, std::uint8_t _site)
{
  const Settlement* settlement = _region.SettlementAt(_site);
  const Structure* wellspring =
    settlement != nullptr ? _region.ResolveStructure(settlement->wellspringSlot, settlement->wellspringId) : nullptr;
  return wellspring != nullptr ? wellspring->position : _region.Terrain()->layout.sites[_site];
}

// MVP-045: the objective of the expedition's battle: the target's Wellspring, or the claimed area's resource site.
Neuron::Vec3 ObjectiveOf(const RegionSimulation& _region, const Expedition& _expedition)
{
  const Claim* claim = ClaimNamed(_region, _expedition.claim);
  const Structure* site = claim != nullptr ? SiteOfArea(_region, claim->area) : nullptr;
  return site != nullptr ? site->position : WellspringOf(_region, _expedition.target);
}

// A point at _distance from _from towards _toward, on the ground.
Neuron::Vec3 Toward(const RegionSimulation& _region, const Neuron::Vec3& _from, const Neuron::Vec3& _toward, double _distance)
{
  const Neuron::Vec3 across{_toward.x - _from.x, 0.0, _toward.z - _from.z};
  const double length = Neuron::Length(across);
  Neuron::Vec3 point = length > 0.0 ? _from + across * (_distance / length) : _from;
  point.y = _region.Terrain()->field.Height(point.x, point.z);
  return point;
}

// MVP-042: where the warning holds the expedition, outside the target's territory, or the claimed area, on the side of
// the attacker's Wellspring: mvp.battle.objective_radius beyond the territory's radius, or two of it from the claimed
// area's resource site and its clearance.
Neuron::Vec3 StagingOf(const RegionSimulation& _region, const Expedition& _expedition)
{
  const TuningMvp& tuning = _region.Tuning();
  const Neuron::Vec3 objective = ObjectiveOf(_region, _expedition);
  const Settlement* target = _region.SettlementAt(_expedition.target);
  const double reach = _expedition.claim == 0 && target != nullptr
                         ? target->territoryRadius + tuning.mvp.battle.objectiveRadius
                         : tuning.wld.resource.wildClearance + 2.0 * tuning.mvp.battle.objectiveRadius;
  return Toward(_region, objective, WellspringOf(_region, _expedition.attacker), reach);
}

// MVP-043: a figure of the expedition stands where its battle starts: in the target's territory, the claimed half, or
// the central resource site's clearance.
bool Entered(const RegionSimulation& _region, const Expedition& _expedition)
{
  const std::uint8_t team = ExpeditionTeamOf(_expedition.attacker);
  if (team >= _region.TeamCount())
  {
    return false;
  }
  const Claim* claim = ClaimNamed(_region, _expedition.claim);
  const CellRaster* territory = _region.TerritoryOf(_expedition.target);
  const Structure* site = claim != nullptr && claim->area == CLAIM_AREA_CENTRAL ? SiteOfArea(_region, CLAIM_AREA_CENTRAL) : nullptr;
  const SlicedPool<Figure>& figures = _region.TeamAt(team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot) || figures.At(slot).dead)
    {
      continue;
    }
    const Neuron::Vec3& at = figures.At(slot).position;
    if (claim == nullptr  ? territory != nullptr && territory->Holds(at.x, at.z)
        : site != nullptr ? Neuron::HorizontalDistance(at, site->center) <= site->radius + _region.Tuning().wld.resource.wildClearance
                          : InBorderHalf(_region, claim->area, at.x, at.z))
    {
      return true;
    }
  }
  return false;
}

// Each living Leader of the expedition walks in formation to _point (R-UNIT-171's formation move).
void Walk(RegionSimulation& _region, const Expedition& _expedition, std::uint8_t _team, const Neuron::Vec3& _point)
{
  if (_team >= _region.TeamCount())
  {
    return;
  }
  SlicedPool<Figure>& figures = _region.TeamAt(_team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    Figure& leader = figures.At(slot);
    if (leader.kind == FigureKind::Leader && !leader.dead && std::ranges::find(_expedition.leaders, leader.id) != _expedition.leaders.end())
    {
      AssignWaypoint(leader, _point);
      FormUp(_region, {leader.id, slot, _team}, _point);
    }
  }
}

// ADR-161: every figure of team _from that _moves picks moves to team _to; the Workers' Leader slots follow their
// Leaders'.
void MoveFigures(RegionSimulation& _region, std::uint8_t _from, std::uint8_t _to, const std::function<bool(const Figure&)>& _moves)
{
  if (_from >= _region.TeamCount())
  {
    return;
  }
  std::vector<std::uint32_t> slots;
  const SlicedPool<Figure>& from = _region.TeamAt(_from).figures;
  for (std::uint32_t slot = 0; slot < from.Capacity(); ++slot)
  {
    if (from.Used(slot) && _moves(from.At(slot)))
    {
      slots.push_back(slot);
    }
  }
  std::vector<std::pair<StableId, std::uint32_t>> leaders;
  for (const std::uint32_t slot : slots)
  {
    const StableId id = _region.TeamAt(_from).figures.At(slot).id;
    const bool leader = _region.TeamAt(_from).figures.At(slot).kind == FigureKind::Leader;
    const std::uint32_t moved = _region.MoveFigure(_from, slot, _to);
    if (leader)
    {
      leaders.emplace_back(id, moved);
    }
  }
  SlicedPool<Figure>& to = _region.TeamAt(_to).figures;
  for (std::uint32_t slot = 0; slot < to.Capacity(); ++slot)
  {
    if (!to.Used(slot))
    {
      continue;
    }
    Figure& worker = to.At(slot);
    const auto led = std::ranges::find(leaders, worker.leaderId, &std::pair<StableId, std::uint32_t>::first);
    if (led != leaders.end())
    {
      worker.leaderSlot = led->second;
    }
  }
}

// The expedition's figures go home to their settlement's team: its Leaders walk in formation to its Wellspring with the
// Workers that follow them, and the stores a won raid took are shared among its living Workers, who carry them to the
// nearest drop-off point (MVP-046).
void ReturnHome(RegionSimulation& _region, const Expedition& _expedition, std::int64_t _storesMilliRu)
{
  const std::uint8_t team = ExpeditionTeamOf(_expedition.attacker);
  MoveFigures(_region, team, _expedition.attacker, [](const Figure&) { return true; });
  _region.SetCommander(team, 0, false);
  Walk(_region, _expedition, _expedition.attacker, WellspringOf(_region, _expedition.attacker));
  if (_storesMilliRu <= 0)
  {
    return;
  }
  SlicedPool<Figure>& figures = _region.TeamAt(_expedition.attacker).figures;
  std::vector<std::uint32_t> carriers;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    // An empty place among the expedition's Leaders is 0, which a Worker that follows no Leader also names.
    const bool returning = figures.Used(slot) && figures.At(slot).kind == FigureKind::Worker && !figures.At(slot).dead &&
                           figures.At(slot).leaderId != 0 &&
                           std::ranges::find(_expedition.leaders, figures.At(slot).leaderId) != _expedition.leaders.end();
    if (returning)
    {
      carriers.push_back(slot);
    }
  }
  const auto count = static_cast<std::int64_t>(carriers.size());
  for (std::size_t place = 0; place < carriers.size(); ++place)
  {
    Figure& carrier = figures.At(carriers[place]);
    StopFollowing(carrier);
    carrier.carriedMilliRu = _storesMilliRu / count + (place == 0 ? _storesMilliRu % count : 0);
    StartCarrying(_region, carrier, _expedition.attacker);
  }
}

// The expedition ends, its figures home with what they carry.
void EndExpedition(RegionSimulation& _region, StableId _expedition, std::int64_t _storesMilliRu)
{
  const auto found = std::ranges::find(_region.Expeditions(), _expedition, &Expedition::id);
  if (found == _region.Expeditions().end())
  {
    return;
  }
  const Expedition expedition = *found;
  _region.Expeditions().erase(found);
  RemoveCause(_region, GrievanceCause::Attack, expedition.id);
  ReturnHome(_region, expedition, _storesMilliRu);
}

// MVP-045 to MVP-048: the battle ends: a won raid takes the capped stores, a claim battle settles its claim, the battle
// relieves the attacker's grievances and starts the defender's respite, and its result is recorded.
void EndBattle(RegionSimulation& _region, std::size_t _index, BattleEnd _end, std::uint64_t _tick)
{
  const TuningMvp& tuning = _region.Tuning();
  const Battle battle = _region.Battles()[_index];
  _region.Battles().erase(_region.Battles().begin() + static_cast<std::ptrdiff_t>(_index));
  _region.ForgetCachedEnemies();
  const auto found = std::ranges::find(_region.Expeditions(), battle.expedition, &Expedition::id);
  const std::int64_t grievance = found != _region.Expeditions().end() ? found->grievanceMicro : 0;
  BattleResult result;
  result.expedition = battle.expedition;
  result.attacker = battle.attacker;
  result.defender = battle.defender;
  result.claim = battle.claim;
  result.end = _end;
  result.attackerWon = _end == BattleEnd::ObjectiveHeld || _end == BattleEnd::LossCap;
  result.startTick = battle.startTick;
  result.endTick = _tick;
  result.attackerLost = battle.attackerLost;
  result.defenderLost = battle.defenderLost;
  result.essenceMoved = battle.essenceMoved;
  result.grievanceMicro = grievance;
  Settlement* defender = MutableSettlementAt(_region, battle.defender);
  if (result.attackerWon && battle.claim == 0 && defender != nullptr)
  {
    // MVP-046: the capped share of the stores the defender had at the start, of what it still has.
    const auto capped =
      static_cast<std::int64_t>(std::floor(tuning.mvp.protection.lossCapStoreShare * static_cast<double>(battle.defenderStoresMilliRu)));
    result.storesMovedMilliRu = std::clamp<std::int64_t>(capped, 0, defender->storesMilliRu);
    static_cast<void>(Withdraw(*defender, result.storesMovedMilliRu));
  }
  if (battle.claim != 0)
  {
    result.areaMoved = result.attackerWon;
    SettleClaim(_region, battle.claim, result.attackerWon);
  }
  RelieveAfterBattle(_region, battle.attacker, battle.defender, result.attackerWon);
  // MVP-047 and PIL-087: a respite after a loss of at least its share of the defender's strength, unless the battle was
  // shorter than pil.battle.min_respite_minutes.
  if (defender != nullptr && battle.defenderStrengthMilliRu > 0)
  {
    const double lost = static_cast<double>(battle.defenderStrengthMilliRu - StrengthMilliRu(_region, *defender)) /
                        static_cast<double>(battle.defenderStrengthMilliRu);
    if (lost >= tuning.mvp.protection.respiteMinLossShare &&
        _tick - battle.startTick >= TicksOf(tuning, tuning.pil.battle.minRespiteMinutes * SECONDS_PER_MINUTE))
    {
      result.respiteUntil = _tick + TicksOf(tuning, tuning.mvp.protection.respiteMinutes * SECONDS_PER_MINUTE);
      std::erase_if(_region.Respites(), [&](const Respite& _respite) { return _respite.site == battle.defender; });
      _region.Respites().push_back({battle.defender, result.respiteUntil});
    }
  }
  _region.BattleResults().push_back(result);
  EndExpedition(_region, battle.expedition, result.storesMovedMilliRu);
}

} // namespace

bool IsSettlementSide(const RegionSimulation& _region, std::uint8_t _team) noexcept
{
  return SettlementOfSide(_region, _team) != nullptr;
}

const Settlement* SettlementOfSide(const RegionSimulation& _region, std::uint8_t _team) noexcept
{
  for (const Settlement& settlement : _region.Settlements())
  {
    if (settlement.team == _team || ExpeditionTeamOf(settlement.site) == _team)
    {
      return &settlement;
    }
  }
  return nullptr;
}

const Expedition* ExpeditionOf(const RegionSimulation& _region, std::uint8_t _site) noexcept
{
  const auto found = std::ranges::find(_region.Expeditions(), _site, &Expedition::attacker);
  return found != _region.Expeditions().end() ? &*found : nullptr;
}

const Battle* BattleOf(const RegionSimulation& _region, StableId _expedition) noexcept
{
  const auto found = std::ranges::find(_region.Battles(), _expedition, &Battle::expedition);
  return found != _region.Battles().end() ? &*found : nullptr;
}

bool UnderRespite(const RegionSimulation& _region, std::uint8_t _site) noexcept
{
  return std::ranges::any_of(_region.Respites(),
                             [&](const Respite& _respite) { return _respite.site == _site && _respite.until > _region.NextTick(); });
}

std::uint32_t LossCapOf(const RegionSimulation& _region, const Battle& _battle) noexcept
{
  const auto& protection = _region.Tuning().mvp.protection;
  const auto share = static_cast<std::int64_t>(std::floor(protection.lossCapWorkerShare * static_cast<double>(_battle.defenderStart)));
  const std::int64_t aboveFloor = std::max<std::int64_t>(0, std::int64_t{_battle.defenderStart} - protection.workerFloor);
  return static_cast<std::uint32_t>(std::clamp<std::int64_t>(std::min(share, aboveFloor), 0, std::int64_t{_battle.defenderStart}));
}

std::expected<StableId, AttackRefusal> DeclareAttack(RegionSimulation& _region, std::uint8_t _attacker,
                                                     const std::array<StableId, EXPEDITION_LEADERS_MAX>& _leaders, std::uint8_t _target,
                                                     StableId _claim)
{
  const TuningMvp& tuning = _region.Tuning();
  Settlement* home = MutableSettlementAt(_region, _attacker);
  if (home == nullptr || _region.Terrain() == nullptr)
  {
    return std::unexpected(AttackRefusal::NoSettlement);
  }
  if (ExpeditionOf(_region, _attacker) != nullptr)
  {
    return std::unexpected(AttackRefusal::Underway);
  }
  // A claim battle's target is the claimed area's holder, or the other claimant of a contested central resource site.
  std::uint8_t target = _target;
  if (_claim != 0)
  {
    const Claim* claim = ClaimNamed(_region, _claim);
    if (claim == nullptr || claim->claimant != _attacker || claim->state == ClaimState::Waiting)
    {
      return std::unexpected(AttackRefusal::NoTarget);
    }
    const std::optional<std::uint8_t> holder = HolderOf(_region, claim->area);
    const auto rival = std::ranges::find_if(_region.Claims(), [&](const Claim& _other)
                                            { return _other.area == claim->area && _other.claimant != _attacker; });
    target = holder.value_or(rival != _region.Claims().end() ? rival->claimant : _attacker);
  }
  if (_region.SettlementAt(target) == nullptr || target == _attacker)
  {
    return std::unexpected(AttackRefusal::NoTarget);
  }
  if (UnderRespite(_region, target))
  {
    return std::unexpected(AttackRefusal::UnderRespite);
  }
  // Its Leaders: the live, named Leaders of the settlement; its Workers: those that follow them.
  std::array<StableId, EXPEDITION_LEADERS_MAX> leaders{};
  std::size_t named = 0;
  std::uint32_t workers = 0;
  const SlicedPool<Figure>& figures = _region.TeamAt(home->team).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& figure = figures.At(slot);
    if (figure.kind == FigureKind::Leader && !figure.dead && !figure.demoted && std::ranges::find(_leaders, figure.id) != _leaders.end() &&
        named < leaders.size())
    {
      leaders[named++] = figure.id;
    }
  }
  if (named == 0)
  {
    return std::unexpected(AttackRefusal::NoLeader);
  }
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& figure = figures.At(slot);
    workers += figure.kind == FigureKind::Worker && !figure.dead && figure.leaderId != 0 &&
                   std::ranges::find(leaders, figure.leaderId) != leaders.end()
                 ? 1U
                 : 0U;
  }
  if (workers > static_cast<std::uint32_t>(tuning.mvp.expedition.sizeMax))
  {
    return std::unexpected(AttackRefusal::TooLarge);
  }
  // PIL-063: unprovoked with no cause on record, no grievance against the target and no claim on what it holds.
  const bool claimed =
    std::ranges::any_of(_region.Claims(), [&](const Claim& _held)
                        { return _held.claimant == _attacker && HolderOf(_region, _held.area) == std::optional(target); });
  const bool unprovoked = _claim == 0 && AngerMicro(_region, _attacker, target) == 0 && !claimed;
  const std::int64_t cost = MilliRuOf(tuning.mvp.conflict.attackCostRu * (unprovoked ? tuning.pil.conflict.unprovokedCostFactor : 1.0));
  if (!Withdraw(*home, cost))
  {
    return std::unexpected(AttackRefusal::CannotAfford);
  }
  Expedition expedition;
  expedition.id = _region.IssueId();
  expedition.attacker = _attacker;
  expedition.target = target;
  expedition.claim = _claim;
  expedition.declaredTick = _region.NextTick();
  expedition.sent = workers;
  expedition.unprovoked = unprovoked;
  expedition.leaders = leaders;
  const std::int64_t before = AngerMicro(_region, target, _attacker);
  AddGrievance(_region, target, _attacker, GrievanceCause::Attack, expedition.id, unprovoked);
  expedition.grievanceMicro = AngerMicro(_region, target, _attacker) - before;
  // The Leaders and their Workers leave the settlement's team, and its tasks, for the expedition's, which its commander
  // commands.
  const std::uint8_t team = ExpeditionTeamOf(_attacker);
  MoveFigures(_region, home->team, team,
              [&](const Figure& _figure)
              {
                return std::ranges::find(leaders, _figure.kind == FigureKind::Leader ? _figure.id : _figure.leaderId) != leaders.end() &&
                       (_figure.kind == FigureKind::Leader || _figure.leaderId != 0);
              });
  SlicedPool<Figure>& members = _region.TeamAt(team).figures;
  for (std::uint32_t slot = 0; slot < members.Capacity(); ++slot)
  {
    if (members.Used(slot))
    {
      members.At(slot).task = WorkerTask::None;
      members.At(slot).idleReason = IdleReason::None;
    }
  }
  const RegionSimulation::Commander commander = _region.CommanderOf(home->team);
  _region.SetCommander(team, commander.sender, commander.ai);
  _region.Expeditions().push_back(expedition);
  Walk(_region, expedition, team, StagingOf(_region, expedition));
  return expedition.id;
}

std::optional<AttackRefusal> WithdrawAttack(RegionSimulation& _region, std::uint8_t _attacker)
{
  if (_region.SettlementAt(_attacker) == nullptr)
  {
    return AttackRefusal::NoSettlement;
  }
  const Expedition* expedition = ExpeditionOf(_region, _attacker);
  if (expedition == nullptr)
  {
    return AttackRefusal::Underway;
  }
  const auto battle = std::ranges::find(_region.Battles(), expedition->id, &Battle::expedition);
  if (battle != _region.Battles().end())
  {
    EndBattle(_region, static_cast<std::size_t>(battle - _region.Battles().begin()), BattleEnd::Withdrawn, _region.NextTick());
  }
  else
  {
    EndExpedition(_region, expedition->id, 0);
  }
  return std::nullopt;
}

void AdvanceBattles(RegionSimulation& _region, std::uint64_t _tick)
{
  const TuningMvp& tuning = _region.Tuning();
  // An expedition left with no figure, before its battle, ends as a withdrawn attack.
  std::vector<StableId> emptied;
  for (const Expedition& expedition : _region.Expeditions())
  {
    if (expedition.state != ExpeditionState::Fighting && LivingOf(_region, ExpeditionTeamOf(expedition.attacker)).figures == 0)
    {
      emptied.push_back(expedition.id);
    }
  }
  for (const StableId id : emptied)
  {
    EndExpedition(_region, id, 0);
  }
  // MVP-042, MVP-043 and MVP-015: the warning's release, then each released expedition's start, in the order declared.
  const std::uint64_t warning = TicksOf(tuning, tuning.mvp.conflict.warningMinutes * SECONDS_PER_MINUTE);
  for (Expedition& expedition : _region.Expeditions())
  {
    if (expedition.state == ExpeditionState::Held && _tick >= expedition.declaredTick + warning)
    {
      expedition.state = ExpeditionState::Released;
      Walk(_region, expedition, ExpeditionTeamOf(expedition.attacker), ObjectiveOf(_region, expedition));
    }
    if (expedition.state != ExpeditionState::Released ||
        _region.Battles().size() >= static_cast<std::size_t>(std::max<std::int64_t>(0, tuning.wld.scale.battlesPerRegionMax)) ||
        !Entered(_region, expedition))
    {
      continue;
    }
    const Settlement* defender = _region.SettlementAt(expedition.target);
    Battle battle;
    battle.expedition = expedition.id;
    battle.attacker = expedition.attacker;
    battle.defender = expedition.target;
    battle.claim = expedition.claim;
    battle.startTick = _tick;
    battle.objective = ObjectiveOf(_region, expedition);
    const double margin = tuning.wld.view.settlementMargin;
    battle.fieldCenter = expedition.claim == 0 ? WellspringOf(_region, expedition.target) : battle.objective;
    battle.fieldRadius =
      (expedition.claim == 0 && defender != nullptr ? defender->territoryRadius : tuning.wld.territory.maxRadius / 2.0) + margin;
    battle.defenderStart = defender != nullptr ? Population(_region, *defender) : 0;
    battle.defenderStoresMilliRu = defender != nullptr ? defender->storesMilliRu : 0;
    battle.defenderStrengthMilliRu = defender != nullptr ? StrengthMilliRu(_region, *defender) : 0;
    expedition.state = ExpeditionState::Fighting;
    _region.Battles().push_back(battle);
    _region.ForgetCachedEnemies();
  }
  // MVP-045: each battle's objective hold, then its ends in order.
  const auto holdTicks = static_cast<std::int32_t>(TicksOf(tuning, tuning.mvp.battle.objectiveHoldS));
  const std::uint64_t maxTicks = TicksOf(tuning, tuning.pil.battle.maxMinutes * SECONDS_PER_MINUTE);
  for (std::size_t index = 0; index < _region.Battles().size();)
  {
    Battle& battle = _region.Battles()[index];
    const std::uint8_t attackers = ExpeditionTeamOf(battle.attacker);
    const bool held = LivingNear(_region, attackers, battle.objective, tuning.mvp.battle.objectiveRadius, true) > 0 &&
                      LivingNear(_region, battle.defender, battle.objective, tuning.mvp.battle.objectiveRadius, false) == 0;
    battle.heldTicks = held ? battle.heldTicks + 1 : 0;
    const auto expedition = std::ranges::find(_region.Expeditions(), battle.expedition, &Expedition::id);
    const std::uint32_t sent = expedition != _region.Expeditions().end() ? expedition->sent : 0;
    const Living living = LivingOf(_region, attackers);
    const std::uint32_t cap = LossCapOf(_region, battle);
    std::optional<BattleEnd> end;
    if (cap > 0 && battle.defenderLost >= cap)
    {
      end = BattleEnd::LossCap;
    }
    else if (battle.heldTicks >= holdTicks)
    {
      end = BattleEnd::ObjectiveHeld;
    }
    else if (living.figures == 0 || static_cast<double>(living.workers) < tuning.mvp.battle.routShare * static_cast<double>(sent))
    {
      end = BattleEnd::Rout;
    }
    else if (_tick - battle.startTick >= maxTicks)
    {
      end = BattleEnd::Time;
    }
    if (end)
    {
      EndBattle(_region, index, *end, _tick);
    }
    else
    {
      ++index;
    }
  }
}

std::int32_t BattleDamage(const RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, std::int32_t _amount) noexcept
{
  for (const Battle& battle : _region.Battles())
  {
    if (battle.defender != _team)
    {
      continue;
    }
    if (battle.defenderLost >= LossCapOf(_region, battle))
    {
      return 0;
    }
    const CellRaster* territory = _region.TerritoryOf(battle.defender);
    if (territory != nullptr && territory->Holds(_position.x, _position.z))
    {
      return static_cast<std::int32_t>(std::llround(static_cast<double>(_amount) * _region.Tuning().mvp.defense.damageTakenFactor));
    }
  }
  return _amount;
}

bool BattleDeath(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position)
{
  for (Battle& battle : _region.Battles())
  {
    if (battle.defender == _team)
    {
      ++battle.defenderLost;
      return true;
    }
    if (ExpeditionTeamOf(battle.attacker) != _team)
    {
      continue;
    }
    ++battle.attackerLost;
    const CellRaster* territory = _region.TerritoryOf(battle.defender);
    if (territory == nullptr || !territory->Holds(_position.x, _position.z))
    {
      return true;
    }
    // PIL-045: the defender takes the expedition's Essence up to the attacker's own share, mvp.protection.
    // loss_cap_worker_share of the Workers it sent; beyond it the Essence ascends.
    const auto expedition = std::ranges::find(_region.Expeditions(), battle.expedition, &Expedition::id);
    const double sent = expedition != _region.Expeditions().end() ? static_cast<double>(expedition->sent) : 0.0;
    const auto bound = static_cast<std::uint32_t>(std::floor(_region.Tuning().mvp.protection.lossCapWorkerShare * sent));
    if (battle.essenceMoved >= bound)
    {
      return false;
    }
    ++battle.essenceMoved;
    return true;
  }
  return true;
}

void WriteExpedition(Neuron::BitWriter& _writer, const Expedition& _expedition)
{
  _writer.WriteBits(_expedition.id, 64);
  _writer.WriteBounded(_expedition.attacker, LOCAL_SITE_MAX);
  _writer.WriteBounded(_expedition.target, LOCAL_SITE_MAX);
  _writer.WriteBits(_expedition.claim, 64);
  _writer.WriteBits(_expedition.declaredTick, 64);
  _writer.WriteBits(_expedition.sent, 32);
  _writer.WriteBool(_expedition.unprovoked);
  _writer.WriteSigned(_expedition.grievanceMicro, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(_expedition.state), EXPEDITION_STATE_MAX);
  for (const StableId leader : _expedition.leaders)
  {
    _writer.WriteBits(leader, 64);
  }
}

bool ReadExpedition(Neuron::BitReader& _reader, Expedition& _expedition)
{
  _expedition.id = _reader.ReadBits(64);
  _expedition.attacker = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _expedition.target = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _expedition.claim = _reader.ReadBits(64);
  _expedition.declaredTick = _reader.ReadBits(64);
  _expedition.sent = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _expedition.unprovoked = _reader.ReadBool();
  _expedition.grievanceMicro = _reader.ReadSigned(64);
  _expedition.state = static_cast<ExpeditionState>(_reader.ReadBounded(EXPEDITION_STATE_MAX));
  for (StableId& leader : _expedition.leaders)
  {
    leader = _reader.ReadBits(64);
  }
  return !_reader.Failed() && _expedition.id != 0 && _expedition.attacker != _expedition.target && _expedition.leaders[0] != 0;
}

void WriteBattle(Neuron::BitWriter& _writer, const Battle& _battle)
{
  _writer.WriteBits(_battle.expedition, 64);
  _writer.WriteBounded(_battle.attacker, LOCAL_SITE_MAX);
  _writer.WriteBounded(_battle.defender, LOCAL_SITE_MAX);
  _writer.WriteBits(_battle.claim, 64);
  _writer.WriteBits(_battle.startTick, 64);
  WriteVec3(_writer, _battle.objective);
  WriteVec3(_writer, _battle.fieldCenter);
  WriteDouble(_writer, _battle.fieldRadius);
  _writer.WriteBits(_battle.defenderStart, 32);
  _writer.WriteSigned(_battle.defenderStoresMilliRu, 64);
  _writer.WriteSigned(_battle.defenderStrengthMilliRu, 64);
  _writer.WriteBits(_battle.defenderLost, 32);
  _writer.WriteBits(_battle.attackerLost, 32);
  _writer.WriteBits(_battle.essenceMoved, 32);
  _writer.WriteSigned(_battle.heldTicks, 32);
}

bool ReadBattle(Neuron::BitReader& _reader, Battle& _battle)
{
  _battle.expedition = _reader.ReadBits(64);
  _battle.attacker = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _battle.defender = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _battle.claim = _reader.ReadBits(64);
  _battle.startTick = _reader.ReadBits(64);
  bool finite = ReadVec3(_reader, _battle.objective);
  finite = ReadVec3(_reader, _battle.fieldCenter) && finite;
  finite = ReadDouble(_reader, _battle.fieldRadius) && finite;
  _battle.defenderStart = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _battle.defenderStoresMilliRu = _reader.ReadSigned(64);
  _battle.defenderStrengthMilliRu = _reader.ReadSigned(64);
  _battle.defenderLost = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _battle.attackerLost = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _battle.essenceMoved = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _battle.heldTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  return !_reader.Failed() && finite && _battle.attacker != _battle.defender && _battle.heldTicks >= 0;
}

void WriteBattleResult(Neuron::BitWriter& _writer, const BattleResult& _result)
{
  _writer.WriteBits(_result.expedition, 64);
  _writer.WriteBounded(_result.attacker, LOCAL_SITE_MAX);
  _writer.WriteBounded(_result.defender, LOCAL_SITE_MAX);
  _writer.WriteBits(_result.claim, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(_result.end), BATTLE_END_MAX);
  _writer.WriteBool(_result.attackerWon);
  _writer.WriteBits(_result.startTick, 64);
  _writer.WriteBits(_result.endTick, 64);
  _writer.WriteBits(_result.attackerLost, 32);
  _writer.WriteBits(_result.defenderLost, 32);
  _writer.WriteSigned(_result.storesMovedMilliRu, 64);
  _writer.WriteBits(_result.essenceMoved, 32);
  _writer.WriteBool(_result.areaMoved);
  _writer.WriteSigned(_result.grievanceMicro, 64);
  _writer.WriteBits(_result.respiteUntil, 64);
}

bool ReadBattleResult(Neuron::BitReader& _reader, BattleResult& _result)
{
  _result.expedition = _reader.ReadBits(64);
  _result.attacker = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _result.defender = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _result.claim = _reader.ReadBits(64);
  _result.end = static_cast<BattleEnd>(_reader.ReadBounded(BATTLE_END_MAX));
  _result.attackerWon = _reader.ReadBool();
  _result.startTick = _reader.ReadBits(64);
  _result.endTick = _reader.ReadBits(64);
  _result.attackerLost = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _result.defenderLost = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _result.storesMovedMilliRu = _reader.ReadSigned(64);
  _result.essenceMoved = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _result.areaMoved = _reader.ReadBool();
  _result.grievanceMicro = _reader.ReadSigned(64);
  _result.respiteUntil = _reader.ReadBits(64);
  return !_reader.Failed() && _result.attacker != _result.defender && _result.endTick >= _result.startTick &&
         _result.storesMovedMilliRu >= 0;
}

void WriteRespite(Neuron::BitWriter& _writer, const Respite& _respite)
{
  _writer.WriteBounded(_respite.site, LOCAL_SITE_MAX);
  _writer.WriteBits(_respite.until, 64);
}

bool ReadRespite(Neuron::BitReader& _reader, Respite& _respite)
{
  _respite.site = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _respite.until = _reader.ReadBits(64);
  return !_reader.Failed();
}

} // namespace Species
