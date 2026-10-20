#pragma once

#include "GpuResourceFactory.h"

#include <cstdint>
#include <mutex>
#include <optional>
#include <variant>
#include <vector>

namespace Neuron
{

/// The factory of ADR-078 without a GPU, for the unit tests of the factory's clients. It keeps the rules of RND-009 and
/// RND-035 (the staging budget, the fence values, the deferred release and the rebuild) and records every call; a test
/// drives the frames and the GPU's progress, and starts and ends a rebuild. Fence values are frame numbers here: an
/// upload scheduled before BeginFrame(n) is submitted with that frame and completes with it.
class InMemoryGpuResourceFactory final : public GpuResourceFactory
{
public:
  struct Desc
  {
    std::uint64_t stagingBudgetBytes = STAGING_BUDGET_MIB * 1024 * 1024;
  };

  enum class Call : std::uint8_t
  {
    CreateTexture,
    CreateBuffer,
    ScheduleUpload,
    Release
  };

  struct CallRecord
  {
    Call call = Call::CreateTexture;
    GpuResourceId resource;
    std::optional<GpuResourceError> refusal;
  };

  explicit InMemoryGpuResourceFactory(Desc _desc);

  [[nodiscard]] std::expected<GpuResourceId, GpuResourceError> CreateTexture(const GpuTextureDesc& _desc) override;
  [[nodiscard]] std::expected<GpuResourceId, GpuResourceError> CreateBuffer(const GpuBufferDesc& _desc) override;
  [[nodiscard]] std::expected<std::uint64_t, GpuResourceError> ScheduleUpload(GpuResourceId _resource,
                                                                              std::span<const GpuSubresourceData> _subresources) override;
  [[nodiscard]] bool IsUploadComplete(std::uint64_t _fenceValue) const override;
  [[nodiscard]] std::uint32_t HeapIndex(GpuResourceId _resource) const override;
  void Release(GpuResourceId _resource) override;

  // The frame thread: the start of a frame submits the uploads scheduled since the last one (RND-009 item 2).
  void BeginFrame();
  // The GPU: every frame up to this one, and the uploads submitted with them, have completed; the releases that no frame
  // in flight uses any more are carried out (RND-009 item 4).
  void CompleteFramesThrough(std::uint64_t _frame);

  // RND-035: while the rebuild runs every call is refused; at its end every live resource keeps its id and heap index
  // and holds no data, and the uploads not yet completed are dropped.
  void BeginRebuild();
  void EndRebuild();

  [[nodiscard]] bool HoldsData(GpuResourceId _resource) const;
  [[nodiscard]] std::uint64_t StagingBytesInFlight() const;
  [[nodiscard]] std::vector<CallRecord> Calls() const;

private:
  struct Slot
  {
    std::uint32_t generation = 0;
    bool live = false;
    bool holdsData = false;
    std::variant<GpuTextureDesc, GpuBufferDesc> desc;
    std::uint64_t lastUploadFence = 0;
    std::optional<std::uint64_t> releasedInFrame;
  };

  struct Upload
  {
    std::uint64_t fence = 0;
    std::uint64_t bytes = 0;
    GpuResourceId resource;
  };

  std::expected<GpuResourceId, GpuResourceError> Create(Call _call, std::variant<GpuTextureDesc, GpuBufferDesc> _desc, bool _valid);
  [[nodiscard]] const Slot* Find(GpuResourceId _resource) const noexcept;
  void Record(Call _call, GpuResourceId _resource, std::optional<GpuResourceError> _refusal);

  Desc m_desc;
  mutable std::mutex m_mutex;
  std::vector<Slot> m_slots;
  std::vector<std::uint32_t> m_freeSlots;
  std::vector<Upload> m_uploads;
  std::uint64_t m_frame = 0;
  std::uint64_t m_completedFrame = 0;
  std::uint64_t m_stagingBytes = 0;
  bool m_rebuilding = false;
  std::vector<CallRecord> m_calls;
};

} // namespace Neuron
