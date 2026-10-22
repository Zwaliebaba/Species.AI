#pragma once

#include "BuildReport.h"
#include "LooseContent.h"

#include <cstddef>
#include <filesystem>
#include <vector>

namespace ContentBuild
{

/// What one run of the content build cooked, before it is written: every entry, and how many manifest rows have no
/// authored file yet.
struct BuildResult
{
  std::vector<Neuron::CookedEntry> entries;
  std::size_t rowsWithoutFile = 0;
  std::size_t placeholders = 0; // the rows without a file that a stand-in fills (ADR-140)
};

// The content build of ADR-108: reads the asset manifest and every source under the repository's GameData/ in name
// order, and cooks each by its row or folder: a texture checked and packed as authored, a CMO mesh or collision asset
// cooked, a data record cooked, and a string table cooked against the system fonts of its font roles (ADR-147). A file
// that belongs to no row and is no record is a stray (AST-013). The pairing pass then checks each collidable mesh
// against its collision asset and writes the simulation data records (ADR-129). It writes nothing; every fault is in the
// report.
[[nodiscard]] BuildResult CookContent(const std::filesystem::path& _root, BuildReport& _report);

} // namespace ContentBuild
