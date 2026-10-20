#pragma once

#include <cstdint>

namespace Species
{

// ADR-083 item 5: every simulated object of a world has a stable 64-bit id, unique in the world for its life: the id of
// the owner that created it in the high 16 bits and that owner's creation counter in the low 48. Messages, snapshots,
// replication, draws and every total order name objects by it. 0 is never an id.
using StableId = std::uint64_t;
using OwnerId = std::uint16_t;

inline constexpr std::uint32_t STABLE_ID_COUNTER_BITS = 48;
inline constexpr std::uint64_t STABLE_ID_COUNTER_MAX = (std::uint64_t{1} << STABLE_ID_COUNTER_BITS) - 1;

// ADR-096: the world owner's id, and the id of region r, REGION_OWNER_ID_FIRST + r.
inline constexpr OwnerId WORLD_OWNER_ID = 1;
inline constexpr OwnerId REGION_OWNER_ID_FIRST = 0x0100;

[[nodiscard]] constexpr StableId MakeStableId(OwnerId _owner, std::uint64_t _counter) noexcept
{
  return (static_cast<StableId>(_owner) << STABLE_ID_COUNTER_BITS) | (_counter & STABLE_ID_COUNTER_MAX);
}

[[nodiscard]] constexpr OwnerId OwnerOf(StableId _id) noexcept
{
  return static_cast<OwnerId>(_id >> STABLE_ID_COUNTER_BITS);
}

[[nodiscard]] constexpr std::uint64_t CounterOf(StableId _id) noexcept
{
  return _id & STABLE_ID_COUNTER_MAX;
}

} // namespace Species
