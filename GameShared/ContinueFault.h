#pragma once

#include <cstdint>

namespace Species
{

// ADR-155: why a saved world could not be continued, from the newest snapshot its folder holds: the reason the front end
// shows the steward (ADR-137).
enum class ContinueFault : std::uint8_t
{
  NoSnapshot, // the folder holds no snapshot
  Unreadable, // the snapshot cannot be read
  Damaged,    // torn, truncated or changed, not a snapshot, a state that does not decode whole, or a seed with no terrain
  Version     // a format or state version this build does not read (MVP-095)
};

} // namespace Species
