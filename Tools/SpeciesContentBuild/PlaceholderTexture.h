#pragma once

#include "AssetManifest.h"
#include "LooseContent.h"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace ContentBuild
{

// AST-082 and GameAssets.md §2.6 (owner decision O88, ADR-140): the stand-in of a texture row of the MVP's priority that
// has no authored file, at its row's runtime key, in its row's format, size and levels and marked as a placeholder
// (AST-083): for a generated figure, a white diamond in the upper part of the card; for a generated glyph, a white ring
// round a dot; for a generated panel, a flat grey panel; for a generated gradient, a white radial gradient; and for a
// variant derived by the cooker, its shape blurred. None for a row of another kind or a format the build does not write
// (BC7_UNORM_SRGB and BC4_UNORM), which keeps no stand-in.
[[nodiscard]] std::optional<Neuron::CookedEntry> MakePlaceholderTexture(const ManifestRow& _row);

// A level of a stand-in in a format the build writes, from its coverage, one value from 0 to 1 per texel in rows from
// the top: BC7 mode 6 blocks of the colour with the coverage as its alpha, or BC4 blocks of the coverage.
[[nodiscard]] std::vector<std::byte> EncodeBc7Coverage(std::span<const float> _coverage, std::uint32_t _width, std::uint32_t _height,
                                                       float _red, float _green, float _blue);
[[nodiscard]] std::vector<std::byte> EncodeBc4Coverage(std::span<const float> _coverage, std::uint32_t _width, std::uint32_t _height);

} // namespace ContentBuild
