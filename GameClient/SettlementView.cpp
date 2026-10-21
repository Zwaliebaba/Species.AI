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
                                       const Neuron::FrameView& _frame, Neuron::SizePx _view, const TuningPresentation& _presentation)
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
    const double scale =
      1.0 + WORKER_DISTANCE_SCALE_FACTOR * std::max(0.0, distance - unit.worker.cardScaleStart) / unit.worker.cardScaleDivisor;
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
    m_verbs(*_desc.presentation)
{
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
    m_verbs.Reset();
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
      if (const std::optional<Neuron::PointPx> warp = m_camera.Advance(_dtSeconds, controls, m_pointer, _view, terrain->field).warpTo)
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
  const std::optional<std::uint8_t> ownTeam = _session.CommandedTeam();
  std::vector<InterpolatedFigure> figures;
  if (m_camera.InWorld() && ownTeam.has_value())
  {
    const Neuron::FrameView frame = m_camera.View(_view);
    const Neuron::Ray ray = Neuron::PickRayAt(static_cast<double>(m_pointer.x), static_cast<double>(m_pointer.y), frame, _view,
                                              m_desc.presentation->cam.pick.rowOffset);
    figures = InterpolateFigures(_session.Replica(), _session.InterpolationFraction());
    m_hovered = LeaderUnderRay(figures, *ownTeam, ray, frame, _view, *m_desc.presentation);
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
  verbs.leaderUnderPointer = m_hovered;
  if (onRegion)
  {
    verbs.ground = ground->position;
  }
  if (advancing)
  {
    VerbControls& controls = verbs.controls;
    controls.primaryDown = _router.Query(ActionOf(GameAction::SelectPick)) || _router.Query(ActionOf(GameAction::SelectPickIdle)) ||
                           _router.Query(ActionOf(GameAction::SelectCircleBegin));
    controls.primaryHeld = _router.Query(ActionOf(GameAction::SelectCircleGrow));
    controls.primaryUp = _router.Query(ActionOf(GameAction::SelectCircleEnd));
    controls.clear = _router.Query(ActionOf(GameAction::SelectClear));
    controls.secondaryDown = _router.Query(ActionOf(GameAction::OrderCrowd)) || _router.Query(ActionOf(GameAction::OrderPromote)) ||
                             _router.Query(ActionOf(GameAction::OrderSpecial));
    controls.straight = _router.Query(ActionOf(GameAction::OrderStraightRoute));
    controls.moveOnly = _router.Query(ActionOf(GameAction::OrderMoveModifier));
    controls.demote = _router.Query(ActionOf(GameAction::OrderDemote));
  }
  const std::optional<std::uint8_t> verbTeam = m_camera.InWorld() ? ownTeam : std::nullopt;
  for (const BattleCommand& command : m_verbs.Update(verbs, verbTeam, _session.Replica(), figures))
  {
    const std::optional<std::uint64_t> sequence =
      std::visit([&_session](const auto& _command) { return _session.SendCommand(_command); }, command);
    if (sequence.has_value())
    {
      m_verbs.Sent(*sequence, command);
    }
  }
  // O78: the highlight also marks the Worker a secondary press would promote.
  if (m_hovered.has_value() || m_verbs.PromotableWorker().has_value())
  {
    m_cursor = CursorKind::Highlight;
  }
  else if (m_camera.InWorld() && !onRegion)
  {
    m_cursor = CursorKind::Unavailable;
  }
  else
  {
    m_cursor = CursorKind::World;
  }
}

Neuron::CursorQuad SettlementView::CursorDraw() const noexcept
{
  return CursorAt(m_cursor, m_pointer, *m_desc.presentation);
}

} // namespace Species
