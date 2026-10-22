#pragma once

#include "DeviceGpuResourceFactory.h"
#include "PipelineCatalog.h"
#include "ShaderLayouts.h"
#include "UploadRing.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Neuron
{

/// The UI's drawing (ADR-031, ADR-145): the UI face's coverage page and the UI's images on the GPU, and the UI's batches
/// over the back buffer, one instanced draw each, in their order (UI-177, UI-179), in pass 19 and with the cursor in
/// pass 20. The UI owns no other GPU resource (UI-003). Every call is made from the frame thread.
class UiRenderer
{
public:
  explicit UiRenderer(DeviceGpuResourceFactory& _factory) noexcept;
  ~UiRenderer();

  UiRenderer(const UiRenderer&) = delete;
  UiRenderer& operator=(const UiRenderer&) = delete;

  // Before the factory's frame: uploads _face's page and _images when they are not the ones held.
  void Show(const std::shared_ptr<const FontAsset>& _face, const std::shared_ptr<const UiImages>& _images);
  // After a device rebuild: the same page and images again, into the resources the factory kept (RND-035).
  void UploadAgain();

  // _batches in their order over the target of _size, whose render target, viewport and scissor are set. A batch whose
  // texture is not on the GPU yet, or whose source and blend have no catalogue entry, is not drawn; the scissor is the
  // whole target again afterwards.
  void Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
              std::span<const UiBatch> _batches);

private:
  struct Upload
  {
    GpuResourceId resource;
    std::vector<GpuSubresourceData> levels; // into the page's or the images' bytes, which the renderer keeps
  };

  void Release();
  void Pump();
  [[nodiscard]] bool Ready() const;

  DeviceGpuResourceFactory& m_factory;
  std::shared_ptr<const FontAsset> m_face;
  std::shared_ptr<const UiImages> m_images;
  GpuResourceId m_page;
  std::vector<GpuResourceId> m_imageResources; // by image index; GpuResourceId{} for one that could not be made
  std::vector<Upload> m_uploads;               // the page's and every image's upload, for a rebuild
  std::vector<Upload> m_pending;
  std::uint64_t m_lastFence = 0;
  std::vector<TextGlyphInstance> m_instances; // the batch's scratch
};

} // namespace Neuron
