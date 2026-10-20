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

// ast.texture.max_dimension, ast.texture.block_dimension and ast.texture.ui_authoring_scale: cook options fixed by the
// tool's version (AST-066).
inline constexpr std::uint32_t TEXTURE_MAX_DIMENSION = 16384;
inline constexpr std::uint32_t TEXTURE_BLOCK_DIMENSION = 4;
inline constexpr std::uint32_t UI_AUTHORING_SCALE = 2;

// A texture authored as a DDS in its row's runtime format, checked against its row and packed as authored, its bytes
// unchanged (ADR-017 as amended by O61, O64 and O69): the format and its colour space, the size, the level count and the
// payload, every difference reported. The entry, or none when the texture or its row is at fault.
[[nodiscard]] std::optional<Neuron::CookedEntry> CheckTexture(const ManifestRow& _row, std::string_view _file,
                                                              std::span<const std::byte> _bytes, BuildReport& _report);

} // namespace ContentBuild
