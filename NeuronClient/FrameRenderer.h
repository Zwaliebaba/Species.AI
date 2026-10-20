#pragma once

#include "AppWindow.h"
#include "Development.h"
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
  std::optional<float> frameRateCap;   // frames per second
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

  // Waits for the frame-latency object and the cap, records _inputs' frame, executes and presents. A size change of the
  // window is applied first (RND-030), and a device removal found anywhere in the frame starts the rebuild of ADR-008.
  [[nodiscard]] FrameOutcome RenderFrame(const FrameInputs& _inputs);

  // DeviceRebuilt: raised after a device-removal rebuild, once presentation runs again (RND-035).
  void OnDeviceRebuilt(std::function<void()> _listener);

  // The frames presented with the world drawn, since the renderer was created.
  [[nodiscard]] std::uint64_t WorldFrames() const noexcept;
  // RND-011, R-SIM-576: the frames presented in the last window, per second of the window.
  [[nodiscard]] double FramesPerSecond() const noexcept;

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
