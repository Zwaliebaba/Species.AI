#pragma once

#include "CollisionAsset.h"
#include "MeshAsset.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// The version of the cooked simulation data record; a reader accepts this version only (ADR-129).
inline constexpr std::uint32_t SIMULATION_RECORD_VERSION = 1;
// The suffix that makes a mesh's key into its simulation data record's (ADR-129): `meshes/wellspring@sim`.
inline constexpr std::string_view SIMULATION_RECORD_SUFFIX = "@sim";

/// A node of the mesh's hierarchy as the simulation reads it (AST-043): its name, its parent (none for a root, which
/// comes before its children) and its rest transform relative to its parent, row-major with row vectors.
struct SimulationNode
{
  std::string name;
  std::optional<std::uint32_t> parent;
  std::array<float, 16> restTransform{};
};

/// A simulation-relevant socket (AST-043, ADR-021): its kind, its index, the node it moves with and its transform
/// relative to that node.
struct SimulationSocket
{
  SocketKind kind = SocketKind::Port;
  std::uint32_t index = 0;
  std::uint32_t node = 0;
  std::array<float, 16> transform{};
};

/// The bounding sphere of one node of the collision asset (AST-046), by the render node it stands for.
struct SimulationSphere
{
  std::uint32_t node = 0;
  BoundingSphere sphere;
};

/// The simulation data record of AST-046 and DAT-279: what the simulation reads of a mesh, so that it never reads the
/// render mesh (AST-043). The mesh's node hierarchy at rest; its simulation-relevant sockets, in the order of AST table
/// 3.8-A and then by index; and, when the mesh is collidable (AST-044), the key of its collision asset, that asset's
/// bounding sphere and each of its nodes' spheres.
struct SimulationRecord
{
  std::vector<SimulationNode> nodes;
  std::vector<SimulationSocket> sockets;
  std::string collision;
  std::optional<BoundingSphere> bounds;
  std::vector<SimulationSphere> nodeSpheres;
};

// The entry bytes of a simulation data record, in the layout of ADR-129.
[[nodiscard]] std::vector<std::byte> WriteSimulationRecord(const SimulationRecord& _record);
// A simulation data record, or why the bytes are not one of this version.
[[nodiscard]] std::expected<SimulationRecord, std::string> ReadSimulationRecord(std::span<const std::byte> _entry);

// The record's sockets of one kind, in index order (ADR-021: a socket is addressed by its kind and index).
[[nodiscard]] std::vector<const SimulationSocket*> SocketsOf(const SimulationRecord& _record, SocketKind _kind);

// A socket's position in the mesh's space at rest: its translation carried through its node's rest chain.
[[nodiscard]] std::array<double, 3> RestPositionOf(const SimulationRecord& _record, const SimulationSocket& _socket);
// A socket's front in the mesh's space at rest: its +z axis, its transform's third row, carried through its node's rest
// chain without the translations.
[[nodiscard]] std::array<double, 3> RestFrontOf(const SimulationRecord& _record, const SimulationSocket& _socket);

} // namespace Neuron
