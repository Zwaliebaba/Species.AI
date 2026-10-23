#include "pch.h"

#include "CommandMessages.h"
#include "MessageCodec.h"
#include "SettlementMessages.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameSharedTests
{

namespace
{

// Whether the bytes decode whole as a T.
template <class T> bool Decodes(const std::vector<std::byte>& _bytes)
{
  Neuron::MessageDispatcher dispatcher;
  dispatcher.Register<T>([](const T&) {});
  return dispatcher.Dispatch(_bytes);
}

} // namespace

TEST_CLASS(SettlementMessageTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp098AFullStatusFitsItsBoundAndRoundTrips)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-037")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp098AFullStatusFitsItsBoundAndRoundTrips)
  {
    // A status listing every construction site it may is exactly its bound, and reads back as it was written.
    Species::SettlementStatus sent;
    sent.tick = 0xfedcba9876543210;
    sent.team = 3;
    sent.storesMilliRu = 1'234'567;
    sent.capacityMilliRu = 2'000'000;
    sent.population = 287;
    sent.populationLimit = 300;
    sent.growth = Species::GrowthState::StoresShort;
    sent.growthTicks = 119;
    sent.priorities = {45, 35, 20};
    sent.tasks = {150, 40, 60};
    sent.idle = {1, 2, 3, 4};
    sent.inactiveStructures = 2;
    sent.upkeepMilliRuPerHour = 1'800'000;
    sent.strengthMilliRu = 9'876'543;
    for (std::uint64_t site = 0; site < Species::STATUS_QUEUE_MAX; ++site)
    {
      sent.queue.push_back({0x100 + site, Species::StructureKind::Turret, 1'000});
    }
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(sent);
    ExpectEqual(static_cast<std::int64_t>(Species::SettlementStatus::MAX_BYTES), static_cast<std::int64_t>(bytes.size()),
                L"its fields and every site");
    Neuron::MessageDispatcher dispatcher;
    std::optional<Species::SettlementStatus> received;
    dispatcher.Register<Species::SettlementStatus>([&received](const Species::SettlementStatus& _status) { received = _status; });
    Assert::IsTrue(dispatcher.Dispatch(bytes), L"it decodes whole");
    Assert::IsTrue(received.has_value() && Neuron::EncodeMessage(received.value_or(Species::SettlementStatus{})) == bytes,
                   L"the same fields");
    Assert::IsTrue(received && received->growth == Species::GrowthState::StoresShort && received->idle[3] == 4 &&
                     received->queue.back().id == 0x100 + Species::STATUS_QUEUE_MAX - 1,
                   L"its growth, its reasons and its last site");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ASettlementCommandOutOfItsBoundsIsMalformed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ASettlementCommandOutOfItsBoundsIsMalformed)
  {
    // ADR-087 item 2: shares that do not sum to 100, a turret id of 0 and a point that is not finite fail the decoding;
    // a placement's refusal travels in its outcome's reason, which version 2 of the report carries.
    Assert::IsTrue(Decodes<Species::SetPrioritiesCommand>(Neuron::EncodeMessage(Species::SetPrioritiesCommand{2, {50, 30, 20}})),
                   L"shares of 100");
    Assert::IsTrue(Decodes<Species::SetPrioritiesCommand>(Neuron::EncodeMessage(Species::SetPrioritiesCommand{2, {0, 0, 100}})),
                   L"one share of 100");
    Assert::IsFalse(Decodes<Species::SetPrioritiesCommand>(Neuron::EncodeMessage(Species::SetPrioritiesCommand{2, {50, 30, 19}})),
                    L"shares of 99");
    Assert::IsFalse(Decodes<Species::RebuildTurretCommand>(Neuron::EncodeMessage(Species::RebuildTurretCommand{2, 0})), L"an id of 0");
    Assert::IsFalse(Decodes<Species::PlaceStructureCommand>(Neuron::EncodeMessage(
                      Species::PlaceStructureCommand{2, Species::StructureKind::Dwelling, {std::numeric_limits<double>::infinity(), 0.0}})),
                    L"a point that is not finite");
    const std::vector<std::byte> place =
      Neuron::EncodeMessage(Species::PlaceStructureCommand{2, Species::StructureKind::Storehouse, {33'000.0, 31'000.0}});
    ExpectEqual(static_cast<std::int64_t>(Species::PlaceStructureCommand::MAX_BYTES), static_cast<std::int64_t>(place.size()),
                L"a placement at its bound");
    Assert::IsTrue(Decodes<Species::PlaceStructureCommand>(place), L"a placement decodes");

    const Species::CommandReport report{9, 41, Species::CommandResult::NoEffect,
                                        static_cast<std::uint8_t>(Species::PlacementRefusal::CannotAfford)};
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(report);
    ExpectEqual(static_cast<std::int64_t>(Species::CommandReport::MAX_BYTES), static_cast<std::int64_t>(bytes.size()), L"the report");
    Neuron::MessageDispatcher dispatcher;
    std::optional<Species::CommandReport> received;
    dispatcher.Register<Species::CommandReport>([&received](const Species::CommandReport& _report) { received = _report; });
    Assert::IsTrue(dispatcher.Dispatch(bytes) && received && received->sequence == 9 && received->tick == 41 &&
                     received->result == Species::CommandResult::NoEffect && received->reason == 7,
                   L"its sequence, tick, result and reason");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp102ANoticeRoundTripsAtItsBound)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp102ANoticeRoundTripsAtItsBound)
  {
    // ADR-139, ADR-165: a guaranteed event of the HUD's notifications reads back whole at its bound; one with no subject
    // is malformed, but for contact and the run's, which name none.
    Species::SettlementNotice notice{
      (std::uint64_t{1} << 63) + 5, Species::NoticeKind::RunEnded,  2,
      0x0100'0000'0000'0042,        Species::StructureKind::Turret, Species::QuantizedPosition{(1 << 23) - 1, -(1 << 19), -(1 << 23)}};
    notice.other = std::uint8_t{3};
    notice.ours = true;
    notice.detail = 255;
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(notice);
    ExpectEqual(static_cast<std::int64_t>(Species::SettlementNotice::MAX_BYTES), static_cast<std::int64_t>(bytes.size()), L"at its bound");
    Neuron::MessageDispatcher dispatcher;
    std::optional<Species::SettlementNotice> received;
    dispatcher.Register<Species::SettlementNotice>([&received](const Species::SettlementNotice& _notice) { received = _notice; });
    Assert::IsTrue(dispatcher.Dispatch(bytes) && received && received->tick == notice.tick && received->kind == notice.kind &&
                     received->team == 2 && received->subject == notice.subject && received->structure == Species::StructureKind::Turret &&
                     received->position == notice.position && received->other == std::optional<std::uint8_t>(std::uint8_t{3}) &&
                     received->ours && received->detail == 255,
                   L"every field");
    Species::SettlementNotice none = notice;
    none.other.reset();
    received.reset();
    Assert::IsTrue(dispatcher.Dispatch(Neuron::EncodeMessage(none)) && received && !received->other.has_value(), L"no other site");
    for (const Species::NoticeKind kind : {Species::NoticeKind::Contact, Species::NoticeKind::RunQuiet, Species::NoticeKind::RunEnded})
    {
      Species::SettlementNotice nameless = notice;
      nameless.kind = kind;
      nameless.subject = 0;
      Assert::IsTrue(Decodes<Species::SettlementNotice>(Neuron::EncodeMessage(nameless)), L"contact and the run name no subject");
    }
    Species::SettlementNotice nobody = notice;
    nobody.kind = Species::NoticeKind::StructureCompleted;
    nobody.subject = 0;
    Assert::IsFalse(Decodes<Species::SettlementNotice>(Neuron::EncodeMessage(nobody)), L"no subject");
  }
};

} // namespace GameSharedTests
