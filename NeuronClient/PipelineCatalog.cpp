#include "pch.h"

#include "Log.h"
#include "PipelineCatalog.h"
#include "TextEncoding.h"

#include <algorithm>
#include <array>
#include <climits>
#include <format>

namespace Neuron
{

namespace
{

// RND-055 and ADR-005: blending off, or the factors of a catalogue blend state. The alpha factors keep the destination's
// alpha, which nothing reads, because an alpha factor may not name a colour (ADR-113).
D3D12_RENDER_TARGET_BLEND_DESC TargetBlendOf(BlendState _blend) noexcept
{
  D3D12_RENDER_TARGET_BLEND_DESC desc{.BlendEnable = TRUE,
                                      .LogicOpEnable = FALSE,
                                      .SrcBlend = D3D12_BLEND_ONE,
                                      .DestBlend = D3D12_BLEND_ZERO,
                                      .BlendOp = D3D12_BLEND_OP_ADD,
                                      .SrcBlendAlpha = D3D12_BLEND_ZERO,
                                      .DestBlendAlpha = D3D12_BLEND_ONE,
                                      .BlendOpAlpha = D3D12_BLEND_OP_ADD,
                                      .LogicOp = D3D12_LOGIC_OP_NOOP,
                                      .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL};
  switch (_blend)
  {
  case BlendState::Opaque:
    desc.BlendEnable = FALSE;
    break;
  case BlendState::Alpha:
    desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    break;
  case BlendState::Additive:
    desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    desc.DestBlend = D3D12_BLEND_ONE;
    break;
  case BlendState::Darkening:
    desc.SrcBlend = D3D12_BLEND_ZERO;
    desc.DestBlend = D3D12_BLEND_INV_SRC_COLOR;
    break;
  }
  return desc;
}

D3D12_BLEND_DESC BlendOf(BlendState _blend) noexcept
{
  const D3D12_RENDER_TARGET_BLEND_DESC target = TargetBlendOf(_blend);
  return D3D12_BLEND_DESC{.AlphaToCoverageEnable = FALSE,
                          .IndependentBlendEnable = FALSE,
                          .RenderTarget = {target, target, target, target, target, target, target, target}};
}

// RND-055: solid fill, counter-clockwise front faces (ADR-016), depth clip on, and the entry's depth bias and slope-scaled
// bias, none by default.
// Under reversed Z a positive bias draws nearer the camera (RND-219).
D3D12_RASTERIZER_DESC RasterizerOf(CullState _cull, bool _multisampled, std::int32_t _depthBias, float _slopeDepthBias) noexcept
{
  return D3D12_RASTERIZER_DESC{.FillMode = D3D12_FILL_MODE_SOLID,
                               .CullMode = _cull == CullState::Back ? D3D12_CULL_MODE_BACK : D3D12_CULL_MODE_NONE,
                               .FrontCounterClockwise = TRUE,
                               .DepthBias = _depthBias,
                               .DepthBiasClamp = 0.0f,
                               .SlopeScaledDepthBias = _slopeDepthBias,
                               .DepthClipEnable = TRUE,
                               .MultisampleEnable = _multisampled ? TRUE : FALSE,
                               .AntialiasedLineEnable = FALSE,
                               .ForcedSampleCount = 0,
                               .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF};
}

// RND-055, RND-059 and ADR-016: reversed Z, so the test is greater-or-equal; no stencil exists.
D3D12_DEPTH_STENCIL_DESC DepthOf(DepthState _depth) noexcept
{
  constexpr D3D12_DEPTH_STENCILOP_DESC KEEP{.StencilFailOp = D3D12_STENCIL_OP_KEEP,
                                            .StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
                                            .StencilPassOp = D3D12_STENCIL_OP_KEEP,
                                            .StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS};
  return D3D12_DEPTH_STENCIL_DESC{.DepthEnable = _depth == DepthState::None ? FALSE : TRUE,
                                  .DepthWriteMask =
                                    _depth == DepthState::TestWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
                                  .DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL,
                                  .StencilEnable = FALSE,
                                  .StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
                                  .StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK,
                                  .FrontFace = KEEP,
                                  .BackFace = KEEP};
}

// The streams' elements, in the order and at the offsets of TerrainVertex, SeaVertex and SkyLayerInstance.
constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 4> TERRAIN_ELEMENTS{{
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
}};
constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 2> SEA_ELEMENTS{{
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
}};
constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 5> SKY_ELEMENTS{{
  {"RECTANGLE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"SHAPE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"SAMPLING", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
}};

constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 3> TEXT_ELEMENTS{{
  {"RECTANGLE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
}};
constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 8> QUAD_ELEMENTS{{
  {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"ENDPOINT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"AXISRIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"AXISUP", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 96, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"SHAPE", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 112, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
}};

// A cooked MeshVertex (32 bytes, AST-076) and, per instance, a MeshInstance.
constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 9> MESH_ELEMENTS{{
  {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"TINT", 0, DXGI_FORMAT_R8G8_UINT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"TINTCOLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"SHADE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
}};

constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 3> TRIANGLE_ELEMENTS{{
  {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
  {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
}};

constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 2> DOT_ELEMENTS{{
  {"CENTER", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
}};

constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 2> OVERLAY_QUAD_ELEMENTS{{
  {"PLACEMENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
  {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
}};

} // namespace

std::span<const D3D12_INPUT_ELEMENT_DESC> InputLayoutOf(VertexLayout _layout) noexcept
{
  // In the order of VertexLayout.
  const std::array<std::span<const D3D12_INPUT_ELEMENT_DESC>, 10> layouts{{{},
                                                                           TERRAIN_ELEMENTS,
                                                                           SEA_ELEMENTS,
                                                                           SKY_ELEMENTS,
                                                                           TEXT_ELEMENTS,
                                                                           DOT_ELEMENTS,
                                                                           QUAD_ELEMENTS,
                                                                           MESH_ELEMENTS,
                                                                           TRIANGLE_ELEMENTS,
                                                                           OVERLAY_QUAD_ELEMENTS}};
  const auto index = static_cast<std::size_t>(_layout);
  return index < layouts.size() ? layouts[index] : std::span<const D3D12_INPUT_ELEMENT_DESC>{};
}

std::expected<PipelineCatalog, std::string> PipelineCatalog::Create(ID3D12Device* _device, ID3D12RootSignature* _rootSignature,
                                                                    std::span<const PipelineEntry> _entries, const Formats& _formats)
{
  PipelineCatalog catalog;
  for (const PipelineEntry& entry : _entries)
  {
    const bool scene = entry.target == PipelineTarget::Scene;
    const std::uint32_t samples = scene ? _formats.sceneSamples : 1;
    const std::span<const D3D12_INPUT_ELEMENT_DESC> elements = InputLayoutOf(entry.layout);
    D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{.pRootSignature = _rootSignature,
                                            .VS = {entry.vertexShader.data(), entry.vertexShader.size()},
                                            .PS = {entry.pixelShader.data(), entry.pixelShader.size()},
                                            .BlendState = BlendOf(entry.blend),
                                            .SampleMask = UINT_MAX,
                                            .RasterizerState = RasterizerOf(entry.cull, samples > 1, entry.depthBias, entry.slopeDepthBias),
                                            .DepthStencilState = DepthOf(scene ? entry.depth : DepthState::None),
                                            .InputLayout = {elements.data(), static_cast<UINT>(elements.size())},
                                            .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
                                            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
                                            .NumRenderTargets = 1,
                                            .RTVFormats = {scene ? _formats.scene : _formats.backBuffer},
                                            // RND-059: a scene pass with no depth test keeps the depth format of the bound depth buffer.
                                            .DSVFormat = scene ? _formats.depth : DXGI_FORMAT_UNKNOWN,
                                            .SampleDesc = {samples, 0},
                                            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE};
    Created created{std::string(entry.key), nullptr};
    if (FAILED(_device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&created.state))))
    {
      return std::unexpected(std::string(entry.key));
    }
    created.state->SetName(ToUtf16(std::format("Pipeline {}", entry.key)).c_str());
    catalog.m_entries.push_back(std::move(created));
  }
  Log(LogCategory::Render, LogLevel::Info, "pipeline catalogue: {} entries at {} scene samples", catalog.m_entries.size(),
      _formats.sceneSamples);
  return catalog;
}

ID3D12PipelineState* PipelineCatalog::Find(std::string_view _key) const noexcept
{
  const auto found = std::ranges::find(m_entries, _key, &Created::key);
  return found != m_entries.end() ? found->state.Get() : nullptr;
}

} // namespace Neuron
