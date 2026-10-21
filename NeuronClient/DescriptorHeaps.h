#pragma once

#include "GraphicsDevice.h"
#include "PresentPolicy.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace Neuron
{

// ADR-018: the closed sampler set, at fixed indices of the sampler heap, which the shared layout header repeats.
enum class SamplerSlot : std::uint8_t
{
  LinearClamp,
  LinearWrap,
  PointClamp,
  PointMagnifyClamp
};
inline constexpr std::uint32_t SAMPLER_COUNT = 4;

/// The shader-visible heaps of ADR-006: one CBV/SRV/UAV heap and one sampler heap, bound once per command list.
/// The resource heap has a persistent region, whose slots are allocated when a resource is made and reused only once
/// every frame in flight that may read them has completed, and a transient region used as a ring per frame in flight.
/// Index 0 holds a default view, so an unset index never reads an undefined descriptor. The persistent region is safe
/// from any thread; the transient region and Bind belong to the frame thread.
class ShaderVisibleHeaps
{
public:
  [[nodiscard]] static std::unique_ptr<ShaderVisibleHeaps> Create(ID3D12Device* _device, std::uint32_t _persistent,
                                                                  std::uint32_t _transientPerFrame);

  [[nodiscard]] std::optional<std::uint32_t> AllocatePersistent();
  // _index is free once frame _frame has completed on the GPU.
  void FreePersistent(std::uint32_t _index, std::uint64_t _frame);
  // Every slot freed by a frame up to _completedFrame may be used again.
  void Reclaim(std::uint64_t _completedFrame);
  // After a rebuild: the persistent slots that live resources keep (RND-035), which are allocated to nobody else.
  void ReservePersistent(std::span<const std::uint32_t> _indices);

  void BeginFrame(std::uint32_t _slot);
  // _count consecutive transient slots of this frame, or none when the frame's share is used up.
  [[nodiscard]] std::optional<std::uint32_t> AllocateTransient(std::uint32_t _count);

  [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE Cpu(std::uint32_t _index) const noexcept;
  void Bind(ID3D12GraphicsCommandList* _commands) const;

  [[nodiscard]] ID3D12DescriptorHeap* Resources() const noexcept
  {
    return m_resources.Get();
  }

private:
  ShaderVisibleHeaps() = default;

  ComPtr<ID3D12DescriptorHeap> m_resources;
  ComPtr<ID3D12DescriptorHeap> m_samplers;
  D3D12_CPU_DESCRIPTOR_HANDLE m_cpuStart{};
  std::uint32_t m_stride = 0;
  std::uint32_t m_persistentCount = 0;
  std::uint32_t m_transientPerFrame = 0;

  mutable std::mutex m_mutex; // the persistent region's
  std::vector<std::uint32_t> m_free;
  std::vector<std::pair<std::uint64_t, std::uint32_t>> m_pending; // freed by a frame still in flight
  std::uint32_t m_nextPersistent = 1;                             // index 0 is the default view

  std::uint32_t m_slot = 0;
  std::uint32_t m_transientHead = 0;
};

/// A CPU-only descriptor heap of render-target or depth-stencil views, allocated linearly by the frame renderer.
class CpuDescriptorHeap
{
public:
  [[nodiscard]] static std::optional<CpuDescriptorHeap> Create(ID3D12Device* _device, D3D12_DESCRIPTOR_HEAP_TYPE _type,
                                                               std::uint32_t _count);

  [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE Handle(std::uint32_t _index) const noexcept;
  [[nodiscard]] std::uint32_t Count() const noexcept
  {
    return m_count;
  }

private:
  CpuDescriptorHeap() = default;

  ComPtr<ID3D12DescriptorHeap> m_heap;
  D3D12_CPU_DESCRIPTOR_HANDLE m_start{};
  std::uint32_t m_stride = 0;
  std::uint32_t m_count = 0;
};

} // namespace Neuron
