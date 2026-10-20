#include "pch.h"

#include "LogBuffer.h"
#include "TestSupport.h"

#include <string>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// Small enough that a few records fill a ring: two rings of 2 KiB, parts of at most 472 bytes.
constexpr std::size_t SMALL_BUFFER_BYTES = 4096;

// A record of 100 characters.
constexpr std::string_view RECORD_TEXT = "0123456789"
                                         "0123456789"
                                         "0123456789"
                                         "0123456789"
                                         "0123456789"
                                         "0123456789"
                                         "0123456789"
                                         "0123456789"
                                         "0123456789"
                                         "0123456789";

int WriteUntil(Neuron::LogBuffer& _buffer, Neuron::LogLevel _level, Neuron::LogWriteResult _stop)
{
  int written = 0;
  while (_buffer.TryWrite(Neuron::LogCategory::Test, _level, 0, 0, RECORD_TEXT) != _stop)
  {
    ++written;
    if (written > 1000)
    {
      Assert::Fail(L"the buffer never filled");
    }
  }
  return written;
}

} // namespace

TEST_CLASS(LogBufferTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg061TraceIsDroppedBeforeDebug)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-061")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg061TraceIsDroppedBeforeDebug)
  {
    // DBG T3: a full buffer drops trace records first, then debug records.
    Neuron::LogBuffer buffer(SMALL_BUFFER_BYTES, "test", {});
    const int traces = WriteUntil(buffer, Neuron::LogLevel::Trace, Neuron::LogWriteResult::Dropped);
    Assert::IsTrue(traces > 0, L"trace records fit while the ring is less than half full");
    Assert::IsTrue(buffer.TryWrite(Neuron::LogCategory::Test, Neuron::LogLevel::Debug, 0, 0, RECORD_TEXT) ==
                     Neuron::LogWriteResult::Written,
                   L"a debug record still fits when trace records no longer do");
    const int debugs = WriteUntil(buffer, Neuron::LogLevel::Debug, Neuron::LogWriteResult::Dropped);
    Assert::IsTrue(debugs > 0, L"debug records fill the rest of the ring");
    ExpectEqual(1, static_cast<std::int64_t>(buffer.Dropped(Neuron::LogCategory::Test, Neuron::LogLevel::Trace)), L"trace drops");
    ExpectEqual(1, static_cast<std::int64_t>(buffer.Dropped(Neuron::LogCategory::Test, Neuron::LogLevel::Debug)), L"debug drops");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg061WarningsAreNeverDropped)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-061")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg061WarningsAreNeverDropped)
  {
    Neuron::LogBuffer buffer(SMALL_BUFFER_BYTES, "test", {});
    WriteUntil(buffer, Neuron::LogLevel::Debug, Neuron::LogWriteResult::Dropped);
    Assert::IsTrue(buffer.TryWrite(Neuron::LogCategory::Test, Neuron::LogLevel::Warning, 0, 0, RECORD_TEXT) ==
                     Neuron::LogWriteResult::Written,
                   L"a warning fits in a buffer full of debug records");
    // The warnings' ring full of nothing but warnings is the one case in which a writer waits: it is full, not dropped.
    const int warnings = WriteUntil(buffer, Neuron::LogLevel::Warning, Neuron::LogWriteResult::Full);
    Assert::IsTrue(warnings > 0, L"warnings fill their ring");
    std::vector<Neuron::LogEntry> entries;
    buffer.Drain(entries);
    Assert::IsTrue(buffer.TryWrite(Neuron::LogCategory::Test, Neuron::LogLevel::Error, 0, 0, RECORD_TEXT) ==
                     Neuron::LogWriteResult::Written,
                   L"a drain frees the ring");
    ExpectEqual(0, static_cast<std::int64_t>(buffer.Dropped(Neuron::LogCategory::Test, Neuron::LogLevel::Warning)), L"warning drops");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg061DrainKeepsTheThreadOrderAcrossBothRings)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-061")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg061DrainKeepsTheThreadOrderAcrossBothRings)
  {
    Neuron::LogBuffer buffer(SMALL_BUFFER_BYTES, "test", {});
    const Neuron::LogLevel levels[] = {Neuron::LogLevel::Info,  Neuron::LogLevel::Debug, Neuron::LogLevel::Warning,
                                       Neuron::LogLevel::Trace, Neuron::LogLevel::Error, Neuron::LogLevel::Debug};
    // Enough rounds to wrap both rings more than once.
    std::vector<Neuron::LogEntry> entries;
    int next = 0;
    for (int round = 0; round < 40; ++round)
    {
      for (const Neuron::LogLevel level : levels)
      {
        Assert::IsTrue(buffer.TryWrite(Neuron::LogCategory::Test, level, next, 0, std::format("record {}", next)) ==
                         Neuron::LogWriteResult::Written,
                       L"written");
        ++next;
      }
      buffer.Drain(entries);
    }
    ExpectEqual(next, static_cast<std::int64_t>(entries.size()), L"records drained");
    for (int index = 0; index < next; ++index)
    {
      const Neuron::LogEntry& entry = entries[static_cast<std::size_t>(index)];
      ExpectEqual(index, entry.timeNanoseconds, L"order");
      ExpectText(std::format("record {}", index), entry.message, L"message");
      Assert::IsTrue(entry.level == levels[index % 6], L"level");
      Assert::AreEqual("test", entry.threadName->c_str());
    }
    Assert::IsTrue(buffer.IsEmpty(), L"the buffer is empty after the drain");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg060LongMessageIsLoggedWhole)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg060LongMessageIsLoggedWhole)
  {
    // A message of more than twice its ring is written in parts while the drain frees space, and arrives whole.
    std::vector<Neuron::LogEntry> entries;
    Neuron::LogBuffer* drained = nullptr;
    Neuron::LogBuffer buffer(SMALL_BUFFER_BYTES, "test", [&] { drained->Drain(entries); });
    drained = &buffer;
    std::string message;
    for (int index = 0; message.size() < 5000; ++index)
    {
      message += std::format("{:04} ", index);
    }
    buffer.Write(Neuron::LogCategory::Test, Neuron::LogLevel::Info, 7, 0, message);
    buffer.Drain(entries);
    ExpectEqual(1, static_cast<std::int64_t>(entries.size()), L"records");
    ExpectText(message, entries.front().message, L"the long message");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg062DropsAreCountedPerCategoryAndLevel)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-062")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg062DropsAreCountedPerCategoryAndLevel)
  {
    Neuron::LogBuffer buffer(SMALL_BUFFER_BYTES, "test", {});
    WriteUntil(buffer, Neuron::LogLevel::Debug, Neuron::LogWriteResult::Dropped);
    for (int drop = 0; drop < 3; ++drop)
    {
      (void)buffer.TryWrite(Neuron::LogCategory::Render, Neuron::LogLevel::Trace, 0, 0, RECORD_TEXT);
    }
    Neuron::LogDropCounts counts{};
    buffer.TakeNewDrops(counts);
    ExpectEqual(3, static_cast<std::int64_t>(counts[static_cast<std::size_t>(Neuron::LogCategory::Render)][0]), L"Render trace");
    ExpectEqual(1, static_cast<std::int64_t>(counts[static_cast<std::size_t>(Neuron::LogCategory::Test)][1]), L"Test debug");
    Neuron::LogDropCounts again{};
    buffer.TakeNewDrops(again);
    ExpectEqual(0, static_cast<std::int64_t>(again[static_cast<std::size_t>(Neuron::LogCategory::Render)][0]), L"only new drops");
  }
};

} // namespace NeuronCoreTests
