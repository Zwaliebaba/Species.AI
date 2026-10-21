#include "pch.h"

#include "Check.h"
#include "DeviceGpuResourceFactory.h"
#include "GpuDescriptions.h"
#include "Log.h"
#include "TextEncoding.h"

#include <algorithm>
#include <cstring>
#include <iterator>
#include <utility>

namespace Neuron
{

namespace
{

// A wait for the copy queue that a teardown bounds (ADR-092).
constexpr DWORD COPY_WAIT_MILLISECONDS = 2000;

// ADR-013: the working colour space is the authored one, so a texture is read as it was authored, with no conversion.
DXGI_FORMAT ViewFormatOf(DXGI_FORMAT _format) noexcept
{
  switch (_format)
  {
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case DXGI_FORMAT_BC1_UNORM_SRGB:
    return DXGI_FORMAT_BC1_UNORM;
  case DXGI_FORMAT_BC2_UNORM_SRGB:
    return DXGI_FORMAT_BC2_UNORM;
  case DXGI_FORMAT_BC3_UNORM_SRGB:
    return DXGI_FORMAT_BC3_UNORM;
  case DXGI_FORMAT_BC7_UNORM_SRGB:
    return DXGI_FORMAT_BC7_UNORM;
  default:
    return _format;
  }
}

void WaitForValue(ID3D12Fence* _fence, std::uint64_t _value)
{
  if (_fence == nullptr || _fence->GetCompletedValue() >= _value)
  {
    return;
  }
  HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (event == nullptr)
  {
    return;
  }
  if (SUCCEEDED(_fence->SetEventOnCompletion(_value, event)) && WaitForSingleObject(event, COPY_WAIT_MILLISECONDS) == WAIT_TIMEOUT)
  {
    Log(LogCategory::Render, LogLevel::Error, "the copy queue did not reach fence value {} within {} ms", _value, COPY_WAIT_MILLISECONDS);
  }
  CloseHandle(event);
}

} // namespace

DeviceGpuResourceFactory::DeviceGpuResourceFactory(Desc _desc)
  : m_desc(_desc)
{
}

DeviceGpuResourceFactory::~DeviceGpuResourceFactory()
{
  Detach();
}

std::expected<GpuResourceId, GpuResourceError> DeviceGpuResourceFactory::CreateTexture(const GpuTextureDesc& _desc)
{
  return Create(_desc, IsValidDescription(_desc));
}

std::expected<GpuResourceId, GpuResourceError> DeviceGpuResourceFactory::CreateBuffer(const GpuBufferDesc& _desc)
{
  return Create(_desc, IsValidDescription(_desc));
}

std::expected<GpuResourceId, GpuResourceError> DeviceGpuResourceFactory::Create(std::variant<GpuTextureDesc, GpuBufferDesc> _desc,
                                                                                bool _valid)
{
  const std::scoped_lock lock(m_mutex);
  if (m_device == nullptr)
  {
    return std::unexpected(GpuResourceError::DeviceRebuilding);
  }
  if (!_valid)
  {
    return std::unexpected(GpuResourceError::InvalidDescription);
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
  slot.desc = std::move(_desc);
  slot.heapIndex = INVALID_HEAP_INDEX;
  slot.releasedInFrame.reset();
  slot.lastUploadFence = 0;
  if (!CreateOnDevice(slot))
  {
    slot.live = false;
    m_freeSlots.push_back(index);
    return std::unexpected(GpuResourceError::OutOfVideoMemory);
  }
  return GpuResourceId{index, slot.generation};
}

bool DeviceGpuResourceFactory::CreateOnDevice(Slot& _slot)
{
  if (_slot.heapIndex == INVALID_HEAP_INDEX)
  {
    const std::optional<std::uint32_t> heapIndex = m_heaps->AllocatePersistent();
    if (!heapIndex)
    {
      Log(LogCategory::Render, LogLevel::Error, "the shader-visible heap's persistent region is full");
      return false;
    }
    _slot.heapIndex = *heapIndex;
  }
  const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_DEFAULT);
  D3D12_SHADER_RESOURCE_VIEW_DESC view{};
  view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  std::string name;
  D3D12_RESOURCE_DESC resource{};
  if (const auto* texture = std::get_if<GpuTextureDesc>(&_slot.desc))
  {
    resource =
      Texture2DDescription(TypelessFormatOf(texture->format), texture->width, texture->height, static_cast<std::uint16_t>(texture->levels));
    view.Format = ViewFormatOf(texture->format);
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Texture2D.MipLevels = texture->levels;
    name = texture->debugName;
  }
  else
  {
    const auto& buffer = std::get<GpuBufferDesc>(_slot.desc);
    resource =
      BufferDescription(buffer.sizeBytes, buffer.gpuWritable ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE);
    view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    if (buffer.strideBytes != 0)
    {
      view.Format = DXGI_FORMAT_UNKNOWN;
      view.Buffer.NumElements = static_cast<UINT>(buffer.sizeBytes / buffer.strideBytes);
      view.Buffer.StructureByteStride = buffer.strideBytes;
    }
    else
    {
      view.Format = DXGI_FORMAT_R32_TYPELESS;
      view.Buffer.NumElements = static_cast<UINT>(buffer.sizeBytes / 4);
      view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    }
    name = buffer.debugName;
  }
  // The copy queue needs its resources in the common state; the direct queue promotes them to a read state.
  if (FAILED(m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &resource, D3D12_RESOURCE_STATE_COMMON, nullptr,
                                               IID_PPV_ARGS(&_slot.resource))))
  {
    Log(LogCategory::Render, LogLevel::Error, "the resource {} could not be created", name);
    return false;
  }
  _slot.resource->SetName(ToUtf16(name).c_str());
  if (view.ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D || view.Buffer.NumElements != 0)
  {
    m_device->CreateShaderResourceView(_slot.resource.Get(), &view, m_heaps->Cpu(_slot.heapIndex));
  }
  return true;
}

std::expected<std::uint64_t, GpuResourceError> DeviceGpuResourceFactory::ScheduleUpload(GpuResourceId _resource,
                                                                                        std::span<const GpuSubresourceData> _subresources)
{
  const std::scoped_lock lock(m_mutex);
  if (m_device == nullptr || !m_commandsOpen)
  {
    return std::unexpected(GpuResourceError::DeviceRebuilding);
  }
  Slot* slot = Find(_resource);
  if (slot == nullptr)
  {
    return std::unexpected(GpuResourceError::UnknownResource);
  }
  const D3D12_RESOURCE_DESC desc = slot->resource->GetDesc();
  const auto* texture = std::get_if<GpuTextureDesc>(&slot->desc);
  const std::uint32_t subresources = texture != nullptr ? texture->levels : 1;
  if (_subresources.size() != subresources)
  {
    return std::unexpected(GpuResourceError::InvalidDescription);
  }
  std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(subresources);
  std::vector<UINT> rows(subresources);
  std::vector<UINT64> rowBytes(subresources);
  UINT64 total = 0;
  if (texture != nullptr)
  {
    m_device->GetCopyableFootprints(&desc, 0, subresources, 0, layouts.data(), rows.data(), rowBytes.data(), &total);
    for (std::uint32_t level = 0; level < subresources; ++level)
    {
      const GpuSubresourceData& data = _subresources[level];
      if (data.rowPitchBytes < rowBytes[level] || data.bytes.size() < static_cast<std::uint64_t>(data.rowPitchBytes) * rows[level])
      {
        return std::unexpected(GpuResourceError::InvalidDescription);
      }
    }
  }
  else
  {
    total = _subresources[0].bytes.size();
    if (total == 0 || total > desc.Width)
    {
      return std::unexpected(GpuResourceError::InvalidDescription);
    }
  }
  if (m_stagingBytes + total > m_desc.stagingBudgetBytes)
  {
    return std::unexpected(GpuResourceError::StagingFull);
  }

  // RND-009 item 2: the bytes are copied into staging memory now, and the copy queue moves them into place.
  const D3D12_HEAP_PROPERTIES uploadHeap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
  const D3D12_RESOURCE_DESC stagingDesc = BufferDescription(total);
  ComPtr<ID3D12Resource> staging;
  if (FAILED(m_device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &stagingDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                               IID_PPV_ARGS(&staging))))
  {
    return std::unexpected(GpuResourceError::StagingFull);
  }
  void* mapped = nullptr;
  const D3D12_RANGE nothingRead{0, 0};
  if (FAILED(staging->Map(0, &nothingRead, &mapped)))
  {
    return std::unexpected(GpuResourceError::StagingFull);
  }
  auto* bytes = static_cast<std::byte*>(mapped);
  if (texture != nullptr)
  {
    for (std::uint32_t level = 0; level < subresources; ++level)
    {
      const GpuSubresourceData& data = _subresources[level];
      for (UINT row = 0; row < rows[level]; ++row)
      {
        std::memcpy(bytes + layouts[level].Offset + static_cast<UINT64>(row) * layouts[level].Footprint.RowPitch,
                    data.bytes.data() + static_cast<std::size_t>(row) * data.rowPitchBytes, static_cast<std::size_t>(rowBytes[level]));
      }
      D3D12_TEXTURE_COPY_LOCATION destination{};
      destination.pResource = slot->resource.Get();
      destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      destination.SubresourceIndex = level;
      D3D12_TEXTURE_COPY_LOCATION source{};
      source.pResource = staging.Get();
      source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      source.PlacedFootprint = layouts[level];
      m_copyCommands->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
    }
  }
  else
  {
    std::memcpy(bytes, _subresources[0].bytes.data(), static_cast<std::size_t>(total));
    m_copyCommands->CopyBufferRegion(slot->resource.Get(), 0, staging.Get(), 0, total);
  }
  staging->Unmap(0, nullptr);
  const std::uint64_t fence = m_nextCopyFence;
  m_staging.push_back({fence, total, std::move(staging)});
  m_stagingBytes += total;
  m_commandsRecorded = true;
  slot->lastUploadFence = fence;
  return fence;
}

bool DeviceGpuResourceFactory::IsUploadComplete(std::uint64_t _fenceValue) const
{
  const std::scoped_lock lock(m_mutex);
  return m_copyFence != nullptr && m_copyFence->GetCompletedValue() >= _fenceValue;
}

std::uint32_t DeviceGpuResourceFactory::HeapIndex(GpuResourceId _resource) const
{
  const std::scoped_lock lock(m_mutex);
  const Slot* slot = Find(_resource);
  return slot != nullptr ? slot->heapIndex : INVALID_HEAP_INDEX;
}

void DeviceGpuResourceFactory::Release(GpuResourceId _resource)
{
  const std::scoped_lock lock(m_mutex);
  if (Slot* slot = Find(_resource); slot != nullptr)
  {
    slot->releasedInFrame = m_frame;
  }
}

ID3D12Resource* DeviceGpuResourceFactory::Resource(GpuResourceId _resource) const
{
  const std::scoped_lock lock(m_mutex);
  const Slot* slot = Find(_resource);
  return slot != nullptr ? slot->resource.Get() : nullptr;
}

bool DeviceGpuResourceFactory::Attach(ID3D12Device* _device, ShaderVisibleHeaps* _heaps)
{
  const std::scoped_lock lock(m_mutex);
  m_device = _device;
  m_heaps = _heaps;
  D3D12_COMMAND_QUEUE_DESC queue{};
  queue.Type = D3D12_COMMAND_LIST_TYPE_COPY;
  // The fence starts where the last device's ended, so a value handed out before a rebuild never names a later copy.
  if (FAILED(m_device->CreateCommandQueue(&queue, IID_PPV_ARGS(&m_copyQueue))) ||
      FAILED(m_device->CreateFence(m_nextCopyFence - 1, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_copyFence))))
  {
    m_device = nullptr;
    return false;
  }
  m_copyQueue->SetName(L"Copy queue");
  m_batches.clear();
  m_batches.emplace_back();
  if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_batches[0].allocator))) ||
      FAILED(
        m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, m_batches[0].allocator.Get(), nullptr, IID_PPV_ARGS(&m_copyCommands))))
  {
    m_device = nullptr;
    return false;
  }
  m_copyCommands->SetName(L"Copy commands");
  m_openBatch = 0;
  m_commandsOpen = true;
  m_commandsRecorded = false;
  // RND-035: every live resource again, under its id and heap index, holding nothing until its owner uploads it.
  std::vector<std::uint32_t> kept;
  for (const Slot& slot : m_slots)
  {
    if (slot.live && slot.heapIndex != INVALID_HEAP_INDEX)
    {
      kept.push_back(slot.heapIndex);
    }
  }
  m_heaps->ReservePersistent(kept);
  for (Slot& slot : m_slots)
  {
    if (slot.live && !CreateOnDevice(slot))
    {
      Log(LogCategory::Render, LogLevel::Error, "a resource could not be created again after the rebuild");
    }
  }
  return true;
}

void DeviceGpuResourceFactory::Detach()
{
  const std::scoped_lock lock(m_mutex);
  if (m_copyQueue != nullptr && m_copyFence != nullptr)
  {
    WaitForValue(m_copyFence.Get(), m_nextCopyFence - 1);
  }
  for (Slot& slot : m_slots)
  {
    slot.resource.Reset();
  }
  m_staging.clear();
  m_stagingBytes = 0;
  m_copyCommands.Reset();
  m_batches.clear();
  m_copyFence.Reset();
  m_copyQueue.Reset();
  m_commandsOpen = false;
  m_commandsRecorded = false;
  m_device = nullptr;
  m_heaps = nullptr;
}

void DeviceGpuResourceFactory::BeginFrame(std::uint64_t _frame, std::uint64_t _completedFrame)
{
  const std::scoped_lock lock(m_mutex);
  m_frame = _frame;
  if (m_device == nullptr)
  {
    return;
  }
  if (m_commandsRecorded && SUCCEEDED(m_copyCommands->Close()))
  {
    ID3D12CommandList* const lists[] = {m_copyCommands.Get()};
    m_copyQueue->ExecuteCommandLists(1, lists);
    m_copyQueue->Signal(m_copyFence.Get(), m_nextCopyFence);
    m_batches[m_openBatch].fence = m_nextCopyFence;
    ++m_nextCopyFence;
    m_commandsOpen = OpenCommands();
    m_commandsRecorded = false;
  }
  ReleaseCompletedStaging();
  const std::uint64_t copied = m_copyFence->GetCompletedValue();
  // RND-009 item 4: a release is carried out once no frame in flight reads it and no copy writes it.
  for (std::uint32_t index = 0; index < m_slots.size(); ++index)
  {
    Slot& slot = m_slots[index];
    if (slot.live && slot.releasedInFrame && *slot.releasedInFrame <= _completedFrame && slot.lastUploadFence <= copied)
    {
      slot.resource.Reset();
      m_heaps->FreePersistent(slot.heapIndex, _completedFrame);
      slot.heapIndex = INVALID_HEAP_INDEX;
      slot.live = false;
      slot.releasedInFrame.reset();
      m_freeSlots.push_back(index);
    }
  }
}

bool DeviceGpuResourceFactory::OpenCommands()
{
  // An allocator whose copies have completed, or a new one.
  const std::uint64_t copied = m_copyFence->GetCompletedValue();
  auto free = std::ranges::find_if(m_batches, [copied](const Batch& _batch) { return _batch.fence <= copied; });
  if (free == m_batches.end())
  {
    Batch batch;
    if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&batch.allocator))))
    {
      return false;
    }
    m_batches.push_back(std::move(batch));
    free = std::prev(m_batches.end());
  }
  else if (FAILED(free->allocator->Reset()))
  {
    return false;
  }
  m_openBatch = static_cast<std::size_t>(free - m_batches.begin());
  return SUCCEEDED(m_copyCommands->Reset(free->allocator.Get(), nullptr));
}

void DeviceGpuResourceFactory::ReleaseCompletedStaging()
{
  const std::uint64_t copied = m_copyFence->GetCompletedValue();
  for (const Staging& staging : m_staging)
  {
    if (staging.fence <= copied)
    {
      m_stagingBytes -= staging.bytes;
    }
  }
  std::erase_if(m_staging, [copied](const Staging& _staging) { return _staging.fence <= copied; });
}

DeviceGpuResourceFactory::Slot* DeviceGpuResourceFactory::Find(GpuResourceId _resource) noexcept
{
  if (_resource.slot >= m_slots.size())
  {
    return nullptr;
  }
  Slot& slot = m_slots[_resource.slot];
  return slot.live && slot.generation == _resource.generation && !slot.releasedInFrame ? &slot : nullptr;
}

const DeviceGpuResourceFactory::Slot* DeviceGpuResourceFactory::Find(GpuResourceId _resource) const noexcept
{
  if (_resource.slot >= m_slots.size())
  {
    return nullptr;
  }
  const Slot& slot = m_slots[_resource.slot];
  return slot.live && slot.generation == _resource.generation && !slot.releasedInFrame ? &slot : nullptr;
}

} // namespace Neuron
