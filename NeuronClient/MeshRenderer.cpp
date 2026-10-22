#include "pch.h"

#include "Log.h"
#include "MeshRenderer.h"
#include "RootSignature.h"
#include "SceneQuads.h"
#include "ShaderLayouts.h"

#include <cstring>
#include <format>

namespace Neuron
{

static_assert(sizeof(MeshVertex) == MESH_VERTEX_BYTES, "the cooked vertex is the vertex stream as it stands");

MeshRenderer::MeshRenderer(DeviceGpuResourceFactory& _factory) noexcept
  : m_factory(_factory)
{
}

MeshRenderer::~MeshRenderer()
{
  Release();
}

void MeshRenderer::Show(const std::shared_ptr<const SceneMeshes>& _meshes)
{
  if (_meshes != m_meshes)
  {
    Release();
    m_meshes = _meshes;
    if (m_meshes != nullptr)
    {
      for (std::size_t n = 0; n < m_meshes->meshes.size(); ++n)
      {
        const MeshAsset* mesh = m_meshes->meshes[n].get();
        if (mesh == nullptr)
        {
          Log(LogCategory::Render, LogLevel::Error, "the scene mesh {} is missing", n);
          m_buffers.push_back(GpuResourceId{});
          continue;
        }
        const std::uint64_t bytes = mesh->vertices.size() * sizeof(MeshVertex);
        if (bytes == 0)
        {
          // A mesh of sockets alone, as the turret's mount is (RND-170, ADR-140), has nothing to draw.
          m_buffers.push_back(GpuResourceId{});
          continue;
        }
        const auto created = m_factory.CreateBuffer({bytes, 0, false, std::format("scene mesh {}", n)});
        if (!created)
        {
          Log(LogCategory::Render, LogLevel::Error, "the scene mesh {} was not created", n);
          m_buffers.push_back(GpuResourceId{});
          continue;
        }
        m_buffers.push_back(*created);
        m_pending.push_back(n);
      }
    }
  }
  Pump();
}

void MeshRenderer::UploadAgain()
{
  m_pending.clear();
  for (std::size_t n = 0; n < m_buffers.size(); ++n)
  {
    if (m_buffers[n] != GpuResourceId{})
    {
      m_pending.push_back(n);
    }
  }
  m_lastFence = 0;
  Pump();
}

void MeshRenderer::Pump()
{
  while (!m_pending.empty())
  {
    const std::size_t n = m_pending.front();
    const std::vector<MeshVertex>& vertices = m_meshes->meshes[n]->vertices;
    const std::array data{GpuSubresourceData{std::as_bytes(std::span(vertices)), 0}};
    const auto fence = m_factory.ScheduleUpload(m_buffers[n], data);
    if (!fence)
    {
      if (fence.error() != GpuResourceError::StagingFull && fence.error() != GpuResourceError::DeviceRebuilding)
      {
        Log(LogCategory::Render, LogLevel::Error, "a scene mesh was not uploaded");
        m_pending.clear();
      }
      return; // tried again next frame
    }
    m_lastFence = *fence;
    m_pending.erase(m_pending.begin());
  }
}

bool MeshRenderer::Ready() const
{
  return m_meshes != nullptr && m_pending.empty() && m_factory.IsUploadComplete(m_lastFence);
}

void MeshRenderer::Release()
{
  for (const GpuResourceId buffer : m_buffers)
  {
    if (buffer != GpuResourceId{})
    {
      m_factory.Release(buffer);
    }
  }
  m_buffers.clear();
  m_pending.clear();
  m_meshes.reset();
  m_lastFence = 0;
}

void MeshRenderer::Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, const FrameView& _frame,
                          ScenePass _pass, std::span<const MeshBatch> _batches)
{
  if (!Ready())
  {
    return;
  }
  const std::array<Float4, 6> planes = FrustumPlanes(_frame);
  for (const MeshBatch& batch : _batches)
  {
    ID3D12PipelineState* pipeline = batch.pass == _pass ? _catalog.Find(batch.entry) : nullptr;
    if (pipeline == nullptr || batch.mesh >= m_buffers.size() || m_buffers[batch.mesh] == GpuResourceId{})
    {
      continue;
    }
    ID3D12Resource* buffer = m_factory.Resource(m_buffers[batch.mesh]);
    const std::vector<MeshVertex>& vertices = m_meshes->meshes[batch.mesh]->vertices;
    m_instances.clear();
    for (const SceneMeshInstance& instance : batch.instances)
    {
      const Vec3 relative = instance.position - _frame.camera;
      const Vec3 extent{batch.cullRadius, batch.cullRadius, batch.cullRadius};
      if (batch.cullRadius <= 0.0f || BoxMayBeVisible(planes, relative - extent, relative + extent))
      {
        m_instances.push_back(MakeMeshInstance(instance, _frame.camera));
      }
    }
    if (m_instances.empty() || buffer == nullptr)
    {
      continue;
    }
    const std::uint64_t bytes = m_instances.size() * sizeof(MeshInstance);
    const UploadAllocation allocation = _ring.Allocate(bytes, sizeof(Float4));
    if (allocation.cpu == nullptr)
    {
      continue;
    }
    std::memcpy(allocation.cpu, m_instances.data(), static_cast<std::size_t>(bytes));
    const std::array<D3D12_VERTEX_BUFFER_VIEW, 2> views{{
      {buffer->GetGPUVirtualAddress(), static_cast<UINT>(vertices.size() * sizeof(MeshVertex)), static_cast<UINT>(sizeof(MeshVertex))},
      {allocation.gpu, static_cast<UINT>(bytes), static_cast<UINT>(sizeof(MeshInstance))},
    }};
    _commands->SetPipelineState(pipeline);
    _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commands->IASetVertexBuffers(0, static_cast<UINT>(views.size()), views.data());
    _commands->DrawInstanced(static_cast<UINT>(vertices.size()), static_cast<UINT>(m_instances.size()), 0, 0);
  }
}

} // namespace Neuron
