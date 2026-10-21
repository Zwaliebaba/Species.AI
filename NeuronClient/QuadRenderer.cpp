#include "pch.h"

#include "DdsTexture.h"
#include "Log.h"
#include "QuadRenderer.h"
#include "RootSignature.h"
#include "SceneQuads.h"
#include "ShaderLayouts.h"

#include <cstring>
#include <format>
#include <utility>

namespace Neuron
{

QuadRenderer::QuadRenderer(DeviceGpuResourceFactory& _factory) noexcept
  : m_factory(_factory)
{
}

QuadRenderer::~QuadRenderer()
{
  Release();
}

void QuadRenderer::Show(const std::shared_ptr<const SceneTextures>& _textures)
{
  if (_textures != m_textures)
  {
    Release();
    m_textures = _textures;
    if (m_textures != nullptr)
    {
      for (std::size_t n = 0; n < m_textures->files.size(); ++n)
      {
        const std::string name = std::format("scene texture {}", n);
        auto described = DescribeDds(m_textures->files[n], name);
        if (!described)
        {
          Log(LogCategory::Render, LogLevel::Error, "the {} cannot be used: {}", name, described.error());
          m_resources.push_back(GpuResourceId{});
          continue;
        }
        const auto created = m_factory.CreateTexture(described->desc);
        if (!created)
        {
          Log(LogCategory::Render, LogLevel::Error, "the {} was not created", name);
          m_resources.push_back(GpuResourceId{});
          continue;
        }
        m_resources.push_back(*created);
        m_uploads.push_back({*created, std::move(described->levels)});
      }
      m_pending = m_uploads;
    }
  }
  Pump();
}

void QuadRenderer::UploadAgain()
{
  m_pending = m_uploads;
  m_lastFence = 0;
  Pump();
}

void QuadRenderer::Pump()
{
  while (!m_pending.empty())
  {
    const auto fence = m_factory.ScheduleUpload(m_pending.front().resource, m_pending.front().levels);
    if (!fence)
    {
      if (fence.error() != GpuResourceError::StagingFull && fence.error() != GpuResourceError::DeviceRebuilding)
      {
        Log(LogCategory::Render, LogLevel::Error, "a scene texture was not uploaded");
        m_pending.clear();
      }
      return; // tried again next frame
    }
    m_lastFence = *fence;
    m_pending.erase(m_pending.begin());
  }
}

bool QuadRenderer::Ready() const
{
  return m_textures != nullptr && m_pending.empty() && m_factory.IsUploadComplete(m_lastFence);
}

void QuadRenderer::Release()
{
  for (const GpuResourceId resource : m_resources)
  {
    if (resource != GpuResourceId{})
    {
      m_factory.Release(resource);
    }
  }
  m_resources.clear();
  m_uploads.clear();
  m_pending.clear();
  m_textures.reset();
  m_lastFence = 0;
}

void QuadRenderer::Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, const FrameView& _frame,
                          QuadPass _pass, std::span<const QuadBatch> _batches)
{
  if (!Ready())
  {
    return;
  }
  const std::array<Float4, 6> planes = FrustumPlanes(_frame);
  for (const QuadBatch& batch : _batches)
  {
    ID3D12PipelineState* pipeline = batch.pass == _pass ? _catalog.Find(batch.entry) : nullptr;
    if (pipeline == nullptr || batch.texture >= m_resources.size() || m_resources[batch.texture] == GpuResourceId{})
    {
      continue;
    }
    m_instances.clear();
    for (const SceneQuad& quad : batch.quads)
    {
      if (batch.cullRadius <= 0.0f || QuadMayBeVisible(planes, quad, _frame.camera, batch.cullRadius))
      {
        m_instances.push_back(MakeQuadInstance(quad, _frame.camera));
      }
    }
    if (m_instances.empty())
    {
      continue;
    }
    const std::uint64_t bytes = m_instances.size() * sizeof(QuadInstance);
    const UploadAllocation allocation = _ring.Allocate(bytes, sizeof(Float4));
    if (allocation.cpu == nullptr)
    {
      continue;
    }
    std::memcpy(allocation.cpu, m_instances.data(), static_cast<std::size_t>(bytes));
    PassConstants constants{};
    constants.values[0] = Float4(batch.cutout, 0.0f, 0.0f, 0.0f);
    const DrawConstants draw{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
                             Float4(1.0f, 1.0f, 1.0f, 1.0f), Uint4{m_factory.HeapIndex(m_resources[batch.texture]), 0, 0, 0}};
    const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(bytes), static_cast<UINT>(sizeof(QuadInstance))};
    _commands->SetPipelineState(pipeline);
    _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _ring.Write(constants));
    _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &draw, 0);
    _commands->IASetVertexBuffers(0, 1, &view);
    _commands->DrawInstanced(6, static_cast<UINT>(m_instances.size()), 0, 0);
    m_drawnQuads += static_cast<std::uint32_t>(m_instances.size());
  }
}

std::uint32_t QuadRenderer::TakeDrawnQuads() noexcept
{
  return std::exchange(m_drawnQuads, 0u);
}

} // namespace Neuron
