#pragma once

#include "AssetManifest.h"
#include "BuildReport.h"
#include "FontAsset.h"
#include "LooseContent.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace ContentBuild
{

// DAT-217 as ADR-146 builds it: one language's string table from its authoring file (DAT-206 to DAT-209), its markers
// expanded in each input mode (DAT-211 to DAT-213), its lengths (UI-225) and its texts' message syntax (ADR-063)
// checked, and every character of each text and of its upper-cased form drawn by _face, the face of the language's
// font roles (UI-199 as O63 amends it). Every fault is reported, naming the file, the id and what is wrong; none
// when any is found.
[[nodiscard]] std::optional<Neuron::CookedEntry> CookStringTable(const ManifestRow& _row, std::string_view _file,
                                                                 std::span<const std::byte> _bytes, const Neuron::FontAsset& _face,
                                                                 BuildReport& _report);

} // namespace ContentBuild
