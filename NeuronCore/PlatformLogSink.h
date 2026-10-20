#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace Neuron
{

enum class PlatformLogLevel : std::uint8_t
{
  Debug,
  Info,
  Warning,
  Error
};

/// Where the platform layer writes its records and enters the fatal path (PLT-133). The platform declares it and
/// never names its implementation; the diagnostics component's logger implements it.
class PlatformLogSink
{
public:
  virtual ~PlatformLogSink() = default;

  // Any thread; never blocks on I/O.
  virtual void Write(PlatformLogLevel _level, std::string_view _message) noexcept = 0;
  // Steps 1 and 2 of the fatal path (DBG-073): the black box and a flushed fatal log record. FailFast follows.
  virtual void RecordFatal(std::string_view _reason) noexcept = 0;
};

// The composition root sets the sink at start-up step 7 and clears it with nullptr at shutdown, when no other thread
// writes (PLT-038, PLT-043). Setting it hands over, in order, the records kept since start-up.
void SetPlatformLogSink(PlatformLogSink* _sink);

// The sink the fatal path enters through, or nullptr while none is set (PLT-118).
[[nodiscard]] PlatformLogSink* CurrentPlatformLogSink() noexcept;

// The platform layer's own records (PLT-129). Before a sink is set they are kept in memory in their order, up to
// PLATFORM_EARLY_RECORDS_MAX, and in Debug builds also written to the debugger output (PLT-133).
void PlatformLog(PlatformLogLevel _level, std::string_view _message);

template <class... Args> void PlatformLog(PlatformLogLevel _level, std::format_string<Args...> _format, Args&&... _args)
{
  PlatformLog(_level, std::string_view{std::format(_format, std::forward<Args>(_args)...)});
}

// How many records the platform keeps before a sink is set; later ones are counted and the count is handed over.
inline constexpr std::size_t PLATFORM_EARLY_RECORDS_MAX = 1024;

} // namespace Neuron
