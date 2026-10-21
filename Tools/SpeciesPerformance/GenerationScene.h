#pragma once

#include "TuningMvp.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Performance
{

/// One valley of the generation corpus (ADR-102, ADR-104, WLD-165): the time of its generation with every attempt, check
/// and re-roll, and the time of its land route graph alone, built again from the nodes the valley placed.
struct GenerationSample
{
  std::uint64_t seed = 0;       // the seed asked for
  std::uint64_t passedSeed = 0; // the seed whose attempt passed every check (MVP-011)
  std::uint32_t attempt = 0;    // that attempt
  std::int64_t generateNanoseconds = 0;
  std::int64_t graphNanoseconds = 0;
  std::size_t nodes = 0;
};

// The valleys of the seeds _firstSeed to _firstSeed + _seeds - 1, each generated on the calling thread as the server and
// the client generate it, outside every tick (TER-242), and its graph then built again on its own.
[[nodiscard]] std::vector<GenerationSample> MeasureGeneration(std::uint64_t _firstSeed, std::uint64_t _seeds,
                                                              const Species::TuningMvp& _tuning);

} // namespace Performance
