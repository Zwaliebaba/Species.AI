#pragma once

// NeuronClient's own header: it brings Direct3D 12 with it and no other project includes it (ADR-078).
#include "WindowsHeaders.h"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include "Development.h"
#include "FrameRenderer.h"

#include <expected>
#include <string>

namespace Neuron
{

template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

/// The device half of the renderer (ADR-001, ADR-002, ADR-008): the DXGI factory, the adapter that passes the
/// capability checks, the device at feature level 12_1 and its direct queue. DRED is enabled in every build before the
/// device exists; Debug builds add the debug layer, whose messages go to the log.
class GraphicsDevice
{
public:
  [[nodiscard]] static std::expected<GraphicsDevice, StartupFailure> Create();

  GraphicsDevice(GraphicsDevice&& _other) noexcept = default;
  GraphicsDevice& operator=(GraphicsDevice&& _other) noexcept = default;
  GraphicsDevice(const GraphicsDevice&) = delete;
  GraphicsDevice& operator=(const GraphicsDevice&) = delete;
  ~GraphicsDevice();

  [[nodiscard]] IDXGIFactory6* Factory() const noexcept
  {
    return m_factory.Get();
  }
  [[nodiscard]] ID3D12Device* Device() const noexcept
  {
    return m_device.Get();
  }
  [[nodiscard]] ID3D12CommandQueue* Queue() const noexcept
  {
    return m_queue.Get();
  }
  // DXGI_FEATURE_PRESENT_ALLOW_TEARING, which the swap chain takes at creation (ADR-012).
  [[nodiscard]] bool TearingSupported() const noexcept
  {
    return m_tearingSupported;
  }
  [[nodiscard]] const std::string& AdapterName() const noexcept
  {
    return m_adapterName;
  }

  [[nodiscard]] bool IsRemoved() const noexcept;
  // ADR-008 step 1: the removal reason, the DRED breadcrumbs of every unfinished command list and the page fault, to
  // the log. The crash-report attachment comes with crash reporting (PLT-119).
  void LogRemoval() const;

#if ENGINE_DEVELOPMENT
  void Remove() const;
#endif

private:
  GraphicsDevice() = default;

  ComPtr<IDXGIFactory6> m_factory;
  ComPtr<IDXGIAdapter1> m_adapter;
  ComPtr<ID3D12Device> m_device;
  ComPtr<ID3D12CommandQueue> m_queue;
  ComPtr<ID3D12InfoQueue1> m_infoQueue;
  DWORD m_callbackCookie = 0;
  bool m_tearingSupported = false;
  std::string m_adapterName;
};

// DXGI_ERROR_DEVICE_REMOVED and DXGI_ERROR_DEVICE_RESET, however they arrive (ADR-008).
[[nodiscard]] bool IsDeviceRemoval(HRESULT _result) noexcept;

} // namespace Neuron
