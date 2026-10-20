#include "pch.h"

#include "AppWindow.h"
#include "AssetError.h"
#include "ClientSession.h"
#include "Development.h"
#include "DisplayAttempt.h"
#include "FatalPath.h"
#include "FloatEnvironment.h"
#include "FrameRenderer.h"
#include "IntegratedServer.h"
#include "Log.h"
#include "LooseContent.h"
#include "PlatformClock.h"
#include "PlatformLogSink.h"
#include "PlatformMessage.h"
#include "PlatformThread.h"
#include "RunId.h"
#include "Sha256.h"
#include "StartupOptions.h"
#include "TextEncoding.h"
#include "Tuning.h"

#include <array>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

// The client's entry point and composition root (ADR-090 item 2, DEC-221). It runs the start-up steps of PLT-038 that
// exist in M0, composes the integrated server through IntegratedServer.h and the client session (M0 task T7), and runs
// the frame loop of task T4; the application's states of ADR-083 item 3 come with the front end.
namespace Species
{

namespace
{

constexpr std::string_view PRODUCT_NAME = "Species";

// RND-032's low rate while the window is minimised or occluded (ADR-092).
constexpr Neuron::Nanoseconds SUSPENDED_POLL_PERIOD{100'000'000};

// The base-language texts of the pre-UI messages M0 can show, compiled in until the string table loads (PLT-039).
constexpr std::array COMPILED_TEXTS{
  Neuron::CompiledText{"ui.error.internal", "Species stopped because of an internal error. Please report it with the run id {runId}."},
  Neuron::CompiledText{
    "ui.error.gpu_unsupported",
    "Species cannot start: the graphics adapter does not support what the game needs.\n\nAdapter: {adapter}\nMissing: {capability}"},
  Neuron::CompiledText{"ui.error.display_mode", "Species cannot start: no window could be shown on this display."},
  Neuron::CompiledText{"ui.error.resolution_rejected", "The preferred screen size was rejected, so Species started with another one."},
  Neuron::CompiledText{"ui.error.content_missing", "Species cannot start: its game data is missing."},
  Neuron::CompiledText{"ui.error.content_damaged", "Species cannot start: its game data is damaged."},
};

constexpr std::intptr_t KEY_REPEAT_BIT = std::intptr_t{1} << 30; // WM_KEYDOWN lParam: the key was already down
constexpr std::intptr_t ALT_DOWN_BIT = std::intptr_t{1} << 29;   // WM_SYSKEYDOWN lParam: Alt is held

/// The stand-in for the input router in M0 (owner decision O65): the close request becomes the quit (PLT-009),
/// Alt+Enter switches between windowed and borderless full screen in place of the screen options, and in Debug builds
/// Ctrl+Shift+F11 removes the device in place of the development command of ADR-008. It goes when the input router
/// and the development commands exist.
class InputStandIn final : public Neuron::WindowInputSink
{
public:
  void OnWindowMessage(const Neuron::WindowMessage& _message) override
  {
    const bool firstPress = (_message.lParam & KEY_REPEAT_BIT) == 0;
    switch (_message.id)
    {
    case WM_CLOSE:
      m_quit = true;
      break;
    case WM_SYSKEYDOWN:
      if (_message.wParam == VK_RETURN && (_message.lParam & ALT_DOWN_BIT) != 0 && firstPress)
      {
        m_toggleMode = true;
      }
      break;
#if ENGINE_DEVELOPMENT
    case WM_KEYDOWN:
      // F12 is the system's debugger key, which breaks into an attached debugger, so the removal key is F11.
      if (_message.wParam == VK_F11 && firstPress && GetKeyState(VK_CONTROL) < 0 && GetKeyState(VK_SHIFT) < 0)
      {
        m_removeDevice = true;
      }
      break;
#endif
    default:
      break;
    }
  }

  void RequestQuit() noexcept
  {
    m_quit = true;
  }
  [[nodiscard]] bool QuitRequested() const noexcept
  {
    return m_quit;
  }
  [[nodiscard]] bool TakeModeToggle() noexcept
  {
    return std::exchange(m_toggleMode, false);
  }
  [[nodiscard]] bool TakeDeviceRemoval() noexcept
  {
    return std::exchange(m_removeDevice, false);
  }

private:
  bool m_quit = false;
  bool m_toggleMode = false;
  bool m_removeDevice = false;
};

// The logs folder beside the executable, until the locations of PLT-015 and PLT-016 exist (ADR-092). An empty path, when
// it cannot be made, leaves the logger without a file.
std::filesystem::path InterimLogsFolder()
{
  std::array<wchar_t, 32'768> path{};
  const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
  if (length == 0 || length >= path.size())
  {
    return {};
  }
  std::filesystem::path folder = std::filesystem::path(path.data()).parent_path() / L"logs";
  std::error_code error;
  std::filesystem::create_directories(folder, error);
  return error ? std::filesystem::path{} : folder;
}

// PLT-038 step 13 with the fallback chain of PLT-112: each attempt makes a window and a renderer, and a failure other
// than the swap chain's ends the start-up at once, because no display setting can mend it. The window lives on the
// heap because the renderer keeps its address.
struct Presentation
{
  std::unique_ptr<Neuron::AppWindow> window;
  std::unique_ptr<Neuron::FrameRenderer> renderer;
};

Presentation CreatePresentation(const Neuron::PlatformClock& _clock)
{
  const std::vector<Neuron::MonitorInfo> monitors = Neuron::EnumerateMonitors();
  Neuron::SizePx desktop = Neuron::FALLBACK_WINDOW_SIZE;
  for (const Neuron::MonitorInfo& monitor : monitors)
  {
    if (monitor.primary)
    {
      desktop = {monitor.desktop.width, monitor.desktop.height};
    }
  }
  // No settings store in M0: the stored mode is the default and no render size is stored (RND-021).
  const std::array attempts = Neuron::StartupDisplayAttempts(Neuron::WindowMode::BorderlessFullScreen, {}, desktop);
  Presentation presentation;
  const Neuron::RenderConfig config;
  const Neuron::DisplayOutcome outcome =
    Neuron::RunDisplayAttempts(attempts,
                               [&](const Neuron::DisplayAttempt& _attempt)
                               {
                                 presentation.renderer.reset();
                                 presentation.window.reset();
                                 Neuron::AppWindow::Desc desc;
                                 desc.clock = &_clock;
                                 desc.title = std::string(PRODUCT_NAME);
                                 desc.placement.mode = _attempt.mode;
                                 desc.renderSize = _attempt.renderSize;
                                 std::expected<Neuron::AppWindow, Neuron::PlatformError> window = Neuron::AppWindow::Create(desc);
                                 if (!window.has_value())
                                 {
                                   return false;
                                 }
                                 presentation.window = std::make_unique<Neuron::AppWindow>(std::move(*window));
                                 std::expected<std::unique_ptr<Neuron::FrameRenderer>, Neuron::StartupFailure> renderer =
                                   Neuron::FrameRenderer::Create(*presentation.window, _clock, config);
                                 if (!renderer.has_value())
                                 {
                                   if (renderer.error().error != Neuron::StartupError::SwapChainFailed)
                                   {
                                     Neuron::EndStartup(renderer.error());
                                   }
                                   return false;
                                 }
                                 presentation.renderer = std::move(*renderer);
                                 return true;
                               });
  if (!outcome.succeeded.has_value())
  {
    Neuron::EndStartup(Neuron::StartupFailure{Neuron::StartupError::SwapChainFailed, {}, {}});
  }
  if (outcome.resolutionRejected)
  {
    // The notice waits for the UI (PLT-112); until it exists it is logged.
    Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Warning, "ui.error.resolution_rejected: started with attempt {}",
                *outcome.succeeded + 1);
  }
  return presentation;
}

void RunFrameLoop(Neuron::PlatformClock& _clock, Neuron::AppWindow& _window, Neuron::FrameRenderer& _renderer, ClientSession& _session)
{
  InputStandIn input;
  _window.SetInputSink(&input);
  const Neuron::WindowListenerId listener = _window.AddListener(
    [&input](const Neuron::WindowEvent& _event)
    {
      if (std::holds_alternative<Neuron::EndOfSession>(_event))
      {
        input.RequestQuit(); // PLT-042
      }
    });
  _renderer.OnDeviceRebuilt([] { Neuron::Log(Neuron::LogCategory::Render, Neuron::LogLevel::Info, "presentation resumed"); });

  while (true)
  {
    _window.PumpMessages();
    if (input.QuitRequested())
    {
      break;
    }
    _session.Update();
    if (input.TakeModeToggle())
    {
      const bool windowed = _window.Mode() == Neuron::WindowMode::Windowed;
      _window.SetMode(windowed ? Neuron::WindowMode::BorderlessFullScreen : Neuron::WindowMode::Windowed, Neuron::Permanence::Temporary);
    }
#if ENGINE_DEVELOPMENT
    if (input.TakeDeviceRemoval())
    {
      _renderer.DebugRemoveDevice();
    }
#endif
    if (_renderer.RenderFrame() == Neuron::FrameOutcome::Suspended)
    {
      _clock.WaitUntil(_clock.Now() + SUSPENDED_POLL_PERIOD);
      continue;
    }
    _clock.FrameBoundary();
  }

  _window.RemoveListener(listener);
  _window.SetInputSink(nullptr);
}

int Run()
{
  // PLT-038 steps 1, 3, 4, 6 and 7; the options, locations, the OS check, the crash context and the settings come later.
  Neuron::InstallFatalHandlers();
  Neuron::NameCurrentThread("main");
  Neuron::PlatformClock clock(std::make_unique<Neuron::CounterTimeSource>());
  Neuron::ApplyDefaultFloatEnvironment();
  const std::uint64_t runId = Neuron::DrawRunId();
  const Neuron::PlatformMessageContext messages{PRODUCT_NAME, runId, COMPILED_TEXTS};
  Neuron::SetPlatformMessageContext(&messages);

  Neuron::Logger::Desc logDesc;
  logDesc.clock = &clock;
  logDesc.directory = InterimLogsFolder();
  logDesc.runId = runId;
  logDesc.header = {{"product", std::string(PRODUCT_NAME)}, {"milestone", "M0"}};
  Neuron::Logger logger(std::move(logDesc));
  Neuron::SetPlatformLogSink(&logger);

  {
    Presentation presentation = CreatePresentation(clock);

    // Logs why the content cannot be loaded and ends with its message (AST-102); EndWithMessage does not return.
    const auto endWithError = [](const Neuron::AssetError& _error)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "{}", Neuron::FormatAssetError(_error));
      Neuron::EndWithMessage(_error.fault == Neuron::AssetFault::Missing ? "ui.error.content_missing" : "ui.error.content_damaged", {});
    };
    // The loose-content folder that the solution build's content build wrote beside the executable, or the one that
    // --loose-content names in a Debug build, mounted once for the client and its integrated server (AST-105, ADR-108).
    const Neuron::StartupOptions options = Neuron::ParseCommandLine();
    for (const std::string& ignored : options.ignoredArguments)
    {
      Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Warning, "the start-up argument {} is unknown and ignored", ignored);
    }
    const std::expected<Neuron::LooseContent, Neuron::AssetError> content = Neuron::LooseContent::Mount(Neuron::ContentDirectory(options));
    if (!content)
    {
      endWithError(content.error());
    }
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Info,
                "loose content of {} entries mounted from {}; simulation content identity {}, reported as modified (AST-105)",
                content->Entries().size(), Neuron::ToUtf8(content->Folder().wstring()), Neuron::ShortHex(content->Identity()));
    // The client loads its own copy of the record: it builds the world's terrain from the server's seed with its own
    // data, as a client of a dedicated server does, and refuses a world whose digest differs (MVP-011).
    const std::expected<TuningMvp, Neuron::AssetError> clientTuning = LoadTuningMvp(*content);
    if (!clientTuning)
    {
      endWithError(clientTuning.error());
    }
    // The integrated server is a child of this root (ADR-083 item 2): it runs on its own tick thread until the session
    // ends, and stopping it freezes the world (O4). It loads the tuning record when it starts (MVP-099).
    IntegratedServer::Desc serverDesc;
    serverDesc.clock = &clock;
    serverDesc.content = &*content;
    // M1 plan §6: the battle sandbox, from the seed of --seed or one the server draws (MVP-005).
    serverDesc.worldSeed = options.worldSeed;
    serverDesc.sandbox = true;
    IntegratedServer server(serverDesc);
    if (const auto started = server.Start(); !started)
    {
      endWithError(started.error());
    }
    ClientSession::Desc sessionDesc;
    sessionDesc.transport = server.Connect();
    sessionDesc.showTitle = [&window = *presentation.window](std::string_view _title) { window.SetTitle(_title); };
    sessionDesc.productName = std::string(PRODUCT_NAME);
    sessionDesc.tuning = &*clientTuning;
    sessionDesc.clock = &clock;
    ClientSession session(std::move(sessionDesc));

    presentation.window->Show();
    RunFrameLoop(clock, *presentation.window, *presentation.renderer, session);
    session.Close();
    server.Stop();
    // PLT-043: the renderer releases its swap chain before the window is destroyed.
    presentation.renderer.reset();
    presentation.window.reset();
  }

  Neuron::SetPlatformLogSink(nullptr);
  Neuron::SetPlatformMessageContext(nullptr);
  return Neuron::EXIT_CODE_NORMAL;
}

} // namespace

} // namespace Species

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
  return Species::Run();
}
