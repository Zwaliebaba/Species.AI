#pragma once

#include "SipHash.h"
#include "StableId.h"
#include "WorldSimulation.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Species
{

// ADR-081 item 8, ADR-096: a state hash is SipHash-2-4 under this public key of zeros, over the bytes an owner's
// WriteState writes. It detects divergence and authenticates nothing, so its key is no secret.
inline constexpr Neuron::SipHashKey STATE_HASH_KEY{};

[[nodiscard]] std::uint64_t HashState(std::span<const std::byte> _state) noexcept;

struct OwnerHash
{
  OwnerId owner = 0;
  std::uint64_t hash = 0;
};

/// The world's hashes at the end of a tick (DBG-147): one per owner, the world owner's first and then each region's in
/// index order, and the world's, a hash over the owners' ids and hashes in that order.
struct WorldHashes
{
  std::uint64_t nextTick = 0;
  std::uint64_t world = 0;
  std::vector<OwnerHash> owners;
};

[[nodiscard]] WorldHashes HashWorld(const WorldSimulation& _world);

} // namespace Species
