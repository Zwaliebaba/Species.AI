#include "pch.h"

#include "Files.h"
#include "Log.h"
#include "LogFile.h"
#include "PlatformClock.h"
#include "PlatformThread.h"
#include "TestSupport.h"

#include <array>
#include <cstddef>
#include <memory>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

/// A logger that writes into a temporary folder, on a manual clock, drained only when the test flushes it.
class TestLogger
{
public:
  explicit TestLogger(std::size_t _threadBufferBytes = Neuron::LOG_THREAD_BUFFER_KIB * 1024)
    : m_clock(std::make_unique<Neuron::ManualTimeSource>())
  {
    Neuron::Logger::Desc desc;
    desc.clock = &m_clock;
    desc.directory = m_folder.Path() / L"logs";
    desc.runId = 0x0123456789ABCDEF;
    desc.header = {{"build", "test"}};
    desc.threadBufferBytes = _threadBufferBytes;
    desc.drainThread = false;
    m_logger = std::make_unique<Neuron::Logger>(std::move(desc));
  }

  [[nodiscard]] Neuron::Logger& Get() noexcept
  {
    return *m_logger;
  }

  [[nodiscard]] std::string Text()
  {
    m_logger->Flush();
    return ReadText(m_logger->FilePath());
  }

private:
  TemporaryFolder m_folder;
  Neuron::PlatformClock m_clock;
  std::unique_ptr<Neuron::Logger> m_logger;
};

} // namespace

TEST_CLASS(LogTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg060FormatTextInAnArgumentIsLoggedVerbatim)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg060FormatTextInAnArgumentIsLoggedVerbatim)
  {
    // DBG T1, the file sink.
    TestLogger logger;
    Neuron::Log(Neuron::LogCategory::Test, Neuron::LogLevel::Info, "player text: {}", "100% {} of %s {0}");
    ExpectContains(logger.Text(), "player text: 100% {} of %s {0}\n", L"the argument as written");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg043RecordCarriesThreadCategoryAndLevel)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-043")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg043RecordCarriesThreadCategoryAndLevel)
  {
    TestLogger logger;
    {
      Neuron::PlatformThread thread = Neuron::PlatformThread::Start(
        "test.logger", [](const std::stop_token&) { Neuron::Log(Neuron::LogCategory::Network, Neuron::LogLevel::Warning, "{} peers", 3); });
    }
    const std::string text = logger.Text();
    ExpectContains(text, "[test.logger] Network warning: 3 peers\n", L"thread, category, level and message");
    ExpectContains(text, "Log of run 0123456789abcdef", L"the header names the run (DBG-044)");
    ExpectContains(text, "build: test\n", L"the header's fields");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg063HexDumpWritesSixteenBytesPerLine)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg063HexDumpWritesSixteenBytesPerLine)
  {
    // DBG T4: 20 bytes are two lines, of 16 and of 4 bytes.
    TestLogger logger;
    std::array<std::byte, 20> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
      bytes[index] = static_cast<std::byte>(index);
    }
    Neuron::LogHexDump(Neuron::LogCategory::Test, Neuron::LogLevel::Info, bytes);
    const std::string text = logger.Text();
    ExpectContains(text, "info: 00000000: 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n", L"the first line");
    ExpectContains(text, "info: 00000010: 10 11 12 13\n", L"the second line");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg063ValueDumpWritesFourValuesPerLine)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg063ValueDumpWritesFourValuesPerLine)
  {
    TestLogger logger;
    const std::array<std::int32_t, 5> values{1, -2, 300, 4, 5};
    Neuron::LogValueDump(Neuron::LogCategory::Test, Neuron::LogLevel::Info, values);
    const std::string text = logger.Text();
    ExpectContains(text, "info:        0: 1 -2 300 4\n", L"the first line");
    ExpectContains(text, "info:        4: 5\n", L"the second line");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg064DebugIsRecordedAndTraceOnlyWhenEnabled)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-064")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg064DebugIsRecordedAndTraceOnlyWhenEnabled)
  {
    TestLogger logger;
    Neuron::Log(Neuron::LogCategory::Test, Neuron::LogLevel::Debug, "debug record");
    Neuron::Log(Neuron::LogCategory::Test, Neuron::LogLevel::Trace, "first trace record");
#if ENGINE_DEVELOPMENT
    Assert::IsFalse(Neuron::IsLogEnabled(Neuron::LogCategory::Test, Neuron::LogLevel::Trace), L"trace starts disabled (DBG-066)");
    Neuron::SetCategoryLevel(Neuron::LogCategory::Test, Neuron::LogLevel::Trace);
    Assert::IsTrue(Neuron::IsLogEnabled(Neuron::LogCategory::Test, Neuron::LogLevel::Trace), L"trace enabled at run time");
    Neuron::Log(Neuron::LogCategory::Test, Neuron::LogLevel::Trace, "second trace record");
    const std::string text = logger.Text();
    ExpectContains(text, "Test debug: debug record\n", L"Debug builds record debug records");
    ExpectContains(text, "Test trace: second trace record\n", L"an enabled trace record");
    Assert::IsTrue(text.find("first trace record") == std::string::npos, L"a disabled trace record is not written");
#else
    const std::string text = logger.Text();
    Assert::IsTrue(text.find("debug record") == std::string::npos, L"Release builds record info and above");
#endif
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg062DrainWarnsWithTheDropCounts)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-062")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg062DrainWarnsWithTheDropCounts)
  {
    // Only trace and debug records are ever dropped, and Release builds have neither.
#if ENGINE_DEVELOPMENT
    TestLogger logger(4096);
    for (int index = 0; index < 40; ++index)
    {
      Neuron::Log(Neuron::LogCategory::Test, Neuron::LogLevel::Debug, "{:100}", index);
    }
    const std::string text = logger.Text();
    ExpectContains(text, "Diagnostics warning: log records dropped since the last report: Test debug ", L"the drop warning");
    Assert::IsTrue(Neuron::GetLogStats().droppedDebug > 0, L"the statistics count the drops");
    Assert::IsTrue(Neuron::GetLogStats().written > 0, L"and the records written");
#endif
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg062DropsOfAnEndedThreadStayCounted)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-062")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg062DropsOfAnEndedThreadStayCounted)
  {
    // The drain retires the buffer of a thread that has ended once it is empty; its drops stay in the statistics.
#if ENGINE_DEVELOPMENT
    constexpr int RECORDS = 40;
    TestLogger logger(4096);
    const auto logRecords = [](const std::stop_token&)
    {
      for (int index = 0; index < RECORDS; ++index)
      {
        Neuron::Log(Neuron::LogCategory::Test, Neuron::LogLevel::Debug, "{:100}", index);
      }
    };
    {
      Neuron::PlatformThread thread = Neuron::PlatformThread::Start("test.dropper", logRecords);
    }
    logger.Get().Flush();
    const Neuron::LogStats stats = logger.Get().Stats();
    Assert::IsTrue(stats.droppedDebug > 0, L"the ended thread's drops are counted");
    ExpectEqual(RECORDS, static_cast<std::int64_t>(stats.written + stats.droppedDebug), L"each record is written or dropped");
#endif
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt133LoggerWritesPlatformRecordsInThePlatformCategory)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-133")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt133LoggerWritesPlatformRecordsInThePlatformCategory)
  {
    TestLogger logger;
    logger.Get().Write(Neuron::PlatformLogLevel::Info, "platform record");
    ExpectContains(logger.Text(), "Platform info: platform record\n", L"a platform record");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg049OnlyTheNewestLogFilesAreKept)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-049")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg049OnlyTheNewestLogFilesAreKept)
  {
    const TemporaryFolder folder;
    for (int index = 0; index < 12; ++index)
    {
      Assert::IsTrue(Neuron::ReplaceFileAtomically(folder.Path() / std::format(L"20261019T0000{:02}Z-old.log", index), {}).has_value(),
                     L"an old log file");
    }
    Neuron::LogFile file;
    ExpectEqual(0, file.Open(folder.Path(), L"20261019T120000Z-new.log", Neuron::LOG_FILES_KEPT), L"the open error");
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(folder.Path()))
    {
      ++count;
      Assert::IsTrue(entry.path().filename().wstring() >= L"20261019T000003Z", L"the oldest files are the ones deleted");
    }
    ExpectEqual(static_cast<std::int64_t>(Neuron::LOG_FILES_KEPT), count, L"files kept");
  }
};

} // namespace NeuronCoreTests
