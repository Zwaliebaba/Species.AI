#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Neuron
{

/// The time spent in one CPU marker (ADR-068) while marker timing is on, summed over every thread that ended one.
struct MarkerTiming
{
  std::string name;
  std::uint64_t count = 0;      // the scopes that ended
  std::int64_t nanoseconds = 0; // their time, the markers nested in them included
};

// Marker timing (ADR-110): off unless a measurement turns it on, when a marker costs one relaxed load more than its PIX
// event. The performance tool turns it on to read the markers' times in-process; no game program does. Each thread
// keeps its own table, so recording takes no lock; at most MARKER_TIMING_THREADS threads and MARKER_TIMING_NAMES
// names a thread are recorded, and a marker beyond either is not.
inline constexpr std::size_t MARKER_TIMING_THREADS = 64;
inline constexpr std::size_t MARKER_TIMING_NAMES = 64;

void SetMarkerTiming(bool _on) noexcept;
[[nodiscard]] bool MarkerTimingOn() noexcept;
// The performance counter in nanoseconds, the clock of marker timing.
[[nodiscard]] std::int64_t MarkerClockNanoseconds() noexcept;
// A marker's end, with its time; ProfileScope calls it. _name is the marker's string literal.
void RecordMarkerTime(const char* _name, std::int64_t _nanoseconds) noexcept;
// The times recorded since the last call, merged by name and in name order; the record starts again. Called while no
// marker is recording, between the measured runs.
[[nodiscard]] std::vector<MarkerTiming> TakeMarkerTimings();

} // namespace Neuron
