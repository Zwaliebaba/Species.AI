#include "pch.h"

#include "PlatformLogSink.h"
#include "TestSupport.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

/// Records what the platform writes, for the tests of PLT-133.
class RecordingSink final : public Neuron::PlatformLogSink
{
public:
  void Write(Neuron::PlatformLogLevel, std::string_view _message) noexcept override
  {
    try
    {
      const std::lock_guard lock(m_mutex);
      if (_message.starts_with("sink test"))
      {
        m_messages.emplace_back(_message);
      }
    }
    catch (...)
    {
      m_lost.store(true, std::memory_order_relaxed);
    }
  }

  void RecordFatal(std::string_view) noexcept override {}

  [[nodiscard]] bool LostRecords() const noexcept
  {
    return m_lost.load(std::memory_order_relaxed);
  }

  [[nodiscard]] std::vector<std::string> Messages()
  {
    const std::lock_guard lock(m_mutex);
    return m_messages;
  }

private:
  std::mutex m_mutex;
  std::atomic<bool> m_lost{false};
  std::vector<std::string> m_messages;
};

} // namespace

TEST_CLASS(PlatformLogSinkTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    Neuron::SetPlatformLogSink(nullptr);
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt133EarlyRecordsArriveInOrderWhenTheSinkIsSet)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-133")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt133EarlyRecordsArriveInOrderWhenTheSinkIsSet)
  {
    // PLT T36: the records written before the sink is set reach it in their order, and later ones follow them.
    RecordingSink drain;
    Neuron::SetPlatformLogSink(&drain);
    Neuron::SetPlatformLogSink(nullptr);
    Assert::IsTrue(Neuron::CurrentPlatformLogSink() == nullptr, L"no sink is set");
    Neuron::PlatformLog(Neuron::PlatformLogLevel::Info, "sink test 1");
    Neuron::PlatformLog(Neuron::PlatformLogLevel::Warning, "sink test {}", 2);
    Neuron::PlatformLog(Neuron::PlatformLogLevel::Error, "sink test 3");
    RecordingSink sink;
    Neuron::SetPlatformLogSink(&sink);
    Assert::IsTrue(Neuron::CurrentPlatformLogSink() == &sink, L"the sink is set");
    Neuron::PlatformLog(Neuron::PlatformLogLevel::Debug, "sink test 4");
    Assert::IsFalse(sink.LostRecords(), L"every record was kept");
    const std::vector<std::string> messages = sink.Messages();
    ExpectEqual(4, static_cast<std::int64_t>(messages.size()), L"records received");
    for (std::size_t index = 0; index < messages.size(); ++index)
    {
      ExpectText(std::format("sink test {}", index + 1), messages[index], L"record order");
    }
  }
};

} // namespace NeuronCoreTests
