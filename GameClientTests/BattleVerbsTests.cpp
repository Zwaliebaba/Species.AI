#include "pch.h"

#include "BattleVerbs.h"
#include "FigureKind.h"
#include "ReplicaTiming.h"
#include "StableId.h"
#include "StateReplica.h"
#include "StateUpdate.h"
#include "TestSupport.h"

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr std::uint8_t OWN = 0;
const Neuron::Vec3 GROUND{1'000.0, 0.0, 1'000.0};

Species::StableId IdOf(std::uint64_t _counter)
{
  return Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter);
}

/// A figure of the replica, with the flags the verbs read.
struct Figure
{
  std::uint64_t counter = 0;
  Species::FigureKind kind = Species::FigureKind::Worker;
  Neuron::Vec3 position = GROUND;
  bool selected = false;
  bool underControl = false;
  bool dead = false;
  bool burning = false;
  std::uint8_t team = OWN;
};

// _replica, emptied, with _tick its latest tick and _figures whole in it. It is filled in place: a StateReplica's maps
// may allocate as they move.
void Fill(Species::StateReplica& _replica, std::uint64_t _tick, const std::vector<Figure>& _figures)
{
  std::vector<Species::StateRecord> records;
  for (const Figure& figure : _figures)
  {
    Species::ReplicatedFigure state;
    state.team = figure.team;
    state.kind = figure.kind;
    state.position = Species::QuantizePosition(figure.position);
    state.health = 50;
    state.selected = figure.selected;
    state.underControl = figure.underControl;
    state.dead = figure.dead;
    state.onGround = true;
    state.task = figure.burning ? static_cast<std::uint8_t>(Species::WorkerState::Burning) : std::uint8_t{0};
    const std::optional<Species::StateRecord> record = Species::MakeRecord(IdOf(figure.counter), Species::ReplicatedState{state}, nullptr);
    Assert::IsTrue(record.has_value(), L"a whole record");
    records.push_back(record.value_or(Species::StateRecord{}));
  }
  _replica.Clear();
  _replica.Apply({_tick, 0, std::move(records)});
}

/// The verbs of the committed record over one replica at a time, with the frame's drawn figures taken from it.
struct Pump
{
  Species::BattleVerbs verbs{CommittedPresentation()};
  Species::StateReplica replica;

  std::vector<Species::BattleCommand> Frame(const Species::VerbFrame& _frame, std::optional<std::uint8_t> _team = OWN)
  {
    const std::vector<Species::InterpolatedFigure> figures = Species::InterpolateFigures(replica, 1.0);
    return verbs.Update(_frame, _team, replica, figures);
  }

  // A frame whose commands are all sent, numbered from _sequence.
  std::vector<Species::BattleCommand> FrameSent(const Species::VerbFrame& _frame, std::uint64_t& _sequence)
  {
    std::vector<Species::BattleCommand> commands = Frame(_frame);
    for (const Species::BattleCommand& command : commands)
    {
      verbs.Sent(_sequence++, command);
    }
    return commands;
  }
};

template <class T> std::vector<T> CommandsOf(const std::vector<Species::BattleCommand>& _commands)
{
  std::vector<T> found;
  for (const Species::BattleCommand& command : _commands)
  {
    if (const T* typed = std::get_if<T>(&command))
    {
      found.push_back(*typed);
    }
  }
  return found;
}

Species::VerbFrame FrameAt(const Neuron::Vec3& _ground, double _step = 1.0 / 60.0)
{
  Species::VerbFrame frame;
  frame.ground = _ground;
  frame.realStepSeconds = _step;
  return frame;
}

} // namespace

TEST_CLASS(BattleVerbsTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl050TheSecondaryPressTogglesOrClearsTheSelection)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-050")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-073")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl050TheSecondaryPressTogglesOrClearsTheSelection)
  {
    // AC-CTRL-AB-01 as O82 resolves it: with a Leader selected and the pointer on it, the secondary press toggles it and
    // gives no order.
    Pump pump;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Leader, GROUND, true}});
    Species::VerbFrame frame = FrameAt(GROUND);
    frame.leaderUnderPointer = IdOf(1);
    frame.controls.secondaryDown = true;
    std::vector<Species::BattleCommand> commands = pump.Frame(frame);
    const std::vector<Species::ToggleUnitCommand> toggles = CommandsOf<Species::ToggleUnitCommand>(commands);
    Assert::IsTrue(toggles.size() == 1 && toggles[0].actor == IdOf(1) && toggles[0].team == OWN, L"the toggle");
    Assert::IsTrue(CommandsOf<Species::OrderUnitCommand>(commands).empty() && CommandsOf<Species::SelectCommand>(commands).empty(),
                   L"and no order or clear");
    // Another Leader under the pointer while one is selected: the selection is cleared, and nothing is toggled.
    Fill(pump.replica, 6,
         {{1, Species::FigureKind::Leader, GROUND, true}, {2, Species::FigureKind::Leader, GROUND + Neuron::Vec3{50.0, 0.0, 0.0}}});
    frame.leaderUnderPointer = IdOf(2);
    commands = pump.Frame(frame);
    std::vector<Species::SelectCommand> selects = CommandsOf<Species::SelectCommand>(commands);
    Assert::IsTrue(selects.size() == 1 && selects[0].actor == 0 && CommandsOf<Species::ToggleUnitCommand>(commands).empty(),
                   L"a Select of nothing");
    // On the ground with Workers selected, the secondary press clears them too, and promotes nobody.
    Fill(pump.replica, 7, {{3, Species::FigureKind::Worker, GROUND, false, true}});
    frame.leaderUnderPointer.reset();
    commands = pump.Frame(frame);
    selects = CommandsOf<Species::SelectCommand>(commands);
    Assert::IsTrue(selects.size() == 1 && selects[0].actor == 0 && CommandsOf<Species::PromoteWorkerCommand>(commands).empty() &&
                     CommandsOf<Species::OrderWorkersCommand>(commands).empty(),
                   L"the Workers released");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl062APrimaryPressOnAnotherLeaderSelectsIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-062")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl062APrimaryPressOnAnotherLeaderSelectsIt)
  {
    // O82: an own Leader under the pointer is selected by the primary press, never an order's target.
    Pump pump;
    Fill(pump.replica, 5,
         {{1, Species::FigureKind::Leader, GROUND, true}, {2, Species::FigureKind::Leader, GROUND + Neuron::Vec3{50.0, 0.0, 0.0}}});
    Species::VerbFrame frame = FrameAt(GROUND + Neuron::Vec3{50.0, 0.0, 0.0});
    frame.leaderUnderPointer = IdOf(2);
    frame.controls.primaryDown = true;
    const std::vector<Species::BattleCommand> commands = pump.Frame(frame);
    const std::vector<Species::SelectCommand> selects = CommandsOf<Species::SelectCommand>(commands);
    Assert::IsTrue(selects.size() == 1 && selects[0].actor == IdOf(2), L"the other Leader selected");
    Assert::IsTrue(CommandsOf<Species::OrderUnitCommand>(commands).empty(), L"and no order");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl043ShiftSendsTheLeaderAloneAndCtrlStraight)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-043")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-041")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl043ShiftSendsTheLeaderAloneAndCtrlStraight)
  {
    // AC-CTRL-AB-02, the keyboard-and-mouse Leader rows on the primary button (O82): move-only with Shift, a straight
    // route with Ctrl.
    Pump pump;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Leader, GROUND, true}});
    const Neuron::Vec3 target = GROUND + Neuron::Vec3{300.0, 0.0, 0.0};
    for (const auto& [moveOnly, straight] : {std::pair{false, false}, std::pair{true, false}, std::pair{false, true}})
    {
      Species::VerbFrame frame = FrameAt(target);
      frame.controls.primaryDown = true;
      frame.controls.moveOnly = moveOnly;
      frame.controls.straight = straight;
      const std::vector<Species::OrderUnitCommand> orders = CommandsOf<Species::OrderUnitCommand>(pump.Frame(frame));
      Assert::IsTrue(orders.size() == 1 && orders[0].moveOnly == moveOnly && orders[0].straight == straight &&
                       orders[0].position.x == target.x,
                     L"one order with the modifiers held");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl044CDemotesTheSelectedLeader)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-044")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl044CDemotesTheSelectedLeader)
  {
    // AC-CTRL-AB-05: C with a Leader selected demotes it; with none selected it sends nothing.
    Pump pump;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Leader, GROUND, true}});
    Species::VerbFrame frame = FrameAt(GROUND);
    frame.controls.demote = true;
    const std::vector<Species::DemoteLeaderCommand> demotes = CommandsOf<Species::DemoteLeaderCommand>(pump.Frame(frame));
    Assert::IsTrue(demotes.size() == 1 && demotes[0].actor == IdOf(1), L"the selected Leader");
    Fill(pump.replica, 6, {{1, Species::FigureKind::Leader, GROUND, false}});
    Assert::IsTrue(CommandsOf<Species::DemoteLeaderCommand>(pump.Frame(frame)).empty(), L"nothing selected, nothing demoted");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl048APressSelectsOnceAndBeginsTheCircleOnEmptyGround)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-062")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-155")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl048APressSelectsOnceAndBeginsTheCircleOnEmptyGround)
  {
    // Edge case 1 and input T21: one press on empty ground with nothing selected is one Select of nothing, and the circle
    // begins; one press on an own Leader is one Select of it; a press on the selected Leader is ignored.
    Pump pump;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Leader, GROUND + Neuron::Vec3{80.0, 0.0, 0.0}}});
    Species::VerbFrame frame = FrameAt(GROUND);
    frame.controls.primaryDown = true;
    std::vector<Species::BattleCommand> commands = pump.Frame(frame);
    std::vector<Species::SelectCommand> selects = CommandsOf<Species::SelectCommand>(commands);
    Assert::IsTrue(commands.size() == 1 && selects.size() == 1 && selects[0].actor == 0, L"one Select of nothing");
    Assert::IsTrue(pump.verbs.Circle().has_value() && pump.verbs.Circle()->center == GROUND && pump.verbs.Circle()->radius == 0.0,
                   L"the circle begins at the ground point");
    frame.controls = {};
    frame.controls.primaryUp = true;
    static_cast<void>(pump.Frame(frame));
    Assert::IsFalse(pump.verbs.Circle().has_value(), L"and ends with the release");

    frame.controls = {};
    frame.controls.primaryDown = true;
    frame.leaderUnderPointer = IdOf(1);
    selects = CommandsOf<Species::SelectCommand>(pump.Frame(frame));
    Assert::IsTrue(selects.size() == 1 && selects[0].actor == IdOf(1), L"one Select of the Leader");
    Assert::IsFalse(pump.verbs.Circle().has_value(), L"and no circle over a Leader");
    Fill(pump.replica, 6, {{1, Species::FigureKind::Leader, GROUND + Neuron::Vec3{80.0, 0.0, 0.0}, true}});
    Assert::IsTrue(CommandsOf<Species::SelectCommand>(pump.Frame(frame)).empty(), L"the selected Leader is not selected again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl156TheCircleGrowsToItsLimitAndSendsOnceATick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-156")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-157")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl156TheCircleGrowsToItsLimitAndSendsOnceATick)
  {
    // AC-CTRL-SP-09: held 1.5 s of real time, the circle reaches its 100 wu limit, sending at most one selection a tick.
    Pump pump;
    Fill(pump.replica, 1, {});
    Species::VerbFrame frame = FrameAt(GROUND);
    frame.controls.primaryDown = true;
    frame.controls.primaryHeld = true;
    std::size_t sends = 0;
    std::size_t ticks = 0;
    for (int index = 0; index < 90; ++index)
    {
      if (index % 6 == 0)
      {
        Fill(pump.replica, static_cast<std::uint64_t>(index / 6) + 1, {});
        ++ticks;
      }
      const std::vector<Species::SelectWorkersCommand> circles = CommandsOf<Species::SelectWorkersCommand>(pump.Frame(frame));
      Assert::IsTrue(circles.size() <= 1, L"at most one a frame");
      sends += circles.size();
      frame.controls.primaryDown = false;
    }
    Assert::IsTrue(pump.verbs.Circle().has_value() && pump.verbs.Circle()->radius == 100.0, L"100 wu after 1.5 s");
    Assert::IsTrue(sends == ticks, L"one selection for each tick");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl156TheCircleTakesThePointersFarthestDrag)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-156")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-157")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl156TheCircleTakesThePointersFarthestDrag)
  {
    // O82: the radius is the larger of the growth on the real clock and the pointer's farthest drag, never shrinking, up
    // to 100 wu; released, the circle fades over mvp.selection.circle_fade_seconds (0.4 s).
    Pump pump;
    Fill(pump.replica, 1, {});
    Species::VerbFrame frame = FrameAt(GROUND, 0.01);
    frame.controls.primaryDown = true;
    frame.controls.primaryHeld = true;
    static_cast<void>(pump.Frame(frame));
    Assert::AreEqual(1.0, pump.verbs.Circle().value_or(Species::SelectionCircle{}).radius, 1e-9, L"grown 1 wu in 0.01 s");
    frame.controls.primaryDown = false;
    frame.ground = GROUND + Neuron::Vec3{60.0, 0.0, 0.0};
    static_cast<void>(pump.Frame(frame));
    Assert::AreEqual(60.0, pump.verbs.Circle().value_or(Species::SelectionCircle{}).radius, 1e-9, L"dragged 60 wu");
    frame.ground = GROUND;
    static_cast<void>(pump.Frame(frame));
    Assert::AreEqual(60.0, pump.verbs.Circle().value_or(Species::SelectionCircle{}).radius, 1e-9, L"and kept when the pointer returns");
    frame.ground = GROUND + Neuron::Vec3{0.0, 0.0, 250.0};
    static_cast<void>(pump.Frame(frame));
    Assert::AreEqual(100.0, pump.verbs.Circle().value_or(Species::SelectionCircle{}).radius, 1e-9, L"up to the limit");
    frame.controls = {};
    frame.controls.primaryUp = true;
    frame.realStepSeconds = 0.0;
    static_cast<void>(pump.Frame(frame));
    Assert::IsFalse(pump.verbs.Circle().has_value(), L"released");
    const std::optional<Species::SelectionCircle> shown = pump.verbs.ShownCircle();
    Assert::IsTrue(shown.has_value() && shown->strength == 1.0, L"and shown");
    frame.controls = {};
    frame.realStepSeconds = 0.2;
    static_cast<void>(pump.Frame(frame));
    Assert::AreEqual(0.5, pump.verbs.ShownCircle().value_or(Species::SelectionCircle{}).strength, 1e-9, L"half faded after 0.2 s");
    static_cast<void>(pump.Frame(frame));
    Assert::IsFalse(pump.verbs.ShownCircle().has_value(), L"gone after 0.4 s");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl160CrowdOrdersKeepTheirInterval)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-160")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-157")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl160CrowdOrdersKeepTheirInterval)
  {
    // AC-CTRL-SP-10: a second crowd order 9 ticks after the first is not sent, 10 after it is; after a circle ends, one
    // may follow at once.
    Pump pump;
    const std::vector<Figure> controlled{{1, Species::FigureKind::Worker, GROUND, false, true}};
    Species::VerbFrame frame = FrameAt(GROUND + Neuron::Vec3{200.0, 0.0, 0.0});
    frame.controls.primaryDown = true;
    frame.controls.straight = true;
    Fill(pump.replica, 10, controlled);
    const std::vector<Species::OrderWorkersCommand> first = CommandsOf<Species::OrderWorkersCommand>(pump.Frame(frame));
    Assert::IsTrue(first.size() == 1 && first[0].straight && first[0].position.x == GROUND.x + 200.0, L"the first order");
    Fill(pump.replica, 19, controlled);
    Assert::IsTrue(CommandsOf<Species::OrderWorkersCommand>(pump.Frame(frame)).empty(), L"9 ticks later, none");
    Fill(pump.replica, 20, controlled);
    Assert::IsTrue(CommandsOf<Species::OrderWorkersCommand>(pump.Frame(frame)).size() == 1, L"10 ticks later, sent");
    // ADR-127: the release of an order's own press resets nothing; only a circle's end does.
    Species::VerbFrame release = FrameAt(GROUND);
    release.controls.primaryUp = true;
    static_cast<void>(pump.Frame(release));
    Fill(pump.replica, 21, controlled);
    Assert::IsTrue(CommandsOf<Species::OrderWorkersCommand>(pump.Frame(frame)).empty(), L"an order's release keeps the interval");
    Fill(pump.replica, 21, {});
    Species::VerbFrame circle = FrameAt(GROUND);
    circle.controls.primaryDown = true;
    circle.controls.primaryHeld = true;
    static_cast<void>(pump.Frame(circle));
    static_cast<void>(pump.Frame(release));
    Fill(pump.replica, 21, controlled);
    Assert::IsTrue(CommandsOf<Species::OrderWorkersCommand>(pump.Frame(frame)).size() == 1, L"after a circle's end, at once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr125ARouteThatCannotBeWalkedHoldsTheOrdersBack)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-354")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-366")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr125ARouteThatCannotBeWalkedHoldsTheOrdersBack)
  {
    // While the unavailable cursor shows for the preview, neither the crowd nor the selected Leader is ordered, and the
    // press does nothing else; a toggle of the Leader under the pointer still goes.
    Pump pump;
    Species::VerbFrame frame = FrameAt(GROUND + Neuron::Vec3{200.0, 0.0, 0.0});
    frame.controls.primaryDown = true;
    frame.routeBlocked = true;
    Fill(pump.replica, 10, {{1, Species::FigureKind::Worker, GROUND, false, true}});
    Assert::IsTrue(pump.Frame(frame).empty() && !pump.verbs.Circle().has_value(), L"no crowd order, and no circle");
    Fill(pump.replica, 11, {{2, Species::FigureKind::Leader, GROUND, true}});
    Assert::IsTrue(CommandsOf<Species::OrderUnitCommand>(pump.Frame(frame)).empty(), L"no order to the selected Leader");
    Species::VerbFrame toggle = frame;
    toggle.controls = {};
    toggle.controls.secondaryDown = true;
    toggle.leaderUnderPointer = IdOf(2);
    Assert::IsTrue(CommandsOf<Species::ToggleUnitCommand>(pump.Frame(toggle)).size() == 1, L"the toggle still goes");
    frame.routeBlocked = false;
    Assert::IsTrue(CommandsOf<Species::OrderUnitCommand>(pump.Frame(frame)).size() == 1, L"and with a walkable route, the order");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl042APromotionNamesTheNearestSelectableWorker)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-042")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-050")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl042APromotionNamesTheNearestSelectableWorker)
  {
    // Row O2 and O78: the nearest own Worker within 10 wu that is not Burning, which the highlight cursor marks.
    Pump pump;
    Fill(pump.replica, 5,
         {{1, Species::FigureKind::Worker, GROUND + Neuron::Vec3{8.0, 0.0, 0.0}},
          {2, Species::FigureKind::Worker, GROUND + Neuron::Vec3{0.0, 0.0, 4.0}},
          {3, Species::FigureKind::Worker, GROUND + Neuron::Vec3{1.0, 0.0, 0.0}, false, false, false, true},
          {4, Species::FigureKind::Worker, GROUND, false, false, false, false, 1}});
    Species::VerbFrame frame = FrameAt(GROUND);
    static_cast<void>(pump.Frame(frame));
    Assert::IsTrue(pump.verbs.PromotableWorker() == IdOf(2), L"the nearest own Worker that is not Burning");
    frame.controls.primaryDown = true;
    Assert::IsTrue(CommandsOf<Species::PromoteWorkerCommand>(pump.Frame(frame)).empty() && pump.verbs.Circle().has_value(),
                   L"the primary press begins the circle and promotes nobody");
    frame.controls = {};
    frame.controls.primaryUp = true;
    static_cast<void>(pump.Frame(frame));
    frame.controls = {};
    frame.controls.secondaryDown = true;
    const std::vector<Species::PromoteWorkerCommand> promotions = CommandsOf<Species::PromoteWorkerCommand>(pump.Frame(frame));
    Assert::IsTrue(promotions.size() == 1 && promotions[0].actor == IdOf(2), L"the secondary press promotes");
    frame.ground = GROUND + Neuron::Vec3{100.0, 0.0, 0.0};
    frame.controls = {};
    static_cast<void>(pump.Frame(frame));
    Assert::IsFalse(pump.verbs.PromotableWorker().has_value(), L"none beyond the promote radius");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr124ASentCommandsSelectionHoldsUntilItsTickArrives)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-050")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-156")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr124ASentCommandsSelectionHoldsUntilItsTickArrives)
  {
    // A crowd order right after a circle resolves as the circle left the selection, before the replica shows it.
    Pump pump;
    std::uint64_t sequence = 0;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Worker, GROUND}});
    Species::VerbFrame frame = FrameAt(GROUND);
    frame.controls.primaryDown = true;
    frame.controls.primaryHeld = true;
    static_cast<void>(pump.FrameSent(frame, sequence));
    frame.controls = {};
    frame.controls.primaryUp = true;
    static_cast<void>(pump.FrameSent(frame, sequence));
    Assert::IsTrue(pump.verbs.WorkersSelected(), L"the circle's Workers, predicted");
    frame.controls = {};
    frame.controls.primaryDown = true;
    frame.ground = GROUND + Neuron::Vec3{300.0, 0.0, 0.0};
    std::vector<Species::BattleCommand> commands = pump.FrameSent(frame, sequence);
    Assert::IsTrue(CommandsOf<Species::OrderWorkersCommand>(commands).size() == 1 && CommandsOf<Species::SelectCommand>(commands).empty() &&
                     !pump.verbs.Circle().has_value(),
                   L"a crowd order, not a new circle");
    // Each report's tick arrives with no Worker under control: the replica stands again.
    for (std::uint64_t reported = 0; reported < sequence; ++reported)
    {
      pump.verbs.Reported({reported, 6, Species::CommandResult::Applied});
    }
    Fill(pump.replica, 6, {{1, Species::FigureKind::Worker, GROUND}});
    frame.controls = {};
    static_cast<void>(pump.Frame(frame));
    Assert::IsFalse(pump.verbs.WorkersSelected() || pump.verbs.LeaderSelected(), L"the replica's selection");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl109ADeadSelectedLeaderIsReleasedOnce)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-109")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl109ADeadSelectedLeaderIsReleasedOnce)
  {
    Pump pump;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Leader, GROUND, true, false, true}});
    const Species::VerbFrame frame = FrameAt(GROUND + Neuron::Vec3{0.0, 0.0, 500.0});
    const std::vector<Species::SelectCommand> selects = CommandsOf<Species::SelectCommand>(pump.Frame(frame));
    Assert::IsTrue(selects.size() == 1 && selects[0].actor == 0, L"one Select of nothing");
    Assert::IsFalse(pump.verbs.LeaderSelected(), L"a dead Leader is not the selection");
    Assert::IsTrue(CommandsOf<Species::SelectCommand>(pump.Frame(frame)).empty(), L"and only once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim438ThePointerGoesOnceAPeriodWhileALeaderIsSelected)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-438")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-168")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim438ThePointerGoesOnceAPeriodWhileALeaderIsSelected)
  {
    Pump pump;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Leader, GROUND, true}});
    Assert::IsTrue(CommandsOf<Species::ControlStateCommand>(pump.Frame(FrameAt(GROUND))).size() == 1, L"the pointer");
    Assert::IsTrue(CommandsOf<Species::ControlStateCommand>(pump.Frame(FrameAt(GROUND + Neuron::Vec3{5.0, 0.0, 0.0}))).empty(),
                   L"not again within the tick");
    Fill(pump.replica, 6, {{1, Species::FigureKind::Leader, GROUND, true}});
    Assert::IsTrue(CommandsOf<Species::ControlStateCommand>(pump.Frame(FrameAt(GROUND + Neuron::Vec3{5.0, 0.0, 0.0}))).size() == 1,
                   L"the next tick, moved");
    Fill(pump.replica, 7, {{1, Species::FigureKind::Leader, GROUND, true}});
    Assert::IsTrue(CommandsOf<Species::ControlStateCommand>(pump.Frame(FrameAt(GROUND + Neuron::Vec3{5.0, 0.0, 0.0}))).empty(),
                   L"unmoved, none");
    Fill(pump.replica, 8, {{1, Species::FigureKind::Leader, GROUND, false}});
    Assert::IsTrue(CommandsOf<Species::ControlStateCommand>(pump.Frame(FrameAt(GROUND))).empty(), L"nothing selected, none");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O77AClientOfNoTeamSendsNothing)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O77AClientOfNoTeamSendsNothing)
  {
    Pump pump;
    Fill(pump.replica, 5, {{1, Species::FigureKind::Worker, GROUND}});
    Species::VerbFrame frame = FrameAt(GROUND);
    frame.controls.primaryDown = true;
    frame.controls.secondaryDown = true;
    frame.controls.clear = true;
    Assert::IsTrue(pump.Frame(frame, std::nullopt).empty() && !pump.verbs.Circle().has_value() &&
                     !pump.verbs.PromotableWorker().has_value(),
                   L"no verbs while watching");
  }
};

} // namespace GameClientTests
