#include "pch.h"

#include "Check.h"
#include "DescriptorHeaps.h"
#include "GpuDescriptions.h"

#include <algorithm>
#include <array>

namespace Neuron
{

namespace
{

D3D12_SAMPLER_DESC SamplerOf(SamplerSlot _slot) noexcept
{
  const bool wrap = _slot == SamplerSlot::LinearWrap;
  const D3D12_TEXTURE_ADDRESS_MODE address = wrap ? D3D12_TEXTURE_ADDRESS_MODE_WRAP : D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  // Every enumeration is named in the initialiser: D3D12_TEXTURE_ADDRESS_MODE has no zero value.
  D3D12_SAMPLER_DESC desc{.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
                          .AddressU = address,
                          .AddressV = address,
                          .AddressW = address,
                          .MipLODBias = 0.0f,
                          .MaxAnisotropy = 1,
                          .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER};
  switch (_slot)
  {
  case SamplerSlot::LinearClamp:
  case SamplerSlot::LinearWrap:
    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    break;
  case SamplerSlot::PointClamp:
    desc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    break;
  case SamplerSlot::PointMagnifyClamp:
    desc.Filter = D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR; // ADR-018: hard-edged blocks, smooth minification
    break;
  }
  desc.MinLOD = 0.0f;
  desc.MaxLOD = D3D12_FLOAT32_MAX;
  return desc;
}

} // namespace

std::unique_ptr<ShaderVisibleHeaps> ShaderVisibleHeaps::Create(ID3D12Device* _device, std::uint32_t _persistent,
                                                               std::uint32_t _transientPerFrame)
{
  auto heaps = std::unique_ptr<ShaderVisibleHeaps>(new ShaderVisibleHeaps());
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors = _persistent + _transientPerFrame * FRAMES_IN_FLIGHT;
  desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if (FAILED(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heaps->m_resources))))
  {
    return nullptr;
  }
  heaps->m_resources->SetName(L"Shader-visible resources");
  D3D12_DESCRIPTOR_HEAP_DESC samplerDesc{};
  samplerDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
  samplerDesc.NumDescriptors = SAMPLER_COUNT;
  samplerDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  if (FAILED(_device->CreateDescriptorHeap(&samplerDesc, IID_PPV_ARGS(&heaps->m_samplers))))
  {
    return nullptr;
  }
  heaps->m_samplers->SetName(L"Shader-visible samplers");
  const D3D12_CPU_DESCRIPTOR_HANDLE samplerStart = heaps->m_samplers->GetCPUDescriptorHandleForHeapStart();
  const std::uint32_t samplerStride = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
  for (std::uint32_t slot = 0; slot < SAMPLER_COUNT; ++slot)
  {
    const D3D12_SAMPLER_DESC sampler = SamplerOf(static_cast<SamplerSlot>(slot));
    _device->CreateSampler(&sampler, OffsetHandle(samplerStart, slot, samplerStride));
  }
  heaps->m_cpuStart = heaps->m_resources->GetCPUDescriptorHandleForHeapStart();
  heaps->m_stride = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  // ADR-006: index 0 is a valid default, a null texture view that reads zero, so an unset index reads nothing undefined.
  D3D12_SHADER_RESOURCE_VIEW_DESC nullView{};
  nullView.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  nullView.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  nullView.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  nullView.Texture2D.MipLevels = 1;
  _device->CreateShaderResourceView(nullptr, &nullView, heaps->m_cpuStart);
  heaps->m_persistentCount = _persistent;
  heaps->m_transientPerFrame = _transientPerFrame;
  return heaps;
}

std::optional<std::uint32_t> ShaderVisibleHeaps::AllocatePersistent()
{
  const std::scoped_lock lock(m_mutex);
  if (!m_free.empty())
  {
    const std::uint32_t index = m_free.back();
    m_free.pop_back();
    return index;
  }
  if (m_nextPersistent >= m_persistentCount)
  {
    return std::nullopt;
  }
  return m_nextPersistent++;
}

void ShaderVisibleHeaps::FreePersistent(std::uint32_t _index, std::uint64_t _frame)
{
  if (_index == 0 || _index >= m_persistentCount)
  {
    return;
  }
  const std::scoped_lock lock(m_mutex);
  m_pending.emplace_back(_frame, _index);
}

void ShaderVisibleHeaps::Reclaim(std::uint64_t _completedFrame)
{
  const std::scoped_lock lock(m_mutex);
  for (const auto& [frame, index] : m_pending)
  {
    if (frame <= _completedFrame)
    {
      m_free.push_back(index);
    }
  }
  std::erase_if(m_pending, [_completedFrame](const auto& _entry) { return _entry.first <= _completedFrame; });
}

void ShaderVisibleHeaps::ReservePersistent(std::span<const std::uint32_t> _indices)
{
  const std::scoped_lock lock(m_mutex);
  std::vector<bool> used(m_persistentCount, false);
  std::uint32_t next = 1;
  for (const std::uint32_t index : _indices)
  {
    if (index > 0 && index < m_persistentCount)
    {
      used[index] = true;
      next = std::max(next, index + 1);
    }
  }
  m_nextPersistent = next;
  m_free.clear();
  m_pending.clear();
  for (std::uint32_t index = next - 1; index > 0; --index)
  {
    if (!used[index])
    {
      m_free.push_back(index);
    }
  }
}

void ShaderVisibleHeaps::BeginFrame(std::uint32_t _slot)
{
  ENGINE_ASSERT(_slot < FRAMES_IN_FLIGHT);
  m_slot = _slot;
  m_transientHead = 0;
}

std::optional<std::uint32_t> ShaderVisibleHeaps::AllocateTransient(std::uint32_t _count)
{
  if (m_transientHead + _count > m_transientPerFrame)
  {
    return std::nullopt;
  }
  const std::uint32_t index = m_persistentCount + m_slot * m_transientPerFrame + m_transientHead;
  m_transientHead += _count;
  return index;
}

D3D12_CPU_DESCRIPTOR_HANDLE ShaderVisibleHeaps::Cpu(std::uint32_t _index) const noexcept
{
  return OffsetHandle(m_cpuStart, _index, m_stride);
}

void ShaderVisibleHeaps::Bind(ID3D12GraphicsCommandList* _commands) const
{
  // ADR-006: the heaps are set before the root signature, which indexes them directly.
  const std::array<ID3D12DescriptorHeap*, 2> heaps{m_resources.Get(), m_samplers.Get()};
  _commands->SetDescriptorHeaps(static_cast<UINT>(heaps.size()), heaps.data());
}

std::optional<CpuDescriptorHeap> CpuDescriptorHeap::Create(ID3D12Device* _device, D3D12_DESCRIPTOR_HEAP_TYPE _type, std::uint32_t _count)
{
  CpuDescriptorHeap heap;
  D3D12_DESCRIPTOR_HEAP_DESC desc{};
  desc.Type = _type;
  desc.NumDescriptors = std::max(_count, 1u);
  if (FAILED(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap.m_heap))))
  {
    return std::nullopt;
  }
  heap.m_start = heap.m_heap->GetCPUDescriptorHandleForHeapStart();
  heap.m_stride = _device->GetDescriptorHandleIncrementSize(_type);
  heap.m_count = desc.NumDescriptors;
  return heap;
}

D3D12_CPU_DESCRIPTOR_HANDLE CpuDescriptorHeap::Handle(std::uint32_t _index) const noexcept
{
  return OffsetHandle(m_start, _index, m_stride);
}

} // namespace Neuron
