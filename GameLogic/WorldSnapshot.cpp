#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "WorldSnapshot.h"

namespace Species
{

Neuron::SnapshotContents SnapshotOf(const WorldSimulation& _world)
{
  Neuron::BitWriter state;
  state.WriteBits(_world.SimulationKey().low, 64);
  state.WriteBits(_world.SimulationKey().high, 64);
  state.WriteBits(_world.RegionCount(), 16);
  _world.WriteState(state);
  for (std::size_t index = 0; index < _world.RegionCount(); ++index)
  {
    _world.Region(index).WriteState(state);
  }
  return Neuron::SnapshotContents{WORLD_STATE_VERSION, _world.NextTick(), state.Take()};
}

std::expected<std::unique_ptr<WorldSimulation>, Neuron::SnapshotError>
RestoreWorld(const Neuron::SnapshotContents& _snapshot, const TuningMvp& _tuning, const StructureRecords* _structures)
{
  // MVP-095: each older version migrates here to the current one; the regions read their own older layout.
  if (_snapshot.stateVersion < WORLD_STATE_OLDEST_VERSION || _snapshot.stateVersion > WORLD_STATE_VERSION)
  {
    return std::unexpected(Neuron::SnapshotError::StateVersion);
  }
  Neuron::BitReader reader(_snapshot.state);
  WorldSimulation::Desc desc;
  desc.simulationKey.low = reader.ReadBits(64);
  desc.simulationKey.high = reader.ReadBits(64);
  desc.regionCount = static_cast<std::uint16_t>(reader.ReadBits(16));
  desc.nextTick = _snapshot.tick;
  desc.tuning = &_tuning;
  desc.structures = _structures;
  if (reader.Failed() || desc.regionCount == 0)
  {
    return std::unexpected(Neuron::SnapshotError::MalformedState);
  }
  auto world = std::make_unique<WorldSimulation>(desc);
  if (!world->ReadState(reader, _snapshot.stateVersion) || !reader.Finish())
  {
    return std::unexpected(Neuron::SnapshotError::MalformedState);
  }
  return world;
}

} // namespace Species
