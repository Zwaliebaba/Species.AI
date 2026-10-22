#pragma once

#include "Statistics.h"
#include "StructureRecords.h"
#include "TuningMvp.h"
#include "WorldSimulation.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace Performance
{

/// One sample of the growth scene, every mvp.stats.sample_minutes of play (MVP-064): the settlement's figures as its
/// status gives them (MVP-098), its structures, and the world's step over the ticks since the last sample.
struct GrowthSample
{
  std::uint64_t tick = 0;
  std::uint32_t workers = 0;
  std::uint32_t limit = 0;
  std::int64_t storesMilliRu = 0;
  std::int64_t strengthMilliRu = 0;
  std::array<std::uint32_t, 3> tasks{}; // in WorkerTask's order from Gather
  std::array<std::uint32_t, 4> idle{};  // in IdleReason's order from NoFreePort
  std::uint32_t dwellings = 0;
  std::uint32_t storehouses = 0;
  double stepMicroseconds = 0.0; // the mean step since the last sample
};

/// The growth scene of one seed (the M2 plan §5, ADR-138): one settlement at the steward's site of a world with no
/// neighbours, run by the scripted steward, stepped on one thread as fast as the machine allows. It holds the server's
/// growth steps (MVP-064), the samples, the run's largest Worker count, and the tick at which the territory first reached
/// the halfway line of one of its valley border pairs, where M3's first contact falls (WLD-064), looked for once a
/// minute of play.
struct GrowthRun
{
  std::uint64_t seed = 0;
  std::uint64_t ticks = 0;
  std::vector<Species::StatisticEntry> steps;
  std::vector<GrowthSample> samples;
  std::uint32_t largestWorkers = 0;
  std::optional<std::uint64_t> halfwayTick;
};

// The scene of the valley of _seed for _ticks ticks. _world, when given, receives the world at the end, for the
// settlement-at-work measurement.
[[nodiscard]] GrowthRun RunGrowth(std::uint64_t _seed, const Species::TuningMvp& _tuning, const Species::StructureRecords& _records,
                                  std::uint64_t _ticks, std::unique_ptr<Species::WorldSimulation>* _world = nullptr);

/// The exit's first criterion as the scene measures it (the scope's table 2, the M2 plan §5 and §9): the plateau, the
/// first growth step followed by an hour of play with no other while its Worker count is within 95 % of the run's
/// largest; and the largest gap between growth steps from the start up to the fifth hour or the plateau, whichever
/// comes first, against pil.settlement.growth_step_minutes (20) and the plateau's window of hours 4 to 6.
struct GrowthJudgement
{
  std::optional<std::uint64_t> plateauTick;
  std::uint64_t largestGapTicks = 0;
  std::uint64_t gapFromTick = 0;
  bool gapMet = false;
  bool plateauMet = false;
};

[[nodiscard]] GrowthJudgement JudgeGrowth(const GrowthRun& _run, double _tickPeriodSeconds);

/// What a whole-world snapshot costs (MVP-075, ADR-137): its file's size, the time to encode the world's state into it,
/// and the time to save it through the snapshot store's atomic replace.
struct SnapshotCost
{
  std::size_t bytes = 0;
  std::int64_t encodeNanoseconds = 0;
  std::int64_t saveNanoseconds = 0;
};

// _world's snapshot, encoded and saved into _folder, which keeps one; none when the save fails.
[[nodiscard]] std::optional<SnapshotCost> MeasureSnapshot(const Species::WorldSimulation& _world, const std::filesystem::path& _folder);

} // namespace Performance
