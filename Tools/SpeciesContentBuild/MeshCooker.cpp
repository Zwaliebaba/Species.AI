#include "pch.h"

#include "CmoFile.h"
#include "MeshAsset.h"
#include "MeshCooker.h"
#include "RowSpec.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ContentBuild
{

namespace
{

constexpr std::array<float, 16> IDENTITY{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

/// What a mesh row's `spec` asks for (AST-040, AST-076): the triangle budget, the node count and depth, and the
/// sockets by their names after the prefix, each with the node it must sit on when the row names one.
struct MeshSpec
{
  std::uint64_t triangleBudget = 0;
  std::uint64_t nodes = 0;
  std::uint64_t depth = 0;
  std::map<std::string, std::string> sockets; // name → its node, or empty
};

// Reads a row's spec into _outSpec, filled in place since a MeshSpec is never moved; false when the spec states no
// triangle budget, node count, depth or socket list.
bool ReadSpec(std::string_view _spec, MeshSpec& _outSpec)
{
  const std::optional<std::uint64_t> budget = NumberAfter(_spec, "<= ");
  const std::optional<std::uint64_t> nodes = NumberBefore(_spec, " node");
  const std::optional<std::uint64_t> depth = NumberAfter(_spec, "depth ");
  const std::size_t clause = _spec.find("sockets: ");
  if (!budget || _spec.find("triangles budget") == std::string_view::npos || !nodes || !depth || clause == std::string_view::npos)
  {
    return false;
  }
  _outSpec.triangleBudget = budget.value_or(0);
  _outSpec.nodes = nodes.value_or(0);
  _outSpec.depth = depth.value_or(0);
  std::string_view list = _spec.substr(clause + std::string_view{"sockets: "}.size());
  list = list.substr(0, list.find(';'));
  if (list == "none")
  {
    return true;
  }
  while (!list.empty())
  {
    const std::size_t comma = list.find(", ");
    std::string_view item = list.substr(0, comma);
    list = comma == std::string_view::npos ? std::string_view{} : list.substr(comma + 2);
    std::string node;
    if (const std::size_t on = item.find(" (on node "); on != std::string_view::npos && item.ends_with(')'))
    {
      const std::size_t from = on + std::string_view{" (on node "}.size();
      node = std::string(item.substr(from, item.size() - 1 - from));
      item = item.substr(0, on);
    }
    _outSpec.sockets.emplace(std::string(item), std::move(node));
  }
  return true;
}

/// A socket bone's name read after the prefix (AST-038): its kind, its index and whether it was written with a number.
struct SocketName
{
  Neuron::SocketKind kind = Neuron::SocketKind::Port;
  std::uint32_t index = 0;
  bool numbered = false;
};

std::expected<SocketName, std::string> ReadSocketName(std::string_view _name)
{
  SocketName socket;
  std::string_view kind = _name;
  const std::size_t digitsAt = _name.size() >= SOCKET_INDEX_DIGITS + 1 ? _name.size() - SOCKET_INDEX_DIGITS : 0;
  const bool numbered = digitsAt > 0 && _name[digitsAt - 1] == '_' &&
                        std::ranges::all_of(_name.substr(digitsAt), [](char _c) { return _c >= '0' && _c <= '9'; });
  std::uint32_t number = 0;
  if (numbered)
  {
    for (const char c : _name.substr(digitsAt))
    {
      number = number * 10 + static_cast<std::uint32_t>(c - '0');
    }
    kind = _name.substr(0, digitsAt - 1);
  }
  const std::optional<Neuron::SocketKind> found = Neuron::SocketKindNamed(kind);
  if (!found)
  {
    return std::unexpected(std::format("the socket {}{} is of no kind of AST table 3.8-A (AST-039)", SOCKET_PREFIX, _name));
  }
  socket.kind = found.value_or(Neuron::SocketKind::Port);
  socket.numbered = numbered;
  if (numbered && !Neuron::SocketKindInfoOf(socket.kind).indexed)
  {
    return std::unexpected(std::format("the socket {}{} numbers a kind that is not indexed (AST-038)", SOCKET_PREFIX, _name));
  }
  if (numbered && number < SOCKET_FIRST_NUMBER)
  {
    return std::unexpected(std::format("the socket {}{} counts from {:02} (AST-038)", SOCKET_PREFIX, _name, SOCKET_FIRST_NUMBER));
  }
  socket.index = numbered ? number - SOCKET_FIRST_NUMBER : 0;
  return socket;
}

std::array<float, 16> Scaled(const std::array<float, 16>& _transform) noexcept
{
  std::array<float, 16> scaled = _transform;
  scaled[12] *= MESH_UNIT_SCALE;
  scaled[13] *= MESH_UNIT_SCALE;
  scaled[14] *= MESH_UNIT_SCALE;
  return scaled;
}

// RND-160: each colour channel × the attenuation, rounded half up, in integer arithmetic so every host gives one byte.
std::uint8_t Attenuated(std::uint32_t _channel) noexcept
{
  return static_cast<std::uint8_t>((_channel * MESH_COLOR_ATTENUATION_PERCENT + 50) / 100);
}

/// The cooker's state: the source, the mesh being cooked and the faults.
class Cooker
{
public:
  Cooker(const ManifestRow& _row, std::string_view _file, BuildReport& _report)
    : m_row(_row),
      m_file(_file),
      m_report(_report)
  {
  }

  std::optional<Neuron::MeshAsset> Cook(const CmoFile& _cmo)
  {
    BuildNodes(_cmo);
    if (!Failed())
    {
      BuildMaterials(_cmo);
    }
    if (!Failed())
    {
      BuildGeometry(_cmo);
    }
    if (!Failed())
    {
      BuildSockets(_cmo);
    }
    if (!Failed())
    {
      CheckRow();
    }
    if (Failed())
    {
      return std::nullopt;
    }
    return std::move(m_mesh);
  }

private:
  [[nodiscard]] bool Failed() const noexcept
  {
    return m_report.Findings().size() != m_findings;
  }

  void Fail(std::string_view _message)
  {
    m_report.Fail(m_file, _message);
  }

  [[nodiscard]] static const CmoMesh* SkeletonOf(const CmoFile& _cmo) noexcept
  {
    const auto found = std::ranges::find_if(_cmo.meshes, [](const CmoMesh& _mesh) { return _mesh.skeleton; });
    return found == _cmo.meshes.end() ? nullptr : &*found;
  }

  // Nodes are the bones that are not sockets, in bone order; a file with no skeleton is one node (ADR-019, O64).
  void BuildNodes(const CmoFile& _cmo)
  {
    const CmoMesh* skeleton = SkeletonOf(_cmo);
    if (skeleton == nullptr)
    {
      Neuron::MeshNode node;
      node.name = _cmo.meshes.front().name;
      node.restTransform = IDENTITY;
      m_mesh.nodes.push_back(std::move(node));
      m_nodeOfBone.clear();
      return;
    }
    for (const CmoBone& bone : skeleton->bones)
    {
      const std::optional<std::uint32_t> parentNode = bone.parent < 0 ? std::nullopt : m_nodeOfBone[static_cast<std::size_t>(bone.parent)];
      if (bone.name.starts_with(SOCKET_PREFIX))
      {
        if (bone.parent < 0 || !parentNode)
        {
          Fail(std::format("the socket {} is a root or the child of a socket; a socket moves with a node (AST-038)", bone.name));
        }
        m_nodeOfBone.push_back(std::nullopt);
        continue;
      }
      if (bone.parent >= 0 && !parentNode)
      {
        Fail(std::format("the node {} is the child of a socket (AST-038)", bone.name));
      }
      Neuron::MeshNode node;
      node.name = bone.name;
      node.parent = parentNode;
      node.restTransform = Scaled(bone.localTransform);
      m_nodeOfBone.push_back(static_cast<std::uint32_t>(m_mesh.nodes.size()));
      m_mesh.nodes.push_back(std::move(node));
    }
  }

  // Materials map onto the shader set of ADR-003 by name and fields (ADR-019 as amended by O64).
  void BuildMaterials(const CmoFile& _cmo)
  {
    for (const CmoMesh& mesh : _cmo.meshes)
    {
      std::vector<std::uint32_t> cooked;
      for (const CmoMaterial& material : mesh.materials)
      {
        const bool white = material.diffuse == std::array<float, 4>{1.0F, 1.0F, 1.0F, 1.0F};
        const bool untextured = material.pixelShader.empty() &&
                                std::ranges::all_of(material.textures, [](const std::string& _texture) { return _texture.empty(); });
        if (!white || !untextured)
        {
          Fail(std::format("the material {} of mesh {} has a diffuse colour other than white, a pixel shader or a texture; a mesh is "
                           "vertex-coloured, and textured meshes arrive with the first that needs one (AST-037, O64)",
                           material.name, mesh.name));
          continue;
        }
        Neuron::MeshMaterial out;
        out.name = material.name;
        out.shading = material.name == UNLIT_MATERIAL ? Neuron::MeshShading::Unlit : Neuron::MeshShading::Lit;
        out.specular = {material.specular[0], material.specular[1], material.specular[2]};
        out.specularPower = material.specularPower;
        out.emissive = {material.emissive[0], material.emissive[1], material.emissive[2]};
        cooked.push_back(static_cast<std::uint32_t>(m_mesh.materials.size()));
        m_mesh.materials.push_back(std::move(out));
      }
      m_materialOf.push_back(std::move(cooked));
    }
  }

  // AST-076: each triangle's three corners, with its facet normal, its one colour attenuated, its tint weight and its
  // corner, node by node.
  void BuildGeometry(const CmoFile& _cmo)
  {
    const bool skeleton = SkeletonOf(_cmo) != nullptr;
    std::vector<std::vector<std::size_t>> meshesOfNode(m_mesh.nodes.size());
    for (std::size_t m = 0; m < _cmo.meshes.size(); ++m)
    {
      const std::string& name = _cmo.meshes[m].name;
      const auto node = std::ranges::find(m_mesh.nodes, name, &Neuron::MeshNode::name);
      if (skeleton && node == m_mesh.nodes.end())
      {
        Fail(std::format("the mesh {} attaches to a socket; a socket has no mesh (AST-038)", name));
        return;
      }
      meshesOfNode[skeleton ? static_cast<std::size_t>(node - m_mesh.nodes.begin()) : 0].push_back(m);
    }
    for (std::size_t n = 0; n < m_mesh.nodes.size(); ++n)
    {
      if (!meshesOfNode[n].empty() && !RestIsIdentity(n))
      {
        Fail(std::format("the node {} has geometry and a rest transform other than the identity; whether a node's vertices are in its "
                         "own space or the model's is settled by the first such mesh (ADR-108)",
                         m_mesh.nodes[n].name));
        return;
      }
      for (const std::size_t m : meshesOfNode[n])
      {
        CookTriangles(_cmo.meshes[m], m, n);
      }
      Bound(m_mesh.nodes[n]);
    }
  }

  [[nodiscard]] bool RestIsIdentity(std::size_t _node) const noexcept
  {
    std::optional<std::uint32_t> at = static_cast<std::uint32_t>(_node);
    while (at.has_value())
    {
      const Neuron::MeshNode& node = m_mesh.nodes[at.value_or(0)];
      if (node.restTransform != IDENTITY)
      {
        return false;
      }
      at = node.parent;
    }
    return true;
  }

  void CookTriangles(const CmoMesh& _mesh, std::size_t _meshIndex, std::size_t _node)
  {
    for (std::size_t s = 0; s < _mesh.submeshes.size(); ++s)
    {
      const CmoSubmesh& submesh = _mesh.submeshes[s];
      const std::vector<std::uint16_t>& indices = _mesh.indexBuffers[submesh.indexBuffer];
      const std::vector<CmoVertex>& vertices = _mesh.vertexBuffers[submesh.vertexBuffer];
      const bool team = _mesh.materials[submesh.material].name == TEAM_MATERIAL;
      Neuron::MeshDraw draw;
      draw.material = m_materialOf[_meshIndex][submesh.material];
      draw.firstVertex = static_cast<std::uint32_t>(m_mesh.vertices.size());
      draw.vertexCount = submesh.primitiveCount * 3;
      for (std::uint32_t t = 0; t < submesh.primitiveCount; ++t)
      {
        const std::size_t first = std::size_t{submesh.startIndex} + std::size_t{t} * 3;
        const std::array<const CmoVertex*, 3> corners{&vertices[indices[first]], &vertices[indices[first + 1]],
                                                      &vertices[indices[first + 2]]};
        if (corners[0]->color != corners[1]->color || corners[0]->color != corners[2]->color)
        {
          Fail(std::format("triangle {} of submesh {} of node {} has corners of different colours; a faceted submesh has one colour a "
                           "triangle (AST-036, AST-076)",
                           t, s, m_mesh.nodes[_node].name));
          return;
        }
        const std::optional<std::array<float, 3>> normal = FacetNormal(corners);
        if (!normal)
        {
          Fail(
            std::format("triangle {} of submesh {} of node {} has no area, so no facet normal (AST-076)", t, s, m_mesh.nodes[_node].name));
          return;
        }
        const std::uint32_t color = corners[0]->color;
        const std::array<std::uint8_t, 4> facet{Attenuated(color & 0xFF), Attenuated((color >> 8) & 0xFF), Attenuated((color >> 16) & 0xFF),
                                                static_cast<std::uint8_t>(color >> 24)};
        for (std::uint8_t c = 0; c < 3; ++c)
        {
          Neuron::MeshVertex vertex;
          for (std::size_t axis = 0; axis < 3; ++axis)
          {
            vertex.position[axis] = corners[c]->position[axis] * MESH_UNIT_SCALE;
          }
          vertex.normal = normal.value_or(std::array<float, 3>{});
          vertex.color = facet;
          vertex.tint = static_cast<std::uint8_t>(team ? 255 : 0);
          vertex.corner = c;
          m_mesh.vertices.push_back(vertex);
        }
        if (team)
        {
          ++m_teamTriangles;
        }
      }
      m_teamMaterial = m_teamMaterial || team;
      m_mesh.nodes[_node].draws.push_back(draw);
    }
  }

  // The facet normal of AST-076 in binary64, rounded to binary32 once; none for a triangle with no area.
  [[nodiscard]] static std::optional<std::array<float, 3>> FacetNormal(const std::array<const CmoVertex*, 3>& _corners) noexcept
  {
    std::array<double, 3> e{};
    std::array<double, 3> f{};
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
      e[axis] = double{_corners[1]->position[axis]} - double{_corners[0]->position[axis]};
      f[axis] = double{_corners[2]->position[axis]} - double{_corners[0]->position[axis]};
    }
    const std::array<double, 3> cross{e[1] * f[2] - e[2] * f[1], e[2] * f[0] - e[0] * f[2], e[0] * f[1] - e[1] * f[0]};
    const double length = std::sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
    if (!(length > 0.0))
    {
      return std::nullopt;
    }
    return std::array<float, 3>{static_cast<float>(cross[0] / length), static_cast<float>(cross[1] / length),
                                static_cast<float>(cross[2] / length)};
  }

  // AST-076: the node's axis-aligned bounds, whose y range is its vertical extent, and the radius about their centre.
  void Bound(Neuron::MeshNode& _node)
  {
    bool any = false;
    for (const Neuron::MeshDraw& draw : _node.draws)
    {
      for (std::uint32_t v = draw.firstVertex; v < draw.firstVertex + draw.vertexCount; ++v)
      {
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
          const float value = m_mesh.vertices[v].position[axis];
          _node.boundsMin[axis] = any ? std::min(_node.boundsMin[axis], value) : value;
          _node.boundsMax[axis] = any ? std::max(_node.boundsMax[axis], value) : value;
        }
        any = true;
      }
    }
    double radius = 0.0;
    for (const Neuron::MeshDraw& draw : _node.draws)
    {
      for (std::uint32_t v = draw.firstVertex; v < draw.firstVertex + draw.vertexCount; ++v)
      {
        double squared = 0.0;
        for (std::size_t axis = 0; axis < 3; ++axis)
        {
          const double center = (double{_node.boundsMin[axis]} + double{_node.boundsMax[axis]}) / 2.0;
          const double offset = double{m_mesh.vertices[v].position[axis]} - center;
          squared += offset * offset;
        }
        radius = std::max(radius, std::sqrt(squared));
      }
    }
    _node.sphereRadius = static_cast<float>(radius);
  }

  // AST-038 to AST-040: the socket bones, their kinds and numbers, sorted by kind in table order and then by index.
  void BuildSockets(const CmoFile& _cmo)
  {
    const CmoMesh* skeleton = SkeletonOf(_cmo);
    if (skeleton == nullptr)
    {
      return;
    }
    std::set<std::pair<Neuron::SocketKind, std::uint32_t>> seen;
    std::map<Neuron::SocketKind, std::pair<bool, bool>> forms; // written with a number, written without
    for (std::size_t b = 0; b < skeleton->bones.size(); ++b)
    {
      const CmoBone& bone = skeleton->bones[b];
      if (!bone.name.starts_with(SOCKET_PREFIX))
      {
        continue;
      }
      const std::string_view spelled = std::string_view{bone.name}.substr(SOCKET_PREFIX.size());
      const auto name = ReadSocketName(spelled);
      if (!name)
      {
        Fail(name.error());
        continue;
      }
      if (!seen.emplace(name->kind, name->index).second)
      {
        Fail(std::format("the socket {} is there twice (AST-040)", bone.name));
        continue;
      }
      auto& [withNumber, withoutNumber] = forms[name->kind];
      (name->numbered ? withNumber : withoutNumber) = true;
      Neuron::MeshSocket socket;
      socket.kind = name->kind;
      socket.index = name->index;
      socket.node = m_nodeOfBone[static_cast<std::size_t>(bone.parent)].value_or(0);
      socket.transform = Scaled(bone.localTransform);
      m_mesh.sockets.push_back(socket);
      m_socketNames.emplace(std::string(spelled), m_mesh.nodes[socket.node].name);
      if (Neuron::SocketKindInfoOf(name->kind).simulationRelevant)
      {
        Fail(std::format("the socket {} is simulation-relevant, and simulation data records arrive with M2's structures (AST-043, "
                         "ADR-108)",
                         bone.name));
      }
    }
    for (const auto& [kind, written] : forms)
    {
      if (written.first && written.second)
      {
        Fail(std::format("the socket kind {} is written both with and without a number (AST-038)", Neuron::SocketKindInfoOf(kind).name));
      }
    }
    std::ranges::sort(m_mesh.sockets, {}, [](const Neuron::MeshSocket& _socket) { return std::pair{_socket.kind, _socket.index}; });
    for (std::size_t s = 0; s < m_mesh.sockets.size(); ++s)
    {
      const bool startsKind = s == 0 || m_mesh.sockets[s - 1].kind != m_mesh.sockets[s].kind;
      const std::uint32_t expected = startsKind ? 0 : m_mesh.sockets[s - 1].index + 1;
      if (m_mesh.sockets[s].index != expected)
      {
        Fail(std::format("the sockets of kind {} have a gap before number {:02} (AST-038)",
                         Neuron::SocketKindInfoOf(m_mesh.sockets[s].kind).name, m_mesh.sockets[s].index + SOCKET_FIRST_NUMBER));
      }
    }
  }

  // The row's spec and the format's capacity (AST-035, AST-040, AST-076).
  void CheckRow()
  {
    MeshSpec spec;
    if (!ReadSpec(m_row.spec, spec))
    {
      Fail(std::format("its row {} (manifest line {}) states no triangle budget, node count, depth or socket list that the content build "
                       "can read (AST-012)",
                       m_row.assetId, m_row.line));
      return;
    }
    const std::size_t triangles = m_mesh.vertices.size() / 3;
    if (triangles > spec.triangleBudget)
    {
      Fail(std::format("{} triangles pass the budget of {} of its row {} (AST-076)", triangles, spec.triangleBudget, m_row.assetId));
    }
    std::uint64_t depth = 0;
    for (std::size_t n = 0; n < m_mesh.nodes.size(); ++n)
    {
      std::uint64_t chain = 0;
      std::optional<std::uint32_t> at = static_cast<std::uint32_t>(n);
      while (at.has_value())
      {
        ++chain;
        at = m_mesh.nodes[at.value_or(0)].parent;
      }
      depth = std::max(depth, chain);
    }
    if (m_mesh.nodes.size() != spec.nodes || depth != spec.depth)
    {
      Fail(std::format("{} nodes of depth {}; its row {} asks for {} of depth {} (AST-076)", m_mesh.nodes.size(), depth, m_row.assetId,
                       spec.nodes, spec.depth));
    }
    const bool sameSockets =
      std::ranges::equal(m_socketNames, spec.sockets, [](const auto& _found, const auto& _listed)
                         { return _found.first == _listed.first && (_listed.second.empty() || _found.second == _listed.second); });
    if (!sameSockets)
    {
      Fail(std::format("its sockets are not those its row {} lists, each on the node it names (AST-040)", m_row.assetId));
    }
    if (m_mesh.nodes.size() > Neuron::MESH_MAX_NODES || m_mesh.sockets.size() > Neuron::MESH_MAX_SOCKETS)
    {
      Fail(std::format("{} nodes and {} sockets pass the capacity of the cooked format, {} and {} (AST-076)", m_mesh.nodes.size(),
                       m_mesh.sockets.size(), Neuron::MESH_MAX_NODES, Neuron::MESH_MAX_SOCKETS));
    }
    if (m_teamMaterial && m_teamTriangles == 0)
    {
      Fail("the material team marks no triangle, so the tint marking carries no weight (AST-035)");
    }
  }

  const ManifestRow& m_row;
  std::string_view m_file;
  BuildReport& m_report;
  std::size_t m_findings = m_report.Findings().size();
  Neuron::MeshAsset m_mesh;
  std::vector<std::optional<std::uint32_t>> m_nodeOfBone;
  std::vector<std::vector<std::uint32_t>> m_materialOf; // per mesh, per material: the cooked material
  std::map<std::string, std::string> m_socketNames;     // as spelled after the prefix → the node it sits on
  bool m_teamMaterial = false;
  std::size_t m_teamTriangles = 0;
};

} // namespace

std::optional<Neuron::CookedEntry> CookMesh(const ManifestRow& _row, std::string_view _file, std::span<const std::byte> _bytes,
                                            BuildReport& _report)
{
  if (_row.subcategory == "collision")
  {
    _report.Fail(_file, "collision assets are cooked from the first collidable structure, at M2 (AST-044, ADR-108)");
    return std::nullopt;
  }
  const auto cmo = ReadCmoFile(_bytes);
  if (!cmo)
  {
    _report.Fail(_file, cmo.error());
    return std::nullopt;
  }
  Cooker cooker(_row, _file, _report);
  std::optional<Neuron::MeshAsset> mesh = cooker.Cook(*cmo);
  if (!mesh)
  {
    return std::nullopt;
  }
  Neuron::CookedEntry entry;
  entry.entry.type = Neuron::ContentType::Mesh;
  entry.bytes = Neuron::WriteMeshAsset(*mesh);
  // AST-091: the entry reads back through the runtime's loader as what the cooker wrote.
  const auto readBack = Neuron::ReadMeshAsset(entry.bytes);
  if (!readBack || Neuron::WriteMeshAsset(*readBack) != entry.bytes)
  {
    _report.Fail(_file,
                 std::format("the cooked mesh does not read back as written: {} (AST-091)", readBack ? "it differs" : readBack.error()));
    return std::nullopt;
  }
  return entry;
}

} // namespace ContentBuild
