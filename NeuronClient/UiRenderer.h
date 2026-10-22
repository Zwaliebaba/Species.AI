#pragma once

#include "DeviceGpuResourceFactory.h"
#include "GlyphAtlas.h"
#include "PipelineCatalog.h"
#include "ShaderLayouts.h"
#include "UploadRing.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Neuron
{

/// The UI's drawing (ADR-031, ADR-145, ADR-147): the glyph atlas's pages and the UI's images on the GPU, and the UI's
/// batches over the back buffer, one instanced draw each, in their order (UI-177, UI-179), in pass 19, with the cursor in
/// pass 20 and with the development overlay's text in pass 29. The UI owns no other GPU resource (UI-003). Every call is
/// made from the frame thread.
class UiRenderer
{
public:
  explicit UiRenderer(DeviceGpuResourceFactory& _factory) noexcept;
  ~UiRenderer();

  UiRenderer(const UiRenderer&) = delete;
  UiRenderer& operator=(const UiRenderer&) = delete;

  // Before the factory's frame: uploads _images when they are not the ones held, and each page of _atlas whose revision
  // is not the one on the GPU into a texture of its own, which the page draws from once its upload completes (ADR-147).
  void Show(const std::shared_ptr<const GlyphAtlas>& _atlas, const std::shared_ptr<const UiImages>& _images);
  // After a device rebuild: the images again, into the resources the factory kept, and every page anew (RND-035).
  void UploadAgain();

  // _batches in their order over the target of _size, whose render target, viewport and scissor are set. A batch whose
  // texture is not on the GPU yet, whose glyph page's texture predates the page's last clear, or whose source and blend
  // have no catalog entry, is not drawn; the scissor is the whole target again afterwards.
  void Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
              std::span<const UiBatch> _batches);

private:
  struct Upload
  {
    GpuResourceId resource;
    std::vector<GpuSubresourceData> levels; // into the images' bytes, which the renderer keeps
  };
  /// A glyph page on the GPU: the texture drawn from and the page's revision it holds, and the texture of a later
  /// revision while its upload is under way.
  struct GlyphTexture
  {
    GpuResourceId shown;
    std::uint64_t shownRevision = 0;
    GpuResourceId incoming;
    std::uint64_t incomingRevision = 0;
    std::uint64_t incomingFence = 0;
    bool incomingScheduled = false;
  };

  void ReleaseImages();
  void ReleasePages();
  void PumpImages();
  void PumpPages();
  [[nodiscard]] bool ImagesReady() const;
  // The heap index of the texture that glyph page _page draws from this frame; none when it has none yet, or when its
  // texture predates the page's last clear.
  [[nodiscard]] std::optional<std::uint32_t> GlyphHeapIndex(std::uint16_t _page) const;

  DeviceGpuResourceFactory& m_factory;
  std::shared_ptr<const GlyphAtlas> m_atlas;
  std::shared_ptr<const UiImages> m_images;
  std::vector<GlyphTexture> m_pages;           // by page index
  std::vector<GpuResourceId> m_imageResources; // by image index; GpuResourceId{} for one that could not be made
  std::vector<Upload> m_uploads;               // every image's upload, for a rebuild
  std::vector<Upload> m_pending;
  std::uint64_t m_lastFence = 0;
  std::vector<TextGlyphInstance> m_instances; // the batch's scratch
};

} // namespace Neuron
