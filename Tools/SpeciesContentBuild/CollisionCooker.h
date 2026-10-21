#pragma once

#include "AssetManifest.h"
#include "BuildReport.h"
#include "LooseContent.h"

#include <optional>
#include <span>
#include <string_view>

namespace ContentBuild
{

// The manifest subcategory of a collision asset's row (AST-044).
inline constexpr std::string_view COLLISION_SUBCATEGORY = "collision";
// The material a collision source's triangles carry, which nothing reads (ADR-129).
inline constexpr std::string_view COLLISION_MATERIAL = "collision";

// The collision cooker of AST-077 (ADR-129): imports a CMO collision source with the checks of ADR-019 and cooks its
// triangles, node by node, with each node's bounding sphere and the asset's, into a Collision entry flagged
// simulation-relevant (AST-098). A source with sockets, a socket bone or more triangles than its row's budget is refused.
// Its nodes answer to its render member's in the pairing pass (ADR-129). The entry, or none with the faults reported.
[[nodiscard]] std::optional<Neuron::CookedEntry> CookCollision(const ManifestRow& _row, std::string_view _file,
                                                               std::span<const std::byte> _bytes, BuildReport& _report);

} // namespace ContentBuild
