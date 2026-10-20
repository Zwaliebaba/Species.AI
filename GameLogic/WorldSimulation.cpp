#include "pch.h"

#include "Check.h"
#include "Profile.h"
#include "WorldSimulation.h"

#include <algorithm>

namespace Species
{

WorldSimulation::WorldSimulation(const Desc& _desc)
  : m_key(_desc.simulationKey),
    m_nextTick(_desc.nextTick),
    m_ids(WORLD_OWNER_ID),
    m_commands(_desc.simulationKey, _desc.nextTick)
{
  ENGINE_CHECK(_desc.regionCount > 0);
  m_regions.reserve(_desc.regionCount);
  for (std::uint16_t index = 0; index < _desc.regionCount; ++index)
  {
    m_regions.push_back(std::make_unique<RegionSimulation>(index, _desc.simulationKey, _desc.nextTick));
  }
}

void WorldSimulation::Step(std::uint64_t _tick, Neuron::WorkerPool& _workers)
{
  ENGINE_CHECK(_tick == m_nextTick, "the world steps tick {} after tick {}", _tick, m_nextTick - 1);
  // The regions read no other owner's state, so their jobs give the serial result on any number of workers (R-SIM-732).
  _workers.Run(m_regions.size(),
               [this, _tick](std::size_t _index)
               {
                 ENGINE_PROFILE_SCOPE("sim.region.step");
                 m_regions[_index]->Step(_tick);
               });
  // Then the world owner's own step: its commands, and the messages of ADR-085 item 2 once regions send any.
  m_commands.Execute(_tick);
  m_nextTick = _tick + 1;
}

void WorldSimulation::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(m_nextTick, 64);
  _writer.WriteBits(m_ids.Next(), 64);
  m_commands.WriteState(_writer);
}

bool WorldSimulation::ReadState(Neuron::BitReader& _reader)
{
  const std::uint64_t nextTick = _reader.ReadBits(64);
  const std::uint64_t nextId = _reader.ReadBits(64);
  if (_reader.Failed() || nextTick != m_nextTick || nextId == 0 || nextId > STABLE_ID_COUNTER_MAX + 1)
  {
    return false;
  }
  m_ids = StableIdCounter(WORLD_OWNER_ID, nextId);
  if (!m_commands.ReadState(_reader))
  {
    return false;
  }
  return std::ranges::all_of(m_regions,
                             [&_reader](const std::unique_ptr<RegionSimulation>& _region) { return _region->ReadState(_reader); });
}

RegionSimulation& WorldSimulation::Region(std::size_t _index)
{
  return *m_regions.at(_index);
}

const RegionSimulation& WorldSimulation::Region(std::size_t _index) const
{
  return *m_regions.at(_index);
}

} // namespace Species
