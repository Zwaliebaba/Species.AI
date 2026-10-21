#pragma once

#include "AssetError.h"
#include "CollisionAsset.h"
#include "LooseContent.h"
#include "SimulationRecord.h"
#include "TuningMvp.h"

#include <array>
#include <expected>
#include <optional>
#include <string_view>

namespace Species
{

/// The simulation data records of the MVP's structure meshes (AST-046, ADR-129): the server loads them when it starts,
/// and the simulation reads a structure's ports, entrance, door, muzzles and bounding sphere from them, never from a
/// render mesh (AST-043).
// The keys of the records, in StructureRecords' order (AST-014, ADR-129).
inline constexpr std::array<std::string_view, 8> STRUCTURE_RECORD_KEYS{
  "meshes/wellspring@sim",          "meshes/dwelling@sim",    "meshes/storehouse@sim", "meshes/resourcesite@sim",
  "meshes/resourcesitecentral@sim", "meshes/turretmount@sim", "meshes/turrethead@sim", "meshes/turretbarrelgun@sim"};

struct StructureRecords
{
  Neuron::SimulationRecord wellspring;
  Neuron::SimulationRecord dwelling;
  Neuron::SimulationRecord storehouse;
  Neuron::SimulationRecord resourceSite;
  Neuron::SimulationRecord resourceSiteCentral;
  Neuron::SimulationRecord turretMount;
  Neuron::SimulationRecord turretHead;
  Neuron::SimulationRecord turretBarrel;
  // The collision asset each record names (AST-044, R-SIM-123), in the records' order; none for a record that names
  // none, and none for every record that the tests write (ADR-131).
  std::array<std::optional<Neuron::CollisionAsset>, STRUCTURE_RECORD_KEYS.size()> collisions;
};

// The record of StructureRecords' member at a place of STRUCTURE_RECORD_KEYS.
[[nodiscard]] Neuron::SimulationRecord& RecordAt(StructureRecords& _records, std::size_t _place) noexcept;
[[nodiscard]] const Neuron::SimulationRecord& RecordAt(const StructureRecords& _records, std::size_t _place) noexcept;

// Loads every record from the mounted content (MVP-099 for the structures' shapes), and the collision asset each
// names: missing when the content lacks one, damaged when one's bytes are not a record or a collision asset.
[[nodiscard]] std::expected<StructureRecords, Neuron::AssetError> LoadStructureRecords(const Neuron::LooseContent& _content);

// The records against the rules the simulation reads them by (ADR-130, ADR-132): the Wellspring's has a door socket
// (R-ECON-043, MVP-021), a resource site's mvp.resource.ports_per_unit port sockets and the central resource site's that
// many for each of its wld.resource.central_factor units (MVP-030), and the dwelling's and the storehouse's as many as
// mvp.construction.ports_dwelling and ports_storehouse (MVP-033). None when they hold; otherwise the first record that
// does not, as damaged.
[[nodiscard]] std::optional<Neuron::AssetError> CheckStructureRecords(const StructureRecords& _records, const TuningMvp& _tuning);

} // namespace Species
