#pragma once

#include <cstdint>
#include <vector>

namespace Performance
{

// DBG T2 (DBG-206): eight threads log 10⁶ debug records between them.
inline constexpr std::uint32_t LOG_FLOOD_THREADS = 8;

/// What a flood of debug records cost the threads that logged them (ADR-091, DBG-206): each thread's time in its calls,
/// and what the logger wrote and dropped.
struct LogFlood
{
  std::uint64_t records = 0; // logged by every thread together
  std::vector<std::int64_t> threadNanoseconds;
  std::uint64_t written = 0;
  std::uint64_t dropped = 0;
};

// About _records debug records, an equal share from each of LOG_FLOOD_THREADS threads started together, into a logger of
// the flood's own with its drain thread and its file in a temporary folder, which is removed afterwards.
[[nodiscard]] LogFlood MeasureLogFlood(std::uint64_t _records);

} // namespace Performance
