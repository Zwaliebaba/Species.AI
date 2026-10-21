#pragma once

#include "CommandMessages.h"
#include "ReplicaTiming.h"
#include "StableId.h"
#include "StateReplica.h"
#include "TuningPresentation.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <span>
#include <variant>
#include <vector>

namespace Species
{

// A command of the battle verbs (R-SIM-450), as the client sends it.
using BattleCommand = std::variant<SelectCommand, SelectWorkersCommand, OrderWorkersCommand, PromoteWorkerCommand, ToggleUnitCommand,
                                   OrderUnitCommand, DemoteLeaderCommand, ControlStateCommand>;

/// The verbs' actions of one frame, as the input router answered them (table 3.6-A rows 19 to 24 and 40 to 45). The
/// primary rows share one press, and the secondary rows another; O82 moves the orders to the primary button (ADR-127).
struct VerbControls
{
  bool primaryDown = false;   // action.select.pick, pick_idle and circle_begin, and action.order.crowd and special (O82)
  bool primaryHeld = false;   // action.select.circle_grow
  bool primaryUp = false;     // action.select.circle_end
  bool clear = false;         // action.select.clear
  bool secondaryDown = false; // action.order.promote
  bool straight = false;      // action.order.straight_route held
  bool moveOnly = false;      // action.order.move_modifier held
  bool demote = false;        // action.order.demote
};

/// What the verbs read of a frame: its controls, the pointer's ground point while it lies on the region (none while the
/// unavailable cursor shows), the client's own living Leader under the pointer (R-CTRL-055), the real frame step, and
/// whether the route preview finds no walkable route, when the unavailable cursor shows too (ADR-125).
struct VerbFrame
{
  VerbControls controls;
  std::optional<Neuron::Vec3> ground;
  std::optional<StableId> leaderUnderPointer;
  double realStepSeconds = 0.0;
  bool routeBlocked = false;
};

/// The local selection circle (R-CTRL-155 to R-CTRL-157): its centre on the ground and its radius, in wu, and how strongly
/// it is drawn: 1 while it is held, then falling to 0 over mvp.selection.circle_fade_seconds after its release (ADR-127).
struct SelectionCircle
{
  Neuron::Vec3 center;
  double radius = 0.0;
  double strength = 1.0;
};

/// The client's battle verbs (M1 task T17a, ADR-124; the buttons of O82, ADR-127): the match input pump of R-CTRL-020
/// step 8 for the verbs of T7.
/// - **The primary button** resolves to one meaning: it selects an own Leader under the pointer (R-CTRL-062); it orders
///   the selected Leader or the selected Workers to the pointer ground point, with Ctrl's straight route and Shift's
///   move-only order (R-CTRL-042 rows O1, O6 and O7, R-CTRL-043, R-CTRL-160); and with nothing selected it selects
///   nothing and begins the selection circle, which grows while held, to the larger of its growth on the real clock and
///   the pointer's farthest drag, and sends its crowd selection at most once a tick (R-CTRL-048, R-CTRL-155 to
///   R-CTRL-157).
/// - **The secondary button** toggles an own Leader under the pointer while nothing or that Leader is selected (row O3),
///   clears any other selection, and with nothing selected promotes a Worker (row O2). C demotes the selected Leader
///   (R-CTRL-044) and Space clears the selection.
/// - **The selection** is the replica's latest, with the effect of each command sent predicted until its tick has arrived,
///   so a press just after another resolves as the first left it. A selected Leader that died is released (R-CTRL-109).
/// - **The pointer** goes to the server while a Leader is selected, at most once a control-state period (R-SIM-438).
/// Nothing is sent while the client commands no team (O77). Frame thread only.
class BattleVerbs
{
public:
  // _presentation outlives the verbs.
  explicit BattleVerbs(const TuningPresentation& _presentation) noexcept;

  // Once per frame, after the camera: the commands of the frame, in the order they are sent. _team is the team the client
  // commands, _replica the session's replica, whose latest tick is the match time, and _figures the frame's drawn figures.
  [[nodiscard]] std::vector<BattleCommand> Update(const VerbFrame& _frame, std::optional<std::uint8_t> _team, const StateReplica& _replica,
                                                  std::span<const InterpolatedFigure> _figures);
  // A command of Update that was sent, with the sequence number the server gives it; and a command's outcome.
  void Sent(std::uint64_t _sequence, const BattleCommand& _command);
  void Reported(const CommandReport& _report);
  // A new world: the circle, the predictions and the times are dropped.
  void Reset() noexcept;

  // O78: the Worker a secondary press would promote this frame, for the highlight cursor.
  [[nodiscard]] std::optional<StableId> PromotableWorker() const noexcept
  {
    return m_promotable;
  }
  [[nodiscard]] const std::optional<SelectionCircle>& Circle() const noexcept
  {
    return m_circle;
  }
  // ADR-127: the circle to draw: the one held, or the last one fading after its release.
  [[nodiscard]] std::optional<SelectionCircle> ShownCircle() const noexcept
  {
    return m_circle.has_value() ? m_circle : m_fadingCircle;
  }
  // The selection the verbs resolve with this frame: whether a Leader is selected, its id when known, and whether
  // Workers are under control.
  [[nodiscard]] bool LeaderSelected() const noexcept
  {
    return m_leaderSelected;
  }
  [[nodiscard]] std::optional<StableId> SelectedLeader() const noexcept
  {
    return m_selectedLeader;
  }
  [[nodiscard]] bool WorkersSelected() const noexcept
  {
    return m_workersSelected;
  }

private:
  // A command's predicted selection: nothing, Workers, or a Leader, held until the replica holds the command's tick. A
  // promotion's Leader is selected before its id is known.
  struct Prediction
  {
    std::uint64_t sequence = 0;
    std::optional<std::uint64_t> tick; // its report's tick, once reported
    bool leader = false;
    StableId leaderId = 0; // 0: not known yet
    bool workers = false;
  };

  void ReadSelection(std::uint8_t _team, const StateReplica& _replica);
  [[nodiscard]] std::optional<StableId> FindPromotable(std::uint8_t _team, const VerbFrame& _frame,
                                                       std::span<const InterpolatedFigure> _figures) const;
  void Primary(std::uint8_t _team, const VerbFrame& _frame, std::uint64_t _tick, std::vector<BattleCommand>& _commands);
  void Secondary(std::uint8_t _team, const VerbFrame& _frame, std::vector<BattleCommand>& _commands);
  void FadeCircle(double _realStepSeconds) noexcept;

  const TuningPresentation* m_presentation;
  std::optional<SelectionCircle> m_circle;
  std::optional<SelectionCircle> m_fadingCircle; // the circle released, while it fades (ADR-127)
  double m_circleGrown = 0.0;                    // the circle's radius from its growth on the real clock alone, wu
  std::optional<std::uint64_t> m_crowdOrderTick; // the match time of the last crowd order or circle send (R-SIM-403)
  std::optional<std::uint64_t> m_controlStateTick;
  std::optional<Neuron::Vec3> m_controlStatePointer;
  std::vector<Prediction> m_predictions;
  bool m_leaderSelected = false;
  std::optional<StableId> m_selectedLeader; // its id, when known
  bool m_workersSelected = false;
  std::optional<StableId> m_promotable;
  std::optional<StableId> m_released; // the dead Leader whose selection was released (R-CTRL-109)
};

} // namespace Species
