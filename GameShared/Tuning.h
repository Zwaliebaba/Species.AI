#pragma once

#include "PlatformClock.h"
#include "RecordReader.h"
#include "TuningMvp.h"

#include <filesystem>
#include <string_view>

namespace Species
{

// The tuning record's authoring file under the content root (DAT-006, DAT-016; ADR-098).
inline constexpr std::string_view TUNING_MVP_FILE = "Records/tuning/mvp.json";

// Reads and checks the tuning record, which the server loads when it starts (MVP-099). Until M1's content build cooks
// it, the server parses the authoring file with the content build's own parser (owner decision O67, ADR-098). A file
// that cannot be read is a missing_record finding; every other finding is the record's.
[[nodiscard]] Neuron::RecordResult<TuningMvp> LoadTuningMvp(const std::filesystem::path& _contentRoot);

// `ref.sim.tick_period` as the server loop's period, to the nearest nanosecond (ADR-079).
[[nodiscard]] Neuron::Nanoseconds TickPeriodOf(const TuningMvp& _tuning) noexcept;

} // namespace Species
