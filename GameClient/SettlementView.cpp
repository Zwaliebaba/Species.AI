#include "pch.h"

#include "AssetError.h"
#include "InputBindings.h"
#include "Log.h"
#include "SettlementView.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Species
{

namespace
{

// The cursors' content keys, in CursorKind order (Design/implementation/mvp_assets.md §6).
constexpr std::array<std::string_view, 3> CURSOR_KEYS{"textures/cursor/world", "textures/cursor/highlight", "textures/cursor/unavailable"};

constexpr float COLOR_LEVELS = 255.0f; // ref.ctrl.cursor.set.unavailable_color: RGB from 0 to 255
#if ENGINE_DEVELOPMENT
// ADR-125: how far the render check's script orders its Leader, and puts the pointer beyond its Workers, wu.
constexpr double SCRIPT_REACH = 300.0;
#endif

} // namespace

std::shared_ptr<const Neuron::CursorImages> LoadCursorImages(const Neuron::LooseContent& _content)
{
  auto images = std::make_shared<Neuron::CursorImages>();
  for (const std::string_view key : CURSOR_KEYS)
  {
    const std::string shadowKey = std::string(key) + "shadow";
    auto art = _content.Read(key);
    auto shadow = _content.Read(shadowKey);
    if (!art || !shadow)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the cursor {}: {}", key,
                  Neuron::FormatAssetError(art ? shadow.error() : art.error()));
      return nullptr;
    }
    images->art.push_back(std::move(*art));
    images->shadows.push_back(std::move(*shadow));
  }
  return images;
}

std::optional<StableId> LeaderUnderRay(std::span<const InterpolatedFigure> _figures, std::uint8_t _ownTeam, const Neuron::Ray& _ray,
                                       const Neuron::FrameView& _frame, Neuron::SizePx _view, const TuningPresentation& _presentation,
                                       double _scaleFactor)
{
  const TuningPresentation::Ref::Unit& unit = _presentation.ref.unit;
  std::optional<StableId> best;
  double bestScore = std::numeric_limits<double>::infinity();
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.dead || figure.state.kind != FigureKind::Leader || figure.state.team != _ownTeam)
    {
      continue;
    }
    // R-UNIT-192: the sphere above the Leader, scaled with the camera's distance as its figure is (R-UNIT-142).
    const Neuron::Vec3 center = figure.position + Neuron::Vec3{0.0, unit.leader.pickCenterLift, 0.0};
    const double distance = Neuron::Length(center - _frame.camera);
    const double scale = 1.0 + _scaleFactor * std::max(0.0, distance - unit.worker.cardScaleStart) / unit.worker.cardScaleDivisor;
    const std::optional<double> along = Neuron::RaySphere(_ray, center, unit.leader.pickRadius * scale);
    if (!along)
    {
      continue;
    }
    // R-CTRL-055 step 3: the score is the screen distance from the sphere's centre to the hit, not the depth.
    const std::optional<Neuron::ScreenPoint> centerOnScreen = Neuron::ProjectToScreen(center, _frame, _view);
    const std::optional<Neuron::ScreenPoint> hitOnScreen = Neuron::ProjectToScreen(_ray.origin + _ray.direction * *along, _frame, _view);
    if (!centerOnScreen || !hitOnScreen)
    {
      continue;
    }
    const double score = std::hypot(centerOnScreen->x - hitOnScreen->x, centerOnScreen->y - hitOnScreen->y);
    if (score < bestScore)
    {
      bestScore = score;
      best = figure.id;
    }
  }
  return best;
}

Neuron::CursorQuad CursorAt(CursorKind _kind, Neuron::PointPx _pointer, const TuningPresentation& _presentation) noexcept
{
  // R-CTRL-352: the top-left corner at the pointer less the side × the hotspot.
  const TuningPresentation::Mvp::Cursor& sizes = _presentation.mvp.cursor;
  const TuningPresentation::Ref::Ctrl::Cursor::Set& set = _presentation.ref.ctrl.cursor.set;
  Neuron::CursorQuad quad;
  quad.image = static_cast<std::uint8_t>(_kind);
  double hotspotX = sizes.worldHotspot.x;
  double hotspotY = sizes.worldHotspot.y;
  double side = sizes.worldSize;
  switch (_kind)
  {
  case CursorKind::World:
    break;
  case CursorKind::Highlight:
    side = sizes.highlightSize;
    hotspotX = set.highlightHotspot.x;
    hotspotY = set.highlightHotspot.y;
    break;
  case CursorKind::Unavailable:
    side = sizes.unavailableSize;
    hotspotX = set.unavailableHotspot.x;
    hotspotY = set.unavailableHotspot.y;
    quad.color = {static_cast<float>(set.unavailableColor.r) / COLOR_LEVELS, static_cast<float>(set.unavailableColor.g) / COLOR_LEVELS,
                  static_cast<float>(set.unavailableColor.b) / COLOR_LEVELS, 1.0f};
    break;
  }
  quad.sidePixels = static_cast<float>(side);
  quad.left = static_cast<float>(_pointer.x) - static_cast<float>(side * hotspotX);
  quad.top = static_cast<float>(_pointer.y) - static_cast<float>(side * hotspotY);
  return quad;
}

SettlementView::SettlementView(const Desc& _desc)
  : m_desc(_desc),
    m_camera(*_desc.presentation, _desc.planes),
    m_verbs(*_desc.presentation),
    m_preview(*_desc.presentation, *_desc.tuning),
    m_markers(*_desc.presentation)
{
}

void SettlementView::UpdatePreview(const VerbFrame& _frame, std::span<const InterpolatedFigure> _figures, std::uint8_t _team)
{
  // O82 (ADR-127): the order target is the pointer ground point; a primary press on an own Leader under the pointer
  // selects it, so it shows no route.
  const std::optional<Neuron::Vec3> to = _frame.leaderUnderPointer.has_value() ? std::nullopt : _frame.ground;
  std::optional<Neuron::Vec3> from;
  Neuron::Vec3 sum{};
  std::size_t workers = 0;
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.team != _team || figure.state.dead)
    {
      continue;
    }
    if (figure.state.kind == FigureKind::Leader && figure.id == m_verbs.SelectedLeader())
    {
      from = figure.position;
    }
    if (figure.state.kind == FigureKind::Worker && figure.state.underControl)
    {
      sum = sum + figure.position;
      ++workers;
    }
  }
  m_selectedWorkers = workers;
  if (m_verbs.LeaderSelected())
  {
    // R-UNIT-170: a press nearer the Leader than the toggle radius forms it up, and walks no route.
    if (from.has_value() && to.has_value() && Neuron::Length(*to - *from) < m_desc.tuning->ref.unit.leader.formationToggleRadius)
    {
      from.reset();
    }
  }
  else if (m_verbs.WorkersSelected() && workers > 0)
  {
    // R-SIM-402: the selected Workers' centre; with none in the replica yet, there is no centre and no preview.
    from = sum * (1.0 / static_cast<double>(workers));
  }
  else
  {
    from.reset();
  }
  if (m_terrain != nullptr)
  {
    m_preview.Update(from, to, _frame.controls.straight, *m_terrain);
  }
}

void SettlementView::Update(ClientSession& _session, Neuron::InputRouter& _router, double _dtSeconds, Neuron::SizePx _view, bool _focused)
{
  // CAM-060 and CAM-063 as amended: a new terrain is a world entered, over the steward's site (mvp.sandbox.steward_site
  // while the sandbox is the only world, ADR-109).
  const RegionTerrain* terrain = _session.Terrain();
  if (terrain != m_terrain)
  {
    m_terrain = terrain;
    m_camera.LeaveWorld();
    m_areas.Clear();
    m_verbs.Reset();
    m_preview.Reset();
    m_markers.Clear();
#if ENGINE_DEVELOPMENT
    m_scriptStep = 0;
#endif
    if (terrain != nullptr)
    {
      const RegionLayout& layout = terrain->layout;
      m_region = {layout.originX, layout.originX + layout.side, layout.originZ, layout.originZ + layout.side};
      const auto site = static_cast<std::size_t>(std::clamp<std::int64_t>(m_desc.tuning->mvp.sandbox.stewardSite, 0, 3));
      m_camera.EnterWorld(terrain->field, layout.sites[site], m_region);
    }
  }
  // CAM-032: the camera advances only with focus, and the pointer is held in the window while it does (INP-159).
  const bool advancing = m_camera.InWorld() && _focused;
  if (advancing != m_confined)
  {
    _router.ConfinePointer(advancing);
    m_confined = advancing;
  }
  m_pointer = _router.Pointer().positionPixels;
  m_viewSize = _view;
  // WLD-244 (ADR-142): the steward's areas from the territory the replica holds; a sandbox has none.
  const std::optional<std::uint8_t> ownTeam = _session.CommandedTeam();
  if (terrain != nullptr)
  {
    m_areas.Update(_session.Replica().Latest(), ownTeam, terrain->layout, *m_desc.tuning);
  }
  if (m_camera.InWorld() && terrain != nullptr)
  {
    if (advancing)
    {
      CameraControls controls;
      controls.panLeft = _router.Query(ActionOf(GameAction::CameraPanLeft));
      controls.panRight = _router.Query(ActionOf(GameAction::CameraPanRight));
      controls.panForward = _router.Query(ActionOf(GameAction::CameraPanForward));
      controls.panBack = _router.Query(ActionOf(GameAction::CameraPanBack));
      controls.fastPan = _router.Query(ActionOf(GameAction::CameraFastPan));
      controls.raise = _router.Query(ActionOf(GameAction::CameraRaise));
      controls.lower = _router.Query(ActionOf(GameAction::CameraLower));
      controls.zoom = _router.Query(ActionOf(GameAction::CameraZoom));
      controls.lookAround = _router.Query(ActionOf(GameAction::CameraLookAround));
      Neuron::ActionValue wheel;
      if (_router.Query(ActionOf(GameAction::CameraHeightWheel), &wheel))
      {
        controls.wheelNotches = wheel.x;
      }
      if (const std::optional<Neuron::PointPx> warp =
            m_camera.Advance(_dtSeconds, controls, m_pointer, _view, terrain->field, &m_areas).warpTo)
      {
        _router.WarpPointer(*warp);
        m_pointer = _router.Pointer().positionPixels;
      }
    }
    else
    {
      m_camera.UpdatePointerGround(m_pointer, _view, terrain->field);
    }
  }
  // The hover over the client's own Leaders (R-CTRL-055), and the cursor it gives.
  m_hovered.reset();
  std::vector<InterpolatedFigure> figures;
  if (m_camera.InWorld() && ownTeam.has_value())
  {
    const Neuron::FrameView frame = m_camera.View(_view);
    const Neuron::Ray ray = Neuron::PickRayAt(static_cast<double>(m_pointer.x), static_cast<double>(m_pointer.y), frame, _view,
                                              m_desc.presentation->cam.pick.rowOffset);
    figures = InterpolateFigures(_session.Replica(), _session.InterpolationFraction());
    m_hovered = LeaderUnderRay(figures, *ownTeam, ray, frame, _view, *m_desc.presentation, m_desc.workerScaleFactor);
  }
  // ADR-119 (O76): unavailable while the pointer's ray misses the terrain or its ground point lies outside the region.
  const auto& ground = m_camera.PointerGround();
  const bool onRegion = ground.has_value() && ground->onTerrain && ground->position.x >= m_region.minX &&
                        ground->position.x <= m_region.maxX && ground->position.z >= m_region.minZ && ground->position.z <= m_region.maxZ;
  // M1 task T17 (ADR-124): the battle verbs follow the camera (R-CTRL-020 step 8), with the outcomes of their commands.
  for (const CommandReport& report : _session.TakeReports())
  {
    m_verbs.Reported(report);
  }
  VerbFrame verbs;
  verbs.realStepSeconds = _dtSeconds;
  // R-CTRL-387: the marker under the pointer at the end of the last frame stands for its Leader; and the last frame's
  // preview that found no walkable route holds the orders back, as its unavailable cursor shows (ADR-125).
  verbs.leaderUnderPointer = m_hovered.has_value() ? m_hovered : m_markers.UnderPointer();
  verbs.routeBlocked = m_preview.Shown() && !m_preview.Walkable();
  if (onRegion)
  {
    verbs.ground = ground->position;
  }
  if (advancing)
  {
    VerbControls& controls = verbs.controls;
    controls.primaryDown = _router.Query(ActionOf(GameAction::SelectPick)) || _router.Query(ActionOf(GameAction::SelectPickIdle)) ||
                           _router.Query(ActionOf(GameAction::SelectCircleBegin)) || _router.Query(ActionOf(GameAction::OrderCrowd)) ||
                           _router.Query(ActionOf(GameAction::OrderSpecial));
    controls.primaryHeld = _router.Query(ActionOf(GameAction::SelectCircleGrow));
    controls.primaryUp = _router.Query(ActionOf(GameAction::SelectCircleEnd));
    controls.clear = _router.Query(ActionOf(GameAction::SelectClear));
    controls.secondaryDown = _router.Query(ActionOf(GameAction::OrderPromote));
    controls.straight = _router.Query(ActionOf(GameAction::OrderStraightRoute));
    controls.moveOnly = _router.Query(ActionOf(GameAction::OrderMoveModifier));
    controls.demote = _router.Query(ActionOf(GameAction::OrderDemote));
  }
  const std::optional<std::uint8_t> verbTeam = m_camera.InWorld() ? ownTeam : std::nullopt;
#if ENGINE_DEVELOPMENT
  if (m_desc.scriptedVerbs && verbTeam.has_value())
  {
    ScriptVerbs(verbs, figures, *verbTeam);
  }
#endif
  bool ordered = false;
  for (const BattleCommand& command : m_verbs.Update(verbs, verbTeam, _session.Replica(), figures))
  {
    const std::optional<std::uint64_t> sequence =
      std::visit([&_session](const auto& _command) { return _session.SendCommand(_command); }, command);
    if (sequence.has_value())
    {
      m_verbs.Sent(*sequence, command);
      ordered = ordered || std::holds_alternative<OrderWorkersCommand>(command) || std::holds_alternative<OrderUnitCommand>(command);
    }
  }
  // ADR-127 (RND-351): an order sent keeps the route its preview showed, which then fades.
  if (ordered)
  {
    m_preview.Commit();
  }
  m_preview.Advance(_dtSeconds);
  // M1 task T17b (ADR-125): the route preview, the world markers and the badge near the pointer, with its count
  // (ADR-127).
  m_badge.clear();
  m_badgeCount.reset();
  m_selectedWorkers = 0;
  if (verbTeam.has_value())
  {
    const std::uint8_t team = verbTeam.value_or(0);
    UpdatePreview(verbs, figures, team);
    MarkerView markers;
    markers.frame = m_camera.View(_view);
    markers.camera = m_camera.Pose().position;
    markers.cameraFront = Neuron::ViewDirection(m_camera.Pose());
    markers.size = _view;
    markers.pointer = m_pointer;
    markers.realStepSeconds = _dtSeconds;
    m_markers.Update(figures, verbTeam, m_verbs.SelectedLeader(), markers);
    m_badge = CursorBadge(m_verbs.LeaderSelected(), m_verbs.WorkersSelected(), team, m_pointer, _view, *m_desc.presentation);
    m_badgeCount = BadgeCount(m_badge, !m_verbs.LeaderSelected() && m_verbs.WorkersSelected() ? m_selectedWorkers : 0);
  }
  else
  {
    m_preview.Reset();
    m_markers.Clear();
  }
  // ADR-119, O76: unavailable off the region and, from T17, while the preview finds no walkable route; the highlight
  // marks an own Leader under the pointer and, by O78, the Worker a secondary press would promote.
  if (m_camera.InWorld() && (!onRegion || (m_preview.Shown() && !m_preview.Walkable())))
  {
    m_cursor = CursorKind::Unavailable;
  }
  else if (m_hovered.has_value() || m_verbs.PromotableWorker().has_value())
  {
    m_cursor = CursorKind::Highlight;
  }
  else
  {
    m_cursor = CursorKind::World;
  }
}

#if ENGINE_DEVELOPMENT
void SettlementView::ScriptVerbs(VerbFrame& _frame, std::span<const InterpolatedFigure> _figures, std::uint8_t _team)
{
  // ADR-125: a render check (ADR-117) draws what the verbs draw. It promotes a Worker, orders the new Leader towards the
  // region's centre, then holds a selection circle over a Worker with the ground point beyond it, so that the order
  // trail, the flag pointer, the route preview and the badge reach the GPU on the WARP adapter; with ADR-127, the
  // ordered route, the selection discs and the badge's count too.
  _frame.controls = {};
  _frame.leaderUnderPointer.reset();
  const InterpolatedFigure* worker = nullptr;
  const InterpolatedFigure* leader = nullptr;
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.team != _team || figure.state.dead)
    {
      continue;
    }
    if (worker == nullptr && figure.state.kind == FigureKind::Worker &&
        figure.state.task != static_cast<std::uint8_t>(WorkerState::Burning))
    {
      worker = &figure;
    }
    if (leader == nullptr && figure.state.kind == FigureKind::Leader && figure.state.selected)
    {
      leader = &figure;
    }
  }
  const Neuron::Vec3 center{(m_region.minX + m_region.maxX) * 0.5, 0.0, (m_region.minZ + m_region.maxZ) * 0.5};
  const auto beyond = [&center](const Neuron::Vec3& _from)
  { return _from + Neuron::NormalizedOr({center.x - _from.x, 0.0, center.z - _from.z}, {1.0, 0.0, 0.0}) * SCRIPT_REACH; };
  const std::uint32_t step = m_scriptStep;
  if (m_scriptStep == 0 && worker != nullptr)
  {
    _frame.ground = worker->position; // a promotion
    _frame.controls.secondaryDown = true;
    m_scriptStep = 1;
  }
  else if (m_scriptStep == 1 && leader != nullptr)
  {
    _frame.ground = beyond(leader->position); // a waypoint order, on the primary button (O82)
    _frame.controls.primaryDown = true;
    m_scriptStep = 2;
  }
  else if (m_scriptStep == 2 && worker != nullptr && !m_verbs.LeaderSelected())
  {
    _frame.ground = worker->position; // the circle begins
    _frame.controls.primaryDown = true;
    m_scriptStep = 3;
  }
  else if (m_scriptStep >= 3 && worker != nullptr)
  {
    _frame.ground = beyond(worker->position); // held, the pointer's ground point beyond the Workers
    _frame.controls.primaryHeld = true;
  }
  if (m_scriptStep != step)
  {
    Neuron::Log(Neuron::LogCategory::Diagnostics, Neuron::LogLevel::Info, "render check: the scripted verbs reach step {}", m_scriptStep);
  }
}
#endif

void SettlementView::EnterAt(const Neuron::Vec3& _ground)
{
  if (m_terrain != nullptr && m_camera.InWorld())
  {
    m_camera.EnterWorld(m_terrain->field, _ground, m_region);
  }
}

Neuron::CursorQuad SettlementView::CursorDraw() const noexcept
{
  return CursorAt(m_cursor, m_pointer, *m_desc.presentation);
}

} // namespace Species
