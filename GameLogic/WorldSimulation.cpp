#include "pch.h"

#include "Battles.h"
#include "Check.h"
#include "Profile.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <cmath>

namespace Species
{

namespace
{

const TuningMvp& RequiredTuning(const TuningMvp* _tuning)
{
  ENGINE_CHECK(_tuning != nullptr);
  return *_tuning;
}

} // namespace

WorldSimulation::WorldSimulation(const Desc& _desc)
  : m_key(_desc.simulationKey),
    m_pace(_desc.pace),
    m_tuning(PacedTuning(RequiredTuning(_desc.tuning), _desc.pace)),
    m_nextTick(_desc.nextTick),
    m_worldSeed(_desc.worldSeed),
    m_ids(WORLD_OWNER_ID),
    m_commands(_desc.simulationKey, _desc.nextTick)
{
  ENGINE_CHECK(_desc.regionCount > 0);
  m_regions.reserve(_desc.regionCount);
  for (std::uint16_t index = 0; index < _desc.regionCount; ++index)
  {
    m_regions.push_back(
      std::make_unique<RegionSimulation>(index, _desc.simulationKey, _desc.nextTick, m_tuning, nullptr, _desc.structures));
  }
  // MVP-060 (ADR-163): a run begins with its world, mvp.run.length_hours at its pace (ADR-183).
  SetRunEnd(_desc.nextTick + RunTicks(m_tuning));
}

std::uint64_t RunTicks(const TuningMvp& _tuning) noexcept
{
  constexpr double SECONDS_PER_HOUR = 3'600.0;
  return static_cast<std::uint64_t>(
    std::max<std::int64_t>(0, std::llround(_tuning.mvp.run.lengthHours * SECONDS_PER_HOUR / _tuning.ref.sim.tickPeriod)));
}

void WorldSimulation::SetRunEnd(std::uint64_t _endTick)
{
  m_run.endTick = _endTick;
  for (const std::unique_ptr<RegionSimulation>& region : m_regions)
  {
    region->SetRunEnd(_endTick);
  }
}

void WorldSimulation::Step(std::uint64_t _tick, Neuron::WorkerPool& _workers)
{
  ENGINE_CHECK(_tick == m_nextTick, "the world steps tick {} after tick {}", _tick, m_nextTick - 1);
  if (m_run.ended)
  {
    // MVP-063: read-only; every owner's tick advances and nothing else, and no queue takes a command (WorldServer).
    for (const std::unique_ptr<RegionSimulation>& region : m_regions)
    {
      region->Pass(_tick);
    }
    m_nextTick = _tick + 1;
    return;
  }
  // The regions read no other owner's state, so their jobs give the serial result on any number of workers (R-SIM-732).
  _workers.Run(m_regions.size(),
               [this, _tick](std::size_t _index)
               {
                 ENGINE_PROFILE_SCOPE("sim.region.step");
                 m_regions[_index]->Step(_tick);
               });
  // Then the world owner's own step: its commands, and the messages of ADR-085 item 2 once regions send any.
  m_commands.Execute(_tick);
  // MVP-060 (ADR-163): from the run's end no expedition starts a battle; the run ends once the battles in progress have
  // finished.
  if (_tick + 1 >= m_run.endTick)
  {
    bool fighting = false;
    for (const std::unique_ptr<RegionSimulation>& region : m_regions)
    {
      EndUnfoughtExpeditions(*region);
      fighting = fighting || !region->Battles().empty();
    }
    if (!fighting)
    {
      m_run.ended = true;
      m_run.endedTick = _tick + 1;
    }
  }
  m_nextTick = _tick + 1;
}

void WorldSimulation::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(m_nextTick, 64);
  _writer.WriteBits(m_worldSeed, 64);
  _writer.WriteBits(m_ids.Next(), 64);
  m_commands.WriteState(_writer);
  _writer.WriteBits(m_run.endTick, 64);
  _writer.WriteBool(m_run.ended);
  _writer.WriteBits(m_run.endedTick, 64);
}

bool WorldSimulation::ReadState(Neuron::BitReader& _reader, std::uint32_t _stateVersion)
{
  const std::uint64_t nextTick = _reader.ReadBits(64);
  // Version 3 added the world seed (M1 task T3); an older world's terrain is built from seed 0.
  const std::uint64_t worldSeed = _stateVersion >= STATE_VERSION_WITH_SEED ? _reader.ReadBits(64) : 0;
  const std::uint64_t nextId = _reader.ReadBits(64);
  if (_reader.Failed() || nextTick != m_nextTick || nextId == 0 || nextId > STABLE_ID_COUNTER_MAX + 1)
  {
    return false;
  }
  m_worldSeed = worldSeed;
  m_ids = StableIdCounter(WORLD_OWNER_ID, nextId);
  if (!m_commands.ReadState(_reader))
  {
    return false;
  }
  // A world before version 15 had no run: its run begins where it continues (ADR-163), as made.
  if (_stateVersion >= STATE_VERSION_WITH_RUN)
  {
    Run run;
    run.endTick = _reader.ReadBits(64);
    run.ended = _reader.ReadBool();
    run.endedTick = _reader.ReadBits(64);
    if (_reader.Failed() || (run.ended && (run.endedTick < run.endTick || run.endedTick > m_nextTick)) ||
        (!run.ended && run.endedTick != 0))
    {
      return false;
    }
    m_run = run;
    SetRunEnd(run.endTick);
  }
  return std::ranges::all_of(m_regions, [&_reader, _stateVersion](const std::unique_ptr<RegionSimulation>& _region)
                             { return _region->ReadState(_reader, _stateVersion); });
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
