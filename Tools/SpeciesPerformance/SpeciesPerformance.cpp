#include "pch.h"

#include "AssetError.h"
#include "CommandJournal.h"
#include "Development.h"
#include "Files.h"
#include "GenerationScene.h"
#include "GrowthScene.h"
#include "LinkScene.h"
#include "LogFloodScene.h"
#include "LooseContent.h"
#include "MarkerTimings.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "Sandbox.h"
#include "SceneRun.h"
#include "SipHash.h"
#include "StructureRecords.h"
#include "TravelScene.h"
#include "Tuning.h"
#include "ValleyAtWar.h"
#include "ValleyScene.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <exception>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

// The performance tool (M1 plan T10, ADR-110): a development tool that never ships. It runs the headless measurement
// scenes, each from a fixed seed and key with a recorded command stream, on one thread of the build it was built in,
// and writes what each step cost, read from the profile markers, as JSON for Build/RunPerformance.py. Beside the
// scenes it times the generation of a corpus of valleys with their land route graphs (WLD-165, ADR-104), and a log call
// while eight threads flood the logger (DBG T2, ADR-091). From M2 task T10 it runs the growth scene of a corpus of
// seeds with the scripted steward and measures the settlement at work at its plateau and the region with four
// (ADR-138); these read the structure records of the content folder. From M2 task T19, --saved-world runs no scene: it
// reads the growth of the steward's own world from the server's statistics in its newest snapshot, for the owner's
// growth run (ADR-156). From M3 task T9 it measures the valley at war, four settlements with two battles in progress,
// and, only when named, runs the valley run of the corpus, each settlement driven by a policy for a run, reporting
// Playtest 1's measurable lines (ADR-164). From M4 task T11, the link scene, only when named, serves a valley over
// QuicTransport to a client of its own, through the link simulator when --link-port names its port, and compares the
// client's replica with the server's view after every tick (ADR-189).
//
//   SpeciesPerformance.exe [--quick] [--scenes <name,...>] [--out <file.json>] [--content <folder> | --tuning-source <GameData>]
//   SpeciesPerformance.exe --saved-world <world folder> [--out <file.json>] [--content <folder> | --tuning-source <GameData>]
//   SpeciesPerformance.exe --scenes link [--quick] [--server-port <port>] [--link-port <port>] [--out <file.json>]
//
// --quick runs every scene for a few ticks, to show that it runs; its figures measure nothing. The tuning record is
// the cooked one of the loose-content folder beside the executable, as the programs read it, unless an option names
// another folder or the authored record.

namespace
{

constexpr std::string_view PRODUCT_NAME = "SpeciesPerformance";

#if defined(_M_ARM64)
constexpr std::string_view PLATFORM = "ARM64";
#elif defined(_M_X64)
constexpr std::string_view PLATFORM = "x64";
#else
constexpr std::string_view PLATFORM = "other";
#endif
#if ENGINE_DEVELOPMENT
constexpr std::string_view CONFIGURATION = "Debug";
#else
constexpr std::string_view CONFIGURATION = "Release";
#endif

// Every scene's world: one simulation key, and the valley of seed 1 that the tests use (MVP-011).
constexpr Neuron::SipHashKey SCENE_KEY{0x70657266736e6501, 0x0123456789abcdef};
constexpr std::uint64_t SCENE_SEED = 1;

// ADR-080 items 3, 4 and 10: the region bound is two battles at the side cap S at two of its settlements and two
// settlements at the population limit L at work, with S = L = 600, the provisional values that this measurement fixes.
constexpr std::uint32_t BOUND_SIDE_FIGURES = 600;
constexpr std::uint32_t BOUND_SETTLEMENT_FIGURES = 600;
constexpr double BOUND_SIDE_OFFSET = 100.0; // wu: each side of a battle stands this far from the site's centre
constexpr double BOUND_SPREAD = 100.0;      // wu: the radius each crowd spawns in
constexpr std::uint8_t AT_WORK_TEAM = 4;    // both stand-in settlements, one team, so that they never fight

// The travel corpus: the valleys of these seeds (O71).
constexpr std::uint64_t TRAVEL_SEED_FIRST = 1;
constexpr std::uint64_t TRAVEL_SEED_COUNT = 8;
constexpr std::uint64_t TRAVEL_TICKS_MAX = 6000; // 10 minutes: a crowd that has not arrived by then is not counted

constexpr std::uint64_t DRAW_VALUES = 10'000'000;
constexpr std::uint64_t DRAW_VALUES_QUICK = 100'000;

// The generation corpus: the valleys of seeds 1 to 100, the corpus of ADR-104's local measurements.
constexpr std::uint64_t GENERATION_SEED_FIRST = 1;
constexpr std::uint64_t GENERATION_SEED_COUNT = 100;

// The growth corpus (the M2 plan §5, ADR-138): the valleys of these seeds, each run for mvp.run.length_hours; the quick
// run takes the first seed's first minute.
constexpr std::uint64_t GROWTH_SEED_FIRST = 1;
constexpr std::uint64_t GROWTH_SEED_COUNT = 8;
constexpr std::uint64_t GROWTH_TICKS_QUICK = 600;
constexpr double SECONDS_PER_HOUR = 3600.0;

// The valley run's corpus (the M3 plan §4, ADR-164): the valleys of these seeds, each run for mvp.run.length_hours with
// the steward's settlement under the Territorial decisions; the quick run takes the first seed's first minute.
constexpr std::uint64_t VALLEY_SEED_FIRST = 1;
constexpr std::uint64_t VALLEY_SEED_COUNT = 8;
constexpr std::uint64_t VALLEY_TICKS_QUICK = 600;

// The link scene (ADR-086, ADR-088, ADR-189): 30 minutes of the server's ticks with a rejoin at minute 20; the quick
// run takes 30 seconds with a rejoin halfway.
constexpr double LINK_MINUTES = 30.0;
constexpr double LINK_REJOIN_MINUTES = 20.0;
constexpr std::uint64_t LINK_TICKS_QUICK = 300;
constexpr std::uint64_t LINK_REJOIN_TICKS_QUICK = 150;
constexpr double SECONDS_PER_MINUTE = 60.0;

// DBG T2: 10⁶ debug records between the flood's threads.
constexpr std::uint64_t LOG_FLOOD_RECORDS = 1'000'000;
constexpr std::uint64_t LOG_FLOOD_RECORDS_QUICK = 8'000;

void Print(std::FILE* _stream, std::string_view _text)
{
  const std::string line = std::format("{}: {}\n", PRODUCT_NAME, _text);
  std::fwrite(line.data(), 1, line.size(), _stream);
  std::fflush(_stream);
}

struct Options
{
  bool quick = false;
  std::vector<std::string> scenes; // none: every scene
  std::filesystem::path out;
  std::filesystem::path content;
  std::filesystem::path tuningSource;
  std::filesystem::path savedWorld; // ADR-156: a saved world's folder to read rather than the scenes
  std::uint16_t serverPort = 0;     // ADR-189: the link scene's listener; 0: one the system chooses
  std::uint16_t linkPort = 0;       // ADR-189: the link simulator's port, which the client connects to; 0: none
};

bool Wanted(const Options& _options, std::string_view _scene)
{
  return _options.scenes.empty() || std::ranges::find(_options.scenes, _scene) != _options.scenes.end();
}

// A scene run only when named, as the valley run of a whole corpus is (ADR-164).
bool Named(const Options& _options, std::string_view _scene)
{
  return std::ranges::find(_options.scenes, _scene) != _options.scenes.end();
}

std::expected<Species::TuningMvp, std::string> LoadTuning(const Options& _options)
{
  if (!_options.tuningSource.empty())
  {
    auto source = Species::ReadTuningMvpSource(_options.tuningSource);
    if (!source)
    {
      return std::unexpected(std::format("the authored tuning record under {} does not read", _options.tuningSource.string()));
    }
    return *source;
  }
  const std::filesystem::path folder = _options.content.empty() ? Neuron::ExecutableFolder() / L"content" : _options.content;
  const auto content = Neuron::LooseContent::Mount(folder);
  if (!content)
  {
    return std::unexpected(Neuron::FormatAssetError(content.error()));
  }
  auto tuning = Species::LoadTuningMvp(*content);
  if (!tuning)
  {
    return std::unexpected(Neuron::FormatAssetError(tuning.error()));
  }
  return *tuning;
}

// The structures' simulation data records of the loose-content folder beside the executable, or of the one --content
// names (ADR-129).
std::expected<Species::StructureRecords, std::string> LoadRecords(const Options& _options)
{
  const std::filesystem::path folder = _options.content.empty() ? Neuron::ExecutableFolder() / L"content" : _options.content;
  const auto content = Neuron::LooseContent::Mount(folder);
  if (!content)
  {
    return std::unexpected(Neuron::FormatAssetError(content.error()));
  }
  auto records = Species::LoadStructureRecords(*content);
  if (!records)
  {
    return std::unexpected(Neuron::FormatAssetError(records.error()));
  }
  return std::move(*records);
}

// A world of one region on the valley, its terrain attached.
std::unique_ptr<Species::WorldSimulation> ValleyWorld(const Species::TuningMvp& _tuning,
                                                      const std::shared_ptr<const Species::RegionTerrain>& _terrain)
{
  auto world = std::make_unique<Species::WorldSimulation>(Species::WorldSimulation::Desc{SCENE_KEY, 0, 1, &_tuning, _terrain->seed});
  world->Region(0).AttachTerrain(_terrain);
  return world;
}

void SpawnCrowd(Species::RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _center, double _offsetX, std::uint32_t _count)
{
  const Species::RegionTerrain& terrain = *_region.Terrain();
  Species::RegionSimulation::Spawn spawn;
  spawn.team = _team;
  spawn.position = {_center.x + _offsetX, 0.0, _center.z};
  spawn.position.y = terrain.field.Height(spawn.position.x, spawn.position.z);
  spawn.count = _count;
  spawn.spread = BOUND_SPREAD;
  static_cast<void>(_region.SpawnFigures(spawn));
}

// The settlements at work until M2: wandering Workers of one team at sites 2 and 3 (M1 plan T10).
void SpawnAtWork(Species::RegionSimulation& _region)
{
  const auto& sites = _region.Terrain()->layout.sites;
  SpawnCrowd(_region, AT_WORK_TEAM, sites[2], 0.0, BOUND_SETTLEMENT_FIGURES);
  SpawnCrowd(_region, AT_WORK_TEAM, sites[3], 0.0, BOUND_SETTLEMENT_FIGURES);
}

// The time of one keyed draw (ADR-082 item 2): SipHash-2-4 of the five words a draw hashes, the domain, the purpose,
// the subject, the tick and the ordinal, over sequential subjects and ordinals 0 to 15. Only the simulation makes draws,
// so the tool hashes the words itself.
double DrawNanoseconds(std::uint64_t _values)
{
  std::uint64_t sink = 0;
  const std::int64_t start = Neuron::MarkerClockNanoseconds();
  for (std::uint64_t context = 0; context < _values / 16; ++context)
  {
    const std::uint64_t subject = (std::uint64_t{0x0100} << 48) + 1 + context;
    for (std::uint64_t ordinal = 0; ordinal < 16; ++ordinal)
    {
      sink ^= Neuron::SipHash24Words(SCENE_KEY, {1, 5, subject, context / 1024, ordinal});
    }
  }
  const std::int64_t elapsed = Neuron::MarkerClockNanoseconds() - start;
  const std::uint64_t drawn = _values / 16 * 16;
  // The values are used, so the loop is not optimised away.
  if (sink == 0x5eed)
  {
    Print(stdout, "a draw sum of 0x5eed");
  }
  return static_cast<double>(elapsed) / static_cast<double>(drawn);
}

double Percentile(std::vector<std::int64_t> _values, double _share)
{
  if (_values.empty())
  {
    return 0.0;
  }
  std::ranges::sort(_values);
  const auto index = static_cast<std::size_t>(_share * static_cast<double>(_values.size() - 1));
  return static_cast<double>(_values[index]);
}

double Mean(const std::vector<std::int64_t>& _values)
{
  double sum = 0.0;
  for (const std::int64_t value : _values)
  {
    sum += static_cast<double>(value);
  }
  return _values.empty() ? 0.0 : sum / static_cast<double>(_values.size());
}

double MedianOf(const std::vector<double>& _sorted, double _share)
{
  if (_sorted.empty())
  {
    return 0.0;
  }
  return _sorted[static_cast<std::size_t>(_share * static_cast<double>(_sorted.size() - 1))];
}

std::string SceneJson(const Performance::SceneRun& _run)
{
  std::string markers;
  for (const Neuron::MarkerTiming& marker : _run.markers)
  {
    markers += std::format("{}\n        \"{}\": {{\"count\": {}, \"microseconds\": {:.3f}}}", markers.empty() ? "" : ",", marker.name,
                           marker.count, static_cast<double>(marker.nanoseconds) / 1000.0);
  }
  return std::format("    {{\n      \"name\": \"{}\",\n      \"ticks\": {},\n      \"first_figures\": {},\n      \"last_figures\": {},\n"
                     "      \"mean_figures\": {:.1f},\n      \"ais\": {},\n      \"sides\": {},\n"
                     "      \"step_us\": {{\"mean\": {:.3f}, \"p50\": {:.3f}, \"p95\": {:.3f}, \"max\": {:.3f}}},\n"
                     "      \"markers\": {{{}\n      }}\n    }}",
                     _run.name, _run.ticks, _run.firstFigures, _run.lastFigures, _run.meanFigures, _run.ais, _run.sides,
                     Mean(_run.stepNanoseconds) / 1000.0, Percentile(_run.stepNanoseconds, 0.5) / 1000.0,
                     Percentile(_run.stepNanoseconds, 0.95) / 1000.0, Percentile(_run.stepNanoseconds, 1.0) / 1000.0, markers);
}

std::string TravelJson(const std::vector<Performance::TravelEdge>& _edges)
{
  std::string edges;
  for (const Performance::TravelEdge& edge : _edges)
  {
    edges += std::format("{}\n      {{\"seed\": {}, \"edge\": \"{}\", \"distance\": {:.1f}, \"workers\": {}, \"arrived\": {}, "
                         "\"factor_median\": {:.4f}, \"factor_p90\": {:.4f}, \"figure_ticks\": {}, \"advance_us\": {:.3f}}}",
                         edges.empty() ? "" : ",", edge.seed, edge.name, edge.distance, edge.workers, edge.factors.size(),
                         MedianOf(edge.factors, 0.5), MedianOf(edge.factors, 0.9), edge.figureTicks,
                         static_cast<double>(edge.advanceNanoseconds) / 1000.0);
  }
  return std::format("[{}\n    ]", edges);
}

std::string GrowthJson(const std::vector<Performance::GrowthRun>& _runs, double _tickPeriodSeconds)
{
  const auto hours = [_tickPeriodSeconds](std::uint64_t _tick)
  { return static_cast<double>(_tick) * _tickPeriodSeconds / SECONDS_PER_HOUR; };
  std::string runs;
  for (const Performance::GrowthRun& run : _runs)
  {
    const Performance::GrowthJudgement judgement = Performance::JudgeGrowth(run, _tickPeriodSeconds);
    std::string steps;
    for (const Species::StatisticEntry& step : run.steps)
    {
      steps +=
        std::format("{}[{}, {}, {}, {}]", steps.empty() ? "" : ", ", step.tick, static_cast<int>(step.kind), step.detail, step.workers);
    }
    std::string samples;
    for (const Performance::GrowthSample& sample : run.samples)
    {
      samples += std::format(
        "{}\n        {{\"tick\": {}, \"workers\": {}, \"limit\": {}, \"stores_ru\": {:.3f}, \"strength_ru\": {:.3f}, "
        "\"tasks\": [{}, {}, {}], \"idle\": [{}, {}, {}, {}], \"dwellings\": {}, \"storehouses\": {}, \"step_us\": {:.3f}}}",
        samples.empty() ? "" : ",", sample.tick, sample.workers, sample.limit, static_cast<double>(sample.storesMilliRu) / 1000.0,
        static_cast<double>(sample.strengthMilliRu) / 1000.0, sample.tasks[0], sample.tasks[1], sample.tasks[2], sample.idle[0],
        sample.idle[1], sample.idle[2], sample.idle[3], sample.dwellings, sample.storehouses, sample.stepMicroseconds);
    }
    runs += std::format("{}\n      {{\"seed\": {}, \"ticks\": {}, \"largest_workers\": {}, \"plateau_h\": {}, \"gap_minutes\": {:.2f}, "
                        "\"gap_from_h\": {:.3f}, \"gap_met\": {}, \"plateau_met\": {}, \"halfway_h\": {},\n       \"steps\": [{}],\n"
                        "       \"samples\": [{}\n       ]}}",
                        runs.empty() ? "" : ",", run.seed, run.ticks, run.largestWorkers,
                        judgement.plateauTick ? std::format("{:.3f}", hours(*judgement.plateauTick)) : std::string("null"),
                        hours(judgement.largestGapTicks) * 60.0, hours(judgement.gapFromTick), judgement.gapMet ? "true" : "false",
                        judgement.plateauMet ? "true" : "false",
                        run.halfwayTick ? std::format("{:.3f}", hours(*run.halfwayTick)) : std::string("null"), steps, samples);
  }
  return std::format("[{}\n    ]", runs);
}

// ADR-164: each valley run with Playtest 1's lines as it reads them, and its battles and claims.
std::string ValleyJson(const std::vector<Performance::ValleyRun>& _runs, const Species::TuningMvp& _tuning)
{
  const double tickSeconds = _tuning.ref.sim.tickPeriod;
  const auto hours = [tickSeconds](std::uint64_t _tick) { return static_cast<double>(_tick) * tickSeconds / SECONDS_PER_HOUR; };
  const auto orNull = [](const std::optional<std::uint32_t>& _value) { return _value ? std::to_string(*_value) : std::string("null"); };
  std::string runs;
  for (const Performance::ValleyRun& run : _runs)
  {
    const Performance::ValleyLines lines = Performance::JudgeValley(run, _tuning);
    std::string battles;
    for (const Performance::ValleyBattle& battle : run.battles)
    {
      battles += std::format("{}[{:.3f}, {:.2f}, {}, {}, {}, {}, {}, {}]", battles.empty() ? "" : ", ", hours(battle.startTick),
                             static_cast<double>(battle.endTick - battle.startTick) * tickSeconds / 60.0, battle.attacker, battle.defender,
                             battle.claim ? 1 : 0, battle.figures, static_cast<int>(battle.end), battle.attackerWon ? 1 : 0);
    }
    std::string claims;
    for (const Performance::ValleyClaim& claim : run.claims)
    {
      claims += std::format("{}[{}, {}, {:.3f}, {}]", claims.empty() ? "" : ", ", claim.claimant, claim.area, hours(claim.madeTick),
                            claim.endTick ? std::format("{:.3f}", hours(claim.endTick.value_or(0))) : std::string("null"));
    }
    runs += std::format(
      "{}\n      {{\"seed\": {}, \"ticks\": {}, \"steward_site\": {}, \"policy\": {}, \"run_ended\": {}, \"contact_h\": {},\n"
      "       \"steward_battles\": {}, \"steward_figures_median\": {}, \"steward_figures_least\": {}, \"battles\": {}, "
      "\"battles_4_to_12\": {:.3f}, \"longest_battle_min\": {:.2f}, \"claims_judged\": {}, \"claims_settled\": {:.3f}, "
      "\"strongest_to_median\": {:.3f}, \"step_gap_min\": {:.2f}, \"step_us\": {:.3f}, \"step_us_max\": {:.3f},\n"
      "       \"workers\": [{}, {}, {}, {}], \"renown\": [{}, {}, {}, {}],\n       \"battle_list\": [{}],\n       \"claim_list\": [{}]}}",
      runs.empty() ? "" : ",", run.seed, run.ticks, run.stewardSite, static_cast<int>(run.stewardPolicy), run.runEnded ? "true" : "false",
      run.firstContactTick ? std::format("{:.3f}", hours(run.firstContactTick.value_or(0))) : std::string("null"), lines.stewardBattles,
      orNull(lines.stewardFiguresMedian), orNull(lines.stewardFiguresLeast), lines.battles, lines.battlesWithinFourToTwelve,
      lines.longestBattleMinutes, lines.claimsJudged, lines.claimsSettled, lines.strongestToMedian, lines.largestStepGapMinutes,
      run.meanStepMicroseconds, run.largestStepMicroseconds, run.workers[0], run.workers[1], run.workers[2], run.workers[3], run.renown[0],
      run.renown[1], run.renown[2], run.renown[3], battles, claims);
  }
  return std::format("[{}\n    ]", runs);
}

std::string GenerationJson(const std::vector<Performance::GenerationSample>& _samples)
{
  std::string samples;
  for (const Performance::GenerationSample& sample : _samples)
  {
    samples +=
      std::format("{}\n      {{\"seed\": {}, \"passed_seed\": {}, \"attempt\": {}, \"generate_ms\": {:.3f}, \"graph_ms\": {:.3f}, "
                  "\"nodes\": {}}}",
                  samples.empty() ? "" : ",", sample.seed, sample.passedSeed, sample.attempt,
                  static_cast<double>(sample.generateNanoseconds) / 1e6, static_cast<double>(sample.graphNanoseconds) / 1e6, sample.nodes);
  }
  return std::format("[{}\n    ]", samples);
}

// A log call's mean cost over every thread's calls, and the slowest thread's mean.
std::string LogFloodJson(const std::optional<Performance::LogFlood>& _flood)
{
  if (!_flood || _flood->records == 0)
  {
    return "null";
  }
  std::int64_t total = 0;
  std::int64_t slowest = 0;
  for (const std::int64_t nanoseconds : _flood->threadNanoseconds)
  {
    total += nanoseconds;
    slowest = std::max(slowest, nanoseconds);
  }
  const auto records = static_cast<double>(_flood->records);
  const auto perThread = records / static_cast<double>(_flood->threadNanoseconds.size());
  return std::format("{{\"threads\": {}, \"records\": {}, \"call_ns_mean\": {:.2f}, \"call_ns_slowest_thread\": {:.2f}, \"written\": {}, "
                     "\"dropped\": {}}}",
                     _flood->threadNanoseconds.size(), _flood->records, static_cast<double>(total) / records,
                     static_cast<double>(slowest) / perThread, _flood->written, _flood->dropped);
}

// The JSON of _text, written to _options' file or to standard output.
int Write(const Options& _options, const std::string& _text)
{
  if (_options.out.empty())
  {
    std::fwrite(_text.data(), 1, _text.size(), stdout);
    return 0;
  }
  std::ofstream file(_options.out, std::ios::binary);
  file << _text;
  if (!file)
  {
    Print(stderr, std::format("{} cannot be written", _options.out.string()));
    return 1;
  }
  return 0;
}

// ADR-156: the growth of the steward's own world, judged as the growth scene's (the M2 plan §5): its growth steps as
// [tick, kind, detail, Workers], and its samples as [tick, Workers, strength in ru].
int RunSavedWorld(const Options& _options, const Species::TuningMvp& _tuning)
{
  const auto records = LoadRecords(_options);
  if (!records)
  {
    Print(stderr, std::format("the structure records cannot be read: {}", records.error()));
    return 1;
  }
  const std::optional<Performance::SavedWorldRun> saved = Performance::ReadSavedWorld(_options.savedWorld, _tuning, *records);
  if (!saved)
  {
    Print(stderr, std::format("no snapshot in {} restores to a world with the steward's settlement", _options.savedWorld.string()));
    return 1;
  }
  const double tickPeriod = _tuning.ref.sim.tickPeriod;
  const auto hours = [tickPeriod](std::uint64_t _tick) { return static_cast<double>(_tick) * tickPeriod / SECONDS_PER_HOUR; };
  const Performance::GrowthJudgement judgement = Performance::JudgeGrowth(saved->run, tickPeriod);
  std::string steps;
  for (const Species::StatisticEntry& step : saved->run.steps)
  {
    steps +=
      std::format("{}[{}, {}, {}, {}]", steps.empty() ? "" : ", ", step.tick, static_cast<int>(step.kind), step.detail, step.workers);
  }
  std::string samples;
  for (const Species::StatisticEntry& sample : saved->samples)
  {
    samples += std::format("{}[{}, {}, {:.3f}]", samples.empty() ? "" : ", ", sample.tick, sample.workers,
                           static_cast<double>(sample.strengthMilliRu) / 1000.0);
  }
  const std::string json =
    std::format("{{\n  \"tool\": \"{}\",\n  \"configuration\": \"{}\",\n  \"platform\": \"{}\",\n  \"tick_period_s\": {},\n"
                "  \"saved_world\": {{\"snapshot\": \"{}\", \"seed\": {}, \"ticks\": {}, \"hours\": {:.3f}, \"largest_workers\": {}, "
                "\"plateau_h\": {}, \"gap_minutes\": {:.2f}, \"gap_from_h\": {:.3f}, \"gap_met\": {}, \"plateau_met\": {},\n"
                "    \"steps\": [{}],\n    \"samples\": [{}]}}\n}}\n",
                PRODUCT_NAME, CONFIGURATION, PLATFORM, tickPeriod, saved->snapshot.filename().string(), saved->run.seed, saved->run.ticks,
                hours(saved->run.ticks), saved->run.largestWorkers,
                judgement.plateauTick ? std::format("{:.3f}", hours(*judgement.plateauTick)) : std::string("null"),
                hours(judgement.largestGapTicks) * 60.0, hours(judgement.gapFromTick), judgement.gapMet ? "true" : "false",
                judgement.plateauMet ? "true" : "false", steps, samples);
  return Write(_options, json);
}

int Run(const Options& _options)
{
  const auto tuning = LoadTuning(_options);
  if (!tuning)
  {
    Print(stderr, std::format("the tuning record cannot be read: {}", tuning.error()));
    return 1;
  }
  if (!_options.savedWorld.empty())
  {
    return RunSavedWorld(_options, *tuning);
  }
  auto generated = Species::GenerateRegionTerrain(SCENE_SEED, *tuning);
  if (!generated)
  {
    Print(stderr, std::format("seed {} gives no valley", SCENE_SEED));
    return 1;
  }
  const auto terrain = std::make_shared<const Species::RegionTerrain>(std::move(*generated));
  const auto ticks = [&_options](std::uint64_t _full, std::uint64_t _quick) { return _options.quick ? _quick : _full; };
  std::vector<Performance::SceneRun> runs;

  if (Wanted(_options, "sandbox-battle"))
  {
    // The sandbox's battle of 400 figures with the AI on both sides.
    const auto world = ValleyWorld(*tuning, terrain);
    Species::PopulateSandbox(world->Region(0));
    static_cast<void>(world->Region(0).AddAi(Species::SANDBOX_STEWARD_TEAM));
    runs.push_back(Performance::MeasureWorld("sandbox-battle", *world, ticks(3000, 20)));
  }
  if (Wanted(_options, "synthetic-battle"))
  {
    // ADR-080 item 2's synthetic battle of mvp.perf.synthetic_figures.
    const auto world = ValleyWorld(*tuning, terrain);
    Species::PopulateSyntheticBattle(world->Region(0));
    runs.push_back(Performance::MeasureWorld("synthetic-battle", *world, ticks(3000, 5)));
  }
  if (Wanted(_options, "region-bound"))
  {
    // ADR-080 item 10: a battle of 2 × S at site 0 and one at site 1, and two settlements of L at work at sites 2 and 3.
    const auto world = ValleyWorld(*tuning, terrain);
    Species::RegionSimulation& region = world->Region(0);
    const auto& sites = terrain->layout.sites;
    SpawnCrowd(region, 0, sites[0], -BOUND_SIDE_OFFSET, BOUND_SIDE_FIGURES);
    SpawnCrowd(region, 1, sites[0], BOUND_SIDE_OFFSET, BOUND_SIDE_FIGURES);
    SpawnCrowd(region, 2, sites[1], -BOUND_SIDE_OFFSET, BOUND_SIDE_FIGURES);
    SpawnCrowd(region, 3, sites[1], BOUND_SIDE_OFFSET, BOUND_SIDE_FIGURES);
    SpawnAtWork(region);
    runs.push_back(Performance::MeasureWorld("region-bound", *world, ticks(1000, 3)));
  }
  if (Wanted(_options, "at-work"))
  {
    // The region bound's settlements at work alone: the cost of a Worker that is not in a battle.
    const auto world = ValleyWorld(*tuning, terrain);
    SpawnAtWork(world->Region(0));
    runs.push_back(Performance::MeasureWorld("at-work", *world, ticks(1000, 5)));
  }
  if (Wanted(_options, "empty-region"))
  {
    // A region with its terrain and no figure: its fixed cost.
    const auto world = ValleyWorld(*tuning, terrain);
    runs.push_back(Performance::MeasureWorld("empty-region", *world, ticks(1000, 5)));
  }
  // M2 task T10 (ADR-138): the growth corpus, then the settlement at work at the end of the first seed's run, and the
  // region at its bound with four scripted settlements, each run for the run's length (ADR-080).
  std::vector<Performance::GrowthRun> growth;
  std::optional<Performance::SnapshotCost> snapshot;
  if (Wanted(_options, "growth") || Wanted(_options, "settlement-at-work") || Wanted(_options, "region-bound-settled"))
  {
    const auto records = LoadRecords(_options);
    if (!records)
    {
      Print(stderr, std::format("the structure records cannot be read: {}", records.error()));
      return 1;
    }
    const auto runTicks =
      static_cast<std::uint64_t>(std::llround(tuning->mvp.run.lengthHours * SECONDS_PER_HOUR / tuning->ref.sim.tickPeriod));
    const std::uint64_t growthTicks = ticks(runTicks, GROWTH_TICKS_QUICK);
    if (Wanted(_options, "growth") || Wanted(_options, "settlement-at-work"))
    {
      const std::uint64_t seeds = _options.quick ? 1 : GROWTH_SEED_COUNT;
      for (std::uint64_t seed = GROWTH_SEED_FIRST; seed < GROWTH_SEED_FIRST + seeds; ++seed)
      {
        std::unique_ptr<Species::WorldSimulation> end;
        growth.push_back(Performance::RunGrowth(seed, *tuning, *records, growthTicks, seed == GROWTH_SEED_FIRST ? &end : nullptr));
        Print(stdout, std::format("the growth of seed {}: {} Workers at the run's end", seed, growth.back().largestWorkers));
        if (end)
        {
          // ADR-137: the whole world's snapshot at the end of the run, saved into a folder of the tool's own.
          const std::filesystem::path folder = std::filesystem::temp_directory_path() / L"SpeciesPerformance-snapshot";
          snapshot = Performance::MeasureSnapshot(*end, folder);
          if (!snapshot)
          {
            Print(stderr, std::format("the snapshot cannot be saved in {}", folder.string()));
          }
          else if (const auto saved = Performance::ReadSavedWorld(folder, *tuning, *records);
                   !saved || saved->run.steps != growth.back().steps)
          {
            // ADR-156: the snapshot read as a saved world gives the run's own growth steps.
            Print(stderr, "the snapshot read as a saved world does not give the run's growth steps");
            return 1;
          }
          std::error_code ignored;
          std::filesystem::remove_all(folder, ignored);
        }
        if (end && Wanted(_options, "settlement-at-work"))
        {
          runs.push_back(Performance::MeasureWorld("settlement-at-work", *end, ticks(1000, 5)));
        }
      }
    }
    if (Wanted(_options, "region-bound-settled"))
    {
      // A settlement at each of the four sites, each run by a scripted steward for the run's length, then measured.
      Species::WorldStart start;
      start.key = SCENE_KEY;
      start.seed = terrain->seed;
      start.stewardSite = std::uint8_t{0};
      const auto world = Species::StartWorld(start, terrain, *tuning, &*records);
      world->SetRunEnd(std::numeric_limits<std::uint64_t>::max()); // ADR-163: measured after the run's length, still running
      Species::RegionSimulation& region = world->Region(0);
      for (std::uint8_t site = 0; site < 4; ++site)
      {
        if (site > 0)
        {
          static_cast<void>(region.FoundSettlement(site, false));
        }
        static_cast<void>(region.AddScriptedSteward(site));
      }
      Neuron::WorkerPool serial(0);
      for (std::uint64_t tick = 0; tick < growthTicks; ++tick)
      {
        world->Step(tick, serial);
      }
      runs.push_back(Performance::MeasureWorld("region-bound-settled", *world, ticks(1000, 3)));
    }
  }
  // M3 task T9 (ADR-164): the region at M3's bound, four settlements with two battles in progress, measured as the region
  // bound is; and, only when named, the valley run of the corpus (the M3 plan §4).
  std::vector<Performance::ValleyRun> valleys;
  if (Wanted(_options, "valley-battles") || Named(_options, "valley"))
  {
    const auto records = LoadRecords(_options);
    if (!records)
    {
      Print(stderr, std::format("the structure records cannot be read: {}", records.error()));
      return 1;
    }
    if (Wanted(_options, "valley-battles"))
    {
      Species::ValleyAtWarDesc desc;
      desc.key = SCENE_KEY;
      desc.terrain = terrain;
      desc.tuning = &*tuning;
      desc.structures = &*records;
      desc.settleTicks = ticks(desc.settleTicks, 20);
      const auto world = Species::StartValleyAtWar(desc);
      Print(stdout, std::format("valley-battles: {} battles in progress", world->Region(0).Battles().size()));
      runs.push_back(Performance::MeasureWorld("valley-battles", *world, ticks(1000, 3)));
    }
    if (Named(_options, "valley"))
    {
      // ADR-164: the steward's settlement's stated conflict policy is a Territorial settlement's decisions.
      valleys = Performance::RunValleys(VALLEY_SEED_FIRST, _options.quick ? 1 : VALLEY_SEED_COUNT, *tuning, *records,
                                        ticks(Species::RunTicks(*tuning), VALLEY_TICKS_QUICK), Species::Temperament::Territorial);
      for (const Performance::ValleyRun& valley : valleys)
      {
        const Performance::ValleyLines lines = Performance::JudgeValley(valley, *tuning);
        Print(stdout, std::format("the valley of seed {}: {} battles, {} of them the steward's, {} claims judged", valley.seed,
                                  lines.battles, lines.stewardBattles, lines.claimsJudged));
      }
    }
  }
  // M4 task T11 (ADR-189): the session over QuicTransport, only when named.
  std::optional<Performance::LinkRun> link;
  if (Named(_options, "link"))
  {
    const auto records = LoadRecords(_options);
    if (!records)
    {
      Print(stderr, std::format("the structure records cannot be read: {}", records.error()));
      return 1;
    }
    const auto minuteTicks = [&tuning](double _minutes)
    { return static_cast<std::uint64_t>(std::llround(_minutes * SECONDS_PER_MINUTE / tuning->ref.sim.tickPeriod)); };
    Performance::LinkSceneDesc desc;
    desc.tuning = &*tuning;
    desc.structures = &*records;
    desc.serverPort = _options.serverPort;
    desc.linkPort = _options.linkPort;
    desc.ticks = ticks(minuteTicks(LINK_MINUTES), LINK_TICKS_QUICK);
    desc.rejoinTick = ticks(minuteTicks(LINK_REJOIN_MINUTES), LINK_REJOIN_TICKS_QUICK);
    Print(stdout, std::format("link: {} ticks over QuicTransport {}", desc.ticks,
                              desc.linkPort != 0 ? std::format("through the link simulator on port {}", desc.linkPort) : "direct"));
    std::expected<Performance::LinkRun, std::string> ran = Performance::RunLink(desc);
    if (!ran)
    {
      Print(stderr, std::format("the link scene failed: {}", ran.error()));
      return 1;
    }
    link = *ran;
    Print(stdout, std::format("link: {} state datagrams, {} lost; a figure's longest freeze over one lost datagram {} ticks; hashes {}",
                              link->stateDatagrams, link->datagramsLost, link->figuresSingleLoss.longestTicks,
                              link->hashesMatch ? "equal" : "different"));
  }
  std::optional<double> drawNanoseconds;
  if (Wanted(_options, "draws"))
  {
    drawNanoseconds = DrawNanoseconds(_options.quick ? DRAW_VALUES_QUICK : DRAW_VALUES);
  }
  std::vector<Performance::TravelEdge> travel;
  if (Wanted(_options, "travel"))
  {
    const std::uint64_t seeds = _options.quick ? 1 : TRAVEL_SEED_COUNT;
    for (std::uint64_t seed = TRAVEL_SEED_FIRST; seed < TRAVEL_SEED_FIRST + seeds; ++seed)
    {
      const auto edges = Performance::MeasureTravel(seed, *tuning, _options.quick ? 1 : std::numeric_limits<std::size_t>::max(),
                                                    ticks(TRAVEL_TICKS_MAX, 50));
      travel.insert(travel.end(), edges.begin(), edges.end());
    }
  }
  std::vector<Performance::GenerationSample> generation;
  if (Wanted(_options, "generation"))
  {
    // WLD-165's generation and ADR-104's land route graph, valley by valley.
    generation = Performance::MeasureGeneration(GENERATION_SEED_FIRST, _options.quick ? 1 : GENERATION_SEED_COUNT, *tuning);
  }
  std::optional<Performance::LogFlood> logFlood;
  if (Wanted(_options, "log-flood"))
  {
    // ADR-091 and DBG-206: a log call's cost on the calling thread while eight threads flood the logger.
    logFlood = Performance::MeasureLogFlood(_options.quick ? LOG_FLOOD_RECORDS_QUICK : LOG_FLOOD_RECORDS);
  }

  std::string scenes;
  for (const Performance::SceneRun& run : runs)
  {
    scenes += std::format("{}\n{}", scenes.empty() ? "" : ",", SceneJson(run));
  }
  const std::string json = std::format(
    "{{\n  \"tool\": \"{}\",\n  \"configuration\": \"{}\",\n  \"platform\": \"{}\",\n  \"quick\": {},\n"
    "  \"seed\": {},\n  \"tick_period_s\": {},\n  \"valley_path_factor\": {},\n  \"draw_ns\": {},\n  \"scenes\": [{}\n  ],\n"
    "  \"travel\": {},\n  \"generation\": {},\n  \"growth\": {},\n  \"valley\": {},\n  \"link\": {},\n  \"snapshot\": {},\n"
    "  \"log\": {}\n}}\n",
    PRODUCT_NAME, CONFIGURATION, PLATFORM, _options.quick ? "true" : "false", SCENE_SEED, tuning->ref.sim.tickPeriod,
    tuning->wld.travel.valleyPathFactor, drawNanoseconds ? std::format("{:.4f}", *drawNanoseconds) : std::string("null"), scenes,
    TravelJson(travel), GenerationJson(generation), GrowthJson(growth, tuning->ref.sim.tickPeriod), ValleyJson(valleys, *tuning),
    link ? Performance::LinkJson(*link) : std::string("null"),
    snapshot ? std::format("{{\"bytes\": {}, \"encode_ms\": {:.3f}, \"save_ms\": {:.3f}}}", snapshot->bytes,
                           static_cast<double>(snapshot->encodeNanoseconds) / 1e6, static_cast<double>(snapshot->saveNanoseconds) / 1e6)
             : std::string("null"),
    LogFloodJson(logFlood));
  if (const int code = Write(_options, json); code != 0 || _options.out.empty())
  {
    return code;
  }
  Print(stdout, std::format("{} scenes, {} travel edges and {} valleys generated into {}", runs.size(), travel.size(), generation.size(),
                            _options.out.string()));
  return 0;
}

std::optional<Options> ParseOptions(std::span<const std::string_view> _arguments)
{
  Options options;
  for (std::size_t i = 0; i < _arguments.size(); ++i)
  {
    const std::string_view argument = _arguments[i];
    const bool hasValue = i + 1 < _arguments.size();
    if (argument == "--quick")
    {
      options.quick = true;
    }
    else if (argument == "--scenes" && hasValue)
    {
      std::string_view list = _arguments[++i];
      while (!list.empty())
      {
        const std::size_t comma = list.find(',');
        options.scenes.emplace_back(list.substr(0, comma));
        list = comma == std::string_view::npos ? std::string_view{} : list.substr(comma + 1);
      }
    }
    else if (argument == "--out" && hasValue)
    {
      options.out = std::filesystem::path(_arguments[++i]);
    }
    else if (argument == "--content" && hasValue)
    {
      options.content = std::filesystem::path(_arguments[++i]);
    }
    else if (argument == "--tuning-source" && hasValue)
    {
      options.tuningSource = std::filesystem::path(_arguments[++i]);
    }
    else if (argument == "--saved-world" && hasValue)
    {
      options.savedWorld = std::filesystem::path(_arguments[++i]);
    }
    else if ((argument == "--server-port" || argument == "--link-port") && hasValue)
    {
      const std::string_view value = _arguments[++i];
      std::uint16_t port = 0;
      if (const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), port);
          error != std::errc{} || end != value.data() + value.size() || port == 0)
      {
        Print(stderr, std::format("{} needs a port, not {}", argument, value));
        return std::nullopt;
      }
      (argument == "--server-port" ? options.serverPort : options.linkPort) = port;
    }
    else
    {
      Print(stderr, std::format("unknown option {}", argument));
      return std::nullopt;
    }
  }
  return options;
}

} // namespace

int main(int _argc, char** _argv)
{
  try
  {
    const std::span arguments(_argv, static_cast<std::size_t>(_argc));
    std::vector<std::string_view> values;
    for (const char* argument : arguments.subspan(1))
    {
      values.emplace_back(argument);
    }
    const std::optional<Options> options = ParseOptions(values);
    if (!options)
    {
      Print(stderr, "usage: SpeciesPerformance.exe [--quick] [--scenes <name,...>] [--saved-world <world folder>] [--out <file>] "
                    "[--content <folder> | --tuning-source <GameData>] [--server-port <port>] [--link-port <port>]");
      return 2;
    }
    return Run(*options);
  }
  catch (const std::exception& error)
  {
    std::fputs("SpeciesPerformance: ", stderr);
    std::fputs(error.what(), stderr);
    std::fputc('\n', stderr);
  }
  catch (...)
  {
    std::fputs("SpeciesPerformance: an unknown exception\n", stderr);
  }
  return 1;
}
