#include "pch.h"

#include "StateValues.h"
#include "Structure.h"

#include <cmath>
#include <limits>

namespace Species
{

namespace
{

// A region's team slots are numbered by a byte (R-SIM-221), so a port holds at most one count per slot.
constexpr std::uint64_t RESERVATION_SLOTS_MAX = std::uint64_t{std::numeric_limits<std::uint8_t>::max()} + 1;
// The most ports a structure reads from its mesh (AST-038's numbering of two digits).
constexpr std::uint64_t PORTS_MAX = 99;

Neuron::Vec3 Horizontal(const Neuron::Vec3& _v, const Neuron::Vec3& _fallback) noexcept
{
  return Neuron::NormalizedOr({_v.x, 0.0, _v.z}, _fallback);
}

// The right of a horizontal front, with +y up: up × front.
Neuron::Vec3 RightOf(const Neuron::Vec3& _front) noexcept
{
  return {_front.z, 0.0, -_front.x};
}

void WriteOptionalTeam(Neuron::BitWriter& _writer, const std::optional<std::uint8_t>& _team)
{
  _writer.WriteBool(_team.has_value());
  if (_team)
  {
    _writer.WriteBits(_team.value_or(0), 8);
  }
}

std::optional<std::uint8_t> ReadOptionalTeam(Neuron::BitReader& _reader)
{
  if (!_reader.ReadBool())
  {
    return std::nullopt;
  }
  return static_cast<std::uint8_t>(_reader.ReadBits(8));
}

} // namespace

std::size_t RecordPlaceOf(StructureKind _kind) noexcept
{
  switch (_kind)
  {
  case StructureKind::ResourceSite:
    return 3;
  case StructureKind::ResourceSiteCentral:
    return 4;
  case StructureKind::Dwelling:
    return 1;
  case StructureKind::Storehouse:
    return 2;
  case StructureKind::Wellspring:
    break;
  }
  return 0;
}

const Neuron::SimulationRecord& RecordOf(const StructureRecords& _records, StructureKind _kind) noexcept
{
  return RecordAt(_records, RecordPlaceOf(_kind));
}

const Neuron::CollisionAsset* CollisionOf(const StructureRecords& _records, StructureKind _kind) noexcept
{
  const std::optional<Neuron::CollisionAsset>& collision = _records.collisions[RecordPlaceOf(_kind)];
  return collision ? &*collision : nullptr;
}

Neuron::Vec3 StructurePoint(const Structure& _structure, const std::array<double, 3>& _meshPoint) noexcept
{
  return _structure.position + StructureDirection(_structure, _meshPoint);
}

Neuron::Vec3 StructureDirection(const Structure& _structure, const std::array<double, 3>& _meshDirection) noexcept
{
  const Neuron::Vec3 right = RightOf(_structure.front);
  return right * _meshDirection[0] + Neuron::Vec3{0.0, _meshDirection[1], 0.0} + _structure.front * _meshDirection[2];
}

Structure MakeStructure(StructureKind _kind, std::optional<std::uint8_t> _owner, std::optional<std::uint32_t> _resourceSite, double _x,
                        double _z, const Neuron::Vec3& _front, const Neuron::SimulationRecord& _record,
                        const Neuron::CollisionAsset* _collision, const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  Structure structure;
  structure.kind = _kind;
  structure.owner = _owner;
  structure.resourceSite = _resourceSite;
  structure.position = {_x, _field.Height(_x, _z), _z};
  structure.front = Horizontal(_front, {1.0, 0.0, 0.0}); // R-BLDG-014: the default front is +x
  // R-BLDG-014: the bounding sphere at the structure's transform, or its position and the default radius.
  if (_record.bounds)
  {
    const Neuron::BoundingSphere& bounds = _record.bounds.value_or(Neuron::BoundingSphere{});
    structure.center = StructurePoint(structure, {double{bounds.center[0]}, double{bounds.center[1]}, double{bounds.center[2]}});
    structure.radius = double{bounds.radius};
  }
  else
  {
    structure.center = structure.position;
    structure.radius = _tuning.ref.sim.structure.defaultRadius;
  }
  // R-BLDG-016 and R-BLDG-020: a port per port socket, in index order, pushed clear by the structure push of R-BLDG-033
  // (while the port's sphere hits the structure, a step directly away from its position) and set on the terrain.
  const ObstacleShape shape = ShapeOf(structure, _collision);
  const double clearance = _tuning.ref.bldg.port.clearanceWu;
  const double step = _tuning.ref.bldg.structure.pushStepWu;
  const auto pushesMax = static_cast<std::int64_t>(
    std::ceil((structure.radius + clearance + Neuron::Length(structure.center - structure.position)) / step) + 1.0);
  for (const Neuron::SimulationSocket* socket : Neuron::SocketsOf(_record, Neuron::SocketKind::Port))
  {
    StructurePort port;
    port.socket = socket->index;
    Neuron::Vec3 position = StructurePoint(structure, Neuron::RestPositionOf(_record, *socket));
    for (std::int64_t push = 0; push < pushesMax && SphereHits(shape, position, clearance); ++push)
    {
      position = position + Neuron::NormalizedOr(position - structure.position, structure.front) * step;
    }
    position.y = _field.Height(position.x, position.z);
    port.position = position;
    port.front = Horizontal(StructureDirection(structure, Neuron::RestFrontOf(_record, *socket)), structure.front);
    structure.ports.push_back(std::move(port));
  }
  return structure;
}

ObstacleShape ShapeOf(const Structure& _structure, const Neuron::CollisionAsset* _collision)
{
  ObstacleShape shape{_structure.center, _structure.radius, {}, std::nullopt};
  if (_collision != nullptr)
  {
    for (const Neuron::CollisionNode& node : _collision->nodes)
    {
      for (std::size_t corner = 0; corner + 2 < node.corners.size(); corner += 3)
      {
        std::array<Neuron::Vec3, 3> triangle;
        for (std::size_t k = 0; k < 3; ++k)
        {
          const std::array<float, 3>& point = node.corners[corner + k];
          triangle[k] = StructurePoint(_structure, {double{point[0]}, double{point[1]}, double{point[2]}});
        }
        shape.triangles.push_back(triangle);
      }
    }
  }
  return shape;
}

void WriteStructure(Neuron::BitWriter& _writer, const Structure& _structure)
{
  _writer.WriteBits(_structure.id, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(_structure.kind), STRUCTURE_KIND_MAX);
  WriteOptionalTeam(_writer, _structure.owner);
  _writer.WriteBool(_structure.resourceSite.has_value());
  if (_structure.resourceSite)
  {
    _writer.WriteBits(_structure.resourceSite.value_or(0), 32);
  }
  WriteVec3(_writer, _structure.position);
  WriteVec3(_writer, _structure.front);
  WriteVec3(_writer, _structure.center);
  WriteDouble(_writer, _structure.radius);
  _writer.WriteBounded(_structure.ports.size(), PORTS_MAX);
  for (const StructurePort& port : _structure.ports)
  {
    _writer.WriteBits(port.socket, 32);
    _writer.WriteBool(port.occupant.has_value());
    if (port.occupant)
    {
      const ActorRef& occupant = port.occupant.value_or(ActorRef{});
      _writer.WriteBits(occupant.id, 64);
      _writer.WriteBits(occupant.slot, 32);
      _writer.WriteBits(occupant.team, 8);
    }
    WriteVec3(_writer, port.position);
    WriteVec3(_writer, port.front);
    _writer.WriteBounded(port.reservations.size(), RESERVATION_SLOTS_MAX);
    for (const std::uint8_t count : port.reservations)
    {
      _writer.WriteBits(count, 8);
    }
  }
  _writer.WriteBool(_structure.built);
  _writer.WriteSigned(_structure.workTicks, 64);
  _writer.WriteSigned(_structure.costMilliRu, 64);
  _writer.WriteBool(_structure.active);
}

bool ReadStructure(Neuron::BitReader& _reader, Structure& _structure)
{
  _structure.id = _reader.ReadBits(64);
  _structure.kind = static_cast<StructureKind>(_reader.ReadBounded(STRUCTURE_KIND_MAX));
  _structure.owner = ReadOptionalTeam(_reader);
  _structure.resourceSite = std::nullopt;
  if (_reader.ReadBool())
  {
    _structure.resourceSite = static_cast<std::uint32_t>(_reader.ReadBits(32));
  }
  bool finite = ReadVec3(_reader, _structure.position);
  finite = ReadVec3(_reader, _structure.front) && finite;
  finite = ReadVec3(_reader, _structure.center) && finite;
  finite = ReadDouble(_reader, _structure.radius) && finite;
  const std::uint64_t ports = _reader.ReadBounded(PORTS_MAX);
  _structure.ports.clear();
  for (std::uint64_t index = 0; index < ports && !_reader.Failed(); ++index)
  {
    StructurePort& port = _structure.ports.emplace_back();
    port.socket = static_cast<std::uint32_t>(_reader.ReadBits(32));
    if (_reader.ReadBool())
    {
      ActorRef occupant;
      occupant.id = _reader.ReadBits(64);
      occupant.slot = static_cast<std::uint32_t>(_reader.ReadBits(32));
      occupant.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
      port.occupant = occupant;
    }
    finite = ReadVec3(_reader, port.position) && finite;
    finite = ReadVec3(_reader, port.front) && finite;
    port.reservations.resize(_reader.ReadBounded(RESERVATION_SLOTS_MAX));
    for (std::uint8_t& count : port.reservations)
    {
      count = static_cast<std::uint8_t>(_reader.ReadBits(8));
    }
  }
  _structure.built = _reader.ReadBool();
  _structure.workTicks = _reader.ReadSigned(64);
  _structure.costMilliRu = _reader.ReadSigned(64);
  _structure.active = _reader.ReadBool();
  return !_reader.Failed() && finite && _structure.id != 0 && _structure.radius >= 0.0 && _structure.workTicks >= 0 &&
         _structure.costMilliRu >= 0;
}

} // namespace Species
