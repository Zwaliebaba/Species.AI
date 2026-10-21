#pragma once

#include "ClientSession.h"
#include "InputRouter.h"
#include "LooseContent.h"
#include "ReplicaTiming.h"
#include "SettlementCamera.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "WorldScene.h"

#include <memory>
#include <optional>
#include <span>
#include <string_view>

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
/// over the client's own Leaders, and the cursor it shows. Frame thread only.
class SettlementView
{
public:
  struct Desc
  {
    const TuningMvp* tuning = nullptr; // the client's records, outliving the view
    const TuningPresentation* presentation = nullptr;
    Neuron::ProjectionPlanes planes;
  };

  explicit SettlementView(const Desc& _desc);

  // Once per frame, after the session's and the world presentation's updates: _dtSeconds is the real frame step, _view
  // the client area the pointer is in.
  void Update(const ClientSession& _session, Neuron::InputRouter& _router, double _dtSeconds, Neuron::SizePx _view, bool _focused);

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

private:
  Desc m_desc;
  SettlementCamera m_camera;
  const RegionTerrain* m_terrain = nullptr; // the terrain the camera entered
  CameraBounds m_region;
  CursorKind m_cursor = CursorKind::World;
  std::optional<StableId> m_hovered;
  Neuron::PointPx m_pointer;
  bool m_confined = false;
};

} // namespace Species
