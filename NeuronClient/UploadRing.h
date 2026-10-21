#pragma once

#include "GraphicsDevice.h"
#include "PresentPolicy.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <vector>

namespace Neuron
{

// ADR-008: the alignment of a constant-buffer view in the ring.
inline constexpr std::uint64_t UPLOAD_CONSTANT_ALIGNMENT = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;

/// Space in the upload ring for this frame: written by the CPU, read by the GPU through its address.
struct UploadAllocation
{
  std::byte* cpu = nullptr;
  D3D12_GPU_VIRTUAL_ADDRESS gpu = 0;
  ID3D12Resource* resource = nullptr;
  std::uint64_t offset = 0;
};

/// The per-frame upload ring of ADR-008: one persistently mapped upload buffer, a segment per frame in flight, bumped
/// linearly. A segment is reused only once the fence of the frame that last used it has completed, which the frame
/// renderer has waited for before BeginFrame. A frame that overflows its segment takes a one-off buffer, which is logged
/// and kept until the segment is reused. Used on the frame thread.
class UploadRing
{
public:
  [[nodiscard]] static std::optional<UploadRing> Create(ID3D12Device* _device, std::uint64_t _segmentBytes);

  // Starts frame slot _slot: its segment and its overflow buffers are free again.
  void BeginFrame(std::uint32_t _slot);
  [[nodiscard]] UploadAllocation Allocate(std::uint64_t _bytes, std::uint64_t _alignment = UPLOAD_CONSTANT_ALIGNMENT);

  // _value copied into the ring, at a constant-buffer alignment: the address a root CBV takes.
  template <class T> [[nodiscard]] D3D12_GPU_VIRTUAL_ADDRESS Write(const T& _value)
  {
    const UploadAllocation allocation = Allocate(sizeof(T));
    std::memcpy(allocation.cpu, &_value, sizeof(T));
    return allocation.gpu;
  }

  [[nodiscard]] bool Overflowed() const noexcept
  {
    return m_overflowed;
  }
  [[nodiscard]] std::uint64_t SegmentBytes() const noexcept
  {
    return m_segmentBytes;
  }

private:
  UploadRing() = default;

  ID3D12Device* m_device = nullptr;
  ComPtr<ID3D12Resource> m_buffer;
  std::byte* m_mapped = nullptr;
  D3D12_GPU_VIRTUAL_ADDRESS m_gpuStart = 0;
  std::uint64_t m_segmentBytes = 0;
  std::uint32_t m_slot = 0;
  std::uint64_t m_head = 0;
  std::array<std::vector<ComPtr<ID3D12Resource>>, FRAMES_IN_FLIGHT> m_overflow;
  bool m_overflowed = false;
};

} // namespace Neuron
