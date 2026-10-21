#pragma once

#include "DeviceGpuResourceFactory.h"
#include "PipelineCatalog.h"
#include "UploadRing.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Neuron
{

/// The game cursor's drawing in pass 20 (RND-365, RND-366; M1 task T14, ADR-119): the cursors' masks and shadows on
/// the GPU, each a one-level texture of its DDS file, and the frame's cursor quad over the final image, its shadow first
/// with the darkening blend and then its art added in its colour. Frame thread only.
class CursorRenderer
{
public:
  explicit CursorRenderer(DeviceGpuResourceFactory& _factory) noexcept;
  ~CursorRenderer();

  CursorRenderer(const CursorRenderer&) = delete;
  CursorRenderer& operator=(const CursorRenderer&) = delete;

  // Before the factory's frame: uploads the images when they are not the ones held.
  void Show(const std::shared_ptr<const CursorImages>& _images);
  // After a device rebuild: the same images again, into the resources the factory kept (RND-035).
  void UploadAgain();

  // Pass 20's cursor over the target of _size, whose render target, viewport and scissor are set.
  void Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
              const CursorQuad& _cursor);

private:
  struct Upload
  {
    GpuResourceId resource;
    std::vector<GpuSubresourceData> levels; // into the images' bytes, which m_images keeps
  };

  void Release();
  void Pump();
  [[nodiscard]] GpuResourceId Create(const std::vector<std::byte>& _file, const char* _name);

  DeviceGpuResourceFactory& m_factory;
  std::shared_ptr<const CursorImages> m_images;
  std::vector<GpuResourceId> m_art;
  std::vector<GpuResourceId> m_shadows; // GpuResourceId{} for a cursor without a shadow
  std::vector<Upload> m_uploads;        // every upload, for a rebuild
  std::vector<Upload> m_pending;
  std::uint64_t m_lastFence = 0;
  bool m_failed = false;
};

} // namespace Neuron
