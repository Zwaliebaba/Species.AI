#include "pch.h"

#include "CollisionAsset.h"
#include "CollisionCooker.h"
#include "MeshAsset.h"
#include "SimulationRecord.h"
#include "SimulationRecordCooker.h"

#include <algorithm>
#include <format>
#include <optional>
#include <string_view>

namespace ContentBuild
{

namespace
{

constexpr std::string_view MESH_PREFIX = "mesh.";
constexpr std::string_view COLLISION_PREFIX = "mesh.collision.";

// The name an asset id ends in: `wellspring` for `mesh.structure.wellspring`.
std::string_view NameOf(std::string_view _assetId) noexcept
{
  const std::size_t dot = _assetId.rfind('.');
  return dot == std::string_view::npos ? _assetId : _assetId.substr(dot + 1);
}

// The member place of a row among its asset's rows (AST-016).
std::optional<std::size_t> MemberOf(const AssetManifest& _manifest, const ManifestRow& _row)
{
  const std::vector<const ManifestRow*> members = _manifest.MembersOf(_row.assetId);
  const auto found = std::ranges::find(members, &_row);
  if (found == members.end())
  {
    return std::nullopt;
  }
  return static_cast<std::size_t>(found - members.begin());
}

// The render asset a collision asset stands for: the one mesh asset that is not a collision asset and ends in its name.
std::optional<std::string> RenderAssetOf(const AssetManifest& _manifest, std::string_view _collisionId)
{
  std::optional<std::string> found;
  const std::string suffix = std::format(".{}", NameOf(_collisionId));
  for (const ManifestRow& row : _manifest.Rows())
  {
    if (row.category == "Meshes" && row.subcategory != COLLISION_SUBCATEGORY && row.assetId.starts_with(MESH_PREFIX) &&
        row.assetId.ends_with(suffix) && row.assetId != found)
    {
      if (found)
      {
        return std::nullopt; // two candidates: the convention names no one asset
      }
      found = row.assetId;
    }
  }
  return found;
}

const CookedGeometry* Cooked(std::span<const CookedGeometry> _cooked, const ManifestRow* _row) noexcept
{
  const auto found = std::ranges::find(_cooked, _row, &CookedGeometry::row);
  return found == _cooked.end() ? nullptr : &*found;
}

} // namespace

void CookSimulationRecords(const AssetManifest& _manifest, std::span<const CookedGeometry> _meshes,
                           std::span<const CookedGeometry> _collisions, std::vector<Neuron::CookedEntry>& _entries, BuildReport& _report)
{
  // AST-090: a collision source stands for an authored render member.
  for (const CookedGeometry& collision : _collisions)
  {
    const std::optional<std::string> render = RenderAssetOf(_manifest, collision.row->assetId);
    const std::optional<std::size_t> member = MemberOf(_manifest, *collision.row);
    const std::vector<const ManifestRow*> renderMembers = render ? _manifest.MembersOf(*render) : std::vector<const ManifestRow*>{};
    if (!render || !member || member.value_or(0) >= renderMembers.size() || Cooked(_meshes, renderMembers[member.value_or(0)]) == nullptr)
    {
      _report.Fail(collision.file, std::format("it is the collision asset of member {} of {}, which has no authored render file (AST-044, "
                                               "AST-090)",
                                               member.value_or(0) + 1, render.value_or(std::string(NameOf(collision.row->assetId)))));
    }
  }

  for (const CookedGeometry& mesh : _meshes)
  {
    // AST-044: a collidable mesh has its collision source authored.
    const CookedGeometry* collision = nullptr;
    const std::optional<std::size_t> member = MemberOf(_manifest, *mesh.row);
    const std::vector<const ManifestRow*> collisionMembers =
      _manifest.MembersOf(std::format("{}{}", COLLISION_PREFIX, NameOf(mesh.row->assetId)));
    if (member && member.value_or(0) < collisionMembers.size())
    {
      const ManifestRow* collisionRow = collisionMembers[member.value_or(0)];
      collision = Cooked(_collisions, collisionRow);
      if (collision == nullptr)
      {
        _report.Fail(mesh.file, std::format("it is collidable, and its collision asset {} has no cooked file (AST-044)",
                                            collisionRow->speciesFilename));
        continue;
      }
    }

    const auto asset = Neuron::ReadMeshAsset(_entries[mesh.entry].bytes);
    if (!asset)
    {
      _report.Fail(mesh.file, std::format("its cooked mesh cannot be read for its simulation data record: {}", asset.error()));
      continue;
    }
    Neuron::SimulationRecord record;
    for (const Neuron::MeshNode& node : asset->nodes)
    {
      record.nodes.push_back(Neuron::SimulationNode{node.name, node.parent, node.restTransform});
    }
    for (const Neuron::MeshSocket& socket : asset->sockets)
    {
      if (Neuron::SocketKindInfoOf(socket.kind).simulationRelevant)
      {
        record.sockets.push_back(Neuron::SimulationSocket{socket.kind, socket.index, socket.node, socket.transform});
      }
    }
    if (collision != nullptr)
    {
      const auto hits = Neuron::ReadCollisionAsset(_entries[collision->entry].bytes);
      if (!hits)
      {
        _report.Fail(collision->file, std::format("its cooked collision asset cannot be read: {}", hits.error()));
        continue;
      }
      bool nodesFound = true;
      for (const Neuron::CollisionNode& node : hits->nodes)
      {
        const auto renderNode = std::ranges::find(asset->nodes, node.name, &Neuron::MeshNode::name);
        if (renderNode == asset->nodes.end())
        {
          _report.Fail(collision->file, std::format("its node {} is no node of its render member {} (AST-077)", node.name, mesh.file));
          nodesFound = false;
          continue;
        }
        record.nodeSpheres.push_back(Neuron::SimulationSphere{static_cast<std::uint32_t>(renderNode - asset->nodes.begin()), node.sphere});
      }
      if (!nodesFound)
      {
        continue;
      }
      record.collision = _entries[collision->entry].entry.key;
      record.bounds = hits->sphere;
    }
    if (record.sockets.empty() && collision == nullptr)
    {
      continue; // the simulation reads nothing of this mesh
    }
    Neuron::CookedEntry entry;
    entry.entry.key = _entries[mesh.entry].entry.key + std::string(Neuron::SIMULATION_RECORD_SUFFIX);
    entry.entry.type = Neuron::ContentType::SimRecord;
    entry.entry.simulationRelevant = true;
    entry.bytes = Neuron::WriteSimulationRecord(record);
    // AST-091: the entry reads back through the runtime's loader as what the cooker wrote.
    const auto readBack = Neuron::ReadSimulationRecord(entry.bytes);
    if (!readBack || Neuron::WriteSimulationRecord(*readBack) != entry.bytes)
    {
      _report.Fail(mesh.file, std::format("its simulation data record does not read back as written: {} (AST-091)",
                                          readBack ? "it differs" : readBack.error()));
      continue;
    }
    _entries.push_back(std::move(entry));
  }
}

} // namespace ContentBuild
