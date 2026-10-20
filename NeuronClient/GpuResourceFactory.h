#pragma once

#include <dxgiformat.h>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>

namespace Neuron
{

inline constexpr std::uint64_t STAGING_BUDGET_MIB = 96; // rnd.resources.staging_budget_mib
inline constexpr std::uint32_t INVALID_HEAP_INDEX = 0xFFFF'FFFF;

// A slot of the factory's table and its generation; unchanged by a device rebuild (RND-035).
struct GpuResourceId
{
  std::uint32_t slot = 0;
  std::uint32_t generation = 0;

  auto operator<=>(const GpuResourceId& _other) const = default;
};

enum class GpuResourceError : std::uint8_t
{
  InvalidDescription,
  OutOfVideoMemory,
  StagingFull,      // the upload is refused at once; the caller repeats it on a later frame
  DeviceRebuilding, // refused while the device is rebuilt; the caller repeats it after DeviceRebuilt
  UnknownResource
};

// From a cooked DDS header (ADR-017) or a run-time texture (AST-062).
struct GpuTextureDesc
{
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::uint32_t levels = 1;
  DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
  std::string debugName; // the asset id or the owner's name, for the allocation record (ADR-008) and PIX
};

struct GpuBufferDesc
{
  std::uint64_t sizeBytes = 0;
  std::uint32_t strideBytes = 0; // structured buffers; 0 for vertex, index and raw data
  bool gpuWritable = false;      // written by compute work in the frame graph (RND-063)
  std::string debugName;
};

// One level of a texture, or the whole of a buffer.
struct GpuSubresourceData
{
  std::span<const std::byte> bytes;
  std::uint32_t rowPitchBytes = 0; // textures only
};

/// The one way a component outside the renderer creates, fills, releases and rebuilds a GPU resource (RND-009,
/// ADR-078). Every call is safe from any thread and never waits for the frame thread; the copy queue belongs to the
/// renderer. Clients include this header and <dxgiformat.h>, never <d3d12.h>.
class GpuResourceFactory
{
public:
  virtual ~GpuResourceFactory() = default;

  [[nodiscard]] virtual std::expected<GpuResourceId, GpuResourceError> CreateTexture(const GpuTextureDesc& _desc) = 0;
  [[nodiscard]] virtual std::expected<GpuResourceId, GpuResourceError> CreateBuffer(const GpuBufferDesc& _desc) = 0;

  // Copies the bytes into staging memory now and queues the copy; returns the copy-queue fence value after which the
  // resource holds them (RND-009 item 2).
  [[nodiscard]] virtual std::expected<std::uint64_t, GpuResourceError>
  ScheduleUpload(GpuResourceId _resource, std::span<const GpuSubresourceData> _subresources) = 0;
  [[nodiscard]] virtual bool IsUploadComplete(std::uint64_t _fenceValue) const = 0;

  [[nodiscard]] virtual std::uint32_t HeapIndex(GpuResourceId _resource) const = 0; // ADR-006; stable across a rebuild
  virtual void Release(GpuResourceId _resource) = 0;                                // destroyed after the frames in flight that may use it
};

// RND-009 item 1: a texture has a size, a format and at most a full mip chain; a buffer has a size, and a structured
// buffer's size is a whole number of elements.
[[nodiscard]] bool IsValidDescription(const GpuTextureDesc& _desc) noexcept;
[[nodiscard]] bool IsValidDescription(const GpuBufferDesc& _desc) noexcept;

} // namespace Neuron
