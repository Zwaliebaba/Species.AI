#include "pch.h"

#include "DevelopmentOverlay.h"
#include "RelationMessages.h"
#include "StableId.h"
#include "StateUpdate.h"
#include "TestSupport.h"
#include "Tuning.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

Species::InterpolatedFigure Figure(std::uint8_t _team, Species::FigureKind _kind, bool _dead, const Neuron::Vec3& _position)
{
  Species::InterpolatedFigure figure;
  figure.state.team = _team;
  figure.state.kind = _kind;
  figure.state.dead = _dead;
  figure.position = _position;
  return figure;
}

Species::StateRecord Record(std::uint64_t _counter, const Species::ReplicatedState& _state)
{
  return Species::MakeRecord(Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter), _state, nullptr)
    .value_or(Species::StateRecord{});
}

Species::ReplicatedFigure ReplicatedOf(std::uint8_t _team, Species::FigureKind _kind, bool _dead)
{
  Species::ReplicatedFigure figure;
  figure.team = _team;
  figure.kind = _kind;
  figure.dead = _dead;
  return figure;
}

bool Contains(const std::string& _text, std::string_view _part)
{
  return _text.find(_part) != std::string::npos;
}

// The frame loop's profile, with the record's history and roll-up period.
Neuron::FrameProfile::Desc ProfileDesc()
{
  return {static_cast<std::size_t>(Species::ClientStep::Count), 200, std::chrono::milliseconds(1000)};
}

// 200 frames of _frameMs each, _waitMs of it the renderer's, into _profile.
void FillProfile(Neuron::FrameProfile& _profile, double _frameMs, double _waitMs)
{
  using Ms = std::chrono::duration<double, std::milli>;
  const auto frame = std::chrono::duration_cast<Neuron::Nanoseconds>(Ms(_frameMs));
  const auto wait = std::chrono::duration_cast<Neuron::Nanoseconds>(Ms(_waitMs));
  Neuron::Nanoseconds now{0};
  for (int n = 0; n < 200; ++n)
  {
    _profile.AddStep(static_cast<std::size_t>(Species::ClientStep::Render), frame - wait);
    _profile.AddStep(static_cast<std::size_t>(Species::ClientStep::Wait), wait);
    _profile.EndFrame(now, frame, wait);
    now += frame;
  }
}

} // namespace

TEST_CLASS(DevelopmentOverlayTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg095TheFrameRateLineCountsTheObjectsWhileAWorldExists)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-576")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg095TheFrameRateLineCountsTheObjectsWhileAWorldExists)
  {
    Assert::AreEqual(std::string("60 fps"), Species::FrameRateText(60.0, std::nullopt));
    Assert::AreEqual(std::string("59 fps \xC2\xB7 412 objects"), Species::FrameRateText(59.4, std::size_t{412}));
    Assert::AreEqual(std::string("0 fps"), Species::FrameRateText(-1.0, std::nullopt), L"never below zero");
    // The anchor is the record's, in unscaled pixels from the bottom-right corner.
    const Neuron::OverlayLine line = Species::FrameRateLine(CommittedPresentation(), 144.0, std::nullopt);
    Assert::AreEqual(std::string("144 fps"), line.text);
    Assert::AreEqual(10.0f, line.insetX);
    Assert::AreEqual(40.0f, line.insetY);
    Assert::IsTrue(line.corner == Neuron::OverlayCorner::BottomRight, L"right-aligned at the bottom-right corner");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim415ATeamsColorIsItsBaseColorByItsSlot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-415")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-416")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim415ATeamsColorIsItsBaseColorByItsSlot)
  {
    const std::array<float, 4> green = Species::TeamColor(0);
    Assert::AreEqual(100.0f / 255.0f, green[0], 1e-6f);
    Assert::AreEqual(1.0f, green[1], 1e-6f);
    Assert::AreEqual(1.0f, green[3]);
    const std::array<float, 4> red = Species::TeamColor(1);
    Assert::AreEqual(1.0f, red[0], 1e-6f);
    Assert::AreEqual(50.0f / 255.0f, red[1], 1e-6f);
    // ADR-167: an expedition's team, 4 + its site, takes its settlement's colour, so team 7 is site 3's blue.
    Assert::IsTrue(Species::TeamColor(Species::ExpeditionTeamOf(0)) == green, L"site 0's expedition is green");
    const std::array<float, 4> blue = Species::TeamColor(Species::ExpeditionTeamOf(3));
    Assert::AreEqual(180.0f / 255.0f, blue[1], 1e-6f, L"site 3's expedition is blue");
    const std::array<float, 4> beyond = Species::TeamColor(8);
    Assert::IsTrue(beyond == std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f}, L"white beyond the player colours");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg004EveryLivingFigureHasADotAboveItsFeet)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-004")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg004EveryLivingFigureHasADotAboveItsFeet)
  {
    const std::vector<Species::InterpolatedFigure> figures{
      Figure(0, Species::FigureKind::Worker, false, {100.0, 5.0, 200.0}),
      Figure(1, Species::FigureKind::Worker, true, {110.0, 5.0, 200.0}),
      Figure(1, Species::FigureKind::Leader, false, {120.0, 6.0, 210.0}),
    };
    const std::vector<Neuron::DevelopmentDot> dots = Species::FigureDots(figures);
    Assert::AreEqual(std::size_t{2}, dots.size(), L"a dead figure has none");
    Assert::AreEqual(5.0 + Species::DOT_LIFT_WU, dots[0].position.y);
    Assert::AreEqual(Species::WORKER_DOT_PIXELS, dots[0].sidePixels);
    Assert::IsTrue(dots[0].color == Species::TeamColor(0));
    Assert::AreEqual(120.0, dots[1].position.x);
    Assert::AreEqual(Species::LEADER_DOT_PIXELS, dots[1].sidePixels, L"a Leader's dot is larger");
    Assert::IsTrue(dots[1].color == Species::TeamColor(1));
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg096TheCountsAreOfTheLivingFiguresByKind)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg096TheCountsAreOfTheLivingFiguresByKind)
  {
    Species::StateReplica replica;
    Species::ReplicatedOrb orb;
    Species::ReplicatedEffect effect;
    replica.Apply(
      {1,
       0,
       {Record(1, ReplicatedOf(0, Species::FigureKind::Worker, false)), Record(2, ReplicatedOf(0, Species::FigureKind::Worker, false)),
        Record(3, ReplicatedOf(0, Species::FigureKind::Leader, false)), Record(4, ReplicatedOf(1, Species::FigureKind::Worker, true)),
        Record(5, ReplicatedOf(1, Species::FigureKind::Leader, false)), Record(6, orb), Record(7, effect)}});
    const Species::ReplicaCounts counts = Species::CountReplica(replica);
    Assert::AreEqual(std::size_t{2}, counts.sides.size(), L"slots 0 and 1");
    Assert::IsTrue(counts.sides[0].workers == 2 && counts.sides[0].leaders == 1, L"team 0: two Workers and a Leader");
    Assert::IsTrue(counts.sides[1].workers == 0 && counts.sides[1].leaders == 1, L"team 1: its dead Worker is not counted");
    Assert::IsTrue(counts.orbs == 1 && counts.effects == 1, L"an orb and an effect");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg096TheFramePageShowsItsRowsAndTheGraph)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-096")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-089")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg096TheFramePageShowsItsRowsAndTheGraph)
  {
    Neuron::FrameProfile profile(ProfileDesc());
    FillProfile(profile, 100.0, 90.0);
    Species::FramePageInputs inputs;
    inputs.profile = &profile;
    inputs.counts.sides = {{300, 1}, {299, 1}};
    inputs.counts.orbs = 4;
    const Species::FramePage page = Species::BuildFramePage(inputs, CommittedPresentation(), CommittedTuning(), 16.0f);
    Assert::IsTrue(page.lines.size() >= 7, L"frame, work, steps, two teams, orbs and server, and the graph's labels");
    Assert::IsTrue(Contains(page.lines[0].text, "frame p50 100.0") && Contains(page.lines[0].text, "p99 100.0 ms"), L"the percentiles");
    Assert::IsTrue(Contains(page.lines[1].text, "work p50 10.0") && Contains(page.lines[1].text, "of 16.7"),
                   L"the work against a frame at 60 per second");
    Assert::IsTrue(Contains(page.lines[2].text, "render 10.0%") && Contains(page.lines[2].text, "wait 90.0%"),
                   L"each step's share of the period");
    Assert::IsTrue(Contains(page.lines[3].text, "team 0 300 Workers") && Contains(page.lines[4].text, "team 1 299 Workers"),
                   L"a line per team");
    Assert::IsTrue(Contains(page.lines[5].text, "orbs 4") && Contains(page.lines[6].text, "no statistics"), L"the counts and the server");
    Assert::IsTrue(page.lines[0].corner == Neuron::OverlayCorner::TopLeft && page.lines[0].insetX == 10.0f &&
                     page.lines[0].insetY == 10.0f && page.lines[1].insetY == 28.0f,
                   L"from the top-left corner, a cell and two pixels apart");
    // DBG-089: a backdrop, a line per frame of the history and a guide line per mark.
    Assert::AreEqual(std::size_t{1 + 200 + 3}, page.rects.size());
    const Neuron::OverlayRect& newest = page.rects[1];
    Assert::IsTrue(newest.left == 10.0f + 199.0f && newest.width == 1.0f && newest.height == 50.0f,
                   L"the newest frame at the right, 0.5 px per ms");
    Assert::IsTrue(page.rects[200].left == 10.0f, L"the oldest at the left");
    const Neuron::OverlayRect& guide = page.rects[201];
    Assert::IsTrue(guide.height == 1.0f && guide.top == page.rects[0].top, L"the 200 ms guide at the graph's top");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg097ARowOverItsBudgetIsInTheWarningColor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-097")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg097ARowOverItsBudgetIsInTheWarningColor)
  {
    const std::array<float, 4> white{1.0f, 1.0f, 1.0f, 1.0f};
    const std::array<float, 4> warning{1.0f, 64.0f / 255.0f, 64.0f / 255.0f, 1.0f};
    Species::FramePageInputs inputs;
    Species::ServerStats server;
    server.tickMeanMicroseconds = 2'000;
    server.tickMaxMicroseconds = 49'000;
    inputs.server = server;
    Neuron::FrameProfile within(ProfileDesc());
    FillProfile(within, 16.7, 10.0);
    inputs.profile = &within;
    Species::FramePage page = Species::BuildFramePage(inputs, CommittedPresentation(), CommittedTuning(), 16.0f);
    Assert::IsTrue(page.lines[1].color == white, L"6.7 ms of work is within 16.7 ms");
    Assert::IsTrue(Contains(page.lines[4].text, "49.00 ms max of 50.0"), L"the tick's longest against half the tick period");
    Assert::IsTrue(page.lines[4].color == white, L"a 49 ms tick is within half the tick period");
    Neuron::FrameProfile over(ProfileDesc());
    FillProfile(over, 20.0, 2.0);
    inputs.profile = &over;
    server.tickMaxMicroseconds = 51'000;
    inputs.server = server;
    page = Species::BuildFramePage(inputs, CommittedPresentation(), CommittedTuning(), 16.0f);
    Assert::IsTrue(page.lines[1].color == warning, L"18 ms of work is over its budget");
    Assert::IsTrue(page.lines[0].color == white, L"the frame time's own row has no budget");
    Assert::IsTrue(page.lines[4].color == warning, L"a 51 ms tick is over its budget");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg096TheStatisticsLineCarriesThePagesFigures)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg096TheStatisticsLineCarriesThePagesFigures)
  {
    Neuron::FrameProfile profile(ProfileDesc());
    FillProfile(profile, 16.0, 8.0);
    Species::FramePageInputs inputs;
    inputs.profile = &profile;
    inputs.framesPerSecond = 62.4;
    inputs.counts.sides = {{300, 1}, {290, 1}};
    inputs.counts.orbs = 9;
    Species::ServerStats server;
    server.tick = 600;
    server.tickMeanMicroseconds = 1'850;
    server.tickMaxMicroseconds = 3'100;
    inputs.server = server;
    const std::string line = Species::StatisticsLine(inputs, CommittedPresentation());
    Assert::IsTrue(Contains(line, "overlay: 62 fps") && Contains(line, "frame p50 16.0") && Contains(line, "work p50 8.0"), L"the frame");
    Assert::IsTrue(Contains(line, "figures 300+1 : 290+1") && Contains(line, "orbs 9"), L"the counts");
    Assert::IsTrue(Contains(line, "server tick 600: 1.85 ms mean, 3.10 ms max"), L"the server");
  }
};

} // namespace GameClientTests
