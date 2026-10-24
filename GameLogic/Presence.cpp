#include "pch.h"

#include "Construction.h"
#include "Presence.h"
#include "RegionSimulation.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <cstddef>

namespace Species
{

namespace
{

constexpr std::uint64_t WORKERS_BOUND = 0xFFFF'FFFF;

} // namespace

void WriteAbsence(Neuron::BitWriter& _writer, const Absence& _absence)
{
  _writer.WriteBits(_absence.team, 8);
  _writer.WriteBits(_absence.fromTick, 64);
  _writer.WriteBool(_absence.toTick.has_value());
  if (_absence.toTick)
  {
    _writer.WriteBits(*_absence.toTick, 64);
  }
  _writer.WriteBits(_absence.workersFrom, 32);
  _writer.WriteBits(static_cast<std::uint64_t>(_absence.storesFromMilliRu), 64);
  _writer.WriteBits(_absence.workersTo, 32);
  _writer.WriteBits(static_cast<std::uint64_t>(_absence.storesToMilliRu), 64);
  _writer.WriteBounded(_absence.actions.size(), DEPUTY_ACTIONS_MAX);
  for (const DeputyAction& action : _absence.actions)
  {
    _writer.WriteBits(action.tick, 64);
    _writer.WriteBounded(static_cast<std::uint64_t>(action.kind), DEPUTY_ACTION_KIND_MAX);
    _writer.WriteBounded(static_cast<std::uint64_t>(action.order), STANDING_ORDER_MAX);
    _writer.WriteBits(action.leader, 64);
    _writer.WriteBits(action.detail, 32);
  }
  _writer.WriteBits(_absence.actionsDropped, 32);
  _writer.WriteBounded(_absence.grievances.size(), ABSENCE_GRIEVANCES_MAX);
  for (const AbsenceGrievance& grievance : _absence.grievances)
  {
    _writer.WriteBits(grievance.tick, 64);
    _writer.WriteBits(grievance.holder, 8);
    _writer.WriteBits(grievance.against, 8);
    _writer.WriteBounded(static_cast<std::uint64_t>(grievance.cause), GRIEVANCE_CAUSE_MAX);
  }
  _writer.WriteBits(_absence.grievancesDropped, 32);
}

bool ReadAbsence(Neuron::BitReader& _reader, Absence& _absence, std::uint32_t _stateVersion)
{
  Absence absence;
  absence.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  absence.fromTick = _reader.ReadBits(64);
  if (_reader.ReadBool())
  {
    absence.toTick = _reader.ReadBits(64);
  }
  absence.workersFrom = static_cast<std::uint32_t>(_reader.ReadBits(32));
  absence.storesFromMilliRu = static_cast<std::int64_t>(_reader.ReadBits(64));
  absence.workersTo = static_cast<std::uint32_t>(_reader.ReadBits(32));
  absence.storesToMilliRu = static_cast<std::int64_t>(_reader.ReadBits(64));
  absence.actions.resize(_reader.ReadBounded(DEPUTY_ACTIONS_MAX));
  std::uint64_t previous = absence.fromTick;
  for (DeputyAction& action : absence.actions)
  {
    action.tick = _reader.ReadBits(64);
    const std::uint64_t kind = _reader.ReadBounded(DEPUTY_ACTION_KIND_MAX);
    const std::uint64_t order = _reader.ReadBounded(STANDING_ORDER_MAX);
    action.leader = _reader.ReadBits(64);
    action.detail = static_cast<std::uint32_t>(_reader.ReadBits(32));
    // The kinds and orders that exist; actions in the order they were taken, inside the absence.
    if (kind > static_cast<std::uint64_t>(DeputyActionKind::LeaderToObjective) ||
        order > static_cast<std::uint64_t>(StandingOrder::TheStewardsAttack) || action.tick < previous ||
        (absence.toTick && action.tick > *absence.toTick))
    {
      return false;
    }
    action.kind = static_cast<DeputyActionKind>(kind);
    action.order = static_cast<StandingOrder>(order);
    previous = action.tick;
  }
  absence.actionsDropped = static_cast<std::uint32_t>(_reader.ReadBits(32));
  // ADR-186: from version 21 its grievances, in the order they were recorded, inside the absence.
  if (_stateVersion >= STATE_VERSION_WITH_ABSENCE_GRIEVANCES)
  {
    absence.grievances.resize(_reader.ReadBounded(ABSENCE_GRIEVANCES_MAX));
    previous = absence.fromTick;
    for (AbsenceGrievance& grievance : absence.grievances)
    {
      grievance.tick = _reader.ReadBits(64);
      grievance.holder = static_cast<std::uint8_t>(_reader.ReadBits(8));
      grievance.against = static_cast<std::uint8_t>(_reader.ReadBits(8));
      grievance.cause = static_cast<GrievanceCause>(_reader.ReadBounded(GRIEVANCE_CAUSE_MAX));
      if (grievance.tick < previous || (absence.toTick && grievance.tick > *absence.toTick) ||
          (grievance.holder != absence.team && grievance.against != absence.team))
      {
        return false;
      }
      previous = grievance.tick;
    }
    absence.grievancesDropped = static_cast<std::uint32_t>(_reader.ReadBits(32));
  }
  if (_reader.Failed() || absence.storesFromMilliRu < 0 || absence.storesToMilliRu < 0 ||
      (!absence.toTick && (absence.workersTo != 0 || absence.storesToMilliRu != 0)))
  {
    return false;
  }
  _absence = std::move(absence);
  return true;
}

const Absence* OpenAbsence(const RegionSimulation& _region, std::uint8_t _team) noexcept
{
  const auto found = std::ranges::find_if(_region.Absences(), [_team](const Absence& _absence)
                                          { return _absence.team == _team && !_absence.toTick.has_value(); });
  return found == _region.Absences().end() ? nullptr : &*found;
}

Absence* OpenAbsence(RegionSimulation& _region, std::uint8_t _team) noexcept
{
  const auto found = std::ranges::find_if(_region.Absences(), [_team](const Absence& _absence)
                                          { return _absence.team == _team && !_absence.toTick.has_value(); });
  return found == _region.Absences().end() ? nullptr : &*found;
}

const Absence* LatestAbsence(const RegionSimulation& _region, std::uint8_t _team) noexcept
{
  const auto found = std::ranges::find_if(_region.Absences(), [_team](const Absence& _absence)
                                          { return _absence.team == _team && _absence.toTick.has_value(); });
  return found == _region.Absences().end() ? nullptr : &*found;
}

bool SetStewardPresence(WorldSimulation& _world, bool _away)
{
  bool changed = false;
  for (std::size_t index = 0; index < _world.RegionCount(); ++index)
  {
    RegionSimulation& region = _world.Region(index);
    for (const Settlement& settlement : region.Settlements())
    {
      if (!settlement.stewarded)
      {
        continue;
      }
      std::vector<Absence>& absences = region.Absences();
      const auto open = std::ranges::find_if(absences, [&settlement](const Absence& _absence)
                                             { return _absence.team == settlement.team && !_absence.toTick.has_value(); });
      const std::uint32_t workers = std::min<std::uint32_t>(Population(region, settlement), WORKERS_BOUND);
      if (_away && open == absences.end())
      {
        static_cast<void>(region.EnsureDeputy(settlement.team)); // MVP-073 (ADR-185)
        Absence absence;
        absence.team = settlement.team;
        absence.fromTick = region.NextTick();
        absence.workersFrom = workers;
        absence.storesFromMilliRu = settlement.storesMilliRu;
        absences.push_back(std::move(absence));
        changed = true;
      }
      else if (!_away && open != absences.end())
      {
        open->toTick = region.NextTick();
        open->workersTo = workers;
        open->storesToMilliRu = settlement.storesMilliRu;
        Absence closed = std::move(*open);
        // The settlement's previous closed absence gives way to this one (O110: the latest is asked for again).
        std::erase_if(absences, [&settlement](const Absence& _absence) { return _absence.team == settlement.team; });
        absences.push_back(std::move(closed));
        changed = true;
      }
    }
  }
  return changed;
}

bool StewardAway(const WorldSimulation& _world) noexcept
{
  for (std::size_t index = 0; index < _world.RegionCount(); ++index)
  {
    if (std::ranges::any_of(_world.Region(index).Absences(), [](const Absence& _absence) { return !_absence.toTick.has_value(); }))
    {
      return true;
    }
  }
  return false;
}

void RecordAbsenceGrievance(RegionSimulation& _region, const AbsenceGrievance& _grievance)
{
  for (const std::uint8_t team : {_grievance.holder, _grievance.against})
  {
    Absence* absence = OpenAbsence(_region, team);
    if (absence == nullptr || (team == _grievance.against && _grievance.holder == _grievance.against))
    {
      continue;
    }
    if (absence->grievances.size() < ABSENCE_GRIEVANCES_MAX)
    {
      absence->grievances.push_back(_grievance);
    }
    else
    {
      ++absence->grievancesDropped;
    }
  }
}

void RecordDeputyAction(RegionSimulation& _region, std::uint8_t _team, const DeputyAction& _action)
{
  Absence* absence = OpenAbsence(_region, _team);
  if (absence == nullptr)
  {
    return;
  }
  if (absence->actions.size() < DEPUTY_ACTIONS_MAX)
  {
    absence->actions.push_back(_action);
  }
  else
  {
    ++absence->actionsDropped;
  }
}

} // namespace Species
