#include "pch.h"

#include "Check.h"
#include "GpuDescriptions.h"
#include "Log.h"
#include "UploadRing.h"

#include <utility>

namespace Neuron
{

namespace
{

std::uint64_t AlignUp(std::uint64_t _value, std::uint64_t _alignment) noexcept
{
  return (_value + _alignment - 1) / _alignment * _alignment;
}

// An upload buffer of _bytes, mapped for the whole of its life (ADR-008).
ComPtr<ID3D12Resource> CreateMappedBuffer(ID3D12Device* _device, std::uint64_t _bytes, std::byte*& _mapped)
{
  const D3D12_HEAP_PROPERTIES heap = HeapProperties(D3D12_HEAP_TYPE_UPLOAD);
  const D3D12_RESOURCE_DESC desc = BufferDescription(_bytes);
  ComPtr<ID3D12Resource> buffer;
  if (FAILED(_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                              IID_PPV_ARGS(&buffer))))
  {
    return nullptr;
  }
  void* mapped = nullptr;
  const D3D12_RANGE nothingRead{0, 0};
  if (FAILED(buffer->Map(0, &nothingRead, &mapped)))
  {
    return nullptr;
  }
  _mapped = static_cast<std::byte*>(mapped);
  return buffer;
}

} // namespace

std::optional<UploadRing> UploadRing::Create(ID3D12Device* _device, std::uint64_t _segmentBytes)
{
  UploadRing ring;
  ring.m_device = _device;
  ring.m_segmentBytes = AlignUp(_segmentBytes, UPLOAD_CONSTANT_ALIGNMENT);
  ring.m_buffer = CreateMappedBuffer(_device, ring.m_segmentBytes * FRAMES_IN_FLIGHT, ring.m_mapped);
  if (ring.m_buffer == nullptr)
  {
    return std::nullopt;
  }
  ring.m_buffer->SetName(L"Upload ring");
  ring.m_gpuStart = ring.m_buffer->GetGPUVirtualAddress();
  return ring;
}

void UploadRing::BeginFrame(std::uint32_t _slot)
{
  ENGINE_ASSERT(_slot < FRAMES_IN_FLIGHT);
  m_slot = _slot;
  m_head = 0;
  m_overflow[_slot].clear();
}

UploadAllocation UploadRing::Allocate(std::uint64_t _bytes, std::uint64_t _alignment)
{
  const std::uint64_t start = AlignUp(m_head, _alignment);
  if (start + _bytes <= m_segmentBytes)
  {
    m_head = start + _bytes;
    const std::uint64_t offset = m_segmentBytes * m_slot + start;
    return {m_mapped + offset, m_gpuStart + offset, m_buffer.Get(), offset};
  }
  // ADR-008: a one-off buffer for this frame, logged; the ring grows at the next reconfiguration.
  if (!m_overflowed)
  {
    Log(LogCategory::Render, LogLevel::Warning, "the upload ring's segment of {} bytes overflowed; a one-off buffer of {} bytes is used",
        m_segmentBytes, _bytes);
  }
  m_overflowed = true;
  std::byte* mapped = nullptr;
  ComPtr<ID3D12Resource> buffer = CreateMappedBuffer(m_device, AlignUp(_bytes, UPLOAD_CONSTANT_ALIGNMENT), mapped);
  ENGINE_CHECK(buffer != nullptr, "an overflow buffer of the upload ring could not be created");
  if (buffer == nullptr)
  {
    return {};
  }
  UploadAllocation allocation{mapped, buffer->GetGPUVirtualAddress(), buffer.Get(), 0};
  m_overflow[m_slot].push_back(std::move(buffer));
  return allocation;
}

} // namespace Neuron
