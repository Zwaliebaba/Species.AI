#include "pch.h"

#include "SettlementMessages.h"

#include <numeric>

namespace Species
{

namespace
{

constexpr std::uint64_t SHARE_MAX = 100;
constexpr std::uint64_t GROWTH_STATE_MAX = static_cast<std::uint64_t>(GrowthState::StoresShort);
constexpr std::uint64_t PROGRESS_MAX = 1'000;

std::uint8_t ReadTeam(Neuron::BitReader& _reader) noexcept
{
  return static_cast<std::uint8_t>(_reader.ReadBits(8));
}

// An amount of thousandths of a resource unit, which is never negative.
void WriteAmount(Neuron::BitWriter& _writer, std::int64_t _milliRu)
{
  _writer.WriteBits(static_cast<std::uint64_t>(_milliRu), 64);
}

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

} // namespace

void PlaceStructureCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBounded(static_cast<std::uint64_t>(kind), STRUCTURE_KIND_MAX);
  WriteGroundPoint(_writer, position);
}

void PlaceStructureCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  kind = static_cast<StructureKind>(_reader.ReadBounded(STRUCTURE_KIND_MAX));
  position = ReadGroundPoint(_reader);
}

void RebuildTurretCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(turret, 64);
}

void RebuildTurretCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  turret = _reader.ReadBits(64);
  if (turret == 0)
  {
    _reader.Fail(); // 0 is never a stable id (ADR-096)
  }
}

void SetPrioritiesCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  for (const std::uint8_t share : shares)
  {
    _writer.WriteBounded(share, SHARE_MAX);
  }
}

void SetPrioritiesCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  for (std::uint8_t& share : shares)
  {
    share = static_cast<std::uint8_t>(_reader.ReadBounded(SHARE_MAX));
  }
  if (std::accumulate(shares.begin(), shares.end(), 0U) != SHARE_MAX)
  {
    _reader.Fail();
  }
}

void ReadSettlementStatusCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
}

void ReadSettlementStatusCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
}

void SettlementStatus::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(tick, 64);
  _writer.WriteBits(team, 8);
  WriteAmount(_writer, storesMilliRu);
  WriteAmount(_writer, capacityMilliRu);
  _writer.WriteBits(population, 32);
  _writer.WriteBits(populationLimit, 32);
  _writer.WriteBounded(static_cast<std::uint64_t>(growth), GROWTH_STATE_MAX);
  _writer.WriteBits(growthTicks, 32);
  for (const std::uint8_t share : priorities)
  {
    _writer.WriteBounded(share, SHARE_MAX);
  }
  for (const std::uint32_t count : tasks)
  {
    _writer.WriteBits(count, 32);
  }
  for (const std::uint32_t count : idle)
  {
    _writer.WriteBits(count, 32);
  }
  _writer.WriteBits(inactiveStructures, 32);
  WriteAmount(_writer, upkeepMilliRuPerHour);
  WriteAmount(_writer, strengthMilliRu);
  _writer.WriteBounded(queue.size(), STATUS_QUEUE_MAX);
  for (const QueuedSite& site : queue)
  {
    _writer.WriteBits(site.id, 64);
    _writer.WriteBounded(static_cast<std::uint64_t>(site.kind), STRUCTURE_KIND_MAX);
    _writer.WriteBounded(site.progressPermille, PROGRESS_MAX);
  }
}

void SettlementStatus::Decode(Neuron::BitReader& _reader)
{
  tick = _reader.ReadBits(64);
  team = ReadTeam(_reader);
  storesMilliRu = ReadAmount(_reader);
  capacityMilliRu = ReadAmount(_reader);
  population = ReadCount(_reader);
  populationLimit = ReadCount(_reader);
  growth = static_cast<GrowthState>(_reader.ReadBounded(GROWTH_STATE_MAX));
  growthTicks = ReadCount(_reader);
  for (std::uint8_t& share : priorities)
  {
    share = static_cast<std::uint8_t>(_reader.ReadBounded(SHARE_MAX));
  }
  for (std::uint32_t& count : tasks)
  {
    count = ReadCount(_reader);
  }
  for (std::uint32_t& count : idle)
  {
    count = ReadCount(_reader);
  }
  inactiveStructures = ReadCount(_reader);
  upkeepMilliRuPerHour = ReadAmount(_reader);
  strengthMilliRu = ReadAmount(_reader);
  queue.resize(_reader.ReadBounded(STATUS_QUEUE_MAX));
  for (QueuedSite& site : queue)
  {
    site.id = _reader.ReadBits(64);
    site.kind = static_cast<StructureKind>(_reader.ReadBounded(STRUCTURE_KIND_MAX));
    site.progressPermille = static_cast<std::uint16_t>(_reader.ReadBounded(PROGRESS_MAX));
  }
}

} // namespace Species
