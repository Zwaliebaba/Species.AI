#pragma once

// NeuronClient's own header: the hand-written descriptions that d3dx12.h would otherwise provide (AGENTS.md R14). It
// brings Direct3D 12 with it, and no other project includes it (ADR-078).
#include "WindowsHeaders.h"

#include <d3d12.h>

#include <cstdint>

namespace Neuron
{

[[nodiscard]] D3D12_HEAP_PROPERTIES HeapProperties(D3D12_HEAP_TYPE _type) noexcept;
[[nodiscard]] D3D12_RESOURCE_DESC BufferDescription(std::uint64_t _sizeBytes,
                                                    D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE) noexcept;
[[nodiscard]] D3D12_RESOURCE_DESC Texture2DDescription(DXGI_FORMAT _format, std::uint32_t _width, std::uint32_t _height,
                                                       std::uint16_t _levels, std::uint32_t _sampleCount = 1,
                                                       D3D12_RESOURCE_FLAGS _flags = D3D12_RESOURCE_FLAG_NONE) noexcept;
[[nodiscard]] D3D12_RESOURCE_BARRIER TransitionBarrier(ID3D12Resource* _resource, D3D12_RESOURCE_STATES _before,
                                                       D3D12_RESOURCE_STATES _after) noexcept;

// ADR-013: an sRGB texture is created typeless and viewed through its UNORM or sRGB format as its use asks; every
// other format is created as it is.
[[nodiscard]] DXGI_FORMAT TypelessFormatOf(DXGI_FORMAT _format) noexcept;
[[nodiscard]] bool IsBlockCompressed(DXGI_FORMAT _format) noexcept;

// A descriptor handle _index descriptors after _start.
[[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE OffsetHandle(D3D12_CPU_DESCRIPTOR_HANDLE _start, std::uint32_t _index,
                                                       std::uint32_t _stride) noexcept;
[[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE OffsetHandle(D3D12_GPU_DESCRIPTOR_HANDLE _start, std::uint32_t _index,
                                                       std::uint32_t _stride) noexcept;

} // namespace Neuron
