#include "pch.h"

#include "CommandMessages.h"

#include <bit>
#include <cmath>

namespace Species
{

namespace
{

constexpr std::uint64_t COMMAND_RESULT_MAX = static_cast<std::uint64_t>(CommandResult::Duplicate);
constexpr std::uint64_t LEADER_OPERATION_MAX = static_cast<std::uint64_t>(LeaderOperation::FormUp);

void WriteReal(Neuron::BitWriter& _writer, double _value)
{
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_value), 64);
}

// A binary64 value; one that is not finite fails the reader.
double ReadReal(Neuron::BitReader& _reader) noexcept
{
  const double value = std::bit_cast<double>(_reader.ReadBits(64));
  if (!std::isfinite(value))
  {
    _reader.Fail();
    return 0.0;
  }
  return value;
}

void WritePoint(Neuron::BitWriter& _writer, const GroundPoint& _point)
{
  WriteReal(_writer, _point.x);
  WriteReal(_writer, _point.z);
}

GroundPoint ReadPoint(Neuron::BitReader& _reader) noexcept
{
  GroundPoint point;
  point.x = ReadReal(_reader);
  point.z = ReadReal(_reader);
  return point;
}

// A stable id; 0 is never one (ADR-096), so it fails the reader.
StableId ReadId(Neuron::BitReader& _reader) noexcept
{
  const StableId id = _reader.ReadBits(64);
  if (id == 0)
  {
    _reader.Fail();
  }
  return id;
}

std::uint8_t ReadTeam(Neuron::BitReader& _reader) noexcept
{
  return static_cast<std::uint8_t>(_reader.ReadBits(8));
}

} // namespace

void CommandReport::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(sequence, 64);
  _writer.WriteBits(tick, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(result), COMMAND_RESULT_MAX);
}

void CommandReport::Decode(Neuron::BitReader& _reader)
{
  sequence = _reader.ReadBits(64);
  tick = _reader.ReadBits(64);
  result = static_cast<CommandResult>(_reader.ReadBounded(COMMAND_RESULT_MAX));
}

void SelectCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBool(actor != 0);
  if (actor != 0)
  {
    _writer.WriteBits(actor, 64);
  }
}

void SelectCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  actor = _reader.ReadBool() ? ReadId(_reader) : 0;
}

void PromoteWorkerCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(actor, 64);
}

void PromoteWorkerCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  actor = ReadId(_reader);
}

void DemoteLeaderCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(actor, 64);
}

void DemoteLeaderCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  actor = ReadId(_reader);
}

void OrderUnitCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBool(straight);
  _writer.WriteBool(moveOnly);
  WritePoint(_writer, position);
}

void OrderUnitCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  straight = _reader.ReadBool();
  moveOnly = _reader.ReadBool();
  position = ReadPoint(_reader);
}

void ToggleUnitCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(actor, 64);
}

void ToggleUnitCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  actor = ReadId(_reader);
}

void SelectWorkersCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  WritePoint(_writer, center);
  WriteReal(_writer, radius);
}

void SelectWorkersCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  center = ReadPoint(_reader);
  radius = ReadReal(_reader);
  if (radius < 0.0)
  {
    _reader.Fail();
  }
}

void OrderWorkersCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBool(straight);
  WritePoint(_writer, position);
}

void OrderWorkersCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  straight = _reader.ReadBool();
  position = ReadPoint(_reader);
}

void ControlStateCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  WritePoint(_writer, pointer);
}

void ControlStateCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  pointer = ReadPoint(_reader);
}

void AiOrderWorkerCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(worker, 64);
  WritePoint(_writer, position);
}

void AiOrderWorkerCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  worker = ReadId(_reader);
  position = ReadPoint(_reader);
}

void AiOrderLeaderCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(leader, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(operation), LEADER_OPERATION_MAX);
  _writer.WriteBool(straight);
  WritePoint(_writer, position);
}

void AiOrderLeaderCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  leader = ReadId(_reader);
  operation = static_cast<LeaderOperation>(_reader.ReadBounded(LEADER_OPERATION_MAX));
  straight = _reader.ReadBool();
  position = ReadPoint(_reader);
}

void AiLeaderFieldsCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBits(leader, 64);
  _writer.WriteBool(noFormations);
  _writer.WriteBool(hasFront);
  if (hasFront)
  {
    WritePoint(_writer, front);
  }
}

void AiLeaderFieldsCommand::Decode(Neuron::BitReader& _reader)
{
  team = ReadTeam(_reader);
  leader = ReadId(_reader);
  noFormations = _reader.ReadBool();
  hasFront = _reader.ReadBool();
  front = hasFront ? ReadPoint(_reader) : GroundPoint{};
}

} // namespace Species
