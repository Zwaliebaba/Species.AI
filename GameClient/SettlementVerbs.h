#pragma once

#include "CommandMessages.h"
#include "Development.h"
#include "RelationMessages.h"
#include "SettlementHud.h"
#include "SettlementMessages.h"
#include "StateReplica.h"
#include "StewardAreas.h"
#include "StructureLook.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "Vec3.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace Species
{

// A command of the settlement verbs (MVP-101), as the client sends it; from M3 task T12 (ADR-167) the conflict verbs', and
// from O106 (ADR-178) the growth hold.
using SettlementCommand = std::variant<PlaceStructureCommand, SetPrioritiesCommand, MakeClaimCommand, WithdrawClaimCommand,
                                       SendTributeCommand, DeclareAttackCommand, WithdrawAttackCommand, HoldGrowthCommand>;

/// MVP-101 (ADR-167): what the conflict verbs read of the client's settlement: the latest valley status, its local site,
/// the Leaders that would lead an expedition, and its stores.
struct ConflictView
{
  std::optional<ValleyStatus> valley;
  std::uint8_t site = 0;
  std::vector<StableId> leaders;
  std::int64_t storesMilliRu = 0;
};

// ADR-167: the living Leaders of team _team in _latest that would lead an expedition: _selected first while it is one of
// them, then the others in id order, at most EXPEDITION_LEADERS_MAX.
[[nodiscard]] std::vector<StableId> ExpeditionLeadersOf(const StateReplica::View& _latest, std::uint8_t _team,
                                                        std::optional<StableId> _selected);
// MVP-042, MVP-101 (ADR-167): the attack a relations row would declare, the button _button its zone index: a raid on a
// Cladan's settlement, unprovoked when its own Cladan holds no grievance against the target and claims nothing the target
// holds (PIL-063), or the battle of the steward's open or contested claim, against the area's holder or the rival
// claimant; with its cost, as the server charges it. None for a row that names no attack or no target.
[[nodiscard]] std::optional<PendingAttack> AttackOf(const RelationRow& _row, std::uint8_t _button, const ConflictView& _view,
                                                    const TuningMvp& _tuning);

// MVP-036: _shares, which sum to 100, with _task's share moved by _deltaPercent and the others moved the other way so
// that they still sum to 100. A raise takes half the step from each other task, the larger giving the odd point; a lower
// gives half to each, the smaller taking the odd point; a tie goes to the earlier task. No share passes 0 or 100: a
// raise beyond what the others hold takes what they hold, and a lower beyond the task's share gives what it holds.
[[nodiscard]] std::array<std::uint8_t, 3> ShiftPriority(std::array<std::uint8_t, 3> _shares, std::size_t _task,
                                                        std::int64_t _deltaPercent) noexcept;

/// What the client holds of its settlement for a placement (ADR-153), from the replica's latest tick and the latest status.
struct PlacementView
{
  std::optional<ReplicatedSettlement> settlement; // the client's own; none: it places nothing
  std::optional<Neuron::Vec3> wellspring;         // its Wellspring's position
  std::int64_t dwellings = 0;                     // its dwellings, built or queued (MVP-034)
  std::optional<SettlementStatus> status;         // its latest status (MVP-098)
};

// The view of the client's settlement of team _team in _latest, with _status when it is that team's.
[[nodiscard]] PlacementView PlacementViewOf(const StateReplica::View& _latest, std::optional<std::uint8_t> _team,
                                            const std::optional<SettlementStatus>& _status);

// ADR-153: the refusal the client foresees for placing _kind at _point, in MVP-032's order, from what it holds: the
// latest status's queue (QueueFull), the territory's cell under the point (OutsideTerritory), the point inside the
// centre ring about the Wellspring (InCenterRing), and the stores against the cost (CannotAfford). None when it foresees
// none. It never decides: the server's checks, which read the structure's footprint, have the last word.
[[nodiscard]] std::optional<PlacementRefusal> PredictPlacement(const PlacementView& _view, const StewardAreas& _areas, StructureKind _kind,
                                                               const Neuron::Vec3& _point, const TuningMvp& _tuning);

/// What the settlement verbs read of a frame: the primary and secondary presses of the battle verbs' controls (O82), the
/// HUD's zone under the pointer in the last frame's layout (UI-461), the pointer's ground point while it lies on the
/// region, and the real frame step.
struct SettlementFrame
{
  bool primaryDown = false;
  bool secondaryDown = false;
  std::optional<HudZoneRect> hudZone;
  std::optional<Neuron::Vec3> ground;
  double realStepSeconds = 0.0;
  // ADR-167: the relations rows of the HUD's last layout, which its buttons' zones index, and the conflict verbs' view.
  std::span<const RelationRow> relations;
  ConflictView conflict;
};

/// A frame's commands of the settlement verbs, and the presses they took, which reach no battle verb.
struct SettlementVerbResult
{
  std::vector<SettlementCommand> commands;
  bool tookPrimary = false;
  bool tookSecondary = false;
  bool openAwayReport = false; // O109 (ADR-188): the press on the away report's button
};

/// The client's settlement verbs (M2 task T17, ADR-153; MVP-032, MVP-036, MVP-101):
/// - **The HUD.** A press over the HUD reaches no world verb. The primary press on a construction bar entry enters
///   placing that structure, or leaves it when it is the one being placed; on a priority's arrow it raises or lowers that
///   share by mvp.hud.priority_step and sends the shares, which the HUD shows until a status holds them; on the growth
///   row's button it holds the growth or lets it run (O106, ADR-178), shown in the same way.
/// - **Placing** is a mode that takes the primary and secondary presses before the battle verbs: the primary press on the
///   region sends the placement at the pointer ground point and stays in the mode; the secondary press leaves it, as it
///   releases a selection (O82). The mode shows the ghost and the cursor of what the client foresees at the pointer.
/// - **Refusals.** The server's refusal of a placement is kept mvp.hud.notice_seconds for the HUD with its reason.
/// - **Conflicts** (M3 task T12, ADR-167). A relations row's button makes a claim, withdraws one, withdraws the
///   expedition or sends a grievance's tribute at once. A raid or a claim battle is declared in two presses: the first
///   shows the attack with its cost and whether it is unprovoked, a second on the same button declares it, and the
///   secondary press, or any other press over the HUD, drops it. Their refusals are kept as a placement's is.
/// - **Results** (M3 task T13, ADR-168). A press on the result card closes the result it shows.
/// - **The away report** (O109, ADR-188). A press on the always-on row's button asks for the latest away report's page.
/// Nothing is sent while the client commands no settlement. Frame thread only.
class SettlementVerbs
{
public:
  // _tuning and _presentation outlive the verbs.
  SettlementVerbs(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept;

  // Once per frame, after the camera: the commands of the frame and the presses taken. _team is the team the client
  // commands, _view its settlement as PlacementViewOf gives it, and _areas the steward's areas of the frame.
  [[nodiscard]] SettlementVerbResult Update(const SettlementFrame& _frame, std::optional<std::uint8_t> _team, const PlacementView& _view,
                                            const StewardAreas& _areas);
  // A command of Update that was sent, with the sequence number the server gives it; and a command's outcome, which
  // the verbs ignore when it is not one of theirs.
  void Sent(std::uint64_t _sequence, const SettlementCommand& _command);
  void Reported(const CommandReport& _report);
  // A new world: the mode, the refusal, the shares and hold sent and the attack being declared are dropped.
  void Reset() noexcept;
  // ADR-167: the attack being declared.
  [[nodiscard]] const std::optional<PendingAttack>& Pending() const noexcept
  {
    return m_pending;
  }

  [[nodiscard]] std::optional<StructureKind> Placing() const noexcept
  {
    return m_placing;
  }
  // The refusal the client foresees at the pointer this frame while placing (PredictPlacement).
  [[nodiscard]] std::optional<PlacementRefusal> Foreseen() const noexcept
  {
    return m_foreseen;
  }
  // The ghost of this frame while placing with the pointer on the region.
  [[nodiscard]] const std::optional<StructureGhost>& Ghost() const noexcept
  {
    return m_ghost;
  }
  // What the HUD shows of the verbs.
  [[nodiscard]] HudVerbs ForHud() const;
#if ENGINE_DEVELOPMENT
  // ADR-153: a Debug render check's ghost for this frame, drawn without the mode, so that the scripted battle verbs keep
  // their presses (ADR-125).
  void ScriptGhost(const StructureGhost& _ghost) noexcept
  {
    m_ghost = _ghost;
  }
#endif

private:
  struct SentShares
  {
    std::array<std::uint8_t, 3> shares{};
    std::uint64_t sequence = 0;
    std::optional<std::uint64_t> tick; // its report's tick, once applied
  };
  struct SentHold
  {
    bool held = false;
    std::uint64_t sequence = 0;
    std::optional<std::uint64_t> tick; // its report's tick, once applied
  };

  void PressHud(const HudZoneRect& _zone, std::uint8_t _team, const PlacementView& _view, const SettlementFrame& _frame,
                SettlementVerbResult& _result);
  void PressRelation(std::uint8_t _button, std::uint8_t _team, const SettlementFrame& _frame, SettlementVerbResult& _result);

  const TuningMvp& m_tuning;
  const TuningPresentation& m_presentation;
  std::optional<StructureKind> m_placing;
  std::optional<PlacementRefusal> m_foreseen;
  std::optional<StructureGhost> m_ghost;
  std::vector<std::uint64_t> m_placements; // the sequences of the placements sent and not yet reported
  std::optional<PlacementRefusal> m_refusal;
  double m_refusalAgeSeconds = 0.0;
  std::optional<SentShares> m_shares;
  std::optional<SentHold> m_hold;                                         // O106 (ADR-178)
  std::optional<PendingAttack> m_pending;                                 // ADR-167
  std::vector<std::pair<std::uint64_t, ConflictKind>> m_conflicts;        // the conflict commands sent and not yet reported
  std::optional<std::pair<ConflictKind, std::uint8_t>> m_conflictRefusal; // the last refused, and its reason
  double m_conflictRefusalAgeSeconds = 0.0;
  std::uint64_t m_resultsClosed = 0; // ADR-168: the presses on the result card, never reset, so the HUD can count them
};

} // namespace Species
