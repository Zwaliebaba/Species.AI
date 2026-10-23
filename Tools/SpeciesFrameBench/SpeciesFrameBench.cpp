#include "pch.h"

#include "AppWindow.h"
#include "AssetError.h"
#include "AudioEngine.h"
#include "ClientSession.h"
#include "CommandJournal.h"
#include "Construction.h"
#include "Development.h"
#include "DisplayAttempt.h"
#include "FatalPath.h"
#include "Files.h"
#include "FloatEnvironment.h"
#include "FrameProfile.h"
#include "FrameRenderer.h"
#include "InputBindings.h"
#include "InputRouter.h"
#include "IntegratedServer.h"
#include "KeyboardMouseBackEnd.h"
#include "Log.h"
#include "LooseContent.h"
#include "Picking.h"
#include "PlatformClock.h"
#include "PlatformLogSink.h"
#include "PlatformMessage.h"
#include "PlatformThread.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "RunId.h"
#include "SettlementView.h"
#include "Snapshot.h"
#include "StructureRecords.h"
#include "Tuning.h"
#include "ValleyAtWar.h"
#include "WorkerPool.h"
#include "WorldPresentation.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"
#include "WorldSounds.h"
#include "XAudio2Device.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
#include <utility>
#include <variant>
#include <vector>

// The frame bench (M1 task T18, ADR-126, O79): a development tool that never ships. It composes the client and the
// integrated server as Species.exe does, in a window of 1,920 x 1,080, and draws the rendered measurement scenes, each from
// seed 1: the battles with both sides under the AI and the camera fixed on the fight, from M2 task T19 the steward's
// settlement at its plateau with the HUD, seen from over its Wellspring (ADR-156), and from M3 task T9 the valley at war,
// four settlements with two battles in progress, seen over the attack on the steward's settlement (ADR-164). After a
// warm-up it records each frame's interval, from its start to the next frame's start (ADR-148), its work, its waits
// apart, its time on the GPU and the refreshes the display took to show the frames, for a number of ticks; from M3 task T16
// it also times a crowd order about once a second, from the frame that sends it to the present of the first frame that
// displays it (ADR-079). It writes them as JSON for Build/RunPerformance.py.
//
//   SpeciesFrameBench.exe [--quick] [--scenes <name,...>] [--out <file.json>] [--warp]
//
// --quick warms up for 1 s and measures 20 ticks, grows the settlement for a minute of play and steps the valley at war
// for 2 s, to show that the scenes run; its figures measure nothing. --warp, in a Debug build, draws on the WARP adapter,
// since CI's runner has no GPU.

namespace FrameBench
{

namespace
{

constexpr std::string_view PRODUCT_NAME = "SpeciesFrameBench";

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

// Every scene's world: the valley of seed 1 that the headless scenes measure (ADR-110).
constexpr std::uint64_t SCENE_SEED = 1;
// O79: a warm-up of 10 s from the first world frame, then 1,200 ticks measured; a quick run's are 1 s and 20 ticks.
constexpr Neuron::Nanoseconds WARM_UP{10'000'000'000};
constexpr Neuron::Nanoseconds WARM_UP_QUICK{1'000'000'000};
constexpr std::uint64_t MEASURED_TICKS = 1'200;
constexpr std::uint64_t MEASURED_TICKS_QUICK = 20;
// MVP-085: the size the scenes are drawn at.
constexpr Neuron::SizePx SCENE_SIZE{1920, 1080};
// A scene that has not drawn its world by then has failed.
constexpr Neuron::Nanoseconds LOAD_LIMIT{120'000'000'000};
// RND-032's low rate while the window is minimised or occluded (ADR-092).
constexpr Neuron::Nanoseconds SUSPENDED_POLL_PERIOD{100'000'000};
constexpr double NANOSECONDS_PER_MILLISECOND = 1.0e6;
// ADR-148: the longest frames written with where their time went.
constexpr std::size_t LONGEST_FRAMES = 10;
// ADR-156: the settlement scene's world, grown on one thread before the window opens, and the folder its snapshot is
// continued from; a quick run grows it for a minute of play.
constexpr Neuron::SipHashKey GROWN_KEY{0x62656e6368000001, 0x0123456789abcdef};
constexpr std::uint64_t GROWN_TICKS_QUICK = 600;
constexpr std::wstring_view GROWN_FOLDER = L"SpeciesFrameBench-settlement";
// ADR-164: the valley at war's world, made before the window opens and continued from its snapshot in its own folder; a
// quick run steps it for 2 s of play, before its battles start.
constexpr Neuron::SipHashKey VALLEY_KEY{0x62656e6368000002, 0x0123456789abcdef};
constexpr std::uint64_t VALLEY_SETTLE_TICKS_QUICK = 20;
constexpr std::wstring_view VALLEY_FOLDER = L"SpeciesFrameBench-valley";
constexpr double SECONDS_PER_HOUR = 3600.0;
// ADR-079 (M3 task T16): while measuring, one crowd order at a time, sent at most once a second, and given up on when its
// tick has not been displayed within 5 s.
constexpr Neuron::Nanoseconds ORDER_PERIOD{1'000'000'000};
constexpr Neuron::Nanoseconds ORDER_TIMEOUT{5'000'000'000};

// The pre-UI messages a failed start can show, as Species.exe has them (PLT-039); the bench runs unattended, so they are
// logged rather than shown.
constexpr std::array COMPILED_TEXTS{
  Neuron::CompiledText{"ui.error.internal", "The frame bench stopped because of an internal error (run id {runId})."},
  Neuron::CompiledText{"ui.error.gpu_unsupported",
                       "The frame bench cannot start: the graphics adapter does not support what the game needs.\n\nAdapter: "
                       "{adapter}\nMissing: {capability}"},
  Neuron::CompiledText{"ui.error.display_mode", "The frame bench cannot start: no window could be shown on this display."},
  Neuron::CompiledText{"ui.error.resolution_rejected", "The preferred screen size was rejected."},
  Neuron::CompiledText{"ui.error.content_missing", "The frame bench cannot start: its game data is missing."},
  Neuron::CompiledText{"ui.error.content_damaged", "The frame bench cannot start: its game data is damaged."},
};

// The rendered scenes of the M1 plan's T18 row (O79): the sandbox's battle of 400, judged against mvp.client.fps_target,
// and a synthetic battle of mvp.client.figures_max figures, reported; and of the M2 plan's T19 row, the settlement at its
// plateau, judged against the same target (ADR-156); and of the M3 plan's T9 row, the valley at war (ADR-164), judged too.
enum class SceneKind : std::uint8_t
{
  Sandbox,
  FiguresMax,
  Settlement,
  Valley
};

struct Scene
{
  std::string_view name;
  SceneKind kind = SceneKind::Sandbox;
};

constexpr std::array<Scene, 4> SCENES{{{"sandbox-drawn", SceneKind::Sandbox},
                                       {"figures-max-drawn", SceneKind::FiguresMax},
                                       {"settlement-drawn", SceneKind::Settlement},
                                       {"valley-drawn", SceneKind::Valley}}};

struct Options
{
  bool quick = false;
  bool warp = false;
  std::vector<std::string> scenes; // none: every scene
  std::filesystem::path out;       // none: standard output
};

struct Summary
{
  double p50 = 0.0;
  double p95 = 0.0;
  double p99 = 0.0;
  double max = 0.0;
  double mean = 0.0;
};

/// One measured frame, in milliseconds (ADR-148): its interval runs from its start, when the swap chain could take it, to
/// the next frame's start, and holds its work, its fence and present, and the next frame's wait to begin.
struct FrameRecord
{
  std::uint64_t frame = 0; // the renderer's number of the frame
  double interval = 0.0;
  double work = 0.0;         // the interval less every wait in it (ADR-120)
  double latencyWait = 0.0;  // the next frame's wait for the frame-latency object, and for the cap
  double fenceWait = 0.0;    // its wait for its slot's fence
  double present = 0.0;      // its Present call
  std::optional<double> gpu; // its command list's time on the GPU, read back two frames later
};

/// The settlement scene's world (ADR-156): the folder that holds its snapshot, the ticks it grew for, and its Workers and
/// structures when it was saved.
struct GrownSettlement
{
  std::filesystem::path folder;
  std::uint64_t ticks = 0;
  std::uint32_t workers = 0;
  std::uint32_t structures = 0;
};

/// The valley at war's world (ADR-164): the folder that holds its snapshot, the ticks it was stepped for, its battles in
/// progress and its living figures when it was saved, and the point over the attack on the steward's settlement.
struct ValleyAtWarWorld
{
  std::filesystem::path folder;
  std::uint64_t ticks = 0;
  std::uint32_t battles = 0;
  std::uint32_t figures = 0;
  Neuron::Vec3 fight;
};

/// What one rendered scene drew: every measured frame, the display's refreshes over them, and its figures.
struct SceneResult
{
  std::string name;
  std::uint32_t figures = 0; // the battle's figures at its start
  std::uint64_t ticks = 0;   // the server ticks measured
  std::vector<FrameRecord> frames;
  std::uint64_t presentsShown = 0;   // ADR-148: the display's statistics over the measured frames
  std::uint64_t refreshesShown = 0;  // the refreshes over which it showed them
  std::uint64_t suspendedFrames = 0; // frames not presented, minimised or occluded
  std::uint32_t aliveMin = std::numeric_limits<std::uint32_t>::max();
  std::uint32_t aliveMax = 0;
  double onScreenSum = 0.0;
  std::uint32_t onScreenMax = 0;
  Neuron::SizePx window;
  std::optional<GrownSettlement> grown;   // the settlement scene's world
  std::optional<ValleyAtWarWorld> valley; // the valley at war's
  // ADR-079 (M3 task T16): each crowd order's time, in ms, from the start of the frame that sent it to the present of the
  // first frame whose display time reaches the tick that applied it; and the orders given up on.
  std::vector<double> orders;
  std::uint32_t ordersLost = 0;
  std::uint32_t samples = 0; // ADR-015: the scene targets' samples per pixel while measuring
};

void Print(std::FILE* _stream, std::string_view _text)
{
  std::fwrite(_text.data(), 1, _text.size(), _stream);
  std::fputc('\n', _stream);
}

Summary SummaryOf(const std::vector<double>& _values)
{
  Summary summary;
  if (_values.empty())
  {
    return summary;
  }
  // The frame page's rule (ADR-120), so the bench and the overlay agree.
  summary.p50 = Neuron::LowerIndexPercentile(_values, 50.0);
  summary.p95 = Neuron::LowerIndexPercentile(_values, 95.0);
  summary.p99 = Neuron::LowerIndexPercentile(_values, 99.0);
  summary.max = *std::ranges::max_element(_values);
  double sum = 0.0;
  for (const double value : _values)
  {
    sum += value;
  }
  summary.mean = sum / static_cast<double>(_values.size());
  return summary;
}

// O75: the display's refresh periods that frames missed: an interval of n periods, rounded, misses n − 1 of them.
std::uint64_t MissedFrames(std::span<const FrameRecord> _frames, double _periodMs)
{
  std::uint64_t missed = 0;
  for (const FrameRecord& frame : _frames)
  {
    const auto periods = static_cast<std::uint64_t>(std::max(std::llround(frame.interval / _periodMs), 1LL));
    missed += periods - 1;
  }
  return missed;
}

// One figure of every measured frame that has it.
template <class Fn> std::vector<double> ValuesOf(std::span<const FrameRecord> _frames, Fn _figure)
{
  std::vector<double> values;
  values.reserve(_frames.size());
  for (const FrameRecord& frame : _frames)
  {
    if (const std::optional<double> value = _figure(frame); value.has_value())
    {
      values.push_back(*value);
    }
  }
  return values;
}

double MillisecondsOf(Neuron::Nanoseconds _time) noexcept
{
  return static_cast<double>(_time.count()) / NANOSECONDS_PER_MILLISECOND;
}

// ADR-148: the GPU time read back this frame joins the measured frame it belongs to, if that frame was measured.
void AttachGpuTime(std::vector<FrameRecord>& _frames, const Neuron::FrameTiming& _timing)
{
  if (_timing.gpuFrame == 0 || _frames.empty() || _timing.gpuFrame < _frames.front().frame)
  {
    return;
  }
  const auto index = static_cast<std::size_t>(_timing.gpuFrame - _frames.front().frame);
  if (index < _frames.size() && _frames[index].frame == _timing.gpuFrame)
  {
    _frames[index].gpu = MillisecondsOf(_timing.gpuTime);
  }
}

// The scene's fight (O79): the sandbox's sides meet between their sites, the synthetic battle stands about the
// valley's centre; the settlement is seen from over its Wellspring, as the settlement view enters it (ADR-156); each on
// the terrain.
Neuron::Vec3 FightOf(SceneKind _kind, const Species::RegionTerrain& _terrain, const Species::TuningMvp& _tuning)
{
  const Species::RegionLayout& layout = _terrain.layout;
  Neuron::Vec3 fight = layout.valleyCenter;
  const auto& sandbox = _tuning.mvp.sandbox;
  const Neuron::Vec3& steward = layout.sites[static_cast<std::size_t>(sandbox.stewardSite) % layout.sites.size()];
  if (_kind == SceneKind::Sandbox)
  {
    const Neuron::Vec3& ai = layout.sites[static_cast<std::size_t>(sandbox.aiSite) % layout.sites.size()];
    fight = (steward + ai) * 0.5;
  }
  else if (_kind == SceneKind::Settlement)
  {
    fight = steward;
  }
  fight.y = _terrain.field.Height(fight.x, fight.z);
  return fight;
}

// The replica's living figures, and those whose positions lie on the screen of _frame.
std::pair<std::uint32_t, std::uint32_t> FiguresOf(const Species::StateReplica& _replica, const Neuron::FrameView& _frame,
                                                  Neuron::SizePx _size)
{
  std::uint32_t alive = 0;
  std::uint32_t onScreen = 0;
  for (const auto& [id, state] : _replica.Latest())
  {
    const auto* figure = std::get_if<Species::ReplicatedFigure>(&state);
    if (figure == nullptr || figure->dead)
    {
      continue;
    }
    ++alive;
    const std::optional<Neuron::ScreenPoint> screen = Neuron::ProjectToScreen(Species::PositionOf(figure->position), _frame, _size);
    if (screen.has_value() && screen->x >= 0.0 && screen->y >= 0.0 && screen->x <= static_cast<double>(_size.width) &&
        screen->y <= static_cast<double>(_size.height))
    {
      ++onScreen;
    }
  }
  return {alive, onScreen};
}

// The settlement scene's world (ADR-156): seed 1's valley with the steward's settlement at mvp.sandbox.steward_site, run
// by its scripted steward on one thread until its population reaches mvp.settlement.population_max, where the growth
// scene's plateau lies (O84), or the run's length passes; then saved as a snapshot into a folder of the bench's own, from
// which the scene continues it. None when the records, the terrain or the save fail.
std::optional<GrownSettlement> GrowSettlement(const Neuron::LooseContent& _content, const Species::TuningMvp& _tuning, bool _quick)
{
  const std::expected<Species::StructureRecords, Neuron::AssetError> records = Species::LoadStructureRecords(_content);
  if (!records)
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "{}", Neuron::FormatAssetError(records.error()));
    return std::nullopt;
  }
  auto generated = Species::GenerateRegionTerrain(SCENE_SEED, _tuning);
  if (!generated)
  {
    return std::nullopt;
  }
  const auto terrain = std::make_shared<const Species::RegionTerrain>(std::move(*generated));
  Species::WorldStart start;
  start.key = GROWN_KEY;
  start.seed = terrain->seed;
  start.watched = true; // ADR-140: a scripted steward runs the steward's settlement until the scene's steward connects
  start.stewardSite = static_cast<std::uint8_t>(std::clamp<std::int64_t>(_tuning.mvp.sandbox.stewardSite, 0, 3));
  const std::unique_ptr<Species::WorldSimulation> world = Species::StartWorld(start, terrain, _tuning, &*records);
  world->SetRunEnd(std::numeric_limits<std::uint64_t>::max()); // ADR-163: the scene draws a settlement, not a run's end
  const Species::RegionSimulation& region = world->Region(0);
  const auto runTicks = static_cast<std::uint64_t>(
    std::max<long long>(1, std::llround(_tuning.mvp.run.lengthHours * SECONDS_PER_HOUR / _tuning.ref.sim.tickPeriod)));
  const std::uint64_t limit = _quick ? GROWN_TICKS_QUICK : runTicks;
  const auto cap = static_cast<std::uint32_t>(std::max<std::int64_t>(_tuning.mvp.settlement.populationMax, 1));
  Neuron::WorkerPool serial(0);
  GrownSettlement grown;
  while (grown.ticks < limit && Species::Population(region, region.Settlements().front()) < cap)
  {
    world->Step(grown.ticks++, serial);
  }
  grown.workers = Species::Population(region, region.Settlements().front());
  const Species::SlicedPool<Species::Structure>& structures = region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    grown.structures += structures.Used(slot) && structures.At(slot).owner == start.stewardSite ? 1U : 0U;
  }
  grown.folder = std::filesystem::temp_directory_path() / GROWN_FOLDER;
  std::error_code error;
  std::filesystem::remove_all(grown.folder, error);
  std::filesystem::create_directories(grown.folder, error);
  Neuron::SnapshotStore store(grown.folder, 1);
  if (error || !store.Save(Species::SnapshotOf(*world)))
  {
    Neuron::Log(Neuron::LogCategory::Diagnostics, Neuron::LogLevel::Error, "frame bench: the settlement's snapshot cannot be saved in {}",
                grown.folder.string());
    return std::nullopt;
  }
  return grown;
}

// The valley at war's world (ADR-164): seed 1's valley started by StartValleyAtWar, stepped on one thread until both its
// battles are in progress, then saved as a snapshot into a folder of the bench's own, from which the scene continues it.
// The camera stands over the attack on the steward's settlement: the middle of its expedition's living figures, or the
// steward's site while it has none. None when the records, the terrain or the save fail.
std::optional<ValleyAtWarWorld> PrepareValley(const Neuron::LooseContent& _content, const Species::TuningMvp& _tuning, bool _quick)
{
  const std::expected<Species::StructureRecords, Neuron::AssetError> records = Species::LoadStructureRecords(_content);
  if (!records)
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "{}", Neuron::FormatAssetError(records.error()));
    return std::nullopt;
  }
  auto generated = Species::GenerateRegionTerrain(SCENE_SEED, _tuning);
  if (!generated)
  {
    return std::nullopt;
  }
  Species::ValleyAtWarDesc desc;
  desc.key = VALLEY_KEY;
  desc.terrain = std::make_shared<const Species::RegionTerrain>(std::move(*generated));
  desc.tuning = &_tuning;
  desc.structures = &*records;
  if (_quick)
  {
    desc.settleTicks = VALLEY_SETTLE_TICKS_QUICK;
  }
  const std::unique_ptr<Species::WorldSimulation> world = Species::StartValleyAtWar(desc);
  const Species::RegionSimulation& region = world->Region(0);
  ValleyAtWarWorld valley;
  valley.ticks = world->NextTick();
  valley.battles = static_cast<std::uint32_t>(region.Battles().size());
  for (std::size_t team = 0; team < region.TeamCount(); ++team)
  {
    const Species::SlicedPool<Species::Figure>& figures = region.TeamAt(team).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      valley.figures += figures.Used(slot) && !figures.At(slot).dead ? 1U : 0U;
    }
  }
  valley.fight = FightOf(SceneKind::Settlement, *desc.terrain, _tuning);
  const auto steward = static_cast<std::uint8_t>(std::clamp<std::int64_t>(_tuning.mvp.sandbox.stewardSite, 0, 3));
  const Species::SlicedPool<Species::Figure>& attackers =
    region.TeamAt(Species::ExpeditionTeamOf(Species::ValleyRaidsOf(steward).front().attacker)).figures;
  Neuron::Vec3 sum;
  std::uint32_t living = 0;
  for (std::uint32_t slot = 0; slot < attackers.Capacity(); ++slot)
  {
    if (attackers.Used(slot) && !attackers.At(slot).dead)
    {
      sum = sum + attackers.At(slot).position;
      ++living;
    }
  }
  if (living > 0)
  {
    valley.fight = sum * (1.0 / static_cast<double>(living));
    valley.fight.y = desc.terrain->field.Height(valley.fight.x, valley.fight.z);
  }
  valley.folder = std::filesystem::temp_directory_path() / VALLEY_FOLDER;
  std::error_code error;
  std::filesystem::remove_all(valley.folder, error);
  std::filesystem::create_directories(valley.folder, error);
  Neuron::SnapshotStore store(valley.folder, 1);
  if (error || !store.Save(Species::SnapshotOf(*world)))
  {
    Neuron::Log(Neuron::LogCategory::Diagnostics, Neuron::LogLevel::Error, "frame bench: the valley's snapshot cannot be saved in {}",
                valley.folder.string());
    return std::nullopt;
  }
  return valley;
}

/// The client's parts that every scene shares: the clock, the window, the renderer, the content and the records, the
/// input router that reads the window's close request, the settlement scene's world when it was grown, and the valley at
/// war's when it was made.
struct Bench
{
  Neuron::PlatformClock* clock = nullptr;
  Neuron::AppWindow* window = nullptr;
  Neuron::FrameRenderer* renderer = nullptr;
  Neuron::InputRouter* router = nullptr;
  Neuron::KeyboardMouseBackEnd* keyboardMouse = nullptr;
  const Neuron::LooseContent* content = nullptr;
  const Species::TuningMvp* tuning = nullptr;
  const Species::TuningPresentation* presentation = nullptr;
  const GrownSettlement* grown = nullptr;
  const ValleyAtWarWorld* valley = nullptr;
  Neuron::AudioEngine* audio = nullptr; // ADR-173: the scenes' sounds, as Species.exe plays them
};

enum class SceneEnd : std::uint8_t
{
  Measured,
  Closed,  // the window was closed
  Stalled, // the world was not drawn within the load limit
  ServerFailed
};

// One scene: its world started on the integrated server, the session drawn until the world is in view, the camera aimed
// at the fight, the warm-up, then every frame of the measured ticks.
SceneEnd RunScene(const Scene& _scene, const Options& _options, const Bench& _bench, SceneResult& _result)
{
  _result.name = std::string(_scene.name);
  const Species::TuningMvp& tuning = *_bench.tuning;
  Species::IntegratedServer::Desc serverDesc;
  serverDesc.clock = _bench.clock;
  serverDesc.content = _bench.content;
  serverDesc.worldSeed = SCENE_SEED;
  serverDesc.sandbox = true;
  serverDesc.watched = true; // O77, O79: both sides under the AI
  _result.figures = static_cast<std::uint32_t>(2 * tuning.mvp.sandbox.sideWorkers);
  if (_scene.kind == SceneKind::FiguresMax)
  {
    serverDesc.syntheticFigures = static_cast<std::uint32_t>(tuning.mvp.client.figuresMax);
    _result.figures = serverDesc.syntheticFigures;
  }
  else if (_scene.kind == SceneKind::Settlement)
  {
    // ADR-156: the grown world continued as the front end continues a saved one, its steward the scene's client.
    if (_bench.grown == nullptr)
    {
      return SceneEnd::ServerFailed;
    }
    serverDesc.sandbox = false;
    serverDesc.watched = false;
    serverDesc.snapshotFolder = _bench.grown->folder;
    serverDesc.restoreOnly = true;
    _result.figures = _bench.grown->workers;
    _result.grown = *_bench.grown;
  }
  else if (_scene.kind == SceneKind::Valley)
  {
    // ADR-164: the valley at war continued as the settlement is, its steward the scene's client.
    if (_bench.valley == nullptr)
    {
      return SceneEnd::ServerFailed;
    }
    serverDesc.sandbox = false;
    serverDesc.watched = false;
    serverDesc.snapshotFolder = _bench.valley->folder;
    serverDesc.restoreOnly = true;
    _result.figures = _bench.valley->figures;
    _result.valley = *_bench.valley;
  }
  Species::IntegratedServer server(serverDesc);
  if (const auto started = server.Start(); !started)
  {
    // A record's fault, or the grown world's that cannot be continued (ADR-155).
    if (const auto* fault = std::get_if<Neuron::AssetError>(&started.error()); fault != nullptr)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "{}", Neuron::FormatAssetError(*fault));
    }
    else
    {
      Neuron::Log(Neuron::LogCategory::Diagnostics, Neuron::LogLevel::Error, "frame bench: the grown settlement cannot be continued");
    }
    return SceneEnd::ServerFailed;
  }
  Species::ClientSession::Desc sessionDesc;
  sessionDesc.transport = server.Connect();
  sessionDesc.showTitle = [&window = *_bench.window](std::string_view _title) { window.SetTitle(_title); };
  sessionDesc.productName = std::string(PRODUCT_NAME);
  sessionDesc.tuning = &tuning;
  sessionDesc.clock = _bench.clock;
  Species::ClientSession session(std::move(sessionDesc));
  Species::WorldPresentation::Desc worldDesc;
  worldDesc.tuning = &tuning;
  worldDesc.presentation = _bench.presentation;
  worldDesc.content = _bench.content;
  worldDesc.clock = _bench.clock;
  worldDesc.audio = _bench.audio;
  Species::WorldPresentation world(worldDesc);
  Species::SettlementView::Desc viewDesc;
  viewDesc.tuning = &tuning;
  viewDesc.presentation = _bench.presentation;
  viewDesc.planes = Species::WorldLookOf(tuning, *_bench.presentation, {}).planes;
  Species::SettlementView view(viewDesc);

  const Neuron::Nanoseconds warmUp = _options.quick ? WARM_UP_QUICK : WARM_UP;
  const std::uint64_t measuredTicks = _options.quick ? MEASURED_TICKS_QUICK : MEASURED_TICKS;
  const Neuron::Nanoseconds start = _bench.clock->Now();
  Neuron::Nanoseconds previousStart = start;
  bool aimed = false;
  std::optional<Neuron::Nanoseconds> warmStart;
  std::optional<std::uint64_t> firstTick;
  std::optional<FrameRecord> pending; // the last frame presented while measuring, until the next frame's start ends it
  // ADR-079: the order in flight, by the sequence the server gives it, and the tick that applied it once reported; the
  // order goes to the fight's point with no Workers selected, so it moves nothing and the scene's work is unchanged.
  struct Order
  {
    std::uint64_t sequence = 0;
    Neuron::Nanoseconds sent{0};
    std::optional<std::uint64_t> tick;
  };
  std::optional<Order> order;
  std::optional<Neuron::Nanoseconds> lastOrder;
  Neuron::Vec3 orderAt;
  SceneEnd end = SceneEnd::Measured;
  while (true)
  {
    // ADR-148: the frame starts once the swap chain can take it, and its start ends the last frame's interval.
    const Neuron::FrameOutcome begun = _bench.renderer->BeginFrame();
    const Neuron::FrameTiming begin = _bench.renderer->LastFrameTiming();
    const Neuron::Nanoseconds frameStart = _bench.clock->Now();
    if (pending.has_value() && begun == Neuron::FrameOutcome::Presented)
    {
      pending->interval = MillisecondsOf(frameStart - previousStart);
      pending->latencyWait = MillisecondsOf(begin.latencyWait + begin.capWait);
      pending->work = pending->interval - pending->latencyWait - pending->fenceWait - pending->present;
      _result.frames.push_back(*pending);
    }
    pending.reset();
    const double stepSeconds = std::chrono::duration<double>(frameStart - previousStart).count();
    previousStart = frameStart;
    _bench.window->PumpMessages();
    _bench.router->AdvanceFrame({.windowFocused = _bench.window->HasFocus()}, _bench.window->ClientSize());
    if (_bench.keyboardMouse->TakeCloseRequest())
    {
      end = SceneEnd::Closed;
      break;
    }
    if (!warmStart.has_value() && _bench.clock->Now() - start > LOAD_LIMIT)
    {
      end = SceneEnd::Stalled;
      break;
    }
    const Neuron::SizePx size = _bench.window->ClientSize();
    // The camera never moves: the view is updated without focus, and aimed at the fight once the world is entered.
    view.Update(session, *_bench.router, stepSeconds, size, false, &world.Hud());
    if (!aimed && view.InWorld() && session.Terrain() != nullptr)
    {
      orderAt = _scene.kind == SceneKind::Valley ? _bench.valley->fight : FightOf(_scene.kind, *session.Terrain(), tuning);
      view.EnterAt(orderAt);
      aimed = true;
    }
    // ADR-079: the order's tick from its outcome, which the view took; then the next order, read at this frame's start as
    // a player's input is (ADR-148).
    if (order.has_value() && !order->tick.has_value())
    {
      for (const Species::CommandReport& report : view.Reports())
      {
        if (report.sequence == order->sequence)
        {
          order->tick = report.tick;
        }
      }
    }
    if (firstTick.has_value() && !order.has_value() && (!lastOrder.has_value() || frameStart - *lastOrder >= ORDER_PERIOD))
    {
      if (const std::optional<std::uint8_t> team = session.CommandedTeam(); team.has_value())
      {
        Species::OrderWorkersCommand command;
        command.team = *team;
        command.position = {orderAt.x, orderAt.z};
        if (const std::optional<std::uint64_t> sequence = session.SendCommand(command); sequence.has_value())
        {
          order = Order{*sequence, frameStart, std::nullopt};
          lastOrder = frameStart;
        }
      }
    }
    session.Update();
    world.Update(session);
    world.UpdateHud(session, view, size, _bench.window->DpiScale()); // ADR-152: drawn as the client draws it, where a settlement exists
    // The display time this frame draws (ADR-084 item 5), read just before the presentation reads it, so never later.
    const std::optional<std::uint64_t> latestTick = session.ServerTick();
    const std::optional<double> displayTick = latestTick.has_value() && *latestTick > 0
                                                ? std::optional(static_cast<double>(*latestTick - 1) + session.InterpolationFraction())
                                                : std::nullopt;
    const Neuron::FrameInputs inputs = world.Frame(session, view, _bench.renderer->FramesPerSecond(), _bench.window->DpiScale());
    const std::uint64_t worldFrames = _bench.renderer->WorldFrames();
    const Neuron::FrameOutcome outcome =
      begun == Neuron::FrameOutcome::Suspended ? Neuron::FrameOutcome::Suspended : _bench.renderer->RenderFrame(inputs);
    const bool measuring = firstTick.has_value();
    if (outcome == Neuron::FrameOutcome::Suspended)
    {
      _result.suspendedFrames += measuring ? 1 : 0;
      _bench.clock->WaitUntil(_bench.clock->Now() + SUSPENDED_POLL_PERIOD);
      continue;
    }
    _bench.clock->FrameBoundary();
    // ADR-079: the order's time ends at the present of the first frame that displays its tick.
    if (order.has_value())
    {
      const Neuron::Nanoseconds presented = _bench.clock->Now();
      if (order->tick.has_value() && displayTick.has_value() && *displayTick >= static_cast<double>(*order->tick))
      {
        _result.orders.push_back(MillisecondsOf(presented - order->sent));
        order.reset();
      }
      else if (presented - order->sent > ORDER_TIMEOUT)
      {
        ++_result.ordersLost;
        order.reset();
      }
    }
    if (measuring)
    {
      const Neuron::FrameTiming& timing = _bench.renderer->LastFrameTiming();
      AttachGpuTime(_result.frames, timing);
      _result.presentsShown += timing.presentsShown;
      _result.refreshesShown += timing.refreshesShown;
      pending = FrameRecord{};
      pending->frame = timing.frame;
      pending->fenceWait = MillisecondsOf(timing.fenceWait);
      pending->present = MillisecondsOf(timing.present);
      const auto [alive, onScreen] = FiguresOf(session.Replica(), view.Camera().View(size), size);
      _result.aliveMin = std::min(_result.aliveMin, alive);
      _result.aliveMax = std::max(_result.aliveMax, alive);
      _result.onScreenSum += onScreen;
      _result.onScreenMax = std::max(_result.onScreenMax, onScreen);
      _result.window = size;
      _result.samples = _bench.renderer->SceneSamples();
      if (session.ServerTick().value_or(0) >= *firstTick + measuredTicks)
      {
        _result.ticks = session.ServerTick().value_or(0) - *firstTick;
        break;
      }
    }
    else if (aimed && worldFrames > 0)
    {
      // The warm-up runs from the first world frame drawn after the camera was aimed.
      if (!warmStart.has_value())
      {
        warmStart = frameStart;
      }
      else if (frameStart - *warmStart >= warmUp && session.ServerTick().has_value())
      {
        firstTick = *session.ServerTick();
        Neuron::Log(Neuron::LogCategory::Diagnostics, Neuron::LogLevel::Info, "frame bench: {} measures from tick {} for {} ticks",
                    _scene.name, *firstTick, measuredTicks);
      }
    }
  }
  session.Close();
  server.Stop();
  return end;
}

std::string JsonOf(const Summary& _summary)
{
  return std::format(R"({{"p50": {:.4f}, "p95": {:.4f}, "p99": {:.4f}, "max": {:.4f}, "mean": {:.4f}}})", _summary.p50, _summary.p95,
                     _summary.p99, _summary.max, _summary.mean);
}

// ADR-148: the frames of the longest intervals, longest first, each with where its time went; a GPU time not read back
// is null.
std::string LongestJsonOf(std::span<const FrameRecord> _frames)
{
  std::vector<const FrameRecord*> longest;
  longest.reserve(_frames.size());
  for (const FrameRecord& frame : _frames)
  {
    longest.push_back(&frame);
  }
  const std::size_t count = std::min(longest.size(), LONGEST_FRAMES);
  std::ranges::partial_sort(longest, longest.begin() + static_cast<std::ptrdiff_t>(count),
                            [](const FrameRecord* _a, const FrameRecord* _b) { return _a->interval > _b->interval; });
  std::string json;
  for (std::size_t i = 0; i < count; ++i)
  {
    const FrameRecord& frame = *longest[i];
    json += std::format(R"({}{{"interval": {:.4f}, "work": {:.4f}, "latency_wait": {:.4f}, "fence_wait": {:.4f}, "present": {:.4f}, )"
                        R"("gpu": {}}})",
                        json.empty() ? "" : ", ", frame.interval, frame.work, frame.latencyWait, frame.fenceWait, frame.present,
                        frame.gpu.has_value() ? std::format("{:.4f}", *frame.gpu) : std::string("null"));
  }
  return "[" + json + "]";
}

// ADR-156: how the settlement scene's world grew; null for the other scenes.
std::string GrownJsonOf(const std::optional<GrownSettlement>& _grown)
{
  return _grown.has_value()
           ? std::format(R"({{"ticks": {}, "workers": {}, "structures": {}}})", _grown->ticks, _grown->workers, _grown->structures)
           : std::string("null");
}

// ADR-164: the valley at war's world when it was saved; null for the other scenes.
std::string ValleyJsonOf(const std::optional<ValleyAtWarWorld>& _valley)
{
  return _valley.has_value()
           ? std::format(R"({{"ticks": {}, "battles": {}, "figures": {}}})", _valley->ticks, _valley->battles, _valley->figures)
           : std::string("null");
}

std::string JsonOf(const std::vector<SceneResult>& _results, const Options& _options, const Species::TuningMvp& _tuning,
                   const Neuron::DesktopMode& _display, std::string_view _adapter)
{
  // O100: the adapter the scenes drew on, which the performance script pins, with a quote or a backslash escaped.
  std::string adapter;
  for (const char c : _adapter)
  {
    if (c == '"' || c == '\\')
    {
      adapter += '\\';
    }
    adapter += c;
  }
  const auto fpsTarget = std::max<std::int64_t>(_tuning.mvp.client.fpsTarget, 1);
  const double periodMs = 1000.0 / static_cast<double>(fpsTarget);
  std::string scenes;
  for (const SceneResult& result : _results)
  {
    const std::span<const FrameRecord> records = result.frames;
    const double frames = static_cast<double>(std::max<std::size_t>(records.size(), 1));
    const std::vector<double> gpu = ValuesOf(records, [](const FrameRecord& _frame) { return _frame.gpu; });
    const std::uint64_t held = result.refreshesShown > result.presentsShown ? result.refreshesShown - result.presentsShown : 0;
    scenes += std::format(R"({}    {{"name": "{}", "seed": {}, "figures": {}, "ticks": {}, "frames": {}, "suspended_frames": {}, )"
                          R"("window": {{"width": {}, "height": {}}}, "interval_ms": {}, "work_ms": {}, "missed_frames": {}, )"
                          R"("gpu_frames": {}, "gpu_ms": {}, "latency_wait_ms": {}, "fence_wait_ms": {}, "present_ms": {}, )"
                          R"("display": {{"presents_shown": {}, "refreshes": {}, "held_refreshes": {}}}, "longest": {}, )"
                          R"("figures_alive": {{"min": {}, "max": {}}}, "figures_on_screen": {{"mean": {:.1f}, "max": {}}}, "grown": {}, )"
                          R"("valley": {}, "orders": {}, "orders_lost": {}, "order_ms": {}, "samples": {}}})",
                          scenes.empty() ? "" : ",\n", result.name, SCENE_SEED, result.figures, result.ticks, records.size(),
                          result.suspendedFrames, result.window.width, result.window.height,
                          JsonOf(SummaryOf(ValuesOf(records, [](const FrameRecord& _frame) { return std::optional(_frame.interval); }))),
                          JsonOf(SummaryOf(ValuesOf(records, [](const FrameRecord& _frame) { return std::optional(_frame.work); }))),
                          MissedFrames(records, periodMs), gpu.size(), JsonOf(SummaryOf(gpu)),
                          JsonOf(SummaryOf(ValuesOf(records, [](const FrameRecord& _frame) { return std::optional(_frame.latencyWait); }))),
                          JsonOf(SummaryOf(ValuesOf(records, [](const FrameRecord& _frame) { return std::optional(_frame.fenceWait); }))),
                          JsonOf(SummaryOf(ValuesOf(records, [](const FrameRecord& _frame) { return std::optional(_frame.present); }))),
                          result.presentsShown, result.refreshesShown, held, LongestJsonOf(records),
                          result.aliveMax == 0 ? 0U : result.aliveMin, result.aliveMax, result.onScreenSum / frames, result.onScreenMax,
                          GrownJsonOf(result.grown), ValleyJsonOf(result.valley), result.orders.size(), result.ordersLost,
                          result.orders.empty() ? std::string("null") : JsonOf(SummaryOf(result.orders)), result.samples);
  }
  return std::format("{{\n  \"tool\": \"{}\", \"configuration\": \"{}\", \"platform\": \"{}\", \"quick\": {}, \"fps_target\": {}, "
                     "\"figures_max\": {},\n  \"adapter\": \"{}\", \"display\": {{\"width\": {}, \"height\": {}, \"refresh_hz\": {}}},\n"
                     "  \"scenes\": [\n{}\n  ]\n}}\n",
                     PRODUCT_NAME, CONFIGURATION, PLATFORM, _options.quick ? "true" : "false", fpsTarget, _tuning.mvp.client.figuresMax,
                     adapter, _display.width, _display.height, _display.refreshHz, scenes);
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
    else if (argument == "--warp")
    {
      options.warp = true;
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
    else
    {
      Print(stderr, std::format("unknown option {}", argument));
      return std::nullopt;
    }
  }
  for (const std::string& name : options.scenes)
  {
    if (std::ranges::find(SCENES, std::string_view(name), &Scene::name) == SCENES.end())
    {
      Print(stderr, std::format("unknown scene {}", name));
      return std::nullopt;
    }
  }
  return options;
}

// The logs folder beside the executable, as Species.exe's until the locations of PLT-015 and PLT-016 exist.
std::filesystem::path LogsFolder()
{
  std::filesystem::path folder = Neuron::ExecutableFolder() / L"logs";
  std::error_code error;
  std::filesystem::create_directories(folder, error);
  return error ? std::filesystem::path{} : folder;
}

int Run(const Options& _options)
{
  Neuron::InstallFatalHandlers();
  Neuron::NameCurrentThread("main");
  Neuron::PlatformClock clock(std::make_unique<Neuron::CounterTimeSource>());
  Neuron::ApplyDefaultFloatEnvironment();
  const std::uint64_t runId = Neuron::DrawRunId();
  const Neuron::PlatformMessageContext messages{PRODUCT_NAME, runId, COMPILED_TEXTS};
  Neuron::SetPlatformMessageContext(&messages);
  Neuron::SetPlatformMessagesUnattended(true); // the performance script runs it; nobody would close a dialog
  Neuron::Logger::Desc logDesc;
  logDesc.clock = &clock;
  logDesc.directory = LogsFolder();
  logDesc.runId = runId;
  logDesc.header = {{"product", std::string(PRODUCT_NAME)}, {"milestone", "M1"}};
  Neuron::Logger logger(std::move(logDesc));
  Neuron::SetPlatformLogSink(&logger);

  // The cooked records of the loose-content folder beside the executable, as the programs read them (ADR-108).
  const std::expected<Neuron::LooseContent, Neuron::AssetError> content =
    Neuron::LooseContent::Mount(Neuron::ExecutableFolder() / L"content");
  if (!content)
  {
    Print(stderr, std::format("the content cannot be mounted: {}", Neuron::FormatAssetError(content.error())));
    return 1;
  }
  const std::expected<Species::TuningMvp, Neuron::AssetError> tuning = Species::LoadTuningMvp(*content);
  const std::expected<Species::TuningPresentation, Neuron::AssetError> presentation = Species::LoadTuningPresentation(*content);
  if (!tuning || !presentation)
  {
    Print(stderr, std::format("the records cannot be read: {}", Neuron::FormatAssetError(!tuning ? tuning.error() : presentation.error())));
    return 1;
  }

  // ADR-156, ADR-164: the settlement scene's world grows, and the valley at war's is made, before the window opens, so that
  // no window waits unpumped meanwhile.
  const auto wanted = [&_options](std::string_view _name)
  { return _options.scenes.empty() || std::ranges::find(_options.scenes, _name) != _options.scenes.end(); };
  std::optional<GrownSettlement> grown;
  if (wanted("settlement-drawn"))
  {
    grown = GrowSettlement(*content, *tuning, _options.quick);
    if (!grown)
    {
      Print(stderr, "the settlement scene's world cannot be grown");
      return 1;
    }
    Print(stdout, std::format("settlement-drawn: grown for {} ticks to {} Workers and {} structures", grown->ticks, grown->workers,
                              grown->structures));
  }
  std::optional<ValleyAtWarWorld> valley;
  if (wanted("valley-drawn"))
  {
    valley = PrepareValley(*content, *tuning, _options.quick);
    if (!valley)
    {
      Print(stderr, "the valley at war's world cannot be made");
      return 1;
    }
    Print(stdout, std::format("valley-drawn: stepped for {} ticks to {} battles in progress and {} figures", valley->ticks, valley->battles,
                              valley->figures));
  }

  // MVP-085: the scenes are drawn at SCENE_SIZE on the primary display. On a display of that size the borderless window
  // covers it at its native mode (O75); on a larger one, a window of that client size is centred on it (O85).
  Neuron::SizePx desktop = Neuron::FALLBACK_WINDOW_SIZE;
  Neuron::DesktopMode display; // the primary display's mode, which the performance script pins (O75)
  for (const Neuron::MonitorInfo& monitor : Neuron::EnumerateMonitors())
  {
    if (monitor.primary)
    {
      desktop = {monitor.desktop.width, monitor.desktop.height};
      display = monitor.desktop;
    }
  }
  const bool windowed = desktop.width > SCENE_SIZE.width && desktop.height > SCENE_SIZE.height;
  const Neuron::DisplayAttempt attempt =
    Neuron::StartupDisplayAttempts(windowed ? Neuron::WindowMode::Windowed : Neuron::WindowMode::BorderlessFullScreen,
                                   windowed ? SCENE_SIZE : Neuron::SizePx{}, desktop)
      .front();
  Neuron::AppWindow::Desc windowDesc;
  windowDesc.clock = &clock;
  windowDesc.title = std::string(PRODUCT_NAME);
  windowDesc.placement.mode = attempt.mode;
  windowDesc.renderSize = attempt.renderSize;
  std::expected<Neuron::AppWindow, Neuron::PlatformError> window = Neuron::AppWindow::Create(windowDesc);
  if (!window)
  {
    Print(stderr, "the window cannot be made");
    return 1;
  }
  Neuron::RenderConfig renderConfig;
  renderConfig.frameRateWindowSeconds = presentation->ref.sim.frame.fpsWindow;
  renderConfig.frameRateInitial = presentation->ref.sim.frame.fpsInitial;
#if ENGINE_DEVELOPMENT
  renderConfig.softwareAdapter = _options.warp;
#endif
  std::expected<std::unique_ptr<Neuron::FrameRenderer>, Neuron::StartupFailure> renderer =
    Neuron::FrameRenderer::Create(*window, clock, renderConfig);
  if (!renderer)
  {
    Print(stderr, std::format("the renderer cannot start on {}: {}", renderer.error().adapter, renderer.error().capability));
    return 1;
  }
  window->Show();

  Neuron::KeyboardMouseBackEnd keyboardMouse(*window);
  const std::vector<Neuron::ActionBindings> bindings = Species::DefaultBindings();
  Neuron::InputRouter router(bindings, keyboardMouse, &keyboardMouse);
  window->SetInputSink(&keyboardMouse);
  const Neuron::WindowListenerId listener = window->AddListener(
    [&keyboardMouse](const Neuron::WindowEvent& _event)
    {
      if (std::holds_alternative<Neuron::EndOfSession>(_event))
      {
        keyboardMouse.RequestClose();
      }
    });
  // ADR-173: the audio engine as Species.exe makes it, at the full master volume, silent with no output device.
  const Neuron::CounterTimeSource audioClock;
  Neuron::AudioEngine::Desc audioDesc;
  audioDesc.settings = Species::AudioSettingsOf(*presentation, static_cast<unsigned>(Species::MASTER_VOLUME_MAX));
  audioDesc.events = Species::GameSoundEvents(*content, *presentation);
  audioDesc.device = Neuron::CreateXAudio2Device(Species::AudioDeviceDescOf(*presentation));
  audioDesc.clock = &audioClock;
  Neuron::AudioEngine audio(std::move(audioDesc));
  const Bench bench{&clock,
                    &*window,
                    renderer->get(),
                    &router,
                    &keyboardMouse,
                    &*content,
                    &*tuning,
                    &*presentation,
                    grown ? &*grown : nullptr,
                    valley ? &*valley : nullptr,
                    &audio};
  std::vector<SceneResult> results;
  int code = 0;
  for (const Scene& scene : SCENES)
  {
    if (!wanted(scene.name))
    {
      continue;
    }
    SceneResult result;
    const SceneEnd end = RunScene(scene, _options, bench, result);
    if (end != SceneEnd::Measured)
    {
      Print(stderr, std::format("{} {}", scene.name,
                                end == SceneEnd::Closed    ? "was ended by closing the window"
                                : end == SceneEnd::Stalled ? "drew no world within the load limit"
                                                           : "could not start its server"));
      code = end == SceneEnd::Closed ? 3 : 1;
      break;
    }
    Print(stdout, std::format("{}: {} frames over {} ticks", scene.name, result.frames.size(), result.ticks));
    results.push_back(std::move(result));
  }
  for (const std::filesystem::path* folder : {grown ? &grown->folder : nullptr, valley ? &valley->folder : nullptr})
  {
    if (folder != nullptr)
    {
      std::error_code ignored;
      std::filesystem::remove_all(*folder, ignored);
    }
  }
  router.ConfinePointer(false);
  window->RemoveListener(listener);
  window->SetInputSink(nullptr);
  const std::string adapterName = (*renderer)->AdapterName();
  renderer->reset(); // PLT-043: the swap chain goes before the window
  if (code == 0)
  {
    const std::string json = JsonOf(results, _options, *tuning, display, adapterName);
    if (_options.out.empty())
    {
      std::fwrite(json.data(), 1, json.size(), stdout);
    }
    else
    {
      std::ofstream file(_options.out, std::ios::binary);
      file << json;
      if (!file)
      {
        Print(stderr, std::format("{} cannot be written", _options.out.string()));
        code = 1;
      }
    }
  }
  Neuron::SetPlatformLogSink(nullptr);
  Neuron::SetPlatformMessageContext(nullptr);
  return code;
}

} // namespace

} // namespace FrameBench

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
    const std::optional<FrameBench::Options> options = FrameBench::ParseOptions(values);
    if (!options)
    {
      FrameBench::Print(stderr, "usage: SpeciesFrameBench.exe [--quick] [--scenes <name,...>] [--out <file>] [--warp]");
      return 2;
    }
    return FrameBench::Run(*options);
  }
  catch (const std::exception& error)
  {
    std::fputs("SpeciesFrameBench: ", stderr);
    std::fputs(error.what(), stderr);
    std::fputc('\n', stderr);
  }
  catch (...)
  {
    std::fputs("SpeciesFrameBench: an unknown exception\n", stderr);
  }
  return 1;
}
