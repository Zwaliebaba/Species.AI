#pragma once

#include "Figure.h"
#include "StableId.h"
#include "TerrainField.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace Species
{

// The formation of a Leader (R-UNIT-177 to R-UNIT-184, ADR-106): its slots live in the Leader, each naming its member by
// stable id, and a Worker is in the formation while it holds one of the slots of the Leader it follows and that Leader
// is in formation mode.

// R-UNIT-183: the order is Follow and the formation flag is set.
[[nodiscard]] bool InFormationMode(const Figure& _leader) noexcept;

// R-UNIT-181, R-UNIT-162: at least the team's formation size of slots are occupied. The MVP has no tutorial.
[[nodiscard]] bool FormationFull(const Figure& _leader, const TuningMvp& _tuning) noexcept;

// The index of the slot that _member holds, or none.
[[nodiscard]] std::optional<std::uint32_t> SlotOf(const Figure& _leader, StableId _member) noexcept;

// Table 2.6-A: the row size for a slot array of _arraySize slots, gaps included.
[[nodiscard]] std::uint32_t RowSizeOf(std::size_t _arraySize, const TuningMvp& _tuning) noexcept;

// R-UNIT-182: the position of formation index _index for a Leader at _position with front _front and a slot array of
// _arraySize, on the terrain.
[[nodiscard]] Neuron::Vec3 FormationSlotPosition(const Neuron::Vec3& _position, std::uint32_t _index, const Neuron::Vec3& _front,
                                                 std::size_t _arraySize, const Neuron::TerrainField& _field, const TuningMvp& _tuning);

// R-UNIT-180: the position of the slot _member holds; otherwise, when the formation is not full, a new slot for it in the
// first gap, or after the last slot when there is none, with the slot timer, and its position; the zero vector when the
// formation is full.
[[nodiscard]] Neuron::Vec3 SlotPositionFor(Figure& _leader, StableId _member, const Neuron::TerrainField& _field, const TuningMvp& _tuning);

// R-UNIT-179: one tick added to the timer of the slot _member holds.
void RefreshSlot(Figure& _leader, StableId _member) noexcept;

// R-UNIT-178: every slot's timer counts down by one tick and a slot at or below 0 empties; then, when the runs of gaps
// count more than the gap rule's fraction of the array, every slot empties.
void FormationUpkeep(Figure& _leader, const TuningMvp& _tuning) noexcept;

// R-UNIT-074, R-UNIT-101: _leader, the live Leader that _worker follows or null, still leads it: it is in follow mode,
// and its formation, when it has one, is not full or holds a slot for the Worker.
[[nodiscard]] bool LeaderLeads(const Figure& _worker, const Figure* _leader, const TuningMvp& _tuning) noexcept;

// R-UNIT-074: the Worker becomes Idle and forgets its Leader.
void StopFollowing(Figure& _worker) noexcept;

// R-UNIT-103: whether the member at formation index _index of a slot array of _arraySize fires at tick _tick, by the
// volley rhythm in integer arithmetic (DEC-104, DEC-241).
[[nodiscard]] bool VolleyFires(std::uint64_t _tick, std::uint32_t _index, std::size_t _arraySize, const TuningMvp& _tuning) noexcept;

} // namespace Species
