#include "pch.h"

#include "AwayMessages.h"
#include "BitReader.h"
#include "BitWriter.h"

#include <algorithm>

namespace Species
{

namespace
{

// A store amount or change in thousandths of a resource unit, as 64 bits of two's complement.
void WriteAmount(Neuron::BitWriter& _writer, std::int64_t _milliRu)
{
  _writer.WriteBits(static_cast<std::uint64_t>(_milliRu), 64);
}

std::int64_t ReadAmount(Neuron::BitReader& _reader)
{
  return static_cast<std::int64_t>(_reader.ReadBits(64));
}

void WriteSite(Neuron::BitWriter& _writer, std::uint8_t _site)
{
  _writer.WriteBounded(_site, VALLEY_SITE_MAX);
}

std::uint8_t ReadSite(Neuron::BitReader& _reader)
{
  return static_cast<std::uint8_t>(_reader.ReadBounded(VALLEY_SITE_MAX));
}

} // namespace

void PresenceReportCommand::Encode(Neuron::BitWriter& /*_writer*/) const {}

void PresenceReportCommand::Decode(Neuron::BitReader& /*_reader*/) {}

void ReadAwayReportCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
}

void ReadAwayReportCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
}

bool AwayReport::HasNews() const noexcept
{
  return absence && (!battles.empty() || actionsTotal > 0 || !freezes.empty() || grievancesTotal > 0 || !claims.empty() ||
                     std::ranges::any_of(structuresCompleted, [](std::uint32_t _count) { return _count > 0; }));
}

void AwayReport::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(tick, 64);
  _writer.WriteBits(team, 8);
  _writer.WriteBool(absence);
  _writer.WriteBool(onReturn);
  if (!absence)
  {
    return;
  }
  _writer.WriteBits(fromTick, 64);
  _writer.WriteBits(toTick, 64);
  _writer.WriteBits(workersFrom, 32);
  _writer.WriteBits(workersTo, 32);
  WriteAmount(_writer, storesFromMilliRu);
  WriteAmount(_writer, storesToMilliRu);
  for (const std::uint32_t count : structuresCompleted)
  {
    _writer.WriteBits(count, 32);
  }
  _writer.WriteBounded(battles.size(), AWAY_BATTLES_MAX);
  for (const AwayBattleLine& battle : battles)
  {
    WriteSite(_writer, battle.other);
    _writer.WriteBool(battle.defended);
    _writer.WriteBool(battle.won);
    _writer.WriteBounded(static_cast<std::uint64_t>(battle.end), BATTLE_END_MAX);
    _writer.WriteBits(battle.endTick, 64);
    _writer.WriteBits(battle.workersLost, 32);
    WriteAmount(_writer, battle.storesMilliRu);
    _writer.WriteBool(battle.areaMoved);
  }
  _writer.WriteBounded(grievances.size(), AWAY_GRIEVANCES_MAX);
  for (const AwayGrievanceLine& grievance : grievances)
  {
    _writer.WriteBits(grievance.tick, 64);
    WriteSite(_writer, grievance.holder);
    WriteSite(_writer, grievance.against);
    _writer.WriteBounded(static_cast<std::uint64_t>(grievance.cause), GRIEVANCE_CAUSE_MAX);
  }
  _writer.WriteBits(grievancesTotal, 32);
  _writer.WriteBounded(claims.size(), AWAY_CLAIMS_MAX);
  for (const AwayClaimLine& claim : claims)
  {
    _writer.WriteBits(claim.tick, 64);
    _writer.WriteBounded(claim.area, CLAIM_AREA_MAX);
    _writer.WriteBool(claim.from.has_value());
    if (claim.from)
    {
      WriteSite(_writer, *claim.from);
    }
    WriteSite(_writer, claim.to);
  }
  _writer.WriteBounded(freezes.size(), AWAY_FREEZES_MAX);
  for (const AwayFreezeLine& freeze : freezes)
  {
    _writer.WriteBits(freeze.tick, 64);
    _writer.WriteBounded(static_cast<std::uint64_t>(freeze.cause), FREEZE_CAUSE_MAX);
    _writer.WriteBits(freeze.seconds, 64);
  }
  _writer.WriteBounded(actions.size(), AWAY_ACTIONS_MAX);
  for (const AwayActionLine& action : actions)
  {
    _writer.WriteBits(action.tick, 64);
    _writer.WriteBounded(static_cast<std::uint64_t>(action.kind), DEPUTY_ACTION_KIND_MAX);
    _writer.WriteBounded(static_cast<std::uint64_t>(action.order), STANDING_ORDER_MAX);
    _writer.WriteBits(action.leader, 64);
    WriteSite(_writer, action.other);
  }
  _writer.WriteBits(actionsTotal, 32);
}

void AwayReport::Decode(Neuron::BitReader& _reader)
{
  tick = _reader.ReadBits(64);
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  absence = _reader.ReadBool();
  onReturn = _reader.ReadBool();
  if (!absence)
  {
    return;
  }
  fromTick = _reader.ReadBits(64);
  toTick = _reader.ReadBits(64);
  workersFrom = static_cast<std::uint32_t>(_reader.ReadBits(32));
  workersTo = static_cast<std::uint32_t>(_reader.ReadBits(32));
  storesFromMilliRu = ReadAmount(_reader);
  storesToMilliRu = ReadAmount(_reader);
  for (std::uint32_t& count : structuresCompleted)
  {
    count = static_cast<std::uint32_t>(_reader.ReadBits(32));
  }
  battles.resize(static_cast<std::size_t>(_reader.ReadBounded(AWAY_BATTLES_MAX)));
  for (AwayBattleLine& battle : battles)
  {
    battle.other = ReadSite(_reader);
    battle.defended = _reader.ReadBool();
    battle.won = _reader.ReadBool();
    battle.end = static_cast<BattleEnd>(_reader.ReadBounded(BATTLE_END_MAX));
    battle.endTick = _reader.ReadBits(64);
    battle.workersLost = static_cast<std::uint32_t>(_reader.ReadBits(32));
    battle.storesMilliRu = ReadAmount(_reader);
    battle.areaMoved = _reader.ReadBool();
  }
  grievances.resize(static_cast<std::size_t>(_reader.ReadBounded(AWAY_GRIEVANCES_MAX)));
  for (AwayGrievanceLine& grievance : grievances)
  {
    grievance.tick = _reader.ReadBits(64);
    grievance.holder = ReadSite(_reader);
    grievance.against = ReadSite(_reader);
    grievance.cause = static_cast<GrievanceCause>(_reader.ReadBounded(GRIEVANCE_CAUSE_MAX));
  }
  grievancesTotal = static_cast<std::uint32_t>(_reader.ReadBits(32));
  claims.resize(static_cast<std::size_t>(_reader.ReadBounded(AWAY_CLAIMS_MAX)));
  for (AwayClaimLine& claim : claims)
  {
    claim.tick = _reader.ReadBits(64);
    claim.area = static_cast<std::uint8_t>(_reader.ReadBounded(CLAIM_AREA_MAX));
    claim.from = _reader.ReadBool() ? std::optional<std::uint8_t>(ReadSite(_reader)) : std::nullopt;
    claim.to = ReadSite(_reader);
  }
  freezes.resize(static_cast<std::size_t>(_reader.ReadBounded(AWAY_FREEZES_MAX)));
  for (AwayFreezeLine& freeze : freezes)
  {
    freeze.tick = _reader.ReadBits(64);
    freeze.cause = static_cast<FreezeCause>(_reader.ReadBounded(FREEZE_CAUSE_MAX));
    freeze.seconds = _reader.ReadBits(64);
  }
  actions.resize(static_cast<std::size_t>(_reader.ReadBounded(AWAY_ACTIONS_MAX)));
  for (AwayActionLine& action : actions)
  {
    action.tick = _reader.ReadBits(64);
    const std::uint64_t kind = _reader.ReadBounded(DEPUTY_ACTION_KIND_MAX);
    const std::uint64_t order = _reader.ReadBounded(STANDING_ORDER_MAX);
    action.leader = _reader.ReadBits(64);
    action.other = ReadSite(_reader);
    // A kind or an order that does not exist fails the reading.
    if (kind > static_cast<std::uint64_t>(DeputyActionKind::LeaderToObjective) ||
        order > static_cast<std::uint64_t>(StandingOrder::TheStewardsAttack))
    {
      _reader.Fail();
      continue;
    }
    action.kind = static_cast<DeputyActionKind>(kind);
    action.order = static_cast<StandingOrder>(order);
  }
  actionsTotal = static_cast<std::uint32_t>(_reader.ReadBits(32));
}

} // namespace Species
