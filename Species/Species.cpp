#include "pch.h"

#include "AppWindow.h"
#include "AssetError.h"
#include "ClientSession.h"
#include "ClientSettings.h"
#include "Development.h"
#include "DisplayAttempt.h"
#include "FatalPath.h"
#include "FloatEnvironment.h"
#include "FrameRenderer.h"
#include "InputBindings.h"
#include "InputRouter.h"
#include "IntegratedServer.h"
#include "KeyboardMouseBackEnd.h"
#include "Log.h"
#include "LooseContent.h"
#include "PlatformClock.h"
#include "PlatformLogSink.h"
#include "PlatformMessage.h"
#include "PlatformThread.h"
#include "RunId.h"
#include "SettingsStore.h"
#include "SettlementView.h"
#include "Sha256.h"
#include "StartupOptions.h"
#include "TextEncoding.h"
#include "Tuning.h"
#include "UserData.h"
#include "WorldPresentation.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

// The client's entry point and composition root (ADR-090 item 2, DEC-221). It runs the start-up steps of PLT-038 that
// exist in M0, composes the integrated server through IntegratedServer.h and the client session (M0 task T7), and runs
// the frame loop of task T4, which draws the world frame of M1 task T13; the application's states of ADR-083 item 3 come
// with the front end.
namespace Species
{

namespace
{

constexpr std::string_view PRODUCT_NAME = "Species";
// PLT-016's product folder under Local AppData, without the publisher's folder until O49 names one (DEC-1004).
constexpr std::wstring_view PRODUCT_FOLDER = L"Species";

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
  Neuron::CompiledText{
    "ui.error.user_data_unavailable",
    "Species cannot write to its folder for your settings, so they will not be kept after this run.\n\nFolder: {folder}"},
  Neuron::CompiledText{"ui.error.user_data_inside_install",
                       "Species cannot start: the folder given for your data lies inside the game's own folder.\n\nFolder: {folder}"},
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
  Neuron::DisplayAttempt used; // the display settings it started with, which the store takes back (PLT-080)
};

Presentation CreatePresentation(const Neuron::PlatformClock& _clock, const Neuron::RenderConfig& _config, Neuron::WindowMode _storedMode,
                                Neuron::SizePx _storedRenderSize)
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
  // RND-021, PLT-112: the stored mode and render size, the desktop size while none is stored (ADR-150).
  const std::array attempts = Neuron::StartupDisplayAttempts(_storedMode, _storedRenderSize, desktop);
  Presentation presentation;
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
                                   Neuron::FrameRenderer::Create(*presentation.window, _clock, _config);
                                 if (!renderer.has_value())
                                 {
                                   if (renderer.error().error != Neuron::StartupError::SwapChainFailed)
                                   {
                                     Neuron::EndStartup(renderer.error());
                                   }
                                   return false;
                                 }
                                 presentation.renderer = std::move(*renderer);
                                 presentation.used = _attempt;
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

// The render check's limit on its run: a check that has not drawn its frames by then fails (ADR-113).
constexpr Neuron::Nanoseconds RENDER_CHECK_LIMIT{120'000'000'000};

// The frame loop until the quit, or, for a render check, until it has drawn _checkFrames world frames or its limit has
// passed.
void RunFrameLoop(Neuron::PlatformClock& _clock, Neuron::AppWindow& _window, Neuron::FrameRenderer& _renderer, ClientSession& _session,
                  WorldPresentation& _world, SettlementView& _view, const Neuron::SettingsStore& _settings,
                  std::optional<std::uint32_t> _checkFrames)
{
  // M1 task T14 (ADR-119): the keyboard and mouse back-end and the input router with the compiled-in bindings replace
  // O65's stand-in; the window's close request is the quit (PLT-009).
  Neuron::KeyboardMouseBackEnd keyboardMouse(_window);
  // ADR-151: the default set, then the active layout's layer and the user layer of binding.<action id> settings
  // (INP-162), built again when the keyboard layout changes (INP-166). A user binding takes effect at the next start
  // (INP-203, O94).
  const std::vector<Neuron::ActionBindings> bindings = DefaultBindings();
  Neuron::InputRouter router(bindings, keyboardMouse, &keyboardMouse, SettingAccessOf(_settings));
  std::string layoutTag = ActiveLayoutTag(_settings, _window.ActiveKeyboardLayoutTag());
  const std::size_t replaced = router.Rebuild(BindingLayers(_settings, _window.ActiveKeyboardLayoutTag()));
  Neuron::Log(Neuron::LogCategory::Input, Neuron::LogLevel::Info, "bindings built for the keyboard layout {}: {} replaced", layoutTag,
              replaced);
  bool layoutChanged = false;
  _window.SetInputSink(&keyboardMouse);
  const Neuron::WindowListenerId listener = _window.AddListener(
    [&keyboardMouse, &layoutChanged](const Neuron::WindowEvent& _event)
    {
      if (std::holds_alternative<Neuron::EndOfSession>(_event))
      {
        keyboardMouse.RequestClose(); // PLT-042
      }
      layoutChanged = layoutChanged || std::holds_alternative<Neuron::KeyboardLayoutChanged>(_event);
    });
  _renderer.OnDeviceRebuilt([] { Neuron::Log(Neuron::LogCategory::Render, Neuron::LogLevel::Info, "presentation resumed"); });

  // ADR-120: each root step's time joins the frame profile, the renderer's waits apart from its work, and every tenth
  // roll-up period the frame page's figures go to the log. ADR-148: a frame starts once the swap chain can take it, so
  // its input, its camera step and its interval are all read at that start; an interval runs from one frame's start to
  // the next and holds the earlier frame's work and render waits, and the later one's begin waits.
  Neuron::FrameProfile& profile = _world.Profile();
  const Neuron::Nanoseconds start = _clock.Now();
  Neuron::Nanoseconds previousFrame = start;
  Neuron::Nanoseconds previousRenderWaits{0};
#if ENGINE_DEVELOPMENT
  std::uint64_t loggedPeriods = 0;
#endif
  while (true)
  {
    const Neuron::FrameOutcome begun = _renderer.BeginFrame();
    const Neuron::Nanoseconds beginWaits = begun == Neuron::FrameOutcome::Suspended ? Neuron::Nanoseconds{0} : _renderer.LastFrameWaits();
    Neuron::Nanoseconds stepStart = _clock.Now();
    const auto endStep = [&](ClientStep _step)
    {
      const Neuron::Nanoseconds now = _clock.Now();
      profile.AddStep(static_cast<std::size_t>(_step), now - stepStart);
      stepStart = now;
    };
    _window.PumpMessages();
    if (std::exchange(layoutChanged, false))
    {
      // INP-166: a change of the system's layout rebuilds the bindings when the active layout changed with it.
      if (std::string active = ActiveLayoutTag(_settings, _window.ActiveKeyboardLayoutTag()); active != layoutTag)
      {
        layoutTag = std::move(active);
        static_cast<void>(router.Rebuild(BindingLayers(_settings, _window.ActiveKeyboardLayoutTag())));
        Neuron::Log(Neuron::LogCategory::Input, Neuron::LogLevel::Info, "bindings rebuilt for the keyboard layout {}", layoutTag);
      }
    }
    router.AdvanceFrame({.windowFocused = _window.HasFocus()}, _window.ClientSize(), _clock.Now());
    if (keyboardMouse.TakeCloseRequest())
    {
      break;
    }
    if (_checkFrames.has_value() && (_renderer.WorldFrames() >= *_checkFrames || _clock.Now() - start > RENDER_CHECK_LIMIT))
    {
      break;
    }
    // ADR-149: the UI before the camera and the verbs, so that a focused window's actions reach neither (UI-006).
    _world.UpdateUi(router, _window.ClientSize(), _window.DpiScale(), _window.HasFocus());
    if (router.Query(ActionOf(GameAction::WindowToggleMode)))
    {
      const bool windowed = _window.Mode() == Neuron::WindowMode::Windowed;
      _window.SetMode(windowed ? Neuron::WindowMode::BorderlessFullScreen : Neuron::WindowMode::Windowed, Neuron::Permanence::Temporary);
    }
#if ENGINE_DEVELOPMENT
    if (router.Query(ActionOf(GameAction::DebugRemoveDevice)))
    {
      _renderer.DebugRemoveDevice();
    }
    if (router.Query(ActionOf(GameAction::DebugOverlay)))
    {
      _world.ToggleOverlay();
    }
    const bool newSeed = router.Query(ActionOf(GameAction::DebugRestartSandboxNewSeed));
    if (newSeed || router.Query(ActionOf(GameAction::DebugRestartSandbox)))
    {
      static_cast<void>(_session.RequestSandboxRestart(newSeed));
    }
#endif
    // The camera's step is the real time since the previous frame's start (CAM-030), clamped by the camera.
    const Neuron::Nanoseconds frameStart = stepStart;
    const Neuron::Nanoseconds frameTime = frameStart - previousFrame;
    previousFrame = frameStart;
    _view.Update(_session, router, std::chrono::duration<double>(frameTime).count(), _window.ClientSize(), _window.HasFocus(),
                 &_world.Hud());
    endStep(ClientStep::Input);
    _session.Update();
    endStep(ClientStep::Session);
    _world.Update(_session);
    _world.UpdateHud(_session, _view, _window.ClientSize(), _window.DpiScale());
    const Neuron::FrameInputs inputs = _world.Frame(_session, _view, _renderer.FramesPerSecond(), _window.DpiScale());
    endStep(ClientStep::Presentation);
    const Neuron::FrameOutcome outcome =
      begun == Neuron::FrameOutcome::Suspended ? Neuron::FrameOutcome::Suspended : _renderer.RenderFrame(inputs);
    const Neuron::Nanoseconds renderWaits =
      outcome == Neuron::FrameOutcome::Suspended ? Neuron::Nanoseconds{0} : _renderer.LastFrameWaits() - beginWaits;
    profile.AddStep(static_cast<std::size_t>(ClientStep::Wait), beginWaits + renderWaits);
    stepStart += renderWaits;
    endStep(ClientStep::Render);
    if (outcome == Neuron::FrameOutcome::Suspended)
    {
      previousRenderWaits = Neuron::Nanoseconds{0};
      _clock.WaitUntil(_clock.Now() + SUSPENDED_POLL_PERIOD);
      continue;
    }
    _clock.FrameBoundary();
    profile.EndFrame(_clock.Now(), frameTime, previousRenderWaits + beginWaits);
    previousRenderWaits = renderWaits;
#if ENGINE_DEVELOPMENT
    if (profile.Periods() != loggedPeriods && profile.Periods() % STATISTICS_LOG_PERIODS == 0)
    {
      loggedPeriods = profile.Periods();
      Neuron::Log(Neuron::LogCategory::Diagnostics, Neuron::LogLevel::Info, "{}",
                  StatisticsLine(_world.FramePageOf(_session, _renderer.FramesPerSecond()), _world.Presentation()));
    }
#endif
  }

  router.ConfinePointer(false);
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

  // The start-up options come first, because a render check changes how the renderer starts (ADR-113).
  const Neuron::StartupOptions options = Neuron::ParseCommandLine();
  Neuron::RenderConfig renderConfig;
  std::optional<std::uint32_t> checkFrames;
#if ENGINE_DEVELOPMENT
  checkFrames = options.renderCheckFrames;
  if (checkFrames.has_value())
  {
    // ADR-113: the WARP adapter with the debug layer's GPU-based validation, and no dialog that nobody would close.
    Neuron::SetPlatformMessagesUnattended(true);
    renderConfig.softwareAdapter = true;
    renderConfig.gpuValidation = true;
    Neuron::Log(Neuron::LogCategory::Render, Neuron::LogLevel::Info, "render check: {} world frames on the WARP adapter", *checkFrames);
  }
#endif

  // PLT-016, PLT-018, PLT-025: the user-data root, which `--user-data` replaces; inside the install directory it ends the
  // start-up, and when it cannot be made the settings live in memory for this run (ADR-150).
  std::optional<std::filesystem::path> settingsFile;
  if (const auto root = Neuron::ResolveUserDataRoot(options.userDataDirectory, PRODUCT_FOLDER); root.has_value())
  {
    if (const std::optional<std::filesystem::path> folder = Neuron::UserDataFolder(*root, Neuron::SETTINGS_FOLDER))
    {
      settingsFile = *folder / Neuron::SETTINGS_FILE;
    }
    Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Info, "user data in {}", Neuron::ToUtf8(root->wstring()));
  }
  else
  {
    const std::string folder =
      Neuron::ToUtf8(options.userDataDirectory.value_or(std::filesystem::path(L"%LOCALAPPDATA%") / PRODUCT_FOLDER).wstring());
    const std::array arguments{Neuron::MessageArgument{"folder", folder}};
    if (root.error() == Neuron::UserDataFault::InsideInstall)
    {
      Neuron::EndWithMessage("ui.error.user_data_inside_install", arguments);
    }
    Neuron::ShowPlatformMessage("ui.error.user_data_unavailable", arguments, Neuron::MessageSeverity::Notice);
  }
  // PLT-030: the one settings store, before the window, the renderer and every reader, and destroyed after them.
  Neuron::SettingsStore settings(ClientSettingSpecs(), settingsFile);
  const RenderSettings stored = RenderSettingsOf(settings);
  renderConfig.presentMode = stored.uncapped ? Neuron::PresentMode::Uncapped : Neuron::PresentMode::VerticalSync;
  renderConfig.frameRateCap = stored.frameRateCap;
  renderConfig.lowLatency = stored.lowLatency;
  renderConfig.sceneSamplesPreferred = stored.sceneSamplesPreferred;
  const Neuron::WindowMode storedMode = stored.windowed ? Neuron::WindowMode::Windowed : Neuron::WindowMode::BorderlessFullScreen;

  bool checkPassed = true;
  {
    Presentation presentation = CreatePresentation(clock, renderConfig, storedMode, stored.renderSize);
    // PLT-080, R-META-424: the renderer's start-up writes the display settings it started with back, and saves them on a
    // first run or when they changed.
    const bool displayChanged = storedMode != presentation.used.mode || stored.renderSize != presentation.used.renderSize;
    WriteBackDisplay(settings, presentation.used.mode == Neuron::WindowMode::Windowed, presentation.used.renderSize);
    if (!settings.LoadedFromFile() || displayChanged)
    {
      // A failed save keeps the previous file and is logged by the store (PLT-081); R-META-407's dialog waits for the UI's
      // dialogs, so the run carries on with the settings in memory.
      if (const std::expected<void, Neuron::FileError> saved = settings.Save(); !saved.has_value())
      {
        Neuron::Log(Neuron::LogCategory::Platform, Neuron::LogLevel::Warning, "ui.dialog.settings_save_failed: the display settings");
      }
    }

    // Logs why the content cannot be loaded and ends with its message (AST-102); EndWithMessage does not return.
    const auto endWithError = [](const Neuron::AssetError& _error)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "{}", Neuron::FormatAssetError(_error));
      Neuron::EndWithMessage(_error.fault == Neuron::AssetFault::Missing ? "ui.error.content_missing" : "ui.error.content_damaged", {});
    };
    // The loose-content folder that the solution build's content build wrote beside the executable, or the one that
    // --loose-content names in a Debug build, mounted once for the client and its integrated server (AST-105, ADR-108).
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
    // The client's presentation values (owner decision O75, ADR-116), which the server never reads.
    const std::expected<TuningPresentation, Neuron::AssetError> presentationTuning = LoadTuningPresentation(*content);
    if (!presentationTuning)
    {
      endWithError(presentationTuning.error());
    }
    // R-SIM-576: the frame-rate counter's window and first reading, once the record is read.
    renderConfig.frameRateWindowSeconds = presentationTuning->ref.sim.frame.fpsWindow;
    renderConfig.frameRateInitial = presentationTuning->ref.sim.frame.fpsInitial;
    presentation.renderer->Reconfigure(renderConfig);
    // The integrated server is a child of this root (ADR-083 item 2): it runs on its own tick thread until the session
    // ends, and stopping it freezes the world (O4). It loads the tuning record when it starts (MVP-099).
    IntegratedServer::Desc serverDesc;
    serverDesc.clock = &clock;
    serverDesc.content = &*content;
    // M1 plan §6: the battle sandbox, from the seed of --seed or one the server draws (MVP-005); a Debug build's --world
    // starts a new world with the steward's settlement in its place, until T18's front end starts worlds (ADR-139).
    serverDesc.worldSeed = options.worldSeed;
    serverDesc.sandbox = !options.world;
    // O77: a Debug build's --watch, both sides under the AI; with --world, the steward's settlement under the scripted
    // steward (ADR-140).
    serverDesc.watched = options.watch;
    if (options.world)
    {
      serverDesc.stewardSite = static_cast<std::uint8_t>(std::clamp<std::int64_t>(clientTuning->mvp.sandbox.stewardSite, 0, 3));
    }
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
    WorldPresentation::Desc worldDesc;
    worldDesc.tuning = &*clientTuning;
    worldDesc.presentation = &*presentationTuning;
    worldDesc.content = &*content;
    worldDesc.clock = &clock;
    worldDesc.display = DisplayOptionsOf(settings); // ADR-150
#if ENGINE_DEVELOPMENT
    worldDesc.uiTest = options.uiTest; // ADR-145: the UI's test page
#endif
    WorldPresentation world(worldDesc);
    SettlementView::Desc viewDesc;
    viewDesc.tuning = &*clientTuning;
    viewDesc.presentation = &*presentationTuning;
    viewDesc.planes = WorldLookOf(*clientTuning, *presentationTuning, {}).planes;
    viewDesc.workerScaleFactor = worldDesc.display.workerScaleFactor;
#if ENGINE_DEVELOPMENT
    viewDesc.scriptedVerbs = checkFrames.has_value(); // ADR-125: the render check draws the verbs' marks
#endif
    SettlementView view(viewDesc);
#if ENGINE_DEVELOPMENT
    if (checkFrames.has_value())
    {
      world.ToggleOverlay(); // ADR-120: the render check draws the overlay, so that the debug layer sees its draws
    }
#endif

    presentation.window->Show();
    RunFrameLoop(clock, *presentation.window, *presentation.renderer, session, world, view, settings, checkFrames);
#if ENGINE_DEVELOPMENT
    const std::uint64_t worldFrames = presentation.renderer->WorldFrames();
#endif
    session.Close();
    server.Stop();
    // PLT-043: the renderer releases its swap chain before the window is destroyed.
    presentation.renderer.reset();
    presentation.window.reset();
#if ENGINE_DEVELOPMENT
    if (checkFrames.has_value())
    {
      const std::uint64_t layerMessages = Neuron::FrameRenderer::DebugLayerMessages();
      checkPassed = worldFrames >= *checkFrames && layerMessages == 0;
      Neuron::Log(Neuron::LogCategory::Render, checkPassed ? Neuron::LogLevel::Info : Neuron::LogLevel::Error,
                  "render check {}: {} of {} world frames drawn, {} debug-layer messages", checkPassed ? "passed" : "failed", worldFrames,
                  *checkFrames, layerMessages);
    }
#endif
  }

  Neuron::SetPlatformLogSink(nullptr);
  Neuron::SetPlatformMessageContext(nullptr);
  return checkPassed ? Neuron::EXIT_CODE_NORMAL : Neuron::EXIT_CODE_FATAL;
}

} // namespace

} // namespace Species

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int)
{
  return Species::Run();
}
