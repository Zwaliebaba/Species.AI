#include "pch.h"

#include "GpuDescriptions.h"

namespace Neuron
{

D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE _type) noexcept
{
  return D3D12_HEAP_PROPERTIES{.Type = _type,
                               .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
                               .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
                               .CreationNodeMask = 1,
                               .VisibleNodeMask = 1};
}

D3D12_RESOURCE_DESC BufferDescription(std::uint64_t _sizeBytes, D3D12_RESOURCE_FLAGS _flags) noexcept
{
  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Alignment = 0;
  desc.Width = _sizeBytes;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count = 1;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = _flags;
  return desc;
}

D3D12_RESOURCE_DESC Texture2DDescription(DXGI_FORMAT _format, std::uint32_t _width, std::uint32_t _height, std::uint16_t _levels,
                                         std::uint32_t _sampleCount, D3D12_RESOURCE_FLAGS _flags) noexcept
{
  D3D12_RESOURCE_DESC desc{};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  desc.Alignment = 0;
  desc.Width = _width;
  desc.Height = _height;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = _levels;
  desc.Format = _format;
  desc.SampleDesc.Count = _sampleCount;
  desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  desc.Flags = _flags;
  return desc;
}

D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _before, D3D12_RESOURCE_STATES _after) noexcept
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

DXGI_FORMAT TypelessFormatOf(DXGI_FORMAT _format) noexcept
{
  switch (_format)
  {
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
    return DXGI_FORMAT_R8G8B8A8_TYPELESS;
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
  case DXGI_FORMAT_B8G8R8A8_UNORM:
    return DXGI_FORMAT_B8G8R8A8_TYPELESS;
  case DXGI_FORMAT_BC1_UNORM_SRGB:
  case DXGI_FORMAT_BC1_UNORM:
    return DXGI_FORMAT_BC1_TYPELESS;
  case DXGI_FORMAT_BC2_UNORM_SRGB:
  case DXGI_FORMAT_BC2_UNORM:
    return DXGI_FORMAT_BC2_TYPELESS;
  case DXGI_FORMAT_BC3_UNORM_SRGB:
  case DXGI_FORMAT_BC3_UNORM:
    return DXGI_FORMAT_BC3_TYPELESS;
  case DXGI_FORMAT_BC7_UNORM_SRGB:
  case DXGI_FORMAT_BC7_UNORM:
    return DXGI_FORMAT_BC7_TYPELESS;
  default:
    return _format;
  }
}

bool IsBlockCompressed(DXGI_FORMAT _format) noexcept
{
  return (_format >= DXGI_FORMAT_BC1_TYPELESS && _format <= DXGI_FORMAT_BC5_SNORM) ||
         (_format >= DXGI_FORMAT_BC6H_TYPELESS && _format <= DXGI_FORMAT_BC7_UNORM_SRGB);
}

D3D12_CPU_DESCRIPTOR_HANDLE OffsetHandle(D3D12_CPU_DESCRIPTOR_HANDLE _start, std::uint32_t _index, std::uint32_t _stride) noexcept
{
  _start.ptr += static_cast<SIZE_T>(_index) * _stride;
  return _start;
}

D3D12_GPU_DESCRIPTOR_HANDLE OffsetHandle(D3D12_GPU_DESCRIPTOR_HANDLE _start, std::uint32_t _index, std::uint32_t _stride) noexcept
{
  _start.ptr += static_cast<UINT64>(_index) * _stride;
  return _start;
}

} // namespace Neuron
