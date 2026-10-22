#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace Neuron
{

// The version of the cooked collision format; a reader accepts this version only (ADR-129).
inline constexpr std::uint32_t COLLISION_ASSET_VERSION = 1;
// The capacity of the cooked format: the largest triangle budget of a collision row (AST table 3.8-B) and its nodes.
inline constexpr std::size_t COLLISION_MAX_TRIANGLES = 25000;
inline constexpr std::size_t COLLISION_MAX_NODES = 100;

/// A sphere in an asset's space: its centre and radius, in world units.
struct BoundingSphere
{
  std::array<float, 3> center{};
  float radius = 0.0F;
};

/// One node of a collision asset (AST-077): the name of the render node it stands for, its triangles as three corners
/// each, in world units relative to the asset's origin, and its bounding sphere about the centre of its bounds.
struct CollisionNode
{
  std::string name;
  std::vector<std::array<float, 3>> corners;
  BoundingSphere sphere;
};

/// The cooked collision asset of AST-044 and AST-077: the hit geometry that a structure's sphere, mesh and ray tests
/// read (R-BLDG-041 to R-BLDG-043), never the drawn triangles, and the sphere that bounds all of it.
struct CollisionAsset
{
  std::vector<CollisionNode> nodes;
  BoundingSphere sphere;
};

// The entry bytes of a cooked collision asset, in the layout of ADR-129.
[[nodiscard]] std::vector<std::byte> WriteCollisionAsset(const CollisionAsset& _asset);
// A cooked collision asset, or why the bytes are not one of this version.
[[nodiscard]] std::expected<CollisionAsset, std::string> ReadCollisionAsset(std::span<const std::byte> _entry);

} // namespace Neuron
