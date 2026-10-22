#pragma once

#include "AppWindow.h"
#include "Development.h"
#include "PlatformClock.h"
#include "PresentPolicy.h"
#include "WorldScene.h"

#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace Neuron
{

class PlatformClock;

enum class StartupError : std::uint8_t // RND-020, RND-023
{
  NoCapableAdapter,       // no adapter passes the checks of ADR-002: ui.error.gpu_unsupported
  DeviceCreationFailed,   // the chosen adapter could not create the device
  SwapChainFailed,        // no attempt of the fallback chain created the swap chain: ui.error.display_mode
  PipelineCreationFailed, // a catalogue entry could not be created (ADR-005)
  FrameGraphInvalid       // the pass table failed validation (ADR-004)
};

// A failed start-up and what its message names (RND-020: the adapter and the capability it lacks).
struct StartupFailure
{
  StartupError error = StartupError::DeviceCreationFailed;
  std::string adapter;
  std::string capability;
};

struct RenderConfig // a change triggers Reconfigure (RND-030)
{
  std::uint32_t renderWidth = 0; // scene-target size (RND-025); the frame graph's targets use it
  std::uint32_t renderHeight = 0;
  bool lowLatency = false; // ADR-012
  PresentMode presentMode = PresentMode::VerticalSync;
  std::optional<float> frameRateCap; // frames per second
  // render.anti_aliasing (RND-442, ADR-015): 4, 2 or 1 scene samples, falling back to 2, then 1, where a count is not
  // supported. Read when the device objects are created, so a change applies at the next start (ADR-150).
  std::uint32_t sceneSamplesPreferred = 4;
  double frameRateWindowSeconds = 1.0; // ref.sim.frame.fps_window: the frame-rate counter's window (R-SIM-576)
  double frameRateInitial = 60.0;      // ref.sim.frame.fps_initial: what it reads until its first window completes
#if ENGINE_DEVELOPMENT
  // The render check of ADR-113: the WARP adapter in place of the hardware one, and the debug layer's GPU-based
  // validation (ADR-001, ADR-092). Both are fixed when the device is created.
  bool softwareAdapter = false;
  bool gpuValidation = false;
#endif
};

enum class FrameOutcome : std::uint8_t
{
  Presented,
  Suspended // minimised or occluded: the frame loop keeps pumping and polls at a low rate (RND-032)
};

/// Where a frame's time went outside its own work, and what the GPU and the display made of earlier frames (ADR-148).
/// A figure whose source had nothing to report this frame is zero.
struct FrameTiming
{
  std::uint64_t frame = 0;    // the frame's number, from 1
  Nanoseconds latencyWait{0}; // BeginFrame: the frame-latency object (ADR-012)
  Nanoseconds capWait{0};     // BeginFrame: the frame-rate cap
  Nanoseconds fenceWait{0};   // RenderFrame: its slot's fence
  Nanoseconds present{0};     // RenderFrame: the Present call
  // The GPU's time for an earlier frame, between the first and last timestamps of its command list, read once its
  // slot's fence has completed; gpuFrame is that frame's number, 0 for none.
  std::uint64_t gpuFrame = 0;
  Nanoseconds gpuTime{0};
  // The swap chain's statistics since they were last read: the presents the display has shown, and the refreshes from
  // the last one shown before them to the last one shown now. A refresh more than the presents held a frame on screen.
  std::uint32_t presentsShown = 0;
  std::uint32_t refreshesShown = 0;

  [[nodiscard]] Nanoseconds Waits() const noexcept
  {
    return latencyWait + capWait + fenceWait + present;
  }
};

struct RendererState;

/// The frame renderer (RND-001, RND-010): the Direct3D 12 device on the Agility SDK, its direct queue and the flip-model
/// swap chain of ADR-012, which always matches the window's client area. One exists per client process, created after
/// the window and destroyed before it; every call is made from the frame thread, which is the main thread of the frame
/// loop. A frame is the world frame of ADR-113 when its inputs hold a world whose surfaces are uploaded, and otherwise
/// the background colour.
class FrameRenderer
{
public:
  // RND-020: the capability checks and the device, the swap chain at the window's client size, the GPU resource
  // factory on the device, the root signature, the catalogue and the frame graph with its scene targets. The renderer
  // keeps the window's address, so the window neither moves nor ends while the renderer lives.
  [[nodiscard]] static std::expected<std::unique_ptr<FrameRenderer>, StartupFailure> Create(AppWindow& _window, const PlatformClock& _clock,
                                                                                            const RenderConfig& _config);

  FrameRenderer(const FrameRenderer&) = delete;
  FrameRenderer& operator=(const FrameRenderer&) = delete;
  ~FrameRenderer();

  void Reconfigure(const RenderConfig& _config);

  // ADR-148: the start of a frame, before its input is read. A size change of the window is applied (RND-030), and the
  // frame waits for the frame-latency object (ADR-012) and the frame-rate cap, unless the wait of a frame that was not
  // presented is still held. Suspended while the window is minimized or occluded (RND-032), or when the wait found the
  // device removed and rebuilt it (ADR-008).
  [[nodiscard]] FrameOutcome BeginFrame();
  // Records _inputs' frame, executes and presents, after its slot's fence. A frame not begun is begun here first, and a
  // size change of the window since is applied; a device removal found anywhere in the frame starts the rebuild.
  [[nodiscard]] FrameOutcome RenderFrame(const FrameInputs& _inputs);

  // DeviceRebuilt: raised after a device-removal rebuild, once presentation runs again (RND-035).
  void OnDeviceRebuilt(std::function<void()> _listener);

  // The frames presented with the world drawn, since the renderer was created.
  [[nodiscard]] std::uint64_t WorldFrames() const noexcept;
  // ADR-155: every frame presented, a world's or not, which a render check of the front end counts.
  [[nodiscard]] std::uint64_t PresentedFrames() const noexcept;
  // RND-011, R-SIM-576: the frames presented in the last window, per second of the window.
  [[nodiscard]] double FramesPerSecond() const noexcept;
  // ADR-120, ADR-148: the time the last frame has spent so far waiting for the frame-latency object, the frame-rate cap,
  // its slot's fence and the present, which the frame page leaves out of the frame's work.
  [[nodiscard]] Nanoseconds LastFrameWaits() const noexcept;
  // ADR-148: the last frame's waits, apart, with the GPU time and the display's statistics read during it.
  [[nodiscard]] const FrameTiming& LastFrameTiming() const noexcept;

#if ENGINE_DEVELOPMENT
  // The debug layer's messages of warning severity or above since the process started: none is the render check's
  // condition (ADR-113).
  [[nodiscard]] static std::uint64_t DebugLayerMessages() noexcept;

  // Removes the device (ID3D12Device5::RemoveDevice), so that the next frame finds the removal and rebuilds. The stand-in
  // for the development command that forces a removal (ADR-008) until development commands exist (owner decision O65).
  void DebugRemoveDevice();
#endif

private:
  explicit FrameRenderer(std::unique_ptr<RendererState> _state) noexcept;

  std::unique_ptr<RendererState> m_state;
};

// A failed start-up ends the client (RND-020, RND-023): a missing capability or a device that cannot be created through
// FatalExit with ui.error.gpu_unsupported, no swap chain with ui.error.display_mode, and the rest as defects.
[[noreturn]] void EndStartup(const StartupFailure& _failure);

} // namespace Neuron
