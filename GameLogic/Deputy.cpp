#include "pch.h"

#include "Battles.h"
#include "CommandMessages.h"
#include "Deputy.h"
#include "Formation.h"
#include "MessageCodec.h"
#include "Presence.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace Species
{

namespace
{

// A Leader whose waypoint stands this close to the objective, horizontally, is already on its way there (wu).
constexpr double ORDERED_SLACK = 1.0;

// A side the deputy's settlement has in a battle in progress: its team, the battle's objective, the other side's site
// and the standing order that permits fighting it.
struct Side
{
  std::uint8_t team = 0;
  Neuron::Vec3 objective;
  std::uint8_t other = 0;
  StandingOrder order = StandingOrder::DefendTheSettlement;
};

std::vector<Side> SidesOf(const RegionSimulation& _region, const Settlement& _settlement)
{
  std::vector<Side> sides;
  // MVP-043: a battle at the settlement or at a border half it holds, which it defends.
  for (const Battle& battle : _region.Battles())
  {
    if (battle.defender == _settlement.site)
    {
      sides.push_back({_settlement.team, battle.objective, battle.attacker, StandingOrder::DefendTheSettlement});
    }
  }
  // O109: the steward's own expedition, once its battle is in progress.
  if (const Expedition* expedition = ExpeditionOf(_region, _settlement.site); expedition != nullptr)
  {
    if (const Battle* battle = BattleOf(_region, expedition->id); battle != nullptr)
    {
      sides.push_back({ExpeditionTeamOf(_settlement.site), battle->objective, battle->defender, StandingOrder::TheStewardsAttack});
    }
  }
  return sides;
}

bool OnItsWay(const Figure& _leader, const Neuron::Vec3& _objective) noexcept
{
  return InFormationMode(_leader) && std::hypot(_leader.waypoint.x - _objective.x, _leader.waypoint.z - _objective.z) <= ORDERED_SLACK;
}

template <class T> void Send(RegionSimulation& _region, Deputy& _deputy, const T& _command)
{
  static_cast<void>(_region.Commands().Submit(_deputy.id, _deputy.nextSequence++, Neuron::EncodeMessage(_command)));
}

} // namespace

const Deputy* DeputyOf(const RegionSimulation& _region, std::uint8_t _team) noexcept
{
  const auto found = std::ranges::find(_region.Deputies(), _team, &Deputy::team);
  return found == _region.Deputies().end() ? nullptr : &*found;
}

bool DeputyMayCommand(const RegionSimulation& _region, Neuron::SenderId _sender, std::uint8_t _team) noexcept
{
  const auto found = std::ranges::find(_region.Deputies(), _sender, &Deputy::id);
  if (_sender == 0 || found == _region.Deputies().end() || OpenAbsence(_region, found->team) == nullptr)
  {
    return false;
  }
  const auto settlement = std::ranges::find(_region.Settlements(), found->team, &Settlement::team);
  return settlement != _region.Settlements().end() && (_team == settlement->team || _team == ExpeditionTeamOf(settlement->site));
}

void StepDeputy(RegionSimulation& _region, Deputy& _deputy)
{
  // PIL-071: while the steward is present the deputy does nothing. It looks every tick of an absence, so that a battle's
  // Leaders are ordered at its first tick: a Leader on its way is not ordered again.
  if (OpenAbsence(_region, _deputy.team) == nullptr)
  {
    return;
  }
  const auto settlement = std::ranges::find(_region.Settlements(), _deputy.team, &Settlement::team);
  if (settlement == _region.Settlements().end())
  {
    return;
  }
  for (const Side& side : SidesOf(_region, *settlement))
  {
    if (side.team >= _region.TeamCount())
    {
      continue;
    }
    // In slot order, so that the deputy's commands and actions are canonical (R-SIM-221).
    const SlicedPool<Figure>& figures = _region.TeamAt(side.team).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (!figures.Used(slot))
      {
        continue;
      }
      const Figure& leader = figures.At(slot);
      if (leader.kind != FigureKind::Leader || leader.dead || leader.demoted || OnItsWay(leader, side.objective))
      {
        continue;
      }
      // MVP-073: the Leader's formation, then its walk, to the objective (R-UNIT-188); the Workers that follow it go with it.
      const GroundPoint objective{side.objective.x, side.objective.z};
      Send(_region, _deputy, AiOrderLeaderCommand{side.team, leader.id, LeaderOperation::FormUp, false, objective});
      Send(_region, _deputy, AiOrderLeaderCommand{side.team, leader.id, LeaderOperation::AssignWaypoint, false, objective});
      RecordDeputyAction(_region, _deputy.team,
                         {_region.NextTick(), DeputyActionKind::LeaderToObjective, side.order, leader.id, side.other});
    }
  }
}

void WriteDeputy(Neuron::BitWriter& _writer, const Deputy& _deputy)
{
  _writer.WriteBits(_deputy.team, 8);
  _writer.WriteBits(_deputy.id, 64);
  _writer.WriteBits(_deputy.nextSequence, 64);
}

bool ReadDeputy(Neuron::BitReader& _reader, Deputy& _deputy)
{
  Deputy deputy;
  deputy.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  deputy.id = _reader.ReadBits(64);
  deputy.nextSequence = _reader.ReadBits(64);
  if (_reader.Failed() || deputy.id == 0)
  {
    return false;
  }
  _deputy = deputy;
  return true;
}

} // namespace Species
