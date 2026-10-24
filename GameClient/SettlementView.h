#pragma once

#include "BattleVerbs.h"
#include "ClientSession.h"
#include "Development.h"
#include "InputRouter.h"
#include "LooseContent.h"
#include "ReplicaTiming.h"
#include "RoutePreview.h"
#include "SettlementCamera.h"
#include "SettlementHud.h"
#include "SettlementVerbs.h"
#include "StewardAreas.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "WorldMarkers.h"
#include "WorldScene.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Species
{

// The cursors of mvp_assets.md table 7, in the order of CursorImages: the default pointer, over a selectable Leader,
// unavailable (ADR-119), and placing a structure (ADR-153).
enum class CursorKind : std::uint8_t
{
  World,
  Highlight,
  Unavailable,
  Place
};

// The cursors' art from the content (AST-014 keys `textures/cursor/<name>` and `<name>shadow`): none when a file is
// missing or damaged, which is logged.
[[nodiscard]] std::shared_ptr<const Neuron::CursorImages> LoadCursorImages(const Neuron::LooseContent& _content);

// R-CTRL-055 for the hover of M1 (ADR-119): the living Leader of _ownTeam, or from M3 of its expedition's team
// _expeditionTeam (ADR-167), whose picking sphere the ray meets with the smallest screen distance between the sphere's
// projected centre and the projected hit point; the earlier on a tie. The sphere grows with the camera's distance by the
// Worker distance scale factor _scaleFactor, as the Leader's figure does (R-UNIT-142, R-UNIT-192).
[[nodiscard]] std::optional<StableId> LeaderUnderRay(std::span<const InterpolatedFigure> _figures, std::uint8_t _ownTeam,
                                                     const Neuron::Ray& _ray, const Neuron::FrameView& _frame, Neuron::SizePx _view,
                                                     const TuningPresentation& _presentation, double _scaleFactor,
                                                     std::optional<std::uint8_t> _expeditionTeam = std::nullopt);

// MVP-049 (ADR-167): the team the battle verbs command: the settlement's team _team, or its expedition's _expeditionTeam
// while the expedition has living figures in _figures, from a primary press on one of its Leaders (_pressed, the Leader
// under the pointer at a primary press) until a press on one of the settlement's Leaders or the expedition's end. _current
// is the team they command now.
[[nodiscard]] std::uint8_t BattleTeamOf(std::span<const InterpolatedFigure> _figures, std::uint8_t _team, std::uint8_t _expeditionTeam,
                                        std::uint8_t _current, std::optional<StableId> _pressed) noexcept;

// The cursor of a kind at the pointer: its image, its side and hotspot (mvp.cursor.*, ref.ctrl.cursor.set.*) and colour.
[[nodiscard]] Neuron::CursorQuad CursorAt(CursorKind _kind, Neuron::PointPx _pointer, const TuningPresentation& _presentation) noexcept;

/// The settlement view of M1 (WLD-240, MVP-101; M1 task T14, ADR-119): the player camera entered over the steward's site
/// when the session's terrain arrives, advanced by the router's camera actions while the window has focus, the hover
/// over the client's own Leaders, the battle verbs that follow the camera (M1 task T17, ADR-124), whose commands it
/// sends through the session, and the cursor it shows; with T17b (ADR-125), the route preview of the selection, the
/// world markers, whose marker under the pointer the verbs read as a Leader under it, and the badge near the pointer;
/// with O82 (ADR-127), the primary button's orders, the ordered route kept fading, and the badge's count; with M2 task
/// T13 (ADR-142), the steward's areas, which bound the camera; with M2 task T17 (ADR-153), the settlement verbs, which take
/// a press over the HUD and the presses of placing before the battle verbs, with the placement cursor and ghost; with M3
/// task T12 (ADR-167), the conflict verbs of the relations panel, the battlefields that join the steward's areas, the
/// attacks' and claims' markers and its expedition's Leaders' markers, and the battle verbs on the expedition's team
/// from a press on one of its Leaders. Frame thread only.
class SettlementView
{
public:
  struct Desc
  {
    const TuningMvp* tuning = nullptr; // the client's records, outliving the view
    const TuningPresentation* presentation = nullptr;
    Neuron::ProjectionPlanes planes;
    bool scriptedVerbs = false;     // ADR-125: a Debug render check's scripted verbs in place of the router's
    double workerScaleFactor = 2.0; // render.worker_scale_factor (RND-442, ADR-150), which scales the picking sphere
  };

  explicit SettlementView(const Desc& _desc);

  // Once per frame, before the session's update: _dtSeconds is the real frame step, _view the client area the pointer is
  // in, and _hud the HUD whose zones of the last frame take the presses over them (UI-461), none where no HUD is drawn.
  // The verbs read the outcomes the session received since the last frame and send their commands through it.
  void Update(ClientSession& _session, Neuron::InputRouter& _router, double _dtSeconds, Neuron::SizePx _view, bool _focused,
              const SettlementHud* _hud);
  // CAM-060's entry pose about the ground point _ground in place of the steward's site, in the world entered: the frame
  // bench aims its fixed camera at a scene's fight this way (O79, ADR-126), O76's pose held across O108 (ADR-180).
  // Nothing while no world is entered.
  void EnterAt(const Neuron::Vec3& _ground);

  [[nodiscard]] bool InWorld() const noexcept
  {
    return m_camera.InWorld();
  }
  [[nodiscard]] Neuron::CameraPose Pose() const noexcept
  {
    return m_camera.Pose();
  }
  [[nodiscard]] const SettlementCamera& Camera() const noexcept
  {
    return m_camera;
  }
  // WLD-244: the steward's areas of this frame, from the session's replica.
  [[nodiscard]] const StewardAreas& Areas() const noexcept
  {
    return m_areas;
  }
  [[nodiscard]] CursorKind Cursor() const noexcept
  {
    return m_cursor;
  }
  [[nodiscard]] std::optional<StableId> HoveredLeader() const noexcept
  {
    return m_hovered;
  }
  // The cursor of this frame at the router's pointer.
  [[nodiscard]] Neuron::CursorQuad CursorDraw() const noexcept;
  [[nodiscard]] const BattleVerbs& Verbs() const noexcept
  {
    return m_verbs;
  }
  // ADR-095: the outcomes of the commands taken at the last update, in the order they arrived, which the frame bench reads
  // to time an order to its display (ADR-079, M3 task T16).
  [[nodiscard]] std::span<const CommandReport> Reports() const noexcept
  {
    return m_reports;
  }
  // ADR-167: the team the battle verbs command in this frame; none while they command none.
  [[nodiscard]] std::optional<std::uint8_t> BattleTeam() const noexcept
  {
    return m_battleTeam;
  }
  // ADR-153: the settlement verbs: placing, the priorities and the refusals the HUD shows.
  [[nodiscard]] const SettlementVerbs& Settlement() const noexcept
  {
    return m_settlement;
  }
  [[nodiscard]] const RoutePreview& Preview() const noexcept
  {
    return m_preview;
  }
  [[nodiscard]] const WorldMarkers& Markers() const noexcept
  {
    return m_markers;
  }
  // R-CTRL-366: the badge near the pointer of this frame; none while nothing is selected.
  [[nodiscard]] const std::vector<Neuron::OverlayQuad>& Badge() const noexcept
  {
    return m_badge;
  }
  // ADR-127, ADR-145: the badge's count of the selected Workers, which the UI draws in the body role (ADR-147); none while no Worker is
  // selected.
  [[nodiscard]] const std::optional<BadgeLabel>& BadgeCountLabel() const noexcept
  {
    return m_badgeCount;
  }
  // The client area of the last update, in pixels.
  [[nodiscard]] Neuron::SizePx ViewSize() const noexcept
  {
    return m_viewSize;
  }

private:
  // The preview's start and end (O78): a selected Leader's position, unless a press would form it up (R-UNIT-170), or
  // the centre of the selected Workers (R-SIM-402); and the order target (R-CTRL-057).
  void UpdatePreview(const VerbFrame& _frame, std::span<const InterpolatedFigure> _figures, std::uint8_t _team);
#if ENGINE_DEVELOPMENT
  void ScriptVerbs(VerbFrame& _frame, std::span<const InterpolatedFigure> _figures, std::uint8_t _team);
#endif

  Desc m_desc;
  SettlementCamera m_camera;
  StewardAreas m_areas;
  const RegionTerrain* m_terrain = nullptr; // the terrain the camera entered
  CameraBounds m_region;
  BattleVerbs m_verbs;
  std::vector<CommandReport> m_reports;
  SettlementVerbs m_settlement;
  RoutePreview m_preview;
  WorldMarkers m_markers;
  std::vector<Neuron::OverlayQuad> m_badge;
  std::optional<BadgeLabel> m_badgeCount;
  Neuron::SizePx m_viewSize;
  std::size_t m_selectedWorkers = 0;        // the local team's living Workers under control in this frame's figures
  std::optional<std::uint8_t> m_battleTeam; // ADR-167: the settlement's team or its expedition's
  std::vector<StewardAreas::Field> m_fields;
  std::vector<MarkerTarget> m_targets;
  CursorKind m_cursor = CursorKind::World;
  std::optional<StableId> m_hovered;
  Neuron::PointPx m_pointer;
  bool m_confined = false;
#if ENGINE_DEVELOPMENT
  std::uint32_t m_scriptStep = 0;
#endif
};

} // namespace Species
