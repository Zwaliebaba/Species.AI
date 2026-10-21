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
/// primary rows share one press, and the secondary rows another.
struct VerbControls
{
  bool primaryDown = false;   // action.select.pick, pick_idle and circle_begin
  bool primaryHeld = false;   // action.select.circle_grow
  bool primaryUp = false;     // action.select.circle_end
  bool clear = false;         // action.select.clear
  bool secondaryDown = false; // action.order.crowd, promote and special
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

/// The local selection circle (R-CTRL-155 to R-CTRL-157): its centre on the ground and its radius, in wu.
struct SelectionCircle
{
  Neuron::Vec3 center;
  double radius = 0.0;
};

/// The client's battle verbs (M1 task T17a, ADR-124): the match input pump of R-CTRL-020 step 8 for the verbs of T7.
/// - **The primary button** selects an own Leader under the pointer, or, with nothing selected, selects nothing and begins
///   the selection circle, which grows while held and sends its crowd selection at most once a tick (R-CTRL-041 rows S1
///   to S6, R-CTRL-048, R-CTRL-062, R-CTRL-155 to R-CTRL-157).
/// - **The secondary button** resolves to one meaning (R-CTRL-050): the toggle of an own Leader, the order of the selected
///   Leader, the crowd order or a promotion, with Ctrl's straight route and Shift's move-only order (R-CTRL-042, R-CTRL-043,
///   R-CTRL-073, R-CTRL-160). C demotes the selected Leader (R-CTRL-044) and Space clears the selection.
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
  void Secondary(std::uint8_t _team, const VerbFrame& _frame, std::span<const InterpolatedFigure> _figures, std::uint64_t _tick,
                 std::vector<BattleCommand>& _commands);

  const TuningPresentation* m_presentation;
  std::optional<SelectionCircle> m_circle;
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
