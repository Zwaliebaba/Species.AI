#include "pch.h"

#include "CookedRecord.h"
#include "MeshAsset.h"

#include <algorithm>
#include <format>

namespace Neuron
{

namespace
{

constexpr std::uint32_t NO_PARENT = 0xFFFFFFFF;

// AST table 3.8-A: the name after `socket_`, whether the kind is indexed and whether the simulation reads it.
constexpr std::array<SocketKindInfo, SOCKET_KIND_COUNT> SOCKET_KINDS{{
  {"port", true, true},           {"status", true, false},       {"light", true, false},           {"glow", true, false},
  {"door", false, true},          {"entrance", false, true},     {"gate_entrance", false, true},   {"gate_exit", true, true},
  {"gate_surface", false, false}, {"beam_focus", false, true},   {"essence_link", false, false},   {"grid_link", false, false},
  {"fuel_point", false, false},   {"arm_tip", false, false},     {"dish_direction", false, false}, {"thruster", false, false},
  {"window", true, false},        {"field_anchor", true, true},  {"barrel_mount", false, true},    {"kill_counter", false, false},
  {"muzzle", true, true},         {"casing_eject", true, false}, {"launch_point", false, true},    {"stem", false, false},
  {"egg_spawn", false, true},     {"egg_lay", false, true},      {"carry", false, true},           {"tail", true, false},
  {"leg", true, false},           {"segment_end", false, false}, {"engine", true, false},          {"flag", false, false},
  {"eye", true, false},           {"grab", true, false},
}};

void WriteFloats(CookedWriter& _writer, std::span<const float> _values)
{
  for (const float value : _values)
  {
    _writer.F32(value);
  }
}

void ReadFloats(CookedReader& _reader, std::span<float> _values)
{
  for (float& value : _values)
  {
    value = _reader.F32();
  }
}

} // namespace

const SocketKindInfo& SocketKindInfoOf(SocketKind _kind) noexcept
{
  const auto index = static_cast<std::size_t>(_kind);
  return SOCKET_KINDS[index < SOCKET_KINDS.size() ? index : 0];
}

std::optional<SocketKind> SocketKindNamed(std::string_view _name) noexcept
{
  const auto found = std::ranges::find(SOCKET_KINDS, _name, &SocketKindInfo::name);
  if (found == SOCKET_KINDS.end())
  {
    return std::nullopt;
  }
  return static_cast<SocketKind>(found - SOCKET_KINDS.begin());
}

std::vector<std::byte> WriteMeshAsset(const MeshAsset& _mesh)
{
  CookedWriter out;
  out.U32(MESH_ASSET_VERSION);
  out.U32(static_cast<std::uint32_t>(_mesh.materials.size()));
  out.U32(static_cast<std::uint32_t>(_mesh.nodes.size()));
  out.U32(static_cast<std::uint32_t>(_mesh.sockets.size()));
  out.U32(static_cast<std::uint32_t>(_mesh.vertices.size()));
  for (const MeshMaterial& material : _mesh.materials)
  {
    out.Text8(material.name);
    out.U8(static_cast<std::uint8_t>(material.shading));
    WriteFloats(out, material.specular);
    out.F32(material.specularPower);
    WriteFloats(out, material.emissive);
  }
  for (const MeshNode& node : _mesh.nodes)
  {
    out.Text8(node.name);
    out.U32(node.parent.value_or(NO_PARENT));
    WriteFloats(out, node.restTransform);
    WriteFloats(out, node.boundsMin);
    WriteFloats(out, node.boundsMax);
    out.F32(node.sphereRadius);
    out.U32(static_cast<std::uint32_t>(node.draws.size()));
    for (const MeshDraw& draw : node.draws)
    {
      out.U32(draw.material);
      out.U32(draw.firstVertex);
      out.U32(draw.vertexCount);
    }
  }
  for (const MeshSocket& socket : _mesh.sockets)
  {
    out.U8(static_cast<std::uint8_t>(socket.kind));
    out.U32(socket.index);
    out.U32(socket.node);
    WriteFloats(out, socket.transform);
  }
  for (const MeshVertex& vertex : _mesh.vertices)
  {
    WriteFloats(out, vertex.position);
    WriteFloats(out, vertex.normal);
    for (const std::uint8_t channel : vertex.color)
    {
      out.U8(channel);
    }
    out.U8(vertex.tint);
    out.U8(vertex.corner);
    out.U16(0);
  }
  return out.Take();
}

std::expected<MeshAsset, std::string> ReadMeshAsset(std::span<const std::byte> _entry)
{
  CookedReader in(_entry);
  if (const std::uint32_t version = in.U32(); version != MESH_ASSET_VERSION)
  {
    return std::unexpected(std::format("the cooked mesh is version {}; this build reads version {} only", version, MESH_ASSET_VERSION));
  }
  const std::uint32_t materials = in.U32();
  const std::uint32_t nodes = in.U32();
  const std::uint32_t sockets = in.U32();
  const std::uint32_t vertices = in.U32();
  if (in.Failed() || nodes > MESH_MAX_NODES || sockets > MESH_MAX_SOCKETS || materials > in.Remaining() ||
      std::uint64_t{vertices} * MESH_VERTEX_BYTES > in.Remaining())
  {
    return std::unexpected(std::string("the cooked mesh's counts pass its capacity or its bytes"));
  }
  MeshAsset mesh;
  mesh.materials.resize(materials);
  for (MeshMaterial& material : mesh.materials)
  {
    material.name = std::string(in.Text8());
    const std::uint8_t shading = in.U8();
    if (shading > static_cast<std::uint8_t>(MeshShading::Unlit))
    {
      return std::unexpected(std::format("a material's shading {} is unknown", shading));
    }
    material.shading = static_cast<MeshShading>(shading);
    ReadFloats(in, material.specular);
    material.specularPower = in.F32();
    ReadFloats(in, material.emissive);
  }
  mesh.nodes.resize(nodes);
  for (std::uint32_t n = 0; n < nodes; ++n)
  {
    MeshNode& node = mesh.nodes[n];
    node.name = std::string(in.Text8());
    if (const std::uint32_t parent = in.U32(); parent != NO_PARENT)
    {
      if (parent >= n)
      {
        return std::unexpected(std::format("node {} names the parent {}, which does not come before it", n, parent));
      }
      node.parent = parent;
    }
    ReadFloats(in, node.restTransform);
    ReadFloats(in, node.boundsMin);
    ReadFloats(in, node.boundsMax);
    node.sphereRadius = in.F32();
    const std::uint32_t draws = in.U32();
    if (draws > in.Remaining())
    {
      return std::unexpected(std::format("node {} states more draws than the entry holds", n));
    }
    node.draws.resize(draws);
    for (MeshDraw& draw : node.draws)
    {
      draw.material = in.U32();
      draw.firstVertex = in.U32();
      draw.vertexCount = in.U32();
      if (draw.material >= materials || draw.firstVertex > vertices || draw.vertexCount > vertices - draw.firstVertex ||
          draw.vertexCount % 3 != 0)
      {
        return std::unexpected(std::format("a draw of node {} lies outside the materials or the vertices", n));
      }
    }
  }
  mesh.sockets.resize(sockets);
  for (MeshSocket& socket : mesh.sockets)
  {
    const std::uint8_t kind = in.U8();
    socket.index = in.U32();
    socket.node = in.U32();
    ReadFloats(in, socket.transform);
    if (kind >= SOCKET_KIND_COUNT || socket.node >= nodes)
    {
      return std::unexpected(std::format("a socket of kind {} on node {} names no kind or node", kind, socket.node));
    }
    socket.kind = static_cast<SocketKind>(kind);
  }
  mesh.vertices.resize(vertices);
  for (MeshVertex& vertex : mesh.vertices)
  {
    ReadFloats(in, vertex.position);
    ReadFloats(in, vertex.normal);
    for (std::uint8_t& channel : vertex.color)
    {
      channel = in.U8();
    }
    vertex.tint = in.U8();
    vertex.corner = in.U8();
    if (in.U16() != 0 || vertex.corner > 2)
    {
      return std::unexpected(std::string("a vertex's corner or reserved bytes are not those of a cooked vertex"));
    }
  }
  if (!in.Finished())
  {
    return std::unexpected(std::string("the cooked mesh's bytes do not end where its counts do"));
  }
  return mesh;
}

} // namespace Neuron
