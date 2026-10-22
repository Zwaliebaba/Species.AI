#include "pch.h"

#include "OverlayRenderer.h"
#include "RootSignature.h"
#include "ShaderLayouts.h"

#include <algorithm>
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

void OverlayRenderer::RecordRects(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog, SizePx _size,
                                  float _scale, std::span<const OverlayRect> _rects)
{
  ID3D12PipelineState* pipeline = _catalog.Find("development_solid");
  if (pipeline == nullptr || _rects.empty())
  {
    return;
  }
  const auto width = static_cast<float>(std::max(_size.width, 1u));
  const auto height = static_cast<float>(std::max(_size.height, 1u));
  PassConstants target{};
  target.values[0] = Float4(width, height, 1.0f / width, 1.0f / height);
  // ADR-120: the rectangles on whole pixels at the DPI scale; the shader reads no texture.
  std::vector<TextGlyphInstance> rectangles;
  rectangles.reserve(_rects.size());
  for (const OverlayRect& rect : _rects)
  {
    const float left = std::round(rect.left * _scale);
    const float top = std::round(rect.top * _scale);
    rectangles.push_back({Float4(left, top, left + std::round(rect.width * _scale), top + std::round(rect.height * _scale)),
                          Float4(0.0f, 0.0f, 0.0f, 0.0f), Float4(rect.color[0], rect.color[1], rect.color[2], rect.color[3])});
  }
  const DrawConstants none{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f), Float4(1.0f, 1.0f, 1.0f, 1.0f),
                           Uint4{0, 0, 0, 0}};
  _commands->SetPipelineState(pipeline);
  _commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _ring.Write(target));
  _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &none, 0);
  if (UploadInstances(_commands, _ring, rectangles))
  {
    _commands->DrawInstanced(6, static_cast<UINT>(rectangles.size()), 0, 0);
  }
}

void OverlayRenderer::RecordDots(ID3D12GraphicsCommandList* _commands, UploadRing& _ring, const PipelineCatalog& _catalog,
                                 const FrameView& _frame, float _scale, std::span<const DevelopmentDot> _dots)
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
    instances.push_back(
      {Float4(static_cast<float>(relative.x), static_cast<float>(relative.y), static_cast<float>(relative.z), dot.sidePixels * _scale),
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
