#pragma once

#include "AssetError.h"
#include "LooseContent.h"
#include "PlatformClock.h"
#include "RecordReader.h"
#include "TuningMvp.h"

#include <expected>
#include <filesystem>
#include <string_view>

namespace Species
{

// The tuning record's authoring file under the content root, which the content build cooks (DAT-006, DAT-016; ADR-098).
inline constexpr std::string_view TUNING_MVP_FILE = "Records/tuning/mvp.json";
// The key of its cooked entry (AST-014).
inline constexpr std::string_view TUNING_MVP_KEY = "records/tuning/mvp";

// Loads the cooked tuning record from the mounted content, which the server does when it starts (MVP-099). Nothing is
// parsed: the content build parsed and cooked it, so every process reads the same bits (DAT-010, DAT-050; ADR-108).
// Missing when the content holds no record; damaged when its bytes are not one.
[[nodiscard]] std::expected<TuningMvp, Neuron::AssetError> LoadTuningMvp(const Neuron::LooseContent& _content);

// Reads the authoring file with the strict parser and the generated reader, as the content build does before it cooks:
// the tests' source of records the content build has not cooked. No program calls it (DAT-010, ADR-108). A file that
// cannot be read is a missing_record finding; every other finding is the record's.
[[nodiscard]] Neuron::RecordResult<TuningMvp> ReadTuningMvpSource(const std::filesystem::path& _contentRoot);

// `ref.sim.tick_period` as the server loop's period, to the nearest nanosecond (ADR-079).
[[nodiscard]] Neuron::Nanoseconds TickPeriodOf(const TuningMvp& _tuning) noexcept;

} // namespace Species
