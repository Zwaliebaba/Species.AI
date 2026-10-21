#include "pch.h"

#include "CursorRenderer.h"
#include "DdsTexture.h"
#include "Log.h"
#include "RootSignature.h"
#include "ShaderLayouts.h"

#include <array>
#include <cstring>
#include <utility>

namespace Neuron
{

CursorRenderer::CursorRenderer(DeviceGpuResourceFactory& _factory) noexcept
  : m_factory(_factory)
{
}

CursorRenderer::~CursorRenderer()
{
  Release();
}

GpuResourceId CursorRenderer::Create(const std::vector<std::byte>& _file, const char* _name)
{
  auto described = DescribeDds(_file, _name);
  if (!described)
  {
    Log(LogCategory::Render, LogLevel::Error, "the cursor's texture cannot be used: {}", described.error());
    m_failed = true;
    return GpuResourceId{};
  }
  const auto created = m_factory.CreateTexture(described->desc);
  if (!created)
  {
    Log(LogCategory::Render, LogLevel::Error, "the cursor texture {} was not created", _name);
    m_failed = true;
    return GpuResourceId{};
  }
  m_uploads.push_back({*created, std::move(described->levels)});
  m_pending.push_back(m_uploads.back());
  return *created;
}

void CursorRenderer::Show(const std::shared_ptr<const CursorImages>& _images)
{
  if (_images != m_images)
  {
    Release();
    m_images = _images;
    if (m_images != nullptr)
    {
      for (std::size_t n = 0; n < m_images->art.size(); ++n)
      {
        m_art.push_back(Create(m_images->art[n], "cursor"));
        const bool shadowed = n < m_images->shadows.size() && !m_images->shadows[n].empty();
        m_shadows.push_back(shadowed ? Create(m_images->shadows[n], "cursor shadow") : GpuResourceId{});
      }
    }
  }
  Pump();
}

void CursorRenderer::UploadAgain()
{
  // The factory kept every id and heap index and holds nothing in them (RND-035).
  m_pending = m_uploads;
  m_lastFence = 0;
  Pump();
}

void CursorRenderer::Pump()
{
  while (!m_pending.empty())
  {
    const auto fence = m_factory.ScheduleUpload(m_pending.front().resource, m_pending.front().levels);
    if (!fence)
    {
      if (fence.error() != GpuResourceError::StagingFull && fence.error() != GpuResourceError::DeviceRebuilding)
      {
        Log(LogCategory::Render, LogLevel::Error, "a cursor texture was not uploaded");
        m_failed = true;
        m_pending.clear();
      }
      return; // tried again next frame
    }
    m_lastFence = *fence;
    m_pending.erase(m_pending.begin());
  }
}

void CursorRenderer::Release()
{
  for (const std::vector<GpuResourceId>* resources : {&m_art, &m_shadows})
  {
    for (const GpuResourceId resource : *resources)
    {
      if (resource != GpuResourceId{})
      {
        m_factory.Release(resource);
      }
    }
  }
  m_art.clear();
  m_shadows.clear();
  m_uploads.clear();
  m_pending.clear();
  m_images.reset();
  m_lastFence = 0;
  m_failed = false;
}

void CursorRenderer::Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
                            const CursorQuad& _cursor)
{
  ID3D12PipelineState* shadowPipeline = _catalog.Find("cursor_shadow");
  ID3D12PipelineState* artPipeline = _catalog.Find("cursor_art");
  const bool ready = !m_failed && m_pending.empty() && m_factory.IsUploadComplete(m_lastFence);
  if (!ready || shadowPipeline == nullptr || artPipeline == nullptr || _cursor.image >= m_art.size() ||
      m_art[_cursor.image] == GpuResourceId{})
  {
    return;
  }
  const auto width = static_cast<float>(std::max(_size.width, 1u));
  const auto height = static_cast<float>(std::max(_size.height, 1u));
  PassConstants target{};
  target.values[0] = Float4(width, height, 1.0f / width, 1.0f / height);
  _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _ring.Write(target));
  // R-CTRL-352: the quad at its corner, the whole mask across it.
  const Float4 rectangle(_cursor.left, _cursor.top, _cursor.left + _cursor.sidePixels, _cursor.top + _cursor.sidePixels);
  const auto draw = [&](ID3D12PipelineState* _pipeline, GpuResourceId _mask, const Float4& _color)
  {
    const TextGlyphInstance instance{rectangle, Float4(0.0f, 0.0f, 1.0f, 1.0f), _color};
    const UploadAllocation allocation = _ring.Allocate(sizeof(instance), sizeof(Float4));
    if (allocation.cpu == nullptr)
    {
      return;
    }
    std::memcpy(allocation.cpu, &instance, sizeof(instance));
    const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(sizeof(instance)), static_cast<UINT>(sizeof(instance))};
    const DrawConstants mask{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
                             Float4(1.0f, 1.0f, 1.0f, 1.0f), Uint4{m_factory.HeapIndex(_mask), 0, 0, 0}};
    _commands->SetPipelineState(_pipeline);
    _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &mask, 0);
    _commands->IASetVertexBuffers(0, 1, &view);
    _commands->DrawInstanced(6, 1, 0, 0);
  };
  // RND-366: the shadow darkens by its texel, in white with alpha 0 (Ink at alpha 0 is Darkening, RND-058); then the art.
  if (_cursor.image < m_shadows.size() && m_shadows[_cursor.image] != GpuResourceId{})
  {
    draw(shadowPipeline, m_shadows[_cursor.image], Float4(1.0f, 1.0f, 1.0f, 0.0f));
  }
  draw(artPipeline, m_art[_cursor.image], Float4(_cursor.color[0], _cursor.color[1], _cursor.color[2], _cursor.color[3]));
}

} // namespace Neuron
