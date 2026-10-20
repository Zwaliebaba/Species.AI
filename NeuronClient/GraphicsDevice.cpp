#include "pch.h"

#include "AdapterChoice.h"
#include "GraphicsDevice.h"
#include "Log.h"
#include "TextEncoding.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "version.lib")

namespace Neuron
{

namespace
{

constexpr SIZE_T BYTES_PER_MIB = SIZE_T{1024} * 1024;

std::string Name(const wchar_t* _name)
{
  return _name != nullptr ? ToUtf8(_name) : std::string("(unnamed)");
}

// ADR-008: auto-breadcrumbs, breadcrumb contexts and page-fault reporting in every build, set before the device exists.
void EnableDred()
{
  ComPtr<ID3D12DeviceRemovedExtendedDataSettings1> settings;
  if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&settings))))
  {
    settings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    settings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    settings->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    return;
  }
  Log(LogCategory::Render, LogLevel::Warning, "DRED is not available: a device removal will have no breadcrumbs");
}

#if ENGINE_DEVELOPMENT
// ADR-001: the debug layer in Debug builds, from D3D12SDKLayers.dll beside the runtime (BLD-042). GPU-based validation
// stays off until a development setting can turn it on (ADR-092).
bool EnableDebugLayer()
{
  ComPtr<ID3D12Debug> debug;
  if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
  {
    Log(LogCategory::Render, LogLevel::Warning, "the Direct3D 12 debug layer is not available");
    return false;
  }
  debug->EnableDebugLayer();
  Log(LogCategory::Render, LogLevel::Info, "the Direct3D 12 debug layer is on");
  return true;
}

void __stdcall OnDebugMessage(D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY _severity, D3D12_MESSAGE_ID _id, LPCSTR _description,
                              void*) noexcept
{
  try
  {
    LogLevel level = LogLevel::Debug;
    switch (_severity)
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    case D3D12_MESSAGE_SEVERITY_ERROR:
      level = LogLevel::Error;
      break;
    case D3D12_MESSAGE_SEVERITY_WARNING:
      level = LogLevel::Warning;
      break;
    case D3D12_MESSAGE_SEVERITY_INFO:
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
      break;
    }
    Log(LogCategory::Render, level, "D3D12 message {}: {}", static_cast<int>(_id), _description != nullptr ? _description : "");
  }
  catch (...)
  {
    return; // a debug message is never worth a failure of the call that caused it
  }
}
#endif

void Probe(ID3D12Device* _device, AdapterCapabilities& _capabilities)
{
  _capabilities.shaderModel =
    HighestShaderModel(D3D_HIGHEST_SHADER_MODEL,
                       [_device](std::uint32_t _model) -> std::optional<std::uint32_t>
                       {
                         D3D12_FEATURE_DATA_SHADER_MODEL query{static_cast<D3D_SHADER_MODEL>(_model)};
                         if (FAILED(_device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &query, sizeof(query))))
                         {
                           return std::nullopt;
                         }
                         return static_cast<std::uint32_t>(query.HighestShaderModel);
                       });
  // Filled by the query; its enumerations have no zero value, so it is not value-initialized.
  D3D12_FEATURE_DATA_D3D12_OPTIONS options;
  if (SUCCEEDED(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options))))
  {
    _capabilities.resourceBindingTier = static_cast<std::uint32_t>(options.ResourceBindingTier);
  }
  D3D12_FEATURE_DATA_D3D12_OPTIONS1 options1{};
  if (SUCCEEDED(_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &options1, sizeof(options1))))
  {
    _capabilities.waveOps = options1.WaveOps != FALSE;
  }
}

std::string DriverVersion(IDXGIAdapter1* _adapter)
{
  LARGE_INTEGER version{};
  if (FAILED(_adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version)))
  {
    return "unknown";
  }
  return std::format("{}.{}.{}.{}", HIWORD(version.HighPart), LOWORD(version.HighPart), HIWORD(version.LowPart), LOWORD(version.LowPart));
}

// ADR-001: the runtime that loaded, which is the OS copy when it is newer than the one beside the executable.
void LogRuntime()
{
  HMODULE core = GetModuleHandleW(L"D3D12Core.dll");
  if (core == nullptr)
  {
    Log(LogCategory::Render, LogLevel::Warning, "D3D12Core.dll is not loaded");
    return;
  }
  std::array<wchar_t, MAX_PATH> path{};
  if (GetModuleFileNameW(core, path.data(), static_cast<DWORD>(path.size())) == 0)
  {
    return;
  }
  std::string version = "unknown";
  DWORD ignored = 0;
  const DWORD size = GetFileVersionInfoSizeW(path.data(), &ignored);
  std::vector<std::byte> data(size);
  VS_FIXEDFILEINFO* info = nullptr;
  UINT length = 0;
  if (size != 0 && GetFileVersionInfoW(path.data(), 0, size, data.data()) != FALSE &&
      VerQueryValueW(data.data(), L"\\", reinterpret_cast<void**>(&info), &length) != FALSE && info != nullptr)
  {
    version = std::format("{}.{}.{}.{}", HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS), HIWORD(info->dwFileVersionLS),
                          LOWORD(info->dwFileVersionLS));
  }
  Log(LogCategory::Render, LogLevel::Info, "Direct3D 12 runtime {} from {}", version, ToUtf8(path.data()));
}

} // namespace

bool IsDeviceRemoval(HRESULT _result) noexcept
{
  return _result == DXGI_ERROR_DEVICE_REMOVED || _result == DXGI_ERROR_DEVICE_RESET;
}

std::expected<GraphicsDevice, StartupFailure> GraphicsDevice::Create()
{
  GraphicsDevice result;
  EnableDred();
  UINT factoryFlags = 0;
#if ENGINE_DEVELOPMENT
  if (EnableDebugLayer())
  {
    factoryFlags = DXGI_CREATE_FACTORY_DEBUG;
  }
#endif
  HRESULT created = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&result.m_factory));
  if (FAILED(created) && factoryFlags != 0)
  {
    // The DXGI debug layer comes with the Graphics Tools; without them the factory is created without it.
    Log(LogCategory::Render, LogLevel::Warning, "the DXGI debug layer is not available: are the Graphics Tools installed?");
    created = CreateDXGIFactory2(0, IID_PPV_ARGS(&result.m_factory));
  }
  if (FAILED(created))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, {}, "a DXGI factory"});
  }
  ComPtr<IDXGIFactory5> factory5;
  BOOL tearing = FALSE;
  if (SUCCEEDED(result.m_factory.As(&factory5)) &&
      SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &tearing, sizeof(tearing))))
  {
    result.m_tearingSupported = tearing != FALSE;
  }

  // ADR-002: the adapters in the order of the high-performance preference; the first that passes every check is kept.
  std::vector<AdapterCapabilities> probed;
  for (UINT index = 0;; ++index)
  {
    ComPtr<IDXGIAdapter1> adapter;
    if (FAILED(result.m_factory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))))
    {
      break;
    }
    DXGI_ADAPTER_DESC1 desc{};
    adapter->GetDesc1(&desc);
    AdapterCapabilities capabilities;
    capabilities.description = ToUtf8(desc.Description);
    capabilities.software = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
    ComPtr<ID3D12Device> device;
    if (!capabilities.software && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device))))
    {
      capabilities.featureLevelMet = true;
      Probe(device.Get(), capabilities);
    }
    const std::string missing = MissingCapability(capabilities);
    Log(LogCategory::Render, LogLevel::Info, "adapter {}: {}, {} MiB of video memory, driver {}, Shader Model {}, binding tier {}: {}",
        index, capabilities.description, desc.DedicatedVideoMemory / BYTES_PER_MIB, DriverVersion(adapter.Get()),
        ShaderModelName(capabilities.shaderModel), capabilities.resourceBindingTier, missing.empty() ? "passes" : "lacks " + missing);
    probed.push_back(capabilities);
    if (missing.empty())
    {
      result.m_adapter = adapter;
      result.m_device = device;
      break;
    }
  }
  const AdapterChoice choice = ChooseAdapter(probed);
  if (!choice.adapter.has_value())
  {
    return std::unexpected(StartupFailure{StartupError::NoCapableAdapter, choice.rejectedAdapter, choice.missingCapability});
  }
  result.m_adapterName = probed[*choice.adapter].description;

  D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12{};
  const bool enhancedBarriers =
    SUCCEEDED(result.m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12))) &&
    options12.EnhancedBarriersSupported != FALSE;
  Log(LogCategory::Render, LogLevel::Info, "device on {}: enhanced barriers {}, tearing {}", result.m_adapterName,
      enhancedBarriers ? "supported" : "not supported", result.m_tearingSupported ? "supported" : "not supported");

  D3D12_COMMAND_QUEUE_DESC queueDesc{};
  queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
  if (FAILED(result.m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&result.m_queue))))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, result.m_adapterName, "a direct command queue"});
  }
  result.m_queue->SetName(L"Frame queue");

#if ENGINE_DEVELOPMENT
  if (SUCCEEDED(result.m_device.As(&result.m_infoQueue)))
  {
    if (IsDebuggerPresent() != FALSE)
    {
      result.m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      result.m_infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }
    if (FAILED(result.m_infoQueue->RegisterMessageCallback(&OnDebugMessage, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr,
                                                           &result.m_callbackCookie)))
    {
      result.m_infoQueue.Reset();
    }
  }
  if (result.m_infoQueue == nullptr)
  {
    Log(LogCategory::Render, LogLevel::Warning, "debug-layer messages go to the debugger output only");
  }
#endif
  LogRuntime();
  return result;
}

GraphicsDevice::~GraphicsDevice()
{
  if (m_infoQueue != nullptr)
  {
    m_infoQueue->UnregisterMessageCallback(m_callbackCookie);
  }
}

bool GraphicsDevice::IsRemoved() const noexcept
{
  return m_device != nullptr && FAILED(m_device->GetDeviceRemovedReason());
}

void GraphicsDevice::LogRemoval() const
{
  const HRESULT reason = m_device->GetDeviceRemovedReason();
  Log(LogCategory::Render, LogLevel::Error, "the device was removed: reason {:#010x}", static_cast<std::uint32_t>(reason));
  ComPtr<ID3D12DeviceRemovedExtendedData1> dred;
  if (FAILED(m_device.As(&dred)))
  {
    Log(LogCategory::Render, LogLevel::Error, "DRED: no data");
    return;
  }
  D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
  if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbs)))
  {
    for (const D3D12_AUTO_BREADCRUMB_NODE1* node = breadcrumbs.pHeadAutoBreadcrumbNode; node != nullptr; node = node->pNext)
    {
      const UINT completed = node->pLastBreadcrumbValue != nullptr ? *node->pLastBreadcrumbValue : 0;
      if (completed >= node->BreadcrumbCount)
      {
        continue;
      }
      Log(LogCategory::Render, LogLevel::Error, "DRED: queue {}, command list {}: {} of {} operations completed, next operation {}",
          Name(node->pCommandQueueDebugNameW), Name(node->pCommandListDebugNameW), completed, node->BreadcrumbCount,
          static_cast<int>(node->pCommandHistory[completed]));
    }
  }
  D3D12_DRED_PAGE_FAULT_OUTPUT1 fault{};
  if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&fault)) && fault.PageFaultVA != 0)
  {
    Log(LogCategory::Render, LogLevel::Error, "DRED: page fault at GPU address {:#018x}", fault.PageFaultVA);
    for (const D3D12_DRED_ALLOCATION_NODE1* node = fault.pHeadExistingAllocationNode; node != nullptr; node = node->pNext)
    {
      Log(LogCategory::Render, LogLevel::Error, "DRED: allocation near the fault: {}", Name(node->ObjectNameW));
    }
    for (const D3D12_DRED_ALLOCATION_NODE1* node = fault.pHeadRecentFreedAllocationNode; node != nullptr; node = node->pNext)
    {
      Log(LogCategory::Render, LogLevel::Error, "DRED: recently freed allocation near the fault: {}", Name(node->ObjectNameW));
    }
  }
}

#if ENGINE_DEVELOPMENT
void GraphicsDevice::Remove() const
{
  ComPtr<ID3D12Device5> device5;
  if (SUCCEEDED(m_device.As(&device5)))
  {
    Log(LogCategory::Render, LogLevel::Warning, "removing the device on request");
    device5->RemoveDevice();
  }
}
#endif

} // namespace Neuron
