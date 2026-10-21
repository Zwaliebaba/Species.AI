#include "pch.h"

#include "DdsTexture.h"
#include "Log.h"
#include "QuadRenderer.h"
#include "RootSignature.h"
#include "SceneQuads.h"
#include "ShaderLayouts.h"

#include <algorithm>
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
                          ScenePass _pass, std::span<const QuadBatch> _batches)
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

void QuadRenderer::RecordTriangles(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog,
                                   const FrameView& _frame, ScenePass _pass, std::span<const TriangleBatch> _batches)
{
  if (!Ready())
  {
    return;
  }
  const auto usable = [this](std::uint16_t _texture) { return _texture < m_resources.size() && m_resources[_texture] != GpuResourceId{}; };
  for (const TriangleBatch& batch : _batches)
  {
    ID3D12PipelineState* pipeline = batch.pass == _pass ? _catalog.Find(batch.entry) : nullptr;
    if (pipeline == nullptr || (batch.textured && (!usable(batch.texture) || !usable(batch.mask))) || batch.vertices.empty())
    {
      continue;
    }
    m_vertices.clear();
    for (const SceneVertex& vertex : batch.vertices)
    {
      // A screen-space corner is already in clip space, so the camera is not taken from it.
      m_vertices.push_back(MakeTriangleVertex(vertex, batch.screenSpace ? Vec3{} : _frame.camera));
    }
    const std::uint64_t bytes = m_vertices.size() * sizeof(TriangleVertex);
    const UploadAllocation allocation = _ring.Allocate(bytes, sizeof(Float4));
    if (allocation.cpu == nullptr)
    {
      continue;
    }
    std::memcpy(allocation.cpu, m_vertices.data(), static_cast<std::size_t>(bytes));
    const Uint4 indices = batch.textured
                            ? Uint4{m_factory.HeapIndex(m_resources[batch.texture]), m_factory.HeapIndex(m_resources[batch.mask]), 0, 0}
                            : Uint4{DEFAULT_RESOURCE_INDEX, DEFAULT_RESOURCE_INDEX, batch.screenSpace ? TRIANGLE_SCREEN_SPACE : 0u, 0};
    const DrawConstants draw{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
                             Float4(1.0f, 1.0f, 1.0f, 1.0f), indices};
    const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(bytes), static_cast<UINT>(sizeof(TriangleVertex))};
    _commands->SetPipelineState(pipeline);
    _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &draw, 0);
    _commands->IASetVertexBuffers(0, 1, &view);
    _commands->DrawInstanced(static_cast<UINT>(m_vertices.size()), 1, 0, 0);
  }
}

void QuadRenderer::RecordOverlay(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
                                 std::span<const OverlayQuad> _quads)
{
  ID3D12PipelineState* alpha = _catalog.Find("overlay_alpha");
  ID3D12PipelineState* additive = _catalog.Find("overlay_additive");
  if (!Ready() || _quads.empty() || alpha == nullptr || additive == nullptr)
  {
    return;
  }
  const auto width = static_cast<float>(std::max(_size.width, 1u));
  const auto height = static_cast<float>(std::max(_size.height, 1u));
  PassConstants target{};
  target.values[0] = Float4(width, height, 1.0f / width, 1.0f / height);
  _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _ring.Write(target));
  for (std::size_t first = 0; first < _quads.size();)
  {
    // A run of the same texture and blend, in order.
    std::size_t end = first + 1;
    while (end < _quads.size() && _quads[end].texture == _quads[first].texture && _quads[end].blend == _quads[first].blend)
    {
      ++end;
    }
    const OverlayQuad& run = _quads[first];
    const bool usable = run.texture < m_resources.size() && m_resources[run.texture] != GpuResourceId{};
    m_overlay.clear();
    for (std::size_t n = first; n < end && usable; ++n)
    {
      const OverlayQuad& quad = _quads[n];
      m_overlay.push_back({Float4(quad.centerX, quad.centerY, quad.halfSize, quad.rotationRadians),
                           Float4(quad.color[0], quad.color[1], quad.color[2], quad.color[3])});
    }
    first = end;
    const std::uint64_t bytes = m_overlay.size() * sizeof(OverlayQuadInstance);
    const UploadAllocation allocation = m_overlay.empty() ? UploadAllocation{} : _ring.Allocate(bytes, sizeof(Float4));
    if (allocation.cpu == nullptr)
    {
      continue;
    }
    std::memcpy(allocation.cpu, m_overlay.data(), static_cast<std::size_t>(bytes));
    const DrawConstants draw{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
                             Float4(1.0f, 1.0f, 1.0f, 1.0f), Uint4{m_factory.HeapIndex(m_resources[run.texture]), 0, 0, 0}};
    const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(bytes), static_cast<UINT>(sizeof(OverlayQuadInstance))};
    _commands->SetPipelineState(run.blend == OverlayBlend::Alpha ? alpha : additive);
    _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &draw, 0);
    _commands->IASetVertexBuffers(0, 1, &view);
    _commands->DrawInstanced(6, static_cast<UINT>(m_overlay.size()), 0, 0);
  }
}

std::uint32_t QuadRenderer::TakeDrawnQuads() noexcept
{
  return std::exchange(m_drawnQuads, 0u);
}

} // namespace Neuron
