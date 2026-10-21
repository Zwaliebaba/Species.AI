#include "pch.h"

#include "DepthBias.h"
#include "DevelopmentOverlay.h"
#include "LeaderLook.h"
#include "StableId.h"
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <string_view>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr Neuron::Vec3 PLACE{1'000.0, 50.0, 2'000.0};
constexpr double SOCKET_HEIGHT = 6.0;

// A one-node figure with its flag socket SOCKET_HEIGHT above its origin, as mesh.unit.leader_figure has one (AST-076).
Neuron::MeshAsset Figure()
{
  Neuron::MeshAsset mesh;
  mesh.nodes.push_back(Neuron::MeshNode{.name = "leader_figure"});
  Neuron::MeshSocket socket;
  socket.kind = Neuron::SocketKind::Flag;
  socket.transform = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, static_cast<float>(SOCKET_HEIGHT),
                      0.0F, 1.0F};
  mesh.sockets.push_back(socket);
  return mesh;
}

Species::InterpolatedFigure LeaderAt(std::uint64_t _counter, std::uint8_t _team)
{
  Species::InterpolatedFigure figure;
  figure.id = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter);
  figure.state.team = _team;
  figure.state.kind = Species::FigureKind::Leader;
  figure.state.onGround = true;
  figure.state.health = 255;
  figure.health = 255.0;
  figure.position = PLACE;
  return figure;
}

// A view 50 wu along +x, looking back along −x: the Leader faces +z, across the view.
Species::CrowdView View()
{
  Species::CrowdView view;
  view.camera = PLACE + Neuron::Vec3{50.0, 0.0, 0.0};
  view.cameraFront = {-1.0, 0.0, 0.0};
  return view;
}

const Neuron::TriangleBatch* BannerOf(const Species::LeaderDraws& _draws, Species::SceneTexture _texture)
{
  for (const Neuron::TriangleBatch& batch : _draws.triangles)
  {
    if (batch.texture == static_cast<std::uint16_t>(_texture))
    {
      return &batch;
    }
  }
  return nullptr;
}

} // namespace

TEST_CLASS(LeaderLookTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit190ALeaderIsItsFigureUnderItsTeamsTint)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-190")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-271")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit190ALeaderIsItsFigureUnderItsTeamsTint)
  {
    Species::LeaderLook look;
    Species::LeaderDraws draws;
    const std::vector<Species::InterpolatedFigure> figures{LeaderAt(5, 1)};
    look.Draw(figures, Figure(), View(), 0.0, CommittedPresentation(), CommittedTuning(), draws);
    Assert::IsTrue(draws.meshes.size() == 1 && draws.meshes[0].entry == "actor_mesh_lit" && draws.meshes[0].instances.size() == 1,
                   L"one body, unflickering at full health");
    const Neuron::SceneMeshInstance& body = draws.meshes[0].instances[0];
    const std::array<float, 4> red = Species::TeamColor(1);
    Assert::AreEqual(red[0] * 0.7f, body.tint[0], 1e-6f, L"the team colour × rnd.tint.factor");
    Assert::AreEqual(1.0f, body.tintWeight);
    Assert::IsTrue(std::abs(body.axisZ.z - 1.0) < 1e-12 && std::abs(body.axisY.y - 1.0) < 1e-12, L"facing +z at scale 1 within 100 wu");
    Assert::IsTrue(body.axisZ.y == 0.0 && body.axisX.y == 0.0 && std::abs(body.axisZ.x) < 1e-12, L"upright, facing its heading");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit191TheBannerFliesFromTheFlagSocketInItsVariant)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-191")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-139")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-141")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit191TheBannerFliesFromTheFlagSocketInItsVariant)
  {
    Species::LeaderLook look;
    Species::InterpolatedFigure idle = LeaderAt(5, 1);
    Species::InterpolatedFigure following = LeaderAt(6, 1);
    following.state.order = static_cast<std::uint8_t>(Species::LeaderOrder::Follow);
    Species::InterpolatedFigure formed = LeaderAt(7, 0);
    formed.state.formation = true;
    formed.state.selected = true;
    Species::LeaderDraws draws;
    look.Draw(std::vector{idle, following, formed}, Figure(), View(), 0.5, CommittedPresentation(), CommittedTuning(), draws);
    // VFX-141: a pole a Leader, at its flag socket, 1.5 sizes high, and 5 × 5 cells of two triangles each.
    Assert::IsTrue(draws.quads.size() == 1 && draws.quads[0].entry == "cloth_pole" && draws.quads[0].quads.size() == 3, L"three poles");
    const Neuron::SceneQuad& pole = draws.quads[0].quads[0];
    Assert::AreEqual(PLACE.y + SOCKET_HEIGHT, pole.position.y, 1e-9, L"at the flag socket");
    Assert::AreEqual(15.0f, pole.halfHeight, 1e-6f, L"half of 20 × 1.5");
    for (const Species::SceneTexture texture :
         {Species::SceneTexture::BannerIdle, Species::SceneTexture::BannerFollow, Species::SceneTexture::BannerFormation})
    {
      const Neuron::TriangleBatch* banner = BannerOf(draws, texture);
      Assert::IsTrue(banner != nullptr && banner->vertices.size() == 150, L"one banner of each variant, 25 cells");
    }
    const Neuron::TriangleBatch* selected = BannerOf(draws, Species::SceneTexture::BannerFormation);
    Assert::IsTrue(selected != nullptr && selected->vertices[0].color[1] == Species::TeamColor(0)[1], L"selected: the whole team colour");
    const Neuron::TriangleBatch* plain = BannerOf(draws, Species::SceneTexture::BannerIdle);
    Assert::IsTrue(plain != nullptr && plain->vertices[0].color[0] == Species::TeamColor(1)[0] * 0.5f, L"else × vfx.cloth.team_tint");
    Assert::IsTrue(plain != nullptr && plain->mask == static_cast<std::uint16_t>(Species::SceneTexture::BannerTintIdle), L"its tint mask");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx141AWaypointBannerPointsAtItsTarget)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-141")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-014")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-191")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx141AWaypointBannerPointsAtItsTarget)
  {
    // ADR-125: the compiled-in bias_flag_pointer is the record's.
    const auto& cloth = CommittedPresentation().vfx.cloth;
    ExpectEqual(Neuron::DEPTH_BIAS_FLAG_POINTER, static_cast<std::int64_t>(cloth.pointerConstantBias), L"the constant bias");
    Assert::AreEqual(static_cast<double>(Neuron::SLOPE_DEPTH_BIAS_FLAG_POINTER), cloth.pointerSlopeBias, L"the slope-scaled bias");
    // The view looks along −x, so −z is to the camera's right: a target 200 wu along −z lies right of the banner.
    Species::InterpolatedFigure waypoint = LeaderAt(5, 1);
    waypoint.state.order = static_cast<std::uint8_t>(Species::LeaderOrder::Waypoint);
    waypoint.state.target = Species::QuantizePosition(PLACE + Neuron::Vec3{0.0, 0.0, -200.0});
    Species::InterpolatedFigure untargeted = LeaderAt(6, 1);
    untargeted.state.order = static_cast<std::uint8_t>(Species::LeaderOrder::Waypoint);
    Species::LeaderLook look;
    Species::LeaderDraws draws;
    look.Draw(std::vector{waypoint, untargeted, LeaderAt(7, 1)}, Figure(), View(), 0.0, CommittedPresentation(), CommittedTuning(), draws);
    const auto pointers = std::ranges::find(draws.quads, Neuron::ScenePass::Pointers, &Neuron::QuadBatch::pass);
    Assert::IsTrue(pointers != draws.quads.end() && pointers->entry == "cloth_pointer" &&
                     pointers->texture == static_cast<std::uint16_t>(Species::SceneTexture::FlagPointer) && pointers->quads.size() == 1,
                   L"one marker.flag_pointer, after the cloth, for the Leader with a target");
    const Neuron::SceneQuad& pointer = pointers->quads[0];
    Assert::AreEqual(5.0f, pointer.halfWidth, 1e-6f, L"vfx.cloth.pointer_size_factor × the banner's size of 20");
    // Its art points down; turned by r it points along (sin r, −cos r) on the screen, here to the right.
    Assert::IsTrue(std::sin(pointer.rotationRadians) > 0.95f, L"turned to point right, towards the target");
    Assert::IsTrue(pointer.orientation == Neuron::QuadOrientation::CameraFacing, L"facing the camera");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx139TheClothKeepsItsLinksAndFollowsItsPole)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-139")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-018")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx139TheClothKeepsItsLinksAndFollowsItsPole)
  {
    Species::LeaderLook look;
    Species::InterpolatedFigure leader = LeaderAt(5, 1);
    Species::CrowdView view = View();
    for (int frame = 0; frame < 120; ++frame)
    {
      view.unscaledSeconds = frame / 60.0;
      if (frame == 60)
      {
        leader.position = PLACE + Neuron::Vec3{0.0, 0.0, 30.0}; // the Leader moves: its pole goes with it
      }
      Species::LeaderDraws draws;
      look.Draw(std::vector{leader}, Figure(), view, 1.0 / 60.0, CommittedPresentation(), CommittedTuning(), draws);
      const Neuron::TriangleBatch* banner = BannerOf(draws, Species::SceneTexture::BannerIdle);
      Assert::IsTrue(banner != nullptr && banner->vertices.size() == 150, L"the banner every frame");
      // The first cell's pole edge sits on the pole, size ÷ 2 above the socket.
      Assert::AreEqual(leader.position.z, banner->vertices[0].position.z, 1e-9, L"the pole column follows the pole at once");
      Assert::AreEqual(leader.position.y + SOCKET_HEIGHT + 10.0, banner->vertices[0].position.y, 1e-9);
      for (std::size_t n = 0; n + 1 < banner->vertices.size(); n += 6)
      {
        Assert::IsTrue(Neuron::Length(banner->vertices[n + 1].position - banner->vertices[n].position) <= 10.0 + 1e-9,
                       L"no link longer than half the size");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx126AWoundedLeaderFlickersAndAWholeOneNever)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-126")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-127")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-051")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx126AWoundedLeaderFlickersAndAWholeOneNever)
  {
    Species::LeaderLook look;
    Species::InterpolatedFigure wounded = LeaderAt(5, 1);
    wounded.state.health = 0;
    wounded.health = 0.0;
    std::size_t flickered = 0;
    std::size_t whole = 0;
    Species::CrowdView view = View();
    for (int step = 0; step < 50; ++step)
    {
      view.scaledSeconds = step * 0.1 + 0.05; // one flicker step each
      Species::LeaderDraws draws;
      look.Draw(std::vector{wounded, LeaderAt(6, 1)}, Figure(), view, 0.0, CommittedPresentation(), CommittedTuning(), draws);
      for (const Neuron::MeshBatch& batch : draws.meshes)
      {
        if (batch.entry == "actor_flicker_additive")
        {
          flickered += batch.instances.size();
        }
        else
        {
          whole += batch.instances.size();
        }
      }
    }
    Assert::IsTrue(flickered > 30, L"a Leader with no health left flickers on most steps");
    Assert::IsTrue(whole >= 50, L"the Leader at full health never does");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd307ALeaderNoLongerDrawnLosesItsState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-307")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd307ALeaderNoLongerDrawnLosesItsState)
  {
    Species::LeaderLook look;
    Species::LeaderDraws draws;
    Species::InterpolatedFigure dead = LeaderAt(6, 1);
    dead.state.dead = true;
    dead.health = 80.0;
    look.Draw(std::vector{LeaderAt(5, 1), dead}, Figure(), View(), 0.0, CommittedPresentation(), CommittedTuning(), draws);
    ExpectEqual(2, static_cast<std::int64_t>(look.KeptLeaders()), L"both kept");
    ExpectEqual(1, static_cast<std::int64_t>(draws.quads.empty() ? 0 : draws.quads[0].quads.size()), L"a dead Leader has no banner (O77)");
    draws = {};
    look.Draw(std::vector{LeaderAt(5, 1)}, Figure(), View(), 0.0, CommittedPresentation(), CommittedTuning(), draws);
    ExpectEqual(1, static_cast<std::int64_t>(look.KeptLeaders()), L"the removed one dropped");
  }
};

} // namespace GameClientTests
