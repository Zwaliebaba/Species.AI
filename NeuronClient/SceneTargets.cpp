#include "pch.h"

#include "GpuDescriptions.h"
#include "Log.h"
#include "SceneTargets.h"

#include <algorithm>

namespace Neuron
{

namespace
{

bool SupportsSamples(ID3D12Device* _device, DXGI_FORMAT _format, std::uint32_t _samples)
{
  D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels{};
  levels.Format = _format;
  levels.SampleCount = _samples;
  levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
  return SUCCEEDED(_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &levels, sizeof(levels))) &&
         levels.NumQualityLevels > 0;
}

} // namespace

std::uint32_t SupportedSceneSamples(ID3D12Device* _device, std::uint32_t _preferred)
{
  for (const std::uint32_t samples : {_preferred, 2u})
  {
    if (samples > 1 && samples <= _preferred && SupportsSamples(_device, SCENE_COLOR_FORMAT, samples) &&
        SupportsSamples(_device, SCENE_DEPTH_FORMAT, samples))
    {
      return samples;
    }
  }
  return 1;
}

std::optional<SceneTargets> SceneTargets::Create(ID3D12Device* _device, SizePx _size, std::uint32_t _samples,
                                                 const std::array<float, 4>& _clearColor, ShaderVisibleHeaps& _heaps,
                                                 std::uint32_t _readSlot)
{
  SceneTargets targets;
  targets.m_size = {std::max(_size.width, 1u), std::max(_size.height, 1u)};
  targets.m_samples = std::max(_samples, 1u);
  targets.m_readSlot = _readSlot;
  targets.m_clearColor = _clearColor;
  targets.m_renderTargets = CpuDescriptorHeap::Create(_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1);
  targets.m_depthTargets = CpuDescriptorHeap::Create(_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);
  if (!targets.m_renderTargets || !targets.m_depthTargets)
  {
    return std::nullopt;
  }
  targets.m_colorView = targets.m_renderTargets->Handle(0);
  targets.m_depthView = targets.m_depthTargets->Handle(0);
  const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
  const bool multisampled = targets.m_samples > 1;

  // The frame graph's initial accesses (ADR-113): scene_colour as the last frame left it, resolved or read.
  D3D12_CLEAR_VALUE colorClear{};
  colorClear.Format = SCENE_COLOR_FORMAT;
  std::ranges::copy(_clearColor, colorClear.Color);
  const D3D12_RESOURCE_DESC color = Texture2DDescription(SCENE_COLOR_FORMAT, targets.m_size.width, targets.m_size.height, 1,
                                                         targets.m_samples, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
  if (FAILED(_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &color,
                                              multisampled ? D3D12_RESOURCE_STATE_RESOLVE_SOURCE : D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
                                              &colorClear, IID_PPV_ARGS(&targets.m_color))))
  {
    Log(LogCategory::Render, LogLevel::Error, "scene_colour was not created at {}x{}, {} samples", targets.m_size.width,
        targets.m_size.height, targets.m_samples);
    return std::nullopt;
  }
  targets.m_color->SetName(L"scene_colour");

  D3D12_CLEAR_VALUE depthClear{};
  depthClear.Format = SCENE_DEPTH_FORMAT;
  depthClear.DepthStencil.Depth = SCENE_DEPTH_CLEAR;
  const D3D12_RESOURCE_DESC depth =
    Texture2DDescription(SCENE_DEPTH_FORMAT, targets.m_size.width, targets.m_size.height, 1, targets.m_samples,
                         D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
  if (FAILED(_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &depth, D3D12_RESOURCE_STATE_DEPTH_WRITE, &depthClear,
                                              IID_PPV_ARGS(&targets.m_depth))))
  {
    Log(LogCategory::Render, LogLevel::Error, "scene_depth was not created");
    return std::nullopt;
  }
  targets.m_depth->SetName(L"scene_depth");

  ID3D12Resource* read = targets.m_color.Get();
  if (multisampled)
  {
    const D3D12_RESOURCE_DESC resolved = Texture2DDescription(SCENE_COLOR_FORMAT, targets.m_size.width, targets.m_size.height, 1);
    if (FAILED(_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &resolved, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, nullptr,
                                                IID_PPV_ARGS(&targets.m_resolved))))
    {
      Log(LogCategory::Render, LogLevel::Error, "scene_resolved was not created");
      return std::nullopt;
    }
    targets.m_resolved->SetName(L"scene_resolved");
    read = targets.m_resolved.Get();
  }

  _device->CreateRenderTargetView(targets.m_color.Get(), nullptr, targets.m_colorView);
  D3D12_DEPTH_STENCIL_VIEW_DESC depthView{};
  depthView.Format = SCENE_DEPTH_FORMAT;
  depthView.ViewDimension = multisampled ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
  _device->CreateDepthStencilView(targets.m_depth.Get(), &depthView, targets.m_depthView);
  D3D12_SHADER_RESOURCE_VIEW_DESC readView{};
  readView.Format = SCENE_COLOR_FORMAT;
  readView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  readView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  readView.Texture2D.MipLevels = 1;
  _device->CreateShaderResourceView(read, &readView, _heaps.Cpu(_readSlot));
  Log(LogCategory::Render, LogLevel::Info, "scene targets {}x{}, {} samples", targets.m_size.width, targets.m_size.height,
      targets.m_samples);
  return targets;
}

} // namespace Neuron
