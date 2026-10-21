#pragma once

#include "MeshModel.h"

#include <vector>

namespace MeshKit
{

// The recipes of the MVP's structure meshes and their collision assets (M2 task T2, ADR-128): drafts that the owner
// approves or redesigns (O86). Each recipe's node names, socket names and triangle count answer to its row of
// Design/assets/asset_manifest.csv, which the content build checks; the port sockets are where the simulation's ports
// stand (AST-043, R-BLDG-020).
[[nodiscard]] std::vector<MeshModel> StructureRecipes();
[[nodiscard]] std::vector<MeshModel> TurretRecipes();

} // namespace MeshKit
