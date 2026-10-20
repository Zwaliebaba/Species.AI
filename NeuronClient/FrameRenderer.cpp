#include "pch.h"

#include "FatalPath.h"
#include "FrameRenderer.h"
#include "GraphicsDevice.h"
#include "Log.h"
#include "PlatformClock.h"
#include "Profile.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <format>
#include <memory>
#include <utility>
#include <vector>

namespace Neuron
{

namespace
{

constexpr UINT BACK_BUFFER_COUNT = 3;                                  // ADR-012
constexpr DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM; // sRGB values written without conversion (ADR-013)
constexpr std::array<float, 4> CLEAR_COLOR{0.0f, 0.0f, 0.0f, 1.0f};
// The bounds of the renderer's waits (ADR-092): a fence or the frame-latency object that has not signalled within
// GPU_CHECK_MILLISECONDS is checked for a removed device, and teardown waits at most TEARDOWN_WAIT_MILLISECONDS.
constexpr DWORD GPU_CHECK_MILLISECONDS = 1000;
constexpr DWORD TEARDOWN_WAIT_MILLISECONDS = 2000;

class ScopedHandle
{
public:
  ScopedHandle() noexcept = default;
  explicit ScopedHandle(HANDLE _handle) noexcept
    : m_handle(_handle)
  {
  }
  ScopedHandle(ScopedHandle&& _other) noexcept
    : m_handle(std::exchange(_other.m_handle, nullptr))
  {
  }
  ScopedHandle& operator=(ScopedHandle&& _other) noexcept
  {
    if (this != &_other)
    {
      Close();
      m_handle = std::exchange(_other.m_handle, nullptr);
    }
    return *this;
  }
  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(const ScopedHandle&) = delete;
  ~ScopedHandle()
  {
    Close();
  }

  [[nodiscard]] HANDLE Get() const noexcept
  {
    return m_handle;
  }

private:
  void Close() noexcept
  {
    if (m_handle != nullptr)
    {
      CloseHandle(m_handle);
      m_handle = nullptr;
    }
  }

  HANDLE m_handle = nullptr;
};

} // namespace

/// Every device object of the renderer, released and created again together by a rebuild (ADR-008).
struct RendererState
{
  AppWindow* window = nullptr;
  const PlatformClock* clock = nullptr;
  RenderConfig config;

  std::unique_ptr<GraphicsDevice> device;
  ComPtr<ID3D12Fence> fence;
  ScopedHandle fenceEvent;
  std::uint64_t fenceValue = 0;
  std::array<std::uint64_t, FRAMES_IN_FLIGHT> frameFenceValues{};
  std::array<ComPtr<ID3D12CommandAllocator>, FRAMES_IN_FLIGHT> allocators;
  ComPtr<ID3D12GraphicsCommandList> commands;
  ComPtr<ID3D12DescriptorHeap> renderTargetHeap;
  UINT renderTargetStride = 0;

  ComPtr<IDXGISwapChain3> swapChain;
  ScopedHandle frameLatencyWaitable;
  UINT swapChainFlags = 0;
  SizePx swapChainSize;
  std::array<ComPtr<ID3D12Resource>, BACK_BUFFER_COUNT> backBuffers;

  std::uint64_t frameNumber = 0;
  bool occluded = false;
  bool rebuilding = false;
  std::optional<Nanoseconds> nextFrameStart;
  std::vector<std::function<void()>> rebuiltListeners;
};

namespace
{

// Waits until the fence passes _value. A wait that outlasts GPU_CHECK_MILLISECONDS is a fence that may never complete:
// a removed device ends it with false, and anything else is logged and waited for again (ADR-008). _limit bounds the
// whole wait for teardown, after which it gives up with false.
bool WaitForFence(RendererState& _state, std::uint64_t _value, std::optional<DWORD> _limit = std::nullopt)
{
  if (_state.fence == nullptr || _state.fence->GetCompletedValue() >= _value)
  {
    return true;
  }
  if (FAILED(_state.fence->SetEventOnCompletion(_value, _state.fenceEvent.Get())))
  {
    return false;
  }
  DWORD waitedMilliseconds = 0;
  while (WaitForSingleObject(_state.fenceEvent.Get(), GPU_CHECK_MILLISECONDS) == WAIT_TIMEOUT)
  {
    waitedMilliseconds += GPU_CHECK_MILLISECONDS;
    if (_state.device->IsRemoved())
    {
      return false;
    }
    if (_limit.has_value() && waitedMilliseconds >= *_limit)
    {
      Log(LogCategory::Render, LogLevel::Error, "the GPU did not finish within {} ms; teardown goes on without it", waitedMilliseconds);
      return false;
    }
    Log(LogCategory::Render, LogLevel::Warning, "the GPU has not reached fence value {} after {} ms", _value, waitedMilliseconds);
  }
  return true;
}

bool WaitForGpu(RendererState& _state, std::optional<DWORD> _limit = std::nullopt)
{
  if (_state.fence == nullptr || _state.device == nullptr || _state.device->IsRemoved())
  {
    return false;
  }
  const std::uint64_t value = ++_state.fenceValue;
  if (FAILED(_state.device->Queue()->Signal(_state.fence.Get(), value)))
  {
    return false;
  }
  return WaitForFence(_state, value, _limit);
}

void AcquireBackBuffers(RendererState& _state)
{
  D3D12_CPU_DESCRIPTOR_HANDLE target = _state.renderTargetHeap->GetCPUDescriptorHandleForHeapStart();
  for (UINT index = 0; index < BACK_BUFFER_COUNT; ++index)
  {
    if (FAILED(_state.swapChain->GetBuffer(index, IID_PPV_ARGS(&_state.backBuffers[index]))))
    {
      continue;
    }
    _state.backBuffers[index]->SetName(std::format(L"Back buffer {}", index).c_str());
    _state.device->Device()->CreateRenderTargetView(_state.backBuffers[index].Get(), nullptr, target);
    target.ptr += _state.renderTargetStride;
  }
}

bool CreateSwapChain(RendererState& _state)
{
  const SizePx client = _state.window->ClientSize();
  _state.swapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  if (_state.device->TearingSupported())
  {
    _state.swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // only at creation: ResizeBuffers cannot add it
  }
  DXGI_SWAP_CHAIN_DESC1 desc{};
  desc.Width = std::max(client.width, 1u);
  desc.Height = std::max(client.height, 1u);
  desc.Format = BACK_BUFFER_FORMAT;
  desc.SampleDesc.Count = 1; // a flip-model back buffer is never multisampled (ADR-012)
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = BACK_BUFFER_COUNT;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  desc.Flags = _state.swapChainFlags;

  HWND window = _state.window->Native();
  ComPtr<IDXGISwapChain1> swapChain;
  const HRESULT created =
    _state.device->Factory()->CreateSwapChainForHwnd(_state.device->Queue(), window, &desc, nullptr, nullptr, &swapChain);
  if (FAILED(created) || FAILED(swapChain.As(&_state.swapChain)))
  {
    Log(LogCategory::Render, LogLevel::Error, "the swap chain was not created at {}x{}: {:#010x}", desc.Width, desc.Height,
        static_cast<std::uint32_t>(created));
    return false;
  }
  // Exclusive full screen does not exist (ADR-012), so DXGI never answers Alt+Enter or changes the window itself.
  _state.device->Factory()->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
  _state.swapChain->SetMaximumFrameLatency(MaximumFrameLatency(_state.config.lowLatency));
  _state.frameLatencyWaitable = ScopedHandle(_state.swapChain->GetFrameLatencyWaitableObject());
  _state.swapChainSize = {desc.Width, desc.Height};
  AcquireBackBuffers(_state);
  Log(LogCategory::Render, LogLevel::Info, "swap chain {}x{}, {} buffers, tearing {}", desc.Width, desc.Height, BACK_BUFFER_COUNT,
      _state.device->TearingSupported() ? "allowed" : "not allowed");
  return true;
}

std::expected<void, StartupFailure> CreateDeviceObjects(RendererState& _state)
{
  std::expected<GraphicsDevice, StartupFailure> device = GraphicsDevice::Create();
  if (!device.has_value())
  {
    return std::unexpected(std::move(device.error()));
  }
  _state.device = std::make_unique<GraphicsDevice>(std::move(*device));
  ID3D12Device* d3d = _state.device->Device();
  const std::string& adapter = _state.device->AdapterName();

  ScopedHandle fenceEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr));
  if (fenceEvent.Get() == nullptr || FAILED(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_state.fence))))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a fence"});
  }
  _state.fenceEvent = std::move(fenceEvent);
  _state.fenceValue = 0;
  _state.frameFenceValues = {};
  for (ComPtr<ID3D12CommandAllocator>& allocator : _state.allocators)
  {
    if (FAILED(d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))))
    {
      return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a command allocator"});
    }
  }
  if (FAILED(
        d3d->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _state.allocators[0].Get(), nullptr, IID_PPV_ARGS(&_state.commands))))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a command list"});
  }
  _state.commands->SetName(L"Frame commands");
  _state.commands->Close();

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  heapDesc.NumDescriptors = BACK_BUFFER_COUNT;
  if (FAILED(d3d->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_state.renderTargetHeap))))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a render-target descriptor heap"});
  }
  _state.renderTargetStride = d3d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  if (!CreateSwapChain(_state))
  {
    return std::unexpected(StartupFailure{StartupError::SwapChainFailed, adapter, {}});
  }
  _state.occluded = false;
  return {};
}

// Releases every device object, the swap chain before the device, after the GPU has finished what it can (ADR-008).
void ReleaseDeviceObjects(RendererState& _state, std::optional<DWORD> _limit)
{
  if (_state.device != nullptr)
  {
    WaitForGpu(_state, _limit);
  }
  for (ComPtr<ID3D12Resource>& buffer : _state.backBuffers)
  {
    buffer.Reset();
  }
  _state.frameLatencyWaitable = ScopedHandle();
  _state.swapChain.Reset();
  _state.renderTargetHeap.Reset();
  _state.commands.Reset();
  for (ComPtr<ID3D12CommandAllocator>& allocator : _state.allocators)
  {
    allocator.Reset();
  }
  _state.fence.Reset();
  _state.fenceEvent = ScopedHandle();
  _state.device.reset();
}

// ADR-008 and RND-034: the removal is logged, every device object is released and created again, presentation
// resumes and DeviceRebuilt is raised. A removal during the rebuild, or a recreation that fails, ends the client with a
// report.
void Rebuild(RendererState& _state)
{
  if (_state.rebuilding)
  {
    EnterFatalPath("the device was removed again during its rebuild");
  }
  _state.rebuilding = true;
  _state.device->LogRemoval();
  ReleaseDeviceObjects(_state, TEARDOWN_WAIT_MILLISECONDS);
  if (const std::expected<void, StartupFailure> created = CreateDeviceObjects(_state); !created.has_value())
  {
    EnterFatalPath(
      std::format("the device could not be rebuilt after its removal: {} {}", created.error().adapter, created.error().capability));
  }
  _state.rebuilding = false;
  Log(LogCategory::Render, LogLevel::Warning, "the device was rebuilt after its removal");
  for (const std::function<void()>& listener : _state.rebuiltListeners)
  {
    listener();
  }
}

// RND-030 as M0 has it: wait for the GPU, release the back buffers, resize them with the creation flags, take them again.
void ApplyResize(RendererState& _state, SizePx _client)
{
  WaitForGpu(_state);
  for (ComPtr<ID3D12Resource>& buffer : _state.backBuffers)
  {
    buffer.Reset();
  }
  const HRESULT resized =
    _state.swapChain->ResizeBuffers(BACK_BUFFER_COUNT, _client.width, _client.height, DXGI_FORMAT_UNKNOWN, _state.swapChainFlags);
  if (IsDeviceRemoval(resized))
  {
    Rebuild(_state);
    return;
  }
  if (FAILED(resized))
  {
    Log(LogCategory::Render, LogLevel::Error, "the swap chain was not resized to {}x{}: {:#010x}", _client.width, _client.height,
        static_cast<std::uint32_t>(resized));
  }
  else
  {
    _state.swapChainSize = _client;
  }
  AcquireBackBuffers(_state);
}

D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _before, D3D12_RESOURCE_STATES _after) noexcept
{
  D3D12_RESOURCE_BARRIER barrier{};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = _resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = _before;
  barrier.Transition.StateAfter = _after;
  return barrier;
}

} // namespace

std::expected<std::unique_ptr<FrameRenderer>, StartupFailure> FrameRenderer::Create(AppWindow& _window, const PlatformClock& _clock,
                                                                                    const RenderConfig& _config)
{
  auto state = std::make_unique<RendererState>();
  state->window = &_window;
  state->clock = &_clock;
  state->config = _config;
  if (std::expected<void, StartupFailure> created = CreateDeviceObjects(*state); !created.has_value())
  {
    ReleaseDeviceObjects(*state, TEARDOWN_WAIT_MILLISECONDS);
    return std::unexpected(std::move(created.error()));
  }
  return std::unique_ptr<FrameRenderer>(new FrameRenderer(std::move(state)));
}

FrameRenderer::FrameRenderer(std::unique_ptr<RendererState> _state) noexcept
  : m_state(std::move(_state))
{
}

FrameRenderer::~FrameRenderer()
{
  try
  {
    ReleaseDeviceObjects(*m_state, TEARDOWN_WAIT_MILLISECONDS);
  }
  catch (...)
  {
    std::terminate(); // a teardown that cannot log is a defect, and it ends through the fatal path
  }
}

void FrameRenderer::Reconfigure(const RenderConfig& _config)
{
  RendererState& state = *m_state;
  if (_config.lowLatency != state.config.lowLatency)
  {
    state.swapChain->SetMaximumFrameLatency(MaximumFrameLatency(_config.lowLatency));
  }
  state.config = _config;
  state.nextFrameStart.reset();
}

FrameOutcome FrameRenderer::RenderFrame()
{
  ENGINE_PROFILE_SCOPE("render.frame"); // the markers of the DBG-045 catalogue
  RendererState& state = *m_state;
  if (state.window->IsMinimized())
  {
    return FrameOutcome::Suspended;
  }
  const SizePx client = state.window->ClientSize();
  if (client.width != 0 && client.height != 0 && client != state.swapChainSize)
  {
    ApplyResize(state, client);
  }
  if (state.occluded)
  {
    // RND-032: a test present says whether the window is visible again without presenting anything.
    if (state.swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
    {
      return FrameOutcome::Suspended;
    }
    state.occluded = false;
  }

  // ADR-012: the frame-latency object before recording, then the optional frame-rate cap.
  if (WaitForSingleObjectEx(state.frameLatencyWaitable.Get(), GPU_CHECK_MILLISECONDS, TRUE) == WAIT_TIMEOUT && state.device->IsRemoved())
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  if (state.config.frameRateCap.has_value() && *state.config.frameRateCap > 0.0f)
  {
    const Nanoseconds period{static_cast<std::int64_t>(std::llround(1.0e9 / static_cast<double>(*state.config.frameRateCap)))};
    const Nanoseconds now = state.clock->Now();
    if (state.nextFrameStart.has_value() && *state.nextFrameStart > now)
    {
      state.clock->WaitUntil(*state.nextFrameStart);
    }
    state.nextFrameStart = std::max(now, state.nextFrameStart.value_or(now)) + period;
  }

  const std::size_t slot = state.frameNumber % FRAMES_IN_FLIGHT;
  if (!WaitForFence(state, state.frameFenceValues[slot]))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  ID3D12CommandAllocator* allocator = state.allocators[slot].Get();
  ID3D12GraphicsCommandList* commands = state.commands.Get();
  if (IsDeviceRemoval(allocator->Reset()) || IsDeviceRemoval(commands->Reset(allocator, nullptr)))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }

  // The stand-in frame of M0 task T4: the back buffer cleared to black. The frame graph and its passes replace it.
  const UINT backBufferIndex = state.swapChain->GetCurrentBackBufferIndex();
  ID3D12Resource* backBuffer = state.backBuffers[backBufferIndex].Get();
  D3D12_CPU_DESCRIPTOR_HANDLE target = state.renderTargetHeap->GetCPUDescriptorHandleForHeapStart();
  target.ptr += static_cast<SIZE_T>(backBufferIndex) * state.renderTargetStride;
  {
    ENGINE_PROFILE_SCOPE("render.clear");
    const D3D12_RESOURCE_BARRIER toTarget = Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commands->ResourceBarrier(1, &toTarget);
    commands->ClearRenderTargetView(target, CLEAR_COLOR.data(), 0, nullptr);
    const D3D12_RESOURCE_BARRIER toPresent = Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commands->ResourceBarrier(1, &toPresent);
  }
  if (IsDeviceRemoval(commands->Close()))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  ID3D12CommandList* const lists[] = {commands};
  state.device->Queue()->ExecuteCommandLists(1, lists);

  const PresentPolicy policy = SelectPresentPolicy(state.config.presentMode, state.device->TearingSupported(), state.window->Mode());
  HRESULT presented = S_OK;
  {
    ENGINE_PROFILE_SCOPE("render.present");
    presented = state.swapChain->Present(policy.syncInterval, policy.allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);
  }
  state.frameFenceValues[slot] = ++state.fenceValue;
  const HRESULT signalled = state.device->Queue()->Signal(state.fence.Get(), state.fenceValue);
  ++state.frameNumber;
  if (IsDeviceRemoval(presented) || IsDeviceRemoval(signalled))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  if (presented == DXGI_STATUS_OCCLUDED)
  {
    state.occluded = true;
  }
  return FrameOutcome::Presented;
}

void FrameRenderer::OnDeviceRebuilt(std::function<void()> _listener)
{
  m_state->rebuiltListeners.push_back(std::move(_listener));
}

#if ENGINE_DEVELOPMENT
void FrameRenderer::DebugRemoveDevice()
{
  m_state->device->Remove();
}
#endif

void EndStartup(const StartupFailure& _failure)
{
  switch (_failure.error)
  {
  case StartupError::NoCapableAdapter:
  case StartupError::DeviceCreationFailed:
  {
    const std::array arguments{MessageArgument{"adapter", _failure.adapter}, MessageArgument{"capability", _failure.capability}};
    FatalExit("ui.error.gpu_unsupported", arguments);
  }
  case StartupError::SwapChainFailed:
    FatalExit("ui.error.display_mode", {});
  case StartupError::PipelineCreationFailed:
  case StartupError::FrameGraphInvalid:
    break;
  }
  EnterFatalPath("the renderer's pipelines or frame graph could not be created");
}

} // namespace Neuron
