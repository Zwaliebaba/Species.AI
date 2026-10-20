#include "pch.h"

#include "InMemoryGpuResourceFactory.h"

#include <algorithm>
#include <type_traits>
#include <utility>

namespace Neuron
{

InMemoryGpuResourceFactory::InMemoryGpuResourceFactory(Desc _desc)
  : m_desc(_desc)
{
}

std::expected<GpuResourceId, GpuResourceError> InMemoryGpuResourceFactory::CreateTexture(const GpuTextureDesc& _desc)
{
  return Create(Call::CreateTexture, _desc, IsValidDescription(_desc));
}

std::expected<GpuResourceId, GpuResourceError> InMemoryGpuResourceFactory::CreateBuffer(const GpuBufferDesc& _desc)
{
  return Create(Call::CreateBuffer, _desc, IsValidDescription(_desc));
}

std::expected<GpuResourceId, GpuResourceError>
InMemoryGpuResourceFactory::Create(Call _call, std::variant<GpuTextureDesc, GpuBufferDesc> _desc, bool _valid)
{
  const std::lock_guard lock(m_mutex);
  if (m_rebuilding || !_valid)
  {
    const GpuResourceError refusal = m_rebuilding ? GpuResourceError::DeviceRebuilding : GpuResourceError::InvalidDescription;
    Record(_call, {}, refusal);
    return std::unexpected(refusal);
  }
  std::uint32_t index = 0;
  if (!m_freeSlots.empty())
  {
    index = m_freeSlots.back();
    m_freeSlots.pop_back();
  }
  else
  {
    index = static_cast<std::uint32_t>(m_slots.size());
    m_slots.emplace_back();
  }
  Slot& slot = m_slots[index];
  ++slot.generation; // generation 0 is never live, so a default id names nothing
  slot.live = true;
  slot.holdsData = false;
  slot.desc = std::move(_desc);
  slot.lastUploadFence = 0;
  slot.releasedInFrame.reset();
  const GpuResourceId id{index, slot.generation};
  Record(_call, id, std::nullopt);
  return id;
}

std::expected<std::uint64_t, GpuResourceError> InMemoryGpuResourceFactory::ScheduleUpload(GpuResourceId _resource,
                                                                                          std::span<const GpuSubresourceData> _subresources)
{
  const std::lock_guard lock(m_mutex);
  const auto refuse = [&](GpuResourceError _error)
  {
    Record(Call::ScheduleUpload, _resource, _error);
    return std::unexpected(_error);
  };
  if (m_rebuilding)
  {
    return refuse(GpuResourceError::DeviceRebuilding);
  }
  if (Find(_resource) == nullptr)
  {
    return refuse(GpuResourceError::UnknownResource);
  }
  Slot& slot = m_slots[_resource.slot];
  std::uint64_t bytes = 0;
  for (const GpuSubresourceData& subresource : _subresources)
  {
    bytes += subresource.bytes.size();
  }
  const bool shaped = std::visit(
    [&](const auto& _desc)
    {
      if constexpr (std::is_same_v<std::decay_t<decltype(_desc)>, GpuTextureDesc>)
      {
        return _subresources.size() == _desc.levels;
      }
      else
      {
        return _subresources.size() == 1 && bytes <= _desc.sizeBytes;
      }
    },
    slot.desc);
  if (!shaped)
  {
    return refuse(GpuResourceError::InvalidDescription);
  }
  if (m_stagingBytes + bytes > m_desc.stagingBudgetBytes)
  {
    return refuse(GpuResourceError::StagingFull);
  }
  const std::uint64_t fence = m_frame + 1;
  m_stagingBytes += bytes;
  m_uploads.push_back({fence, bytes, _resource});
  slot.lastUploadFence = fence;
  Record(Call::ScheduleUpload, _resource, std::nullopt);
  return fence;
}

bool InMemoryGpuResourceFactory::IsUploadComplete(std::uint64_t _fenceValue) const
{
  const std::lock_guard lock(m_mutex);
  return _fenceValue <= m_completedFrame;
}

std::uint32_t InMemoryGpuResourceFactory::HeapIndex(GpuResourceId _resource) const
{
  const std::lock_guard lock(m_mutex);
  return Find(_resource) != nullptr ? _resource.slot : INVALID_HEAP_INDEX;
}

void InMemoryGpuResourceFactory::Release(GpuResourceId _resource)
{
  const std::lock_guard lock(m_mutex);
  // A release touches no device object, so it is taken during a rebuild too, and carried out like any other.
  if (Find(_resource) == nullptr)
  {
    Record(Call::Release, _resource, GpuResourceError::UnknownResource);
    return;
  }
  m_slots[_resource.slot].releasedInFrame = m_frame;
  Record(Call::Release, _resource, std::nullopt);
}

void InMemoryGpuResourceFactory::BeginFrame()
{
  const std::lock_guard lock(m_mutex);
  ++m_frame;
}

void InMemoryGpuResourceFactory::CompleteFramesThrough(std::uint64_t _frame)
{
  const std::lock_guard lock(m_mutex);
  m_completedFrame = std::max(m_completedFrame, std::min(_frame, m_frame));
  std::erase_if(m_uploads,
                [&](const Upload& _upload)
                {
                  if (_upload.fence > m_completedFrame)
                  {
                    return false;
                  }
                  m_stagingBytes -= _upload.bytes;
                  if (Find(_upload.resource) != nullptr)
                  {
                    m_slots[_upload.resource.slot].holdsData = true;
                  }
                  return true;
                });
  for (std::uint32_t index = 0; index < m_slots.size(); ++index)
  {
    Slot& slot = m_slots[index];
    if (slot.live && slot.releasedInFrame.has_value() && *slot.releasedInFrame <= m_completedFrame &&
        slot.lastUploadFence <= m_completedFrame)
    {
      slot.live = false;
      slot.releasedInFrame.reset();
      m_freeSlots.push_back(index);
    }
  }
}

void InMemoryGpuResourceFactory::BeginRebuild()
{
  const std::lock_guard lock(m_mutex);
  m_rebuilding = true;
}

void InMemoryGpuResourceFactory::EndRebuild()
{
  const std::lock_guard lock(m_mutex);
  m_rebuilding = false;
  m_uploads.clear();
  m_stagingBytes = 0;
  for (Slot& slot : m_slots)
  {
    slot.holdsData = false;
    slot.lastUploadFence = 0;
  }
}

bool InMemoryGpuResourceFactory::HoldsData(GpuResourceId _resource) const
{
  const std::lock_guard lock(m_mutex);
  const Slot* slot = Find(_resource);
  return slot != nullptr && slot->holdsData;
}

std::uint64_t InMemoryGpuResourceFactory::StagingBytesInFlight() const
{
  const std::lock_guard lock(m_mutex);
  return m_stagingBytes;
}

std::vector<InMemoryGpuResourceFactory::CallRecord> InMemoryGpuResourceFactory::Calls() const
{
  const std::lock_guard lock(m_mutex);
  return m_calls;
}

const InMemoryGpuResourceFactory::Slot* InMemoryGpuResourceFactory::Find(GpuResourceId _resource) const noexcept
{
  if (_resource.slot >= m_slots.size())
  {
    return nullptr;
  }
  const Slot& slot = m_slots[_resource.slot];
  return slot.live && slot.generation == _resource.generation && !slot.releasedInFrame.has_value() ? &slot : nullptr;
}

void InMemoryGpuResourceFactory::Record(Call _call, GpuResourceId _resource, std::optional<GpuResourceError> _refusal)
{
  m_calls.push_back({_call, _resource, _refusal});
}

} // namespace Neuron
