#include "pch.h"

#include "AwayMessages.h"
#include "MessageCodec.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <optional>
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
    // ADR-185: the report carries nothing but its type and version, and decodes from its header alone.
    const std::vector<std::byte> report = Neuron::EncodeMessage(Species::PresenceReportCommand{});
    ExpectEqual(static_cast<std::int64_t>(Species::PresenceReportCommand::MAX_BYTES), static_cast<std::int64_t>(report.size()),
                L"the report at its bound");
    Neuron::MessageDispatcher dispatcher;
    bool received = false;
    dispatcher.Register<Species::PresenceReportCommand>([&received](const Species::PresenceReportCommand&) { received = true; });
    Assert::IsTrue(dispatcher.Dispatch(report) && received, L"it decodes");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp074AnAwayReportAtEveryBoundRoundTrips)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-074")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp074AnAwayReportAtEveryBoundRoundTrips)
  {
    // ADR-187: a report with every list at its bound fits a reliable message and reads back whole; one without an absence
    // is its header, tick, team and two flags; a deputy action of an order that does not exist is malformed.
    Species::AwayReport full;
    full.tick = 1'000'000;
    full.team = 2;
    full.absence = true;
    full.onReturn = true;
    full.fromTick = 10;
    full.toTick = 999'999;
    full.workersFrom = 120;
    full.workersTo = 90;
    full.storesFromMilliRu = 5'000'000;
    full.storesToMilliRu = 1'250;
    full.structuresCompleted = {3, 1, 2};
    full.battles.assign(Species::AWAY_BATTLES_MAX, {3, true, false, Species::BattleEnd::LossCap, 500, 90, -12'000, true});
    full.grievances.assign(Species::AWAY_GRIEVANCES_MAX, {400, 3, 2, Species::GrievanceCause::Attack});
    full.claims.assign(Species::AWAY_CLAIMS_MAX, {600, 2, std::uint8_t{2}, 3});
    full.claims.back().from.reset();
    full.freezes.assign(Species::AWAY_FREEZES_MAX, {700, Species::FreezeCause::UncleanStop, 36'000});
    full.actions.assign(Species::AWAY_ACTIONS_MAX, {800, Species::DeputyActionKind::LeaderToObjective,
                                                    Species::StandingOrder::TheStewardsAttack, 0x0001'0000'0000'0042, 3});
    full.grievancesTotal = 300;
    full.actionsTotal = 5'000;
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(full);
    Assert::IsTrue(bytes.size() <= Species::AwayReport::MAX_BYTES, L"within a reliable message");
    Neuron::MessageDispatcher dispatcher;
    std::optional<Species::AwayReport> read;
    dispatcher.Register<Species::AwayReport>([&read](const Species::AwayReport& _report) { read = _report; });
    Assert::IsTrue(dispatcher.Dispatch(bytes) && read.has_value() && *read == full && read->HasNews(), L"reads back whole");

    Species::AwayReport none;
    none.tick = 5;
    none.team = 2;
    read.reset();
    Assert::IsTrue(dispatcher.Dispatch(Neuron::EncodeMessage(none)) && read.has_value() && *read == none && !read->HasNews(),
                   L"no absence, no news");

    Species::AwayReport wrong = full;
    wrong.actions.assign(1, {800, Species::DeputyActionKind::LeaderToObjective, static_cast<Species::StandingOrder>(9), 1, 3});
    read.reset();
    Assert::IsFalse(dispatcher.Dispatch(Neuron::EncodeMessage(wrong)) && read.has_value(), L"an order that does not exist is malformed");
  }
};

} // namespace GameSharedTests
