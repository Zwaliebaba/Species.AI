#pragma once

#include "Battles.h"
#include "Settlement.h"
#include "StructureRecords.h"
#include "TuningMvp.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Performance
{

/// One battle of the valley run (M3 plan §4, ADR-164): its ticks, its sides, whether it settled a claim, its figures (the
/// most of both sides' living figures on its battlefield at once, counted every second), and how it ended.
struct ValleyBattle
{
  std::uint64_t startTick = 0;
  std::uint64_t endTick = 0;
  std::uint8_t attacker = 0;
  std::uint8_t defender = 0;
  bool claim = false;
  std::uint32_t figures = 0;
  Species::BattleEnd end = Species::BattleEnd::Time;
  bool attackerWon = false;
};

/// One claim of the valley run: its claimant and area, when it was made, and when it ended, if it did.
struct ValleyClaim
{
  std::uint8_t claimant = 0;
  std::uint8_t area = 0;
  std::uint64_t madeTick = 0;
  std::optional<std::uint64_t> endTick;
};

/// The valley run of one seed (the M3 plan §4, ADR-164): the four settlements of a valley, each driven by a policy and
/// stepped as fast as the machine allows for a run. The steward's settlement is run by its scripted steward with the
/// decisions of `stewardPolicy`, the conflict policy the run states, and the AI settlements by their own. It keeps what
/// Playtest 1's measured lines read (the MVP scope's table 5): every battle and claim, each settlement's growth steps,
/// strength and renown at the end, and the world's mean step.
struct ValleyRun
{
  std::uint64_t seed = 0;
  std::uint64_t ticks = 0;
  std::uint8_t stewardSite = 0;
  Species::Temperament stewardPolicy = Species::Temperament::Territorial;
  std::vector<ValleyBattle> battles;
  std::vector<ValleyClaim> claims;
  std::array<std::int64_t, 4> strengthMilliRu{}; // at the end, by site
  std::array<std::int64_t, 4> renown{};          // at the end, by site
  std::array<std::uint32_t, 4> workers{};        // at the end, by site
  std::vector<std::uint64_t> stewardGrowthSteps; // the ticks of the steward's growth steps
  std::optional<std::uint64_t> firstContactTick;
  bool runEnded = false;
  double meanStepMicroseconds = 0.0;
  double largestStepMicroseconds = 0.0;
};

// The valley run of _seed for _ticks ticks, its steward's settlement at mvp.sandbox.steward_site with the decisions of
// _policy, or none for Temperament::None.
[[nodiscard]] ValleyRun RunValley(std::uint64_t _seed, const Species::TuningMvp& _tuning, const Species::StructureRecords& _records,
                                  std::uint64_t _ticks, Species::Temperament _policy);

// The valley runs of _count seeds from _firstSeed, as many at once as the machine has hardware threads: each is a world of
// its own, so their steps' times are those of runs sharing the machine.
[[nodiscard]] std::vector<ValleyRun> RunValleys(std::uint64_t _firstSeed, std::uint64_t _count, const Species::TuningMvp& _tuning,
                                                const Species::StructureRecords& _records, std::uint64_t _ticks,
                                                Species::Temperament _policy);

/// Playtest 1's measurable lines as the valley run reads them (the MVP scope's table 5, ADR-164), for one seed: the
/// steward's battles' figures (typical 300 to 450, never below 200), every battle's length (90 % within 4 to 12 minutes,
/// none above 20), the claims ended within 7 hours of play, PIL-123's 7 world days at the session pace (at least 60 %),
/// the strongest settlement's strength at the end against the median (at most 3 times), the steward's battles in a run
/// (4 to 10), and the largest gap between the steward's growth steps in the first 5 hours (at most 20 minutes).
struct ValleyLines
{
  std::uint32_t stewardBattles = 0;
  std::optional<std::uint32_t> stewardFiguresMedian;
  std::optional<std::uint32_t> stewardFiguresLeast;
  std::uint32_t battles = 0;
  double battlesWithinFourToTwelve = 0.0; // the share
  double longestBattleMinutes = 0.0;
  std::uint32_t claimsJudged = 0;
  double claimsSettled = 0.0; // the share of the judged claims
  double strongestToMedian = 0.0;
  double largestStepGapMinutes = 0.0;
};

[[nodiscard]] ValleyLines JudgeValley(const ValleyRun& _run, const Species::TuningMvp& _tuning);

} // namespace Performance
