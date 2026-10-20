#pragma once

#include "AppWindow.h"
#include "Development.h"
#include "PresentPolicy.h"

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
/// loop. Until the frame graph exists, a frame is the clear to black of M0 task T4.
class FrameRenderer
{
public:
  // RND-020 as far as M0 has it: the capability checks and the device, then the swap chain at the window's client
  // size. The GPU resource factory on the device, the root signatures, the catalogue and the frame graph join this
  // order where RND-020 puts them when they exist. The renderer keeps the window's address, so the window neither moves
  // nor ends while the renderer lives.
  [[nodiscard]] static std::expected<std::unique_ptr<FrameRenderer>, StartupFailure> Create(AppWindow& _window, const PlatformClock& _clock,
                                                                                            const RenderConfig& _config);

  FrameRenderer(const FrameRenderer&) = delete;
  FrameRenderer& operator=(const FrameRenderer&) = delete;
  ~FrameRenderer();

  void Reconfigure(const RenderConfig& _config);

  // Waits for the frame-latency object and the cap, records, executes and presents. A size change of the window is
  // applied first (RND-030), and a device removal found anywhere in the frame starts the rebuild of ADR-008.
  [[nodiscard]] FrameOutcome RenderFrame();

  // DeviceRebuilt: raised after a device-removal rebuild, once presentation runs again (RND-035).
  void OnDeviceRebuilt(std::function<void()> _listener);

#if ENGINE_DEVELOPMENT
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
