#include "pch.h"

#include "BitWriter.h"
#include "StateHash.h"

namespace Species
{

std::uint64_t HashState(std::span<const std::byte> _state) noexcept
{
  return Neuron::SipHash24(STATE_HASH_KEY, _state);
}

WorldHashes HashWorld(const WorldSimulation& _world)
{
  WorldHashes hashes;
  hashes.nextTick = _world.NextTick();
  hashes.owners.reserve(1 + _world.RegionCount());
  {
    Neuron::BitWriter state;
    _world.WriteState(state);
    hashes.owners.push_back({_world.Ids().Owner(), HashState(state.Bytes())});
  }
  for (std::size_t index = 0; index < _world.RegionCount(); ++index)
  {
    Neuron::BitWriter state;
    _world.Region(index).WriteState(state);
    hashes.owners.push_back({_world.Region(index).Owner(), HashState(state.Bytes())});
  }
  Neuron::BitWriter combined;
  for (const OwnerHash& owner : hashes.owners)
  {
    combined.WriteBits(owner.owner, 16);
    combined.WriteBits(owner.hash, 64);
  }
  hashes.world = HashState(combined.Bytes());
  return hashes;
}

} // namespace Species
