#include "pch.h"

#include "AssetError.h"
#include "Development.h"
#include "Files.h"
#include "LooseContent.h"
#include "MarkerTimings.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "Sandbox.h"
#include "SceneRun.h"
#include "SipHash.h"
#include "TravelScene.h"
#include "Tuning.h"
#include "WorldSimulation.h"

#include <algorithm>
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
#include <vector>

// The performance tool (M1 plan T10, ADR-110): a development tool that never ships. It runs the headless measurement
// scenes, each from a fixed seed and key with a recorded command stream, on one thread of the build it was built in,
// and writes what each step cost, read from the profile markers, as JSON for Build/RunPerformance.py.
//
//   SpeciesPerformance.exe [--quick] [--scenes <name,...>] [--out <file.json>] [--content <folder> | --tuning-source <GameData>]
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
};

bool Wanted(const Options& _options, std::string_view _scene)
{
  return _options.scenes.empty() || std::ranges::find(_options.scenes, _scene) != _options.scenes.end();
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

int Run(const Options& _options)
{
  const auto tuning = LoadTuning(_options);
  if (!tuning)
  {
    Print(stderr, std::format("the tuning record cannot be read: {}", tuning.error()));
    return 1;
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

  std::string scenes;
  for (const Performance::SceneRun& run : runs)
  {
    scenes += std::format("{}\n{}", scenes.empty() ? "" : ",", SceneJson(run));
  }
  const std::string json =
    std::format("{{\n  \"tool\": \"{}\",\n  \"configuration\": \"{}\",\n  \"platform\": \"{}\",\n  \"quick\": {},\n"
                "  \"seed\": {},\n  \"tick_period_s\": {},\n  \"valley_path_factor\": {},\n  \"draw_ns\": {},\n  \"scenes\": [{}\n  ],\n"
                "  \"travel\": {}\n}}\n",
                PRODUCT_NAME, CONFIGURATION, PLATFORM, _options.quick ? "true" : "false", SCENE_SEED, tuning->ref.sim.tickPeriod,
                tuning->wld.travel.valleyPathFactor, drawNanoseconds ? std::format("{:.4f}", *drawNanoseconds) : std::string("null"),
                scenes, TravelJson(travel));
  if (_options.out.empty())
  {
    std::fwrite(json.data(), 1, json.size(), stdout);
    return 0;
  }
  std::ofstream file(_options.out, std::ios::binary);
  file << json;
  if (!file)
  {
    Print(stderr, std::format("{} cannot be written", _options.out.string()));
    return 1;
  }
  Print(stdout, std::format("{} scenes and {} travel edges into {}", runs.size(), travel.size(), _options.out.string()));
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
      Print(stderr, "usage: SpeciesPerformance.exe [--quick] [--scenes <name,...>] [--out <file>] [--content <folder> | --tuning-source "
                    "<GameData>]");
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
