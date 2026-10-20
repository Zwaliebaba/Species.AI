#include "pch.h"

#include "MarkerTimings.h"
#include "PlatformClock.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <map>
#include <string_view>
#include <utility>

namespace Neuron
{

namespace
{

// One marker name of one thread's table. Only the owning thread writes it while timing runs; TakeMarkerTimings reads
// and clears it between runs, so relaxed atomics are enough.
struct MarkerSlot
{
  std::atomic<const char*> name{nullptr};
  std::atomic<std::uint64_t> count{0};
  std::atomic<std::int64_t> nanoseconds{0};
};

struct ThreadTable
{
  std::array<MarkerSlot, MARKER_TIMING_NAMES> slots;
};

std::atomic<bool> g_timingOn{false};
std::array<ThreadTable, MARKER_TIMING_THREADS> g_tables;
std::atomic<std::size_t> g_tablesTaken{0};
thread_local ThreadTable* g_threadTable = nullptr;

// The calling thread's table, taken at its first marker; none once every table is taken.
ThreadTable* TableOfThisThread() noexcept
{
  if (g_threadTable == nullptr)
  {
    const std::size_t index = g_tablesTaken.fetch_add(1, std::memory_order_relaxed);
    if (index < g_tables.size())
    {
      g_threadTable = &g_tables[index];
    }
  }
  return g_threadTable;
}

} // namespace

void SetMarkerTiming(bool _on) noexcept
{
  g_timingOn.store(_on, std::memory_order_relaxed);
}

bool MarkerTimingOn() noexcept
{
  return g_timingOn.load(std::memory_order_relaxed);
}

std::int64_t MarkerClockNanoseconds() noexcept
{
  static const CounterTimeSource COUNTER;
  return COUNTER.Now().count();
}

void RecordMarkerTime(const char* _name, std::int64_t _nanoseconds) noexcept
{
  ThreadTable* const table = TableOfThisThread();
  if (table == nullptr)
  {
    return;
  }
  for (MarkerSlot& slot : table->slots)
  {
    const char* const held = slot.name.load(std::memory_order_relaxed);
    if (held == nullptr)
    {
      slot.name.store(_name, std::memory_order_relaxed);
    }
    else if (held != _name)
    {
      continue;
    }
    slot.count.store(slot.count.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    slot.nanoseconds.store(slot.nanoseconds.load(std::memory_order_relaxed) + _nanoseconds, std::memory_order_relaxed);
    return;
  }
}

std::vector<MarkerTiming> TakeMarkerTimings()
{
  // One literal may have one address in each translation unit, so the names are merged by their text.
  std::map<std::string_view, MarkerTiming> merged;
  const std::size_t used = std::min(g_tablesTaken.load(std::memory_order_relaxed), g_tables.size());
  for (std::size_t index = 0; index < used; ++index)
  {
    for (MarkerSlot& slot : g_tables[index].slots)
    {
      const char* const name = slot.name.load(std::memory_order_relaxed);
      if (name == nullptr)
      {
        break;
      }
      MarkerTiming& timing = merged[name];
      timing.name = name;
      timing.count += slot.count.exchange(0, std::memory_order_relaxed);
      timing.nanoseconds += slot.nanoseconds.exchange(0, std::memory_order_relaxed);
    }
  }
  std::vector<MarkerTiming> timings;
  timings.reserve(merged.size());
  for (auto& [name, timing] : merged)
  {
    timings.push_back(std::move(timing));
  }
  return timings;
}

} // namespace Neuron
