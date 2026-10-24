#include "pch.h"

#include "CommandMessages.h"
#include "Construction.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "ServerHost.h"
#include "SettlementCommands.h"
#include "SettlementMessages.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldTerrain.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_TEAM = 2; // SettledWorld's settlement, whose team is its site
constexpr Neuron::SenderId STEWARD = 7;
constexpr Neuron::SenderId AI = 9;
constexpr double PI = 3.141592653589793;

std::uint8_t ReasonOf(Species::PlacementRefusal _refusal)
{
  return static_cast<std::uint8_t>(_refusal);
}

// SettledWorld's region, whose settlement the steward commands, and the senders' sequence numbers.
struct Settled
{
  std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
  Species::RegionSimulation& region = world->Region(0);
  std::uint64_t sequence = 0;

  Settled()
  {
    region.SetCommander(SETTLED_TEAM, STEWARD, false);
  }

  Species::Settlement& Settlement()
  {
    return region.Settlements().front();
  }
  template <class T> bool Submit(Neuron::SenderId _sender, const T& _command)
  {
    return region.Commands().Submit(_sender, sequence++, Neuron::EncodeMessage(_command));
  }
  // The queued commands executed alone, as the next tick starts, and the outcomes since the last call.
  std::vector<Neuron::CommandOutcome> Execute()
  {
    region.Commands().Execute(region.NextTick());
    return region.Commands().TakeOutcomes();
  }
  // The one command executed, and its outcome.
  template <class T> Neuron::CommandOutcome Executed(const T& _command)
  {
    Assert::IsTrue(Submit(STEWARD, _command), L"permitted");
    const std::vector<Neuron::CommandOutcome> outcomes = Execute();
    ExpectEqual(1, static_cast<std::int64_t>(outcomes.size()), L"one outcome");
    return outcomes.front();
  }
  // A point _distance wu from the Wellspring at _degrees.
  Species::GroundPoint Around(double _degrees, double _distance)
  {
    const Neuron::Vec3 wellspring = region.Structures().At(Settlement().wellspringSlot).position;
    return {wellspring.x + _distance * std::cos(_degrees * PI / 180.0), wellspring.z + _distance * std::sin(_degrees * PI / 180.0)};
  }
};

// A reporting end that keeps what the server sends on the reliable channel.
class KeptMessages final : public Neuron::ConnectionSender
{
public:
  [[nodiscard]] std::optional<std::uint64_t> Send(Neuron::ConnectionId, Neuron::Channel _channel,
                                                  std::span<const std::byte> _message) override
  {
    if (_channel == Neuron::Channel::Reliable)
    {
      reliable.emplace_back(_message.begin(), _message.end());
      return 0;
    }
    return ++datagrams;
  }

  // A sender of a test closes nothing: its server is given no identity, so it refuses no join (ADR-189).
  void Close(Neuron::ConnectionId /*_connection*/) override {}

  std::vector<std::vector<std::byte>> reliable;
  std::uint64_t datagrams = 0;
};

} // namespace

TEST_CLASS(SettlementCommandTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp101APlacementIsACommandWhoseOutcomeGivesItsRefusal)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-032")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp101APlacementIsACommandWhoseOutcomeGivesItsRefusal)
  {
    // A placement that a check refuses has no effect and gives its PlacementRefusal; one that every check passes pays
    // and queues its site. Each outcome names the tick that executed it.
    Settled settled;
    const auto refused =
      [&](Species::StructureKind _kind, const Species::GroundPoint& _point, Species::PlacementRefusal _refusal, const wchar_t* _what)
    {
      const std::uint64_t tick = settled.region.NextTick();
      const Neuron::CommandOutcome outcome = settled.Executed(Species::PlaceStructureCommand{SETTLED_TEAM, _kind, _point});
      Assert::IsTrue(!outcome.rejection && outcome.effect == Neuron::CommandEffect::NoEffect, _what);
      ExpectEqual(ReasonOf(_refusal), outcome.reason, _what);
      ExpectEqual(static_cast<std::int64_t>(tick), static_cast<std::int64_t>(outcome.tick), L"the tick that executed it");
    };
    refused(Species::StructureKind::Wellspring, settled.Around(0.0, 150.0), Species::PlacementRefusal::NotBuildable, L"a Wellspring");
    refused(Species::StructureKind::Dwelling, settled.Around(0.0, 5'000.0), Species::PlacementRefusal::OutsideTerritory, L"far away");
    refused(Species::StructureKind::Dwelling, settled.Around(0.0, 0.0), Species::PlacementRefusal::InCenterRing, L"at the Wellspring");

    // The first place round the Wellspring that only the stores refuse, then placed there once they hold its cost.
    settled.Settlement().storesMilliRu = 0;
    std::optional<Species::GroundPoint> affordable;
    for (int turn = 0; turn < 36 && !affordable; ++turn)
    {
      const Species::GroundPoint point = settled.Around(turn * 10.0, 150.0);
      const Neuron::CommandOutcome outcome =
        settled.Executed(Species::PlaceStructureCommand{SETTLED_TEAM, Species::StructureKind::Dwelling, point});
      if (outcome.reason == ReasonOf(Species::PlacementRefusal::CannotAfford))
      {
        affordable = point;
      }
    }
    Assert::IsTrue(affordable.has_value(), L"a place that only the stores refuse");
    const Species::GroundPoint place = affordable.value_or(Species::GroundPoint{});
    settled.Settlement().storesMilliRu = Species::MilliRuOf(1'000.0);
    const Neuron::CommandOutcome placed =
      settled.Executed(Species::PlaceStructureCommand{SETTLED_TEAM, Species::StructureKind::Dwelling, place});
    Assert::IsTrue(placed.effect == Neuron::CommandEffect::Applied, L"placed");
    ExpectEqual(0, placed.reason, L"no reason");
    ExpectEqual(1, static_cast<std::int64_t>(settled.Settlement().queue.size()), L"its site queued");
    ExpectEqual(Species::MilliRuOf(700.0), settled.Settlement().storesMilliRu, L"300 ru paid");
    refused(Species::StructureKind::Dwelling, place, Species::PlacementRefusal::Overlaps, L"on the site");

    // An AI that commands the settlement places through the same path (DEC-122).
    settled.region.SetCommander(SETTLED_TEAM, AI, true);
    Assert::IsFalse(settled.Submit(STEWARD, Species::SetPrioritiesCommand{SETTLED_TEAM, {50, 30, 20}}),
                    L"the steward no longer commands it");
    static_cast<void>(settled.region.Commands().TakeOutcomes());
    Assert::IsTrue(
      settled.Submit(AI, Species::PlaceStructureCommand{SETTLED_TEAM, Species::StructureKind::Dwelling, settled.Around(0.0, 0.0)}),
      L"the AI's placement");
    const std::vector<Neuron::CommandOutcome> outcomes = settled.Execute();
    Assert::IsTrue(outcomes.size() == 1 && outcomes.front().reason == ReasonOf(Species::PlacementRefusal::InCenterRing),
                   L"checked as the steward's is");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp036ThePrioritiesAreACommand)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-036")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp036ThePrioritiesAreACommand)
  {
    // The shares hold from the tick the command executes in; a team the steward commands that has no settlement sets
    // nothing.
    Settled settled;
    const Neuron::CommandOutcome set = settled.Executed(Species::SetPrioritiesCommand{SETTLED_TEAM, {20, 30, 50}});
    Assert::IsTrue(set.effect == Neuron::CommandEffect::Applied, L"applied");
    Assert::IsTrue(settled.Settlement().priorities == std::array<std::uint8_t, 3>{20, 30, 50}, L"the shares set");
    settled.region.SetCommander(0, STEWARD, false);
    const Neuron::CommandOutcome none = settled.Executed(Species::SetPrioritiesCommand{0, {100, 0, 0}});
    Assert::IsTrue(none.effect == Neuron::CommandEffect::NoEffect, L"no settlement on team 0");
    Assert::IsTrue(settled.Settlement().priorities == std::array<std::uint8_t, 3>{20, 30, 50}, L"unchanged");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp024TheGrowthHoldIsACommand)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-024")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp024TheGrowthHoldIsACommand)
  {
    // O106 (ADR-178): the hold holds from the tick it executes in, and its status says so before any other cause; a hold
    // the settlement already has does nothing; a scripted steward that takes the settlement lets it go.
    Settled settled;
    Assert::IsTrue(settled.Executed(Species::HoldGrowthCommand{SETTLED_TEAM, true}).effect == Neuron::CommandEffect::Applied &&
                     settled.Settlement().growthHeld,
                   L"held");
    settled.Settlement().storesMilliRu = 0; // short as well
    Assert::IsTrue(Species::StatusOf(settled.region, settled.Settlement(), 0).growth == Species::GrowthState::Held,
                   L"the status says held");
    Assert::IsTrue(settled.Executed(Species::HoldGrowthCommand{SETTLED_TEAM, true}).effect == Neuron::CommandEffect::NoEffect,
                   L"held already");
    Assert::IsTrue(settled.Executed(Species::HoldGrowthCommand{SETTLED_TEAM, false}).effect == Neuron::CommandEffect::Applied &&
                     !settled.Settlement().growthHeld,
                   L"let go");
    Assert::IsTrue(Species::StatusOf(settled.region, settled.Settlement(), 0).growth == Species::GrowthState::StoresShort,
                   L"then its own cause");
    settled.Settlement().growthHeld = true;
    settled.region.AddScriptedSteward(SETTLED_TEAM);
    Assert::IsFalse(settled.Settlement().growthHeld, L"a scripted steward lets it go");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp034ARebuildIsACommand)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp034ARebuildIsACommand)
  {
    // A destroyed turret's site is rebuilt by its stable id; an id that names no such site is not buildable, and empty
    // stores cannot afford it.
    Settled settled;
    const Species::GroundPoint point = settled.Around(0.0, 120.0);
    const std::uint32_t slot =
      settled.region.AddStructure(Species::StructureKind::Turret, SETTLED_TEAM, std::nullopt, point.x, point.z, {0.0, 0.0, 1.0});
    Species::Structure& turret = settled.region.Structures().At(slot);
    turret.built = false; // destroyed: a site that is not queued (ADR-133)
    turret.costMilliRu = Species::MilliRuOf(200.0);
    const Species::StableId wellspring = settled.Settlement().wellspringId;
    Assert::IsTrue(settled.Executed(Species::RebuildTurretCommand{SETTLED_TEAM, wellspring}).reason ==
                     ReasonOf(Species::PlacementRefusal::NotBuildable),
                   L"the Wellspring is no turret site");
    settled.Settlement().storesMilliRu = 0;
    Assert::IsTrue(settled.Executed(Species::RebuildTurretCommand{SETTLED_TEAM, turret.id}).reason ==
                     ReasonOf(Species::PlacementRefusal::CannotAfford),
                   L"empty stores");
    settled.Settlement().storesMilliRu = Species::MilliRuOf(250.0);
    Assert::IsTrue(settled.Executed(Species::RebuildTurretCommand{SETTLED_TEAM, turret.id}).effect == Neuron::CommandEffect::Applied,
                   L"rebuilt");
    Assert::IsTrue(Species::Queued(settled.region, settled.region.Structures().At(slot)), L"queued");
    ExpectEqual(Species::MilliRuOf(50.0), settled.Settlement().storesMilliRu, L"200 ru paid");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094EverySettlementCommandRefusalIsCounted)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094EverySettlementCommandRefusalIsCounted)
  {
    // A sender commands only its own settlement, and a field out of its bounds is malformed (ADR-087 item 2); a status
    // request is no queued command. Every refusal is counted and names the tick it would have executed in.
    Settled settled;
    const std::uint64_t tick = settled.region.NextTick();
    const auto refused = [&](const wchar_t* _what, Neuron::CommandRejection _reason, bool _submitted)
    {
      Assert::IsFalse(_submitted, _what);
      const std::vector<Neuron::CommandOutcome> outcomes = settled.region.Commands().TakeOutcomes();
      Assert::IsTrue(outcomes.size() == 1 && outcomes.front().rejection == _reason, _what);
      ExpectEqual(static_cast<std::int64_t>(tick), static_cast<std::int64_t>(outcomes.front().tick), L"the tick it would have executed in");
    };
    const Species::GroundPoint point = settled.Around(0.0, 150.0);
    refused(L"another team", Neuron::CommandRejection::NotPermitted,
            settled.Submit(STEWARD, Species::PlaceStructureCommand{1, Species::StructureKind::Dwelling, point}));
    refused(L"a sender who commands nothing", Neuron::CommandRejection::NotPermitted,
            settled.Submit(42, Species::SetPrioritiesCommand{SETTLED_TEAM, {50, 30, 20}}));
    refused(L"shares of 99", Neuron::CommandRejection::Malformed,
            settled.Submit(STEWARD, Species::SetPrioritiesCommand{SETTLED_TEAM, {50, 30, 19}}));
    refused(L"a turret id of 0", Neuron::CommandRejection::Malformed,
            settled.Submit(STEWARD, Species::RebuildTurretCommand{SETTLED_TEAM, 0}));
    refused(L"a point that is not finite", Neuron::CommandRejection::Malformed,
            settled.Submit(STEWARD, Species::PlaceStructureCommand{SETTLED_TEAM,
                                                                   Species::StructureKind::Dwelling,
                                                                   {std::numeric_limits<double>::quiet_NaN(), point.z}}));
    refused(L"a status request", Neuron::CommandRejection::UnknownType,
            settled.Submit(STEWARD, Species::ReadSettlementStatusCommand{SETTLED_TEAM}));
    const Neuron::CommandRejectionCounts& counts = settled.region.Commands().Rejections();
    ExpectEqual(6, static_cast<std::int64_t>(counts.Total()), L"every refusal counted");
    ExpectEqual(2, static_cast<std::int64_t>(counts.notPermitted), L"not permitted");
    ExpectEqual(3, static_cast<std::int64_t>(counts.malformed), L"malformed");
    ExpectEqual(1, static_cast<std::int64_t>(counts.unknownType), L"unknown");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp098TheStatusIsReadThroughServerMessages)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-037")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp098TheStatusIsReadThroughServerMessages)
  {
    // A steward asks for its settlement's status and receives the outcome and, after the next tick, the status of that
    // tick; a request for a team it does not command is not permitted and answered with no status. A placement's
    // refusal reaches the steward in its outcome.
    Species::WorldServer::Desc desc;
    desc.tuning = CommittedTuning();
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = SETTLED_TEAM;
    Species::WorldServer server(desc);
    const std::uint64_t first = server.Open();
    server.Connect(1);
    const auto send = [&server](const auto& _command)
    { server.Receive(1, Neuron::ReceivedMessage{Neuron::Channel::Reliable, Neuron::EncodeMessage(_command)}); };
    send(Species::ReadSettlementStatusCommand{SETTLED_TEAM});
    send(Species::ReadSettlementStatusCommand{1});
    send(Species::PlaceStructureCommand{SETTLED_TEAM, Species::StructureKind::Dwelling, {0.0, 0.0}});
    Neuron::WorkerPool pool(0);
    server.Step(first, pool);
    KeptMessages kept;
    server.Publish(first, kept);

    std::vector<Species::CommandReport> reports;
    std::vector<Species::SettlementStatus> statuses;
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<Species::CommandReport>([&reports](const Species::CommandReport& _report) { reports.push_back(_report); });
    dispatcher.Register<Species::SettlementStatus>([&statuses](const Species::SettlementStatus& _status) { statuses.push_back(_status); });
    dispatcher.Register<Species::WorldTerrain>([](const Species::WorldTerrain&) {});
    for (const std::vector<std::byte>& message : kept.reliable)
    {
      Assert::IsTrue(dispatcher.Dispatch(message), L"every message decodes");
    }
    ExpectEqual(3, static_cast<std::int64_t>(reports.size()), L"an outcome for each");
    Assert::IsTrue(reports[0].sequence == 0 && reports[0].result == Species::CommandResult::Applied && reports[0].tick == first,
                   L"the request, with the tick of its status");
    Assert::IsTrue(reports[1].sequence == 1 && reports[1].result == Species::CommandResult::NotPermitted, L"another team's");
    Assert::IsTrue(reports[2].sequence == 2 && reports[2].result == Species::CommandResult::NoEffect &&
                     reports[2].reason == ReasonOf(Species::PlacementRefusal::OutsideTerritory),
                   L"the placement's refusal");
    ExpectEqual(1, static_cast<std::int64_t>(statuses.size()), L"one status");
    const Species::RegionSimulation& region = server.Simulation()->Region(0);
    const Species::SettlementStatus expected = Species::StatusOf(region, region.Settlements().front(), first);
    Assert::IsTrue(Neuron::EncodeMessage(statuses.front()) == Neuron::EncodeMessage(expected), L"the settlement's status after the tick");
    ExpectEqual(static_cast<std::int64_t>(first), static_cast<std::int64_t>(statuses.front().tick), L"its tick");
    ExpectEqual(CommittedTuning().mvp.settlement.startWorkers, statuses.front().population, L"its founding Workers");
    Assert::IsTrue(statuses.front().priorities == std::array<std::uint8_t, 3>{50, 30, 20}, L"the default shares");
  }
};

} // namespace GameLogicTests
