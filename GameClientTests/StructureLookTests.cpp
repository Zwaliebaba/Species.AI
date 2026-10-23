#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "RegionTerrain.h"
#include "StableId.h"
#include "StateReplica.h"
#include "StructureLook.h"
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

constexpr double BODY_HEIGHT = 10.0;
constexpr double PORT_OFFSET = 8.0;
constexpr double PAD_STATUS_HEIGHT = 3.0;
constexpr double LIGHT_HEIGHT = 10.0;
constexpr double CORE_HEIGHT = 10.0;
constexpr Neuron::Vec3 BARREL_MOUNT{0.0, 3.0, 1.0};
constexpr Neuron::Vec3 TURRET_STATUS{0.0, 5.0, 0.0};
constexpr double TICK_SECONDS = 0.1;

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

// The steward's site on the valley's floor.
Neuron::Vec3 Ground()
{
  const Species::RegionTerrain& terrain = SeedOneTerrain();
  Neuron::Vec3 site = terrain.layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.stewardSite)];
  site.y = terrain.field.Height(site.x, site.z);
  return site;
}

std::array<float, 16> Translation(const Neuron::Vec3& _at)
{
  return {1.0F,
          0.0F,
          0.0F,
          0.0F,
          0.0F,
          1.0F,
          0.0F,
          0.0F,
          0.0F,
          0.0F,
          1.0F,
          0.0F,
          static_cast<float>(_at.x),
          static_cast<float>(_at.y),
          static_cast<float>(_at.z),
          1.0F};
}

void AddSocket(Neuron::MeshAsset& _mesh, Neuron::SocketKind _kind, std::uint32_t _index, const Neuron::Vec3& _at)
{
  Neuron::MeshSocket socket;
  socket.kind = _kind;
  socket.index = _index;
  socket.transform = Translation(_at);
  _mesh.sockets.push_back(socket);
}

// A drawn node of _height from its origin up, and 5 wu about it.
Neuron::MeshNode Node(std::string_view _name, double _low, double _high)
{
  Neuron::MeshNode node;
  node.name = _name;
  node.draws.push_back(Neuron::MeshDraw{});
  node.boundsMin = {-5.0F, static_cast<float>(_low), -5.0F};
  node.boundsMax = {5.0F, static_cast<float>(_high), 5.0F};
  node.sphereRadius = 8.0F;
  return node;
}

// Every scene mesh: a body BODY_HEIGHT tall with two ports, either side of its front, and a light on its top; the port
// pad with its status socket; the turret's mount of one port and its status, its head with its barrel mount; and the
// Wellspring with its crystal.
Neuron::SceneMeshes Meshes()
{
  Neuron::SceneMeshes meshes;
  for (std::size_t n = 0; n < static_cast<std::size_t>(Species::SceneMesh::Count); ++n)
  {
    auto mesh = std::make_shared<Neuron::MeshAsset>();
    const auto kind = static_cast<Species::SceneMesh>(n);
    switch (kind)
    {
    case Species::SceneMesh::PortPad:
      mesh->nodes.push_back(Node("pad", 0.0, 1.0));
      AddSocket(*mesh, Neuron::SocketKind::Status, 0, {0.0, PAD_STATUS_HEIGHT, 0.0});
      break;
    case Species::SceneMesh::TurretMount:
      mesh->nodes.emplace_back().name = "mount"; // no draws
      AddSocket(*mesh, Neuron::SocketKind::Port, 0, {0.0, 0.0, -PORT_OFFSET});
      AddSocket(*mesh, Neuron::SocketKind::Status, 0, TURRET_STATUS);
      break;
    case Species::SceneMesh::TurretHead:
      mesh->nodes.push_back(Node("head", 0.0, 4.0));
      AddSocket(*mesh, Neuron::SocketKind::BarrelMount, 0, BARREL_MOUNT);
      break;
    case Species::SceneMesh::Wellspring:
      mesh->nodes.push_back(Node("wellspring_base", 0.0, BODY_HEIGHT));
      mesh->nodes.push_back(Node("wellspring_core", CORE_HEIGHT - 2.0, CORE_HEIGHT + 2.0));
      AddSocket(*mesh, Neuron::SocketKind::Port, 0, {PORT_OFFSET, 0.0, 0.0});
      break;
    default:
      mesh->nodes.push_back(Node("body", 0.0, BODY_HEIGHT));
      AddSocket(*mesh, Neuron::SocketKind::Port, 0, {PORT_OFFSET, 0.0, 0.0});
      AddSocket(*mesh, Neuron::SocketKind::Port, 1, {-PORT_OFFSET, 0.0, 0.0});
      AddSocket(*mesh, Neuron::SocketKind::Light, 0, {0.0, LIGHT_HEIGHT, 0.0});
      break;
    }
    meshes.meshes.push_back(std::move(mesh));
  }
  return meshes;
}

Species::InterpolatedStructure StructureAt(Species::StructureKind _kind, std::optional<std::uint8_t> _owner, const Neuron::Vec3& _at)
{
  Species::InterpolatedStructure structure;
  structure.id = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 1);
  structure.state.kind = _kind;
  structure.state.owner = _owner;
  structure.state.position = Species::QuantizePosition(_at);
  return structure;
}

Species::StructureView ViewAt(double _displayTick)
{
  return {_displayTick, TICK_SECONDS, 0.0};
}

Species::StructureDraws Draw(Species::StructureLook& _look, const std::vector<Species::InterpolatedStructure>& _structures,
                             double _displayTick = 0.0)
{
  static const Neuron::SceneMeshes MESHES = Meshes();
  Species::StructureDraws draws;
  _look.Draw(_structures, MESHES, SeedOneTerrain().field, ViewAt(_displayTick), draws);
  return draws;
}

const Neuron::MeshBatch* MeshBatchOf(const Species::StructureDraws& _draws, Species::SceneMesh _mesh)
{
  const auto found = std::ranges::find(_draws.meshes, static_cast<std::uint16_t>(_mesh), &Neuron::MeshBatch::mesh);
  return found != _draws.meshes.end() ? &*found : nullptr;
}

const Neuron::QuadBatch* QuadBatchOf(const Species::StructureDraws& _draws, Species::SceneTexture _texture)
{
  const auto found = std::ranges::find(_draws.quads, static_cast<std::uint16_t>(_texture), &Neuron::QuadBatch::texture);
  return found != _draws.quads.end() ? &*found : nullptr;
}

std::size_t QuadCount(const Species::StructureDraws& _draws)
{
  std::size_t count = 0;
  for (const Neuron::QuadBatch& batch : _draws.quads)
  {
    count += batch.quads.size();
  }
  return count;
}

bool Near(const Neuron::Vec3& _a, const Neuron::Vec3& _b, double _tolerance = 1e-6)
{
  return Neuron::Length(_a - _b) < _tolerance;
}

std::array<float, 3> TintOf(std::uint8_t _team)
{
  const std::array<float, 4> color = Species::TeamColor(_team);
  const double factor = CommittedPresentation().rnd.tint.factor;
  return {static_cast<float>(color[0] * factor), static_cast<float>(color[1] * factor), static_cast<float>(color[2] * factor)};
}

Species::StateRecord RecordOf(Species::StableId _id, const Species::ReplicatedState& _state, const Species::ReplicatedState* _baseline)
{
  const std::optional<Species::StateRecord> record = Species::MakeRecord(_id, _state, _baseline);
  Assert::IsTrue(record.has_value(), L"the state differs from its baseline");
  return record.value_or(Species::StateRecord{});
}

} // namespace

TEST_CLASS(StructureLookTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp083ASiteRisesWithItsWorkAndAnInactiveStructureIsDark)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-083")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-035")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-163")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-271")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp083ASiteRisesWithItsWorkAndAnInactiveStructureIsDark)
  {
    const Species::TuningPresentation& presentation = CommittedPresentation();
    const double depth = BODY_HEIGHT * presentation.mvp.structure.riseDepthFactor;
    const Neuron::Vec3 ground = Ground();
    Species::StructureLook look(presentation);
    Species::InterpolatedStructure dwelling = StructureAt(Species::StructureKind::Dwelling, std::uint8_t{2}, ground);
    const Neuron::Vec3 rest = Species::PositionOf(dwelling.state.position);

    dwelling.state.built = false;
    dwelling.state.progress = 0;
    Species::StructureDraws draws = Draw(look, {dwelling});
    const Neuron::MeshBatch* solid = MeshBatchOf(draws, Species::SceneMesh::Dwelling);
    Assert::IsTrue(solid != nullptr && solid->instances.size() == 1, L"one dwelling");
    Assert::IsTrue(solid->pass == Neuron::ScenePass::Structures && solid->entry == "mesh_lit_fog", L"pass 6, lit and fogged");
    Assert::AreEqual(rest.y - depth, solid->instances.front().position.y, 1e-9, L"a new site is sunk by its height");
    Assert::IsTrue(solid->instances.front().tint == TintOf(2) && solid->instances.front().tintWeight == 1.0f, L"in its team's tint");

    dwelling.state.progress = 500;
    draws = Draw(look, {dwelling});
    Assert::AreEqual(rest.y - depth * 0.5, MeshBatchOf(draws, Species::SceneMesh::Dwelling)->instances.front().position.y, 1e-9,
                     L"halfway built, half risen");

    dwelling.state.built = true;
    draws = Draw(look, {dwelling});
    const Neuron::SceneMeshInstance& built = MeshBatchOf(draws, Species::SceneMesh::Dwelling)->instances.front();
    Assert::AreEqual(rest.y, built.position.y, 1e-9, L"built: at its place");
    Assert::AreEqual(1.0f, built.brightness, 1e-6f, L"and lit");

    dwelling.state.active = false;
    draws = Draw(look, {dwelling});
    Assert::AreEqual(static_cast<float>(presentation.mvp.structure.inactiveBrightness),
                     MeshBatchOf(draws, Species::SceneMesh::Dwelling)->instances.front().brightness, 1e-6f, L"inactive: dark");

    // A resource site belongs to no team (WLD-084): untinted.
    const Species::InterpolatedStructure site = StructureAt(Species::StructureKind::ResourceSite, std::nullopt, ground);
    draws = Draw(look, {site});
    Assert::AreEqual(0.0f, MeshBatchOf(draws, Species::SceneMesh::ResourceSite)->instances.front().tintWeight, 1e-6f, L"no team, no tint");
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg046ADwellingsPortsShowOnlyWhileItIsASite)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-046")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-017")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-168")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-083")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg046ADwellingsPortsShowOnlyWhileItIsASite)
  {
    const Species::TuningPresentation& presentation = CommittedPresentation();
    const auto& port = presentation.ref.bldg.port;
    const Neuron::Vec3 ground = Ground();
    Species::StructureLook look(presentation);
    Species::InterpolatedStructure dwelling = StructureAt(Species::StructureKind::Dwelling, std::uint8_t{0}, ground);
    dwelling.state.built = false;
    dwelling.state.mannedPorts = 0b01;
    Species::StructureDraws draws = Draw(look, {dwelling});

    const Neuron::MeshBatch* pads = MeshBatchOf(draws, Species::SceneMesh::PortPad);
    Assert::IsTrue(pads != nullptr && pads->instances.size() == 2, L"a pad for each port");
    Assert::IsTrue(pads->pass == Neuron::ScenePass::StructureAlpha && pads->entry == "port_pad", L"in pass 8");
    const Neuron::Vec3 rest = Species::PositionOf(dwelling.state.position);
    const Neuron::Vec3& first = pads->instances[0].position;
    Assert::IsTrue(std::abs(first.x - (rest.x + PORT_OFFSET)) < 1e-9 && std::abs(first.z - rest.z) < 1e-9,
                   L"port 0 at its socket, the dwelling facing +z");
    Assert::AreEqual(SeedOneTerrain().field.Height(first.x, first.z) + port.padHeightWu, first.y, 1e-9, L"on the terrain, raised");

    const Neuron::QuadBatch* stars = QuadBatchOf(draws, Species::SceneTexture::Starburst);
    Assert::IsTrue(stars != nullptr && stars->quads.size() == 2, L"a star for each port, and no light on a site");
    Assert::IsTrue(stars->pass == Neuron::ScenePass::StructureAlpha && stars->entry == "glow_additive_fog", L"additive, fogged");
    const Neuron::SceneQuad& manned = stars->quads[0];
    const Neuron::SceneQuad& empty = stars->quads[1];
    Assert::AreEqual(static_cast<float>(port.statusStarSizeWu * 0.5), manned.halfWidth, 1e-6f, L"the star's size across");
    Assert::IsTrue(Near(manned.position, first + Neuron::Vec3{0.0, PAD_STATUS_HEIGHT, 0.0}), L"at the pad's status socket");
    Assert::AreEqual(static_cast<float>(port.statusOccupiedRgb.g), manned.color[1], 1e-6f, L"port 0 is manned: green");
    Assert::AreEqual(static_cast<float>(port.statusOccupiedRgb.r), manned.color[0], 1e-6f, L"green's red");
    Assert::AreEqual(static_cast<float>(port.statusEmptyRgb.r), empty.color[0], 1e-6f, L"port 1 is empty: red");
    Assert::AreEqual(static_cast<float>(port.statusEmptyRgb.g), empty.color[1], 1e-6f, L"red's green");

    // O89: a built dwelling's construction ports are gone.
    dwelling.state.built = true;
    draws = Draw(look, {dwelling}, 4.0); // a frame without team 0's lights
    Assert::IsTrue(MeshBatchOf(draws, Species::SceneMesh::PortPad) == nullptr, L"no pads once built");
    ExpectEqual(0, static_cast<std::int64_t>(QuadCount(draws)), L"no stars once built");

    // A resource site's gathering ports stay, untinted.
    Species::InterpolatedStructure site = StructureAt(Species::StructureKind::ResourceSite, std::nullopt, ground);
    site.state.mannedPorts = 0b10;
    draws = Draw(look, {site});
    Assert::IsTrue(MeshBatchOf(draws, Species::SceneMesh::PortPad)->instances.size() == 2, L"a site's ports are always drawn");
    Assert::AreEqual(0.0f, MeshBatchOf(draws, Species::SceneMesh::PortPad)->instances[0].tintWeight, 1e-6f, L"untinted");
    Assert::AreEqual(static_cast<float>(port.statusOccupiedRgb.g), QuadBatchOf(draws, Species::SceneTexture::Starburst)->quads[1].color[1],
                     1e-6f, L"its port 1 manned");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd167LightsShowInTheirTeamsCommandFrames)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-167")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-045")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-035")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd167LightsShowInTheirTeamsCommandFrames)
  {
    const Species::TuningPresentation& presentation = CommittedPresentation();
    const auto& lights = presentation.ref.bldg.structure;
    const Neuron::Vec3 ground = Ground();
    Species::StructureLook look(presentation);
    // The world_renderers.md test T11: for frames 0 to 19, each team's lights in its pair of frames, and team 5's never.
    for (std::uint8_t team = 0; team <= 5; ++team)
    {
      const Species::InterpolatedStructure storehouse = StructureAt(Species::StructureKind::Storehouse, team, ground);
      for (std::int64_t frame = 0; frame < 2 * lights.lightCycleTicks; ++frame)
      {
        const bool on = (frame % lights.lightCycleTicks) / lights.lightTicksPerTeam == team;
        const Species::StructureDraws draws = Draw(look, {storehouse}, static_cast<double>(frame) + 0.5);
        ExpectEqual(on ? lights.lightQuads - 1 : 0, static_cast<std::int64_t>(QuadCount(draws)),
                    std::format(L"team {} in frame {}", team, frame));
      }
    }
    const std::int64_t teamOneFrame = lights.lightTicksPerTeam;
    Species::InterpolatedStructure storehouse = StructureAt(Species::StructureKind::Storehouse, std::uint8_t{1}, ground);
    Species::StructureDraws draws = Draw(look, {storehouse}, static_cast<double>(teamOneFrame));
    const Neuron::QuadBatch* quads = QuadBatchOf(draws, Species::SceneTexture::Starburst);
    Assert::IsTrue(quads != nullptr && quads->quads.size() == static_cast<std::size_t>(lights.lightQuads - 1), L"nested quads");
    const Neuron::Vec3 light = Species::PositionOf(storehouse.state.position) + Neuron::Vec3{0.0, LIGHT_HEIGHT, 0.0};
    Assert::IsTrue(Near(quads->quads.front().position, light), L"at the light socket");
    Assert::AreEqual(static_cast<float>(lights.lightSizeWu / static_cast<double>(lights.lightQuads)), quads->quads.front().halfWidth, 1e-6f,
                     L"quad 1 of a tenth of the size");
    Assert::IsTrue(quads->quads.back().color == Species::TeamColor(1), L"in the team colour");

    storehouse.state.active = false;
    ExpectEqual(0, static_cast<std::int64_t>(QuadCount(Draw(look, {storehouse}, static_cast<double>(teamOneFrame)))),
                L"an inactive structure is dark");
    storehouse.state.active = true;
    storehouse.state.built = false;
    const Species::StructureDraws site = Draw(look, {storehouse}, static_cast<double>(teamOneFrame));
    ExpectEqual(2, static_cast<std::int64_t>(QuadCount(site)), L"a site shows its ports' stars and no lights");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd170ATurretsHeadAndBarrelsFollowItsAim)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-170")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-428")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-046")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd170ATurretsHeadAndBarrelsFollowItsAim)
  {
    const Species::TuningPresentation& presentation = CommittedPresentation();
    const Neuron::Vec3 ground = Ground();
    Species::StructureLook look(presentation);
    Species::InterpolatedStructure turret = StructureAt(Species::StructureKind::Turret, std::uint8_t{0}, ground);
    turret.state.turretTeam = std::uint8_t{3};
    turret.state.mannedPorts = 0b1;
    turret.aimYawRadians = std::numbers::pi / 2.0; // facing +x
    const double pitch = 0.25;
    turret.aimPitchRadians = pitch;
    const Species::StructureDraws draws = Draw(look, {turret}, 4.0);
    const Neuron::Vec3 rest = Species::PositionOf(turret.state.position);

    Assert::IsTrue(MeshBatchOf(draws, Species::SceneMesh::TurretMount) == nullptr, L"the mount is sockets only");
    const Neuron::MeshBatch* heads = MeshBatchOf(draws, Species::SceneMesh::TurretHead);
    Assert::IsTrue(heads != nullptr && heads->instances.size() == 1, L"one head");
    const Neuron::SceneMeshInstance& head = heads->instances.front();
    Assert::IsTrue(Near(head.axisZ, {1.0, 0.0, 0.0}) && Near(head.axisX, {0.0, 0.0, -1.0}), L"turned to its aim");
    Assert::IsTrue(head.tint == TintOf(3), L"tinted by the team that mans it");
    Assert::AreEqual(static_cast<float>(presentation.ref.bldg.turret.tintWeight), head.tintWeight, 1e-6f, L"by the turret's weight");

    const Neuron::SceneMeshInstance& barrels = MeshBatchOf(draws, Species::SceneMesh::TurretBarrelGun)->instances.front();
    Assert::IsTrue(Near(barrels.position, rest + Neuron::Vec3{BARREL_MOUNT.z, BARREL_MOUNT.y, -BARREL_MOUNT.x}), L"at the barrel mount");
    Assert::IsTrue(Near(barrels.axisY, {std::sin(pitch), std::cos(pitch), 0.0}), L"tilted towards the head's front");
    Assert::IsTrue(Near(barrels.axisZ, {std::cos(pitch), -std::sin(pitch), 0.0}), L"its front lowered with it");
    Assert::AreEqual(0.0f, barrels.tintWeight, 1e-6f, L"the barrels untinted");

    const Neuron::QuadBatch* stars = QuadBatchOf(draws, Species::SceneTexture::Starburst);
    Assert::IsTrue(stars != nullptr && stars->quads.size() == 1, L"the mount's one port");
    Assert::IsTrue(Near(stars->quads.front().position, rest + TURRET_STATUS), L"its star at the mount's status socket");

    turret.state.turretTeam.reset();
    Assert::AreEqual(0.0f, MeshBatchOf(Draw(look, {turret}, 4.0), Species::SceneMesh::TurretHead)->instances.front().tintWeight, 1e-6f,
                     L"no team: untinted");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp083ACompletedStructureFlashesOnce)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-083")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp083ACompletedStructureFlashesOnce)
  {
    const Species::TuningPresentation& presentation = CommittedPresentation();
    const auto& structure = presentation.mvp.structure;
    const double flashTicks = structure.flashSeconds / TICK_SECONDS;
    Species::StructureLook look(presentation);
    Species::ReceivedEvent completed;
    completed.tick = 10;
    completed.event.kind = Species::EffectEventKind::StructureCompleted;
    completed.event.team = 1;
    completed.event.position = Ground();
    look.Update(std::vector{completed});

    ExpectEqual(0, static_cast<std::int64_t>(QuadCount(Draw(look, {}, 8.5))), L"not before its tick");
    Species::StructureDraws draws = Draw(look, {}, 9.0);
    const Neuron::QuadBatch* flash = QuadBatchOf(draws, Species::SceneTexture::Starburst);
    Assert::IsTrue(flash != nullptr && flash->quads.size() == 1, L"one flash as the display leaves tick 9");
    Assert::AreEqual(static_cast<float>(structure.flashSize), flash->quads.front().halfWidth, 1e-6f, L"its size");
    Assert::AreEqual(1.0f, flash->quads.front().color[3], 1e-6f, L"at full strength");
    Assert::AreEqual(Species::TeamColor(1)[0], flash->quads.front().color[0], 1e-6f, L"in its team's colour");
    draws = Draw(look, {}, 9.0 + flashTicks * 0.5);
    Assert::AreEqual(0.5f, QuadBatchOf(draws, Species::SceneTexture::Starburst)->quads.front().color[3], 1e-6f, L"faded by half");
    ExpectEqual(0, static_cast<std::int64_t>(QuadCount(Draw(look, {}, 9.0 + flashTicks))), L"gone at its end");
    ExpectEqual(0, static_cast<std::int64_t>(QuadCount(Draw(look, {}, 9.0))), L"and once only");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp083TheWellspringsGlowCirclesItsCrystal)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-083")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-025")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp083TheWellspringsGlowCirclesItsCrystal)
  {
    const Species::TuningPresentation& presentation = CommittedPresentation();
    const auto& wellspring = presentation.mvp.wellspring;
    Species::StructureLook look(presentation);
    const Species::InterpolatedStructure structure = StructureAt(Species::StructureKind::Wellspring, std::uint8_t{2}, Ground());
    const Species::StructureDraws draws = Draw(look, {structure}, 4.0);
    const Neuron::QuadBatch* glows = QuadBatchOf(draws, Species::SceneTexture::GlowCloudy);
    Assert::IsTrue(glows != nullptr && glows->quads.size() == static_cast<std::size_t>(wellspring.glowBlobs), L"its blobs");
    Assert::IsTrue(MeshBatchOf(draws, Species::SceneMesh::PortPad) == nullptr, L"and no ports, which serve no task at M2");
    const Neuron::Vec3 core = Species::PositionOf(structure.state.position) + Neuron::Vec3{0.0, CORE_HEIGHT, 0.0};
    for (const Neuron::SceneQuad& blob : glows->quads)
    {
      Assert::AreEqual(core.y, blob.position.y, 1e-9, L"at the crystal's height");
      Assert::AreEqual(wellspring.glowRadius, Neuron::Length(blob.position - core), 1e-9, L"on its circle");
      Assert::AreEqual(static_cast<float>(wellspring.glowAlpha), blob.color[3], 1e-6f, L"its alpha");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl366ThePlacementGhostIsItsStructureInPass12)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-366")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-365")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl366ThePlacementGhostIsItsStructureInPass12)
  {
    static const Neuron::SceneMeshes MESHES = Meshes();
    std::vector<Neuron::MeshBatch> batches;
    const Species::StructureGhost dwelling{Species::StructureKind::Dwelling, Ground(), std::numbers::pi * 0.5, 2};
    Species::AppendGhost(dwelling, MESHES, CommittedPresentation(), batches);
    Assert::IsTrue(batches.size() == 1 && batches.front().instances.size() == 1, L"one ghost");
    const Neuron::MeshBatch& ghost = batches.front();
    Assert::IsTrue(ghost.pass == Neuron::ScenePass::Routes && ghost.entry == "cursor_ghost", L"in pass 12 with the ghost blend (ADR-153)");
    Assert::IsTrue(ghost.mesh == static_cast<std::uint16_t>(Species::SceneMesh::Dwelling), L"the dwelling's mesh");
    const Neuron::SceneMeshInstance& instance = ghost.instances.front();
    Assert::IsTrue(Near(instance.position, Ground()) && Near(instance.axisZ, {1.0, 0.0, 0.0}), L"at the pointer, facing its yaw");
    Assert::IsTrue(instance.tint == TintOf(2) && instance.tintWeight == 1.0f, L"in its team's colour");
    Assert::AreEqual(CommittedPresentation().mvp.structure.ghostBrightness, static_cast<double>(instance.brightness), 1e-6,
                     L"at mvp.structure.ghost_brightness");
    batches.clear();
    Species::AppendGhost({Species::StructureKind::Turret, Ground(), 0.0, 2}, MESHES, CommittedPresentation(), batches);
    Assert::IsTrue(batches.size() == 1 && batches.front().mesh == static_cast<std::uint16_t>(Species::SceneMesh::TurretHead),
                   L"a turret's ghost is its head");
    batches.clear();
    Species::AppendGhost(dwelling, Neuron::SceneMeshes{}, CommittedPresentation(), batches);
    Assert::IsTrue(batches.empty(), L"no meshes, no ghost");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg416ATurretsAimTurnsTheShorterWayRound)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-416")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg416ATurretsAimTurnsTheShorterWayRound)
  {
    Species::StateReplica replica;
    Species::ReplicatedStructure turret;
    turret.kind = Species::StructureKind::Turret;
    turret.aimYaw = static_cast<std::uint16_t>(Species::YAW_STEPS - 24);
    turret.aimPitch = 10;
    const Species::StableId id = Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, 7);
    replica.Apply({1, 0, {RecordOf(id, turret, nullptr)}});
    const Species::ReplicatedState before{turret};
    turret.aimYaw = 24;
    turret.aimPitch = 30;
    replica.Apply({2, 1, {RecordOf(id, turret, &before)}});
    const std::vector<Species::InterpolatedStructure> drawn = Species::InterpolateStructures(replica, 0.5);
    Assert::IsTrue(drawn.size() == 1 && drawn[0].id == id, L"the turret");
    Assert::AreEqual(0.0, std::remainder(drawn[0].aimYawRadians, 2.0 * std::numbers::pi), 1e-9, L"through its front, not its back");
    Assert::AreEqual(20.0 * 2.0 * std::numbers::pi / Species::YAW_STEPS, drawn[0].aimPitchRadians, 1e-9, L"the tilt halfway");
    Assert::IsTrue(drawn[0].state == turret, L"its latest state");
  }
};

} // namespace GameClientTests
