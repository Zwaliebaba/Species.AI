#include "pch.h"

#include "Picking.h"
#include "RegionTerrain.h"
#include "SettlementCamera.h"
#include "SettlementView.h"
#include "StewardAreas.h"
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <variant>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr Neuron::SizePx VIEW{1'920, 1'080};
constexpr Neuron::PointPx CENTER{960, 540};

const Species::RegionTerrain& SeedOne()
{
  static const std::shared_ptr<const Species::RegionTerrain> TERRAIN = []
  {
    auto terrain = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(terrain.has_value(), L"seed 1 gives a region");
    return std::make_shared<const Species::RegionTerrain>(std::move(*terrain));
  }();
  return *TERRAIN;
}

Species::CameraBounds RegionSquare()
{
  const Species::RegionLayout& layout = SeedOne().layout;
  return {layout.originX, layout.originX + layout.side, layout.originZ, layout.originZ + layout.side};
}

// The camera entered over the steward's site, and its first advance, which centres the pointer.
std::unique_ptr<Species::SettlementCamera> EnteredCamera(Neuron::PointPx& _pointer)
{
  auto camera = std::make_unique<Species::SettlementCamera>(CommittedPresentation(), Neuron::ProjectionPlanes{});
  camera->EnterWorld(SeedOne().field, SeedOne().layout.sites[0], RegionSquare());
  const Species::SettlementCamera::PointerRequest request = camera->Advance(0.0, {}, {0, 0}, VIEW, SeedOne().field);
  Assert::IsTrue(request.warpTo.has_value(), L"the entry centres the pointer (R-CTRL-470)");
  _pointer = request.warpTo.value_or(CENTER);
  return camera;
}

// High on the screen: on the entry pose, a ground point far beyond the steward's site.
constexpr Neuron::PointPx FAR_POINTER{960, 380};

// Advances _frames frames of _dt with the pointer kept where the cursor lock puts it.
void Run(Species::SettlementCamera& _camera, const Species::CameraControls& _controls, Neuron::PointPx& _pointer, double _dt, int _frames,
         const Species::StewardAreas* _areas = nullptr)
{
  for (int n = 0; n < _frames; ++n)
  {
    if (const auto warp = _camera.Advance(_dt, _controls, _pointer, VIEW, SeedOne().field, _areas).warpTo)
    {
      _pointer = *warp;
    }
  }
}

// Advances _frames frames of _dt with the pointer held where it is, the cursor lock's warps not followed, so that over
// a short run the front hardly turns.
void RunHeld(Species::SettlementCamera& _camera, const Species::CameraControls& _controls, Neuron::PointPx _pointer, double _dt,
             int _frames)
{
  for (int n = 0; n < _frames; ++n)
  {
    static_cast<void>(_camera.Advance(_dt, _controls, _pointer, VIEW, SeedOne().field));
  }
}

double Land(const Neuron::Vec3& _at)
{
  return std::max(SeedOne().field.Height(_at.x, _at.z), CommittedPresentation().ref.ctrl.camera.minHeight);
}

// A replica in which the settlement of site 0, team 0, holds the territory cells whose centres lie within _radius of
// its site, in chunks as ADR-139 sends them.
Species::StateReplica::View TerritoryDisc(double _radius)
{
  const Species::RegionLayout& layout = SeedOne().layout;
  const double cell = CommittedTuning().mvp.territory.cellSize;
  Species::StateReplica::View view;
  Species::ReplicatedSettlement settlement;
  view.emplace(Species::SettlementObjectId(0), settlement);
  const auto cells = static_cast<std::int32_t>(std::ceil(layout.side / cell));
  for (std::int32_t z = 0; z < cells; ++z)
  {
    for (std::int32_t x = 0; x < cells; ++x)
    {
      const Neuron::Vec3 center{layout.originX + (x + 0.5) * cell, 0.0, layout.originZ + (z + 0.5) * cell};
      if (Neuron::HorizontalDistance(center, layout.sites[0]) > _radius)
      {
        continue;
      }
      const auto chunkX = static_cast<std::uint8_t>(x / Species::CELL_CHUNK_SIDE);
      const auto chunkZ = static_cast<std::uint8_t>(z / Species::CELL_CHUNK_SIDE);
      const auto entry = view.try_emplace(Species::CellChunkObjectId(Species::CellLayer::Territory, 0, chunkX, chunkZ),
                                          Species::ReplicatedCells{Species::CellLayer::Territory, 0, chunkX, chunkZ, 0});
      std::get<Species::ReplicatedCells>(entry.first->second).cells |=
        std::uint64_t{1} << ((z % Species::CELL_CHUNK_SIDE) * Species::CELL_CHUNK_SIDE + x % Species::CELL_CHUNK_SIDE);
    }
  }
  return view;
}

} // namespace

TEST_CLASS(SettlementCameraTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Cam060TheCameraEntersTheWorldLookingAtTheStewardsSite)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"CAM-060")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-470")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Cam060TheCameraEntersTheWorldLookingAtTheStewardsSite)
  {
    // O76: at the fallback height along the fallback direction, the screen's centre on the site's centre.
    Neuron::PointPx pointer;
    const auto camera = EnteredCamera(pointer);
    Assert::IsTrue(pointer.x == CENTER.x && pointer.y == CENTER.y, L"the pointer at the screen centre");
    Assert::AreEqual(1'000.0, camera->Position().y, 1e-9, L"ref.sim.match_entry.fallback_camera_height");
    Assert::AreEqual(60.0, camera->FieldOfViewDegrees(), 1e-12, L"ref.ctrl.camera.fov_base");
    const Neuron::Vec3 direction = Neuron::Normalized({-1.0, -0.7, -1.0});
    Assert::AreEqual(1.0, Neuron::Dot(camera->Front(), direction), 1e-12, L"along the fallback direction");
    const auto& entered = camera->PointerGround();
    Assert::IsTrue(entered.has_value() && entered->onTerrain, L"the pointer's ground point is on the terrain");
    const Species::SettlementCamera::GroundPoint ground = entered.value_or(Species::SettlementCamera::GroundPoint{});
    const Neuron::Vec3& site = SeedOne().layout.sites[0];
    Assert::IsTrue(Neuron::HorizontalDistance(ground.position, site) < 10.0, L"within a few pixels of the site's centre");
    // The bounds are the region's square ± ref.sim.match_entry.camera_border.
    Assert::AreEqual(SeedOne().layout.originX - 200.0, camera->Bounds().minX, 1e-9);
    Assert::AreEqual(SeedOne().layout.originZ + SeedOne().layout.side + 200.0, camera->Bounds().maxZ, 1e-9);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(AcCtrlCa01TheKeysPanAtTheirSpeed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(AcCtrlCa01TheKeysPanAtTheirSpeed)
  {
    // One second of the forward key moves the target 250 wu, and with the fast pan 1,000 wu, along the view (O90), and
    // one second of the side key as far along the ground. The camera is raised first, so that no cliff shortens the step,
    // and the pointer is held at the screen's centre, so that the front does not turn.
    for (const bool fast : {false, true})
    {
      for (const bool side : {false, true})
      {
        Neuron::PointPx pointer;
        auto camera = EnteredCamera(pointer);
        Species::CameraControls raise;
        raise.wheelNotches = 40.0f;
        RunHeld(*camera, raise, pointer, 0.01, 1);
        const Neuron::Vec3 before = camera->Target();
        Species::CameraControls keys;
        keys.panForward = !side;
        keys.panLeft = side;
        keys.fastPan = fast;
        RunHeld(*camera, keys, pointer, 0.01, 100);
        const double moved = Neuron::Length(camera->Target() - before);
        Assert::AreEqual(fast ? 1'000.0 : 250.0, moved, fast ? 4.0 : 1.0, L"the path, nearly straight");
        if (side)
        {
          Assert::AreEqual(before.y, camera->Target().y, 1e-9, L"the side keys keep the height");
        }
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O90TheForwardKeyFliesAlongTheView)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O90TheForwardKeyFliesAlongTheView)
  {
    // From the entry pose, which looks down along the fallback direction, one second of the forward key lowers the
    // camera by about 250 wu × the front's downward share and moves it the rest along the ground; the front turns a
    // little towards the ground point as the camera closes on it (R-CTRL-471 step 8).
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::Vec3 front = camera->Front();
    const double height = camera->Height();
    const Neuron::Vec3 before = camera->Target();
    Species::CameraControls forward;
    forward.panForward = true;
    Run(*camera, forward, pointer, 0.01, 100);
    Assert::AreEqual(height + front.y * 250.0, camera->Height(), 2.0, L"down along the view");
    Assert::AreEqual(std::hypot(front.x, front.z) * 250.0, Neuron::HorizontalDistance(camera->Target(), before), 2.0,
                     L"and forward along the ground");
    Species::CameraControls back;
    back.panBack = true;
    Run(*camera, back, pointer, 0.01, 100);
    Assert::AreEqual(height, camera->Height(), 2.0, L"back up along it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O90TheForwardKeyFliesToThePointUnderThePointer)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-464")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O90TheForwardKeyFliesToThePointUnderThePointer)
  {
    // The cursor lock keeps the pointer on its ground point and the camera turns to face it, so the forward key held
    // brings the camera down onto that point: it comes to rest above it at the ground clearance, looking almost straight
    // down (ADR-142). The back key held climbs away from it to the highest height.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const auto& entered = camera->PointerGround();
    Assert::IsTrue(entered.has_value() && entered->onTerrain);
    const Neuron::Vec3 ground = entered.value_or(Species::SettlementCamera::GroundPoint{}).position;
    Species::CameraControls forward;
    forward.panForward = true;
    forward.fastPan = true;
    Run(*camera, forward, pointer, 0.01, 1'000);
    const double clearance = CommittedPresentation().ref.ctrl.camera.groundClearance;
    Assert::IsTrue(Neuron::HorizontalDistance(camera->Position(), ground) < 50.0, L"above the point");
    Assert::AreEqual(Land(camera->Target()) + clearance, camera->Height(), 1.0, L"at the ground clearance");
    Assert::IsTrue(camera->Front().y < -0.99, L"looking down");
    Species::CameraControls climb;
    climb.panBack = true;
    climb.fastPan = true;
    Run(*camera, climb, pointer, 0.25, 200);
    Assert::AreEqual(CommittedPresentation().ref.ctrl.camera.maxHeight, camera->Height(), 1e-9, L"at most 5,000 wu");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(AcCtrlCa02ZoomConvergesOnAQuarterOfTheField)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-460")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(AcCtrlCa02ZoomConvergesOnAQuarterOfTheField)
  {
    // At 100 frames per second each frame removes 10 % of the difference, so after 0.1 s 0.9¹⁰ of it remains.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    Species::CameraControls zoom;
    zoom.zoom = true;
    Run(*camera, zoom, pointer, 0.01, 10);
    Assert::AreEqual(std::pow(0.9, 10.0), (camera->FieldOfViewDegrees() - 15.0) / 45.0, 1e-9);
    Run(*camera, {}, pointer, 0.01, 600);
    Assert::AreEqual(60.0, camera->FieldOfViewDegrees(), 1e-3, L"back to the base field once released");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(AcCtrlCa03AWheelNotchRaisesByTheRootOfTheAltitude)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-461")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(AcCtrlCa03AWheelNotchRaisesByTheRootOfTheAltitude)
  {
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const double altitude = camera->Height() - Land(camera->Position());
    Species::CameraControls wheel;
    wheel.wheelNotches = 1.0f;
    const double before = camera->Height();
    Run(*camera, wheel, pointer, 0.01, 1);
    Assert::AreEqual(2.0 * std::sqrt(altitude), camera->Height() - before, 1e-9, L"2 × √altitude per notch");
    // The height never passes ref.ctrl.camera.max_height, and never falls below the land + the ground clearance.
    Species::CameraControls raise;
    raise.raise = true;
    Run(*camera, raise, pointer, 0.25, 400);
    Assert::AreEqual(5'000.0, camera->Height(), 1e-9, L"at most 5,000 wu");
    Species::CameraControls lower;
    lower.lower = true;
    Run(*camera, lower, pointer, 0.25, 2'000);
    Assert::IsTrue(camera->Height() >= Land(camera->Position()) + 10.0 - 1e-9, L"at least the clearance above the land");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(AcCtrlCa04ThePanStopsAtTheRegionsBorder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(AcCtrlCa04ThePanStopsAtTheRegionsBorder)
  {
    // With the pointer high on the screen, on a point beyond the region's low corner, flying for 60 s towards it leaves
    // the camera on the bound on at least one axis (ADR-119); a sandbox has no territory, so the region alone bounds it.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    pointer = FAR_POINTER;
    camera->UpdatePointerGround(pointer, VIEW, SeedOne().field);
    Species::CameraControls forward;
    forward.panForward = true;
    forward.fastPan = true;
    Species::StewardAreas areas;
    Run(*camera, forward, pointer, 0.25, 240, &areas);
    const Species::CameraBounds& bounds = camera->Bounds();
    Assert::IsTrue(camera->Position().x >= bounds.minX - 1e-9 && camera->Position().z >= bounds.minZ - 1e-9, L"never beyond it");
    Assert::IsTrue(std::abs(camera->Position().x - bounds.minX) < 1e-6 || std::abs(camera->Position().z - bounds.minZ) < 1e-6,
                   L"on the bound");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244TheCameraStaysInTheStewardsAreas)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244TheCameraStaysInTheStewardsAreas)
  {
    // With the steward's territory a disc of 300 wu about its site, its areas reach 1,100 wu from the site. Flying for
    // 60 s towards a point beyond them, the camera's ground point never leaves them, and comes to rest at their edge
    // (ADR-142).
    constexpr double RADIUS = 300.0;
    const double reach = RADIUS + CommittedTuning().wld.view.settlementMargin;
    const double cell = CommittedTuning().mvp.territory.cellSize;
    Species::StewardAreas areas;
    areas.Update(TerritoryDisc(RADIUS), std::uint8_t{0}, SeedOne().layout, CommittedTuning());
    Assert::IsTrue(areas.Known());
    const Neuron::Vec3& site = SeedOne().layout.sites[0];
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    pointer = FAR_POINTER;
    camera->UpdatePointerGround(pointer, VIEW, SeedOne().field);
    Species::CameraControls forward;
    forward.panForward = true;
    forward.fastPan = true;
    double farthest = 0.0;
    for (int n = 0; n < 240; ++n)
    {
      Run(*camera, forward, pointer, 0.25, 1, &areas);
      Assert::IsTrue(areas.Contains(camera->Target().x, camera->Target().z) && areas.Contains(camera->Position().x, camera->Position().z),
                     L"inside the areas every frame");
      farthest = std::max(farthest, Neuron::HorizontalDistance(camera->Target(), site));
    }
    Assert::IsTrue(farthest <= reach + cell * std::numbers::sqrt2, L"never beyond the margin's last cell");
    Assert::IsTrue(Neuron::HorizontalDistance(camera->Target(), site) > reach - 3.0 * cell, L"at rest on the areas' edge");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(AcCtrlCa05ThePointerStaysOnItsGroundPointWhileTheCameraMoves)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-464")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-058")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(AcCtrlCa05ThePointerStaysOnItsGroundPointWhileTheCameraMoves)
  {
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    // The pointer off the centre, then a pan with the mouse still.
    pointer = {700, 650};
    camera->UpdatePointerGround(pointer, VIEW, SeedOne().field);
    const auto& before = camera->PointerGround();
    Assert::IsTrue(before.has_value() && before->onTerrain);
    const auto start = before.value_or(Species::SettlementCamera::GroundPoint{});
    Species::CameraControls right;
    right.panRight = true;
    Run(*camera, right, pointer, 0.016, 30);
    Assert::IsTrue(camera->PointerGround().has_value(), L"still a ground point");
    const auto end = camera->PointerGround().value_or(Species::SettlementCamera::GroundPoint{});
    // One pixel's footprint at the point's distance: the distance × the field's height ÷ the view's rows.
    const double distance = Neuron::Length(end.position - camera->Position());
    const double footprint = distance * 2.0 * std::tan(std::numbers::pi / 6.0) / static_cast<double>(VIEW.height);
    const double drift = Neuron::HorizontalDistance(end.position, start.position);
    Assert::IsTrue(drift < 3.0 * footprint / std::max(0.2, -camera->Front().y), L"the ground point holds within a few pixels");
    Assert::IsTrue(pointer.x != 700 || pointer.y != 650, L"the cursor lock moved the pointer");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl471TheCameraTurnsTowardsThePointUnderThePointer)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl471TheCameraTurnsTowardsThePointUnderThePointer)
  {
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::Vec3 front = camera->Front();
    pointer = {1'500, 540}; // to the right of the centre
    camera->UpdatePointerGround(pointer, VIEW, SeedOne().field);
    Run(*camera, {}, pointer, 0.016, 5);
    const Neuron::Vec3 right = Neuron::Normalized({-front.z, 0.0, front.x}); // world up × front, negated
    Assert::IsTrue(Neuron::Dot(camera->Front() - front, right) > 0.0, L"the front turned to the right");
    // Looking around freezes the position while the front still follows the pointer.
    const Neuron::Vec3 position = camera->Position();
    Species::CameraControls look;
    look.lookAround = true;
    look.panForward = true;
    Run(*camera, look, pointer, 0.016, 5);
    Assert::AreEqual(0.0, Neuron::Length(camera->Position() - position), 1e-9, L"frozen");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl055TheHoverIsTheOwnLeaderNearestThePointerOnScreen)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-055")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-192")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl055TheHoverIsTheOwnLeaderNearestThePointerOnScreen)
  {
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::FrameView frame = camera->View(VIEW);
    const auto& ground = camera->PointerGround();
    Assert::IsTrue(ground.has_value());
    const Neuron::Vec3 at = ground.value_or(Species::SettlementCamera::GroundPoint{}).position;
    // Two own Leaders and a Worker under the pointer, an enemy Leader on it, and a dead own Leader on it.
    std::vector<Species::InterpolatedFigure> figures(5);
    const auto place =
      [&](std::size_t _n, Species::StableId _id, std::uint8_t _team, Species::FigureKind _kind, bool _dead, const Neuron::Vec3& _offset)
    {
      figures[_n].id = _id;
      figures[_n].state.team = _team;
      figures[_n].state.kind = _kind;
      figures[_n].state.dead = _dead;
      figures[_n].position = at + _offset;
    };
    place(0, 10, 1, Species::FigureKind::Leader, false, {});
    place(1, 11, 0, Species::FigureKind::Leader, true, {});
    place(2, 12, 0, Species::FigureKind::Worker, false, {});
    place(3, 13, 0, Species::FigureKind::Leader, false, {6.0, 0.0, 0.0});
    place(4, 14, 0, Species::FigureKind::Leader, false, {0.0, 0.0, 0.0});
    const Neuron::Ray ray = Neuron::PickRayAt(pointer.x, pointer.y, frame, VIEW, 1.0);
    const auto hovered = Species::LeaderUnderRay(figures, 0, ray, frame, VIEW, CommittedPresentation());
    Assert::IsTrue(hovered == Species::StableId{14}, L"the own living Leader centred under the pointer");
    Assert::IsFalse(Species::LeaderUnderRay(figures, 2, ray, frame, VIEW, CommittedPresentation()).has_value(), L"no Leader of team 2");
    // Far off the pointer, no sphere is met.
    const Neuron::Ray elsewhere = Neuron::PickRayAt(10.0, 10.0, frame, VIEW, 1.0);
    Assert::IsFalse(Species::LeaderUnderRay(figures, 0, elsewhere, frame, VIEW, CommittedPresentation()).has_value());
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl352ACursorSitsOnItsHotspot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-352")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-367")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl352ACursorSitsOnItsHotspot)
  {
    // O76: 48 px for the pointer and the highlight, 60 px for the unavailable cursor, in physical pixels.
    const Neuron::CursorQuad world = Species::CursorAt(Species::CursorKind::World, {500, 300}, CommittedPresentation());
    Assert::AreEqual(48.0f, world.sidePixels);
    Assert::AreEqual(500.0f - 48.0f * 0.113f, world.left, 1e-3f, L"the arrow's tip on the pointer");
    Assert::AreEqual(300.0f - 48.0f * 0.082f, world.top, 1e-3f);
    Assert::IsTrue(world.image == 0 && world.color[0] == 1.0f && world.color[1] == 1.0f, L"white");
    const Neuron::CursorQuad highlight = Species::CursorAt(Species::CursorKind::Highlight, {500, 300}, CommittedPresentation());
    Assert::IsTrue(highlight.image == 1 && highlight.left == 476.0f && highlight.top == 276.0f, L"centred");
    const Neuron::CursorQuad unavailable = Species::CursorAt(Species::CursorKind::Unavailable, {500, 300}, CommittedPresentation());
    Assert::IsTrue(unavailable.image == 2 && unavailable.sidePixels == 60.0f && unavailable.left == 470.0f, L"60 px, centred");
    Assert::IsTrue(unavailable.color[0] == 1.0f && unavailable.color[1] == 0.0f && unavailable.color[2] == 0.0f, L"red");
  }
};

} // namespace GameClientTests
