#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace MeshKit
{

using Point = std::array<double, 3>;

/// A vertex colour in sRGB, opaque (ADR-020: one colour a triangle).
struct Color
{
  std::uint8_t red = 0;
  std::uint8_t green = 0;
  std::uint8_t blue = 0;
};

/// The material a triangle is drawn with (ADR-019 as amended by O64): `facet` is lit, `team` is lit and carries the tint
/// weight of ADR-020, and `unlit` glows at its own colour.
enum class Surface : std::uint8_t
{
  Facet,
  Team,
  Unlit
};

struct Triangle
{
  std::array<Point, 3> corners{};
  Color color{};
  Surface surface = Surface::Facet;
};

/// A node of the asset (ADR-021): a name, which its CMO mesh and its bone carry, and its triangles in the asset's own
/// space, since every node the kit writes rests at the identity (ADR-128).
struct ModelNode
{
  std::string name;
  std::vector<Triangle> triangles;
};

/// A socket (ADR-021, AST-038): its name after the `socket_` prefix, the node it moves with and its position in the
/// asset's space; its axes are the asset's.
struct ModelSocket
{
  std::string name;
  std::string node;
  Point position{};
};

/// One CMO file the kit writes: its file name under GameData/Meshes, its nodes and its sockets. A collision asset
/// (AST-044) holds triangles alone, drawn with no material that the content build reads.
struct MeshModel
{
  std::string file;
  std::vector<ModelNode> nodes;
  std::vector<ModelSocket> sockets;
  bool collision = false;

  [[nodiscard]] ModelNode& Node(const std::string& _name);
  void AddSocket(std::string _name, const std::string& _node, const Point& _position);
  [[nodiscard]] std::size_t TriangleCount() const noexcept;
};

} // namespace MeshKit
