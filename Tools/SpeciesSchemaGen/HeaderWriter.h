#pragma once

#include "SchemaModel.h"

#include <cstddef>
#include <expected>
#include <string>

namespace SchemaGen
{

// .clang-format's ColumnLimit: a generated line longer than it is refused, so the output is formatted as written.
inline constexpr std::size_t GENERATED_COLUMN_LIMIT = 140;

// The header of one schema (ADR-062): the record's types, its id and version, and its reader, in AGENTS.md style and as
// clang-format leaves it, with line feeds. The reason, when a line would pass the column limit.
[[nodiscard]] std::expected<std::string, std::string> WriteHeader(const SchemaModel& _model);

// The file the header is written to, relative to the repository root: <x-species-project>/<TypeName>.h.
[[nodiscard]] std::string HeaderPath(const SchemaModel& _model);

} // namespace SchemaGen
