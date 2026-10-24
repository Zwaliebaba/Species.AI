#pragma once

#include "AwayMessages.h"
#include "BitReader.h"
#include "BitWriter.h"

#include <cstdint>
#include <vector>

namespace Species
{

/// One freeze of the world (PIL-131): the world tick at which it froze, its cause, and the whole seconds of wall time it
/// lasted as far as the server knows them, 0 when it does not.
struct FreezeRecord
{
  std::uint64_t tick = 0;
  FreezeCause cause = FreezeCause::ServerStall;
  std::uint64_t seconds = 0;

  friend constexpr bool operator==(const FreezeRecord& _a, const FreezeRecord& _b) noexcept = default;
};

/// The world record (ADR-137): what the world keeps of its own history that is no simulation state, because it comes
/// from wall time and a replay does not make it again. A snapshot holds it after the regions, under its check value but
/// outside the state hash: the world's freezes (PIL-131), and the calendar time of the snapshot it was written with,
/// in whole seconds since 1970, 0 when the server knows none.
struct WorldRecord
{
  std::vector<FreezeRecord> freezes;
  std::int64_t savedAtSeconds = 0;

  friend bool operator==(const WorldRecord& _a, const WorldRecord& _b) = default;
};

// A world record of state version 22 and later holds M4's freeze causes, written in two bits (ADR-187); one before it
// knew two causes, written in one.
inline constexpr std::uint32_t STATE_VERSION_WITH_FREEZE_CAUSES = 22;
inline constexpr std::uint64_t FREEZE_CAUSE_MAX_BEFORE_M4 = 1;

void WriteWorldRecord(Neuron::BitWriter& _writer, const WorldRecord& _record);
// The record as a snapshot of _stateVersion holds it; false when it does not read whole.
[[nodiscard]] bool ReadWorldRecord(Neuron::BitReader& _reader, WorldRecord& _record, std::uint32_t _stateVersion);

} // namespace Species
