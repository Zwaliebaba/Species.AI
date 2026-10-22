#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "RegionTerrain.h"
#include "StableId.h"
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <numbers>
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

// A point on the valley's floor: the steward's site.
Neuron::Vec3 Ground()
{
  const Species::RegionTerrain& terrain = SeedOneTerrain();
  Neuron::Vec3 site = terrain.layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.stewardSite)];
  site.y = terrain.field.Height(site.x, site.z);
  return site;
}

// A Worker at the site, of unit counter _counter, facing +z.
Species::InterpolatedFigure WorkerAt(std::uint64_t _counter, std::uint8_t _team, const Neuron::Vec3& _position)
{
  Species::InterpolatedFigure figure;
  figure.id = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter);
  figure.state.team = _team;
  figure.state.kind = Species::FigureKind::Worker;
  figure.state.onGround = true;
  figure.state.health = 50;
  figure.health = 50.0;
  figure.position = _position;
  return figure;
}

// A view 60 wu in front of the Worker, along +z, looking back at it.
Species::CrowdView ViewOf(const Neuron::Vec3& _target)
{
  Species::CrowdView view;
  view.camera = _target + Neuron::Vec3{0.0, 10.0, 60.0};
  view.cameraFront = Neuron::Normalized(_target - view.camera);
  return view;
}

std::vector<const Neuron::QuadBatch*> BatchesOf(const std::vector<Neuron::QuadBatch>& _batches, std::string_view _entry)
{
  std::vector<const Neuron::QuadBatch*> found;
  for (const Neuron::QuadBatch& batch : _batches)
  {
    if (batch.entry == _entry)
    {
      found.push_back(&batch);
    }
  }
  return found;
}

std::size_t QuadsOf(const std::vector<Neuron::QuadBatch>& _batches, std::string_view _entry)
{
  std::size_t count = 0;
  for (const Neuron::QuadBatch* batch : BatchesOf(_batches, _entry))
  {
    count += batch->quads.size();
  }
  return count;
}

} // namespace

TEST_CLASS(CrowdLookTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit142ACardGrowsWithItsDistance)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-142")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-314")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-446")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit142ACardGrowsWithItsDistance)
  {
    const Species::TuningPresentation& presentation = CommittedPresentation();
    // Unit counter 10 is variation step 0 (ADR-121); RND T3: 3.0 × (1 + 2 × 1,900 ÷ 1,000) = 14.4 wu at 2,000 wu.
    const Species::StableId plain = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 10);
    Assert::AreEqual(3.0, Species::WorkerCardHalfSize(presentation, 2.0, plain, 50.0), 1e-12, L"near: the card's half-width");
    Assert::AreEqual(14.4, Species::WorkerCardHalfSize(presentation, 2.0, plain, 2'000.0), 1e-9, L"far: grown with the distance");
    // Unit counter 1 is step 9: 3.0 × (1 + 0.03 × 9).
    const Species::StableId varied = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 1);
    Assert::AreEqual(3.81, Species::WorkerCardHalfSize(presentation, 2.0, varied, 50.0), 1e-12, L"the individual variation");
    Assert::AreEqual(1.0, Species::WorkerDetail(presentation, 1, 0.0), 1e-12, L"full detail at the camera");
    Assert::AreEqual(0.0, Species::WorkerDetail(presentation, 3, 500.0), 1e-12, L"RND-446: entity detail 3 ends it at 500 wu");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx118ALivingWorkerHasItsShadowAndBody)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-118")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-317")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-147")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx118ALivingWorkerHasItsShadowAndBody)
  {
    const Neuron::Vec3 ground = Ground();
    const std::vector<Species::InterpolatedFigure> figures{WorkerAt(10, 1, ground)};
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendWorkerBatches(figures, SeedOneTerrain().field, ViewOf(ground), CommittedPresentation(), CommittedTuning(), batches);
    ExpectEqual(1, static_cast<std::int64_t>(QuadsOf(batches, "worker_ground_shadow")), L"one shadow");
    ExpectEqual(1, static_cast<std::int64_t>(QuadsOf(batches, "worker_card")), L"one body, seen face on, with no side strip");
    ExpectEqual(0, static_cast<std::int64_t>(QuadsOf(batches, "worker_glow")), L"no glow for another team's Worker");
    Assert::IsTrue(batches.front().entry == "worker_ground_shadow", L"the shadows first (VFX-118's order)");
    const Neuron::SceneQuad& body = BatchesOf(batches, "worker_card").front()->quads.front();
    Assert::IsTrue(body.anchor == Neuron::QuadAnchor::BottomCenter && std::abs(body.position.y - ground.y) < 1e-9,
                   L"its foot on the terrain");
    Assert::IsTrue(body.color == Species::TeamColor(1), L"full health: the team colour × (0.3 + 0.7)");
    const Neuron::SceneQuad& shadow = BatchesOf(batches, "worker_ground_shadow").front()->quads.front();
    Assert::IsTrue(shadow.axisUp == Neuron::Vec3(0.0, 0.0, 1.0) && shadow.color[3] > 0.58f && shadow.color[3] < 0.59f,
                   L"flat along +z, black at 150 ÷ 255");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx118ACorpseDriftsApartAsFragments)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-118")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-146")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx118ACorpseDriftsApartAsFragments)
  {
    const Neuron::Vec3 ground = Ground();
    Species::InterpolatedFigure lying = WorkerAt(10, 1, ground);
    lying.state.dead = true;
    lying.health = 60.0;
    Species::InterpolatedFigure flying = WorkerAt(20, 1, ground + Neuron::Vec3{0.0, 20.0, 0.0});
    flying.state.dead = true;
    flying.state.onGround = false;
    flying.health = 100.0;
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendWorkerBatches(std::vector{lying}, SeedOneTerrain().field, ViewOf(ground), CommittedPresentation(), CommittedTuning(),
                                 batches);
    ExpectEqual(3, static_cast<std::int64_t>(QuadsOf(batches, "worker_card")), L"a corpse on the ground: its three fragments only");
    ExpectEqual(0, static_cast<std::int64_t>(QuadsOf(batches, "worker_ground_shadow")), L"and no shadow");
    const Neuron::SceneQuad& fragment = BatchesOf(batches, "worker_card").front()->quads.front();
    Assert::AreEqual(1.5f, fragment.halfWidth, 1e-6f, L"half the card");
    Assert::AreEqual(0.6f, fragment.color[3], 1e-6f, L"alpha 60 × 2.55 ÷ 255");
    Assert::AreEqual(ground.y + 4.0, fragment.position.y, 1e-9, L"risen by 10 − 60 ÷ 10");
    batches.clear();
    Species::AppendWorkerBatches(std::vector{flying}, SeedOneTerrain().field, ViewOf(ground), CommittedPresentation(), CommittedTuning(),
                                 batches);
    ExpectEqual(4, static_cast<std::int64_t>(QuadsOf(batches, "worker_card")), L"in the air: its body and three fragments");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit147TheLocalTeamsControlledWorkerGlows)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-147")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-118")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit147TheLocalTeamsControlledWorkerGlows)
  {
    const Neuron::Vec3 ground = Ground();
    Species::InterpolatedFigure controlled = WorkerAt(10, 0, ground);
    controlled.state.underControl = true;
    Species::CrowdView view = ViewOf(ground);
    view.localTeam = std::uint8_t{0};
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendWorkerBatches(std::vector{controlled}, SeedOneTerrain().field, view, CommittedPresentation(), CommittedTuning(),
                                 batches);
    ExpectEqual(10, static_cast<std::int64_t>(QuadsOf(batches, "worker_glow")), L"ten stacked glowing cards");
    Assert::IsTrue(batches.back().entry == "worker_glow", L"drawn last");
    Assert::AreEqual(0.3f, BatchesOf(batches, "worker_glow").front()->quads.front().color[3], 1e-6f, L"never below 0.3 (at t = 0)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit147ASelectedWorkerHasADiscOnTheGround)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-147")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit147ASelectedWorkerHasADiscOnTheGround)
  {
    // ADR-127: marker.team_disc on the terrain under the local team's controlled Worker, in the control glow's colour at
    // mvp.selection.disc_alpha, 0.8 × its card's half-size; none for another team's, an uncontrolled or a dead Worker.
    const Neuron::Vec3 ground = Ground();
    Species::InterpolatedFigure controlled = WorkerAt(10, 0, ground + Neuron::Vec3{0.0, 3.0, 0.0});
    controlled.state.underControl = true;
    Species::InterpolatedFigure theirs = WorkerAt(11, 1, ground);
    theirs.state.underControl = true;
    Species::InterpolatedFigure dead = WorkerAt(12, 0, ground);
    dead.state.underControl = true;
    dead.state.dead = true;
    const Species::InterpolatedFigure idle = WorkerAt(13, 0, ground);
    Species::CrowdView view = ViewOf(ground);
    view.localTeam = std::uint8_t{0};
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendSelectionDiscs(std::vector{controlled, theirs, dead, idle}, SeedOneTerrain().field, view, CommittedPresentation(),
                                  batches);
    Assert::IsTrue(batches.size() == 1 && batches[0].pass == Neuron::ScenePass::Workers && batches[0].entry == "effect_additive_nodepth" &&
                     batches[0].texture == static_cast<std::uint16_t>(Species::SceneTexture::TeamDisc) && batches[0].quads.size() == 1,
                   L"one disc of marker.team_disc in pass 7d");
    const Neuron::SceneQuad& disc = batches[0].quads[0];
    Assert::AreEqual(SeedOneTerrain().field.Height(ground.x, ground.z), disc.position.y, 1e-9, L"on the terrain, not at the figure");
    Assert::IsTrue(disc.orientation == Neuron::QuadOrientation::WorldOriented, L"lying on the ground");
    const double card =
      Species::WorkerCardHalfSize(CommittedPresentation(), 2.0, controlled.id, Neuron::Length(disc.position - view.camera));
    Assert::AreEqual(0.8 * card, static_cast<double>(disc.halfWidth), 1e-4, L"mvp.selection.disc_size_factor");
    Assert::AreEqual(0.35f, disc.color[3], 1e-6f, L"mvp.selection.disc_alpha");
    view.localTeam.reset();
    batches.clear();
    Species::AppendSelectionDiscs(std::vector{controlled}, SeedOneTerrain().field, view, CommittedPresentation(), batches);
    Assert::IsTrue(batches.empty(), L"none while the client commands no team");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit144AnEdgeOnCardGainsItsSideStrip)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-144")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-118")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit144AnEdgeOnCardGainsItsSideStrip)
  {
    const Neuron::Vec3 ground = Ground();
    Species::InterpolatedFigure sideways = WorkerAt(10, 1, ground);
    sideways.yawRadians = std::numbers::pi / 2.0; // facing +x, across the view along +z
    Species::CrowdView view = ViewOf(ground);
    view.camera = ground + Neuron::Vec3{0.0, 0.0, 60.0};
    view.cameraFront = {0.0, 0.0, -1.0};
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendWorkerBatches(std::vector{sideways}, SeedOneTerrain().field, view, CommittedPresentation(), CommittedTuning(), batches);
    const auto side = std::ranges::find_if(batches, [](const Neuron::QuadBatch& _batch)
                                           { return _batch.texture == static_cast<std::uint16_t>(Species::SceneTexture::WorkerSide); });
    Assert::IsTrue(side != batches.end() && side->quads.size() == 1, L"the side strip");
    Assert::AreEqual(1.0f, side->quads.front().color[3], 1e-6f, L"fully faded in when wholly edge-on: 20 × (1 − 0.95)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp082ACarriedLoadIsVisibleOnTheCard)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-082")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-031")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp082ACarriedLoadIsVisibleOnTheCard)
  {
    const Neuron::Vec3 ground = Ground();
    const Species::CrowdView view = ViewOf(ground);
    Species::InterpolatedFigure carrier = WorkerAt(10, 1, ground);
    carrier.state.loaded = true;
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendWorkerBatches(std::vector{carrier}, SeedOneTerrain().field, view, CommittedPresentation(), CommittedTuning(), batches);
    ExpectEqual(2, static_cast<std::int64_t>(QuadsOf(batches, "worker_card")), L"the body and its load");
    const auto load =
      std::ranges::find(batches, static_cast<std::uint16_t>(Species::SceneTexture::WorkerCarryLoad), &Neuron::QuadBatch::texture);
    Assert::IsTrue(load != batches.end() && load->quads.size() == 1, L"the load's card");
    const Neuron::SceneQuad& body = BatchesOf(batches, "worker_card").front()->quads.front();
    const Neuron::SceneQuad& carried = load->quads.front();
    Assert::IsTrue(carried.halfWidth == body.halfWidth && carried.axisRight == body.axisRight && carried.axisUp == body.axisUp,
                   L"scaled and turned with the card");
    Assert::AreEqual(Neuron::Length(view.camera - body.position) - 0.05, Neuron::Length(view.camera - carried.position), 1e-9,
                     L"just in front of it");
    const auto& color = CommittedPresentation().mvp.worker.loadColor;
    Assert::AreEqual(static_cast<float>(color.b), carried.color[2], 1e-6f, L"in the load's colour");

    carrier.state.dead = true;
    batches.clear();
    Species::AppendWorkerBatches(std::vector{carrier}, SeedOneTerrain().field, view, CommittedPresentation(), CommittedTuning(), batches);
    Assert::IsTrue(std::ranges::find(batches, static_cast<std::uint16_t>(Species::SceneTexture::WorkerCarryLoad),
                                     &Neuron::QuadBatch::texture) == batches.end(),
                   L"a corpse carries nothing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp025AnOrbDriftingHomeOrWaitingIsWrappedInACloudyGlow)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-023")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-025")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp025AnOrbDriftingHomeOrWaitingIsWrappedInACloudyGlow)
  {
    const auto& orb = CommittedPresentation().mvp.orb;
    std::vector<Species::InterpolatedOrb> orbs(3);
    orbs[0].state.state = Species::OrbState::Floating;
    orbs[1].state.state = Species::OrbState::Drifting;
    orbs[1].state.team = 2;
    orbs[2].state.state = Species::OrbState::Waiting;
    orbs[2].state.team = 2;
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendOrbBatches(orbs, CommittedPresentation(), batches);
    Assert::IsTrue(batches.size() == 2 && batches[0].quads.size() == 6, L"two quads an orb, then the clouds");
    const Neuron::QuadBatch& clouds = batches[1];
    Assert::IsTrue(clouds.texture == static_cast<std::uint16_t>(Species::SceneTexture::GlowCloudy) && clouds.quads.size() == 2,
                   L"a cloud about each orb on its way home or waiting, and none about a floating one");
    Assert::AreEqual(static_cast<float>(orb.cloudySize), clouds.quads[0].halfWidth, 1e-6f, L"its size");
    Assert::AreEqual(static_cast<float>(orb.cloudyAlpha), clouds.quads[1].color[3], 1e-6f, L"its alpha");
    Assert::AreEqual(Species::TeamColor(2)[0], clouds.quads[1].color[0], 1e-6f, L"in its settlement's colour");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit331AnOrbGrowsAtBirthAndFadesInAscent)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-330")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-331")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit331AnOrbGrowsAtBirthAndFadesInAscent)
  {
    std::vector<Species::InterpolatedOrb> orbs(3);
    orbs[0].state.state = Species::OrbState::Birth;
    orbs[0].state.timer = 30;
    orbs[1].state.state = Species::OrbState::Floating;
    orbs[2].state.state = Species::OrbState::Ascent;
    orbs[2].state.timer = 450;
    std::vector<Neuron::QuadBatch> batches;
    Species::AppendOrbBatches(orbs, CommittedPresentation(), batches);
    Assert::IsTrue(batches.size() == 1 && batches[0].entry == "orb_additive" && batches[0].quads.size() == 6, L"two quads an orb");
    const std::vector<Neuron::SceneQuad>& quads = batches[0].quads;
    Assert::AreEqual(0.5f * 0.7f, quads[0].halfWidth, 1e-6f, L"Birth: × (1.2 − 30 ÷ 60)");
    Assert::AreEqual(0.5f, quads[2].halfWidth, 1e-6f, L"Floating: the inner size");
    Assert::AreEqual(1.5f, quads[3].halfWidth, 1e-6f, L"and the outer");
    Assert::AreEqual(0.25f, quads[4].halfWidth, 1e-6f, L"Ascent: × 450 ÷ 900");
    Assert::AreEqual(127.0f / 255.0f * 0.5f, quads[4].color[3], 1e-6f, L"and fading by the same factor");
  }
};

} // namespace GameClientTests
