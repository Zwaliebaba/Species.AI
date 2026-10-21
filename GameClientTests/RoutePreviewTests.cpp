#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "RegionTerrain.h"
#include "RoutePreview.h"
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

const Species::RegionTerrain& SeedOneTerrain()
{
  static const Species::RegionTerrain REGION = []
  {
    auto generated = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(generated.has_value(), L"seed 1 gives a region that passes");
    return generated ? std::move(*generated) : Species::RegionTerrain{};
  }();
  return REGION;
}

Neuron::Vec3 OnGround(double _x, double _z)
{
  return {_x, SeedOneTerrain().field.Height(_x, _z), _z};
}

// The steward's site, and a point 200 wu from it towards the region's centre, on the valley's floor.
Neuron::Vec3 Site()
{
  const Neuron::Vec3 site = SeedOneTerrain().layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.stewardSite)];
  return OnGround(site.x, site.z);
}

Neuron::Vec3 TowardsCenter(double _distance)
{
  const Species::RegionLayout& layout = SeedOneTerrain().layout;
  const Neuron::Vec3 site = Site();
  const Neuron::Vec3 towards =
    Neuron::Normalized(Neuron::Vec3{layout.originX + layout.side * 0.5 - site.x, 0.0, layout.originZ + layout.side * 0.5 - site.z});
  return OnGround(site.x + towards.x * _distance, site.z + towards.z * _distance);
}

// A point of the sea channel on the region's border (WLD-163 check 1), level with the steward's site.
std::optional<Neuron::Vec3> UnderTheSea()
{
  const Species::RegionLayout& layout = SeedOneTerrain().layout;
  const double sea = CommittedTuning().ref.sim.physics.seaLevel;
  for (const double x : {layout.originX + 20.0, layout.originX + layout.side - 20.0})
  {
    const Neuron::Vec3 at = OnGround(x, Site().z);
    if (at.y <= sea)
    {
      return at;
    }
  }
  return std::nullopt;
}

const Neuron::QuadBatch* BatchOf(const std::vector<Neuron::QuadBatch>& _batches, std::string_view _entry)
{
  const auto found = std::ranges::find(_batches, _entry, &Neuron::QuadBatch::entry);
  return found != _batches.end() ? &*found : nullptr;
}

} // namespace

TEST_CLASS(RoutePreviewTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd353AWalkableRouteIsDottedInTheTeamColor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-353")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-355")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-023")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd353AWalkableRouteIsDottedInTheTeamColor)
  {
    Species::RoutePreview preview(CommittedPresentation(), CommittedTuning());
    preview.Update(Site(), TowardsCenter(200.0), false, SeedOneTerrain());
    Assert::IsTrue(preview.Shown() && preview.Walkable(), L"a walkable route across the valley's floor");
    const Neuron::Vec3 camera = Site() + Neuron::Vec3{0.0, 100.0, 0.0};
    std::vector<Neuron::QuadBatch> batches;
    preview.AppendBatches(camera, Neuron::Normalized(TowardsCenter(200.0) - camera), 0.0, Species::TeamColor(0), batches);
    const Neuron::QuadBatch* spots = BatchOf(batches, "route_line");
    Assert::IsTrue(spots != nullptr && spots->pass == Neuron::ScenePass::Routes &&
                     spots->texture == static_cast<std::uint16_t>(Species::SceneTexture::ParticleDot) && spots->quads.size() > 10,
                   L"spots of fx.particle_dot in pass 12");
    // Spot 0 at the start, its half-size its distance ÷ 100; spot 1 one spot's size, twice that, along the route.
    const Neuron::SceneQuad& first = spots->quads[0];
    const double half = Neuron::Length(preview.Points()[0] - camera) / 100.0;
    Assert::AreEqual(half, static_cast<double>(first.halfWidth), 1e-5, L"camera distance ÷ rnd.route.spot_size_divisor");
    Assert::AreEqual(2.0 * half, Neuron::Length(spots->quads[1].position - first.position), 1e-4, L"one spot's size apart");
    // The team colour with the grey offset, capped; alpha × (0.5 + 0.5 sin 0) for spot 0 at real time 0.
    const std::array<float, 4> team = Species::TeamColor(0);
    Assert::AreEqual(std::min(team[1] + 50.0f / 255.0f, 1.0f), first.color[1], 1e-6f, L"the grey offset");
    Assert::AreEqual(team[3] * 0.5f, first.color[3], 1e-6f, L"the pulse at spot 0");
    Assert::AreEqual(team[3] * static_cast<float>(0.5 + 0.5 * std::sin(0.7)), spots->quads[1].color[3], 1e-6f, L"and at spot 1");
    const Neuron::QuadBatch* arrows = BatchOf(batches, "route_arrow");
    Assert::IsTrue(arrows != nullptr && arrows->texture == static_cast<std::uint16_t>(Species::SceneTexture::RouteArrow) &&
                     !arrows->quads.empty() && arrows->quads.size() < preview.Points().size(),
                   L"an arrow of marker.route_arrow on the legs that hold one (DEC-959 item 4)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd354ARouteIntoTheSeaIsNotWalkable)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-354")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-353")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd354ARouteIntoTheSeaIsNotWalkable)
  {
    const std::optional<Neuron::Vec3> sea = UnderTheSea();
    Assert::IsTrue(sea.has_value(), L"the region has sea");
    Species::RoutePreview preview(CommittedPresentation(), CommittedTuning());
    preview.Update(Site(), sea, true, SeedOneTerrain());
    Assert::IsTrue(preview.Shown() && !preview.Walkable(), L"shown, and not walkable");
    std::vector<Neuron::QuadBatch> batches;
    preview.AppendBatches(Site() + Neuron::Vec3{0.0, 500.0, 0.0}, {0.0, -1.0, 0.0}, 0.0, Species::TeamColor(0), batches);
    const Neuron::QuadBatch* spots = BatchOf(batches, "route_line");
    Assert::IsTrue(spots != nullptr && !spots->quads.empty(), L"its spots");
    // rnd.route.blocked_color (0, 0, 0, 100) with the grey offset of 50.
    Assert::AreEqual(50.0f / 255.0f, spots->quads[0].color[0], 1e-6f, L"the blocked colour, greyed");
    Assert::AreEqual(100.0f / 255.0f * 0.5f, spots->quads[0].color[3], 1e-6f, L"its alpha at spot 0");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl023ThePreviewIsKeptUntilAnEndMoves)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-023")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-358")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl023ThePreviewIsKeptUntilAnEndMoves)
  {
    Species::RoutePreview preview(CommittedPresentation(), CommittedTuning());
    const Neuron::Vec3 to = TowardsCenter(200.0);
    preview.Update(Site(), to, true, SeedOneTerrain());
    const Neuron::Vec3 end = preview.Points().back();
    preview.Update(Site(), to + Neuron::Vec3{0.5, 0.0, 0.0}, true, SeedOneTerrain());
    Assert::IsTrue(preview.Points().back() == end, L"kept within rnd.route.preview_rebuild_distance");
    preview.Update(Site(), to + Neuron::Vec3{20.0, 0.0, 0.0}, true, SeedOneTerrain());
    Assert::IsTrue(std::abs(preview.Points().back().x - (to.x + 20.0)) < 1e-6, L"built again beyond it");
    preview.Update(std::nullopt, to, true, SeedOneTerrain());
    Assert::IsFalse(preview.Shown(), L"none without a selection");
    std::vector<Neuron::QuadBatch> batches;
    preview.AppendBatches(Site(), {0.0, -1.0, 0.0}, 0.0, Species::TeamColor(0), batches);
    Assert::IsTrue(batches.empty(), L"and nothing drawn");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd352AnOrderedRouteFadesOverFourSeconds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-351")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-352")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-160")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd352AnOrderedRouteFadesOverFourSeconds)
  {
    // ADR-127: an order sent keeps the walkable preview as an ordered route with the fade value 1, which the preview's
    // clearing leaves, and which falls by rnd.route.fade_rate (0.25) a second until it is dropped.
    Species::RoutePreview preview(CommittedPresentation(), CommittedTuning());
    preview.Update(Site(), TowardsCenter(200.0), false, SeedOneTerrain());
    preview.Commit();
    preview.Clear();
    Assert::IsTrue(!preview.Shown() && preview.OrderedRoutes().size() == 1 && preview.OrderedRoutes()[0].fade == 1.0,
                   L"kept, at full strength, when the preview goes");
    const Neuron::Vec3 camera = Site() + Neuron::Vec3{0.0, 100.0, 0.0};
    const Neuron::Vec3 front = Neuron::Normalized(TowardsCenter(200.0) - camera);
    std::vector<Neuron::QuadBatch> full;
    preview.AppendBatches(camera, front, 0.0, Species::TeamColor(0), full);
    preview.Advance(2.0);
    Assert::AreEqual(0.5, preview.OrderedRoutes()[0].fade, 1e-12, L"half after 2 s");
    std::vector<Neuron::QuadBatch> half;
    preview.AppendBatches(camera, front, 0.0, Species::TeamColor(0), half);
    const Neuron::QuadBatch* fullSpots = BatchOf(full, "route_line");
    const Neuron::QuadBatch* halfSpots = BatchOf(half, "route_line");
    Assert::IsTrue(fullSpots != nullptr && halfSpots != nullptr, L"its spots drawn");
    Assert::AreEqual(fullSpots->quads[0].color[3] * 0.5f, halfSpots->quads[0].color[3], 1e-6f, L"at half the alpha");
    preview.Advance(2.0);
    Assert::IsTrue(preview.OrderedRoutes().empty(), L"dropped after 4 s");
    // A preview that cannot be walked is never kept, and a new world drops the routes kept.
    const std::optional<Neuron::Vec3> sea = UnderTheSea();
    Assert::IsTrue(sea.has_value(), L"the region has sea");
    preview.Update(Site(), sea, true, SeedOneTerrain());
    preview.Commit();
    Assert::IsTrue(preview.OrderedRoutes().empty(), L"no route kept into the sea");
    preview.Update(Site(), TowardsCenter(200.0), false, SeedOneTerrain());
    preview.Commit();
    preview.Reset();
    Assert::IsTrue(preview.OrderedRoutes().empty() && !preview.Shown(), L"a reset drops both");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd353TheRouteEndsInAMarkOnTheGroundAndItsArrowsMeetTheScreensMiddle)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-353")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-186")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd353TheRouteEndsInAMarkOnTheGroundAndItsArrowsMeetTheScreensMiddle)
  {
    // ADR-127: marker.order_trail on the ground at the route's end, sized as the trail marks; and each arrow at the point of
    // its leg whose projection is the middle of the leg's, the fraction w0 ÷ (w0 + w1) of the ends' depths.
    Species::RoutePreview preview(CommittedPresentation(), CommittedTuning());
    preview.Update(Site(), TowardsCenter(300.0), true, SeedOneTerrain());
    Assert::IsTrue(preview.Shown(), L"a route");
    const Neuron::Vec3 camera = Site() + Neuron::Vec3{0.0, 150.0, 0.0} - (TowardsCenter(300.0) - Site()) * 0.5;
    const Neuron::Vec3 front = Neuron::Normalized(TowardsCenter(300.0) - camera);
    std::vector<Neuron::QuadBatch> batches;
    preview.AppendBatches(camera, front, 0.0, Species::TeamColor(0), batches);
    const auto end = std::ranges::find_if(batches, [](const Neuron::QuadBatch& _batch)
                                          { return _batch.texture == static_cast<std::uint16_t>(Species::SceneTexture::OrderTrail); });
    Assert::IsTrue(end != batches.end() && end->entry == "route_arrow" && end->pass == Neuron::ScenePass::Routes && end->quads.size() == 1,
                   L"one mark of marker.order_trail in pass 12");
    const Neuron::SceneQuad& mark = end->quads[0];
    Assert::IsTrue(mark.position == preview.Points().back() && mark.orientation == Neuron::QuadOrientation::WorldOriented,
                   L"lying at the route's end");
    const auto& leader = CommittedPresentation().ref.unit.leader;
    const double size =
      std::max(leader.trailSizeMin, leader.trailSizeScale * Neuron::Length(preview.Points().back() - camera) / leader.trailSizeDivisor);
    Assert::AreEqual(size, static_cast<double>(mark.halfWidth), 1e-4, L"the trail marks' size");
    const Neuron::QuadBatch* arrows = BatchOf(batches, "route_arrow");
    Assert::IsTrue(arrows != nullptr && arrows->texture == static_cast<std::uint16_t>(Species::SceneTexture::RouteArrow) &&
                     !arrows->quads.empty(),
                   L"the arrows");
    const std::vector<Neuron::Vec3>& points = preview.Points();
    for (const Neuron::SceneQuad& arrow : arrows->quads)
    {
      bool onItsLeg = false;
      for (std::size_t leg = 0; leg + 1 < points.size(); ++leg)
      {
        const double fromDepth = Neuron::Dot(points[leg] - camera, front);
        const double toDepth = Neuron::Dot(points[leg + 1] - camera, front);
        const Neuron::Vec3 middle = points[leg] + (points[leg + 1] - points[leg]) * (fromDepth / (fromDepth + toDepth));
        onItsLeg = onItsLeg || Neuron::Length(arrow.position - middle) < 1e-6;
      }
      Assert::IsTrue(onItsLeg, L"at its leg's middle on the screen");
    }
  }
};

} // namespace GameClientTests
