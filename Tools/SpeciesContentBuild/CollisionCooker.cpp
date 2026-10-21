#include "pch.h"

#include "CmoFile.h"
#include "CollisionAsset.h"
#include "CollisionCooker.h"
#include "MeshCooker.h"
#include "RowSpec.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <string>
#include <vector>

namespace ContentBuild
{

namespace
{

constexpr std::array<float, 16> IDENTITY{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

// The sphere about the centre of the corners' bounds that holds them all, made in binary64 and rounded once (AST-077).
Neuron::BoundingSphere SphereOf(const std::vector<std::array<float, 3>>& _corners)
{
  Neuron::BoundingSphere sphere;
  if (_corners.empty())
  {
    return sphere;
  }
  std::array<float, 3> low = _corners.front();
  std::array<float, 3> high = _corners.front();
  for (const std::array<float, 3>& corner : _corners)
  {
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
      low[axis] = std::min(low[axis], corner[axis]);
      high[axis] = std::max(high[axis], corner[axis]);
    }
  }
  std::array<double, 3> center{};
  for (std::size_t axis = 0; axis < 3; ++axis)
  {
    center[axis] = (double{low[axis]} + double{high[axis]}) / 2.0;
    sphere.center[axis] = static_cast<float>(center[axis]);
  }
  double radius = 0.0;
  for (const std::array<float, 3>& corner : _corners)
  {
    double squared = 0.0;
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
      const double offset = double{corner[axis]} - center[axis];
      squared += offset * offset;
    }
    radius = std::max(radius, std::sqrt(squared));
  }
  sphere.radius = static_cast<float>(radius);
  return sphere;
}

} // namespace

std::optional<Neuron::CookedEntry> CookCollision(const ManifestRow& _row, std::string_view _file, std::span<const std::byte> _bytes,
                                                 BuildReport& _report)
{
  const std::optional<std::uint64_t> budget = NumberAfter(_row.spec, "<= ");
  if (!budget || _row.spec.find("triangles budget") == std::string::npos)
  {
    _report.Fail(_file, std::format("its row {} (manifest line {}) states no triangle budget that the content build can read (AST-012)",
                                    _row.assetId, _row.line));
    return std::nullopt;
  }
  const auto cmo = ReadCmoFile(_bytes);
  if (!cmo)
  {
    _report.Fail(_file, cmo.error());
    return std::nullopt;
  }
  const std::size_t findings = _report.Findings().size();

  // Nodes: the skeleton's bones, every one a node at rest at the identity; a file with no skeleton is one node.
  Neuron::CollisionAsset asset;
  const auto skeleton = std::ranges::find_if(cmo->meshes, [](const CmoMesh& _mesh) { return _mesh.skeleton; });
  if (skeleton == cmo->meshes.end())
  {
    asset.nodes.push_back(Neuron::CollisionNode{cmo->meshes.front().name, {}, {}});
  }
  else
  {
    for (const CmoBone& bone : skeleton->bones)
    {
      if (bone.name.starts_with(SOCKET_PREFIX))
      {
        _report.Fail(_file, std::format("the bone {} is a socket; a collision asset has none (AST-044)", bone.name));
      }
      else if (bone.parent >= 0 || bone.localTransform != IDENTITY)
      {
        _report.Fail(_file, std::format("the node {} is not a root at the identity; a collision asset's nodes are in the asset's own space "
                                        "(AST-077, ADR-129)",
                                        bone.name));
      }
      asset.nodes.push_back(Neuron::CollisionNode{bone.name, {}, {}});
    }
  }
  if (asset.nodes.size() > Neuron::COLLISION_MAX_NODES)
  {
    _report.Fail(
      _file, std::format("{} nodes pass the capacity of the cooked format, {} (AST-077)", asset.nodes.size(), Neuron::COLLISION_MAX_NODES));
  }
  if (_report.Findings().size() != findings)
  {
    return std::nullopt;
  }

  // Triangles: every submesh's, whatever its material, in world units (AST-077).
  std::size_t triangles = 0;
  std::vector<std::array<float, 3>> all;
  for (const CmoMesh& mesh : cmo->meshes)
  {
    const auto node =
      skeleton == cmo->meshes.end() ? asset.nodes.begin() : std::ranges::find(asset.nodes, mesh.name, &Neuron::CollisionNode::name);
    if (node == asset.nodes.end())
    {
      continue; // ADR-019 check 7 has every mesh named after a bone, and every bone is a node here
    }
    for (const CmoSubmesh& submesh : mesh.submeshes)
    {
      const std::vector<std::uint16_t>& indices = mesh.indexBuffers[submesh.indexBuffer];
      const std::vector<CmoVertex>& vertices = mesh.vertexBuffers[submesh.vertexBuffer];
      for (std::uint32_t i = 0; i < submesh.primitiveCount * 3; ++i)
      {
        const CmoVertex& vertex = vertices[indices[std::size_t{submesh.startIndex} + i]];
        const std::array<float, 3> corner{vertex.position[0] * MESH_UNIT_SCALE, vertex.position[1] * MESH_UNIT_SCALE,
                                          vertex.position[2] * MESH_UNIT_SCALE};
        node->corners.push_back(corner);
        all.push_back(corner);
      }
      triangles += submesh.primitiveCount;
    }
  }
  if (triangles > budget.value_or(0))
  {
    _report.Fail(_file,
                 std::format("{} triangles pass the budget of {} of its row {} (AST-077)", triangles, budget.value_or(0), _row.assetId));
    return std::nullopt;
  }
  if (triangles == 0)
  {
    _report.Fail(_file, "it holds no triangle, so no hit test could read it (AST-044)");
    return std::nullopt;
  }
  for (Neuron::CollisionNode& node : asset.nodes)
  {
    node.sphere = SphereOf(node.corners);
  }
  asset.sphere = SphereOf(all);

  Neuron::CookedEntry entry;
  entry.entry.type = Neuron::ContentType::Collision;
  entry.entry.simulationRelevant = true;
  entry.bytes = Neuron::WriteCollisionAsset(asset);
  // AST-091: the entry reads back through the runtime's loader as what the cooker wrote.
  const auto readBack = Neuron::ReadCollisionAsset(entry.bytes);
  if (!readBack || Neuron::WriteCollisionAsset(*readBack) != entry.bytes)
  {
    _report.Fail(_file, std::format("the cooked collision asset does not read back as written: {} (AST-091)",
                                    readBack ? "it differs" : readBack.error()));
    return std::nullopt;
  }
  return entry;
}

} // namespace ContentBuild
