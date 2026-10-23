#pragma once

#include "AssetManifest.h"
#include "BuildReport.h"
#include "LooseContent.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace ContentBuild
{

// aen.residency.stream_threshold: a group whose longest member is longer is streamed (AEN-072, AST-080); the MVP cooks
// no stream, so a member longer than this is a build error (ADR-171).
inline constexpr double STREAM_THRESHOLD_SECONDS = 20.0;
// aen.memory.interface_bank_budget: every resident member of the MVP is in the interface bank, held for the process's
// lifetime (AEN-074, AST-081, ADR-171).
inline constexpr std::size_t INTERFACE_BANK_BUDGET_BYTES = std::size_t{8} * 1024 * 1024;

// A manifest category whose rows are sample groups (AST table 3.10-A).
[[nodiscard]] bool IsSoundCategory(std::string_view _category);

// AST-079 and AST-080: an authored WAV source of a sound row cooked to its resident entry: 16-bit PCM at 48 kHz, each
// sample rounded to the nearest step with no dither. The build fails, naming the file, a source at another rate, of
// another encoding than 24-bit integer or 32-bit float PCM, with no frames, stereo when its row is positioned or with
// more than two channels, a row whose target format is not resident, or a member longer than the stream threshold.
[[nodiscard]] std::optional<Neuron::CookedEntry> CookSound(const ManifestRow& _row, std::string_view _file,
                                                           std::span<const std::byte> _bytes, BuildReport& _report);

// AST-082 to AST-084 (O98, ADR-171): the synthesized stand-in of a sound row of the MVP's priority with no authored
// file, whose placeholder is a synthesised tone: a recipe of its group's role, a pure function of the asset id and the
// member's number, cooked as an authored member is and marked as a placeholder. None for a row of another kind.
[[nodiscard]] std::optional<Neuron::CookedEntry> MakePlaceholderSound(const ManifestRow& _row, BuildReport& _report);

// AST-081 (ADR-171): the resident members' bytes against the interface bank's budget; a fault names the largest members.
void CheckSoundBank(std::span<const Neuron::CookedEntry> _entries, BuildReport& _report);

} // namespace ContentBuild
