#pragma once

#include "BattleVerbs.h"
#include "ClientSession.h"
#include "Development.h"
#include "InputRouter.h"
#include "LooseContent.h"
#include "ReplicaTiming.h"
#include "RoutePreview.h"
#include "SettlementCamera.h"
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

// The cursors of mvp_assets.md table 7, in the order of CursorImages: the default pointer, over a selectable Leader, and
// unavailable (ADR-119).
enum class CursorKind : std::uint8_t
{
  World,
  Highlight,
  Unavailable
};

// rnd.option.worker_scale_factor.default: the Worker distance scale factor, which also scales a Leader's picking sphere
// with the camera's distance (R-UNIT-142, R-UNIT-192). A named constant until the settings store exists (ADR-116).
inline constexpr double WORKER_DISTANCE_SCALE_FACTOR = 2.0;

// The cursors' art from the content (AST-014 keys `textures/cursor/<name>` and `<name>shadow`): none when a file is
// missing or damaged, which is logged.
[[nodiscard]] std::shared_ptr<const Neuron::CursorImages> LoadCursorImages(const Neuron::LooseContent& _content);

// R-CTRL-055 for the hover of M1 (ADR-119): the living Leader of _ownTeam whose picking sphere the ray meets with the
// smallest screen distance between the sphere's projected centre and the projected hit point; the earlier on a tie.
[[nodiscard]] std::optional<StableId> LeaderUnderRay(std::span<const InterpolatedFigure> _figures, std::uint8_t _ownTeam,
                                                     const Neuron::Ray& _ray, const Neuron::FrameView& _frame, Neuron::SizePx _view,
                                                     const TuningPresentation& _presentation);

// The cursor of a kind at the pointer: its image, its side and hotspot (mvp.cursor.*, ref.ctrl.cursor.set.*) and colour.
[[nodiscard]] Neuron::CursorQuad CursorAt(CursorKind _kind, Neuron::PointPx _pointer, const TuningPresentation& _presentation) noexcept;

/// The settlement view of M1 (WLD-240, MVP-101; M1 task T14, ADR-119): the player camera entered over the steward's site
/// when the session's terrain arrives, advanced by the router's camera actions while the window has focus, the hover
/// over the client's own Leaders, the battle verbs that follow the camera (M1 task T17, ADR-124), whose commands it
/// sends through the session, and the cursor it shows; with T17b (ADR-125), the route preview of the selection, the
/// world markers, whose marker under the pointer the verbs read as a Leader under it, and the badge near the pointer;
/// with O82 (ADR-127), the primary button's orders, the ordered route kept fading, and the badge's count; with M2 task
/// T13 (ADR-142), the steward's areas, which bound the camera. Frame thread only.
class SettlementView
{
public:
  struct Desc
  {
    const TuningMvp* tuning = nullptr; // the client's records, outliving the view
    const TuningPresentation* presentation = nullptr;
    Neuron::ProjectionPlanes planes;
    bool scriptedVerbs = false; // ADR-125: a Debug render check's scripted verbs in place of the router's
  };

  explicit SettlementView(const Desc& _desc);

  // Once per frame, before the session's update: _dtSeconds is the real frame step, _view the client area the pointer is
  // in. The verbs read the outcomes the session received since the last frame and send their commands through it.
  void Update(ClientSession& _session, Neuron::InputRouter& _router, double _dtSeconds, Neuron::SizePx _view, bool _focused);
  // CAM-060's entry pose about the ground point _ground in place of the steward's site, in the world entered: the frame
  // bench aims its fixed camera at a scene's fight this way (O79, ADR-126). Nothing while no world is entered.
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
  // ADR-127, ADR-145: the badge's count of the selected Workers, which the UI draws in GameFont; none while no Worker is
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
  RoutePreview m_preview;
  WorldMarkers m_markers;
  std::vector<Neuron::OverlayQuad> m_badge;
  std::optional<BadgeLabel> m_badgeCount;
  Neuron::SizePx m_viewSize;
  std::size_t m_selectedWorkers = 0; // the local team's living Workers under control in this frame's figures
  CursorKind m_cursor = CursorKind::World;
  std::optional<StableId> m_hovered;
  Neuron::PointPx m_pointer;
  bool m_confined = false;
#if ENGINE_DEVELOPMENT
  std::uint32_t m_scriptStep = 0;
#endif
};

} // namespace Species
