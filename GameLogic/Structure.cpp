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

void WriteOptionalActor(Neuron::BitWriter& _writer, const std::optional<ActorRef>& _actor)
{
  _writer.WriteBool(_actor.has_value());
  if (_actor)
  {
    const ActorRef& actor = _actor.value_or(ActorRef{});
    _writer.WriteBits(actor.id, 64);
    _writer.WriteBits(actor.slot, 32);
    _writer.WriteBits(actor.team, 8);
  }
}

std::optional<ActorRef> ReadOptionalActor(Neuron::BitReader& _reader)
{
  if (!_reader.ReadBool())
  {
    return std::nullopt;
  }
  ActorRef actor;
  actor.id = _reader.ReadBits(64);
  actor.slot = static_cast<std::uint32_t>(_reader.ReadBits(32));
  actor.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  return actor;
}

void WriteTurret(Neuron::BitWriter& _writer, const TurretState& _turret)
{
  WriteOptionalTeam(_writer, _turret.team);
  _writer.WriteSigned(_turret.health, 32);
  _writer.WriteSigned(_turret.recountTicks, 32);
  _writer.WriteSigned(_turret.searchTicks, 32);
  _writer.WriteSigned(_turret.fireTicks, 32);
  _writer.WriteSigned(_turret.hoseTicks, 32);
  _writer.WriteBits(_turret.barrelCursor, 8);
  WriteOptionalActor(_writer, _turret.target);
  WriteVec3(_writer, _turret.aimPoint);
  WriteVec3(_writer, _turret.headFront);
  WriteVec3(_writer, _turret.barrelUp);
  _writer.WriteBool(_turret.swept);
}

bool ReadTurret(Neuron::BitReader& _reader, TurretState& _turret)
{
  _turret.team = ReadOptionalTeam(_reader);
  _turret.health = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _turret.recountTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _turret.searchTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _turret.fireTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _turret.hoseTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _turret.barrelCursor = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _turret.target = ReadOptionalActor(_reader);
  bool finite = ReadVec3(_reader, _turret.aimPoint);
  finite = ReadVec3(_reader, _turret.headFront) && finite;
  finite = ReadVec3(_reader, _turret.barrelUp) && finite;
  _turret.swept = _reader.ReadBool();
  return finite;
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
  case StructureKind::Turret:
    return 5;
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
  const std::optional<Neuron::CollisionAsset>& collision =
    _records.collisions[_kind == StructureKind::Turret ? TURRET_HEAD_PLACE : RecordPlaceOf(_kind)];
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
  if (_kind == StructureKind::Turret)
  {
    structure.center = structure.position;
    structure.radius = _tuning.ref.bldg.turret.radius;
    structure.turret = NewTurret(structure, _tuning);
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

TurretState NewTurret(const Structure& _structure, const TuningMvp& _tuning) noexcept
{
  TurretState turret;
  turret.health = static_cast<std::int32_t>(_tuning.ref.bldg.turret.health);
  turret.aimPoint = _structure.position + _structure.front * _tuning.ref.bldg.turret.sweepDistanceBase;
  turret.headFront = _structure.front;
  turret.barrelUp = _structure.front;
  return turret;
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

ObstacleShape StructureShape(const Structure& _structure, const StructureRecords* _records, const TuningMvp& _tuning)
{
  ObstacleShape shape = ShapeOf(_structure, _records != nullptr ? CollisionOf(*_records, _structure.kind) : nullptr);
  if (_structure.kind != StructureKind::Turret)
  {
    return shape;
  }
  const auto& turret = _tuning.ref.bldg.turret;
  shape.rayCenter = _structure.position + Neuron::Vec3{0.0, turret.raySphereHeight, 0.0};
  shape.rayRadius = turret.raySphereRadius;
  if (_records == nullptr)
  {
    return shape;
  }
  const std::optional<Neuron::CollisionAsset>& barrel = _records->collisions[TURRET_BARREL_PLACE];
  const std::vector<const Neuron::SimulationSocket*> mounts = Neuron::SocketsOf(_records->turretHead, Neuron::SocketKind::BarrelMount);
  if (!barrel || mounts.empty())
  {
    return shape;
  }
  // The barrels at rest: the barrel set's own space carried to the head's barrel mount (ADR-133).
  const std::array<double, 3> mount = Neuron::RestPositionOf(_records->turretHead, *mounts.front());
  for (const Neuron::CollisionNode& node : barrel->nodes)
  {
    for (std::size_t corner = 0; corner + 2 < node.corners.size(); corner += 3)
    {
      std::array<Neuron::Vec3, 3> triangle;
      for (std::size_t k = 0; k < 3; ++k)
      {
        const std::array<float, 3>& point = node.corners[corner + k];
        triangle[k] = StructurePoint(_structure, {mount[0] + double{point[0]}, mount[1] + double{point[1]}, mount[2] + double{point[2]}});
      }
      shape.triangles.push_back(triangle);
    }
  }
  return shape;
}

Neuron::Vec3 DropPointOf(const Structure& _structure, const Neuron::SimulationRecord& _record, const ObstacleShape& _shape,
                         const Neuron::TerrainField& _field, const TuningMvp& _tuning)
{
  const auto kind = _structure.kind == StructureKind::Wellspring ? Neuron::SocketKind::Door : Neuron::SocketKind::Entrance;
  const std::vector<const Neuron::SimulationSocket*> sockets = Neuron::SocketsOf(_record, kind);
  Neuron::Vec3 point =
    sockets.empty() ? _structure.position : StructurePoint(_structure, Neuron::RestPositionOf(_record, *sockets.front()));
  point.y = _field.Height(point.x, point.z);
  if (sockets.empty())
  {
    return point;
  }
  // A Worker within the snap distance of the point is clear of the shape when the point is clear by the sum: the Worker
  // is tested on the terrain, so the point is too.
  const auto& worker = _tuning.ref.unit.worker;
  const double clearance = worker.structureClearRadius + worker.snapDistance;
  const double step = _tuning.ref.bldg.structure.pushStepWu;
  const Neuron::Vec3 away = Horizontal(point - _structure.position, _structure.front);
  const auto pushesMax = static_cast<std::int64_t>(
    std::ceil((_structure.radius + clearance + Neuron::Length(_structure.center - _structure.position)) / step) + 1.0);
  for (std::int64_t push = 0; push < pushesMax && SphereHits(_shape, point, clearance); ++push)
  {
    point = point + away * step;
    point.y = _field.Height(point.x, point.z);
  }
  return point;
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
    WriteOptionalActor(_writer, port.occupant);
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
  // A turret's own state, which only a turret has (ADR-133).
  if (_structure.kind == StructureKind::Turret)
  {
    WriteTurret(_writer, _structure.turret.value_or(TurretState{}));
  }
  _writer.WriteBits(_structure.completedTick, 64);
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
    port.occupant = ReadOptionalActor(_reader);
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
  _structure.turret.reset();
  if (_structure.kind == StructureKind::Turret)
  {
    TurretState turret;
    finite = ReadTurret(_reader, turret) && finite;
    _structure.turret = turret;
  }
  _structure.completedTick = _reader.ReadBits(64);
  return !_reader.Failed() && finite && _structure.id != 0 && _structure.radius >= 0.0 && _structure.workTicks >= 0 &&
         _structure.costMilliRu >= 0;
}

} // namespace Species
