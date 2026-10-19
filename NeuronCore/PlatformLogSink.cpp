#include "pch.h"

#include "Development.h"
#include "PlatformLogSink.h"
#include "TextEncoding.h"

#include <atomic>
#include <mutex>
#include <vector>

namespace Neuron
{

namespace
{

struct EarlyRecord
{
  PlatformLogLevel level;
  std::string message;
};

/// The platform's records before a sink is set, and the sink once it is (PLT-133).
struct PlatformLogState
{
  std::mutex mutex;
  std::atomic<PlatformLogSink*> sink{nullptr};
  std::vector<EarlyRecord> early;
  std::size_t droppedEarly = 0;
};

PlatformLogState& State()
{
  static PlatformLogState g_state;
  return g_state;
}

void WriteToDebugger([[maybe_unused]] std::string_view _message) noexcept
{
#if ENGINE_DEVELOPMENT
  std::wstring line = ToUtf16(_message);
  line.push_back(L'\n');
  OutputDebugStringW(line.c_str());
#endif
}

} // namespace

void SetPlatformLogSink(PlatformLogSink* _sink)
{
  PlatformLogState& state = State();
  const std::lock_guard lock(state.mutex);
  if (_sink != nullptr)
  {
    for (const EarlyRecord& record : state.early)
    {
      _sink->Write(record.level, record.message);
    }
    if (state.droppedEarly > 0)
    {
      _sink->Write(PlatformLogLevel::Warning,
                   std::format("{} platform records before the log sink was set were not kept", state.droppedEarly));
    }
    state.early.clear();
    state.droppedEarly = 0;
  }
  // Stored after the hand-over, so a writer that sees the sink writes after the records it was handed.
  state.sink.store(_sink, std::memory_order_release);
}

PlatformLogSink* CurrentPlatformLogSink() noexcept
{
  return State().sink.load(std::memory_order_acquire);
}

void PlatformLog(PlatformLogLevel _level, std::string_view _message)
{
  PlatformLogState& state = State();
  if (PlatformLogSink* sink = state.sink.load(std::memory_order_acquire))
  {
    sink->Write(_level, _message);
    return;
  }
  const std::lock_guard lock(state.mutex);
  if (PlatformLogSink* sink = state.sink.load(std::memory_order_acquire))
  {
    sink->Write(_level, _message);
    return;
  }
  if (state.early.size() < PLATFORM_EARLY_RECORDS_MAX)
  {
    state.early.push_back(EarlyRecord{_level, std::string{_message}});
  }
  else
  {
    ++state.droppedEarly;
  }
  WriteToDebugger(_message);
}

} // namespace Neuron
