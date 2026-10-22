#include "pch.h"

#include "AssetError.h"
#include "ConsoleStop.h"
#include "FatalPath.h"
#include "Files.h"
#include "FloatEnvironment.h"
#include "Log.h"
#include "LooseContent.h"
#include "PlatformClock.h"
#include "PlatformLogSink.h"
#include "PlatformThread.h"
#include "RunId.h"
#include "ServerHost.h"
#include "Sha256.h"
#include "StartupOptions.h"
#include "StructureRecords.h"
#include "TextEncoding.h"
#include "Tuning.h"
#include "WorldServer.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

// The headless server's entry point and composition root (ADR-090 item 2, ADR-100). It composes the same world as the
// client's integrated server, the headless library's WorldServer behind ServerHost, on the real clock at the period
// the tuning record states, and runs it until its console asks it to stop; stopping freezes the world (O4). At M0 it
// has no transport listener, because the MsQuic transport is not built yet, and it saves no snapshot, because saving
// starts at M2 (MVP-075).
namespace Species
{

namespace
{

constexpr std::string_view PRODUCT_NAME = "SpeciesServer";

// ADR-080 item 7's integrated server runs at most 2 workers; M0's one region needs no more, and the dedicated hosts of
// ADR-080 item 8 size the pool when a world has more regions than one.
constexpr std::size_t SERVER_WORKERS = 2;

// What the operator reads in the console; the log holds the rest.
void Print(std::FILE* _stream, std::string_view _line)
{
  std::fputs(std::format("{}: {}\n", PRODUCT_NAME, _line).c_str(), _stream);
  std::fflush(_stream);
}

// The logs folder beside the executable, apart from the client's, until the server's data locations are decided
// (PLT-005, DEC-251; ADR-092's interim folder). An empty path, when it cannot be made, leaves the logger without a file.
std::filesystem::path InterimLogsFolder()
{
  std::filesystem::path folder = Neuron::ExecutableFolder() / L"logs" / L"server";
  std::error_code error;
  std::filesystem::create_directories(folder, error);
  return error ? std::filesystem::path{} : folder;
}

int Run(std::span<const std::wstring> _arguments)
{
  // PLT-038's steps that a headless process has (PLT-005): the fatal handlers, the thread's name, the clock and the
  // floating-point environment. A failure is written to the log and the console, never shown as a dialog.
  Neuron::InstallFatalHandlers();
  Neuron::NameCurrentThread("main");
  const Neuron::PlatformClock clock(std::make_unique<Neuron::CounterTimeSource>());
  Neuron::ApplyDefaultFloatEnvironment();
  const std::uint64_t runId = Neuron::DrawRunId();

  Neuron::Logger::Desc logDesc;
  logDesc.clock = &clock;
  logDesc.directory = InterimLogsFolder();
  logDesc.runId = runId;
  logDesc.header = {{"product", std::string(PRODUCT_NAME)}, {"milestone", "M0"}};
  Neuron::Logger logger(std::move(logDesc));
  Neuron::SetPlatformLogSink(&logger);
  Print(stdout, std::format("run {:016x}, log {}", runId, Neuron::ToUtf8(logger.FilePath().native())));

  int exitCode = Neuron::EXIT_CODE_NORMAL;
  {
    // The loose-content folder that the solution build's content build wrote beside the executable, or the one that
    // --loose-content names in a Debug build (AST-105, ADR-108); the cooked tuning record is loaded from it when the
    // server starts (MVP-099).
    const Neuron::StartupOptions options = Neuron::ParseArguments(_arguments);
    for (const std::string& ignored : options.ignoredArguments)
    {
      Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Warning, "the start-up argument {} is unknown and ignored", ignored);
      Print(stderr, std::format("the argument {} is unknown and ignored", ignored));
    }
    const std::expected<Neuron::LooseContent, Neuron::AssetError> content = Neuron::LooseContent::Mount(Neuron::ContentDirectory(options));
    std::expected<TuningMvp, Neuron::AssetError> tuning = std::unexpected(content ? Neuron::AssetError{} : content.error());
    std::expected<StructureRecords, Neuron::AssetError> structures = std::unexpected(Neuron::AssetError{});
    if (content)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Info,
                  "loose content of {} entries mounted from {}; simulation content identity {}, reported as modified (AST-105)",
                  content->Entries().size(), Neuron::ToUtf8(content->Folder().wstring()), Neuron::ShortHex(content->Identity()));
      tuning = LoadTuningMvp(*content);
      structures = LoadStructureRecords(*content); // ADR-129
    }
    const std::optional<Neuron::AssetError> fault =
      !tuning ? std::optional(tuning.error())
              : (!structures ? std::optional(structures.error()) : CheckStructureRecords(*structures, *tuning)); // ADR-130
    if (fault)
    {
      const std::string reason = Neuron::FormatAssetError(*fault);
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the server cannot start: {}", reason);
      Print(stderr, std::format("the game data is {}: {}", fault->fault == Neuron::AssetFault::Missing ? "missing" : "damaged", reason));
      exitCode = Neuron::EXIT_CODE_FATAL;
    }
    else if (!Neuron::InstallConsoleStopHandler())
    {
      Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Error, "the console's stop handler could not be installed");
      Print(stderr, "the console's stop handler could not be installed");
      exitCode = Neuron::EXIT_CODE_FATAL;
    }
    else
    {
      WorldServer::Desc worldDesc;
      worldDesc.tuning = *tuning;
      worldDesc.structures = std::move(*structures);
      // M1 plan §6: the battle sandbox, from the seed of --seed or one the server draws (MVP-005); a Debug build's --world
      // starts a new world with the steward's settlement in its place (ADR-139).
      worldDesc.worldSeed = options.worldSeed;
      worldDesc.sandbox = !options.world;
      if (options.world)
      {
        worldDesc.stewardSite = static_cast<std::uint8_t>(std::clamp<std::int64_t>(tuning->mvp.sandbox.stewardSite, 0, 3));
      }
      worldDesc.watched = options.watch;                             // O77: a Debug build's --watch, both sides under the AI
      worldDesc.calendar = [&clock] { return clock.CalendarUtc(); }; // ADR-137: the world record's wall times
      WorldServer world(worldDesc);
      Neuron::ServerHost::Desc hostDesc;
      hostDesc.clock = &clock;
      hostDesc.tickPeriod = TickPeriodOf(world.Tuning());
      hostDesc.workers = SERVER_WORKERS;
      Neuron::ServerHost host(std::move(hostDesc), world);
      host.Start();
      // MVP-005: the seed that gives this world's terrain again.
      Print(stdout, std::format("the world of seed {} runs, a tick every {} ms; Ctrl+C stops it", world.Terrain()->seed,
                                std::chrono::duration_cast<std::chrono::milliseconds>(TickPeriodOf(world.Tuning())).count()));
      Neuron::WaitForConsoleStop();
      host.Stop();
      Print(stdout, std::format("the world froze before tick {}", world.Simulation() != nullptr ? world.Simulation()->NextTick() : 0));
    }
  }

  logger.Flush();
  Neuron::SetPlatformLogSink(nullptr);
  Neuron::ConsoleStopFinished();
  return exitCode;
}

} // namespace

} // namespace Species

// The Unicode console entry, the counterpart of the client's wWinMain. Like it, it catches nothing: an exception that
// leaves the composition root reaches Windows Error Reporting untouched (PLT-117).
int wmain(int _argc, wchar_t** _argv)
{
  const std::span arguments(_argv, static_cast<std::size_t>(_argc));
  const std::vector<std::wstring> options(arguments.begin() + (arguments.empty() ? 0 : 1), arguments.end());
  return Species::Run(options);
}
