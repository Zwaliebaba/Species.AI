#pragma once

#include "ActorGrid.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "ObstacleGrid.h"
#include "SimulationRecord.h"
#include "StableId.h"
#include "StructureRecords.h"
#include "TerrainField.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace Species
{

// The MVP's structure kinds (MVP-021, MVP-030, MVP-034), a state value that is never renumbered.
enum class StructureKind : std::uint8_t
{
  Wellspring = 0,
  ResourceSite = 1,
  ResourceSiteCentral = 2,
  Dwelling = 3,
  Storehouse = 4
};

inline constexpr std::uint64_t STRUCTURE_KIND_MAX = 4;

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
};

// The place of a kind's mesh among the structure records (STRUCTURE_RECORD_KEYS), its simulation data record (ADR-129)
// and the collision asset that record names, or null.
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
// triangles, or its sphere without one) and set on the terrain (R-BLDG-020). Its id is the caller's.
[[nodiscard]] Structure MakeStructure(StructureKind _kind, std::optional<std::uint8_t> _owner, std::optional<std::uint32_t> _resourceSite,
                                      double _x, double _z, const Neuron::Vec3& _front, const Neuron::SimulationRecord& _record,
                                      const Neuron::CollisionAsset* _collision, const Neuron::TerrainField& _field,
                                      const TuningMvp& _tuning);

// The structure's hit volume as the obstacle grid and the pushes test it (ADR-103, ADR-131): its bounding sphere, and
// _collision's triangles at its transform when it has one (R-SIM-123).
[[nodiscard]] ObstacleShape ShapeOf(const Structure& _structure, const Neuron::CollisionAsset* _collision);

// A structure canonically, as the region's state holds it (ADR-081 item 7), and the values Write wrote; Read is false
// when they do not read whole or name a kind this build does not have.
void WriteStructure(Neuron::BitWriter& _writer, const Structure& _structure);
[[nodiscard]] bool ReadStructure(Neuron::BitReader& _reader, Structure& _structure);

} // namespace Species
