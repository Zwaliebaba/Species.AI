#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "MeshAsset.h"
#include "StructureLook.h"
#include "TuningPresentation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string_view>
#include <utility>

namespace Species
{

namespace
{

constexpr Neuron::Vec3 WORLD_UP{0.0, 1.0, 0.0};
// RND-163, RND-168 (ADR-140): the catalogue entries of the structures' passes.
constexpr std::string_view SOLID_ENTRY = "mesh_lit_fog";
constexpr std::string_view PAD_ENTRY = "port_pad";
constexpr std::string_view GLOW_ENTRY = "glow_additive_fog";
constexpr std::string_view GHOST_ENTRY = "cursor_ghost"; // ADR-153
// The node of the Wellspring's floating crystal (ADR-128), which its cloudy glow circles.
constexpr std::string_view WELLSPRING_CORE_NODE = "wellspring_core";
constexpr double PERMILLE = 1'000.0;

/// A structure's axes in the world (ADR-130): its mesh's +z along its front, +y up and +x to the front's right.
struct Axes
{
  Neuron::Vec3 origin;
  Neuron::Vec3 right{1.0, 0.0, 0.0};
  Neuron::Vec3 up{0.0, 1.0, 0.0};
  Neuron::Vec3 front{0.0, 0.0, 1.0};

  // A socket's point, its transform's translation in the mesh (every node rests at the identity; ADR-128).
  [[nodiscard]] Neuron::Vec3 Point(const std::array<float, 16>& _transform) const noexcept
  {
    return origin + right * static_cast<double>(_transform[12]) + up * static_cast<double>(_transform[13]) +
           front * static_cast<double>(_transform[14]);
  }
};

Neuron::Vec3 Cross(const Neuron::Vec3& _a, const Neuron::Vec3& _b) noexcept
{
  return {_a.y * _b.z - _a.z * _b.y, _a.z * _b.x - _a.x * _b.z, _a.x * _b.y - _a.y * _b.x};
}

// A heading in the world: yaw 0 faces +z and a quarter turn faces +x (ADR-111).
Axes AxesOf(const Neuron::Vec3& _origin, double _yawRadians) noexcept
{
  const double s = std::sin(_yawRadians);
  const double c = std::cos(_yawRadians);
  return {_origin, {c, 0.0, -s}, WORLD_UP, {s, 0.0, c}};
}

Neuron::SceneMeshInstance InstanceOf(const Axes& _axes) noexcept
{
  Neuron::SceneMeshInstance instance;
  instance.position = _axes.origin;
  instance.axisX = _axes.right;
  instance.axisY = _axes.up;
  instance.axisZ = _axes.front;
  return instance;
}

SceneMesh MeshOf(StructureKind _kind) noexcept
{
  switch (_kind)
  {
  case StructureKind::Wellspring:
    return SceneMesh::Wellspring;
  case StructureKind::ResourceSite:
    return SceneMesh::ResourceSite;
  case StructureKind::ResourceSiteCentral:
    return SceneMesh::ResourceSiteCentral;
  case StructureKind::Dwelling:
    return SceneMesh::Dwelling;
  case StructureKind::Storehouse:
    return SceneMesh::Storehouse;
  case StructureKind::Turret:
    break;
  }
  return SceneMesh::TurretMount;
}

// The vertical extent of a mesh's drawn nodes, which a construction site rises through.
double HeightOf(const Neuron::MeshAsset& _mesh) noexcept
{
  double low = 0.0;
  double high = 0.0;
  bool any = false;
  for (const Neuron::MeshNode& node : _mesh.nodes)
  {
    if (node.draws.empty())
    {
      continue;
    }
    low = any ? std::min(low, static_cast<double>(node.boundsMin[1])) : static_cast<double>(node.boundsMin[1]);
    high = any ? std::max(high, static_cast<double>(node.boundsMax[1])) : static_cast<double>(node.boundsMax[1]);
    any = true;
  }
  return any ? high - low : 0.0;
}

// The radius about a mesh's origin that holds every node's bounding sphere: the view test's.
float RadiusOf(const Neuron::MeshAsset& _mesh) noexcept
{
  double radius = 0.0;
  for (const Neuron::MeshNode& node : _mesh.nodes)
  {
    const Neuron::Vec3 center{(node.boundsMin[0] + node.boundsMax[0]) * 0.5, (node.boundsMin[1] + node.boundsMax[1]) * 0.5,
                              (node.boundsMin[2] + node.boundsMax[2]) * 0.5};
    radius = std::max(radius, Neuron::Length(center) + static_cast<double>(node.sphereRadius));
  }
  return static_cast<float>(radius);
}

// A mesh's sockets of one kind in the order of their indices.
std::vector<const Neuron::MeshSocket*> SocketsOf(const Neuron::MeshAsset& _mesh, Neuron::SocketKind _kind)
{
  std::vector<const Neuron::MeshSocket*> sockets;
  for (const Neuron::MeshSocket& socket : _mesh.sockets)
  {
    if (socket.kind == _kind)
    {
      sockets.push_back(&socket);
    }
  }
  std::ranges::sort(sockets, {}, &Neuron::MeshSocket::index);
  return sockets;
}

// The height above its origin of a mesh's named node's centre; the mesh's half height without that node.
double NodeCenterHeight(const Neuron::MeshAsset& _mesh, std::string_view _name) noexcept
{
  const auto node = std::ranges::find(_mesh.nodes, _name, &Neuron::MeshNode::name);
  return node != _mesh.nodes.end() ? (node->boundsMin[1] + node->boundsMax[1]) * 0.5 : HeightOf(_mesh) * 0.5;
}

std::array<float, 3> Rgb(const std::array<float, 4>& _color, double _factor) noexcept
{
  return {static_cast<float>(_color[0] * _factor), static_cast<float>(_color[1] * _factor), static_cast<float>(_color[2] * _factor)};
}

std::array<float, 4> StarColor(double _red, double _green, double _blue) noexcept
{
  return {static_cast<float>(_red), static_cast<float>(_green), static_cast<float>(_blue), 1.0f};
}

Neuron::SceneQuad GlowQuad(const Neuron::Vec3& _position, double _half, const std::array<float, 4>& _color) noexcept
{
  Neuron::SceneQuad quad;
  quad.position = _position;
  quad.halfWidth = static_cast<float>(_half);
  quad.halfHeight = quad.halfWidth;
  quad.color = _color;
  quad.endColor = _color;
  return quad;
}

} // namespace

void AppendGhost(const StructureGhost& _ghost, const Neuron::SceneMeshes& _meshes, const TuningPresentation& _presentation,
                 std::vector<Neuron::MeshBatch>& _out)
{
  if (_meshes.meshes.size() < static_cast<std::size_t>(SceneMesh::Count))
  {
    return;
  }
  // R-CTRL-366 row 1b: a turret's ghost is its head (mvp_assets.md), which is the turret drawn on the ground at M2.
  const SceneMesh mesh = _ghost.kind == StructureKind::Turret ? SceneMesh::TurretHead : MeshOf(_ghost.kind);
  Neuron::MeshBatch batch;
  batch.pass = Neuron::ScenePass::Routes;
  batch.entry = GHOST_ENTRY;
  batch.mesh = static_cast<std::uint16_t>(mesh);
  batch.cullRadius = RadiusOf(*_meshes.meshes[static_cast<std::size_t>(mesh)]);
  Neuron::SceneMeshInstance instance = InstanceOf(AxesOf(_ghost.position, _ghost.yawRadians));
  instance.tint = Rgb(TeamColor(_ghost.team), _presentation.rnd.tint.factor);
  instance.tintWeight = 1.0f;
  instance.brightness = static_cast<float>(_presentation.mvp.structure.ghostBrightness);
  batch.instances.push_back(instance);
  _out.push_back(std::move(batch));
}

StructureLook::StructureLook(const TuningPresentation& _presentation) noexcept
  : m_presentation(_presentation)
{
}

void StructureLook::Update(std::span<const ReceivedEvent> _events)
{
  for (const ReceivedEvent& received : _events)
  {
    if (received.event.kind == EffectEventKind::StructureCompleted)
    {
      // ADR-123: an event of tick t happens as the display leaves t − 1.
      m_flashes.push_back({received.event.position, received.event.team, static_cast<double>(received.tick) - 1.0});
    }
  }
}

void StructureLook::Clear() noexcept
{
  m_flashes.clear();
}

void StructureLook::Draw(std::span<const InterpolatedStructure> _structures, const Neuron::SceneMeshes& _meshes,
                         const Neuron::TerrainField& _field, const StructureView& _view, StructureDraws& _draws)
{
  if (_meshes.meshes.size() < static_cast<std::size_t>(SceneMesh::Count))
  {
    return;
  }
  const auto meshOf = [&_meshes](SceneMesh _mesh) -> const Neuron::MeshAsset& { return *_meshes.meshes[static_cast<std::size_t>(_mesh)]; };
  const auto& port = m_presentation.ref.bldg.port;
  const auto& lights = m_presentation.ref.bldg.structure;
  const auto& look = m_presentation.mvp.structure;
  const auto& wellspring = m_presentation.mvp.wellspring;
  const double tintFactor = m_presentation.rnd.tint.factor;

  // A solid batch for each structure mesh, and the pads' batch; each made as its first instance needs it.
  std::array<Neuron::MeshBatch, static_cast<std::size_t>(SceneMesh::Count)> solids;
  for (std::size_t mesh = 0; mesh < solids.size(); ++mesh)
  {
    solids[mesh].pass = Neuron::ScenePass::Structures;
    solids[mesh].entry = SOLID_ENTRY;
    solids[mesh].mesh = static_cast<std::uint16_t>(mesh);
    solids[mesh].cullRadius = RadiusOf(*_meshes.meshes[mesh]);
  }
  Neuron::MeshBatch pads;
  pads.pass = Neuron::ScenePass::StructureAlpha;
  pads.entry = PAD_ENTRY;
  pads.mesh = static_cast<std::uint16_t>(SceneMesh::PortPad);
  pads.cullRadius = RadiusOf(meshOf(SceneMesh::PortPad));
  const auto quadBatch = [](SceneTexture _texture, double _cullRadius)
  {
    Neuron::QuadBatch batch;
    batch.pass = Neuron::ScenePass::StructureAlpha;
    batch.entry = GLOW_ENTRY;
    batch.texture = static_cast<std::uint16_t>(_texture);
    batch.cullRadius = static_cast<float>(_cullRadius);
    return batch;
  };
  Neuron::QuadBatch lightQuads = quadBatch(SceneTexture::Starburst, lights.lightSizeWu);
  Neuron::QuadBatch stars = quadBatch(SceneTexture::Starburst, port.statusStarSizeWu);
  Neuron::QuadBatch glows = quadBatch(SceneTexture::GlowCloudy, wellspring.glowRadius + wellspring.glowSize);
  Neuron::QuadBatch flashes = quadBatch(SceneTexture::Starburst, look.flashSize);

  const std::vector<const Neuron::MeshSocket*> padStatus = SocketsOf(meshOf(SceneMesh::PortPad), Neuron::SocketKind::Status);
  const double padStatusHeight = padStatus.empty() ? 0.0 : static_cast<double>(padStatus.front()->transform[13]);
  // RND-167 (ADR-140): the command frame n is the display tick's, the older of the two states drawn (ADR-084).
  const auto lightTick = static_cast<std::int64_t>(std::floor(std::max(_view.displayTick, 0.0)));
  const std::int64_t cycle = std::max<std::int64_t>(lights.lightCycleTicks, 1);
  const std::int64_t perTeam = std::max<std::int64_t>(lights.lightTicksPerTeam, 1);
  const std::int64_t lightTeam = (lightTick % cycle) / perTeam;

  for (const InterpolatedStructure& structure : _structures)
  {
    const ReplicatedStructure& state = structure.state;
    const SceneMesh meshKind = MeshOf(state.kind);
    const bool turret = state.kind == StructureKind::Turret;
    const Neuron::MeshAsset& mesh = meshOf(meshKind);
    const Axes rest = AxesOf(PositionOf(state.position), YawRadians(state.yaw));
    // MVP-033, MVP-083: a site is sunk by the share of its work still to come; MVP-035: an inactive structure is dark.
    const double height = HeightOf(turret ? meshOf(SceneMesh::TurretHead) : mesh);
    const double remaining = state.built ? 0.0 : 1.0 - std::clamp(static_cast<double>(state.progress) / PERMILLE, 0.0, 1.0);
    Axes drawn = rest;
    drawn.origin = rest.origin - WORLD_UP * (remaining * height * look.riseDepthFactor);
    const float brightness = state.built && !state.active ? static_cast<float>(look.inactiveBrightness) : 1.0f;
    const std::array<float, 4> team = state.owner ? TeamColor(*state.owner) : std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f};
    const float ownerWeight = state.owner ? 1.0f : 0.0f;

    if (!turret)
    {
      Neuron::SceneMeshInstance body = InstanceOf(drawn);
      body.tint = Rgb(team, tintFactor);
      body.tintWeight = ownerWeight;
      body.brightness = brightness;
      solids[static_cast<std::size_t>(meshKind)].instances.push_back(body);
    }
    else
    {
      // RND-170, R-BLDG-416, R-BLDG-428: the head turned to its aim and tinted by the team that mans it, and the gun
      // barrels at its barrel mount, their up vector tilted towards the head's front.
      const Axes head = AxesOf(drawn.origin, structure.aimYawRadians);
      const std::array<float, 4> crew = state.turretTeam ? TeamColor(*state.turretTeam) : std::array<float, 4>{1.0f, 1.0f, 1.0f, 1.0f};
      Neuron::SceneMeshInstance headInstance = InstanceOf(head);
      headInstance.tint = Rgb(crew, tintFactor);
      headInstance.tintWeight = state.turretTeam ? static_cast<float>(m_presentation.ref.bldg.turret.tintWeight) : 0.0f;
      headInstance.brightness = brightness;
      solids[static_cast<std::size_t>(SceneMesh::TurretHead)].instances.push_back(headInstance);
      const std::vector<const Neuron::MeshSocket*> mount = SocketsOf(meshOf(SceneMesh::TurretHead), Neuron::SocketKind::BarrelMount);
      Axes barrels = head;
      barrels.origin = mount.empty() ? head.origin : head.Point(mount.front()->transform);
      barrels.up = head.up * std::cos(structure.aimPitchRadians) + head.front * std::sin(structure.aimPitchRadians);
      barrels.front = Neuron::NormalizedOr(Cross(head.right, barrels.up), head.front);
      Neuron::SceneMeshInstance barrelInstance = InstanceOf(barrels);
      barrelInstance.brightness = brightness;
      solids[static_cast<std::size_t>(SceneMesh::TurretBarrelGun)].instances.push_back(barrelInstance);
    }

    // R-BLDG-046 (O89): the ports' pads and stars, a dwelling's and a storehouse's only while it is a site. The Wellspring's
    // ports serve no task at M2 (ADR-140), so it draws none.
    const bool constructionPorts = state.kind == StructureKind::Dwelling || state.kind == StructureKind::Storehouse;
    if (state.kind != StructureKind::Wellspring && (!constructionPorts || !state.built))
    {
      const std::vector<const Neuron::MeshSocket*> ports = SocketsOf(mesh, Neuron::SocketKind::Port);
      const std::vector<const Neuron::MeshSocket*> statuses = SocketsOf(mesh, Neuron::SocketKind::Status);
      for (std::size_t n = 0; n < ports.size(); ++n)
      {
        // R-BLDG-020: at its socket's place, on the terrain.
        Neuron::Vec3 at = rest.Point(ports[n]->transform);
        at.y = _field.Height(at.x, at.z) + port.padHeightWu;
        Axes pad = rest;
        pad.origin = at;
        Neuron::SceneMeshInstance padInstance = InstanceOf(pad);
        padInstance.tint = Rgb(team, tintFactor);
        padInstance.tintWeight = ownerWeight;
        pads.instances.push_back(padInstance);
        const bool manned = ports[n]->index < STRUCTURE_PORTS_MAX && (state.mannedPorts & (1u << ports[n]->index)) != 0;
        const std::array<float, 4> starColor = manned
                                                 ? StarColor(port.statusOccupiedRgb.r, port.statusOccupiedRgb.g, port.statusOccupiedRgb.b)
                                                 : StarColor(port.statusEmptyRgb.r, port.statusEmptyRgb.g, port.statusEmptyRgb.b);
        // RND-170: a turret's star of port i stands at its mount's status socket i.
        const auto status = std::ranges::find(statuses, ports[n]->index, &Neuron::MeshSocket::index);
        const Neuron::Vec3 star = turret && status != statuses.end() ? rest.Point((*status)->transform) : at + WORLD_UP * padStatusHeight;
        stars.quads.push_back(GlowQuad(star, port.statusStarSizeWu * 0.5, starColor));
      }
    }

    // RND-167: a built, active structure of a team shows its lights in its team's command frames.
    if (state.owner && state.built && state.active && lightTeam == std::int64_t{*state.owner})
    {
      const auto quads = std::max<std::int64_t>(lights.lightQuads, 0);
      for (const Neuron::MeshSocket* light : SocketsOf(mesh, Neuron::SocketKind::Light))
      {
        const Neuron::Vec3 at = drawn.Point(light->transform);
        // Quad 0 has no size, so it is not drawn.
        for (std::int64_t quad = 1; quad < quads; ++quad)
        {
          lightQuads.quads.push_back(GlowQuad(at, lights.lightSizeWu * static_cast<double>(quad) / static_cast<double>(quads), team));
        }
      }
    }

    // ADR-140 (O77): the Wellspring's cloudy glow, its blobs circling its crystal.
    if (state.kind == StructureKind::Wellspring)
    {
      const auto blobs = std::max<std::int64_t>(wellspring.glowBlobs, 0);
      const Neuron::Vec3 center = drawn.origin + WORLD_UP * NodeCenterHeight(mesh, WELLSPRING_CORE_NODE);
      const double turn = wellspring.glowTurnSeconds > 0.0 ? 2.0 * std::numbers::pi * _view.realSeconds / wellspring.glowTurnSeconds : 0.0;
      for (std::int64_t blob = 0; blob < blobs; ++blob)
      {
        const double angle = turn + 2.0 * std::numbers::pi * static_cast<double>(blob) / static_cast<double>(blobs);
        const Neuron::Vec3 at =
          center + drawn.right * (wellspring.glowRadius * std::cos(angle)) + drawn.front * (wellspring.glowRadius * std::sin(angle));
        glows.quads.push_back(
          GlowQuad(at, wellspring.glowSize, {team[0], team[1], team[2], static_cast<float>(wellspring.glowAlpha * brightness)}));
      }
    }
  }

  // MVP-083: each completed structure flashes once, fading over mvp.structure.flash_seconds from its event.
  const double flashTicks = _view.tickSeconds > 0.0 ? look.flashSeconds / _view.tickSeconds : 0.0;
  std::erase_if(m_flashes, [&](const Flash& _flash) { return _view.displayTick >= _flash.startTick + flashTicks; });
  for (const Flash& flash : m_flashes)
  {
    if (_view.displayTick < flash.startTick || flashTicks <= 0.0)
    {
      continue;
    }
    const double fade = 1.0 - (_view.displayTick - flash.startTick) / flashTicks;
    const std::array<float, 4> team = TeamColor(flash.team);
    flashes.quads.push_back(GlowQuad(flash.position + WORLD_UP * (look.flashSize * 0.5), look.flashSize,
                                     {team[0], team[1], team[2], static_cast<float>(std::clamp(fade, 0.0, 1.0))}));
  }

  for (Neuron::MeshBatch& solid : solids)
  {
    if (!solid.instances.empty())
    {
      _draws.meshes.push_back(std::move(solid));
    }
  }
  if (!pads.instances.empty())
  {
    _draws.meshes.push_back(std::move(pads));
  }
  // RND-164, RND-165: the lights, then the ports' stars, then the glows.
  for (Neuron::QuadBatch* batch : {&lightQuads, &stars, &glows, &flashes})
  {
    if (!batch->quads.empty())
    {
      _draws.quads.push_back(std::move(*batch));
    }
  }
}

} // namespace Species
