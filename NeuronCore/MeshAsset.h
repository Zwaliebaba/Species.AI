#pragma once

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

// The version of the cooked mesh format; a reader accepts this version only (ADR-019, ADR-108).
inline constexpr std::uint32_t MESH_ASSET_VERSION = 1;
// rnd.mesh.max_nodes and rnd.mesh.max_sockets: the capacity of the cooked format (RND-156, AST-076).
inline constexpr std::size_t MESH_MAX_NODES = 100;
inline constexpr std::size_t MESH_MAX_SOCKETS = 100;
// The bytes of one cooked vertex (ADR-108).
inline constexpr std::size_t MESH_VERTEX_BYTES = 32;

// The socket kinds of AST table 3.8-A, in its order, which is the order of a cooked socket table (AST-076).
enum class SocketKind : std::uint8_t
{
  Port,
  Status,
  Light,
  Glow,
  Door,
  Entrance,
  GateEntrance,
  GateExit,
  GateSurface,
  BeamFocus,
  EssenceLink,
  GridLink,
  FuelPoint,
  ArmTip,
  DishDirection,
  Thruster,
  Window,
  FieldAnchor,
  BarrelMount,
  KillCounter,
  Muzzle,
  CasingEject,
  LaunchPoint,
  Stem,
  EggSpawn,
  EggLay,
  Carry,
  Tail,
  Leg,
  SegmentEnd,
  Engine,
  Flag,
  Eye,
  Grab
};

inline constexpr std::size_t SOCKET_KIND_COUNT = 34;

// A socket kind's name as a CMO bone spells it after `socket_`, and whether it is indexed (AST-038, AST-039).
struct SocketKindInfo
{
  std::string_view name;
  bool indexed = false;
  bool simulationRelevant = false;
};

[[nodiscard]] const SocketKindInfo& SocketKindInfoOf(SocketKind _kind) noexcept;
[[nodiscard]] std::optional<SocketKind> SocketKindNamed(std::string_view _name) noexcept;

// The material shaders of ADR-003 that a cooked material selects (ADR-019 as amended by O64).
enum class MeshShading : std::uint8_t
{
  Lit,
  Unlit
};

struct MeshMaterial
{
  std::string name;
  MeshShading shading = MeshShading::Lit;
  std::array<float, 3> specular{};
  float specularPower = 0.0F;
  std::array<float, 3> emissive{};
};

/// One cooked vertex (AST-076): a corner of one triangle, in its node's space and in world units, with its triangle's
/// facet normal, its facet colour (sRGB-encoded 8-bit red, green, blue and alpha, the attenuation applied), the
/// triangle's tint weight (0 to 255 for 0 to 1) and which corner it is (0, 1 or 2, the barycentric corner).
struct MeshVertex
{
  std::array<float, 3> position{};
  std::array<float, 3> normal{};
  std::array<std::uint8_t, 4> color{};
  std::uint8_t tint = 0;
  std::uint8_t corner = 0;
};

// A node's vertices that one material draws: three a triangle.
struct MeshDraw
{
  std::uint32_t material = 0;
  std::uint32_t firstVertex = 0;
  std::uint32_t vertexCount = 0;
};

/// One node of the hierarchy (AST-076): its name, its parent (none for a root, which comes before its children), its
/// rest transform relative to its parent (row-major, row vectors, translation in the last row, as DirectXMath and the
/// CMO file write it), its draws, its axis-aligned bounds, whose y range is its vertical extent, and its bounding sphere
/// about the bounds' centre.
struct MeshNode
{
  std::string name;
  std::optional<std::uint32_t> parent;
  std::array<float, 16> restTransform{};
  std::vector<MeshDraw> draws;
  std::array<float, 3> boundsMin{};
  std::array<float, 3> boundsMax{};
  float sphereRadius = 0.0F;
};

// A socket (ADR-021, AST-038): its kind, its index (its number minus 1), the node it moves with and its transform
// relative to that node.
struct MeshSocket
{
  SocketKind kind = SocketKind::Port;
  std::uint32_t index = 0;
  std::uint32_t node = 0;
  std::array<float, 16> transform{};
};

/// The cooked mesh of ADR-019: what the runtime loads, with no source format left in it. The renderer never parses a
/// CMO file (AST-004).
struct MeshAsset
{
  std::vector<MeshMaterial> materials;
  std::vector<MeshNode> nodes;
  std::vector<MeshSocket> sockets; // in the order of AST table 3.8-A, then by index
  std::vector<MeshVertex> vertices;
};

// The entry bytes of a cooked mesh, in the layout of ADR-108.
[[nodiscard]] std::vector<std::byte> WriteMeshAsset(const MeshAsset& _mesh);
// A cooked mesh, or why the bytes are not one of this version.
[[nodiscard]] std::expected<MeshAsset, std::string> ReadMeshAsset(std::span<const std::byte> _entry);

} // namespace Neuron
