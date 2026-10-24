#include "pch.h"

#include "AwayMessages.h"
#include "BitReader.h"
#include "ClientSession.h"
#include "GameUi.h"
#include "InProcessTransport.h"
#include "MessageCodec.h"
#include "MessageLocale.h"
#include "PlatformClock.h"
#include "PresentationEvents.h"
#include "RegionTerrain.h"
#include "RelationMessages.h"
#include "ReplicaTiming.h"
#include "SettlementHud.h"
#include "SettlementMessages.h"
#include "StateUpdate.h"
#include "TestSupport.h"
#include "UiCore.h"
#include "UiLook.h"
#include "WorldMenu.h"
#include "WorldTerrain.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr Neuron::Nanoseconds MILLISECOND{1'000'000};

// The region of seed 1 as this client builds it: what an honest server sends.
const Species::RegionTerrain& SeedOne()
{
  static const Species::RegionTerrain REGION = []
  {
    auto generated = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(generated.has_value(), L"seed 1 gives a region that passes");
    return generated ? std::move(*generated) : Species::RegionTerrain{};
  }();
  return REGION;
}

/// A session on a manual clock (PLT-048) with the server's end of its transport, and the titles it showed.
struct Fixture
{
  explicit Fixture(std::optional<Species::JoinIdentity> _identity = std::nullopt, Species::WorldPace _pace = Species::WorldPace::Session)
    : clock(MakeSource())
  {
    auto [client, serverEnd] = Neuron::InProcessTransport::CreatePair({});
    server = std::move(serverEnd);
    Species::ClientSession::Desc desc;
    desc.transport = std::move(client);
    desc.showTitle = [this](std::string_view _title) { titles.emplace_back(_title); };
    desc.productName = "Species";
    desc.tuning = &CommittedTuning();
    desc.clock = &clock;
    desc.identity = _identity;
    desc.pace = _pace;
    session = std::make_unique<Species::ClientSession>(std::move(desc));
  }

  std::unique_ptr<Neuron::TimeSource> MakeSource()
  {
    auto source = std::make_unique<Neuron::ManualTimeSource>();
    manual = source.get();
    return source;
  }

  template <class T> void Send(const T& _message)
  {
    Assert::IsTrue(server->Send(Neuron::Channel::State, Neuron::EncodeMessage(_message)).has_value(), L"sent");
  }

  Neuron::ManualTimeSource* manual = nullptr;
  Neuron::PlatformClock clock;
  std::unique_ptr<Neuron::Transport> server;
  std::vector<std::string> titles;
  std::unique_ptr<Species::ClientSession> session;
};

// The record MakeRecord makes, which the test expects to exist.
Species::StateRecord RecordOf(Species::StableId _id, const Species::ReplicatedState& _state, const Species::ReplicatedState* _baseline)
{
  const std::optional<Species::StateRecord> record = Species::MakeRecord(_id, _state, _baseline);
  Assert::IsTrue(record.has_value(), L"the state differs from its baseline");
  return record.value_or(Species::StateRecord{});
}

Species::StateRecord WholeFigure(std::uint64_t _counter, std::uint8_t _team, std::int32_t _x, bool _dead = false)
{
  Species::ReplicatedFigure figure;
  figure.team = _team;
  figure.position = {_x, 0, 0};
  figure.dead = _dead;
  return RecordOf(Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter), figure, nullptr);
}

} // namespace

TEST_CLASS(ClientSessionTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090TheSessionShowsTheReplicasLatestTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090TheSessionShowsTheReplicasLatestTick)
  {
    Fixture f;
    f.session->Update();
    Assert::IsFalse(f.session->ServerTick().has_value(), L"no state yet");
    Assert::IsTrue(f.titles.empty(), L"the title waits for the first state");
    Assert::IsTrue(f.session->Connected());

    for (std::uint64_t tick = 10; tick < 13; ++tick)
    {
      f.Send(Species::StateUpdate{tick, 0, {}});
    }
    f.session->Update();
    Assert::IsTrue(f.session->ServerTick() == std::optional<std::uint64_t>{12}, L"the latest of the updates that arrived");
    Assert::IsTrue(f.titles.size() == 1 && f.titles[0] == "Species — tick 12", L"one title a frame, with the latest tick");

    // A message the session cannot read is counted and changes nothing.
    const std::vector<std::byte> garbage{std::byte{0xff}, std::byte{0xff}, std::byte{0x01}};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, garbage).has_value());
    f.session->Update();
    Assert::IsTrue(f.session->ServerTick() == std::optional<std::uint64_t>{12});
    ExpectEqual(1, static_cast<std::int64_t>(f.titles.size()), L"an unchanged tick leaves the title");
    ExpectEqual(1, static_cast<std::int64_t>(f.session->Drops().Total()), L"drops");

    f.session->Close();
    Assert::IsFalse(f.session->Connected(), L"closed");
    Assert::IsTrue(f.server->State() == Neuron::ConnectionState::Closed, L"the server's end sees the close");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090TheTitleNamesTheTickAndEachSidesFigures)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090TheTitleNamesTheTickAndEachSidesFigures)
  {
    Assert::IsTrue(Species::SessionTitle("Species", std::nullopt, std::nullopt) == "Species", L"the product alone before a state");
    Assert::IsTrue(Species::SessionTitle("Species", std::nullopt, 1234) == "Species — tick 1234", L"the product and the tick");
    Assert::IsTrue(Species::SessionTitle("Species", 7, 1234) == "Species — seed 7 — tick 1234", L"the seed of MVP-005 too");
    const std::vector<std::uint32_t> sides = {200, 198};
    Assert::IsTrue(Species::SessionTitle("Species", 7, 1234, sides) == "Species — seed 7 — tick 1234 — figures 200 : 198", L"each side");
    Assert::IsTrue(Species::SessionTitle("Species", 7, 1234, sides, true) ==
                     "Species — seed 7 — tick 1234 — figures 200 : 198 — waiting for the server",
                   L"and the wait");
    // ADR-139: the client's own settlement, in whole resource units.
    const Species::ReplicatedSettlement settlement{2, 2, 2, 345'999, 1'500'000, 23, 40, 0};
    Assert::IsTrue(Species::SessionTitle("Species", 7, 1234, sides, false, settlement) ==
                     "Species — seed 7 — tick 1234 — figures 200 : 198 — stores 345 of 1500 — Workers 23 of 40",
                   L"its stores and Workers");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084TheTitleCountsEachSidesFiguresFromTheReplica)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084TheTitleCountsEachSidesFiguresFromTheReplica)
  {
    // M1 task T12's run check: each side's figures that are alive, from the replica; a side with none left shows 0.
    Fixture f;
    f.Send(Species::StateUpdate{
      5, 0, {WholeFigure(1, 0, 0), WholeFigure(2, 0, 16), WholeFigure(3, 0, 32, true), WholeFigure(4, 1, 48), WholeFigure(5, 1, 64)}});
    f.session->Update();
    Assert::IsTrue(f.session->SideFigures() == std::vector<std::uint32_t>{2, 2}, L"two alive a side; a corpse does not count");
    Assert::IsTrue(!f.titles.empty() && f.titles.back() == "Species — tick 5 — figures 2 : 2", L"the title");
    f.Send(Species::StateUpdate{6,
                                0,
                                {Species::RemovalRecord(Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 4)),
                                 Species::RemovalRecord(Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 5))}});
    f.session->Update();
    Assert::IsTrue(f.titles.back() == "Species — tick 6 — figures 2 : 0", L"a side with no figure left");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084TheReplicaIsDrawnOneTickBehindAndHeld)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-085")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084TheReplicaIsDrawnOneTickBehindAndHeld)
  {
    // A figure at x = 0 at tick 1 and x = 1 wu at tick 2, which arrives at 100 ms: the frame at its arrival draws tick
    // 1's state, then moves to tick 2's over one tick period, holds it, and after 250 ms more shows the wait.
    Fixture f;
    f.Send(Species::StateUpdate{1, 0, {WholeFigure(1, 0, 0)}});
    f.session->Update();
    f.manual->Advance(100 * MILLISECOND);
    const Species::ReplicatedState held{std::get<Species::ReplicatedFigure>(WholeFigure(1, 0, 0).state)};
    Species::ReplicatedFigure moved = std::get<Species::ReplicatedFigure>(held);
    moved.position.x = 16;
    f.Send(Species::StateUpdate{2, 1, {RecordOf(Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 1), moved, &held)}});
    f.session->Update();
    const auto drawnX = [&f]
    {
      const std::vector<Species::InterpolatedFigure> figures =
        Species::InterpolateFigures(f.session->Replica(), f.session->InterpolationFraction());
      Assert::IsTrue(figures.size() == 1, L"one figure");
      return figures[0].position.x;
    };
    Assert::IsTrue(drawnX() == 0.0, L"at its arrival, the previous tick's state");
    f.manual->Advance(50 * MILLISECOND);
    Assert::IsTrue(std::abs(drawnX() - 0.5) < 1e-9, L"half a period on, half way");
    f.manual->Advance(80 * MILLISECOND);
    Assert::IsTrue(drawnX() == 1.0, L"held at the latest state, never beyond it");
    Assert::IsFalse(f.session->WaitingForServer(), L"within the hold");
    f.manual->Advance(221 * MILLISECOND);
    Assert::IsTrue(f.session->WaitingForServer(), L"beyond the period and the hold");
    f.session->Update();
    Assert::IsTrue(f.titles.back().ends_with(" — waiting for the server"), L"the title says so");
    f.Send(Species::StateUpdate{3, 0, {}});
    f.session->Update();
    Assert::IsFalse(f.session->WaitingForServer(), L"the next state ends the wait");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084AnEventOlderThanItsLifeIsNotPlayed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084AnEventOlderThanItsLifeIsNotPlayed)
  {
    Fixture f;
    Species::EffectEvent died;
    died.kind = Species::EffectEventKind::FigureDied;
    died.subject = 9;
    f.Send(Species::StateUpdate{3, 0, {}});
    f.Send(Species::PresentationEvents{3, {died, died}});
    f.session->Update();
    const std::vector<Species::ReceivedEvent> taken = f.session->TakeEvents();
    Assert::IsTrue(taken.size() == 2 && taken[0].tick == 3 && taken[0].event.subject == 9, L"due as its tick arrives");
    Assert::IsTrue(f.session->TakeEvents().empty(), L"taken once");
    // Events nobody takes while the display moves on 0.6 s are dropped.
    f.Send(Species::PresentationEvents{3, {died}});
    f.session->Update();
    for (std::uint64_t tick = 4; tick < 10; ++tick)
    {
      f.manual->Advance(100 * MILLISECOND);
      f.Send(Species::StateUpdate{tick, 0, {}});
      f.session->Update();
    }
    f.manual->Advance(100 * MILLISECOND);
    Assert::IsTrue(f.session->TakeEvents().empty(), L"too old to play");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084TimingAndInterpolationOfTheReplica)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-085")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084TimingAndInterpolationOfTheReplica)
  {
    Species::ReplicaTiming timing(100 * MILLISECOND);
    Assert::IsTrue(timing.Fraction(Neuron::Nanoseconds{0}) == 1.0 && !timing.Waiting(Neuron::Nanoseconds{0}), L"nothing received");
    timing.Arrived(7, 1000 * MILLISECOND);
    Assert::IsTrue(timing.Fraction(1025 * MILLISECOND) == 0.25, L"a quarter period");
    Assert::IsTrue(timing.DisplayTick(1025 * MILLISECOND, 6) == 6.25, L"a quarter of the way from tick 6");
    timing.Arrived(7, 1090 * MILLISECOND);
    Assert::IsTrue(timing.Fraction(1100 * MILLISECOND) == 1.0, L"the same tick again is not a new arrival");
    Assert::IsFalse(timing.Waiting(1350 * MILLISECOND) || !timing.Waiting(1351 * MILLISECOND), L"the hold ends 350 ms after the arrival");

    // A turn interpolates the shorter way round.
    Species::StateReplica replica;
    Species::ReplicatedFigure figure;
    figure.yaw = 1000;
    replica.Apply({1, 0, {RecordOf(1, figure, nullptr)}});
    const Species::ReplicatedState before{figure};
    figure.yaw = 20;
    replica.Apply({2, 1, {RecordOf(1, figure, &before)}});
    const std::vector<Species::InterpolatedFigure> drawn = Species::InterpolateFigures(replica, 0.5);
    Assert::IsTrue(drawn.size() == 1 && std::abs(drawn[0].yawRadians - Species::YawRadians(1022)) < 1e-9, L"through north, not south");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp011TheClientBuildsTheTerrainTheServerSent)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp011TheClientBuildsTheTerrainTheServerSent)
  {
    // The server's seed and digest; the client builds the region with its own library and data, and its digest matches.
    Fixture f;
    const Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest, std::uint8_t{0}};
    Assert::IsFalse(f.session->CommandedTeam().has_value(), L"no team before the terrain");
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    f.Send(Species::StateUpdate{3, 0, {}});
    f.session->Update();
    Assert::IsTrue(f.session->CommandedTeam() == std::uint8_t{0}, L"the team the server names (ADR-119)");
    Assert::IsTrue(f.session->Terrain() != nullptr && f.session->Terrain()->digest == SeedOne().digest, L"the client's terrain");
    Assert::IsFalse(f.session->Refused(), L"accepted");
    Assert::IsTrue(f.session->Connected(), L"still connected");
    Assert::IsTrue(!f.titles.empty() && f.titles.back() == "Species — seed 1 — tick 3", L"the seed is shown");
    // The same terrain again changes nothing.
    const Species::RegionTerrain* built = f.session->Terrain();
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    f.session->Update();
    Assert::IsTrue(f.session->Terrain() == built, L"not built again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp011AClientRefusesATerrainWhoseDigestDiffers)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp011AClientRefusesATerrainWhoseDigestDiffers)
  {
    // A server whose library or data differs sends another digest: the client refuses the world with a message, and
    // closes the connection.
    Fixture f;
    Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest};
    sent.digest[0] ^= std::byte{1};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    f.session->Update();
    Assert::IsTrue(f.session->Refused() && f.session->Terrain() == nullptr, L"refused");
    Assert::IsFalse(f.session->Connected(), L"the client closed");
    Assert::IsTrue(f.server->State() == Neuron::ConnectionState::Closed, L"the server's end sees the close");
    Assert::IsTrue(!f.titles.empty() && f.titles.back() == Species::RefusedTitle("Species"), L"the refusal's message");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AJoinGoesFirstAndARefusalClosesTheSession)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AJoinGoesFirstAndARefusalClosesTheSession)
  {
    // ADR-190: the session's first message is its join, which the server does not number; a refusal closes the session,
    // which then sends nothing and shows the reason.
    Species::JoinIdentity identity;
    identity.build.fill(std::byte{7});
    Fixture f(identity);
    Assert::IsTrue(f.session->SendCommand(Species::SelectCommand{0, 0}) == std::uint64_t{0}, L"the first command is numbered 0");
    std::vector<Neuron::MessageTypeId> types;
    std::optional<Species::JoinCommand> join;
    Neuron::MessageDispatcher server;
    server.Register<Species::JoinCommand>([&join](const Species::JoinCommand& _join) { join = _join; });
    while (std::optional<Neuron::ReceivedMessage> message = f.server->Receive())
    {
      Neuron::BitReader reader(message->bytes);
      types.push_back(Neuron::ReadMessageHeader(reader).type);
      static_cast<void>(server.Dispatch(message->bytes));
    }
    Assert::IsTrue(types.size() == 2 && types[0] == Species::JoinCommand::TYPE && types[1] == Species::SelectCommand::TYPE,
                   L"the join, then the command");
    Assert::IsTrue(join.has_value() && join->identity == identity, L"with the client's identity");

    Species::JoinIdentity other = identity;
    other.build.fill(std::byte{8});
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::JoinAnswer{Species::JoinRefusal::Build, other}))
                     .has_value());
    f.session->Update();
    const std::optional<Species::JoinAnswer> refused = f.session->JoinRefused();
    Assert::IsTrue(refused.has_value() && refused->refusal == Species::JoinRefusal::Build && refused->server == other,
                   L"the refusal, with the server's identity");
    Assert::IsFalse(f.session->Connected(), L"the session closed");
    Assert::IsFalse(f.session->SendCommand(Species::SelectCommand{0, 0}).has_value(), L"and sends nothing more");
    Assert::IsTrue(!f.titles.empty() && f.titles.back() == Species::JoinRefusedTitle("Species", Species::JoinRefusal::Build),
                   L"the refusal's message");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp072InputIsReportedAtMostEveryFifteenSeconds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp072InputIsReportedAtMostEveryFifteenSeconds)
  {
    // ADR-185: a frame with input sends a presence report when none has gone for PRESENCE_REPORT_SECONDS; a frame without
    // input sends none, however long it has been. Each report is numbered as a command.
    Fixture f;
    const auto reports = [&f]()
    {
      std::size_t count = 0;
      while (std::optional<Neuron::ReceivedMessage> message = f.server->Receive())
      {
        Neuron::BitReader reader(message->bytes);
        count += Neuron::ReadMessageHeader(reader).type == Species::PresenceReportCommand::TYPE ? 1 : 0;
      }
      return count;
    };
    f.session->NoteInput(true);
    ExpectEqual(1, static_cast<std::int64_t>(reports()), L"the first input is reported");
    f.manual->Advance(1'000 * MILLISECOND);
    f.session->NoteInput(true);
    ExpectEqual(0, static_cast<std::int64_t>(reports()), L"not again within the period");
    f.manual->Advance(15'000 * MILLISECOND);
    f.session->NoteInput(false);
    ExpectEqual(0, static_cast<std::int64_t>(reports()), L"a frame without input reports nothing");
    f.session->NoteInput(true);
    ExpectEqual(1, static_cast<std::int64_t>(reports()), L"input after the period is reported");
    Assert::IsTrue(f.session->SendCommand(Species::SelectCommand{0, 0}) == std::uint64_t{2}, L"each report was numbered");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr095CommandsAreNumberedAsTheServerCountsThemAndTheirReportsArrive)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr095CommandsAreNumberedAsTheServerCountsThemAndTheirReportsArrive)
  {
    // ADR-124: every reliable message counts, the sandbox restart's too, so each sequence is the one the server gives.
    Fixture f;
    Assert::IsTrue(f.session->SendCommand(Species::SelectCommand{0, 0}) == std::uint64_t{0}, L"the first command");
    Assert::IsTrue(f.session->RequestSandboxRestart(false), L"the restart");
    Assert::IsTrue(f.session->SendCommand(Species::ControlStateCommand{0, {1.0, 2.0}}) == std::uint64_t{2}, L"the third");
    std::size_t received = 0;
    while (std::optional<Neuron::ReceivedMessage> message = f.server->Receive())
    {
      Assert::IsTrue(message->channel == Neuron::Channel::Reliable, L"on the reliable channel");
      ++received;
    }
    ExpectEqual(3, static_cast<std::int64_t>(received), L"each arrived");
    const Species::CommandReport report{2, 41, Species::CommandResult::Applied};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(report)).has_value());
    f.session->Update();
    const std::vector<Species::CommandReport> reports = f.session->TakeReports();
    Assert::IsTrue(reports.size() == 1 && reports[0].sequence == 2 && reports[0].tick == 41 &&
                     reports[0].result == Species::CommandResult::Applied,
                   L"the report, decoded");
    Assert::IsTrue(f.session->TakeReports().empty(), L"taken once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr139TheTitleShowsTheOwnSettlementAndItsNoticesArrive)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr139TheTitleShowsTheOwnSettlementAndItsNoticesArrive)
  {
    // M2 task T11's run check: the title shows the stores and the population of the settlement of the team the client
    // commands, from the replica, and not another team's; a notice on the reliable channel is taken once.
    Fixture f;
    const Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest, std::uint8_t{2}};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    const Species::ReplicatedSettlement own{2, 2, 2, 120'500, 1'500'000, 20, 40, 5'600};
    const Species::ReplicatedSettlement other{1, 1, 1, 0, 0, 0, 0, 5'600};
    f.Send(Species::StateUpdate{
      3, 0, {RecordOf(Species::SettlementObjectId(1), other, nullptr), RecordOf(Species::SettlementObjectId(2), own, nullptr)}});
    const Species::SettlementNotice notice{3,
                                           Species::NoticeKind::StructureCompleted,
                                           2,
                                           Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 9),
                                           Species::StructureKind::Dwelling,
                                           Species::QuantizedPosition{16, 0, 32}};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(notice)).has_value());
    f.session->Update();
    Assert::IsTrue(f.session->OwnSettlement() == own, L"the settlement of its team");
    Assert::IsTrue(!f.titles.empty() && f.titles.back() == "Species — seed 1 — tick 3 — stores 120 of 1500 — Workers 20 of 40",
                   L"its stores and Workers in the title");
    const std::vector<Species::SettlementNotice> notices = f.session->TakeNotices();
    Assert::IsTrue(notices.size() == 1 && notices[0].subject == notice.subject && notices[0].structure == Species::StructureKind::Dwelling,
                   L"the notice, decoded");
    Assert::IsTrue(f.session->TakeNotices().empty(), L"taken once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr165TheValleyStatusAndTheBattleResultsArrive)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr165TheValleyStatusAndTheBattleResultsArrive)
  {
    // ADR-165: the session keeps the latest valley status the server answered, and the battle results to be taken once,
    // in the order they arrived.
    Fixture f;
    Assert::IsFalse(f.session->LatestValleyStatus().has_value(), L"none before the first");
    Species::ValleyStatus first;
    first.tick = 10;
    first.team = 2;
    Species::ValleyStatus second = first;
    second.tick = 20;
    second.claims.push_back({Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 4), 2, 3, 15, Species::ClaimState::Open});
    Species::BattleResultNotice raid;
    raid.expedition = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 7);
    raid.attacker = 1;
    raid.defender = 2;
    raid.endTick = 18;
    Species::BattleResultNotice claim = raid;
    claim.expedition = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 8);
    claim.area = std::uint8_t{3};
    for (const std::vector<std::byte>& bytes :
         {Neuron::EncodeMessage(first), Neuron::EncodeMessage(raid), Neuron::EncodeMessage(second), Neuron::EncodeMessage(claim)})
    {
      Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, bytes).has_value(), L"sent");
    }
    f.session->Update();
    const std::optional<Species::ValleyStatus>& status = f.session->LatestValleyStatus();
    Assert::IsTrue(status && status->tick == 20 && status->claims == second.claims, L"the latest status");
    const std::vector<Species::BattleResultNotice> results = f.session->TakeBattleResults();
    Assert::IsTrue(results.size() == 2 && results[0].expedition == raid.expedition && results[1].area == claim.area,
                   L"both results, in order");
    Assert::IsTrue(f.session->TakeBattleResults().empty(), L"taken once");
    // ADR-169: and the latest epilogue.
    Assert::IsFalse(f.session->LatestEpilogue().has_value(), L"no epilogue before the first");
    Species::Epilogue epilogue;
    epilogue.tick = 30;
    epilogue.team = 2;
    epilogue.runEnded = true;
    epilogue.settlements.push_back({});
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(epilogue)).has_value(), L"sent");
    f.session->Update();
    Assert::IsTrue(f.session->LatestEpilogue() == epilogue, L"the epilogue kept");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp048EachOwnResultWaitsOnTheCardUntilClosed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp048EachOwnResultWaitsOnTheCardUntilClosed)
  {
    // ADR-168: the HUD takes the session's results: the settlement's own wait on the result card in the order they came,
    // each closed by a press on the card, which the verbs count; another battle's public line is a card among the notices.
    Fixture f;
    const Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest, std::uint8_t{2}};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    const Species::ReplicatedSettlement own{2, 2, 2, 120'500, 1'500'000, 20, 40, 5'600};
    f.Send(Species::StateUpdate{3, 0, {RecordOf(Species::SettlementObjectId(2), own, nullptr)}});
    Species::BattleResultNotice raid;
    raid.expedition = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 7);
    raid.attacker = 2;
    raid.defender = 1;
    raid.attackerWon = true;
    raid.endTick = 3;
    raid.side = true;
    Species::BattleResultNotice claim = raid;
    claim.expedition = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 8);
    claim.area = std::uint8_t{4};
    Species::BattleResultNotice publicLine = raid;
    publicLine.expedition = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 9);
    publicLine.attacker = 0;
    publicLine.defender = 3;
    publicLine.side = false;
    for (const std::vector<std::byte>& bytes :
         {Neuron::EncodeMessage(raid), Neuron::EncodeMessage(publicLine), Neuron::EncodeMessage(claim)})
    {
      Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, bytes).has_value(), L"sent");
    }
    f.session->Update();
    Species::SettlementHud hud(CommittedPresentation(), CommittedTuning());
    const Neuron::UiMetrics metrics = Species::UiMetricsOf(CommittedPresentation(), {1920, 1080}, 1.0f, std::nullopt, std::nullopt);
    Species::HudVerbs verbs;
    verbs.resultsClosed = 4; // the verbs' count before this HUD
    hud.Update(*f.session, 1.0, metrics, verbs);
    const Species::HudModel& model = hud.Model();
    Assert::IsTrue(model.results.size() == 2 && model.results[0].expedition == raid.expedition &&
                     model.results[1].expedition == claim.expedition,
                   L"its two results wait, in order");
    Assert::IsTrue(model.notices.size() == 1 && model.notices[0].result.has_value() &&
                     model.notices[0].result.value_or(Species::BattleResultNotice{}).expedition == publicLine.expedition,
                   L"the public line is a card");
    verbs.resultsClosed = 5;
    hud.Update(*f.session, 1.1, metrics, verbs);
    Assert::IsTrue(hud.Model().results.size() == 1 && hud.Model().results[0].expedition == claim.expedition, L"the first closed");
    verbs.resultsClosed = 6;
    hud.Update(*f.session, 1.2, metrics, verbs);
    Assert::IsTrue(hud.Model().results.empty(), L"and the second");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp074AReturnWithNewsOpensItsAwayReportOnce)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-074")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"UI-372")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp074AReturnWithNewsOpensItsAwayReportOnce)
  {
    // ADR-191, O110: in an always-on world the menu asks once for the latest away report; one the server sends at a return
    // with news opens its page, once; one without news is kept for the menu but opens nothing.
    Fixture f(std::nullopt, Species::WorldPace::AlwaysOn);
    const Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest, std::uint8_t{2}};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    const Species::ReplicatedSettlement own{2, 2, 2, 120'500, 1'500'000, 20, 40, 5'600};
    f.Send(Species::StateUpdate{3, 0, {RecordOf(Species::SettlementObjectId(2), own, nullptr)}});
    f.session->Update();
    const Neuron::MessageLocale locale("en-US");
    Species::WorldMenu::Desc desc;
    desc.presentation = &CommittedPresentation();
    desc.tuning = &CommittedTuning();
    desc.strings = &CommittedStrings();
    desc.locale = &locale;
    Species::WorldMenu menu(desc);
    std::vector<std::uint8_t> asked;
    Neuron::MessageDispatcher server;
    server.Register<Species::ReadAwayReportCommand>([&asked](const Species::ReadAwayReportCommand& _command)
                                                    { asked.push_back(_command.team); });
    const auto drain = [&]()
    {
      while (std::optional<Neuron::ReceivedMessage> message = f.server->Receive())
      {
        static_cast<void>(server.Dispatch(message->bytes));
      }
    };
    menu.Update(*f.session);
    menu.Update(*f.session);
    drain();
    Assert::IsTrue(asked == std::vector<std::uint8_t>{2} && !menu.Away().has_value(), L"asked for once, and none yet");

    Species::AwayReport quiet;
    quiet.tick = 10;
    quiet.team = 2;
    quiet.absence = true;
    quiet.onReturn = true;
    quiet.toTick = 600;
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(quiet)).has_value());
    f.session->Update();
    menu.Update(*f.session);
    Assert::IsTrue(menu.Away().has_value() && !menu.TakeAwayReportArrived(), L"a return without news opens nothing");
    Species::AwayReport news = quiet;
    news.battles = {{3, true, true, Species::BattleEnd::ObjectiveHeld, 500, 4, 0, false}};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(news)).has_value());
    f.session->Update();
    menu.Update(*f.session);
    Assert::IsTrue(menu.Away() == std::optional<Species::AwayReport>(news), L"the latest report kept");
    Assert::IsTrue(menu.TakeAwayReportArrived(), L"a return with news opens it");
    menu.Update(*f.session);
    Assert::IsFalse(menu.TakeAwayReportArrived(), L"once");

    // Its page over the menu: the summary and the deputy's actions, and Back, without Leave.
    Neuron::UiCore ui(
      [&menu](Neuron::UiPageId _page) -> std::unique_ptr<Neuron::UiPage>
      { return menu.MakePage(static_cast<Species::WorldMenuPage>(_page - static_cast<Neuron::UiPageId>(Species::GamePage::WorldMenu))); });
    const Neuron::UiMetrics metrics = Species::UiMetricsOf(CommittedPresentation(), {1920, 1080}, 1.0f, std::nullopt, std::nullopt);
    ui.OpenScreen(static_cast<Neuron::UiPageId>(Species::GamePage::WorldMenu));
    ui.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(Species::GamePage::WorldAwayReport)});
    ui.Update({}, metrics);
    ui.Update({}, metrics);
    const Neuron::UiWindow* page = ui.Focused();
    Assert::IsTrue(ui.Stack().size() == 2 && page != nullptr && Neuron::ValidateWindow(*page).empty(),
                   L"UI-372: the away report passes the validator");
    Assert::IsTrue(page->Contains(page->Find("section_1")) && !page->Contains(page->Find("section_2")) &&
                     !page->Contains(page->Find("leave")),
                   L"two sections and no Leave");
    Assert::IsTrue(page->Cursor() == page->Find("section_0"), L"the cursor starts on the summary");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp063TheEpilogueIsAskedForOnceTheRunEndsAndOpensOnce)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-063")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp063TheEpilogueIsAskedForOnceTheRunEndsAndOpensOnce)
  {
    // ADR-170: the world's menu asks for the epilogue once the valley status says the run has ended, once, and keeps the
    // ended run's epilogue when it comes, whose page then opens once.
    Fixture f;
    const Species::WorldTerrain sent{SeedOne().seed, SeedOne().digest, std::uint8_t{2}};
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(sent)).has_value());
    const Species::ReplicatedSettlement own{2, 2, 2, 120'500, 1'500'000, 20, 40, 5'600};
    f.Send(Species::StateUpdate{3, 0, {RecordOf(Species::SettlementObjectId(2), own, nullptr)}});
    f.session->Update();
    const Neuron::MessageLocale locale("en-US");
    Species::WorldMenu::Desc desc;
    desc.presentation = &CommittedPresentation();
    desc.tuning = &CommittedTuning();
    desc.strings = &CommittedStrings();
    desc.locale = &locale;
    Species::WorldMenu menu(desc);
    std::vector<std::uint8_t> asked;
    Neuron::MessageDispatcher server;
    server.Register<Species::ReadEpilogueCommand>([&asked](const Species::ReadEpilogueCommand& _command)
                                                  { asked.push_back(_command.team); });
    const auto drain = [&]()
    {
      while (std::optional<Neuron::ReceivedMessage> message = f.server->Receive())
      {
        static_cast<void>(server.Dispatch(message->bytes));
      }
    };
    menu.Update(*f.session);
    drain();
    Assert::IsTrue(menu.Site() == std::optional<std::uint8_t>(std::uint8_t{2}) && !menu.RunEnded() && asked.empty(),
                   L"nothing asked while the run goes on");

    Species::ValleyStatus valley;
    valley.tick = 4;
    valley.team = 2;
    valley.runEnded = true;
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(valley)).has_value());
    f.session->Update();
    menu.Update(*f.session);
    menu.Update(*f.session);
    drain();
    Assert::IsTrue(menu.RunEnded() && asked == std::vector<std::uint8_t>{2}, L"the run's end asks for the epilogue, once");
    Assert::IsFalse(menu.Ended().has_value() || menu.TakeEpilogueArrived(), L"which has not come yet");

    Species::Epilogue epilogue;
    epilogue.tick = 5;
    epilogue.team = 2;
    epilogue.runEnded = true;
    for (const std::uint8_t site : std::array<std::uint8_t, 4>{2, 0, 3, 1})
    {
      Species::EpilogueLine line;
      line.site = site;
      line.renown.site = site;
      epilogue.settlements.push_back(line);
    }
    Assert::IsTrue(f.server->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(epilogue)).has_value());
    f.session->Update();
    menu.Update(*f.session);
    Assert::IsTrue(menu.Ended() == std::optional<Species::Epilogue>(epilogue), L"the epilogue kept");
    Assert::IsTrue(menu.TakeEpilogueArrived(), L"its page opens");
    menu.Update(*f.session);
    Assert::IsFalse(menu.TakeEpilogueArrived(), L"once");
    // Its page over the menu: a row for each section, the standing shown first, Leave to the front end, and Back.
    Neuron::UiCore ui(
      [&menu](Neuron::UiPageId _page) -> std::unique_ptr<Neuron::UiPage>
      { return menu.MakePage(static_cast<Species::WorldMenuPage>(_page - static_cast<Neuron::UiPageId>(Species::GamePage::WorldMenu))); });
    const Neuron::UiMetrics metrics = Species::UiMetricsOf(CommittedPresentation(), {1920, 1080}, 1.0f, std::nullopt, std::nullopt);
    ui.OpenScreen(static_cast<Neuron::UiPageId>(Species::GamePage::WorldMenu));
    ui.Request({Neuron::UiPageRequest::Kind::Push, static_cast<Neuron::UiPageId>(Species::GamePage::WorldEpilogue)});
    ui.Update({}, metrics);
    ui.Update({}, metrics);
    const Neuron::UiWindow* page = ui.Focused();
    Assert::IsTrue(ui.Stack().size() == 2 && page != nullptr && Neuron::ValidateWindow(*page).empty(),
                   L"UI-372: the epilogue passes the validator");
    Assert::IsTrue(page->Contains(page->Find("section_4")) && page->Contains(page->Find("line_7")) && !page->Contains(page->Find("line_8")),
                   L"five sections, and as many lines as the longest");
    Assert::IsTrue(page->Cursor() == page->Find("section_0"), L"the cursor starts on the standing");
    const Neuron::UiRect lastLine = page->RectOf(page->Find("line_7"));
    const Neuron::UiRect rightBox = page->RectOf(page->Find("right_box"));
    const Neuron::UiRect leave = page->RectOf(page->Find("leave"));
    const Neuron::UiRect back = page->RectOf(page->Find("back"));
    Assert::IsTrue(lastLine.bottom <= rightBox.bottom && leave.bottom <= back.top, L"the text and the rows fit their boxes");
    Neuron::UiInput activate;
    activate.activate = true;
    for (int step = 0; step < 5; ++step)
    {
      Neuron::UiInput down;
      down.down = true;
      ui.Update(down, metrics);
    }
    Assert::IsTrue(page->Cursor() == page->Find("leave"), L"Leave below the sections");
    ui.Update(activate, metrics);
    Assert::IsTrue(menu.TakeChoice().value_or(Species::WorldMenuChoice{}).kind == Species::WorldMenuChoice::Kind::Leave, L"which leaves");
    Neuron::UiInput close;
    close.close = true;
    ui.Update(close, metrics);
    ui.Update({}, metrics);
    Assert::IsTrue(ui.Stack().size() == 1 && !menu.TakeChoice().has_value(), L"Back returns to the menu");
    menu.Reset();
    Assert::IsFalse(menu.RunEnded() || menu.Ended().has_value() || menu.Site().has_value(), L"a new world starts with none");
  }
};

} // namespace GameClientTests
