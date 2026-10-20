#include "pch.h"

#include "DdsTexture.h"
#include "Log.h"
#include "WorldPresentation.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numbers>
#include <span>
#include <string_view>
#include <utility>

namespace Species
{

namespace
{

// The content keys of the world frame's textures (AST-014; Design/implementation/mvp_assets.md).
constexpr std::string_view RAMP_KEY = "textures/terrain/ramp01";
constexpr std::string_view SHORE_WAVE_GRADIENT_KEY = "textures/terrain/shorewavegradient01";
constexpr std::string_view SEA_TILE_KEY = "textures/terrain/seatile01";
constexpr std::string_view TERRAIN_GRID_KEY = "textures/fx/terraingrid";
constexpr std::string_view SKY_CLOUD_KEY = "textures/fx/skycloud";
constexpr std::string_view BEAM_SHORT_KEY = "textures/fx/beamshort";

// The development view's stand-in pose (ADR-115): 400 wu beyond the region's +z coast at a height of 700 wu, looking
// along ref.ctrl.camera.initial_pose.front, normalize(0, -0.5, -1), with its 60° field of view.
constexpr double VIEW_HEIGHT = 700.0;
constexpr double VIEW_BEYOND_COAST = 400.0;
constexpr double VIEW_FRONT_Y = -0.5;
constexpr double VIEW_FIELD_DEGREES = 60.0;

std::array<float, 3> Rgb(double _r, double _g, double _b) noexcept
{
  return {static_cast<float>(_r), static_cast<float>(_g), static_cast<float>(_b)};
}

std::array<float, 4> Rgba(double _r, double _g, double _b, double _a) noexcept
{
  return {static_cast<float>(_r), static_cast<float>(_g), static_cast<float>(_b), static_cast<float>(_a)};
}

double Radians(double _degrees) noexcept
{
  return _degrees * std::numbers::pi / 180.0;
}

std::optional<std::vector<std::byte>> ReadTexture(const Neuron::LooseContent& _content, std::string_view _key)
{
  auto bytes = _content.Read(_key);
  if (!bytes)
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the world frame's texture {} cannot be read", _key);
    return std::nullopt;
  }
  return std::move(*bytes);
}

// AST-072: a texture read on the CPU, its first level as R8G8B8A8 texels.
std::optional<Neuron::CpuImage> CpuImageOf(const std::vector<std::byte>& _file, std::string_view _key)
{
  const auto texture = Neuron::DescribeDds(_file, std::string(_key));
  if (!texture || texture->levels.empty() ||
      (texture->desc.format != DXGI_FORMAT_R8G8B8A8_UNORM && texture->desc.format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB))
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "{} is not an R8G8B8A8 texture the CPU can read", _key);
    return std::nullopt;
  }
  const std::span<const std::byte> texels = texture->levels.front().bytes;
  return Neuron::CpuImage{texture->desc.width, texture->desc.height,
                          std::span(reinterpret_cast<const std::uint8_t*>(texels.data()), texels.size())};
}

} // namespace

Neuron::TerrainLook TerrainLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept
{
  Neuron::TerrainLook look;
  look.sinkThreshold = _presentation.rnd.terrain.sinkThreshold;
  look.sinkHeight = _presentation.rnd.terrain.sinkHeight;
  look.rampSize = static_cast<std::uint32_t>(_presentation.ter.color.rampSize);
  look.centerFactor = _presentation.ter.color.centerFactor;
  look.slopeExponent = _presentation.ter.color.slopeExponent;
  look.jitterNumerator = _presentation.ter.color.jitterNumerator;
  look.jitterOffset = _presentation.ter.color.jitterOffset;
  look.presentationSeed = static_cast<std::uint32_t>(_presentation.ref.sim.match.presentationSeed);
  look.chunkSide = _tuning.wld.terrain.chunkSide;
  return look;
}

Neuron::SeaLook SeaLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation) noexcept
{
  const TuningPresentation::Ter::Sea& sea = _presentation.ter.sea;
  Neuron::SeaLook look;
  look.seaLevel = _tuning.ref.sim.physics.seaLevel;
  look.flatHeight = _presentation.rnd.sea.flatHeight;
  look.maskSize = static_cast<std::uint32_t>(sea.maskSize);
  look.maskLandOffset = static_cast<std::uint32_t>(sea.maskLandOffset);
  look.blurTaps = {sea.blurTaps.tap0, sea.blurTaps.tap1, sea.blurTaps.tap2, sea.blurTaps.tap3, sea.blurTaps.tap4, sea.blurTaps.tap5,
                   sea.blurTaps.tap6, sea.blurTaps.tap7, sea.blurTaps.tap8, sea.blurTaps.tap9, sea.blurTaps.tap10};
  look.blurNormalizer = sea.blurNormalizer;
  look.blurSkipTolerance = sea.blurSkipTolerance;
  look.maskGain = sea.maskGain;
  look.maskMax = sea.maskMax;
  look.depthOutside = sea.depthOutside;
  look.flatTileRatio = static_cast<std::uint32_t>(sea.flatTileRatio);
  look.cellDivisor = sea.cellDivisor;
  look.vertexLandLimit = sea.vertexLandLimit;
  look.vertexDepthLimit = sea.vertexDepthLimit;
  look.shoreNoise = sea.wave.shoreNoise;
  look.shoreBrightening = sea.wave.shoreBrightening;
  look.tableMargin = static_cast<std::uint32_t>(sea.wave.tableMargin);
  look.amplitudeScale = sea.wave.amplitudeScale;
  const auto term = [](const auto& _term) { return Neuron::WaveTerm{_term.perWu, _term.perSecond, _term.amplitude}; };
  look.xTerms = {term(sea.wave.xTerms.first), term(sea.wave.xTerms.second)};
  look.zTerms = {term(sea.wave.zTerms.first), term(sea.wave.zTerms.second)};
  look.indexBias = sea.wave.indexBias;
  look.brightnessScale = sea.wave.brightnessScale;
  look.mixOther = sea.wave.colorMix.other;
  look.mixOwn = sea.wave.colorMix.own;
  look.detail = 1; // rnd.option.sea_detail.default (RND-442): the settings store holds it once it exists
  return look;
}

Neuron::WorldLook WorldLookOf(const TuningMvp& _tuning, const TuningPresentation& _presentation, const Neuron::Vec3& _regionOrigin) noexcept
{
  const TuningPresentation::Rnd& rnd = _presentation.rnd;
  Neuron::WorldLook look;
  look.fogColor = Rgb(rnd.frame.clearColor.r, rnd.frame.clearColor.g, rnd.frame.clearColor.b);
  // O59, RND-222: the fog ends a region's side plus the margin from the camera, at every world size.
  const double side = _tuning.wld.region.side;
  look.fogEnd = std::max(rnd.fog.minEnd, side + rnd.fog.edgeMargin);
  look.fogStart = rnd.fog.startFraction * look.fogEnd;
  const TuningPresentation::Mvp::Light& light = _presentation.mvp.light;
  look.keyTowards = {light.key.towards.x, light.key.towards.y, light.key.towards.z};
  look.keyColor = Rgb(light.key.color.r, light.key.color.g, light.key.color.b);
  look.fillTowards = {light.fill.towards.x, light.fill.towards.y, light.fill.towards.z};
  look.fillColor = Rgb(light.fill.color.r, light.fill.color.g, light.fill.color.b);
  const auto specular = static_cast<float>(rnd.terrain.overlaySpecular);
  look.overlaySpecular = {specular, specular, specular};
  look.overlayShininess = static_cast<float>(rnd.terrain.overlayShininess);
  look.seaTileRepeats = rnd.sea.tileRepeats;
  look.seaTileScrollPeriod = rnd.sea.tileScrollPeriod;
  look.planes = {rnd.view.nearPlane, rnd.view.farPlane, Radians(rnd.view.fovMin), Radians(rnd.view.fovMax)};

  const TuningPresentation::Rnd::Sky& sky = rnd.sky;
  Neuron::SkyLook& out = look.sky;
  out.gridHeight = sky.gridHeight;
  out.gridSpacing = sky.gridSpacing;
  out.gridMin = sky.gridMin;
  out.gridMax = sky.gridMax;
  out.gridHalfWidth = sky.gridLineHalfWidth;
  out.gridColor = Rgba(sky.gridColor.r, sky.gridColor.g, sky.gridColor.b, sky.gridColor.a);
  out.gridFogStart = sky.gridFogStart;
  out.gridFogEnd = sky.gridFogEnd;
  out.velocity = {sky.velocity.x, sky.velocity.y, sky.velocity.z};
  out.tickSeconds = _tuning.ref.sim.tickPeriod;
  out.cloudMin = sky.cloudMin;
  out.cloudMax = sky.cloudMax;
  out.cloudFogStart = sky.cloudFogStart;
  out.cloudFogEnd = sky.cloudFogEnd;
  out.layer2Height = sky.layer2Height;
  out.layer3Height = sky.layer3Height;
  out.blobbyColor = Rgba(sky.blobbyColor.r, sky.blobbyColor.g, sky.blobbyColor.b, sky.blobbyColor.a);
  out.flatColor = Rgba(sky.flatColor.r, sky.flatColor.g, sky.flatColor.b, sky.flatColor.a);
  out.layer1Repeats = sky.layer1Repeats;
  out.layer2Repeats = sky.layer2Repeats;
  out.layer3Repeats = sky.layer3Repeats;
  out.regionOriginX = _regionOrigin.x;
  out.regionOriginZ = _regionOrigin.z;
  out.regionCenter = {_regionOrigin.x + side / 2.0, 0.0, _regionOrigin.z + side / 2.0};
  return look;
}

Neuron::CameraPose DevelopmentView(const Neuron::Vec3& _regionOrigin, double _regionSide) noexcept
{
  Neuron::CameraPose pose;
  pose.position = {_regionOrigin.x + _regionSide / 2.0, VIEW_HEIGHT, _regionOrigin.z + _regionSide + VIEW_BEYOND_COAST};
  pose.yawRadians = 0.0; // along -z
  pose.pitchRadians = std::atan2(VIEW_FRONT_Y, 1.0);
  pose.verticalFovRadians = Radians(VIEW_FIELD_DEGREES);
  return pose;
}

std::shared_ptr<const Neuron::WorldScene> BuildWorldScene(const RegionTerrain& _terrain, const TuningMvp& _tuning,
                                                          const TuningPresentation& _presentation, const Neuron::LooseContent& _content)
{
  auto ramp = ReadTexture(_content, RAMP_KEY);
  auto gradient = ReadTexture(_content, SHORE_WAVE_GRADIENT_KEY);
  auto seaTile = ReadTexture(_content, SEA_TILE_KEY);
  auto terrainGrid = ReadTexture(_content, TERRAIN_GRID_KEY);
  auto skyCloud = ReadTexture(_content, SKY_CLOUD_KEY);
  auto beamShort = ReadTexture(_content, BEAM_SHORT_KEY);
  if (!ramp || !gradient || !seaTile || !terrainGrid || !skyCloud || !beamShort)
  {
    return nullptr;
  }
  const std::optional<Neuron::CpuImage> rampImage = CpuImageOf(*ramp, RAMP_KEY);
  const std::optional<Neuron::CpuImage> gradientImage = CpuImageOf(*gradient, SHORE_WAVE_GRADIENT_KEY);
  if (!rampImage || !gradientImage)
  {
    return nullptr;
  }
  const Neuron::SurfaceGrid<double>& heights = _terrain.field.Heights();
  const Neuron::Vec3 origin{heights.OriginX(), 0.0, heights.OriginZ()};
  auto scene = std::make_shared<Neuron::WorldScene>();
  scene->terrain = Neuron::BuildTerrainSurface(_terrain.field, *rampImage, TerrainLookOf(_tuning, _presentation));
  scene->seaLook = SeaLookOf(_tuning, _presentation);
  scene->sea = Neuron::BuildSeaSurface(_terrain.field, scene->seaLook, scene->terrain.colorChainEnd, *gradientImage,
                                       static_cast<std::uint32_t>(_presentation.rnd.sea.waveTableRow));
  scene->look = WorldLookOf(_tuning, _presentation, origin);
  scene->seaTile = std::move(*seaTile);
  scene->terrainGrid = std::move(*terrainGrid);
  scene->skyCloud = std::move(*skyCloud);
  scene->beamShort = std::move(*beamShort);
  return scene;
}

WorldPresentation::WorldPresentation(Desc _desc)
  : m_desc(_desc),
    m_start(_desc.clock->Now())
{
}

void WorldPresentation::Update(const ClientSession& _session)
{
  const RegionTerrain* terrain = _session.Terrain();
  if (terrain == m_terrain)
  {
    return;
  }
  m_terrain = terrain;
  m_firstDisplayTick.reset();
  if (terrain == nullptr)
  {
    m_world.reset();
    return;
  }
  const Neuron::Nanoseconds before = m_desc.clock->Now();
  m_world = BuildWorldScene(*terrain, *m_desc.tuning, *m_desc.presentation, *m_desc.content);
  const auto milliseconds = std::chrono::duration<double, std::milli>(m_desc.clock->Now() - before).count();
  if (m_world != nullptr)
  {
    Neuron::Log(Neuron::LogCategory::Render, Neuron::LogLevel::Info, "the world of seed {} was built for the frame in {:.1f} ms",
                terrain->seed, milliseconds);
  }
}

Neuron::FrameInputs WorldPresentation::Frame(const ClientSession& _session)
{
  const TuningPresentation::Rnd::Frame::ClearColorNoMatch& background = m_desc.presentation->rnd.frame.clearColorNoMatch;
  Neuron::FrameInputs inputs;
  inputs.world = m_world;
  inputs.background = Rgb(background.r, background.g, background.b);
  inputs.presentationSeconds = std::chrono::duration<double>(m_desc.clock->Now() - m_start).count();
  if (m_terrain != nullptr)
  {
    const Neuron::SurfaceGrid<double>& heights = m_terrain->field.Heights();
    inputs.camera = DevelopmentView({heights.OriginX(), 0.0, heights.OriginZ()}, m_desc.tuning->wld.region.side);
  }
  // RND-152: the sky moves one tick's worth per displayed tick, the displayed tick standing one behind the latest.
  if (const std::optional<std::uint64_t> latest = _session.ServerTick(); latest.has_value() && *latest > 0)
  {
    const double displayed = static_cast<double>(*latest - 1) + _session.InterpolationFraction();
    if (!m_firstDisplayTick)
    {
      m_firstDisplayTick = displayed;
    }
    inputs.skyTicks = displayed - *m_firstDisplayTick;
  }
  return inputs;
}

} // namespace Species
