#include "pch.h"

#include "CollisionAsset.h"
#include "CookedRecord.h"

#include <cmath>
#include <format>

namespace Neuron
{

namespace
{

constexpr std::size_t CORNER_BYTES = 12;

void WriteSphere(CookedWriter& _out, const BoundingSphere& _sphere)
{
  for (const float value : _sphere.center)
  {
    _out.F32(value);
  }
  _out.F32(_sphere.radius);
}

BoundingSphere ReadSphere(CookedReader& _in)
{
  BoundingSphere sphere;
  for (float& value : sphere.center)
  {
    value = _in.F32();
  }
  sphere.radius = _in.F32();
  return sphere;
}

bool Finite(const BoundingSphere& _sphere) noexcept
{
  return std::isfinite(_sphere.center[0]) && std::isfinite(_sphere.center[1]) && std::isfinite(_sphere.center[2]) &&
         std::isfinite(_sphere.radius) && _sphere.radius >= 0.0F;
}

} // namespace

std::vector<std::byte> WriteCollisionAsset(const CollisionAsset& _asset)
{
  CookedWriter out;
  out.U32(COLLISION_ASSET_VERSION);
  out.U32(static_cast<std::uint32_t>(_asset.nodes.size()));
  WriteSphere(out, _asset.sphere);
  for (const CollisionNode& node : _asset.nodes)
  {
    out.Text8(node.name);
    WriteSphere(out, node.sphere);
    out.U32(static_cast<std::uint32_t>(node.corners.size()));
    for (const std::array<float, 3>& corner : node.corners)
    {
      for (const float value : corner)
      {
        out.F32(value);
      }
    }
  }
  return out.Take();
}

std::expected<CollisionAsset, std::string> ReadCollisionAsset(std::span<const std::byte> _entry)
{
  CookedReader in(_entry);
  if (const std::uint32_t version = in.U32(); version != COLLISION_ASSET_VERSION)
  {
    return std::unexpected(
      std::format("the cooked collision asset is version {}; this build reads version {} only", version, COLLISION_ASSET_VERSION));
  }
  const std::uint32_t nodes = in.U32();
  if (in.Failed() || nodes > COLLISION_MAX_NODES)
  {
    return std::unexpected(std::string("the cooked collision asset's node count passes its capacity"));
  }
  CollisionAsset asset;
  asset.sphere = ReadSphere(in);
  std::size_t triangles = 0;
  asset.nodes.resize(nodes);
  for (CollisionNode& node : asset.nodes)
  {
    node.name = std::string(in.Text8());
    node.sphere = ReadSphere(in);
    const std::uint32_t corners = in.U32();
    triangles += corners / 3;
    if (in.Failed() || corners % 3 != 0 || triangles > COLLISION_MAX_TRIANGLES || std::uint64_t{corners} * CORNER_BYTES > in.Remaining())
    {
      return std::unexpected(
        std::format("the node {} states corners that are not whole triangles, pass the capacity or the entry", node.name));
    }
    node.corners.resize(corners);
    for (std::array<float, 3>& corner : node.corners)
    {
      for (float& value : corner)
      {
        value = in.F32();
        if (!std::isfinite(value))
        {
          return std::unexpected(std::format("a corner of node {} is not finite", node.name));
        }
      }
    }
    if (!Finite(node.sphere))
    {
      return std::unexpected(std::format("the sphere of node {} is not finite", node.name));
    }
  }
  if (!Finite(asset.sphere) || !in.Finished())
  {
    return std::unexpected(std::string("the cooked collision asset's sphere is not finite, or its bytes do not end where its counts do"));
  }
  return asset;
}

} // namespace Neuron
