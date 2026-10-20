#pragma once

#include "Development.h"
#include "PlatformLogSink.h"
#include "PlatformThread.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <format>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Neuron
{

class LogBuffer;
class LogFile;
class PlatformClock;
struct LogEntry;

enum class LogLevel : std::uint8_t
{
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Fatal
};

// The nine categories of ADR-067, then Diagnostics and Test (DBG-043).
enum class LogCategory : std::uint8_t
{
  Platform,
  Render,
  Audio,
  Input,
  Ui,
  Content,
  Simulation,
  Network,
  Tools,
  Diagnostics,
  Test
};

inline constexpr std::size_t LOG_LEVEL_COUNT = 6;
inline constexpr std::size_t LOG_CATEGORY_COUNT = 11;

inline constexpr std::size_t LOG_THREAD_BUFFER_KIB = 256; // dbg.log.thread_buffer_kib
inline constexpr std::size_t LOG_FILES_KEPT = 10;         // dbg.log.files_kept
inline constexpr std::size_t LOG_HEX_BYTES_PER_LINE = 16; // dbg.log.hex_bytes_per_line

// What a build can record, and what each category records until it is changed (DBG-064). Debug builds record debug
// and above, with trace enabled per category at run time; Release builds record info and above, and their debug and
// trace calls fold away at compile time.
#if ENGINE_DEVELOPMENT
inline constexpr LogLevel LOG_BUILD_MINIMUM = LogLevel::Trace;
inline constexpr LogLevel LOG_DEFAULT_MINIMUM = LogLevel::Debug;
#else
inline constexpr LogLevel LOG_BUILD_MINIMUM = LogLevel::Info;
inline constexpr LogLevel LOG_DEFAULT_MINIMUM = LogLevel::Info;
#endif

struct LogStats
{
  std::uint64_t written;
  std::uint64_t droppedTrace;
  std::uint64_t droppedDebug;
};

[[nodiscard]] std::string_view LogLevelName(LogLevel _level) noexcept;
[[nodiscard]] std::string_view LogCategoryName(LogCategory _category) noexcept;

// The level test: the whole cost of a disabled record (DBG-066).
[[nodiscard]] bool IsLogEnabled(LogCategory _category, LogLevel _level) noexcept;
void LogFormatted(LogCategory _category, LogLevel _level, std::string_view _format, std::format_args _arguments);

// The one log call (ADR-067, DBG-060): the format string is a compile-time constant, so text from content, players or
// the network is always an argument and is logged verbatim. Nothing formats into a fixed-size buffer.
template <class... Args> void Log(LogCategory _category, LogLevel _level, std::format_string<Args...> _format, Args&&... _args)
{
  if (_level < LOG_BUILD_MINIMUM || !IsLogEnabled(_category, _level))
  {
    return;
  }
  LogFormatted(_category, _level, _format.get(), std::make_format_args(_args...));
}

// DBG-063: dbg.log.hex_bytes_per_line bytes per line in hexadecimal, and a quarter as many 32-bit values per line in
// decimal; each line is one record, led by the offset of its first byte or value.
void LogHexDump(LogCategory _category, LogLevel _level, std::span<const std::byte> _bytes);
void LogValueDump(LogCategory _category, LogLevel _level, std::span<const std::int32_t> _values);

#if ENGINE_DEVELOPMENT
void SetCategoryLevel(LogCategory _category, LogLevel _minimum);
#endif

[[nodiscard]] LogStats GetLogStats();

// The fatal path only (DBG-073): every buffer drained and the file flushed, on the calling thread.
void FlushLogSynchronously();

/// The logging system (ADR-067, DBG-061): per-thread buffers, one logging thread that drains them, and the sinks
/// (DBG-064): the rotating UTF-8 file, the TraceLogging provider, and the debugger output in Debug builds. The
/// composition root owns it (DBG-040): it is created after the platform services, sets itself as the target of Log
/// for its lifetime, is set as the platform's log sink by the root (PLT-133), and is destroyed last, when no other
/// thread logs.
class Logger final : public PlatformLogSink
{
public:
  struct Desc
  {
    const PlatformClock* clock = nullptr;
    std::filesystem::path directory;                         // the logs folder; empty for no file
    std::uint64_t runId = 0;                                 // names the file (PLT-036)
    std::vector<std::pair<std::string, std::string>> header; // the start-up header's fields (DBG-044)
    const void* traceProvider = nullptr;                     // a registered TraceLoggingHProvider, or none
    std::size_t threadBufferBytes = LOG_THREAD_BUFFER_KIB * 1024;
    std::size_t filesKept = LOG_FILES_KEPT;
    bool drainThread = true; // false: drained only by Flush, for tests
  };

  explicit Logger(Desc _desc);
  ~Logger() override;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  // PlatformLogSink: the platform's records, in the Platform category.
  void Write(PlatformLogLevel _level, std::string_view _message) noexcept override;
  void RecordFatal(std::string_view _reason) noexcept override;

  // Drains every buffer to the sinks now.
  void Flush();
  [[nodiscard]] const std::filesystem::path& FilePath() const noexcept;
  [[nodiscard]] LogStats Stats() const;

  [[nodiscard]] bool IsEnabled(LogCategory _category, LogLevel _level) const noexcept;
  void Submit(LogCategory _category, LogLevel _level, std::string_view _message);
  void SetLevel(LogCategory _category, LogLevel _minimum) noexcept;
  void FlushForFatal();

private:
  LogBuffer& ThreadBuffer();
  void RequestDrain();
  void Drain();
  void WriteToSinks(const LogEntry& _entry);
  void RunDrainThread(const std::stop_token& _stop);
  static void ReportLostRecords() noexcept;

  Desc m_desc;
  std::uint64_t m_id;
  std::array<std::atomic<LogLevel>, LOG_CATEGORY_COUNT> m_minimum;
  std::unique_ptr<LogFile> m_file;
  std::string m_line;
  std::vector<LogEntry> m_entries;

  mutable std::mutex m_buffersMutex;
  std::vector<std::shared_ptr<LogBuffer>> m_buffers;

  std::mutex m_drainMutex;
  std::atomic<std::uint64_t> m_written{0};
  std::array<std::array<std::uint64_t, 2>, LOG_CATEGORY_COUNT> m_droppedTotals{};

  std::mutex m_wakeMutex;
  std::condition_variable_any m_wake;
  bool m_wakeRequested = false;
  PlatformThread m_thread;
};

} // namespace Neuron
