#include "pch.h"

#include "ConsoleStop.h"
#include "ContentDiagnostic.h"
#include "FatalPath.h"
#include "Files.h"
#include "FloatEnvironment.h"
#include "Log.h"
#include "PlatformClock.h"
#include "PlatformLogSink.h"
#include "PlatformThread.h"
#include "RunId.h"
#include "ServerHost.h"
#include "TextEncoding.h"
#include "Tuning.h"
#include "WorldServer.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
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

int Run()
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
    // The tuning record is read from the content root when the server starts (MVP-099; ADR-098's interim root).
    const std::optional<std::filesystem::path> contentRoot = Neuron::FindInterimContentRoot();
    Neuron::RecordResult<TuningMvp> tuning = std::unexpected(std::vector<Neuron::ContentDiagnostic>{});
    if (contentRoot)
    {
      tuning = LoadTuningMvp(*contentRoot);
    }
    if (!tuning)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the server cannot start: its game data is missing or damaged");
      Print(stderr, contentRoot ? "the game data cannot be read:" : "no GameData folder beside the executable or above it");
      for (const Neuron::ContentDiagnostic& diagnostic : tuning.error())
      {
        const std::string finding = Neuron::FormatDiagnostic(diagnostic);
        Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "{}", finding);
        Print(stderr, finding);
      }
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
      WorldServer world(worldDesc);
      Neuron::ServerHost::Desc hostDesc;
      hostDesc.clock = &clock;
      hostDesc.tickPeriod = TickPeriodOf(world.Tuning());
      hostDesc.workers = SERVER_WORKERS;
      Neuron::ServerHost host(std::move(hostDesc), world);
      host.Start();
      Print(stdout, std::format("the world runs, a tick every {} ms; Ctrl+C stops it",
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
int wmain()
{
  return Species::Run();
}
