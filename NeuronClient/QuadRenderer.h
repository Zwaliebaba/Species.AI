#pragma once

#include "DeviceGpuResourceFactory.h"
#include "FrameView.h"
#include "PipelineCatalog.h"
#include "UploadRing.h"
#include "WindowGeometry.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Neuron
{

/// The quads of the actor, effect, orb and particle passes (ADR-009's M1 form, ADR-121): the scene's textures on the GPU,
/// and each pass's batches, of which only the quads that pass the CPU view test are written into the upload ring and
/// drawn, one instanced call per batch; the scene's textured triangles, the cloth flags of ADR-122; and the overlay's
/// quads of ADR-125, which read the same textures. Frame thread only.
class QuadRenderer
{
public:
  explicit QuadRenderer(DeviceGpuResourceFactory& _factory) noexcept;
  ~QuadRenderer();

  QuadRenderer(const QuadRenderer&) = delete;
  QuadRenderer& operator=(const QuadRenderer&) = delete;

  // Before the factory's frame: uploads the textures when they are not the ones held.
  void Show(const std::shared_ptr<const SceneTextures>& _textures);
  // After a device rebuild: the same textures again, into the resources the factory kept (RND-035).
  void UploadAgain();

  // The batches of _pass in their order, into the scene targets that are bound, with the frame constants bound.
  void Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, const FrameView& _frame,
              ScenePass _pass, std::span<const QuadBatch> _batches);

  // The triangle batches of _pass in their order.
  void RecordTriangles(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, const FrameView& _frame,
                       ScenePass _pass, std::span<const TriangleBatch> _batches);

  // The overlay's quads in their order, over the target of _size, whose render target, viewport and scissor are set
  // (ADR-125).
  void RecordOverlay(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
                     std::span<const OverlayQuad> _quads);

  // The quads drawn since the last call, of every pass.
  [[nodiscard]] std::uint32_t TakeDrawnQuads() noexcept;

private:
  struct Upload
  {
    GpuResourceId resource;
    std::vector<GpuSubresourceData> levels; // into the textures' bytes, which m_textures keeps
  };

  void Release();
  void Pump();
  [[nodiscard]] bool Ready() const;

  DeviceGpuResourceFactory& m_factory;
  std::shared_ptr<const SceneTextures> m_textures;
  std::vector<GpuResourceId> m_resources; // by texture index; GpuResourceId{} for one that could not be made
  std::vector<Upload> m_uploads;          // every upload, for a rebuild
  std::vector<Upload> m_pending;
  std::vector<QuadInstance> m_instances; // the frame's scratch
  std::vector<TriangleVertex> m_vertices;
  std::vector<OverlayQuadInstance> m_overlay;
  std::uint64_t m_lastFence = 0;
  std::uint32_t m_drawnQuads = 0;
};

} // namespace Neuron
