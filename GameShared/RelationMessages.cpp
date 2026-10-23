#include "pch.h"

#include "RelationMessages.h"

#include <initializer_list>

namespace Species
{

namespace
{

// A region's local sites are 0 to 3 (WLD-040), and the value that names none.
constexpr std::uint64_t LOCAL_SITE_MAX = VALLEY_SITE_MAX;
constexpr std::uint8_t NO_SITE = VALLEY_SITE_MAX + 1;
constexpr std::uint8_t NO_AREA = CLAIM_AREA_MAX + 1;

void WritePosition(Neuron::BitWriter& _writer, const QuantizedPosition& _position)
{
  _writer.WriteSigned(_position.x, POSITION_HORIZONTAL_BITS);
  _writer.WriteSigned(_position.y, POSITION_VERTICAL_BITS);
  _writer.WriteSigned(_position.z, POSITION_HORIZONTAL_BITS);
}

QuantizedPosition ReadPosition(Neuron::BitReader& _reader) noexcept
{
  QuantizedPosition position;
  position.x = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_HORIZONTAL_BITS));
  position.y = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_VERTICAL_BITS));
  position.z = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_HORIZONTAL_BITS));
  return position;
}

std::uint8_t ReadSite(Neuron::BitReader& _reader) noexcept
{
  return static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
}

// An amount that is never negative: a weight or the stores.
std::int64_t ReadAmount(Neuron::BitReader& _reader) noexcept
{
  const auto amount = static_cast<std::int64_t>(_reader.ReadBits(64));
  if (amount < 0)
  {
    _reader.Fail();
    return 0;
  }
  return amount;
}

std::uint32_t ReadCount(Neuron::BitReader& _reader) noexcept
{
  return static_cast<std::uint32_t>(_reader.ReadBits(32));
}

// A stable id, which is never 0 (ADR-096).
StableId ReadId(Neuron::BitReader& _reader) noexcept
{
  const StableId id = _reader.ReadBits(64);
  if (id == 0)
  {
    _reader.Fail();
  }
  return id;
}

} // namespace

void MakeClaimCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBounded(area, CLAIM_AREA_MAX);
}

void MakeClaimCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  area = static_cast<std::uint8_t>(_reader.ReadBounded(CLAIM_AREA_MAX));
}

void WithdrawClaimCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(claim, 64);
}

void WithdrawClaimCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  claim = _reader.ReadBits(64);
  if (claim == 0)
  {
    _reader.Fail(); // 0 is never a stable id (ADR-096)
  }
}

void DeclareAttackCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  for (const StableId leader : leaders)
  {
    _writer.WriteBits(leader, 64);
  }
  _writer.WriteBounded(target, LOCAL_SITE_MAX);
  _writer.WriteBits(claim, 64);
}

void DeclareAttackCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  for (StableId& leader : leaders)
  {
    leader = _reader.ReadBits(64);
  }
  target = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  claim = _reader.ReadBits(64);
}

void WithdrawAttackCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
}

void WithdrawAttackCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
}

void SendTributeCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBounded(to, LOCAL_SITE_MAX);
  _writer.WriteBounded(cause, GRIEVANCE_CAUSE_MAX);
  _writer.WriteBits(subject, 64);
  _writer.WriteBits(static_cast<std::uint64_t>(amountMilliRu), 64);
}

void SendTributeCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  to = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  cause = static_cast<std::uint8_t>(_reader.ReadBounded(GRIEVANCE_CAUSE_MAX));
  subject = _reader.ReadBits(64);
  amountMilliRu = static_cast<std::int64_t>(_reader.ReadBits(64));
  if (amountMilliRu < 0)
  {
    _reader.Fail(); // an amount of the stores is never negative
  }
}

void ReadValleyStatusCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
}

void ReadValleyStatusCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
}

void ValleyStatus::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(tick, 64);
  _writer.WriteBits(team, 8);
  _writer.WriteBits(runEndTick, 64);
  _writer.WriteBits(quietTick, 64);
  _writer.WriteBool(runEnded);
  _writer.WriteBounded(renown.size(), VALLEY_SETTLEMENTS_MAX);
  for (const RenownLine& line : renown)
  {
    _writer.WriteBounded(line.site, LOCAL_SITE_MAX);
    _writer.WriteBits(line.defense, 32);
    _writer.WriteBits(line.claims, 32);
    _writer.WriteBits(line.easing, 32);
    _writer.WriteBits(line.structures, 32);
  }
  for (const AreaLine& line : areas)
  {
    _writer.WriteBounded(line.holder.value_or(NO_SITE), NO_SITE);
    _writer.WriteBits(line.claimableTick, 64);
  }
  _writer.WriteBounded(claims.size(), VALLEY_CLAIMS_MAX);
  for (const ClaimLine& line : claims)
  {
    _writer.WriteBits(line.id, 64);
    _writer.WriteBounded(line.claimant, LOCAL_SITE_MAX);
    _writer.WriteBounded(line.area, CLAIM_AREA_MAX);
    _writer.WriteBits(line.madeTick, 64);
    _writer.WriteBounded(static_cast<std::uint64_t>(line.state), CLAIM_STATE_MAX);
  }
  _writer.WriteBounded(grievances.size(), VALLEY_GRIEVANCES_MAX);
  for (const GrievanceLine& line : grievances)
  {
    _writer.WriteBounded(line.holder, LOCAL_SITE_MAX);
    _writer.WriteBounded(line.against, LOCAL_SITE_MAX);
    _writer.WriteBounded(static_cast<std::uint64_t>(line.cause), GRIEVANCE_CAUSE_MAX);
    _writer.WriteBits(line.subject, 64);
    _writer.WriteBool(line.unprovoked);
    _writer.WriteBool(line.causeRemoved);
    _writer.WriteBits(static_cast<std::uint64_t>(line.weightMicro), 64);
  }
  _writer.WriteBounded(expeditions.size(), VALLEY_SETTLEMENTS_MAX);
  for (const ExpeditionLine& line : expeditions)
  {
    _writer.WriteBits(line.id, 64);
    _writer.WriteBounded(line.attacker, LOCAL_SITE_MAX);
    _writer.WriteBounded(line.target, LOCAL_SITE_MAX);
    _writer.WriteBits(line.claim, 64);
    _writer.WriteBits(line.declaredTick, 64);
    _writer.WriteBounded(static_cast<std::uint64_t>(line.state), EXPEDITION_STATE_MAX);
    _writer.WriteBool(line.unprovoked);
    _writer.WriteBits(line.sent, 32);
  }
  _writer.WriteBounded(battles.size(), VALLEY_SETTLEMENTS_MAX);
  for (const BattleLine& line : battles)
  {
    _writer.WriteBits(line.expedition, 64);
    _writer.WriteBounded(line.attacker, LOCAL_SITE_MAX);
    _writer.WriteBounded(line.defender, LOCAL_SITE_MAX);
    _writer.WriteBits(line.claim, 64);
    _writer.WriteBits(line.startTick, 64);
    _writer.WriteBool(line.side);
    WritePosition(_writer, line.objective);
    WritePosition(_writer, line.fieldCenter);
    _writer.WriteBits(line.fieldRadius, SETTLEMENT_RADIUS_BITS);
    _writer.WriteBits(line.attackerLost, 32);
    _writer.WriteBits(line.defenderLost, 32);
    _writer.WriteBits(line.lossCap, 32);
  }
  _writer.WriteBounded(respites.size(), VALLEY_SETTLEMENTS_MAX);
  for (const RespiteLine& line : respites)
  {
    _writer.WriteBounded(line.site, LOCAL_SITE_MAX);
    _writer.WriteBits(line.until, 64);
  }
}

void ValleyStatus::Decode(Neuron::BitReader& _reader)
{
  tick = _reader.ReadBits(64);
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  runEndTick = _reader.ReadBits(64);
  quietTick = _reader.ReadBits(64);
  runEnded = _reader.ReadBool();
  renown.resize(_reader.ReadBounded(VALLEY_SETTLEMENTS_MAX));
  for (RenownLine& line : renown)
  {
    line.site = ReadSite(_reader);
    line.defense = ReadCount(_reader);
    line.claims = ReadCount(_reader);
    line.easing = ReadCount(_reader);
    line.structures = ReadCount(_reader);
  }
  for (AreaLine& line : areas)
  {
    const auto holder = static_cast<std::uint8_t>(_reader.ReadBounded(NO_SITE));
    line.holder = holder != NO_SITE ? std::optional<std::uint8_t>(holder) : std::nullopt;
    line.claimableTick = _reader.ReadBits(64);
  }
  claims.resize(_reader.ReadBounded(VALLEY_CLAIMS_MAX));
  for (ClaimLine& line : claims)
  {
    line.id = ReadId(_reader);
    line.claimant = ReadSite(_reader);
    line.area = static_cast<std::uint8_t>(_reader.ReadBounded(CLAIM_AREA_MAX));
    line.madeTick = _reader.ReadBits(64);
    line.state = static_cast<ClaimState>(_reader.ReadBounded(CLAIM_STATE_MAX));
  }
  grievances.resize(_reader.ReadBounded(VALLEY_GRIEVANCES_MAX));
  for (GrievanceLine& line : grievances)
  {
    line.holder = ReadSite(_reader);
    line.against = ReadSite(_reader);
    line.cause = static_cast<GrievanceCause>(_reader.ReadBounded(GRIEVANCE_CAUSE_MAX));
    line.subject = _reader.ReadBits(64);
    line.unprovoked = _reader.ReadBool();
    line.causeRemoved = _reader.ReadBool();
    line.weightMicro = ReadAmount(_reader);
  }
  expeditions.resize(_reader.ReadBounded(VALLEY_SETTLEMENTS_MAX));
  for (ExpeditionLine& line : expeditions)
  {
    line.id = ReadId(_reader);
    line.attacker = ReadSite(_reader);
    line.target = ReadSite(_reader);
    line.claim = _reader.ReadBits(64);
    line.declaredTick = _reader.ReadBits(64);
    line.state = static_cast<ExpeditionState>(_reader.ReadBounded(EXPEDITION_STATE_MAX));
    line.unprovoked = _reader.ReadBool();
    line.sent = ReadCount(_reader);
  }
  battles.resize(_reader.ReadBounded(VALLEY_SETTLEMENTS_MAX));
  for (BattleLine& line : battles)
  {
    line.expedition = ReadId(_reader);
    line.attacker = ReadSite(_reader);
    line.defender = ReadSite(_reader);
    line.claim = _reader.ReadBits(64);
    line.startTick = _reader.ReadBits(64);
    line.side = _reader.ReadBool();
    line.objective = ReadPosition(_reader);
    line.fieldCenter = ReadPosition(_reader);
    line.fieldRadius = static_cast<std::uint32_t>(_reader.ReadBits(SETTLEMENT_RADIUS_BITS));
    line.attackerLost = ReadCount(_reader);
    line.defenderLost = ReadCount(_reader);
    line.lossCap = ReadCount(_reader);
  }
  respites.resize(_reader.ReadBounded(VALLEY_SETTLEMENTS_MAX));
  for (RespiteLine& line : respites)
  {
    line.site = ReadSite(_reader);
    line.until = _reader.ReadBits(64);
  }
}

void BattleResultNotice::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(expedition, 64);
  _writer.WriteBounded(attacker, LOCAL_SITE_MAX);
  _writer.WriteBounded(defender, LOCAL_SITE_MAX);
  _writer.WriteBounded(area.value_or(NO_AREA), NO_AREA);
  _writer.WriteBounded(static_cast<std::uint64_t>(end), BATTLE_END_MAX);
  _writer.WriteBool(attackerWon);
  _writer.WriteBits(startTick, 64);
  _writer.WriteBits(endTick, 64);
  _writer.WriteBool(areaMoved);
  _writer.WriteBool(side);
  _writer.WriteBits(attackerLost, 32);
  _writer.WriteBits(defenderLost, 32);
  _writer.WriteBits(static_cast<std::uint64_t>(storesMovedMilliRu), 64);
  _writer.WriteBits(essenceMoved, 32);
  _writer.WriteBits(static_cast<std::uint64_t>(grievanceMicro), 64);
  _writer.WriteBits(respiteUntil, 64);
  _writer.WriteBits(renown, 32);
}

void BattleResultNotice::Decode(Neuron::BitReader& _reader)
{
  expedition = ReadId(_reader);
  attacker = ReadSite(_reader);
  defender = ReadSite(_reader);
  const auto claimed = static_cast<std::uint8_t>(_reader.ReadBounded(NO_AREA));
  area = claimed != NO_AREA ? std::optional<std::uint8_t>(claimed) : std::nullopt;
  end = static_cast<BattleEnd>(_reader.ReadBounded(BATTLE_END_MAX));
  attackerWon = _reader.ReadBool();
  startTick = _reader.ReadBits(64);
  endTick = _reader.ReadBits(64);
  areaMoved = _reader.ReadBool();
  side = _reader.ReadBool();
  attackerLost = ReadCount(_reader);
  defenderLost = ReadCount(_reader);
  storesMovedMilliRu = ReadAmount(_reader);
  essenceMoved = ReadCount(_reader);
  grievanceMicro = ReadAmount(_reader);
  respiteUntil = _reader.ReadBits(64);
  renown = ReadCount(_reader);
  if (endTick < startTick || (areaMoved && !area))
  {
    _reader.Fail(); // a battle ends after it starts, and only a claim battle moves an area
  }
}

void ReadEpilogueCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
}

void ReadEpilogueCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
}

void Epilogue::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(tick, 64);
  _writer.WriteBits(team, 8);
  _writer.WriteBits(runEndTick, 64);
  _writer.WriteBool(runEnded);
  _writer.WriteBounded(settlements.size(), VALLEY_SETTLEMENTS_MAX);
  for (const EpilogueLine& line : settlements)
  {
    _writer.WriteBounded(line.site, LOCAL_SITE_MAX);
    for (const std::uint32_t count : {line.renown.defense, line.renown.claims, line.renown.easing, line.renown.structures, line.battlesWon,
                                      line.battlesLost, line.claimsMade, line.claimsWon, line.claimsLost, line.claimsWithdrawn,
                                      line.structures, line.grievances, line.workersMost, line.workersLast})
    {
      _writer.WriteBits(count, 32);
    }
    _writer.WriteBounded(line.strengthMilliRu.size(), EPILOGUE_SAMPLES_MAX);
    for (const std::int64_t strength : line.strengthMilliRu)
    {
      _writer.WriteBits(static_cast<std::uint64_t>(strength), 64);
    }
  }
  _writer.WriteBits(growthSteps, 32);
  _writer.WriteBits(largestGapTicks, 64);
  _writer.WriteBits(firstHalfwayTick, 64);
}

void Epilogue::Decode(Neuron::BitReader& _reader)
{
  tick = _reader.ReadBits(64);
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  runEndTick = _reader.ReadBits(64);
  runEnded = _reader.ReadBool();
  settlements.resize(static_cast<std::size_t>(_reader.ReadBounded(VALLEY_SETTLEMENTS_MAX)));
  for (EpilogueLine& line : settlements)
  {
    line.site = ReadSite(_reader);
    line.renown.site = line.site;
    for (std::uint32_t* count : {&line.renown.defense, &line.renown.claims, &line.renown.easing, &line.renown.structures, &line.battlesWon,
                                 &line.battlesLost, &line.claimsMade, &line.claimsWon, &line.claimsLost, &line.claimsWithdrawn,
                                 &line.structures, &line.grievances, &line.workersMost, &line.workersLast})
    {
      *count = ReadCount(_reader);
    }
    line.strengthMilliRu.resize(static_cast<std::size_t>(_reader.ReadBounded(EPILOGUE_SAMPLES_MAX)));
    for (std::int64_t& strength : line.strengthMilliRu)
    {
      strength = ReadAmount(_reader);
    }
  }
  growthSteps = ReadCount(_reader);
  largestGapTicks = _reader.ReadBits(64);
  firstHalfwayTick = _reader.ReadBits(64);
}

} // namespace Species
