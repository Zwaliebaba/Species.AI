#pragma once

#include "DeviceGpuResourceFactory.h"
#include "FrameView.h"
#include "PipelineCatalog.h"
#include "UploadRing.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace Neuron
{

/// The world frame's surfaces on the GPU (RND-130 to RND-155, ADR-011, ADR-115): the terrain's chunks, the sea's flat
/// layer and shore mask, the textures the passes read, and each frame's wave mesh and sky instances in the upload ring.
/// It keeps the world it shows, so that after a device rebuild the same surfaces are uploaded again (RND-034). It also
/// holds the view mask the terrain and the sea read (ADR-144). Every call is made from the frame thread.
class WorldRenderer
{
public:
  explicit WorldRenderer(DeviceGpuResourceFactory& _factory) noexcept;
  ~WorldRenderer();

  WorldRenderer(const WorldRenderer&) = delete;
  WorldRenderer& operator=(const WorldRenderer&) = delete;

  // Before the factory's frame: shows _world, uploading its surfaces when it is not the world shown and releasing the
  // last one's, and schedules the uploads that waited for staging memory.
  void Show(const std::shared_ptr<const WorldScene>& _world);
  // After a device rebuild: every surface again, into the resources the factory kept (RND-035).
  void UploadAgain();
  // A world is shown and every upload of it has completed.
  [[nodiscard]] bool Ready() const;

  // ADR-144: before the factory's frame, the view mask _mask, uploaded once into a texture of its own. The mask shown
  // stays until the new one's upload completes, so a frame never waits for it; none clears it at once.
  void ShowMask(const std::shared_ptr<const ViewMask>& _mask);
  // The mask shown as the frame constants place it (FrameConstants::viewMask, viewMaskIndex), about the camera _camera.
  void MaskConstants(const Vec3& _camera, Float4& _place, Uint4& _index) const;

  struct PassContext
  {
    ID3D12GraphicsCommandList* commands = nullptr;
    UploadRing* ring = nullptr;
    const PipelineCatalog* catalog = nullptr;
    const FrameView* frame = nullptr;
    double presentationSeconds = 0.0;
    double skyTicks = 0.0;
  };

  void RecordSky(const PassContext& _pass);     // pass 3 (RND-150)
  void RecordTerrain(const PassContext& _pass); // pass 4 (RND-136, RND-138)
  void RecordFlatSea(const PassContext& _pass); // pass 5a (RND-143)
  void RecordWaves(const PassContext& _pass);   // pass 5b (RND-146)

  // The chunks the last terrain pass drew, of the chunks the world has.
  [[nodiscard]] std::uint32_t DrawnChunks() const noexcept
  {
    return m_drawnChunks;
  }

private:
  struct Upload
  {
    GpuResourceId resource;
    std::vector<GpuSubresourceData> data;
  };

  /// A view mask in its texture, and the fence after which the texture holds it; none while its upload waits.
  struct MaskTexture
  {
    std::shared_ptr<const ViewMask> mask;
    std::optional<GpuResourceId> texture;
    std::optional<std::uint64_t> fence;
  };

  void Release();
  void Create();
  void ReleaseMask(MaskTexture& _mask);
  void PumpMask();
  void Schedule(GpuResourceId _resource, std::vector<GpuSubresourceData> _data);
  void Pump();
  [[nodiscard]] D3D12_VERTEX_BUFFER_VIEW BufferView(GpuResourceId _resource, std::uint64_t _bytes, std::uint32_t _stride) const;
  void Draw(ID3D12GraphicsCommandList* _commands, const Vec3& _origin, const Vec3& _camera, const Uint4& _indices) const;

  DeviceGpuResourceFactory& m_factory;
  std::shared_ptr<const WorldScene> m_world;
  std::vector<GpuResourceId> m_chunks;
  GpuResourceId m_flatSea;
  GpuResourceId m_shoreMask;
  GpuResourceId m_seaTile;
  GpuResourceId m_terrainGrid;
  GpuResourceId m_skyCloud;
  GpuResourceId m_beamShort;
  std::vector<GpuResourceId> m_resources; // every resource above, for release
  std::vector<Upload> m_uploads;          // every upload of the world, which a rebuild repeats
  std::vector<Upload> m_pending;          // those not yet scheduled, refused while staging memory was full
  std::uint64_t m_lastFence = 0;
  bool m_failed = false; // a surface could not be created: the world is never drawn
  std::vector<SeaVertex> m_waves;
  std::uint32_t m_drawnChunks = 0;
  MaskTexture m_shownMask;                       // the mask the frames read
  MaskTexture m_nextMask;                        // the mask wanted, until its upload completes
  std::shared_ptr<const ViewMask> m_refusedMask; // a mask whose texture the factory refused, which is not asked again
};

} // namespace Neuron
