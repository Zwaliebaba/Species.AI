#include "pch.h"

#include "Battles.h"
#include "BitWriter.h"
#include "Claims.h"
#include "CommandJournal.h"
#include "CommandMessages.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "RelationMessages.h"
#include "SettlementMessages.h"
#include "TestSupport.h"
#include "WorldServer.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2; // TestSupport's valley: the steward at site 2, valley neighbour of site 3 (pair 1)
constexpr std::uint8_t NEIGHBOR_SITE = 3;
constexpr std::uint8_t NEIGHBORS_HALF = 3; // pair 1's half nearer site 3
constexpr Neuron::ConnectionId STEWARD_CONNECTION = 1;
constexpr Neuron::SenderId STEWARD = 7;
// The contested action's run (ADR-087's confirmation): its ticks, and the shares each sender wins within.
constexpr std::uint64_t CONTESTED_TICKS = 1'000;
constexpr double SHARE_LEAST = 0.45;
constexpr double SHARE_MOST = 0.55;
// Sequence numbers of the test's own, above any an AI settlement's commander reaches in the run.
constexpr std::uint64_t TEST_SEQUENCE_FIRST = 1'000'000'000;

// What a connection was sent, the reliable channel's messages kept.
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

  // A sender of a test closes nothing: its server is given no identity, so it refuses no join (ADR-186).
  void Close(Neuron::ConnectionId /*_connection*/) override {}

  std::vector<std::vector<std::byte>> reliable;
  std::uint64_t datagrams = 0;
};

// The command reports among what a server published.
std::vector<Species::CommandReport> ReportsOf(Species::WorldServer& _server, std::uint64_t _tick)
{
  KeptMessages kept;
  _server.Publish(_tick, kept);
  std::vector<Species::CommandReport> reports;
  for (const std::vector<std::byte>& message : kept.reliable)
  {
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<Species::CommandReport>([&reports](const Species::CommandReport& _report) { reports.push_back(_report); });
    static_cast<void>(dispatcher.Dispatch(message));
  }
  return reports;
}

// A command's message with its last _bytes cut off, as a test client that leaves out a field sends it.
template <class T> std::vector<std::byte> Truncated(const T& _command, std::size_t _bytes)
{
  std::vector<std::byte> message = Neuron::EncodeMessage(_command);
  message.resize(message.size() - _bytes);
  return message;
}

std::uint8_t ReasonOf(Species::AttackRefusal _refusal)
{
  return static_cast<std::uint8_t>(_refusal);
}

} // namespace

TEST_CLASS(CommandPathTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094AForgedCommandIsRefusedWithItsReasonAndCounted)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094AForgedCommandIsRefusedWithItsReasonAndCounted)
  {
    // ADR-087's confirmation on M3's build (ADR-164): the forged commands of a test client in the valley, each refused
    // with its reason and its counter: another settlement's order, a missing field, and a pool slot in place of an id.
    Species::WorldServer::Desc desc;
    desc.tuning = CommittedTuning();
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = STEWARD_SITE;
    desc.aiSettlements = 3;
    Species::WorldServer server(desc);
    std::uint64_t tick = server.Open();
    server.Connect(STEWARD_CONNECTION);
    const auto send = [&server](std::span<const std::byte> _message)
    {
      server.Receive(STEWARD_CONNECTION,
                     Neuron::ReceivedMessage{Neuron::Channel::Reliable, std::vector<std::byte>(_message.begin(), _message.end())});
    };
    Neuron::WorkerPool pool(0);

    // The steward promotes one of its Workers, whose pool slot the forged declaration names in place of its id.
    const Species::RegionSimulation& region = server.Simulation()->Region(0);
    const auto& figures = region.TeamAt(STEWARD_SITE).figures;
    std::uint32_t slot = 0;
    while (!figures.Used(slot))
    {
      ++slot;
    }
    send(Neuron::EncodeMessage(Species::PromoteWorkerCommand{STEWARD_SITE, figures.At(slot).id}));
    server.Step(tick, pool);
    Assert::IsTrue(ReportsOf(server, tick++).front().result == Species::CommandResult::Applied, L"promoted");
    slot = 0;
    while (slot < figures.Capacity() && !(figures.Used(slot) && figures.At(slot).kind == Species::FigureKind::Leader))
    {
      ++slot;
    }
    Assert::IsTrue(slot < figures.Capacity(), L"a Leader of the steward's");
    const Species::StableId leader = figures.At(slot).id;
    for (std::uint32_t other = 0; other < figures.Capacity(); ++other)
    {
      Assert::IsFalse(figures.Used(other) && figures.At(other).id == slot, L"no figure of the team has the slot's number as its id");
    }

    const Species::DeclareAttackCommand declaration{STEWARD_SITE, {leader, 0, 0}, NEIGHBOR_SITE, 0};
    send(Neuron::EncodeMessage(Species::DeclareAttackCommand{NEIGHBOR_SITE, {}, STEWARD_SITE, 0}));
    send(Neuron::EncodeMessage(Species::MakeClaimCommand{NEIGHBOR_SITE, NEIGHBORS_HALF}));
    send(Neuron::EncodeMessage(Species::SendTributeCommand{NEIGHBOR_SITE, STEWARD_SITE, 0, 0, 1'000}));
    send(Neuron::EncodeMessage(Species::PlaceStructureCommand{NEIGHBOR_SITE, Species::StructureKind::Dwelling, {0.0, 0.0}}));
    send(Truncated(declaration, sizeof(Species::StableId)));
    send(Truncated(Species::MakeClaimCommand{STEWARD_SITE, NEIGHBORS_HALF}, 1));
    send(Neuron::EncodeMessage(Species::DeclareAttackCommand{STEWARD_SITE, {slot, 0, 0}, NEIGHBOR_SITE, 0}));
    server.Step(tick, pool);
    const std::vector<Species::CommandReport> reports = ReportsOf(server, tick);
    ExpectEqual(7, static_cast<std::int64_t>(reports.size()), L"an outcome for each");
    const auto result = [&reports](std::uint64_t _sequence)
    {
      const auto found = std::ranges::find(reports, _sequence, &Species::CommandReport::sequence);
      Assert::IsTrue(found != reports.end(), L"the outcome of each sequence number");
      return *found;
    };
    for (const std::uint64_t sequence : std::array<std::uint64_t, 4>{1, 2, 3, 4})
    {
      Assert::IsTrue(result(sequence).result == Species::CommandResult::NotPermitted,
                     L"another settlement's declaration, claim, tribute and placement are not the steward's");
    }
    for (const std::uint64_t sequence : std::array<std::uint64_t, 2>{5, 6})
    {
      Assert::IsTrue(result(sequence).result == Species::CommandResult::Malformed, L"a command with a field missing is malformed");
    }
    Assert::IsTrue(result(7).result == Species::CommandResult::NoEffect && result(7).reason == ReasonOf(Species::AttackRefusal::NoLeader),
                   L"a pool slot names no Leader, and the declaration is refused with its reason");
    Assert::IsTrue(std::ranges::none_of(region.Expeditions(), [](const Species::Expedition& _expedition)
                                        { return _expedition.attacker == STEWARD_SITE || _expedition.target == STEWARD_SITE; }) &&
                     std::ranges::none_of(region.Claims(), [](const Species::Claim& _claim) { return _claim.area == NEIGHBORS_HALF; }),
                   L"none of them declared an attack or claimed the half");
    const Neuron::CommandRejectionCounts& counts = region.Commands().Rejections();
    ExpectEqual(4, static_cast<std::int64_t>(counts.notPermitted), L"each not permitted counted");
    ExpectEqual(2, static_cast<std::int64_t>(counts.malformed), L"each malformed counted");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094ACrowdOrderForAGroupEmptiedBeforeItsTickDoesNothing)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-140")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094ACrowdOrderForAGroupEmptiedBeforeItsTickDoesNothing)
  {
    // ADR-087 item 3 on M3's build (ADR-164): the steward selects Workers of its settlement and orders them; the
    // Workers die between the order's arrival and its tick, so the order, checked again against that tick, does nothing.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    Species::RegionSimulation& region = world->Region(0);
    region.SetCommander(STEWARD_SITE, STEWARD, false);
    const Species::Settlement& settlement = *region.SettlementAt(STEWARD_SITE);
    const Neuron::Vec3 home = region.ResolveStructure(settlement.wellspringSlot, settlement.wellspringId)->position;
    std::uint64_t sequence = 0;
    const auto executed = [&](const auto& _command)
    {
      Assert::IsTrue(region.Commands().Submit(STEWARD, sequence++, Neuron::EncodeMessage(_command)), L"permitted");
      region.Commands().Execute(region.NextTick());
      const std::vector<Neuron::CommandOutcome> outcomes = region.Commands().TakeOutcomes();
      ExpectEqual(1, static_cast<std::int64_t>(outcomes.size()), L"one outcome");
      return outcomes.front();
    };
    Assert::IsTrue(executed(Species::SelectWorkersCommand{STEWARD_SITE, {home.x, home.z}, 400.0}).effect == Neuron::CommandEffect::Applied,
                   L"the steward selects its Workers");
    auto& figures = region.TeamAt(STEWARD_SITE).figures;
    std::vector<std::uint32_t> group;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (figures.Used(slot) && figures.At(slot).underControl)
      {
        group.push_back(slot);
      }
    }
    Assert::IsFalse(group.empty(), L"a group under control");

    const Species::GroundPoint target{home.x + 300.0, home.z};
    Assert::IsTrue(
      region.Commands().Submit(STEWARD, sequence++, Neuron::EncodeMessage(Species::OrderWorkersCommand{STEWARD_SITE, false, target})),
      L"the order arrives and is queued");
    for (const std::uint32_t slot : group)
    {
      figures.At(slot).dead = true;
    }
    region.Commands().Execute(region.NextTick());
    const std::vector<Neuron::CommandOutcome> outcomes = region.Commands().TakeOutcomes();
    Assert::IsTrue(outcomes.size() == 1 && !outcomes.front().rejection && outcomes.front().effect == Neuron::CommandEffect::NoEffect,
                   L"executed, with no effect");
    Assert::IsTrue(std::ranges::none_of(group, [&](std::uint32_t _slot)
                                        { return figures.At(_slot).workerState == Species::WorkerState::OnWaypointOrder; }),
                   L"no Worker of the emptied group was ordered");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094TheStewardAndAnAiSettlementWinAContestedTickInEqualShares)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094TheStewardAndAnAiSettlementWinAContestedTickInEqualShares)
  {
    // ADR-087 item 4 on M3's build (ADR-164): the steward and an AI settlement send a command that reaches the same tick,
    // every tick for 1,000 ticks of the valley; the keyed order of the world's key executes the steward's first in
    // 45 % to 55 % of them. At the MVP no two senders command one object, so what the order decides between them is
    // which executes first.
    const std::unique_ptr<Species::WorldSimulation> world = ValleyWorld();
    const Neuron::SenderId steward = Species::ConnectSteward(*world, false);
    Species::RegionSimulation& region = world->Region(0);
    const Species::RegionSimulation::Commander ai = region.CommanderOf(NEIGHBOR_SITE);
    Assert::IsTrue(ai.ai && ai.sender != 0 && ai.sender != steward, L"the neighbour is an AI settlement with a sender of its own");
    Neuron::WorkerPool pool(0);
    std::uint64_t stewardFirst = 0;
    for (std::uint64_t step = 0; step < CONTESTED_TICKS; ++step)
    {
      const std::uint64_t sequence = TEST_SEQUENCE_FIRST + step;
      Assert::IsTrue(
        region.Commands().Submit(steward, sequence, Neuron::EncodeMessage(Species::SetPrioritiesCommand{STEWARD_SITE, {50, 30, 20}})),
        L"the steward's");
      Assert::IsTrue(
        region.Commands().Submit(ai.sender, sequence, Neuron::EncodeMessage(Species::SetPrioritiesCommand{NEIGHBOR_SITE, {50, 30, 20}})),
        L"the AI settlement's");
      world->Step(world->NextTick(), pool);
      const std::vector<Neuron::CommandOutcome> outcomes = region.Commands().TakeOutcomes();
      const auto first = std::ranges::find_if(outcomes, [sequence](const Neuron::CommandOutcome& _outcome)
                                              { return _outcome.sequence == sequence && !_outcome.rejection; });
      Assert::IsTrue(first != outcomes.end(), L"both executed in the tick");
      stewardFirst += first->sender == steward ? 1U : 0U;
    }
    const double share = static_cast<double>(stewardFirst) / static_cast<double>(CONTESTED_TICKS);
    Assert::IsTrue(share >= SHARE_LEAST && share <= SHARE_MOST,
                   std::format(L"the steward first in {:.1f} % of the ticks", share * 100.0).c_str());
  }
};

} // namespace GameLogicTests
