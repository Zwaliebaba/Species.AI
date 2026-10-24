#pragma once

#include "StructureRecords.h"
#include "TuningMvp.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Performance
{

/// PIL-102's session patterns, which the away run plays (the M4 plan T12, ADR-190).
enum class AwayPattern : std::uint8_t
{
  Typical, // an hour of active play and two check-ins a world day
  CheckIn  // one check-in a world day
};

/// One absence of the away run's steward: its ticks, the battles its settlement defended that began in it, the deputy's
/// actions, the settlement's strength (MVP-038) when the steward left and when they came back, and the first hourly
/// sample after the return at which the strength was back to what it was when they left.
struct AwayAbsence
{
  std::uint64_t fromTick = 0;
  std::uint64_t toTick = 0;
  std::uint32_t battles = 0;
  std::uint32_t deputyActions = 0;
  std::int64_t strengthBeforeMilliRu = 0;
  std::int64_t strengthAfterMilliRu = 0;
  std::optional<std::uint64_t> regainTick;
};

/// The away run of one seed and pattern (the M4 plan T12, ADR-190): an always-on valley of four settlements for its run
/// (MVP-060, 8 world days at the always-on pace). While its steward is present, by the pattern's visits, the steward's
/// settlement is run by its scripted steward with a Territorial settlement's decisions, as the valley run's is (ADR-164);
/// while they are away, by the deputy alone (ADR-182). It keeps each absence, and the strength of the steward's
/// settlement and the AI settlements' mean at each world day's start and at the run's end.
struct AwayRun
{
  std::uint64_t seed = 0;
  AwayPattern pattern = AwayPattern::Typical;
  std::uint64_t ticks = 0;
  std::uint8_t stewardSite = 0;
  std::vector<AwayAbsence> absences;
  std::vector<std::int64_t> stewardStrengthMilliRu; // at each world day's start, then at the run's end
  std::vector<std::int64_t> aiStrengthMilliRu;      // the AI settlements' mean, likewise
  std::vector<std::uint32_t> stewardWorkers;        // likewise
  std::uint32_t stewardBattles = 0;                 // the battles the steward's settlement was a side of
  bool runEnded = false;
  double meanStepMicroseconds = 0.0;
};

// The away run of _seed at _pattern for _ticks ticks.
[[nodiscard]] AwayRun RunAway(std::uint64_t _seed, AwayPattern _pattern, const Species::TuningMvp& _tuning,
                              const Species::StructureRecords& _records, std::uint64_t _ticks);
// The away runs of _count seeds from _firstSeed at both patterns, as many at once as the machine has hardware threads;
// _ticks 0 is each world's own run (MVP-060).
[[nodiscard]] std::vector<AwayRun> RunAways(std::uint64_t _firstSeed, std::uint64_t _count, const Species::TuningMvp& _tuning,
                                            const Species::StructureRecords& _records, std::uint64_t _ticks);

// _runs as the tool's JSON, their ticks in hours of world time at _tickSeconds and their strengths in resource units.
[[nodiscard]] std::string AwayJson(const std::vector<AwayRun>& _runs, double _tickSeconds);

} // namespace Performance
