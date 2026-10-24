#include "pch.h"

#include "Picking.h"
#include "RegionTerrain.h"
#include "RelationMessages.h"
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
constexpr double DEGREES = std::numbers::pi / 180.0;

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

// Two seconds with no control, at 100 frames a second: the camera closes on its goals.
void Settle(Species::SettlementCamera& _camera, Neuron::PointPx& _pointer)
{
  Run(_camera, {}, _pointer, 0.01, 200);
}

double Land(const Neuron::Vec3& _at)
{
  return std::max(SeedOne().field.Height(_at.x, _at.z), CommittedPresentation().ref.ctrl.camera.minHeight);
}

// The camera's distance from its ground point, from its height and the tilt it gives (ADR-180).
double DistanceOf(const Species::SettlementCamera& _camera)
{
  return _camera.Height() / std::sin(_camera.PitchRadiansAt(_camera.Height()));
}

// The level front of the camera's heading, and its left.
Neuron::Vec3 ForwardOf(const Species::SettlementCamera& _camera)
{
  return {-std::sin(_camera.HeadingRadians()), 0.0, -std::cos(_camera.HeadingRadians())};
}

Neuron::Vec3 LeftOf(const Species::SettlementCamera& _camera)
{
  const Neuron::Vec3 forward = ForwardOf(_camera);
  return {forward.z, 0.0, -forward.x};
}

// The ground point under _pointer as the camera stands.
Neuron::Vec3 GroundUnder(Species::SettlementCamera& _camera, Neuron::PointPx _pointer)
{
  _camera.UpdatePointerGround(_pointer, VIEW, SeedOne().field);
  const auto& ground = _camera.PointerGround();
  Assert::IsTrue(ground.has_value() && ground->onTerrain, L"the pointer's ground point is on the terrain");
  return ground.value_or(Species::SettlementCamera::GroundPoint{}).position;
}

// One pixel's footprint at a point: its distance × the field's height ÷ the view's rows.
double FootprintAt(const Species::SettlementCamera& _camera, const Neuron::Vec3& _at)
{
  return Neuron::Length(_at - _camera.Position()) * 2.0 * std::tan(_camera.FieldOfViewDegrees() * DEGREES / 2.0) /
         static_cast<double>(VIEW.height);
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
  BEGIN_TEST_METHOD_ATTRIBUTE(Cam060TheCameraEntersTheWorldOverTheStewardsSite)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"CAM-060")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-470")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Cam060TheCameraEntersTheWorldOverTheStewardsSite)
  {
    // O76 as O108 amends it (ADR-180): held over the site's center at the fallback height, with the fallback direction's
    // heading and the tilt that height gives; the screen's center on the site.
    Neuron::PointPx pointer;
    const auto camera = EnteredCamera(pointer);
    Assert::IsTrue(pointer.x == CENTER.x && pointer.y == CENTER.y, L"the pointer at the screen centre");
    const Neuron::Vec3& site = SeedOne().layout.sites[0];
    Assert::AreEqual(site.x, camera->Focus().x, 1e-9, L"over the site");
    Assert::AreEqual(site.z, camera->Focus().z, 1e-9);
    Assert::AreEqual(1'000.0, camera->Height(), 1e-9, L"ref.sim.match_entry.fallback_camera_height above the ground");
    Assert::AreEqual(45.0 * DEGREES, camera->HeadingRadians(), 1e-12, L"the heading of (−1, −0.7, −1)");
    Assert::AreEqual(camera->PitchRadiansAt(1'000.0), std::asin(-camera->Front().y), 1e-9, L"the tilt of the height");
    Assert::AreEqual(60.0, camera->FieldOfViewDegrees(), 1e-12, L"ref.ctrl.camera.fov_base");
    const Neuron::Vec3 ground = GroundUnder(*camera, CENTER);
    Assert::IsTrue(Neuron::HorizontalDistance(ground, site) < 5.0 * FootprintAt(*camera, ground), L"the screen's center on the site");
    // The bounds are the region's square ± ref.sim.match_entry.camera_border.
    Assert::AreEqual(SeedOne().layout.originX - 200.0, camera->Bounds().minX, 1e-9);
    Assert::AreEqual(SeedOne().layout.originZ + SeedOne().layout.side + 200.0, camera->Bounds().maxZ, 1e-9);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108TheTiltFollowsTheHeight)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108TheTiltFollowsTheHeight)
  {
    // cam.orbit.pitch_low (30°) at cam.orbit.height_min (40 wu) to cam.orbit.pitch_high (65°) at 5,000 wu, by the share of
    // the height's logarithm: halfway at the heights' geometric mean, and held beyond the ends.
    const Species::SettlementCamera camera(CommittedPresentation(), Neuron::ProjectionPlanes{});
    Assert::AreEqual(30.0, camera.PitchRadiansAt(40.0) / DEGREES, 1e-9, L"the lowest height");
    Assert::AreEqual(65.0, camera.PitchRadiansAt(5'000.0) / DEGREES, 1e-9, L"the highest height");
    Assert::AreEqual(47.5, camera.PitchRadiansAt(std::sqrt(40.0 * 5'000.0)) / DEGREES, 1e-9, L"halfway");
    Assert::AreEqual(30.0, camera.PitchRadiansAt(10.0) / DEGREES, 1e-9, L"held below");
    Assert::AreEqual(65.0, camera.PitchRadiansAt(9'000.0) / DEGREES, 1e-9, L"held above");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108TheKeysPanByAShareOfTheDistance)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108TheKeysPanByAShareOfTheDistance)
  {
    // One second of the forward or the side key moves the ground point by cam.orbit.pan_rate (0.5) × the camera's
    // distance from it, × cam.orbit.fast_pan_factor (3) with the fast pan, level along the heading or across it; the
    // height is kept and the ground point stays on the ground.
    for (const bool fast : {false, true})
    {
      for (const bool side : {false, true})
      {
        Neuron::PointPx pointer;
        auto camera = EnteredCamera(pointer);
        const Neuron::Vec3 before = camera->Focus();
        const double distance = DistanceOf(*camera);
        Species::CameraControls keys;
        keys.panForward = !side;
        keys.panLeft = side;
        keys.fastPan = fast;
        Run(*camera, keys, pointer, 0.01, 100);
        Settle(*camera, pointer);
        const double moved = Neuron::HorizontalDistance(camera->Focus(), before);
        Assert::AreEqual(0.5 * distance * (fast ? 3.0 : 1.0), moved, 0.5, L"a share of the distance a second");
        Assert::AreEqual(1'000.0, camera->Height(), 1e-9, L"the height kept");
        const Neuron::Vec3 direction{(camera->Focus().x - before.x) / moved, 0.0, (camera->Focus().z - before.z) / moved};
        Assert::AreEqual(1.0, Neuron::Dot(direction, side ? LeftOf(*camera) : ForwardOf(*camera)), 1e-6, L"along the heading or across it");
        Assert::AreEqual(Land(camera->Focus()), camera->Focus().y, 1.0, L"on the ground");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108TheWheelZoomsTowardsThePointUnderThePointer)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-461")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108TheWheelZoomsTowardsThePointUnderThePointer)
  {
    // A notch forward multiplies the height by cam.orbit.zoom_step (0.85) and a notch back divides it, between
    // cam.orbit.height_min and ref.ctrl.camera.max_height, and the ground under a still pointer stays under it.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    pointer = {600, 700};
    const Neuron::Vec3 ground = GroundUnder(*camera, pointer);
    Species::CameraControls wheel;
    wheel.wheelNotches = 1.0f;
    Run(*camera, wheel, pointer, 0.01, 1);
    Settle(*camera, pointer);
    Assert::AreEqual(850.0, camera->Height(), 1e-3, L"one notch forward");
    Neuron::Vec3 under = GroundUnder(*camera, pointer);
    Assert::IsTrue(Neuron::HorizontalDistance(ground, under) < 2.0 * FootprintAt(*camera, under), L"the ground under the pointer");
    wheel.wheelNotches = -2.0f;
    Run(*camera, wheel, pointer, 0.01, 1);
    Settle(*camera, pointer);
    Assert::AreEqual(850.0 / 0.85 / 0.85, camera->Height(), 1e-3, L"two notches back");
    under = GroundUnder(*camera, pointer);
    Assert::IsTrue(Neuron::HorizontalDistance(ground, under) < 3.0 * FootprintAt(*camera, under), L"and out again");
    wheel.wheelNotches = 60.0f;
    Run(*camera, wheel, pointer, 0.01, 1);
    Run(*camera, {}, pointer, 0.01, 400);
    Assert::AreEqual(40.0, camera->Height(), 1e-3, L"at least cam.orbit.height_min");
    // Low over the ground, a pan keeps the camera above the land under it by the ground clearance (R-CTRL-471 step 6).
    Species::CameraControls forward;
    forward.panForward = true;
    for (int n = 0; n < 100; ++n)
    {
      Run(*camera, forward, pointer, 0.01, 1);
      Assert::IsTrue(camera->Position().y >= Land(camera->Position()) + 10.0 - 1e-6, L"above the land");
    }
    wheel.wheelNotches = -80.0f;
    Run(*camera, wheel, pointer, 0.01, 1);
    Run(*camera, {}, pointer, 0.01, 400);
    Assert::AreEqual(5'000.0, camera->Height(), 1e-3, L"at most 5,000 wu");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108TheZoomKeysZoomAboutTheCenter)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-461")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108TheZoomKeysZoomAboutTheCenter)
  {
    // One second of Page Down is cam.orbit.zoom_key_notches (4) notches in, and the ground point does not move.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::Vec3 focus = camera->Focus();
    Species::CameraControls lower;
    lower.lower = true;
    Run(*camera, lower, pointer, 0.01, 100);
    Settle(*camera, pointer);
    Assert::AreEqual(1'000.0 * std::pow(0.85, 4.0), camera->Height(), 0.5, L"four notches");
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), focus), 1e-9, L"about the screen's center");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108QAndETurnAboutTheGroundPoint)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108QAndETurnAboutTheGroundPoint)
  {
    // One second of Q turns the heading cam.orbit.turn_rate (90°) to the left about the ground point, which stays under
    // the screen's center; Backspace restores the entry heading, and three seconds of E turn 270° to the right.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::Vec3 focus = camera->Focus();
    const double radius = Neuron::HorizontalDistance(camera->Position(), focus);
    Species::CameraControls left;
    left.turnLeft = true;
    Run(*camera, left, pointer, 0.01, 100);
    Settle(*camera, pointer);
    Assert::AreEqual(135.0 * DEGREES, camera->HeadingRadians(), 1e-3, L"90° to the left");
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), focus), 1e-9, L"about the ground point");
    Assert::AreEqual(radius, Neuron::HorizontalDistance(camera->Position(), focus), 1e-6, L"on its circle");
    const Neuron::Vec3 ground = GroundUnder(*camera, CENTER);
    Assert::IsTrue(Neuron::HorizontalDistance(ground, focus) < 5.0 * FootprintAt(*camera, ground), L"under the screen's center");
    Species::CameraControls reset;
    reset.resetHeading = true;
    Run(*camera, reset, pointer, 0.01, 1);
    Settle(*camera, pointer);
    Assert::AreEqual(45.0 * DEGREES, camera->HeadingRadians(), 1e-3, L"the entry heading");
    Species::CameraControls right;
    right.turnRight = true;
    Run(*camera, right, pointer, 0.01, 300);
    Settle(*camera, pointer);
    Assert::AreEqual(0.0, std::remainder(camera->HeadingRadians() - (45.0 - 270.0) * DEGREES, 2.0 * std::numbers::pi), 1e-3,
                     L"270° to the right");
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), focus), 1e-9, L"still about the ground point");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108TheMiddleButtonDraggedHoldsTheGround)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108TheMiddleButtonDraggedHoldsTheGround)
  {
    // The ground under the pointer when the middle button goes down stays under it as it moves, the pointer is never
    // moved, and the camera stays where the drag left it.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    pointer = {900, 600};
    const Neuron::Vec3 ground = GroundUnder(*camera, pointer);
    Species::CameraControls drag;
    drag.drag = true;
    static_cast<void>(camera->Advance(0.016, drag, pointer, VIEW, SeedOne().field));
    for (int n = 0; n < 50; ++n)
    {
      pointer.x += 4;
      pointer.y -= 2;
      Assert::IsFalse(camera->Advance(0.016, drag, pointer, VIEW, SeedOne().field).warpTo.has_value(), L"the pointer is free");
    }
    const Neuron::Vec3 under = GroundUnder(*camera, pointer);
    Assert::IsTrue(Neuron::HorizontalDistance(ground, under) < 2.0 * FootprintAt(*camera, under), L"the ground follows the pointer");
    Assert::IsTrue(Neuron::HorizontalDistance(camera->Focus(), SeedOne().layout.sites[0]) > 100.0, L"the camera moved");
    const Neuron::Vec3 held = camera->Focus();
    Run(*camera, {}, pointer, 0.01, 100);
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), held), 1e-6, L"no drift after the release");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108TheMiddleButtonDraggedWithAltTurns)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108TheMiddleButtonDraggedWithAltTurns)
  {
    // With Alt held both drag actions fire (INP-085) and the camera turns: the pointer 100 px to the right turns the
    // heading 100 × cam.orbit.drag_turn (0.3°) to the right at once, and holds no ground.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    Species::CameraControls turn;
    turn.turnDrag = true;
    turn.drag = true;
    Run(*camera, turn, pointer, 0.016, 1);
    const Neuron::Vec3 focus = camera->Focus();
    pointer.x += 100;
    Run(*camera, turn, pointer, 0.016, 1);
    Assert::AreEqual((45.0 - 30.0) * DEGREES, camera->HeadingRadians(), 1e-9, L"30° to the right");
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), focus), 1e-9, L"no ground held");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108TheHorizontalWheelPansAcross)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108TheHorizontalWheelPansAcross)
  {
    // A notch of the horizontal wheel to the right, or a touchpad's slide, moves the ground point to the right by
    // cam.orbit.wheel_pan_share (0.1) of the distance.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::Vec3 focus = camera->Focus();
    const double distance = DistanceOf(*camera);
    Species::CameraControls across;
    across.panWheelNotches = 1.0f;
    Run(*camera, across, pointer, 0.01, 1);
    Settle(*camera, pointer);
    Assert::AreEqual(-0.1 * distance, Neuron::Dot(camera->Focus() - focus, LeftOf(*camera)), 1e-6, L"to the right");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O108HomeFliesBackToTheSite)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O108HomeFliesBackToTheSite)
  {
    // After a pan, a turn and a zoom, Home brings the camera back to its entry pose over the steward's site.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    Species::CameraControls away;
    away.panForward = true;
    away.fastPan = true;
    away.turnLeft = true;
    Run(*camera, away, pointer, 0.01, 200);
    Species::CameraControls closer;
    closer.wheelNotches = 3.0f;
    Run(*camera, closer, pointer, 0.01, 1);
    Settle(*camera, pointer);
    Species::CameraControls home;
    home.home = true;
    Run(*camera, home, pointer, 0.01, 1);
    Run(*camera, {}, pointer, 0.01, 300);
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), SeedOne().layout.sites[0]), 1e-3, L"over the site");
    Assert::AreEqual(1'000.0, camera->Height(), 1e-3, L"at the entry height");
    Assert::AreEqual(45.0 * DEGREES, camera->HeadingRadians(), 1e-3, L"with the entry heading");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181OneFingerHoldsTheGroundUnderIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181OneFingerHoldsTheGroundUnderIt)
  {
    // The ground where the finger went down stays under it as it drags, the camera stays where the drag left it, and a
    // hold's drag, which the verbs take (ADR-182), moves nothing.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::TouchPoint start{900.0, 600.0};
    const Neuron::Vec3 ground = GroundUnder(*camera, {900, 600});
    Species::CameraControls finger;
    finger.touchDrag = Neuron::TouchDrag{Neuron::GesturePhase::Begin, start, {915.0, 600.0}, false};
    Run(*camera, finger, pointer, 0.016, 1);
    for (int n = 1; n <= 40; ++n)
    {
      finger.touchDrag = Neuron::TouchDrag{Neuron::GesturePhase::Move, start, {915.0 + 5.0 * n, 600.0 - 3.0 * n}, false};
      Run(*camera, finger, pointer, 0.016, 1);
    }
    const Neuron::Vec3 under = GroundUnder(*camera, {1'115, 480});
    Assert::IsTrue(Neuron::HorizontalDistance(ground, under) < 2.0 * FootprintAt(*camera, under), L"the ground follows the finger");
    finger.touchDrag = Neuron::TouchDrag{Neuron::GesturePhase::End, start, {1'115.0, 480.0}, false};
    Run(*camera, finger, pointer, 0.016, 1);
    const Neuron::Vec3 held = camera->Focus();
    Run(*camera, {}, pointer, 0.01, 100);
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), held), 1e-6, L"no drift after the lift");
    Species::CameraControls hold;
    hold.touchDrag = Neuron::TouchDrag{Neuron::GesturePhase::Begin, start, start, true};
    Run(*camera, hold, pointer, 0.016, 1);
    hold.touchDrag = Neuron::TouchDrag{Neuron::GesturePhase::Move, start, {1'200.0, 300.0}, true};
    Run(*camera, hold, pointer, 0.016, 10);
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), held), 1e-6, L"a hold's drag moves no camera");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181TwoFingersPanPinchAndTurn)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181TwoFingersPanPinchAndTurn)
  {
    // Over ten frames two fingers spread to 1.25 times their span, turn 20° clockwise and move their center 100 px to the
    // right: the height falls to 1,000 ÷ 1.25 at once, the heading turns 20° to the left, which turns the picture with
    // the fingers, and the ground under their center stays under it.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    const Neuron::Vec3 ground = GroundUnder(*camera, {800, 600});
    const double heading = camera->HeadingRadians();
    Species::CameraControls two;
    two.touchManipulation = Neuron::TouchManipulation{Neuron::GesturePhase::Begin, {800.0, 600.0}, 1.0, 0.0};
    Run(*camera, two, pointer, 0.016, 1);
    for (int n = 1; n <= 10; ++n)
    {
      two.touchManipulation =
        Neuron::TouchManipulation{Neuron::GesturePhase::Move, {800.0 + 10.0 * n, 600.0}, std::pow(1.25, 0.1), 2.0 * DEGREES};
      Run(*camera, two, pointer, 0.016, 1);
    }
    Assert::AreEqual(800.0, camera->Height(), 1e-6, L"pinched closer");
    Assert::AreEqual(0.0, std::remainder(camera->HeadingRadians() - heading - 20.0 * DEGREES, 2.0 * std::numbers::pi), 1e-9,
                     L"turned with the fingers");
    const Neuron::Vec3 under = GroundUnder(*camera, {900, 600});
    Assert::IsTrue(Neuron::HorizontalDistance(ground, under) < 2.0 * FootprintAt(*camera, under), L"the ground under their center");
    two.touchManipulation = Neuron::TouchManipulation{Neuron::GesturePhase::End, {900.0, 600.0}, 1.0, 0.0};
    Run(*camera, two, pointer, 0.016, 1);
    const Neuron::Vec3 held = camera->Focus();
    Run(*camera, {}, pointer, 0.01, 100);
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), held), 1e-6, L"no drift after the lift");
    Assert::AreEqual(800.0, camera->Height(), 1e-6, L"nor any zoom");
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

  BEGIN_TEST_METHOD_ATTRIBUTE(AcCtrlCa04TheGroundPointStopsAtTheRegionsBorder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-471")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(AcCtrlCa04TheGroundPointStopsAtTheRegionsBorder)
  {
    // Panning back for 60 s, away from the entry heading's corner, leaves the ground point on the bound on at least one
    // axis (ADR-119, ADR-180); a sandbox has no territory, so the region alone bounds it.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    Species::CameraControls back;
    back.panBack = true;
    back.fastPan = true;
    Species::StewardAreas areas;
    Run(*camera, back, pointer, 0.25, 240, &areas);
    const Species::CameraBounds& bounds = camera->Bounds();
    Assert::IsTrue(camera->Focus().x <= bounds.maxX + 1e-9 && camera->Focus().z <= bounds.maxZ + 1e-9, L"never beyond it");
    Assert::IsTrue(std::abs(camera->Focus().x - bounds.maxX) < 1e-6 || std::abs(camera->Focus().z - bounds.maxZ) < 1e-6, L"on the bound");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld244TheGroundPointStaysInTheStewardsAreas)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-244")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld244TheGroundPointStaysInTheStewardsAreas)
  {
    // With the steward's territory a disc of 300 wu about its site, its areas reach 1,100 wu from the site. Panning for
    // 6.4 s towards a point beyond them, the camera's ground point never leaves them, and comes to rest at their edge
    // (ADR-142, ADR-180).
    constexpr double RADIUS = 300.0;
    const double reach = RADIUS + CommittedTuning().wld.view.settlementMargin;
    const double cell = CommittedTuning().mvp.territory.cellSize;
    Species::StewardAreas areas;
    areas.Update(TerritoryDisc(RADIUS), std::uint8_t{0}, SeedOne().layout, CommittedTuning());
    Assert::IsTrue(areas.Known());
    const Neuron::Vec3& site = SeedOne().layout.sites[0];
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    Species::CameraControls forward;
    forward.panForward = true;
    double farthest = 0.0;
    for (int n = 0; n < 400; ++n)
    {
      Run(*camera, forward, pointer, 0.016, 1, &areas);
      Assert::IsTrue(areas.Contains(camera->Focus().x, camera->Focus().z), L"inside the areas every frame");
      farthest = std::max(farthest, Neuron::HorizontalDistance(camera->Focus(), site));
    }
    Assert::IsTrue(farthest <= reach + cell * std::numbers::sqrt2, L"never beyond the margin's last cell");
    Assert::IsTrue(Neuron::HorizontalDistance(camera->Focus(), site) > reach - 3.0 * cell, L"at rest on the areas' edge");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl464ThePointerIsFreeUnlessLookingAround)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-464")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-472")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-058")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl464ThePointerIsFreeUnlessLookingAround)
  {
    // O108 (ADR-180): the pointer stays where it is on the screen while the keys move the camera. Looking around holds the
    // camera still over its ground point, turns its front towards the point under the pointer, and the cursor lock keeps
    // that point under it; released, the front returns to the ground point.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    pointer = {700, 650};
    Species::CameraControls right;
    right.panRight = true;
    Run(*camera, right, pointer, 0.016, 30);
    Assert::IsTrue(pointer.x == 700 && pointer.y == 650, L"the free pointer stays put");
    const Neuron::Vec3 ground = GroundUnder(*camera, pointer);
    const Neuron::Vec3 focus = camera->Focus();
    const Neuron::Vec3 front = camera->Front();
    Species::CameraControls look;
    look.lookAround = true;
    look.panForward = true;
    Run(*camera, look, pointer, 0.016, 30);
    Assert::AreEqual(0.0, Neuron::HorizontalDistance(camera->Focus(), focus), 1e-9, L"held still");
    Assert::IsTrue(Neuron::Length(camera->Front() - front) > 0.01, L"the front turned");
    Assert::IsTrue(pointer.x != 700 || pointer.y != 650, L"the cursor lock moved the pointer");
    const Neuron::Vec3 under = GroundUnder(*camera, pointer);
    Assert::IsTrue(Neuron::HorizontalDistance(ground, under) < 3.0 * FootprintAt(*camera, under) / std::max(0.2, -camera->Front().y),
                   L"the ground point holds within a few pixels");
    Run(*camera, {}, pointer, 0.016, 120);
    const double pitch = camera->PitchRadiansAt(camera->Height());
    const Neuron::Vec3 onto = ForwardOf(*camera) * std::cos(pitch) + Neuron::Vec3{0.0, -std::sin(pitch), 0.0};
    Assert::IsTrue(Neuron::Length(camera->Front() - onto) < 1e-2, L"back onto the ground point");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr126TheBenchHoldsTheEntryPose)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"CAM-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr126TheBenchHoldsTheEntryPose)
  {
    // ADR-180 keeps the frame bench's view: O76's pose at the fallback height along the fallback direction.
    Neuron::PointPx pointer;
    auto camera = EnteredCamera(pointer);
    camera->HoldEntryPose(SeedOne().field, SeedOne().layout.sites[0]);
    Assert::AreEqual(1'000.0, camera->Position().y, 1e-9, L"ref.sim.match_entry.fallback_camera_height");
    Assert::AreEqual(1.0, Neuron::Dot(camera->Front(), Neuron::Normalized({-1.0, -0.7, -1.0})), 1e-12, L"along the fallback direction");
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
    const auto hovered = Species::LeaderUnderRay(figures, 0, ray, frame, VIEW, CommittedPresentation(), 2.0);
    Assert::IsTrue(hovered == Species::StableId{14}, L"the own living Leader centred under the pointer");
    Assert::IsFalse(Species::LeaderUnderRay(figures, 2, ray, frame, VIEW, CommittedPresentation(), 2.0).has_value(),
                    L"no Leader of team 2");
    // Far off the pointer, no sphere is met.
    const Neuron::Ray elsewhere = Neuron::PickRayAt(10.0, 10.0, frame, VIEW, 1.0);
    Assert::IsFalse(Species::LeaderUnderRay(figures, 0, elsewhere, frame, VIEW, CommittedPresentation(), 2.0).has_value());
    // ADR-167: with team 1 as the expedition's, its Leader is hovered too, the one centred under the pointer winning.
    figures[4].state.dead = true;
    figures[3].position = at + Neuron::Vec3{30.0, 0.0, 0.0};
    Assert::IsTrue(Species::LeaderUnderRay(figures, 0, ray, frame, VIEW, CommittedPresentation(), 2.0, std::uint8_t{1}) ==
                     Species::StableId{10},
                   L"the expedition's Leader under the pointer");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp049APressOnAnExpeditionLeaderCommandsItsTeam)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-049")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp049APressOnAnExpeditionLeaderCommandsItsTeam)
  {
    // ADR-167: the battle verbs command the settlement's team until a press on a Leader of its expedition, then the
    // expedition's until a press on one of the settlement's Leaders or the expedition's end.
    constexpr std::uint8_t TEAM = 2;
    const std::uint8_t expedition = Species::ExpeditionTeamOf(TEAM);
    std::vector<Species::InterpolatedFigure> figures(3);
    figures[0].id = 20;
    figures[0].state = {.team = TEAM, .kind = Species::FigureKind::Leader};
    figures[1].id = 21;
    figures[1].state = {.team = expedition, .kind = Species::FigureKind::Leader};
    figures[2].id = 22;
    figures[2].state = {.team = 1, .kind = Species::FigureKind::Leader};
    ExpectEqual(TEAM, Species::BattleTeamOf(figures, TEAM, expedition, TEAM, std::nullopt), L"the settlement's, with no press");
    ExpectEqual(expedition, Species::BattleTeamOf(figures, TEAM, expedition, TEAM, Species::StableId{21}), L"a press on its Leader");
    ExpectEqual(expedition, Species::BattleTeamOf(figures, TEAM, expedition, expedition, std::nullopt), L"kept");
    ExpectEqual(expedition, Species::BattleTeamOf(figures, TEAM, expedition, expedition, Species::StableId{22}), L"another's Leader");
    ExpectEqual(TEAM, Species::BattleTeamOf(figures, TEAM, expedition, expedition, Species::StableId{20}), L"back on a press on its own");
    figures[1].state.dead = true;
    ExpectEqual(TEAM, Species::BattleTeamOf(figures, TEAM, expedition, expedition, std::nullopt), L"the expedition's end");
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
    // ADR-153: the placement cursor, mvp.cursor.place_size, centred on ref.ctrl.cursor.set.place_hotspot, white.
    const Neuron::CursorQuad place = Species::CursorAt(Species::CursorKind::Place, {500, 300}, CommittedPresentation());
    Assert::IsTrue(place.image == 3 && place.sidePixels == 48.0f && place.left == 476.0f && place.top == 276.0f, L"48 px, centred");
    Assert::IsTrue(place.color[0] == 1.0f && place.color[1] == 1.0f && place.color[2] == 1.0f, L"white");
  }
};

} // namespace GameClientTests
