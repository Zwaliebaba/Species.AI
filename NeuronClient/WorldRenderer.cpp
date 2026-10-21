#include "pch.h"

#include "DdsTexture.h"
#include "Log.h"
#include "Profile.h"
#include "RootSignature.h"
#include "SkyLayers.h"
#include "WorldRenderer.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace Neuron
{

namespace
{

template <class T> std::span<const std::byte> BytesOf(const std::vector<T>& _values) noexcept
{
  return std::as_bytes(std::span(_values));
}

} // namespace

WorldRenderer::WorldRenderer(DeviceGpuResourceFactory& _factory) noexcept
  : m_factory(_factory)
{
}

WorldRenderer::~WorldRenderer()
{
  Release();
}

void WorldRenderer::Show(const std::shared_ptr<const WorldScene>& _world)
{
  if (_world != m_world)
  {
    Release();
    m_world = _world;
    if (m_world != nullptr)
    {
      Create();
    }
  }
  Pump();
}

void WorldRenderer::UploadAgain()
{
  // The factory kept every id and heap index and holds nothing in them (RND-035): the same surfaces are queued again.
  m_pending = m_uploads;
  m_lastFence = 0;
  Pump();
}

bool WorldRenderer::Ready() const
{
  return m_world != nullptr && !m_failed && m_pending.empty() && m_factory.IsUploadComplete(m_lastFence);
}

void WorldRenderer::Release()
{
  for (GpuResourceId resource : m_resources)
  {
    m_factory.Release(resource);
  }
  m_resources.clear();
  m_chunks.clear();
  m_uploads.clear();
  m_pending.clear();
  m_flatSea = {};
  m_shoreMask = {};
  m_seaTile = {};
  m_terrainGrid = {};
  m_skyCloud = {};
  m_beamShort = {};
  m_lastFence = 0;
  m_failed = false;
  m_world.reset();
}

void WorldRenderer::Create()
{
  const WorldScene& world = *m_world;
  const auto buffer = [this](std::span<const std::byte> _bytes, const char* _name)
  {
    if (_bytes.empty())
    {
      return GpuResourceId{};
    }
    const auto created = m_factory.CreateBuffer({_bytes.size(), 0, false, _name});
    if (!created)
    {
      Log(LogCategory::Render, LogLevel::Error, "the world's {} buffer of {} bytes was not created", _name, _bytes.size());
      m_failed = true;
      return GpuResourceId{};
    }
    m_resources.push_back(*created);
    Schedule(*created, {GpuSubresourceData{_bytes, 0}});
    return *created;
  };
  const auto texture = [this](std::span<const std::byte> _file, const char* _name)
  {
    auto described = DescribeDds(_file, _name);
    if (!described)
    {
      Log(LogCategory::Render, LogLevel::Error, "the world's texture cannot be used: {}", described.error());
      m_failed = true;
      return GpuResourceId{};
    }
    const auto created = m_factory.CreateTexture(described->desc);
    if (!created)
    {
      Log(LogCategory::Render, LogLevel::Error, "the texture {} was not created", _name);
      m_failed = true;
      return GpuResourceId{};
    }
    m_resources.push_back(*created);
    Schedule(*created, std::move(described->levels));
    return *created;
  };

  for (const TerrainChunk& chunk : world.terrain.chunks)
  {
    m_chunks.push_back(buffer(BytesOf(chunk.vertices), "terrain chunk"));
  }
  m_flatSea = buffer(BytesOf(world.sea.flatVertices), "flat sea");
  if (world.sea.maskSize > 0 && world.sea.shoreMask.size() == std::size_t{world.sea.maskSize} * world.sea.maskSize)
  {
    // TER-197: the shore mask as a run-time texture of one level, its grey in the red channel (ADR-115).
    const auto created = m_factory.CreateTexture({world.sea.maskSize, world.sea.maskSize, 1, DXGI_FORMAT_R8_UNORM, "shore mask"});
    if (created)
    {
      m_shoreMask = *created;
      m_resources.push_back(*created);
      Schedule(*created, {GpuSubresourceData{BytesOf(world.sea.shoreMask), world.sea.maskSize}});
    }
    else
    {
      m_failed = true;
    }
  }
  m_seaTile = texture(world.seaTile, "terrain.sea_tile");
  m_terrainGrid = texture(world.terrainGrid, "fx.terrain_grid");
  m_skyCloud = texture(world.skyCloud, "fx.sky_cloud");
  m_beamShort = texture(world.beamShort, "fx.beam_short");
  Log(LogCategory::Render, LogLevel::Info, "world surfaces: {} terrain triangles in {} chunks, {} flat sea tiles, {} wave triangles",
      world.terrain.triangles, world.terrain.chunks.size(), world.sea.flatVertices.size() / 6, world.sea.waveTriangles);
}

void WorldRenderer::Schedule(GpuResourceId _resource, std::vector<GpuSubresourceData> _data)
{
  m_uploads.push_back({_resource, std::move(_data)});
  m_pending.push_back(m_uploads.back());
}

void WorldRenderer::Pump()
{
  // ADR-114: an upload refused for want of staging memory, or during a rebuild, is repeated on a later frame.
  while (!m_pending.empty())
  {
    const Upload& upload = m_pending.front();
    const auto fence = m_factory.ScheduleUpload(upload.resource, upload.data);
    if (!fence)
    {
      if (fence.error() == GpuResourceError::StagingFull || fence.error() == GpuResourceError::DeviceRebuilding)
      {
        return;
      }
      Log(LogCategory::Render, LogLevel::Error, "an upload of the world's surfaces was refused");
      m_failed = true;
    }
    else
    {
      m_lastFence = std::max(m_lastFence, *fence);
    }
    m_pending.erase(m_pending.begin());
  }
}

D3D12_VERTEX_BUFFER_VIEW WorldRenderer::BufferView(GpuResourceId _resource, std::uint64_t _bytes, std::uint32_t _stride) const
{
  ID3D12Resource* resource = m_factory.Resource(_resource);
  return {resource != nullptr ? resource->GetGPUVirtualAddress() : 0, static_cast<UINT>(_bytes), _stride};
}

void WorldRenderer::Draw(ID3D12GraphicsCommandList* _commands, const Vec3& _origin, const Vec3& _camera, const Uint4& _indices) const
{
  const DrawConstants constants{CameraRelativeTransform(_origin, _camera), Float4(1.0f, 1.0f, 1.0f, 1.0f), _indices};
  _commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &constants, 0);
}

void WorldRenderer::RecordSky(const PassContext& _pass)
{
  ID3D12PipelineState* pipeline = _pass.catalog->Find("sky");
  if (!Ready() || pipeline == nullptr)
  {
    return;
  }
  std::vector<SkyLayerInstance> layers;
  {
    ENGINE_PROFILE_SCOPE("render.sky.advance");
    layers = BuildSkyLayers(m_world->look.sky, _pass.frame->camera, _pass.skyTicks, m_factory.HeapIndex(m_beamShort),
                            m_factory.HeapIndex(m_skyCloud));
  }
  const std::uint64_t bytes = layers.size() * sizeof(SkyLayerInstance);
  const UploadAllocation allocation = _pass.ring->Allocate(bytes, sizeof(Float4));
  if (allocation.cpu == nullptr)
  {
    return;
  }
  std::memcpy(allocation.cpu, layers.data(), static_cast<std::size_t>(bytes));
  const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(bytes), sizeof(SkyLayerInstance)};
  _pass.commands->SetPipelineState(pipeline);
  _pass.commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _pass.commands->IASetVertexBuffers(0, 1, &view);
  _pass.commands->DrawInstanced(6, static_cast<UINT>(layers.size()), 0, 0);
}

void WorldRenderer::RecordTerrain(const PassContext& _pass)
{
  m_drawnChunks = 0;
  ID3D12PipelineState* pipeline = _pass.catalog->Find("terrain");
  if (!Ready() || pipeline == nullptr)
  {
    return;
  }
  const WorldLook& look = m_world->look;
  PassConstants constants{};
  constants.values[0] = Float4(look.overlaySpecular[0], look.overlaySpecular[1], look.overlaySpecular[2], look.overlayShininess);
  _pass.commands->SetPipelineState(pipeline);
  _pass.commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _pass.commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _pass.ring->Write(constants));
  // RND-138 in the M1 form (owner decision O75): each chunk's box against the view volume on the CPU, one draw each.
  const Vec3& camera = _pass.frame->camera;
  const std::array<Float4, 6> planes = FrustumPlanes(*_pass.frame);
  const Uint4 indices{m_factory.HeapIndex(m_terrainGrid), 0, 0, 0};
  const std::vector<TerrainChunk>& chunks = m_world->terrain.chunks;
  for (std::size_t c = 0; c < chunks.size() && c < m_chunks.size(); ++c)
  {
    const TerrainChunk& chunk = chunks[c];
    if (!BoxMayBeVisible(planes, chunk.boundsMin - camera, chunk.boundsMax - camera))
    {
      continue;
    }
    const D3D12_VERTEX_BUFFER_VIEW view = BufferView(m_chunks[c], chunk.vertices.size() * sizeof(TerrainVertex), sizeof(TerrainVertex));
    _pass.commands->IASetVertexBuffers(0, 1, &view);
    Draw(_pass.commands, chunk.origin, camera, indices);
    _pass.commands->DrawInstanced(static_cast<UINT>(chunk.vertices.size()), 1, 0, 0);
    ++m_drawnChunks;
  }
}

void WorldRenderer::RecordFlatSea(const PassContext& _pass)
{
  ID3D12PipelineState* pipeline = _pass.catalog->Find("sea_flat");
  if (!Ready() || pipeline == nullptr || m_world->sea.flatVertices.empty())
  {
    return;
  }
  const WorldLook& look = m_world->look;
  const SeaSurface& sea = m_world->sea;
  PassConstants constants{};
  constants.values[0] = Float4(static_cast<float>(look.seaTileRepeats), static_cast<float>(1.0 / std::max(look.seaTileScrollPeriod, 1e-6)),
                               static_cast<float>(1.0 / std::max(sea.planeSide, 1e-6)), 0.0f);
  _pass.commands->SetPipelineState(pipeline);
  _pass.commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _pass.commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _pass.ring->Write(constants));
  const D3D12_VERTEX_BUFFER_VIEW view = BufferView(m_flatSea, sea.flatVertices.size() * sizeof(SeaVertex), sizeof(SeaVertex));
  _pass.commands->IASetVertexBuffers(0, 1, &view);
  Draw(_pass.commands, sea.planeOrigin, _pass.frame->camera, {m_factory.HeapIndex(m_seaTile), m_factory.HeapIndex(m_shoreMask), 0, 0});
  _pass.commands->DrawInstanced(static_cast<UINT>(sea.flatVertices.size()), 1, 0, 0);
}

void WorldRenderer::RecordWaves(const PassContext& _pass)
{
  ID3D12PipelineState* pipeline = _pass.catalog->Find("sea_waves");
  if (!Ready() || pipeline == nullptr || m_world->sea.waveTriangles == 0)
  {
    return;
  }
  // ADR-115: the waves are evaluated on the CPU each frame into the upload ring, in the M1 form of RND-145.
  {
    ENGINE_PROFILE_SCOPE("render.sea.advance");
    EvaluateWaves(m_world->sea, m_world->seaLook, _pass.presentationSeconds, m_waves);
  }
  const std::uint64_t bytes = m_waves.size() * sizeof(SeaVertex);
  const UploadAllocation allocation = _pass.ring->Allocate(bytes, sizeof(SeaVertex));
  if (allocation.cpu == nullptr)
  {
    return;
  }
  std::memcpy(allocation.cpu, m_waves.data(), static_cast<std::size_t>(bytes));
  const D3D12_VERTEX_BUFFER_VIEW view{allocation.gpu, static_cast<UINT>(bytes), sizeof(SeaVertex)};
  _pass.commands->SetPipelineState(pipeline);
  _pass.commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  _pass.commands->IASetVertexBuffers(0, 1, &view);
  Draw(_pass.commands, m_world->sea.planeOrigin, _pass.frame->camera, {});
  _pass.commands->DrawInstanced(static_cast<UINT>(m_waves.size()), 1, 0, 0);
}

} // namespace Neuron
