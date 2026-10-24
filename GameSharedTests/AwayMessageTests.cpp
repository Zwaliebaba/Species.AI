#include "pch.h"

#include "AwayMessages.h"
#include "MessageCodec.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

TEST_CLASS(AwayMessageTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp072APresenceReportIsItsHeader)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp072APresenceReportIsItsHeader)
  {
    // ADR-181: the report carries nothing but its type and version, and decodes from its header alone.
    const std::vector<std::byte> report = Neuron::EncodeMessage(Species::PresenceReportCommand{});
    ExpectEqual(static_cast<std::int64_t>(Species::PresenceReportCommand::MAX_BYTES), static_cast<std::int64_t>(report.size()),
                L"the report at its bound");
    Neuron::MessageDispatcher dispatcher;
    bool received = false;
    dispatcher.Register<Species::PresenceReportCommand>([&received](const Species::PresenceReportCommand&) { received = true; });
    Assert::IsTrue(dispatcher.Dispatch(report) && received, L"it decodes");
  }
};

} // namespace GameSharedTests
