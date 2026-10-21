#pragma once

#include "AssetError.h"
#include "LooseContent.h"
#include "SimulationRecord.h"

#include <array>
#include <expected>
#include <string_view>

namespace Species
{

/// The simulation data records of the MVP's structure meshes (AST-046, ADR-129): the server loads them when it starts,
/// and the simulation reads a structure's ports, entrance, door, muzzles and bounding sphere from them, never from a
/// render mesh (AST-043).
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
};

// The keys of those records, in StructureRecords' order (AST-014, ADR-129).
inline constexpr std::array<std::string_view, 8> STRUCTURE_RECORD_KEYS{
  "meshes/wellspring@sim",          "meshes/dwelling@sim",    "meshes/storehouse@sim", "meshes/resourcesite@sim",
  "meshes/resourcesitecentral@sim", "meshes/turretmount@sim", "meshes/turrethead@sim", "meshes/turretbarrelgun@sim"};

// The record of StructureRecords' member at a place of STRUCTURE_RECORD_KEYS.
[[nodiscard]] Neuron::SimulationRecord& RecordAt(StructureRecords& _records, std::size_t _place) noexcept;

// Loads every record from the mounted content (MVP-099 for the structures' shapes): missing when the content lacks one,
// damaged when one's bytes are not a record.
[[nodiscard]] std::expected<StructureRecords, Neuron::AssetError> LoadStructureRecords(const Neuron::LooseContent& _content);

} // namespace Species
