#pragma once

#include "AssetManifest.h"
#include "BuildReport.h"
#include "LooseContent.h"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace ContentBuild
{

/// A cooked mesh or collision asset as the pairing pass reads it: its row, its source file and its entry's place in
/// the run's entries.
struct CookedGeometry
{
  const ManifestRow* row = nullptr;
  std::string file;
  std::size_t entry = 0;
};

// The pairing pass of ADR-129, after every source is cooked. A render mesh is collidable when the manifest gives its
// asset a collision asset, `mesh.collision.<name>` for `mesh.<class>.<name>`, with a member in its member's place; its
// collision source must then be authored (AST-044), and a collision source must have its render member authored
// (AST-090), every node of it naming a node of that member (AST-077). Each render mesh with a simulation-relevant socket
// or a collision asset then gets its simulation data record (AST-043, AST-046), keyed `<mesh key>@sim` and flagged
// simulation-relevant (AST-098), appended to _entries.
void CookSimulationRecords(const AssetManifest& _manifest, std::span<const CookedGeometry> _meshes,
                           std::span<const CookedGeometry> _collisions, std::vector<Neuron::CookedEntry>& _entries, BuildReport& _report);

} // namespace ContentBuild
