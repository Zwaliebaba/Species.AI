#include "pch.h"

#include "RelationMessages.h"

namespace Species
{

namespace
{

// A region's local sites are 0 to 3 (WLD-040).
constexpr std::uint64_t LOCAL_SITE_MAX = 3;
// GrievanceCause's values, of which the GameLogic owns the meaning (ADR-159).
constexpr std::uint64_t CAUSE_MAX = 2;

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

void SendTributeCommand::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(team, 8);
  _writer.WriteBounded(to, LOCAL_SITE_MAX);
  _writer.WriteBounded(cause, CAUSE_MAX);
  _writer.WriteBits(subject, 64);
  _writer.WriteBits(static_cast<std::uint64_t>(amountMilliRu), 64);
}

void SendTributeCommand::Decode(Neuron::BitReader& _reader)
{
  team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  to = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  cause = static_cast<std::uint8_t>(_reader.ReadBounded(CAUSE_MAX));
  subject = _reader.ReadBits(64);
  amountMilliRu = static_cast<std::int64_t>(_reader.ReadBits(64));
  if (amountMilliRu < 0)
  {
    _reader.Fail(); // an amount of the stores is never negative
  }
}

} // namespace Species
