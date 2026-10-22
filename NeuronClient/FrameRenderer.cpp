#include "pch.h"

#include "DepthBias.h"
#include "DescriptorHeaps.h"
#include "DeviceGpuResourceFactory.h"
#include "FatalPath.h"
#include "FrameRenderer.h"
#include "FrameView.h"
#include "GpuDescriptions.h"
#include "GraphicsDevice.h"
#include "Log.h"
#include "CursorRenderer.h"
#include "MeshRenderer.h"
#include "OverlayRenderer.h"
#include "PassTable.h"
#include "PipelineCatalog.h"
#include "PlatformClock.h"
#include "Profile.h"
#include "QuadRenderer.h"
#include "RootSignature.h"
#include "SceneTargets.h"
#include "ShaderLayouts.h"
#include "UploadRing.h"
#include "UiRenderer.h"
#include "WorldRenderer.h"

// The compiled shaders (AGENTS.md §2, ADR-112): this file binds the pipeline states, and no other file includes them.
#include "CompiledShader/ClothPS.h"
#include "CompiledShader/ClothVS.h"
#include "CompiledShader/CursorPS.h"
#include "CompiledShader/CursorVS.h"
#include "CompiledShader/DevelopmentDotPS.h"
#include "CompiledShader/DevelopmentDotVS.h"
#include "CompiledShader/DevelopmentRectPS.h"
#include "CompiledShader/DevelopmentRectVS.h"
#include "CompiledShader/FinalPS.h"
#include "CompiledShader/FinalVS.h"
#include "CompiledShader/MeshLitFogPS.h"
#include "CompiledShader/MeshLitPS.h"
#include "CompiledShader/MeshLitVS.h"
#include "CompiledShader/OverlayQuadAlphaPS.h"
#include "CompiledShader/OverlayQuadPS.h"
#include "CompiledShader/OverlayQuadVS.h"
#include "CompiledShader/SeaFlatPS.h"
#include "CompiledShader/SeaFlatVS.h"
#include "CompiledShader/SeaWavesPS.h"
#include "CompiledShader/SeaWavesVS.h"
#include "CompiledShader/SkyPS.h"
#include "CompiledShader/SkyVS.h"
#include "CompiledShader/TerrainPS.h"
#include "CompiledShader/TerrainVS.h"
#include "CompiledShader/UiGlyphsPS.h"
#include "CompiledShader/UiImagePS.h"
#include "CompiledShader/UiSolidPS.h"
#include "CompiledShader/UiVS.h"
#include "CompiledShader/UnlitTexturedCutoutPS.h"
#include "CompiledShader/UnlitTexturedGlowFogPS.h"
#include "CompiledShader/UnlitTexturedLevelZeroPS.h"
#include "CompiledShader/UnlitTexturedMaskPS.h"
#include "CompiledShader/UnlitTexturedPS.h"
#include "CompiledShader/UnlitTexturedParticleGlowPS.h"
#include "CompiledShader/UnlitTexturedParticleSmokePS.h"
#include "CompiledShader/UnlitTexturedSolidPS.h"
#include "CompiledShader/UnlitTexturedVS.h"
#include "CompiledShader/UnlitVertexColorPS.h"
#include "CompiledShader/UnlitVertexColorVS.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <exception>
#include <format>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace Neuron
{

namespace
{

constexpr UINT BACK_BUFFER_COUNT = 3;                                  // ADR-012
constexpr DXGI_FORMAT BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM; // sRGB values written without conversion (ADR-013)
// ADR-114: the shader-visible heap's persistent region, the transient share of each frame in flight, and the upload
// ring's segment, which holds a frame's constants, sky instances and wave mesh with room to spare.
constexpr std::uint32_t PERSISTENT_DESCRIPTORS = 16'384;
constexpr std::uint32_t TRANSIENT_DESCRIPTORS = 2'048;
constexpr std::uint64_t UPLOAD_SEGMENT_BYTES = 8ull * 1024 * 1024;
// rnd.option.anti_aliasing.default (RND-442, ADR-015): 4x, then 2x, then off; the settings store holds it once it exists.
constexpr std::uint32_t PREFERRED_SCENE_SAMPLES = 4;
// The bounds of the renderer's waits (ADR-092): a fence or the frame-latency object that has not signalled within
// GPU_CHECK_MILLISECONDS is checked for a removed device, and teardown waits at most TEARDOWN_WAIT_MILLISECONDS.
constexpr DWORD GPU_CHECK_MILLISECONDS = 1000;
constexpr DWORD TEARDOWN_WAIT_MILLISECONDS = 2000;

class ScopedHandle
{
public:
  ScopedHandle() noexcept = default;
  explicit ScopedHandle(HANDLE _handle) noexcept
    : m_handle(_handle)
  {
  }
  ScopedHandle(ScopedHandle&& _other) noexcept
    : m_handle(std::exchange(_other.m_handle, nullptr))
  {
  }
  ScopedHandle& operator=(ScopedHandle&& _other) noexcept
  {
    if (this != &_other)
    {
      Close();
      m_handle = std::exchange(_other.m_handle, nullptr);
    }
    return *this;
  }
  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(const ScopedHandle&) = delete;
  ~ScopedHandle()
  {
    Close();
  }

  [[nodiscard]] HANDLE Get() const noexcept
  {
    return m_handle;
  }

private:
  void Close() noexcept
  {
    if (m_handle != nullptr)
    {
      CloseHandle(m_handle);
      m_handle = nullptr;
    }
  }

  HANDLE m_handle = nullptr;
};

} // namespace

// The recorders of the world frame's passes (ADR-113).
enum class WorldPass : std::uint8_t
{
  Clear,
  Sky,
  Terrain,
  FlatSea,
  Waves,
  Structures,
  Actors,
  Workers,
  StructureAlpha,
  Effects,
  Lasers,
  Orbs,
  Routes,
  Particles,
  Dots,
  Resolve,
  Final,
  Markers,
  Ui,
  Cursor,
  OverlayText,
  Present
};

/// Every device object of the renderer, released and created again together by a rebuild (ADR-008). The factory and the
/// world renderer outlive every device (RND-035), and the world renderer ends before the factory.
struct RendererState
{
  AppWindow* window = nullptr;
  const PlatformClock* clock = nullptr;
  RenderConfig config;
  std::unique_ptr<DeviceGpuResourceFactory> factory;
  std::unique_ptr<WorldRenderer> world;
  std::unique_ptr<OverlayRenderer> overlay;
  std::unique_ptr<UiRenderer> ui; // ADR-145, ADR-147: the glyph atlas's pages, the UI's images and its batches
  std::unique_ptr<CursorRenderer> cursor;
  std::unique_ptr<QuadRenderer> quads;
  std::unique_ptr<MeshRenderer> meshes;

  std::unique_ptr<GraphicsDevice> device;
  ComPtr<ID3D12Fence> fence;
  ScopedHandle fenceEvent;
  std::uint64_t fenceValue = 0;
  std::array<std::uint64_t, FRAMES_IN_FLIGHT> frameFenceValues{};
  std::array<ComPtr<ID3D12CommandAllocator>, FRAMES_IN_FLIGHT> allocators;
  ComPtr<ID3D12GraphicsCommandList> commands;
  ComPtr<ID3D12DescriptorHeap> renderTargetHeap;
  UINT renderTargetStride = 0;

  ComPtr<IDXGISwapChain3> swapChain;
  ScopedHandle frameLatencyWaitable;
  UINT swapChainFlags = 0;
  SizePx swapChainSize;
  std::array<ComPtr<ID3D12Resource>, BACK_BUFFER_COUNT> backBuffers;

  std::unique_ptr<ShaderVisibleHeaps> heaps;
  std::unique_ptr<UploadRing> ring;
  ComPtr<ID3D12RootSignature> rootSignature;
  std::unique_ptr<PipelineCatalog> catalog;
  std::optional<SceneTargets> targets;
  std::uint32_t sceneSamples = 1;
  std::uint32_t sceneReadSlot = 0;
  BarrierPlan plan; // the world frame's barriers at the scene's sample count (ADR-004)
  std::vector<WorldPass> passKinds;

  std::uint64_t frameNumber = 0;
  std::uint64_t worldFrames = 0;
  std::deque<Nanoseconds> presentTimes; // R-SIM-576: the frames presented within the counter's window
  bool frameRateWindowFilled = false;
  bool occluded = false;
  bool rebuilding = false;
  std::optional<Nanoseconds> nextFrameStart;
  Nanoseconds lastWaits{}; // ADR-120: the last frame's waits for the latency object, the cap, the fence and the present
  std::vector<std::function<void()>> rebuiltListeners;
};

namespace
{

// Waits until the fence passes _value. A wait that outlasts GPU_CHECK_MILLISECONDS is a fence that may never complete:
// a removed device ends it with false, and anything else is logged and waited for again (ADR-008). _limit bounds the
// whole wait for teardown, after which it gives up with false.
bool WaitForFence(RendererState& _state, std::uint64_t _value, std::optional<DWORD> _limit = std::nullopt)
{
  if (_state.fence == nullptr || _state.fence->GetCompletedValue() >= _value)
  {
    return true;
  }
  if (FAILED(_state.fence->SetEventOnCompletion(_value, _state.fenceEvent.Get())))
  {
    return false;
  }
  DWORD waitedMilliseconds = 0;
  while (WaitForSingleObject(_state.fenceEvent.Get(), GPU_CHECK_MILLISECONDS) == WAIT_TIMEOUT)
  {
    waitedMilliseconds += GPU_CHECK_MILLISECONDS;
    if (_state.device->IsRemoved())
    {
      return false;
    }
    if (_limit.has_value() && waitedMilliseconds >= *_limit)
    {
      Log(LogCategory::Render, LogLevel::Error, "the GPU did not finish within {} ms; teardown goes on without it", waitedMilliseconds);
      return false;
    }
    Log(LogCategory::Render, LogLevel::Warning, "the GPU has not reached fence value {} after {} ms", _value, waitedMilliseconds);
  }
  return true;
}

bool WaitForGpu(RendererState& _state, std::optional<DWORD> _limit = std::nullopt)
{
  if (_state.fence == nullptr || _state.device == nullptr || _state.device->IsRemoved())
  {
    return false;
  }
  const std::uint64_t value = ++_state.fenceValue;
  if (FAILED(_state.device->Queue()->Signal(_state.fence.Get(), value)))
  {
    return false;
  }
  return WaitForFence(_state, value, _limit);
}

void AcquireBackBuffers(RendererState& _state)
{
  D3D12_CPU_DESCRIPTOR_HANDLE target = _state.renderTargetHeap->GetCPUDescriptorHandleForHeapStart();
  for (UINT index = 0; index < BACK_BUFFER_COUNT; ++index)
  {
    if (FAILED(_state.swapChain->GetBuffer(index, IID_PPV_ARGS(&_state.backBuffers[index]))))
    {
      continue;
    }
    _state.backBuffers[index]->SetName(std::format(L"Back buffer {}", index).c_str());
    _state.device->Device()->CreateRenderTargetView(_state.backBuffers[index].Get(), nullptr, target);
    target.ptr += _state.renderTargetStride;
  }
}

bool CreateSwapChain(RendererState& _state)
{
  const SizePx client = _state.window->ClientSize();
  _state.swapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  if (_state.device->TearingSupported())
  {
    _state.swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // only at creation: ResizeBuffers cannot add it
  }
  DXGI_SWAP_CHAIN_DESC1 desc{};
  desc.Width = std::max(client.width, 1u);
  desc.Height = std::max(client.height, 1u);
  desc.Format = BACK_BUFFER_FORMAT;
  desc.SampleDesc.Count = 1; // a flip-model back buffer is never multisampled (ADR-012)
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = BACK_BUFFER_COUNT;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
  desc.Flags = _state.swapChainFlags;

  HWND window = _state.window->Native();
  ComPtr<IDXGISwapChain1> swapChain;
  const HRESULT created =
    _state.device->Factory()->CreateSwapChainForHwnd(_state.device->Queue(), window, &desc, nullptr, nullptr, &swapChain);
  if (FAILED(created) || FAILED(swapChain.As(&_state.swapChain)))
  {
    Log(LogCategory::Render, LogLevel::Error, "the swap chain was not created at {}x{}: {:#010x}", desc.Width, desc.Height,
        static_cast<std::uint32_t>(created));
    return false;
  }
  // Exclusive full screen does not exist (ADR-012), so DXGI never answers Alt+Enter or changes the window itself.
  _state.device->Factory()->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
  _state.swapChain->SetMaximumFrameLatency(MaximumFrameLatency(_state.config.lowLatency));
  _state.frameLatencyWaitable = ScopedHandle(_state.swapChain->GetFrameLatencyWaitableObject());
  _state.swapChainSize = {desc.Width, desc.Height};
  AcquireBackBuffers(_state);
  Log(LogCategory::Render, LogLevel::Info, "swap chain {}x{}, {} buffers, tearing {}", desc.Width, desc.Height, BACK_BUFFER_COUNT,
      _state.device->TearingSupported() ? "allowed" : "not allowed");
  return true;
}

// The world frame's pipeline states (RND-061, ADR-113): the entries its passes use, and the final pass's.
const std::array<PipelineEntry, 40> WORLD_PIPELINES{{
  {"sky", g_SkyVS, g_SkyPS, VertexLayout::SkyInstance, BlendState::Additive, DepthState::None, CullState::Back, PipelineTarget::Scene},
  {"terrain", g_TerrainVS, g_TerrainPS, VertexLayout::Terrain, BlendState::Opaque, DepthState::TestWrite, CullState::Back,
   PipelineTarget::Scene},
  {"sea_flat", g_SeaFlatVS, g_SeaFlatPS, VertexLayout::Sea, BlendState::Opaque, DepthState::Test, CullState::None, PipelineTarget::Scene},
  // RND-061's pass table gives the waves TestWrite, which ADR-113 follows over RND-417's wording.
  {"sea_waves", g_SeaWavesVS, g_SeaWavesPS, VertexLayout::Sea, BlendState::Additive, DepthState::TestWrite, CullState::Back,
   PipelineTarget::Scene},
  {"final", g_FinalVS, g_FinalPS, VertexLayout::None, BlendState::Opaque, DepthState::None, CullState::None, PipelineTarget::BackBuffer},
  // M1 task T15a: the development dots in the scene (DBG-099).
  {"development_dots", g_DevelopmentDotVS, g_DevelopmentDotPS, VertexLayout::DotInstance, BlendState::Opaque, DepthState::Test,
   CullState::None, PipelineTarget::Scene},
  // M1 task T15b: the overlay's filled rectangles under its text (ADR-120), which the UI's glyph pipelines draw (ADR-147).
  {"development_solid", g_DevelopmentRectVS, g_DevelopmentRectPS, VertexLayout::TextGlyph, BlendState::Alpha, DepthState::None,
   CullState::None, PipelineTarget::BackBuffer},
  // M1 task T14: the game cursor's shadow and art (RND-366, ADR-119).
  {"cursor_shadow", g_CursorVS, g_CursorPS, VertexLayout::TextGlyph, BlendState::Darkening, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  {"cursor_art", g_CursorVS, g_CursorPS, VertexLayout::TextGlyph, BlendState::Additive, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  // M1 task T16b (ADR-122): the Leaders' figures, their damage flicker and their cloth banners in pass 7b (RND-308,
  // RND-324, VFX-014).
  {"actor_mesh_lit", g_MeshLitVS, g_MeshLitPS, VertexLayout::MeshInstanced, BlendState::Opaque, DepthState::TestWrite, CullState::Back,
   PipelineTarget::Scene},
  {"actor_flicker_additive", g_MeshLitVS, g_MeshLitPS, VertexLayout::MeshInstanced, BlendState::Additive, DepthState::TestWrite,
   CullState::Back, PipelineTarget::Scene},
  {"cloth_cells", g_ClothVS, g_ClothPS, VertexLayout::TriangleVertex, BlendState::Alpha, DepthState::Test, CullState::None,
   PipelineTarget::Scene},
  {"cloth_pole", g_UnlitTexturedVS, g_UnlitTexturedSolidPS, VertexLayout::QuadInstance, BlendState::Alpha, DepthState::Test,
   CullState::None, PipelineTarget::Scene},
  // M1 task T16 (ADR-121): the Worker cards, their ground shadows and their glows in pass 7d (RND-308), and the Essence
  // orbs in pass 11 (VFX-225).
  {"worker_card", g_UnlitTexturedVS, g_UnlitTexturedCutoutPS, VertexLayout::QuadInstance, BlendState::Alpha, DepthState::TestWrite,
   CullState::None, PipelineTarget::Scene},
  {"worker_ground_shadow", g_UnlitTexturedVS, g_UnlitTexturedCutoutPS, VertexLayout::QuadInstance, BlendState::Alpha, DepthState::Test,
   CullState::None, PipelineTarget::Scene},
  {"worker_glow", g_UnlitTexturedVS, g_UnlitTexturedPS, VertexLayout::QuadInstance, BlendState::Additive, DepthState::Test, CullState::None,
   PipelineTarget::Scene},
  {"orb_additive", g_UnlitTexturedVS, g_UnlitTexturedMaskPS, VertexLayout::QuadInstance, BlendState::Additive, DepthState::Test,
   CullState::None, PipelineTarget::Scene},
  // M1 task T16c (ADR-123): the effects of pass 10a (RND-308, RND-331 to RND-334, VFX-222, VFX-234), the grenade's mesh
  // lit as the actors' are; the laser bolts of pass 10b with the bias variant `bias_laser` (RND-219, RND-338); and the
  // particles' smoke and glow batches of pass 15 (VFX-217).
  {"effect_additive", g_UnlitTexturedVS, g_UnlitTexturedMaskPS, VertexLayout::QuadInstance, BlendState::Additive, DepthState::Test,
   CullState::None, PipelineTarget::Scene},
  {"effect_additive_nodepth", g_UnlitTexturedVS, g_UnlitTexturedMaskPS, VertexLayout::QuadInstance, BlendState::Additive, DepthState::None,
   CullState::None, PipelineTarget::Scene},
  {"effect_gradient_nodepth", g_UnlitVertexColorVS, g_UnlitVertexColorPS, VertexLayout::TriangleVertex, BlendState::Additive,
   DepthState::None, CullState::None, PipelineTarget::Scene},
  {"effect_mesh_lit", g_MeshLitVS, g_MeshLitPS, VertexLayout::MeshInstanced, BlendState::Opaque, DepthState::TestWrite, CullState::Back,
   PipelineTarget::Scene},
  {"laser_bolt", g_UnlitTexturedVS, g_UnlitTexturedLevelZeroPS, VertexLayout::QuadInstance, BlendState::Additive, DepthState::Test,
   CullState::None, PipelineTarget::Scene, DEPTH_BIAS_LASER},
  {"particle_darkening", g_UnlitTexturedVS, g_UnlitTexturedParticleSmokePS, VertexLayout::QuadInstance, BlendState::Darkening,
   DepthState::Test, CullState::None, PipelineTarget::Scene},
  {"particle_additive", g_UnlitTexturedVS, g_UnlitTexturedParticleGlowPS, VertexLayout::QuadInstance, BlendState::Additive,
   DepthState::Test, CullState::None, PipelineTarget::Scene},
  // M1 task T17 (ADR-125): the flag pointer over a Waypoint banner in pass 7b with the bias variant `bias_flag_pointer`
  // (VFX-141); the route preview's spots and arrows in pass 12 (RND-353, RND-355); and the overlay's masks of the world
  // markers in pass 16 and the cursor's badge in pass 20 (RND-357, R-CTRL-366).
  {"cloth_pointer", g_UnlitTexturedVS, g_UnlitTexturedMaskPS, VertexLayout::QuadInstance, BlendState::Alpha, DepthState::Test,
   CullState::None, PipelineTarget::Scene, DEPTH_BIAS_FLAG_POINTER, SLOPE_DEPTH_BIAS_FLAG_POINTER},
  {"route_line", g_UnlitTexturedVS, g_UnlitTexturedParticleGlowPS, VertexLayout::QuadInstance, BlendState::Alpha, DepthState::None,
   CullState::None, PipelineTarget::Scene},
  {"route_arrow", g_UnlitTexturedVS, g_UnlitTexturedMaskPS, VertexLayout::QuadInstance, BlendState::Alpha, DepthState::None,
   CullState::None, PipelineTarget::Scene},
  {"overlay_alpha", g_OverlayQuadVS, g_OverlayQuadAlphaPS, VertexLayout::OverlayQuadInstance, BlendState::Alpha, DepthState::None,
   CullState::None, PipelineTarget::BackBuffer},
  {"overlay_additive", g_OverlayQuadVS, g_OverlayQuadPS, VertexLayout::OverlayQuadInstance, BlendState::Additive, DepthState::None,
   CullState::None, PipelineTarget::BackBuffer},
  // M2 task T12 (ADR-140): the structures' meshes of pass 6 (RND-163), and in pass 8 the port pads (RND-168) and the
  // additive glows of the stars, the lights, the Wellspring and the completion flash (RND-167), each with level fog.
  {"mesh_lit_fog", g_MeshLitVS, g_MeshLitFogPS, VertexLayout::MeshInstanced, BlendState::Opaque, DepthState::TestWrite, CullState::Back,
   PipelineTarget::Scene},
  {"port_pad", g_MeshLitVS, g_MeshLitFogPS, VertexLayout::MeshInstanced, BlendState::Opaque, DepthState::Test, CullState::None,
   PipelineTarget::Scene},
  {"glow_additive_fog", g_UnlitTexturedVS, g_UnlitTexturedGlowFogPS, VertexLayout::QuadInstance, BlendState::Additive, DepthState::Test,
   CullState::None, PipelineTarget::Scene},
  // M2 task T14 (ADR-145): the UI's quads over the back buffer in pass 19 and with the cursor in pass 20, by what they
  // read and how they blend (UI-180, UI-184); the UI never tests or writes depth (UI-183).
  {"ui_solid_alpha", g_UiVS, g_UiSolidPS, VertexLayout::TextGlyph, BlendState::Alpha, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  {"ui_solid_additive", g_UiVS, g_UiSolidPS, VertexLayout::TextGlyph, BlendState::Additive, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  {"ui_glyphs_alpha", g_UiVS, g_UiGlyphsPS, VertexLayout::TextGlyph, BlendState::Alpha, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  {"ui_glyphs_additive", g_UiVS, g_UiGlyphsPS, VertexLayout::TextGlyph, BlendState::Additive, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  {"ui_glyphs_ink", g_UiVS, g_UiGlyphsPS, VertexLayout::TextGlyph, BlendState::Ink, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  {"ui_image_alpha", g_UiVS, g_UiImagePS, VertexLayout::TextGlyph, BlendState::Alpha, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
  {"ui_image_additive", g_UiVS, g_UiImagePS, VertexLayout::TextGlyph, BlendState::Additive, DepthState::None, CullState::None,
   PipelineTarget::BackBuffer},
}};

// The world frame's targets, at these indices of its table (RND-063, ADR-113).
constexpr std::uint32_t TARGET_SCENE_COLOR = 0;
constexpr std::uint32_t TARGET_SCENE_DEPTH = 1;
constexpr std::uint32_t TARGET_SCENE_RESOLVED = 2;
constexpr std::uint32_t TARGET_BACK_BUFFER = 3;

/// The ordered pass table of the world frame (RND-067 amendment, RND-070), in engine terms, with the recorder of each
/// pass and the catalogue entry it draws with, if any.
struct WorldFrameTable
{
  std::vector<TargetDecl> targets;
  std::vector<PassDecl> passes;
  std::vector<WorldPass> kinds;
  std::vector<std::string_view> catalogKeys;
};

WorldFrameTable WorldFrame(bool _multisampled)
{
  WorldFrameTable table;
  const TargetAccess sceneAtRest = _multisampled ? TargetAccess::ResolveSource : TargetAccess::ShaderRead;
  table.targets = {{"scene_colour", sceneAtRest, sceneAtRest},
                   {"scene_depth", TargetAccess::DepthWrite, TargetAccess::DepthWrite},
                   {"scene_resolved", TargetAccess::ShaderRead, TargetAccess::ShaderRead},
                   {"back_buffer", TargetAccess::Present, TargetAccess::Present}};
  const std::vector<TargetUse> scene{{TARGET_SCENE_COLOR, TargetAccess::RenderTarget}, {TARGET_SCENE_DEPTH, TargetAccess::DepthWrite}};
  const auto add = [&table](WorldPass _kind, std::string_view _name, std::string_view _key, std::vector<TargetUse> _uses)
  {
    table.passes.push_back({_name, std::move(_uses)});
    table.kinds.push_back(_kind);
    table.catalogKeys.push_back(_key);
  };
  add(WorldPass::Clear, "pass_02_clear", {}, scene);
  add(WorldPass::Sky, "pass_03_sky", "sky", scene);
  add(WorldPass::Terrain, "pass_04_terrain", "terrain", scene);
  add(WorldPass::FlatSea, "pass_05a_flat_sea", "sea_flat", scene);
  add(WorldPass::Waves, "pass_05b_coastline_waves", "sea_waves", scene);
  add(WorldPass::Structures, "pass_06_structures_solid", "mesh_lit_fog", scene);
  add(WorldPass::Actors, "pass_07b_other_actors", "actor_mesh_lit", scene);
  add(WorldPass::Workers, "pass_07d_workers", "worker_card", scene);
  add(WorldPass::StructureAlpha, "pass_08_structures_alpha", "glow_additive_fog", scene);
  add(WorldPass::Effects, "pass_10a_effects", "effect_additive", scene);
  add(WorldPass::Lasers, "pass_10b_laser_bolts", "laser_bolt", scene);
  add(WorldPass::Orbs, "pass_11_essence_orbs", "orb_additive", scene);
  add(WorldPass::Routes, "pass_12_route_lines", "route_line", scene);
  add(WorldPass::Particles, "pass_15_particles", "particle_additive", scene);
#if ENGINE_DEVELOPMENT
  add(WorldPass::Dots, "pass_24_development_drawing", "development_dots", scene); // RND-076: Debug builds only
#endif
  if (_multisampled)
  {
    add(WorldPass::Resolve, "resolve", {},
        {{TARGET_SCENE_COLOR, TargetAccess::ResolveSource}, {TARGET_SCENE_RESOLVED, TargetAccess::ResolveDestination}});
  }
  add(WorldPass::Final, "final", "final",
      {{_multisampled ? TARGET_SCENE_RESOLVED : TARGET_SCENE_COLOR, TargetAccess::ShaderRead},
       {TARGET_BACK_BUFFER, TargetAccess::RenderTarget}});
  add(WorldPass::Markers, "pass_16_world_markers", "overlay_additive", {{TARGET_BACK_BUFFER, TargetAccess::RenderTarget}});
  add(WorldPass::Ui, "pass_19_ui", "ui_glyphs_additive", {{TARGET_BACK_BUFFER, TargetAccess::RenderTarget}});
  add(WorldPass::Cursor, "pass_20_world_cursor", "cursor_art", {{TARGET_BACK_BUFFER, TargetAccess::RenderTarget}});
  add(WorldPass::OverlayText, "pass_29_text_overlays", "development_solid", {{TARGET_BACK_BUFFER, TargetAccess::RenderTarget}});
  add(WorldPass::Present, "pass_34_present", {}, {{TARGET_BACK_BUFFER, TargetAccess::Present}});
  return table;
}

// ADR-007's resource-state path: the state of each engine access.
D3D12_RESOURCE_STATES StateOf(TargetAccess _access) noexcept
{
  switch (_access)
  {
  case TargetAccess::RenderTarget:
    return D3D12_RESOURCE_STATE_RENDER_TARGET;
  case TargetAccess::DepthWrite:
    return D3D12_RESOURCE_STATE_DEPTH_WRITE;
  case TargetAccess::DepthRead:
    return D3D12_RESOURCE_STATE_DEPTH_READ;
  case TargetAccess::ShaderRead:
    return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
  case TargetAccess::ResolveSource:
    return D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
  case TargetAccess::ResolveDestination:
    return D3D12_RESOURCE_STATE_RESOLVE_DEST;
  case TargetAccess::CopySource:
    return D3D12_RESOURCE_STATE_COPY_SOURCE;
  case TargetAccess::CopyDestination:
    return D3D12_RESOURCE_STATE_COPY_DEST;
  case TargetAccess::Present:
    break;
  }
  return D3D12_RESOURCE_STATE_PRESENT;
}

void Transition(ID3D12GraphicsCommandList* _commands, std::span<const TargetTransition> _transitions,
                const std::array<ID3D12Resource*, 4>& _resources)
{
  std::array<D3D12_RESOURCE_BARRIER, 4> barriers{};
  UINT count = 0;
  for (const TargetTransition& transition : _transitions)
  {
    if (transition.target < _resources.size() && _resources[transition.target] != nullptr && count < barriers.size())
    {
      barriers[count++] = TransitionBarrier(_resources[transition.target], StateOf(transition.before), StateOf(transition.after));
    }
  }
  if (count > 0)
  {
    _commands->ResourceBarrier(count, barriers.data());
  }
}

// RND-030: the scene targets at the swap chain's size, once the GPU has finished with the last ones.
bool CreateSceneTargets(RendererState& _state)
{
  _state.targets.reset();
  _state.targets = SceneTargets::Create(_state.device->Device(), _state.swapChainSize, _state.sceneSamples, {0.0f, 0.0f, 0.0f, 1.0f},
                                        *_state.heaps, _state.sceneReadSlot);
  return _state.targets.has_value();
}

// The frame infrastructure on a new device (RND-020): the heaps, the factory, the ring, the root signature, the
// catalogue, the world frame's barriers and its scene targets.
std::expected<void, StartupFailure> CreateFrameObjects(RendererState& _state)
{
  ID3D12Device* d3d = _state.device->Device();
  const std::string& adapter = _state.device->AdapterName();
  _state.heaps = ShaderVisibleHeaps::Create(d3d, PERSISTENT_DESCRIPTORS, TRANSIENT_DESCRIPTORS);
  if (_state.heaps == nullptr)
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "the shader-visible descriptor heaps"});
  }
  if (!_state.factory->Attach(d3d, _state.heaps.get()))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a copy queue"});
  }
  std::optional<UploadRing> ring = UploadRing::Create(d3d, UPLOAD_SEGMENT_BYTES);
  if (!ring)
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "the upload ring"});
  }
  _state.ring = std::make_unique<UploadRing>(std::move(*ring));
  _state.rootSignature = CreateGraphicsRootSignature(d3d);
  if (_state.rootSignature == nullptr)
  {
    return std::unexpected(StartupFailure{StartupError::PipelineCreationFailed, adapter, "the root signature"});
  }
  _state.sceneSamples = SupportedSceneSamples(d3d, PREFERRED_SCENE_SAMPLES);
  const PipelineCatalog::Formats formats{SCENE_COLOR_FORMAT, SCENE_DEPTH_FORMAT, BACK_BUFFER_FORMAT, _state.sceneSamples};
  std::expected<PipelineCatalog, std::string> catalog = PipelineCatalog::Create(d3d, _state.rootSignature.Get(), WORLD_PIPELINES, formats);
  if (!catalog)
  {
    return std::unexpected(StartupFailure{StartupError::PipelineCreationFailed, adapter, catalog.error()});
  }
  _state.catalog = std::make_unique<PipelineCatalog>(std::move(*catalog));
  // ADR-004: the table is validated and its barriers derived once, and every pass's catalogue key exists.
  WorldFrameTable table = WorldFrame(_state.sceneSamples > 1);
  std::expected<BarrierPlan, PassTableError> plan = PlanBarriers(table.targets, table.passes);
  if (!plan)
  {
    Log(LogCategory::Render, LogLevel::Error, "the world frame's pass table is invalid at pass {} and target {}", plan.error().pass,
        plan.error().target);
    return std::unexpected(StartupFailure{StartupError::FrameGraphInvalid, adapter, plan.error().pass});
  }
  for (std::size_t p = 0; p < table.passes.size(); ++p)
  {
    if (!table.catalogKeys[p].empty() && _state.catalog->Find(table.catalogKeys[p]) == nullptr)
    {
      return std::unexpected(StartupFailure{StartupError::FrameGraphInvalid, adapter, std::string(table.passes[p].name)});
    }
  }
  _state.plan = std::move(*plan);
  _state.passKinds = std::move(table.kinds);
  const std::optional<std::uint32_t> readSlot = _state.heaps->AllocatePersistent();
  if (!readSlot)
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a descriptor of the scene"});
  }
  _state.sceneReadSlot = *readSlot;
  if (!CreateSceneTargets(_state))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "the scene targets"});
  }
  Log(LogCategory::Render, LogLevel::Info, "world frame: {} passes, {} scene samples, resource-state barriers", table.passes.size(),
      _state.sceneSamples);
  return {};
}

std::expected<void, StartupFailure> CreateDeviceObjects(RendererState& _state)
{
  std::expected<GraphicsDevice, StartupFailure> device = GraphicsDevice::Create(_state.config);
  if (!device.has_value())
  {
    return std::unexpected(std::move(device.error()));
  }
  _state.device = std::make_unique<GraphicsDevice>(std::move(*device));
  ID3D12Device* d3d = _state.device->Device();
  const std::string& adapter = _state.device->AdapterName();

  ScopedHandle fenceEvent(CreateEventW(nullptr, FALSE, FALSE, nullptr));
  if (fenceEvent.Get() == nullptr || FAILED(d3d->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_state.fence))))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a fence"});
  }
  _state.fenceEvent = std::move(fenceEvent);
  _state.fenceValue = 0;
  _state.frameFenceValues = {};
  for (ComPtr<ID3D12CommandAllocator>& allocator : _state.allocators)
  {
    if (FAILED(d3d->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator))))
    {
      return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a command allocator"});
    }
  }
  if (FAILED(
        d3d->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _state.allocators[0].Get(), nullptr, IID_PPV_ARGS(&_state.commands))))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a command list"});
  }
  _state.commands->SetName(L"Frame commands");
  _state.commands->Close();

  D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};
  heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
  heapDesc.NumDescriptors = BACK_BUFFER_COUNT;
  if (FAILED(d3d->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_state.renderTargetHeap))))
  {
    return std::unexpected(StartupFailure{StartupError::DeviceCreationFailed, adapter, "a render-target descriptor heap"});
  }
  _state.renderTargetStride = d3d->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

  if (!CreateSwapChain(_state))
  {
    return std::unexpected(StartupFailure{StartupError::SwapChainFailed, adapter, {}});
  }
  _state.occluded = false;
  return CreateFrameObjects(_state);
}

// Releases every device object, the swap chain before the device, after the GPU has finished what it can (ADR-008).
void ReleaseDeviceObjects(RendererState& _state, std::optional<DWORD> _limit)
{
  if (_state.device != nullptr)
  {
    WaitForGpu(_state, _limit);
  }
  _state.targets.reset();
  _state.catalog.reset();
  _state.rootSignature.Reset();
  _state.ring.reset();
  if (_state.factory != nullptr)
  {
    _state.factory->Detach();
  }
  _state.heaps.reset();
  for (ComPtr<ID3D12Resource>& buffer : _state.backBuffers)
  {
    buffer.Reset();
  }
  _state.frameLatencyWaitable = ScopedHandle();
  _state.swapChain.Reset();
  _state.renderTargetHeap.Reset();
  _state.commands.Reset();
  for (ComPtr<ID3D12CommandAllocator>& allocator : _state.allocators)
  {
    allocator.Reset();
  }
  _state.fence.Reset();
  _state.fenceEvent = ScopedHandle();
  _state.device.reset();
}

// ADR-008 and RND-034: the removal is logged, every device object is released and created again, presentation
// resumes and DeviceRebuilt is raised. A removal during the rebuild, or a recreation that fails, ends the client with a
// report.
void Rebuild(RendererState& _state)
{
  if (_state.rebuilding)
  {
    EnterFatalPath("the device was removed again during its rebuild");
  }
  _state.rebuilding = true;
  _state.device->LogRemoval();
  ReleaseDeviceObjects(_state, TEARDOWN_WAIT_MILLISECONDS);
  if (const std::expected<void, StartupFailure> created = CreateDeviceObjects(_state); !created.has_value())
  {
    EnterFatalPath(
      std::format("the device could not be rebuilt after its removal: {} {}", created.error().adapter, created.error().capability));
  }
  _state.rebuilding = false;
  _state.world->UploadAgain(); // RND-034: the same surfaces, from the unchanged world
  _state.ui->UploadAgain();
  _state.cursor->UploadAgain();
  _state.quads->UploadAgain();
  _state.meshes->UploadAgain();
  Log(LogCategory::Render, LogLevel::Warning, "the device was rebuilt after its removal");
  for (const std::function<void()>& listener : _state.rebuiltListeners)
  {
    listener();
  }
}

// RND-030 as M0 has it: wait for the GPU, release the back buffers, resize them with the creation flags, take them again.
void ApplyResize(RendererState& _state, SizePx _client)
{
  WaitForGpu(_state);
  for (ComPtr<ID3D12Resource>& buffer : _state.backBuffers)
  {
    buffer.Reset();
  }
  const HRESULT resized =
    _state.swapChain->ResizeBuffers(BACK_BUFFER_COUNT, _client.width, _client.height, DXGI_FORMAT_UNKNOWN, _state.swapChainFlags);
  if (IsDeviceRemoval(resized))
  {
    Rebuild(_state);
    return;
  }
  if (FAILED(resized))
  {
    Log(LogCategory::Render, LogLevel::Error, "the swap chain was not resized to {}x{}: {:#010x}", _client.width, _client.height,
        static_cast<std::uint32_t>(resized));
  }
  else
  {
    _state.swapChainSize = _client;
  }
  AcquireBackBuffers(_state);
  if (!CreateSceneTargets(_state))
  {
    Log(LogCategory::Render, LogLevel::Error, "the scene targets were not created at {}x{}", _state.swapChainSize.width,
        _state.swapChainSize.height);
  }
}

// RND-057: the per-frame constants, written once before any pass records.
FrameConstants MakeFrameConstants(const FrameView& _frame, const CameraPose& _camera, const WorldLook& _look, double _seconds, SizePx _size)
{
  const auto unit = [](const Vec3& _v)
  {
    const double length = std::sqrt(LengthSquared(_v));
    const Vec3 v = length > 0.0 ? _v * (1.0 / length) : Vec3{0.0, 0.0, -1.0}; // rnd.lighting.unused_light_direction
    return Float4(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z), 0.0f);
  };
  FrameConstants constants{};
  constants.view = _frame.view;
  constants.projection = _frame.projection;
  constants.viewProjection = _frame.viewProjection;
  constants.cameraFront = unit(ViewDirection(_camera));
  constants.fogColor = Float4(_look.fogColor[0], _look.fogColor[1], _look.fogColor[2], 1.0f);
  const double fogSpan = std::max(_look.fogEnd - _look.fogStart, 1e-3);
  constants.fogRange =
    Float4(static_cast<float>(_look.fogStart), static_cast<float>(_look.fogEnd), static_cast<float>(1.0 / fogSpan), 0.0f);
  constants.keyTowards = unit(_look.keyTowards);
  constants.keyColor = Float4(_look.keyColor[0], _look.keyColor[1], _look.keyColor[2], 1.0f);
  constants.fillTowards = unit(_look.fillTowards);
  constants.fillColor = Float4(_look.fillColor[0], _look.fillColor[1], _look.fillColor[2], 1.0f);
  constants.time = Float4(static_cast<float>(_seconds), 0.0f, 0.0f, 0.0f);
  const auto width = static_cast<float>(std::max(_size.width, 1u));
  const auto height = static_cast<float>(std::max(_size.height, 1u));
  constants.viewport = Float4(width, height, 1.0f / width, 1.0f / height);
  return constants;
}

// The world frame of ADR-113: the passes of its table in order, each after the barriers the table derived for it.
void RecordWorldFrame(RendererState& _state, const FrameInputs& _inputs, ID3D12Resource* _backBuffer,
                      D3D12_CPU_DESCRIPTOR_HANDLE _backBufferView)
{
  ID3D12GraphicsCommandList* commands = _state.commands.Get();
  const SceneTargets& targets = *_state.targets;
  const WorldLook& look = _inputs.world->look;
  const SizePx size = targets.Size();
  FrameView frame;
  {
    ENGINE_PROFILE_SCOPE("render.camera");
    frame = MakeFrameView(_inputs.camera, static_cast<double>(size.width) / static_cast<double>(size.height), look.planes);
  }
  {
    ENGINE_PROFILE_SCOPE("render.frame_constants");
    FrameConstants constants = MakeFrameConstants(frame, _inputs.camera, look, _inputs.presentationSeconds, size);
    _state.world->MaskConstants(frame.camera, constants.viewMask, constants.viewMaskIndex); // ADR-144
    commands->SetGraphicsRootConstantBufferView(ROOT_FRAME_CONSTANTS, _state.ring->Write(constants));
    // Every root parameter holds a value before the first draw; the passes replace them as they need.
    commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _state.ring->Write(PassConstants{}));
    const DrawConstants none{};
    commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &none, 0);
  }
  const std::array<ID3D12Resource*, 4> resources{targets.Color(), targets.Depth(), targets.Resolved(), _backBuffer};
  const D3D12_CPU_DESCRIPTOR_HANDLE colorView = targets.ColorView();
  const D3D12_CPU_DESCRIPTOR_HANDLE depthView = targets.DepthView();
  const D3D12_VIEWPORT sceneViewport{0.0f, 0.0f, static_cast<float>(size.width), static_cast<float>(size.height), 0.0f, 1.0f};
  const D3D12_RECT sceneScissor{0, 0, static_cast<LONG>(size.width), static_cast<LONG>(size.height)};
  const WorldRenderer::PassContext pass{commands, &*_state.ring, &*_state.catalog, &frame, _inputs.presentationSeconds, _inputs.skyTicks};
  for (std::size_t p = 0; p < _state.passKinds.size() && p < _state.plan.before.size(); ++p)
  {
    Transition(commands, _state.plan.before[p], resources);
    switch (_state.passKinds[p])
    {
    case WorldPass::Clear:
    {
      ENGINE_PROFILE_SCOPE("render.clear");
      const std::array<float, 4> clear{look.fogColor[0], look.fogColor[1], look.fogColor[2], 1.0f};
      commands->ClearRenderTargetView(colorView, clear.data(), 0, nullptr);
      commands->ClearDepthStencilView(depthView, D3D12_CLEAR_FLAG_DEPTH, SCENE_DEPTH_CLEAR, 0, 0, nullptr);
      commands->OMSetRenderTargets(1, &colorView, FALSE, &depthView);
      commands->RSSetViewports(1, &sceneViewport);
      commands->RSSetScissorRects(1, &sceneScissor);
      break;
    }
    case WorldPass::Sky:
    {
      ENGINE_PROFILE_SCOPE("render.sky");
      _state.world->RecordSky(pass);
      break;
    }
    case WorldPass::Terrain:
    {
      ENGINE_PROFILE_SCOPE("render.terrain.surface");
      _state.world->RecordTerrain(pass);
      break;
    }
    case WorldPass::FlatSea:
    {
      ENGINE_PROFILE_SCOPE("render.sea");
      _state.world->RecordFlatSea(pass);
      break;
    }
    case WorldPass::Waves:
    {
      ENGINE_PROFILE_SCOPE("render.sea.wave_build");
      _state.world->RecordWaves(pass);
      break;
    }
    case WorldPass::Structures:
    {
      // RND-163 (ADR-140): every structure's solid parts, the turret's head and barrels with them.
      ENGINE_PROFILE_SCOPE("render.structures");
      _state.meshes->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Structures, _inputs.meshBatches);
      break;
    }
    case WorldPass::Actors:
    {
      // RND-324: the Leaders' figures, then their poles and cloth (VFX-141), and the flag pointers over the cloth.
      ENGINE_PROFILE_SCOPE("render.ungrouped_actors");
      _state.meshes->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Actors, _inputs.meshBatches);
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Actors, _inputs.quadBatches);
      _state.quads->RecordTriangles(commands, *_state.ring, *_state.catalog, frame, ScenePass::Actors, _inputs.triangleBatches);
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Pointers, _inputs.quadBatches);
      break;
    }
    case WorldPass::Workers:
    {
      ENGINE_PROFILE_SCOPE("render.workers");
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Workers, _inputs.quadBatches);
      break;
    }
    case WorldPass::StructureAlpha:
    {
      // RND-164, RND-165: each structure's lights, then its ports' pads and stars, and the structures' glows.
      ENGINE_PROFILE_SCOPE("render.structures.transparent");
      _state.meshes->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::StructureAlpha, _inputs.meshBatches);
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::StructureAlpha, _inputs.quadBatches);
      break;
    }
    case WorldPass::Effects:
    {
      // RND-331: the grenades' meshes, which write depth, then the additive quads, and last the rings' discs and the
      // full-screen flash (VFX-234).
      ENGINE_PROFILE_SCOPE("render.weapons"); // the DBG-045 marker of the weapons' effects
      _state.meshes->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Effects, _inputs.meshBatches);
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Effects, _inputs.quadBatches);
      _state.quads->RecordTriangles(commands, *_state.ring, *_state.catalog, frame, ScenePass::Effects, _inputs.triangleBatches);
      break;
    }
    case WorldPass::Lasers:
    {
      ENGINE_PROFILE_SCOPE("render.weapons");
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Lasers, _inputs.quadBatches);
      break;
    }
    case WorldPass::Orbs:
    {
      ENGINE_PROFILE_SCOPE("render.essence_orbs");
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Orbs, _inputs.quadBatches);
      break;
    }
    case WorldPass::Routes:
    {
      // RND-353: the route preview's spots, then its arrows.
      ENGINE_PROFILE_SCOPE("render.route_lines");
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Routes, _inputs.quadBatches);
      break;
    }
    case WorldPass::Particles:
    {
      // VFX-217: the smoke batch, then the glow batch, in the order the game wrote them.
      ENGINE_PROFILE_SCOPE("effects.particles.render");
      _state.quads->Record(commands, *_state.ring, *_state.catalog, frame, ScenePass::Particles, _inputs.quadBatches);
      break;
    }
    case WorldPass::Dots:
    {
      _state.overlay->RecordDots(commands, *_state.ring, *_state.catalog, frame, _state.window->DpiScale(), _inputs.dots);
      break;
    }
    case WorldPass::Resolve:
    {
      ENGINE_PROFILE_SCOPE("render.resolve");
      commands->ResolveSubresource(targets.Resolved(), 0, targets.Color(), 0, SCENE_COLOR_FORMAT);
      break;
    }
    case WorldPass::Final:
    {
      ENGINE_PROFILE_SCOPE("render.final");
      const D3D12_VIEWPORT viewport{
        0.0f, 0.0f, static_cast<float>(_state.swapChainSize.width), static_cast<float>(_state.swapChainSize.height), 0.0f, 1.0f};
      const D3D12_RECT scissor{0, 0, static_cast<LONG>(_state.swapChainSize.width), static_cast<LONG>(_state.swapChainSize.height)};
      commands->OMSetRenderTargets(1, &_backBufferView, FALSE, nullptr);
      commands->RSSetViewports(1, &viewport);
      commands->RSSetScissorRects(1, &scissor);
      commands->SetPipelineState(_state.catalog->Find("final"));
      PassConstants brightness{};
      brightness.values[0] = Float4(_inputs.brightness, 0.0f, 0.0f, 0.0f);
      commands->SetGraphicsRootConstantBufferView(ROOT_PASS_CONSTANTS, _state.ring->Write(brightness));
      const DrawConstants read{Float3x4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
                               Float4(1.0f, 1.0f, 1.0f, 1.0f), Uint4{targets.ReadSlot(), 0, 0, 0}};
      commands->SetGraphicsRoot32BitConstants(ROOT_DRAW_CONSTANTS, DRAW_CONSTANT_DWORDS, &read, 0);
      commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
      commands->DrawInstanced(3, 1, 0, 0);
      break;
    }
    case WorldPass::Markers:
    {
      // RND-356: in the overlay's pixels, nearest last.
      ENGINE_PROFILE_SCOPE("render.world_markers");
      _state.quads->RecordOverlay(commands, *_state.ring, *_state.catalog, _state.swapChainSize, _inputs.markers);
      break;
    }
    case WorldPass::Ui:
    {
      // UI-008, UI-175 (ADR-145): the UI over the final image and the world's markers.
      ENGINE_PROFILE_SCOPE("render.ui");
      _state.ui->Record(commands, *_state.ring, *_state.catalog, _state.swapChainSize, _inputs.ui);
      break;
    }
    case WorldPass::Cursor:
    {
      // R-CTRL-366: the badge near the pointer, its count in the UI's batches (ADR-145), then the pointer over them.
      ENGINE_PROFILE_SCOPE("render.world_cursor");
      _state.quads->RecordOverlay(commands, *_state.ring, *_state.catalog, _state.swapChainSize, _inputs.cursorBadge);
      _state.ui->Record(commands, *_state.ring, *_state.catalog, _state.swapChainSize, _inputs.uiCursor);
      if (_inputs.cursor)
      {
        _state.cursor->Record(commands, *_state.ring, *_state.catalog, _state.swapChainSize, *_inputs.cursor);
      }
      break;
    }
    case WorldPass::OverlayText:
    {
      // DBG-095, ADR-120: the overlay's rectangles, then its text over them in the UI's glyph pipelines (ADR-147).
      ENGINE_PROFILE_SCOPE("render.development_text");
      _state.overlay->RecordRects(commands, *_state.ring, *_state.catalog, _state.swapChainSize, _state.window->DpiScale(),
                                  _inputs.overlayRects);
      _state.ui->Record(commands, *_state.ring, *_state.catalog, _state.swapChainSize, _inputs.overlayText);
      break;
    }
    case WorldPass::Present:
      break;
    }
  }
  Transition(commands, _state.plan.after, resources);
}

} // namespace

std::expected<std::unique_ptr<FrameRenderer>, StartupFailure> FrameRenderer::Create(AppWindow& _window, const PlatformClock& _clock,
                                                                                    const RenderConfig& _config)
{
  auto state = std::make_unique<RendererState>();
  state->window = &_window;
  state->clock = &_clock;
  state->config = _config;
  state->factory = std::make_unique<DeviceGpuResourceFactory>(DeviceGpuResourceFactory::Desc{});
  state->world = std::make_unique<WorldRenderer>(*state->factory);
  state->overlay = std::make_unique<OverlayRenderer>();
  state->ui = std::make_unique<UiRenderer>(*state->factory);
  state->cursor = std::make_unique<CursorRenderer>(*state->factory);
  state->quads = std::make_unique<QuadRenderer>(*state->factory);
  state->meshes = std::make_unique<MeshRenderer>(*state->factory);
  if (std::expected<void, StartupFailure> created = CreateDeviceObjects(*state); !created.has_value())
  {
    ReleaseDeviceObjects(*state, TEARDOWN_WAIT_MILLISECONDS);
    return std::unexpected(std::move(created.error()));
  }
  return std::unique_ptr<FrameRenderer>(new FrameRenderer(std::move(state)));
}

FrameRenderer::FrameRenderer(std::unique_ptr<RendererState> _state) noexcept
  : m_state(std::move(_state))
{
}

FrameRenderer::~FrameRenderer()
{
  try
  {
    ReleaseDeviceObjects(*m_state, TEARDOWN_WAIT_MILLISECONDS);
  }
  catch (...)
  {
    std::terminate(); // a teardown that cannot log is a defect, and it ends through the fatal path
  }
}

void FrameRenderer::Reconfigure(const RenderConfig& _config)
{
  RendererState& state = *m_state;
  if (_config.lowLatency != state.config.lowLatency)
  {
    state.swapChain->SetMaximumFrameLatency(MaximumFrameLatency(_config.lowLatency));
  }
  state.config = _config;
  state.nextFrameStart.reset();
}

FrameOutcome FrameRenderer::RenderFrame(const FrameInputs& _inputs)
{
  ENGINE_PROFILE_SCOPE("render.frame"); // the markers of the DBG-045 catalogue
  RendererState& state = *m_state;
  if (state.window->IsMinimized())
  {
    return FrameOutcome::Suspended;
  }
  const SizePx client = state.window->ClientSize();
  if (client.width != 0 && client.height != 0 && client != state.swapChainSize)
  {
    ApplyResize(state, client);
  }
  if (state.occluded)
  {
    // RND-032: a test present says whether the window is visible again without presenting anything.
    if (state.swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
    {
      return FrameOutcome::Suspended;
    }
    state.occluded = false;
  }

  // ADR-012: the frame-latency object before recording, then the optional frame-rate cap.
  const Nanoseconds waitsStart = state.clock->Now();
  state.lastWaits = Nanoseconds{0};
  if (WaitForSingleObjectEx(state.frameLatencyWaitable.Get(), GPU_CHECK_MILLISECONDS, TRUE) == WAIT_TIMEOUT && state.device->IsRemoved())
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  if (state.config.frameRateCap.has_value() && *state.config.frameRateCap > 0.0f)
  {
    const Nanoseconds period{static_cast<std::int64_t>(std::llround(1.0e9 / static_cast<double>(*state.config.frameRateCap)))};
    const Nanoseconds now = state.clock->Now();
    if (state.nextFrameStart.has_value() && *state.nextFrameStart > now)
    {
      state.clock->WaitUntil(*state.nextFrameStart);
    }
    state.nextFrameStart = std::max(now, state.nextFrameStart.value_or(now)) + period;
  }

  const std::size_t slot = state.frameNumber % FRAMES_IN_FLIGHT;
  if (!WaitForFence(state, state.frameFenceValues[slot]))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  state.lastWaits = state.clock->Now() - waitsStart;
  ID3D12CommandAllocator* allocator = state.allocators[slot].Get();
  ID3D12GraphicsCommandList* commands = state.commands.Get();
  if (IsDeviceRemoval(allocator->Reset()) || IsDeviceRemoval(commands->Reset(allocator, nullptr)))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }

  // The frame's share of the factory, the heaps and the ring (ADR-008): frame ids start at 1, and every frame up to
  // `completed` has finished on the GPU, since the fence of this slot was waited for.
  const std::uint64_t frameId = state.frameNumber + 1;
  const std::uint64_t completed = frameId > FRAMES_IN_FLIGHT ? frameId - FRAMES_IN_FLIGHT : 0;
  state.world->Show(_inputs.world);
  state.world->ShowMask(_inputs.viewMask);
  state.ui->Show(_inputs.glyphs, _inputs.uiImages);
  state.cursor->Show(_inputs.cursorImages);
  state.quads->Show(_inputs.sceneTextures);
  state.meshes->Show(_inputs.sceneMeshes);
  state.factory->BeginFrame(frameId, completed);
  state.heaps->Reclaim(completed);
  state.heaps->BeginFrame(static_cast<std::uint32_t>(slot));
  state.ring->BeginFrame(static_cast<std::uint32_t>(slot));
  // ADR-006: the heaps before the root signature, as directly indexed heaps require.
  state.heaps->Bind(commands);
  commands->SetGraphicsRootSignature(state.rootSignature.Get());

  const UINT backBufferIndex = state.swapChain->GetCurrentBackBufferIndex();
  ID3D12Resource* backBuffer = state.backBuffers[backBufferIndex].Get();
  D3D12_CPU_DESCRIPTOR_HANDLE target = state.renderTargetHeap->GetCPUDescriptorHandleForHeapStart();
  target.ptr += static_cast<SIZE_T>(backBufferIndex) * state.renderTargetStride;
  const bool drawsWorld = _inputs.world != nullptr && state.world->Ready() && state.targets.has_value();
  if (drawsWorld)
  {
    RecordWorldFrame(state, _inputs, backBuffer, target);
  }
  else
  {
    // No world, or its surfaces still on their way: the background colour (RND-070).
    ENGINE_PROFILE_SCOPE("render.clear");
    const std::array<float, 4> background{_inputs.background[0], _inputs.background[1], _inputs.background[2], 1.0f};
    const D3D12_RESOURCE_BARRIER toTarget = TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commands->ResourceBarrier(1, &toTarget);
    commands->ClearRenderTargetView(target, background.data(), 0, nullptr);
    if (!_inputs.overlayText.empty() || !_inputs.overlayRects.empty() || _inputs.cursor || !_inputs.ui.empty() || !_inputs.uiCursor.empty())
    {
      // DBG-095, O65 and ADR-145: the UI, the frame-rate line and the game cursor are drawn without a world too.
      const D3D12_VIEWPORT viewport{
        0.0f, 0.0f, static_cast<float>(state.swapChainSize.width), static_cast<float>(state.swapChainSize.height), 0.0f, 1.0f};
      const D3D12_RECT scissor{0, 0, static_cast<LONG>(state.swapChainSize.width), static_cast<LONG>(state.swapChainSize.height)};
      commands->OMSetRenderTargets(1, &target, FALSE, nullptr);
      commands->RSSetViewports(1, &viewport);
      commands->RSSetScissorRects(1, &scissor);
      // Every root parameter holds a value before the draw, the frame's too, which the text does not read.
      commands->SetGraphicsRootConstantBufferView(ROOT_FRAME_CONSTANTS, state.ring->Write(FrameConstants{}));
      state.ui->Record(commands, *state.ring, *state.catalog, state.swapChainSize, _inputs.ui);
      state.ui->Record(commands, *state.ring, *state.catalog, state.swapChainSize, _inputs.uiCursor);
      if (_inputs.cursor)
      {
        state.cursor->Record(commands, *state.ring, *state.catalog, state.swapChainSize, *_inputs.cursor);
      }
      state.overlay->RecordRects(commands, *state.ring, *state.catalog, state.swapChainSize, state.window->DpiScale(),
                                 _inputs.overlayRects);
      state.ui->Record(commands, *state.ring, *state.catalog, state.swapChainSize, _inputs.overlayText);
    }
    const D3D12_RESOURCE_BARRIER toPresent =
      TransitionBarrier(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commands->ResourceBarrier(1, &toPresent);
  }
  if (IsDeviceRemoval(commands->Close()))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  ID3D12CommandList* const lists[] = {commands};
  state.device->Queue()->ExecuteCommandLists(1, lists);

  const PresentPolicy policy = SelectPresentPolicy(state.config.presentMode, state.device->TearingSupported(), state.window->Mode());
  HRESULT presented = S_OK;
  {
    ENGINE_PROFILE_SCOPE("render.present");
    const Nanoseconds presentStart = state.clock->Now();
    presented = state.swapChain->Present(policy.syncInterval, policy.allowTearing ? DXGI_PRESENT_ALLOW_TEARING : 0);
    state.lastWaits += state.clock->Now() - presentStart;
  }
  state.frameFenceValues[slot] = ++state.fenceValue;
  const HRESULT signalled = state.device->Queue()->Signal(state.fence.Get(), state.fenceValue);
  ++state.frameNumber;
  if (IsDeviceRemoval(presented) || IsDeviceRemoval(signalled))
  {
    Rebuild(state);
    return FrameOutcome::Suspended;
  }
  if (presented == DXGI_STATUS_OCCLUDED)
  {
    state.occluded = true;
  }
  state.worldFrames += drawsWorld ? 1 : 0;
  // R-SIM-576: the frames presented within the last window of real time.
  const Nanoseconds now = state.clock->Now();
  const Nanoseconds window{static_cast<std::int64_t>(std::llround(state.config.frameRateWindowSeconds * 1.0e9))};
  state.presentTimes.push_back(now);
  state.frameRateWindowFilled = state.frameRateWindowFilled || now - state.presentTimes.front() >= window;
  while (now - state.presentTimes.front() > window)
  {
    state.presentTimes.pop_front();
  }
  return FrameOutcome::Presented;
}

void FrameRenderer::OnDeviceRebuilt(std::function<void()> _listener)
{
  m_state->rebuiltListeners.push_back(std::move(_listener));
}

std::uint64_t FrameRenderer::WorldFrames() const noexcept
{
  return m_state->worldFrames;
}

Nanoseconds FrameRenderer::LastFrameWaits() const noexcept
{
  return m_state->lastWaits;
}

double FrameRenderer::FramesPerSecond() const noexcept
{
  const RendererState& state = *m_state;
  if (!state.frameRateWindowFilled)
  {
    return state.config.frameRateInitial;
  }
  return static_cast<double>(state.presentTimes.size()) / std::max(state.config.frameRateWindowSeconds, 1e-3);
}

#if ENGINE_DEVELOPMENT
std::uint64_t FrameRenderer::DebugLayerMessages() noexcept
{
  return GraphicsDevice::DebugLayerMessages();
}

void FrameRenderer::DebugRemoveDevice()
{
  m_state->device->Remove();
}
#endif

void EndStartup(const StartupFailure& _failure)
{
  switch (_failure.error)
  {
  case StartupError::NoCapableAdapter:
  case StartupError::DeviceCreationFailed:
  {
    const std::array arguments{MessageArgument{"adapter", _failure.adapter}, MessageArgument{"capability", _failure.capability}};
    FatalExit("ui.error.gpu_unsupported", arguments);
  }
  case StartupError::SwapChainFailed:
    FatalExit("ui.error.display_mode", {});
  case StartupError::PipelineCreationFailed:
  case StartupError::FrameGraphInvalid:
    break;
  }
  EnterFatalPath("the renderer's pipelines or frame graph could not be created");
}

} // namespace Neuron
