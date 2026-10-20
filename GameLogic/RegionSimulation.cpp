#include "pch.h"

#include "Check.h"
#include "RegionSimulation.h"

namespace Species
{

RegionSimulation::RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick)
  : m_index(_index),
    m_ids(static_cast<OwnerId>(REGION_OWNER_ID_FIRST + _index)),
    m_commands(_simulationKey, _nextTick),
    m_nextTick(_nextTick)
{
}

void RegionSimulation::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(m_index, 16);
  _writer.WriteBits(m_nextTick, 64);
  _writer.WriteBits(m_ids.Next(), 64);
  m_commands.WriteState(_writer);
}

bool RegionSimulation::ReadState(Neuron::BitReader& _reader)
{
  const auto index = static_cast<std::uint16_t>(_reader.ReadBits(16));
  const std::uint64_t nextTick = _reader.ReadBits(64);
  const std::uint64_t nextId = _reader.ReadBits(64);
  if (_reader.Failed() || index != m_index || nextTick != m_nextTick || nextId == 0 || nextId > STABLE_ID_COUNTER_MAX + 1)
  {
    return false;
  }
  m_ids = StableIdCounter(m_ids.Owner(), nextId);
  return m_commands.ReadState(_reader);
}

void RegionSimulation::Step(std::uint64_t _tick)
{
  ENGINE_ASSERT(_tick == m_nextTick);
  m_commands.Execute(_tick);
  m_nextTick = _tick + 1;
}

} // namespace Species
