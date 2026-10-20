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

void RegionSimulation::Step(std::uint64_t _tick)
{
  ENGINE_ASSERT(_tick == m_nextTick);
  m_commands.Execute(_tick);
  m_nextTick = _tick + 1;
}

} // namespace Species
