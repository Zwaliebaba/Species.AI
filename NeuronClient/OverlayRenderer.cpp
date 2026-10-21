#include "pch.h"

#include "Log.h"
#include "OverlayRenderer.h"
#include "RootSignature.h"
#include "ShaderLayouts.h"
#include "TextLayout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace Neuron
{

namespace
{

template <class T> bool UploadInstances(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const std::vector<T>& _instances)
{
  const std::uint64_t bytes = _instances.size() * sizeof(T);
  const UploadAllocation allocation = _ring.Allocate(bytes, sizeof(Float4));
  if (allocation.cpu == nullptr)
  {
    return false;
  }
  std::memcpy(allocation.cpu, _instances.data(), static_cast<std::size_t>(bytes));
  const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(bytes), sizeof(T)};
  _commands->IASetVertexBuffers(0, 1, &view);
  return true;
}

} // namespace

OverlayRenderer::OverlayRenderer(DeviceGpuResourceFactory& _factory) noexcept
  : m_factory(_factory)
{
}

OverlayRenderer::~OverlayRenderer()
{
  Release();
}

void OverlayRenderer::Show(const std::shared_ptr<const FontAsset>& _font)
{
  if (_font != m_font)
  {
    Release();
    m_font = _font;
    if (m_font != nullptr && m_font->pageWidth > 0 && m_font->pageHeight > 0 &&
        m_font->page.size() == std::size_t{m_font->pageWidth} * m_font->pageHeight)
    {
      // ADR-034, O63: the face's one R8_UNORM coverage page, never block-compressed.
      const auto created = m_factory.CreateTexture({m_font->pageWidth, m_font->pageHeight, 1, DXGI_FORMAT_R8_UNORM, "font.dev.editor"});
      if (created)
      {
        m_page = *created;
        m_pending = true;
      }
      else
      {
        Log(LogCategory::Render, LogLevel::Error, "the development face's page was not created");
      }
    }
  }
  if (m_pending)
  {
    Schedule();
  }
}

void OverlayRenderer::UploadAgain()
{
  if (m_page == GpuResourceId{})
  {
    return;
  }
  m_pending = true;
  m_fence = 0;
  Schedule();
}

bool OverlayRenderer::Ready() const
{
  return m_page != GpuResourceId{} && !m_pending && m_factory.IsUploadComplete(m_fence);
}

void OverlayRenderer::Release()
{
  if (m_page != GpuResourceId{})
  {
    m_factory.Release(m_page);
  }
  m_page = {};
  m_font.reset();
  m_fence = 0;
  m_pending = false;
}

void OverlayRenderer::Schedule()
{
  const std::array data{GpuSubresourceData{std::as_bytes(std::span(m_font->page)), m_font->pageWidth}};
  const auto fence = m_factory.ScheduleUpload(m_page, data);
  if (fence)
  {
    m_fence = *fence;
    m_pending = false;
  }
  else if (fence.error() != GpuResourceError::StagingFull && fence.error() != GpuResourceError::DeviceRebuilding)
  {
    Log(LogCategory::Render, LogLevel::Error, "the development face's page was not uploaded");
    m_factory.Release(m_page);
    m_pending = false;
    m_page = {};
  }
}

void OverlayRenderer::RecordText(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
                                 std::uint32_t _scale, std::span<const OverlayLine> _lines, std::span<const OverlayRect> _rects)
{
  ID3D12PipelineState* pipeline = _catalog.Find("development_text");
  ID3D12PipelineState* solidPipeline = _catalog.Find("development_solid");
  const auto width = static_cast<float>(std::max(_size.width, 1u));
  const auto height = static_cast<float>(std::max(_size.height, 1u));
  const auto scale = static_cast<float>(_scale);
  PassConstants target{};
  target.values[0] = Float4(width, height, 1.0f / width, 1.0f / height);
  const D3D12_GPU_VIRTUAL_ADDRESS targetConstants = _ring.Write(target);
  if (solidPipeline != nullptr && !_rects.empty())
  {
    // ADR-120: the rectangles in whole pixels at the text's scale, under the text; the solid shader reads no texture.
    std::vector<TextGlyphInstance> rectangles;
    rectangles.reserve(_rects.size());
    for (const OverlayRect& rect : _rects)
    {
      const float left = std::round(rect.left * scale);
      const float top = std::round(rect.top * scale);
      rectangles.push_back({Float4(left, top, left + std::round(rect.width * scale), top + std::round(rect.height * scale)),
                            Float4(0.0f, 0.0f, 0.0f, 0.0f), Float4(rect.color[0], rect.color[1], rect.color[2], rect.color[3])});
    }
    const DrawConstants none{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
                             Float4(1.0f, 1.0f, 1.0f, 1.0f), Uint4{0, 0, 0, 0}};
    _commands->SetPipelineState(solidPipeline);
    _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, targetConstants);
    _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &none, 0);
    if (UploadInstances(_commands, _ring, rectangles))
    {
      _commands->DrawInstanced(6, static_cast<UINT>(rectangles.size()), 0, 0);
    }
  }
  if (!Ready() || pipeline == nullptr || _lines.empty())
  {
    return;
  }
  const float cellHeight = static_cast<float>(m_font->cellHeightPixels) * scale;
  std::vector<TextGlyphInstance> glyphs;
  for (const OverlayLine& line : _lines)
  {
    // DBG-095: a bottom-right line is right-aligned; DBG-096: a page's line starts at its top-left corner. The anchor's
    // distances scale with the text.
    float left = std::round(line.insetX * scale);
    float top = std::round(line.insetY * scale);
    if (line.corner == OverlayCorner::BottomRight)
    {
      left = width - left - TextWidth(*m_font, line.text, _scale);
      top = height - top - cellHeight;
    }
    const Float4 color(line.color[0], line.color[1], line.color[2], line.color[3]);
    for (const GlyphQuad& quad : LayoutText(*m_font, line.text, _scale, left, top))
    {
      glyphs.push_back({Float4(quad.rectangle[0], quad.rectangle[1], quad.rectangle[2], quad.rectangle[3]),
                        Float4(quad.uv[0], quad.uv[1], quad.uv[2], quad.uv[3]), color});
    }
  }
  if (glyphs.empty())
  {
    return;
  }
  _commands->SetPipelineState(pipeline);
  _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, targetConstants);
  const DrawConstants page{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f), Float4(1.0f, 1.0f, 1.0f, 1.0f),
                           Uint4{m_factory.HeapIndex(m_page), 0, 0, 0}};
  _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &page, 0);
  if (UploadInstances(_commands, _ring, glyphs))
  {
    _commands->DrawInstanced(6, static_cast<UINT>(glyphs.size()), 0, 0);
  }
}

void OverlayRenderer::RecordDots(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog,
                                 const FrameView& _frame, std::uint32_t _scale, std::span<const DevelopmentDot> _dots)
{
  ID3D12PipelineState* pipeline = _catalog.Find("development_dots");
  if (pipeline == nullptr || _dots.empty())
  {
    return;
  }
  std::vector<DotInstance> instances;
  instances.reserve(_dots.size());
  for (const DevelopmentDot& dot : _dots)
  {
    const Vec3 relative = dot.position - _frame.camera; // WLD-246: the difference in binary64, rounded once
    instances.push_back({Float4(static_cast<float>(relative.x), static_cast<float>(relative.y), static_cast<float>(relative.z),
                                dot.sidePixels * static_cast<float>(_scale)),
                         Float4(dot.color[0], dot.color[1], dot.color[2], dot.color[3])});
  }
  _commands->SetPipelineState(pipeline);
  _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  if (UploadInstances(_commands, _ring, instances))
  {
    _commands->DrawInstanced(6, static_cast<UINT>(instances.size()), 0, 0);
  }
}

} // namespace Neuron
