#include "pch.h"

#include "CookedRecord.h"
#include "SimulationRecord.h"

#include <algorithm>
#include <cmath>
#include <format>

namespace Neuron
{

namespace
{

constexpr std::uint32_t NO_PARENT = 0xFFFFFFFF;

void WriteMatrix(CookedWriter& _out, const std::array<float, 16>& _matrix)
{
  for (const float value : _matrix)
  {
    _out.F32(value);
  }
}

bool ReadMatrix(CookedReader& _in, std::array<float, 16>& _matrix)
{
  bool finite = true;
  for (float& value : _matrix)
  {
    value = _in.F32();
    finite = finite && std::isfinite(value);
  }
  return finite;
}

void WriteSphere(CookedWriter& _out, const BoundingSphere& _sphere)
{
  for (const float value : _sphere.center)
  {
    _out.F32(value);
  }
  _out.F32(_sphere.radius);
}

bool ReadSphere(CookedReader& _in, BoundingSphere& _sphere)
{
  bool finite = true;
  for (float& value : _sphere.center)
  {
    value = _in.F32();
    finite = finite && std::isfinite(value);
  }
  _sphere.radius = _in.F32();
  return finite && std::isfinite(_sphere.radius) && _sphere.radius >= 0.0F;
}

} // namespace

std::vector<std::byte> WriteSimulationRecord(const SimulationRecord& _record)
{
  CookedWriter out;
  out.U32(SIMULATION_RECORD_VERSION);
  out.U32(static_cast<std::uint32_t>(_record.nodes.size()));
  out.U32(static_cast<std::uint32_t>(_record.sockets.size()));
  out.U32(static_cast<std::uint32_t>(_record.nodeSpheres.size()));
  out.Text8(_record.collision);
  out.U8(_record.bounds.has_value() ? 1 : 0);
  if (_record.bounds)
  {
    WriteSphere(out, _record.bounds.value_or(BoundingSphere{}));
  }
  for (const SimulationNode& node : _record.nodes)
  {
    out.Text8(node.name);
    out.U32(node.parent.value_or(NO_PARENT));
    WriteMatrix(out, node.restTransform);
  }
  for (const SimulationSocket& socket : _record.sockets)
  {
    out.U8(static_cast<std::uint8_t>(socket.kind));
    out.U32(socket.index);
    out.U32(socket.node);
    WriteMatrix(out, socket.transform);
  }
  for (const SimulationSphere& sphere : _record.nodeSpheres)
  {
    out.U32(sphere.node);
    WriteSphere(out, sphere.sphere);
  }
  return out.Take();
}

std::expected<SimulationRecord, std::string> ReadSimulationRecord(std::span<const std::byte> _entry)
{
  CookedReader in(_entry);
  if (const std::uint32_t version = in.U32(); version != SIMULATION_RECORD_VERSION)
  {
    return std::unexpected(
      std::format("the simulation data record is version {}; this build reads version {} only", version, SIMULATION_RECORD_VERSION));
  }
  const std::uint32_t nodes = in.U32();
  const std::uint32_t sockets = in.U32();
  const std::uint32_t spheres = in.U32();
  if (in.Failed() || nodes > MESH_MAX_NODES || sockets > MESH_MAX_SOCKETS || spheres > nodes)
  {
    return std::unexpected(std::string("the simulation data record's counts pass its capacity"));
  }
  SimulationRecord record;
  record.collision = std::string(in.Text8());
  const std::uint8_t hasBounds = in.U8();
  if (hasBounds > 1)
  {
    return std::unexpected(std::string("the simulation data record's bounds flag is neither 0 nor 1"));
  }
  if (hasBounds == 1)
  {
    BoundingSphere bounds;
    if (!ReadSphere(in, bounds))
    {
      return std::unexpected(std::string("the simulation data record's bounding sphere is not finite"));
    }
    record.bounds = bounds;
  }
  record.nodes.resize(nodes);
  for (std::uint32_t n = 0; n < nodes; ++n)
  {
    SimulationNode& node = record.nodes[n];
    node.name = std::string(in.Text8());
    if (const std::uint32_t parent = in.U32(); parent != NO_PARENT)
    {
      if (parent >= n)
      {
        return std::unexpected(std::format("node {} names the parent {}, which does not come before it", n, parent));
      }
      node.parent = parent;
    }
    if (!ReadMatrix(in, node.restTransform))
    {
      return std::unexpected(std::format("the rest transform of node {} is not finite", n));
    }
  }
  record.sockets.resize(sockets);
  for (SimulationSocket& socket : record.sockets)
  {
    const std::uint8_t kind = in.U8();
    socket.index = in.U32();
    socket.node = in.U32();
    const bool finite = ReadMatrix(in, socket.transform);
    if (kind >= SOCKET_KIND_COUNT || !SocketKindInfoOf(static_cast<SocketKind>(kind)).simulationRelevant || socket.node >= nodes || !finite)
    {
      return std::unexpected(
        std::format("a socket of kind {} on node {} is not a simulation-relevant socket of a node", kind, socket.node));
    }
    socket.kind = static_cast<SocketKind>(kind);
  }
  record.nodeSpheres.resize(spheres);
  for (SimulationSphere& sphere : record.nodeSpheres)
  {
    sphere.node = in.U32();
    if (!ReadSphere(in, sphere.sphere) || sphere.node >= nodes)
    {
      return std::unexpected(std::format("the sphere of node {} names no node or is not finite", sphere.node));
    }
  }
  if (!in.Finished())
  {
    return std::unexpected(std::string("the simulation data record's bytes do not end where its counts do"));
  }
  return record;
}

std::vector<const SimulationSocket*> SocketsOf(const SimulationRecord& _record, SocketKind _kind)
{
  std::vector<const SimulationSocket*> found;
  for (const SimulationSocket& socket : _record.sockets)
  {
    if (socket.kind == _kind)
    {
      found.push_back(&socket);
    }
  }
  std::ranges::sort(found, {}, &SimulationSocket::index);
  return found;
}

std::array<double, 3> RestPositionOf(const SimulationRecord& _record, const SimulationSocket& _socket)
{
  std::array<double, 3> position{double{_socket.transform[12]}, double{_socket.transform[13]}, double{_socket.transform[14]}};
  std::optional<std::uint32_t> node = _socket.node < _record.nodes.size() ? std::optional(_socket.node) : std::nullopt;
  while (node)
  {
    const std::array<float, 16>& m = _record.nodes[node.value_or(0)].restTransform;
    const std::array<double, 3> p = position;
    for (std::size_t axis = 0; axis < 3; ++axis)
    {
      position[axis] = p[0] * double{m[axis]} + p[1] * double{m[4 + axis]} + p[2] * double{m[8 + axis]} + double{m[12 + axis]};
    }
    node = _record.nodes[node.value_or(0)].parent;
  }
  return position;
}

} // namespace Neuron
