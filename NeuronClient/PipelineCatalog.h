#pragma once

#include "GraphicsDevice.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

// RND-058, ADR-005: the catalogue's blend states. M1 creates the ones its passes use.
enum class BlendState : std::uint8_t
{
  Opaque,
  Alpha,
  Additive,
  Darkening
};

// RND-059: the depth variants; a pass with depth testing off never writes depth (RND-056).
enum class DepthState : std::uint8_t
{
  TestWrite,
  Test,
  None
};

enum class CullState : std::uint8_t
{
  Back,
  None
};

// The vertex streams the input assembler reads (ADR-006): none, the terrain's corners, the sea's, or the instances of
// the sky, the overlay's glyphs and the development dots.
enum class VertexLayout : std::uint8_t
{
  None,
  Terrain,
  Sea,
  SkyInstance,
  TextGlyph,
  DotInstance,
  QuadInstance,
  MeshInstanced, // a cooked mesh's vertices in slot 0 and MeshInstance in slot 1
  TriangleVertex
};

// The targets an entry draws into: the scene targets at their sample count with the depth buffer bound, or the back
// buffer with no depth.
enum class PipelineTarget : std::uint8_t
{
  Scene,
  BackBuffer
};

/// One catalogue entry (ADR-005, RND-061): its key, its shaders and its differences from the default state of RND-055.
struct PipelineEntry
{
  std::string_view key;
  std::span<const unsigned char> vertexShader;
  std::span<const unsigned char> pixelShader;
  VertexLayout layout = VertexLayout::None;
  BlendState blend = BlendState::Opaque;
  DepthState depth = DepthState::TestWrite;
  CullState cull = CullState::Back;
  PipelineTarget target = PipelineTarget::Scene;
};

/// The pipeline-state catalogue (ADR-005, RND-055, RND-062): every entry a complete pipeline state, built from the
/// default and its differences, all created at start-up and at a reconfiguration, never during a frame. There is no
/// pipeline library at M1 (owner decision O75).
class PipelineCatalog
{
public:
  struct Formats
  {
    DXGI_FORMAT scene = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT depth = DXGI_FORMAT_D32_FLOAT;
    DXGI_FORMAT backBuffer = DXGI_FORMAT_R8G8B8A8_UNORM;
    std::uint32_t sceneSamples = 1;
  };

  // Every entry, or the key of the first that could not be created.
  [[nodiscard]] static std::expected<PipelineCatalog, std::string> Create(ID3D12Device* _device, ID3D12RootSignature* _rootSignature,
                                                                          std::span<const PipelineEntry> _entries, const Formats& _formats);

  // The entry of _key, or null for a key the catalogue does not hold (ADR-004's UnknownCatalogKey).
  [[nodiscard]] ID3D12PipelineState* Find(std::string_view _key) const noexcept;
  [[nodiscard]] std::size_t Size() const noexcept
  {
    return m_entries.size();
  }

private:
  struct Created
  {
    std::string key;
    ComPtr<ID3D12PipelineState> state;
  };

  std::vector<Created> m_entries;
};

// The input layout of each vertex stream; its offsets match TerrainVertex, SeaVertex, SkyLayerInstance, TextGlyphInstance
// and DotInstance.
[[nodiscard]] std::span<const D3D12_INPUT_ELEMENT_DESC> InputLayoutOf(VertexLayout _layout) noexcept;

} // namespace Neuron
