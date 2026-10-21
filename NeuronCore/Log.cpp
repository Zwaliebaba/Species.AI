#include "pch.h"

#include <TraceLoggingProvider.h>
#include <winmeta.h>

#include "Log.h"
#include "LogBuffer.h"
#include "LogFile.h"
#include "PlatformClock.h"
#include "TextEncoding.h"

#include <algorithm>
#include <chrono>
#include <iterator>

namespace Neuron
{

namespace
{

// How often the logging thread drains the buffers when no writer asks it to sooner.
constexpr std::chrono::milliseconds DRAIN_PERIOD{10};

/// The calling thread's buffer of the logger with the given id; a thread that outlives its logger keeps the buffer
/// alive until it ends, and never uses it again.
struct ThreadLogState
{
  std::uint64_t loggerId = 0;
  std::shared_ptr<LogBuffer> buffer;

  ThreadLogState() = default;
  ThreadLogState(const ThreadLogState&) = delete;
  ThreadLogState& operator=(const ThreadLogState&) = delete;

  ~ThreadLogState()
  {
    if (buffer)
    {
      buffer->MarkThreadEnded();
    }
  }
};

thread_local ThreadLogState g_threadLog;
thread_local std::string g_scratch;
// Set on the logging thread, whose own records go straight to the sinks: it cannot wait for itself.
thread_local const Logger* g_drainingLogger = nullptr;

// The logger that Log writes to: a registration made by its owner for the owner's lifetime, not an owning global
// (DBG-040, ADR-091).
std::atomic<Logger*> g_logger{nullptr};
std::atomic<std::uint64_t> g_nextLoggerId{1};

constexpr std::array<std::string_view, LOG_LEVEL_COUNT> LEVEL_NAMES{"trace", "debug", "info", "warning", "error", "fatal"};
constexpr std::array<std::string_view, LOG_CATEGORY_COUNT> CATEGORY_NAMES{"Platform",   "Render",  "Audio", "Input",       "Ui",  "Content",
                                                                          "Simulation", "Network", "Tools", "Diagnostics", "Test"};

LogLevel ToLogLevel(PlatformLogLevel _level) noexcept
{
  switch (_level)
  {
  case PlatformLogLevel::Debug:
    return LogLevel::Debug;
  case PlatformLogLevel::Info:
    return LogLevel::Info;
  case PlatformLogLevel::Warning:
    return LogLevel::Warning;
  case PlatformLogLevel::Error:
    return LogLevel::Error;
  }
  return LogLevel::Error;
}

// One line per record: platform-clock seconds, UTC time of day, thread, category, level and message (DBG-043).
void AppendLine(std::string& _line, const LogEntry& _entry)
{
  const std::chrono::system_clock::time_point utc{std::chrono::system_clock::duration{_entry.utcTicks}};
  std::format_to(std::back_inserter(_line), "{:.6f} {:%H:%M:%S} [{}] {} {}: {}\n",
                 static_cast<double>(_entry.timeNanoseconds) / 1'000'000'000.0, std::chrono::floor<std::chrono::milliseconds>(utc),
                 _entry.threadName != nullptr ? *_entry.threadName : "", LogCategoryName(_entry.category), LogLevelName(_entry.level),
                 _entry.message);
}

// TraceLogging needs the level as a compile-time constant, so each level is its own event site (DBG-064).
void WriteTrace(const void* _provider, const LogEntry& _entry, const std::string& _thread) noexcept
{
  const auto provider = static_cast<TraceLoggingHProvider>(_provider);
  const char* category = CATEGORY_NAMES[static_cast<std::size_t>(_entry.category)].data();
  const char* message = _entry.message.c_str();
  const char* thread = _thread.c_str();
  switch (_entry.level)
  {
  case LogLevel::Trace:
  case LogLevel::Debug:
    TraceLoggingWrite(provider, "Log", TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE), TraceLoggingString(category, "Category"),
                      TraceLoggingUtf8String(thread, "Thread"), TraceLoggingUtf8String(message, "Message"));
    break;
  case LogLevel::Info:
    TraceLoggingWrite(provider, "Log", TraceLoggingLevel(WINEVENT_LEVEL_INFO), TraceLoggingString(category, "Category"),
                      TraceLoggingUtf8String(thread, "Thread"), TraceLoggingUtf8String(message, "Message"));
    break;
  case LogLevel::Warning:
    TraceLoggingWrite(provider, "Log", TraceLoggingLevel(WINEVENT_LEVEL_WARNING), TraceLoggingString(category, "Category"),
                      TraceLoggingUtf8String(thread, "Thread"), TraceLoggingUtf8String(message, "Message"));
    break;
  case LogLevel::Error:
    TraceLoggingWrite(provider, "Log", TraceLoggingLevel(WINEVENT_LEVEL_ERROR), TraceLoggingString(category, "Category"),
                      TraceLoggingUtf8String(thread, "Thread"), TraceLoggingUtf8String(message, "Message"));
    break;
  case LogLevel::Fatal:
    TraceLoggingWrite(provider, "Log", TraceLoggingLevel(WINEVENT_LEVEL_CRITICAL), TraceLoggingString(category, "Category"),
                      TraceLoggingUtf8String(thread, "Thread"), TraceLoggingUtf8String(message, "Message"));
    break;
  }
}

} // namespace

std::string_view LogLevelName(LogLevel _level) noexcept
{
  const auto index = static_cast<std::size_t>(_level);
  return index < LEVEL_NAMES.size() ? LEVEL_NAMES[index] : "unknown";
}

std::string_view LogCategoryName(LogCategory _category) noexcept
{
  const auto index = static_cast<std::size_t>(_category);
  return index < CATEGORY_NAMES.size() ? CATEGORY_NAMES[index] : "Unknown";
}

bool IsLogEnabled(LogCategory _category, LogLevel _level) noexcept
{
  const Logger* logger = g_logger.load(std::memory_order_acquire);
  return logger != nullptr && logger->IsEnabled(_category, _level);
}

void LogFormatted(LogCategory _category, LogLevel _level, std::string_view _format, std::format_args _arguments)
{
  Logger* logger = g_logger.load(std::memory_order_acquire);
  if (logger == nullptr)
  {
    return;
  }
  g_scratch.clear();
  std::vformat_to(std::back_inserter(g_scratch), _format, _arguments);
  logger->Submit(_category, _level, g_scratch);
}

void LogHexDump(LogCategory _category, LogLevel _level, std::span<const std::byte> _bytes)
{
  if (_level < LOG_BUILD_MINIMUM || !IsLogEnabled(_category, _level))
  {
    return;
  }
  std::string line;
  for (std::size_t offset = 0; offset < _bytes.size(); offset += LOG_HEX_BYTES_PER_LINE)
  {
    line.clear();
    std::format_to(std::back_inserter(line), "{:08x}:", offset);
    for (const std::byte value : _bytes.subspan(offset, std::min(LOG_HEX_BYTES_PER_LINE, _bytes.size() - offset)))
    {
      std::format_to(std::back_inserter(line), " {:02x}", static_cast<unsigned int>(value));
    }
    g_logger.load(std::memory_order_acquire)->Submit(_category, _level, line);
  }
}

void LogValueDump(LogCategory _category, LogLevel _level, std::span<const std::int32_t> _values)
{
  if (_level < LOG_BUILD_MINIMUM || !IsLogEnabled(_category, _level))
  {
    return;
  }
  constexpr std::size_t VALUES_PER_LINE = LOG_HEX_BYTES_PER_LINE / sizeof(std::int32_t);
  std::string line;
  for (std::size_t index = 0; index < _values.size(); index += VALUES_PER_LINE)
  {
    line.clear();
    std::format_to(std::back_inserter(line), "{:8}:", index);
    for (const std::int32_t value : _values.subspan(index, std::min(VALUES_PER_LINE, _values.size() - index)))
    {
      std::format_to(std::back_inserter(line), " {}", value);
    }
    g_logger.load(std::memory_order_acquire)->Submit(_category, _level, line);
  }
}

#if ENGINE_DEVELOPMENT
void SetCategoryLevel(LogCategory _category, LogLevel _minimum)
{
  if (Logger* logger = g_logger.load(std::memory_order_acquire))
  {
    logger->SetLevel(_category, _minimum);
  }
}
#endif

LogStats GetLogStats()
{
  const Logger* logger = g_logger.load(std::memory_order_acquire);
  return logger != nullptr ? logger->Stats() : LogStats{0, 0, 0};
}

void FlushLogSynchronously()
{
  if (Logger* logger = g_logger.load(std::memory_order_acquire))
  {
    logger->FlushForFatal();
  }
}

Logger::Logger(Desc _desc)
  : m_desc(std::move(_desc)),
    m_id(g_nextLoggerId.fetch_add(1, std::memory_order_relaxed))
{
  for (std::atomic<LogLevel>& minimum : m_minimum)
  {
    minimum.store(LOG_DEFAULT_MINIMUM, std::memory_order_relaxed);
  }
  const std::chrono::system_clock::time_point start =
    m_desc.clock != nullptr ? m_desc.clock->CalendarUtc() : std::chrono::system_clock::now();
  if (!m_desc.directory.empty())
  {
    m_file = std::make_unique<LogFile>();
    const std::string fileName =
      std::format("{:%Y%m%dT%H%M%S}Z-{:016x}.log", std::chrono::floor<std::chrono::seconds>(start), m_desc.runId);
    const std::uint32_t error = m_file->Open(m_desc.directory, ToUtf16(fileName), m_desc.filesKept);
    if (error == 0)
    {
      std::string header = std::format("Log of run {:016x}, started {:%Y-%m-%d %H:%M:%S} UTC\n", m_desc.runId,
                                       std::chrono::floor<std::chrono::seconds>(start));
      for (const auto& [name, value] : m_desc.header)
      {
        std::format_to(std::back_inserter(header), "{}: {}\n", name, value);
      }
      m_file->Write(header);
    }
    else
    {
      m_entries.push_back(LogEntry{0, start.time_since_epoch().count(), LogCategory::Diagnostics, LogLevel::Error, nullptr,
                                   std::format("the log file {} could not be opened (error {})", ToUtf8(m_file->Path().native()), error)});
      m_file.reset();
    }
  }
  Logger* expected = nullptr;
  g_logger.compare_exchange_strong(expected, this, std::memory_order_acq_rel);
  for (const LogEntry& entry : m_entries)
  {
    WriteToSinks(entry);
  }
  m_entries.clear();
  if (m_desc.drainThread)
  {
    m_thread = PlatformThread::Start("log", [this](const std::stop_token& _stop) { RunDrainThread(_stop); });
  }
}

Logger::~Logger()
{
  Logger* expected = this;
  g_logger.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel);
  try
  {
    m_thread.RequestStop();
    m_thread.Join();
    Drain();
  }
  catch (...)
  {
    ReportLostRecords();
  }
}

void Logger::Write(PlatformLogLevel _level, std::string_view _message) noexcept
{
  try
  {
    const LogLevel level = ToLogLevel(_level);
    if (level >= LOG_BUILD_MINIMUM && IsEnabled(LogCategory::Platform, level))
    {
      Submit(LogCategory::Platform, level, _message);
    }
  }
  catch (...)
  {
    ReportLostRecords();
  }
}

void Logger::RecordFatal(std::string_view _reason) noexcept
{
  // The black box's failure record (DBG-073 step 1) arrives with the black box; step 2 is the flushed fatal record,
  // written under the drain's lock so that it never interleaves with the logging thread's drain.
  try
  {
    const std::chrono::system_clock::time_point utc = std::chrono::system_clock::now();
    const LogEntry entry{m_desc.clock != nullptr ? m_desc.clock->Now().count() : 0,
                         utc.time_since_epoch().count(),
                         LogCategory::Diagnostics,
                         LogLevel::Fatal,
                         &CurrentThreadName(),
                         std::string{_reason}};
    if (g_drainingLogger == this)
    {
      WriteToSinks(entry);
    }
    else
    {
      Drain();
      const std::lock_guard drain(m_drainMutex);
      WriteToSinks(entry);
    }
  }
  catch (...)
  {
    ReportLostRecords();
  }
  if (m_file)
  {
    m_file->Flush();
  }
}

void Logger::ReportLostRecords() noexcept
{
  // Out of memory or a failed thread join, on a path that must not throw: say so where nothing allocates.
  OutputDebugStringW(L"Neuron logger: records were lost\n");
}

void Logger::Flush()
{
  Drain();
}

const std::filesystem::path& Logger::FilePath() const noexcept
{
  static const std::filesystem::path NO_PATH;
  return m_file ? m_file->Path() : NO_PATH;
}

LogStats Logger::Stats() const
{
  LogStats stats{m_written.load(std::memory_order_relaxed), 0, 0};
  const std::lock_guard lock(m_buffersMutex);
  stats.droppedTrace = m_retiredDropped[0];
  stats.droppedDebug = m_retiredDropped[1];
  for (const std::shared_ptr<LogBuffer>& buffer : m_buffers)
  {
    for (std::size_t category = 0; category < LOG_CATEGORY_COUNT; ++category)
    {
      stats.droppedTrace += buffer->Dropped(static_cast<LogCategory>(category), LogLevel::Trace);
      stats.droppedDebug += buffer->Dropped(static_cast<LogCategory>(category), LogLevel::Debug);
    }
  }
  return stats;
}

bool Logger::IsEnabled(LogCategory _category, LogLevel _level) const noexcept
{
  const auto index = static_cast<std::size_t>(_category);
  return index < LOG_CATEGORY_COUNT && _level >= m_minimum[index].load(std::memory_order_relaxed);
}

void Logger::SetLevel(LogCategory _category, LogLevel _minimum) noexcept
{
  const auto index = static_cast<std::size_t>(_category);
  if (index < LOG_CATEGORY_COUNT)
  {
    m_minimum[index].store(_minimum < LOG_BUILD_MINIMUM ? LOG_BUILD_MINIMUM : _minimum, std::memory_order_relaxed);
  }
}

void Logger::Submit(LogCategory _category, LogLevel _level, std::string_view _message)
{
  const std::int64_t timeNanoseconds = m_desc.clock != nullptr ? m_desc.clock->Now().count() : 0;
  const std::int64_t utcTicks = std::chrono::system_clock::now().time_since_epoch().count();
  if (g_drainingLogger == this)
  {
    const LogEntry entry{timeNanoseconds, utcTicks, _category, _level, &CurrentThreadName(), std::string{_message}};
    WriteToSinks(entry);
    return;
  }
  ThreadBuffer().Write(_category, _level, timeNanoseconds, utcTicks, _message);
}

void Logger::FlushForFatal()
{
  // On the logging thread, which may be inside a drain, the buffers are left as they are.
  if (g_drainingLogger != this)
  {
    Drain();
  }
  if (m_file)
  {
    m_file->Flush();
  }
}

LogBuffer& Logger::ThreadBuffer()
{
  if (g_threadLog.loggerId != m_id || !g_threadLog.buffer)
  {
    if (g_threadLog.buffer)
    {
      g_threadLog.buffer->MarkThreadEnded();
    }
    auto buffer = std::make_shared<LogBuffer>(m_desc.threadBufferBytes, CurrentThreadName(), [this] { RequestDrain(); });
    {
      const std::lock_guard lock(m_buffersMutex);
      m_buffers.push_back(buffer);
    }
    g_threadLog.loggerId = m_id;
    g_threadLog.buffer = std::move(buffer);
  }
  return *g_threadLog.buffer;
}

void Logger::RequestDrain()
{
  {
    const std::lock_guard lock(m_wakeMutex);
    m_wakeRequested = true;
  }
  m_wake.notify_one();
}

void Logger::Drain()
{
  const std::lock_guard drain(m_drainMutex);
  std::vector<std::shared_ptr<LogBuffer>> buffers;
  {
    const std::lock_guard lock(m_buffersMutex);
    buffers = m_buffers;
  }
  m_entries.clear();
  LogDropCounts newDrops{};
  for (const std::shared_ptr<LogBuffer>& buffer : buffers)
  {
    buffer->Drain(m_entries);
    buffer->TakeNewDrops(newDrops);
  }
  // Each buffer is in its thread's order; across threads, records go out in time order.
  std::stable_sort(m_entries.begin(), m_entries.end(),
                   [](const LogEntry& _left, const LogEntry& _right) { return _left.timeNanoseconds < _right.timeNanoseconds; });
  for (const LogEntry& entry : m_entries)
  {
    WriteToSinks(entry);
  }
  m_written.fetch_add(m_entries.size(), std::memory_order_relaxed);

  // DBG-062: a warning states the drops since the previous such warning whenever a drain finds new ones.
  std::string report;
  for (std::size_t category = 0; category < LOG_CATEGORY_COUNT; ++category)
  {
    for (std::size_t level = 0; level < 2; ++level)
    {
      if (newDrops[category][level] > 0)
      {
        m_droppedTotals[category][level] += newDrops[category][level];
        std::format_to(std::back_inserter(report), "{}{} {} {}", report.empty() ? "" : ", ", CATEGORY_NAMES[category], LEVEL_NAMES[level],
                       newDrops[category][level]);
      }
    }
  }
  if (!report.empty())
  {
    const LogEntry warning{m_desc.clock != nullptr ? m_desc.clock->Now().count() : 0,
                           std::chrono::system_clock::now().time_since_epoch().count(),
                           LogCategory::Diagnostics,
                           LogLevel::Warning,
                           &CurrentThreadName(),
                           "log records dropped since the last report: " + report};
    WriteToSinks(warning);
  }
  m_entries.clear();

  // A buffer whose thread has ended is retired once it is empty, and its drops stay in the statistics: no record can
  // be written into it, or dropped from it, any more.
  const std::lock_guard lock(m_buffersMutex);
  std::erase_if(m_buffers,
                [this](const std::shared_ptr<LogBuffer>& _buffer)
                {
                  if (!_buffer->ThreadEnded() || !_buffer->IsEmpty())
                  {
                    return false;
                  }
                  for (std::size_t category = 0; category < LOG_CATEGORY_COUNT; ++category)
                  {
                    m_retiredDropped[0] += _buffer->Dropped(static_cast<LogCategory>(category), LogLevel::Trace);
                    m_retiredDropped[1] += _buffer->Dropped(static_cast<LogCategory>(category), LogLevel::Debug);
                  }
                  return true;
                });
}

void Logger::WriteToSinks(const LogEntry& _entry)
{
  m_line.clear();
  AppendLine(m_line, _entry);
  if (m_file)
  {
    m_file->Write(m_line);
  }
  if (m_desc.traceProvider != nullptr)
  {
    WriteTrace(m_desc.traceProvider, _entry, _entry.threadName != nullptr ? *_entry.threadName : std::string{});
  }
#if ENGINE_DEVELOPMENT
  OutputDebugStringW(ToUtf16(m_line).c_str());
#endif
}

void Logger::RunDrainThread(const std::stop_token& _stop)
{
  g_drainingLogger = this;
  while (!_stop.stop_requested())
  {
    {
      std::unique_lock lock(m_wakeMutex);
      m_wake.wait_for(lock, _stop, DRAIN_PERIOD, [this] { return m_wakeRequested; });
      m_wakeRequested = false;
    }
    Drain();
  }
  g_drainingLogger = nullptr;
}

} // namespace Neuron
