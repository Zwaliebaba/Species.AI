#pragma once

#include "AssetManifest.h"
#include "BuildReport.h"
#include "LooseContent.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace ContentBuild
{

/// A face's two members (DAT-219 as amended by O61 and O63): its one glyph sheet and its glyph table, each with its
/// row, its path from the repository root and its bytes.
struct FaceSources
{
  const ManifestRow* sheetRow = nullptr;
  std::string_view sheetFile;
  std::span<const std::byte> sheet;
  const ManifestRow* tableRow = nullptr;
  std::string_view tableFile;
  std::span<const std::byte> table;
};

// The font packer of ADR-034 as amended by O56 and O63: checks a face's sheet and glyph table against their rows and
// each other (DAT-219), and cooks the face's `Font` entry, one R8_UNORM page of the sheet's coverage and the glyphs
// sorted by code point. The entry, or none when a member is at fault.
[[nodiscard]] std::optional<Neuron::CookedEntry> PackFont(const FaceSources& _face, BuildReport& _report);

} // namespace ContentBuild
