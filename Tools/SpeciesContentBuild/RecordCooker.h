#pragma once

#include "BuildReport.h"
#include "LooseContent.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace ContentBuild
{

// The record cooker (ADR-061, ADR-098, ADR-108): reads a data record's authoring file with the strict parser and the
// reader its schema generated, and writes its cooked entry with the cooker its schema generated, so the runtime never
// parses it (DAT-010, DAT-050). The record type is the one the file's `schema` names, and the file lies in that type's
// folder (DAT-016). _file is the path from the repository root, _contentPath the path under GameData/. The entry, or
// none with every finding reported.
[[nodiscard]] std::optional<Neuron::CookedEntry> CookRecord(std::string_view _file, std::string_view _contentPath,
                                                            std::span<const std::byte> _bytes, BuildReport& _report);

} // namespace ContentBuild
