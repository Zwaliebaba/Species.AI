#pragma once

#include "AssetManifest.h"
#include "BuildReport.h"
#include "LooseContent.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace ContentBuild
{

// Cook options fixed by the tool's version (AST-066): rnd.mesh.color_attenuation as a percentage (RND-160), the socket
// numbering of ast.mesh.socket_index_digits and ast.mesh.socket_first_number (AST-038), and the unit scale of every
// mesh until import records exist, one world unit per source unit (owner decision O73).
inline constexpr std::uint32_t MESH_COLOR_ATTENUATION_PERCENT = 85;
inline constexpr std::size_t SOCKET_INDEX_DIGITS = 2;
inline constexpr std::uint32_t SOCKET_FIRST_NUMBER = 1;
inline constexpr float MESH_UNIT_SCALE = 1.0F;

// The names that carry meaning in a CMO file (ADR-019 as amended by O64, AST-034, AST-038).
inline constexpr std::string_view SOCKET_PREFIX = "socket_";
inline constexpr std::string_view TEAM_MATERIAL = "team";
inline constexpr std::string_view UNLIT_MATERIAL = "unlit";

// The mesh cooker of AST-076: imports a CMO source with the checks of ADR-019, applies the Species rules (nodes and
// sockets, materials, faceting, tint marking, the row's triangle budget, node count, depth and socket list) and cooks
// the MeshAsset that the renderer loads. The entry, or none with the faults reported.
[[nodiscard]] std::optional<Neuron::CookedEntry> CookMesh(const ManifestRow& _row, std::string_view _file,
                                                          std::span<const std::byte> _bytes, BuildReport& _report);

} // namespace ContentBuild
