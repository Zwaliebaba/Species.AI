#pragma once

#include "DescriptorHeaps.h"
#include "GpuResourceFactory.h"
#include "GraphicsDevice.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <variant>
#include <vector>

namespace Neuron
{

/// The GPU resource factory on the device (ADR-078, RND-009, RND-035): committed resources in the default heap, filled
/// through staging buffers that the copy queue copies from, and a view of each in the persistent region of the
/// shader-visible heap, whose index is the resource's for its life. Every public call of the interface is safe from any
/// thread and never waits for the frame thread. The frame thread submits the copies scheduled since the last frame at
/// the start of each frame, and carries out the releases that no frame in flight reads any more. A device rebuild
/// keeps every live resource's id and heap index; its contents are the owner's to upload again after DeviceRebuilt.
class DeviceGpuResourceFactory final : public GpuResourceFactory
{
public:
  struct Desc
  {
    std::uint64_t stagingBudgetBytes = STAGING_BUDGET_MIB * 1024 * 1024;
  };

  explicit DeviceGpuResourceFactory(Desc _desc);
  ~DeviceGpuResourceFactory() override;

  DeviceGpuResourceFactory(const DeviceGpuResourceFactory&) = delete;
  DeviceGpuResourceFactory& operator=(const DeviceGpuResourceFactory&) = delete;

  [[nodiscard]] std::expected<GpuResourceId, GpuResourceError> CreateTexture(const GpuTextureDesc& _desc) override;
  [[nodiscard]] std::expected<GpuResourceId, GpuResourceError> CreateBuffer(const GpuBufferDesc& _desc) override;
  [[nodiscard]] std::expected<std::uint64_t, GpuResourceError> ScheduleUpload(GpuResourceId _resource,
                                                                              std::span<const GpuSubresourceData> _subresources) override;
  [[nodiscard]] bool IsUploadComplete(std::uint64_t _fenceValue) const override;
  [[nodiscard]] std::uint32_t HeapIndex(GpuResourceId _resource) const override;
  void Release(GpuResourceId _resource) override;

  // The renderer's own: the resource behind an id, or null.
  [[nodiscard]] ID3D12Resource* Resource(GpuResourceId _resource) const;

  // The frame thread: attaches the device and the heaps, which a rebuild replaces; false when the copy queue, its fence
  // or its first command list cannot be created. Every live resource is created on the device again.
  [[nodiscard]] bool Attach(ID3D12Device* _device, ShaderVisibleHeaps* _heaps);
  // The frame thread, before the device objects are released: every call is refused until Attach.
  void Detach();
  // The start of frame _frame: submits the copies scheduled since the last frame, and carries out the releases of every
  // frame up to _completedFrame, which the GPU has finished.
  void BeginFrame(std::uint64_t _frame, std::uint64_t _completedFrame);

private:
  struct Slot
  {
    std::uint32_t generation = 0;
    bool live = false;
    std::variant<GpuTextureDesc, GpuBufferDesc> desc;
    ComPtr<ID3D12Resource> resource;
    std::uint32_t heapIndex = INVALID_HEAP_INDEX;
    std::optional<std::uint64_t> releasedInFrame;
    std::uint64_t lastUploadFence = 0; // a release waits for its last copy
  };

  struct Staging
  {
    std::uint64_t fence = 0;
    std::uint64_t bytes = 0;
    ComPtr<ID3D12Resource> buffer;
  };

  struct Batch
  {
    ComPtr<ID3D12CommandAllocator> allocator;
    std::uint64_t fence = 0; // the copy fence value after which the allocator may be reset
  };

  [[nodiscard]] std::expected<GpuResourceId, GpuResourceError> Create(std::variant<GpuTextureDesc, GpuBufferDesc> _desc, bool _valid);
  [[nodiscard]] Slot* Find(GpuResourceId _resource) noexcept;
  [[nodiscard]] const Slot* Find(GpuResourceId _resource) const noexcept;
  [[nodiscard]] bool CreateOnDevice(Slot& _slot);
  [[nodiscard]] bool OpenCommands();
  void ReleaseCompletedStaging();

  Desc m_desc;
  mutable std::mutex m_mutex;
  ID3D12Device* m_device = nullptr;
  ShaderVisibleHeaps* m_heaps = nullptr;
  std::vector<Slot> m_slots;
  std::vector<std::uint32_t> m_freeSlots;

  ComPtr<ID3D12CommandQueue> m_copyQueue;
  ComPtr<ID3D12Fence> m_copyFence;
  ComPtr<ID3D12GraphicsCommandList> m_copyCommands;
  std::vector<Batch> m_batches;
  std::size_t m_openBatch = 0;
  bool m_commandsOpen = false;
  bool m_commandsRecorded = false;
  std::uint64_t m_nextCopyFence = 1;
  std::vector<Staging> m_staging;
  std::uint64_t m_stagingBytes = 0;
  std::uint64_t m_frame = 0;
};

} // namespace Neuron
