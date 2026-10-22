#include "pch.h"

#include "DdsTexture.h"
#include "Log.h"
#include "RootSignature.h"
#include "UiRenderer.h"

#include <algorithm>
#include <cstring>
#include <format>
#include <string>
#include <string_view>

namespace Neuron
{

namespace
{

// The catalogue entries of the UI (ADR-145), by source and blend; an empty key is a pair the UI does not draw.
std::string_view EntryOf(UiSource _source, UiBlend _blend) noexcept
{
  switch (_source)
  {
  case UiSource::Solid:
    return _blend == UiBlend::Alpha ? "ui_solid_alpha" : _blend == UiBlend::Additive ? "ui_solid_additive" : "";
  case UiSource::Glyphs:
    return _blend == UiBlend::Alpha ? "ui_glyphs_alpha" : _blend == UiBlend::Additive ? "ui_glyphs_additive" : "ui_glyphs_ink";
  case UiSource::Image:
    return _blend == UiBlend::Alpha ? "ui_image_alpha" : _blend == UiBlend::Additive ? "ui_image_additive" : "";
  }
  return "";
}

} // namespace

UiRenderer::UiRenderer(DeviceGpuResourceFactory& _factory) noexcept
  : m_factory(_factory)
{
}

UiRenderer::~UiRenderer()
{
  Release();
}

void UiRenderer::Show(const std::shared_ptr<const FontAsset>& _face, const std::shared_ptr<const UiImages>& _images)
{
  if (_face != m_face || _images != m_images)
  {
    Release();
    m_face = _face;
    m_images = _images;
    if (m_face != nullptr && m_face->pageWidth > 0 && m_face->pageHeight > 0 &&
        m_face->page.size() == std::size_t{m_face->pageWidth} * m_face->pageHeight)
    {
      // ADR-034, O63: the face's one R8_UNORM coverage page, never block-compressed.
      const auto created = m_factory.CreateTexture({m_face->pageWidth, m_face->pageHeight, 1, DXGI_FORMAT_R8_UNORM, "font.ui.primary"});
      if (created)
      {
        m_page = *created;
        m_uploads.push_back({m_page, {GpuSubresourceData{std::as_bytes(std::span(m_face->page)), m_face->pageWidth}}});
      }
      else
      {
        Log(LogCategory::Render, LogLevel::Error, "the UI face's page was not created");
      }
    }
    if (m_images != nullptr)
    {
      for (std::size_t n = 0; n < m_images->files.size(); ++n)
      {
        const std::string name = std::format("UI image {}", n);
        auto described = DescribeDds(m_images->files[n], name);
        if (!described)
        {
          Log(LogCategory::Render, LogLevel::Error, "the {} cannot be used: {}", name, described.error());
          m_imageResources.push_back(GpuResourceId{});
          continue;
        }
        const auto created = m_factory.CreateTexture(described->desc);
        if (!created)
        {
          Log(LogCategory::Render, LogLevel::Error, "the {} was not created", name);
          m_imageResources.push_back(GpuResourceId{});
          continue;
        }
        m_imageResources.push_back(*created);
        m_uploads.push_back({*created, std::move(described->levels)});
      }
    }
    m_pending = m_uploads;
  }
  Pump();
}

void UiRenderer::UploadAgain()
{
  m_pending = m_uploads;
  m_lastFence = 0;
  Pump();
}

void UiRenderer::Pump()
{
  while (!m_pending.empty())
  {
    const auto fence = m_factory.ScheduleUpload(m_pending.front().resource, m_pending.front().levels);
    if (!fence)
    {
      if (fence.error() != GpuResourceError::StagingFull && fence.error() != GpuResourceError::DeviceRebuilding)
      {
        Log(LogCategory::Render, LogLevel::Error, "a UI texture was not uploaded");
        m_pending.clear();
      }
      return; // tried again next frame
    }
    m_lastFence = *fence;
    m_pending.erase(m_pending.begin());
  }
}

bool UiRenderer::Ready() const
{
  return (m_face != nullptr || m_images != nullptr) && m_pending.empty() && m_factory.IsUploadComplete(m_lastFence);
}

void UiRenderer::Release()
{
  if (m_page != GpuResourceId{})
  {
    m_factory.Release(m_page);
  }
  for (const GpuResourceId resource : m_imageResources)
  {
    if (resource != GpuResourceId{})
    {
      m_factory.Release(resource);
    }
  }
  m_page = {};
  m_imageResources.clear();
  m_uploads.clear();
  m_pending.clear();
  m_face.reset();
  m_images.reset();
  m_lastFence = 0;
}

void UiRenderer::Record(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
                        std::span<const UiBatch> _batches)
{
  if (_batches.empty())
  {
    return;
  }
  const bool ready = Ready();
  const auto width = static_cast<float>(std::max(_size.width, 1u));
  const auto height = static_cast<float>(std::max(_size.height, 1u));
  PassConstants target{};
  target.values[0] = Float4(width, height, 1.0f / width, 1.0f / height);
  _commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _ring.Write(target));
  _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  const D3D12_RECT whole{0, 0, static_cast<LONG>(std::max(_size.width, 1u)), static_cast<LONG>(std::max(_size.height, 1u))};
  bool scissored = false;
  for (const UiBatch& batch : _batches)
  {
    ID3D12PipelineState* pipeline = batch.quads.empty() ? nullptr : _catalog.Find(EntryOf(batch.source, batch.blend));
    std::uint32_t texture = 0;
    if (batch.source == UiSource::Glyphs)
    {
      if (!ready || m_page == GpuResourceId{})
      {
        continue;
      }
      texture = m_factory.HeapIndex(m_page);
    }
    else if (batch.source == UiSource::Image)
    {
      if (!ready || batch.image >= m_imageResources.size() || m_imageResources[batch.image] == GpuResourceId{})
      {
        continue;
      }
      texture = m_factory.HeapIndex(m_imageResources[batch.image]);
    }
    if (pipeline == nullptr)
    {
      continue;
    }
    // UI-181: the batch's scissor, or the whole target.
    if (batch.scissor.has_value())
    {
      const UiScissor& clip = *batch.scissor;
      const D3D12_RECT rect{std::clamp<LONG>(clip.left, 0, whole.right), std::clamp<LONG>(clip.top, 0, whole.bottom),
                            std::clamp<LONG>(clip.right, 0, whole.right), std::clamp<LONG>(clip.bottom, 0, whole.bottom)};
      if (rect.right <= rect.left || rect.bottom <= rect.top)
      {
        continue;
      }
      _commands->RSSetScissorRects(1, &rect);
      scissored = true;
    }
    else if (scissored)
    {
      _commands->RSSetScissorRects(1, &whole);
      scissored = false;
    }
    m_instances.clear();
    for (const UiQuad& quad : batch.quads)
    {
      m_instances.push_back({Float4(quad.rectangle[0], quad.rectangle[1], quad.rectangle[2], quad.rectangle[3]),
                             Float4(quad.uv[0], quad.uv[1], quad.uv[2], quad.uv[3]),
                             Float4(quad.color[0], quad.color[1], quad.color[2], quad.color[3])});
    }
    const std::uint64_t bytes = m_instances.size() * sizeof(TextGlyphInstance);
    const UploadAllocation allocation = _ring.Allocate(bytes, sizeof(Float4));
    if (allocation.cpu == nullptr)
    {
      break; // the ring is full this frame
    }
    std::memcpy(allocation.cpu, m_instances.data(), static_cast<std::size_t>(bytes));
    const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(bytes), static_cast<UINT>(sizeof(TextGlyphInstance))};
    const DrawConstants draw{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
                             Float4(1.0f, 1.0f, 1.0f, 1.0f), Uint4{texture, 0, 0, 0}};
    _commands->SetPipelineState(pipeline);
    _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &draw, 0);
    _commands->IASetVertexBuffers(0, 1, &view);
    _commands->DrawInstanced(6, static_cast<UINT>(m_instances.size()), 0, 0);
  }
  if (scissored)
  {
    _commands->RSSetScissorRects(1, &whole);
  }
}

} // namespace Neuron
