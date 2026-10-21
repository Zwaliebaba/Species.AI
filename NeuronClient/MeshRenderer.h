#pragma once

#include "DeviceGpuResourceFactory.h"
#include "FrameView.h"
#include "PipelineCatalog.h"
#include "UploadRing.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace Neuron
{

/// The scene's meshes (ADR-019, ADR-122): each cooked mesh's vertices in one buffer, uploaded once and again after a
/// device rebuild, and each pass's mesh batches, of which only the instances that pass the CPU view test are drawn, one
/// instanced call per batch (ADR-009's M1 form). Frame thread only.
class MeshRenderer
{
public:
  explicit MeshRenderer(DeviceGpuResourceFactory& _factory) noexcept;
  ~MeshRenderer();

  MeshRenderer(const MeshRenderer&) = delete;
  MeshRenderer& operator=(const MeshRenderer&) = delete;

  // Before the factory's frame: uploads the meshes when they are not the ones held.
  void Show(const std::shared_ptr<const SceneMeshes>& _meshes);
  // After a device rebuild: the same meshes again, into the buffers the factory kept (RND-035).
  void UploadAgain();

  // The batches of _pass in their order, into the scene targets that are bound, with the frame constants bound.
  void Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, const FrameView& _frame,
              ScenePass _pass, std::span<const MeshBatch> _batches);

private:
  void Release();
  void Pump();
  [[nodiscard]] bool Ready() const;

  DeviceGpuResourceFactory& m_factory;
  std::shared_ptr<const SceneMeshes> m_meshes;
  std::vector<GpuResourceId> m_buffers; // by mesh index; GpuResourceId{} for one that could not be made
  std::vector<std::size_t> m_pending;   // the meshes not yet scheduled
  std::vector<MeshInstance> m_instances;
  std::uint64_t m_lastFence = 0;
};

} // namespace Neuron
