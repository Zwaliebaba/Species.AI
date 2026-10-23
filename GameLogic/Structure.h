#pragma once

#include "ActorGrid.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "ObstacleGrid.h"
#include "SimulationRecord.h"
#include "StableId.h"
#include "StructureKind.h"
#include "StructureRecords.h"
#include "TerrainField.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

// A structure by stable id and slot, which resolves only while the structure holds the slot (R-SIM-325's rule).
struct StructureRef
{
  StableId id = 0;
  std::uint32_t slot = 0;

  friend constexpr bool operator==(const StructureRef& _a, const StructureRef& _b) noexcept = default;
};

/// A structure port (R-BLDG-019): the port socket it was made from, the Worker operating it, its world transform, and
/// one reservation count for each team slot. The counts are held for the slots up to the highest that has reserved it;
/// a slot beyond them counts 0.
struct StructurePort
{
  std::uint32_t socket = 0; // the index of its port socket (AST-038)
  std::optional<ActorRef> occupant;
  Neuron::Vec3 position; // R-BLDG-020: pushed clear of its structure, at the terrain's height
  Neuron::Vec3 front;    // the socket's front in the world, horizontal and of unit length
  std::vector<std::uint8_t> reservations;
};

/// A gun turret's own state (R-BLDG-405 to R-BLDG-423, ADR-133): the team that mans it, which may differ from the
/// settlement it belongs to, its health, its three countdowns, the fire steps it still hoses, its barrel cursor, its
/// target, its aim point, its head's horizontal front and its barrels' blended up vector, and whether it has drawn its
/// first idle aim point.
struct TurretState
{
  std::optional<std::uint8_t> team; // R-BLDG-408
  std::int32_t health = 0;          // R-BLDG-422
  std::int32_t recountTicks = 0;    // R-BLDG-408: until the next ownership evaluation, which the first advance runs
  std::int32_t searchTicks = 0;     // R-BLDG-411: until the next target search, which the first eligible advance runs
  std::int32_t fireTicks = 0;       // R-BLDG-418: the fire timer
  std::int32_t hoseTicks = 0;       // R-BLDG-415: the fire steps still run at the last aim point without a target
  std::uint8_t barrelCursor = 0;    // R-BLDG-418
  std::optional<ActorRef> target;   // R-BLDG-411
  Neuron::Vec3 aimPoint;            // R-BLDG-412, R-BLDG-414, R-BLDG-416
  Neuron::Vec3 headFront;           // R-BLDG-416: horizontal and of unit length
  Neuron::Vec3 barrelUp;            // R-BLDG-416: the barrels' up vector, blended towards the aim direction
  bool swept = false;               // R-BLDG-412: its first idle aim point is drawn
};

/// A structure of a region (R-BLDG-014, R-BLDG-029, ADR-130): its stable id and kind, the team of the settlement it
/// belongs to (none for a resource site, which is world data, WLD-084), the resource site of the region's terrain it
/// stands for, its position on the terrain and its horizontal front, the bounding sphere of its collision asset at its
/// transform, its ports in socket order, and, from M2 task T5 (ADR-132), whether it is built or still a construction
/// site, the Worker-ticks given to it, what it cost and whether it is active (MVP-033 to MVP-035).
struct Structure
{
  StableId id = 0;
  StructureKind kind = StructureKind::Wellspring;
  std::optional<std::uint8_t> owner;
  std::optional<std::uint32_t> resourceSite; // its place in RegionTerrain::resourceSites
  Neuron::Vec3 position;
  Neuron::Vec3 front;
  Neuron::Vec3 center;
  double radius = 0.0;
  std::vector<StructurePort> ports;
  bool built = true;
  std::int64_t workTicks = 0;
  std::int64_t costMilliRu = 0;
  bool active = true;
  std::optional<TurretState> turret; // a turret's, from M2 task T5 (ADR-133)
  std::uint64_t completedTick = 0;   // MVP-035: the tick it was built; 0 for one the world placed (ADR-135)
};

// The places of the turret's head and gun barrel set among the structure records; the turret kind's own is its mount's.
inline constexpr std::size_t TURRET_HEAD_PLACE = 6;
inline constexpr std::size_t TURRET_BARREL_PLACE = 7;

// The place of a kind's mesh among the structure records (STRUCTURE_RECORD_KEYS), its simulation data record (ADR-129)
// and the collision asset that record names, or null; a turret's is its head's, as its mount has no triangles (AST-045).
[[nodiscard]] std::size_t RecordPlaceOf(StructureKind _kind) noexcept;
[[nodiscard]] const Neuron::SimulationRecord& RecordOf(const StructureRecords& _records, StructureKind _kind) noexcept;
[[nodiscard]] const Neuron::CollisionAsset* CollisionOf(const StructureRecords& _records, StructureKind _kind) noexcept;

// A point of a structure's mesh at rest in the world: the mesh's +z along the structure's front, +y up and +x to the
// front's right (ADR-130); and a direction of the mesh in the world the same way.
[[nodiscard]] Neuron::Vec3 StructurePoint(const Structure& _structure, const std::array<double, 3>& _meshPoint) noexcept;
[[nodiscard]] Neuron::Vec3 StructureDirection(const Structure& _structure, const std::array<double, 3>& _meshDirection) noexcept;

// R-SIM-118 and R-BLDG-029 for a structure that a rule creates: at (_x, _z) on the terrain, facing _front, its centre
// and radius from its record's bounding sphere at that transform (ref.sim.structure.default_radius when its record has
// none), and its ports built once from its record's port sockets, each pushed clear of its shape (_collision's
// triangles, or its sphere without one) and set on the terrain (R-BLDG-020). A turret's centre is its position and its
// radius ref.bldg.turret.radius (R-BLDG-407); it starts at full health with its head and barrels along its front, and
// its countdowns at 0 (ADR-133). Its id is the caller's.
[[nodiscard]] Structure MakeStructure(StructureKind _kind, std::optional<std::uint8_t> _owner, std::optional<std::uint32_t> _resourceSite,
                                      double _x, double _z, const Neuron::Vec3& _front, const Neuron::SimulationRecord& _record,
                                      const Neuron::CollisionAsset* _collision, const Neuron::TerrainField& _field,
                                      const TuningMvp& _tuning);

// A turret's state when it is made or its site is left (ADR-133): full health, its head and barrels along its front, its
// aim point ahead at the idle sweep's base distance, no team, and its countdowns at 0.
[[nodiscard]] TurretState NewTurret(const Structure& _structure, const TuningMvp& _tuning) noexcept;

// The structure's hit volume as the obstacle grid and the pushes test it (ADR-103, ADR-131): its bounding sphere, and
// _collision's triangles at its transform when it has one (R-SIM-123).
[[nodiscard]] ObstacleShape ShapeOf(const Structure& _structure, const Neuron::CollisionAsset* _collision);
// The hit volume of a structure of the region, from the records the region holds (null in a test without them): ShapeOf
// with its kind's collision asset, and for a turret its head's and its barrels' triangles at rest, the barrels at the
// head's barrel mount, and its ray sphere (R-BLDG-407, ADR-133).
[[nodiscard]] ObstacleShape StructureShape(const Structure& _structure, const StructureRecords* _records, const TuningMvp& _tuning);

// MVP-031: where a Worker puts a load down at a drop-off, the Wellspring's door or a storehouse's entrance at rest, on the
// terrain; the structure's position on the terrain for a kind with neither. As a port is (R-BLDG-020), it is pushed by
// steps of ref.bldg.structure.push_step_wu directly away from the structure's position until a Worker anywhere within
// ref.unit.worker.snap_distance of it is clear of _shape by ref.unit.worker.structure_clear_radius, so the Worker's
// structure push (R-UNIT-114) never holds it short of arriving (ADR-180).
[[nodiscard]] Neuron::Vec3 DropPointOf(const Structure& _structure, const Neuron::SimulationRecord& _record, const ObstacleShape& _shape,
                                       const Neuron::TerrainField& _field, const TuningMvp& _tuning);

// A structure canonically, as the region's state holds it (ADR-081 item 7), and the values Write wrote; Read is false
// when they do not read whole or name a kind this build does not have.
void WriteStructure(Neuron::BitWriter& _writer, const Structure& _structure);
[[nodiscard]] bool ReadStructure(Neuron::BitReader& _reader, Structure& _structure);

} // namespace Species
