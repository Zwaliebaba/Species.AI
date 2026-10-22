#include "pch.h"

#include "AssetError.h"
#include "CrowdLook.h"
#include "DdsTexture.h"
#include "Development.h"
#include "DevelopmentOverlay.h"
#include "LeaderLook.h"
#include "Log.h"
#include "UiLook.h"
#include "WorldPresentation.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <memory>
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
// ADR-145: GameFont, the player-facing face (MVP-100, O62) and the UI's face.
constexpr std::string_view GAME_FACE_KEY = "fonts/gamefont";
// ref.meta.lang.base: the base language and the MVP's one language (MVP scope §5.1), until the settings store chooses one
// (M2 task T15, ADR-063, ADR-146).
constexpr std::string_view UI_LANGUAGE = "en-US";

// GameFont from the content: none when it is missing or damaged, which is logged.
std::shared_ptr<const Neuron::FontAsset> LoadGameFace(const Neuron::LooseContent& _content)
{
  const auto bytes = _content.Read(GAME_FACE_KEY);
  if (!bytes)
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the game's face: {}", Neuron::FormatAssetError(bytes.error()));
    return nullptr;
  }
  auto face = Neuron::ReadFontAsset(*bytes);
  if (!face)
  {
    Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the game's face {} is damaged: {}", GAME_FACE_KEY, face.error());
    return nullptr;
  }
  return std::make_shared<const Neuron::FontAsset>(std::move(*face));
}

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

// ADR-120: the frame loop's profile of the record's history and roll-up period, with a step for each of ClientStep's.
Neuron::FrameProfile::Desc ProfileDescOf(const TuningPresentation& _presentation)
{
  const TuningPresentation::Dbg::Profile& profile = _presentation.dbg.profile;
  return {static_cast<std::size_t>(ClientStep::Count), static_cast<std::size_t>(std::max<std::int64_t>(profile.frameHistory, 1)),
          Neuron::Nanoseconds{std::max<std::int64_t>(std::llround(profile.rollupPeriodMs * 1.0e6), 1)}};
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
  look.gridCellsPerRepeat = _presentation.rnd.terrain.gridCellsPerRepeat;
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
    m_start(_desc.clock->Now()),
    m_effects(*_desc.presentation, *_desc.tuning),
    m_structures(*_desc.presentation),
    m_territories(*_desc.presentation, *_desc.tuning),
    m_profile(ProfileDescOf(*_desc.presentation)),
    m_presentationClock(_desc.presentation->ref.sim.frame.deltaClamp)
{
  m_cursorImages = LoadCursorImages(*m_desc.content);
  m_sceneTextures = LoadSceneTextures(*m_desc.content);
  m_sceneMeshes = LoadSceneMeshes(*m_desc.content);
  m_effects.UseMeshes(m_sceneMeshes); // ADR-141: the turret barrel set places each shot's casing
  m_gameFace = LoadGameFace(*m_desc.content);
  m_uiImages = LoadUiImages(*m_desc.content);
  m_strings = Neuron::LoadStringTable(*m_desc.content, UI_LANGUAGE);
  m_locale = std::make_unique<Neuron::MessageLocale>(UI_LANGUAGE);
#if ENGINE_DEVELOPMENT
  m_developmentFace = LoadDevelopmentFace(*m_desc.content);
#endif
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
  m_effects.Clear();
  m_structures.Clear();
  m_territories.Clear();
  m_viewMask.reset();
  m_viewMaskRevision.reset();
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

Neuron::FrameInputs WorldPresentation::Frame(ClientSession& _session, const SettlementView& _view, double _framesPerSecond)
{
  const TuningPresentation::Rnd::Frame::ClearColorNoMatch& background = m_desc.presentation->rnd.frame.clearColorNoMatch;
  Neuron::FrameInputs inputs;
  inputs.world = m_world;
  inputs.background = Rgb(background.r, background.g, background.b);
  inputs.presentationSeconds = std::chrono::duration<double>(m_desc.clock->Now() - m_start).count();
  // The player camera's pose (ADR-119), and the game cursor in every frame, since the system pointer is hidden (O65).
  inputs.camera = _view.Pose();
  if (m_cursorImages != nullptr)
  {
    inputs.cursorImages = m_cursorImages;
    inputs.cursor = _view.CursorDraw();
  }
  // RND-152: the sky moves one tick's worth per displayed tick, the displayed tick standing one behind the latest.
  std::optional<double> displayTick;
  if (const std::optional<std::uint64_t> latest = _session.ServerTick(); latest.has_value() && *latest > 0)
  {
    displayTick = static_cast<double>(*latest - 1) + _session.InterpolationFraction();
    if (!m_firstDisplayTick)
    {
      m_firstDisplayTick = displayTick;
    }
    inputs.skyTicks = *displayTick - *m_firstDisplayTick;
  }
  // ADR-123: the presentation events are taken every frame, and dropped while no world is drawn.
  const std::vector<ReceivedEvent> events = _session.TakeEvents();
  // M1 task T16 (ADR-121): the crowds, on the presentation clock (VFX-016; no world is held at M1, at game speed 1).
  m_presentationClock.Advance(m_desc.clock->Now(), false, 1.0);
  if (m_terrain != nullptr && m_sceneTextures != nullptr)
  {
    CrowdView crowd;
    crowd.camera = inputs.camera.position;
    crowd.cameraFront = Neuron::ViewDirection(inputs.camera);
    crowd.scaledSeconds = m_presentationClock.ScaledSeconds();
    crowd.unscaledSeconds = m_presentationClock.UnscaledSeconds();
    crowd.localTeam = _session.CommandedTeam();
    const double fraction = _session.InterpolationFraction();
    std::vector<InterpolatedFigure> figures = InterpolateFigures(_session.Replica(), fraction);
    // M1 task T16c (ADR-123): the events and per-tick emitters first, then the shooters' recoil on the figures drawn.
    EffectsView effectsView;
    effectsView.camera = crowd.camera;
    effectsView.cameraFront = crowd.cameraFront;
    effectsView.verticalFovRadians = inputs.camera.verticalFovRadians;
    effectsView.displayTick = displayTick.value_or(0.0);
    effectsView.fraction = fraction;
    effectsView.scaledSeconds = m_presentationClock.ScaledSeconds();
    effectsView.unscaledSeconds = m_presentationClock.UnscaledSeconds();
    effectsView.unscaledStep = m_presentationClock.UnscaledStepSeconds();
    effectsView.realSeconds = m_presentationClock.RealSeconds();
    if (displayTick.has_value())
    {
      m_effects.Update(events, _session.Replica(), figures, effectsView);
      m_effects.EmitCircle(_view.Verbs().ShownCircle(), m_terrain->field, effectsView);
      m_effects.ApplyRecoil(figures, effectsView);
    }
    inputs.sceneTextures = m_sceneTextures;
    // M2 task T13 (WLD-244, ADR-144): the black fog outside the steward's areas, its mask built again when they change,
    // and no Worker, Leader, orb or structure drawn outside them.
    const StewardAreas& areas = _view.Areas();
    if (m_viewMaskRevision != areas.Revision())
    {
      m_viewMask = BuildViewMask(areas, *m_desc.presentation);
      m_viewMaskRevision = areas.Revision();
    }
    inputs.viewMask = m_viewMask;
    EraseOutside(areas, figures, [](const InterpolatedFigure& _figure) { return _figure.position; });
    // ADR-127: the selection discs under the Worker layers of pass 7d.
    AppendSelectionDiscs(figures, m_terrain->field, crowd, *m_desc.presentation, inputs.quadBatches);
    AppendWorkerBatches(figures, m_terrain->field, crowd, *m_desc.presentation, *m_desc.tuning, inputs.quadBatches);
    // M1 task T16b (ADR-122): the Leaders' figures and banners in pass 7b.
    if (m_sceneMeshes != nullptr && !m_sceneMeshes->meshes.empty())
    {
      LeaderDraws leaders;
      m_leaders.Draw(figures, *m_sceneMeshes->meshes[static_cast<std::size_t>(SceneMesh::LeaderFigure)], crowd,
                     m_presentationClock.UnscaledStepSeconds(), *m_desc.presentation, *m_desc.tuning, leaders);
      inputs.sceneMeshes = m_sceneMeshes;
      inputs.meshBatches = std::move(leaders.meshes);
      inputs.quadBatches.insert(inputs.quadBatches.end(), std::make_move_iterator(leaders.quads.begin()),
                                std::make_move_iterator(leaders.quads.end()));
      inputs.triangleBatches = std::move(leaders.triangles);
    }
    // M2 task T12 (ADR-140): the structures' solid parts in pass 6 and their pads, stars, lights and glows in pass 8.
    m_structures.Update(events);
    if (displayTick.has_value() && m_sceneMeshes != nullptr && m_sceneMeshes->meshes.size() == static_cast<std::size_t>(SceneMesh::Count))
    {
      const StructureView structureView{*displayTick, m_desc.tuning->ref.sim.tickPeriod, m_presentationClock.RealSeconds()};
      StructureDraws structures;
      std::vector<InterpolatedStructure> shown = InterpolateStructures(_session.Replica(), fraction);
      EraseOutside(areas, shown, [](const InterpolatedStructure& _structure) { return PositionOf(_structure.state.position); });
      m_structures.Draw(shown, *m_sceneMeshes, m_terrain->field, structureView, structures);
      inputs.sceneMeshes = m_sceneMeshes;
      inputs.meshBatches.insert(inputs.meshBatches.end(), std::make_move_iterator(structures.meshes.begin()),
                                std::make_move_iterator(structures.meshes.end()));
      inputs.quadBatches.insert(inputs.quadBatches.end(), std::make_move_iterator(structures.quads.begin()),
                                std::make_move_iterator(structures.quads.end()));
    }
    std::vector<InterpolatedOrb> orbs = InterpolateOrbs(_session.Replica(), fraction);
    EraseOutside(areas, orbs, [](const InterpolatedOrb& _orb) { return _orb.position; });
    AppendOrbBatches(orbs, *m_desc.presentation, inputs.quadBatches);
    // M2 task T13 (ADR-143): every settlement's territory line and the border areas' spots, in pass 10a.
    m_territories.Update(_session.Replica().Latest(), *m_terrain);
    m_territories.AppendBatches(inputs.quadBatches);
    // M1 task T17b (ADR-125): the route preview in pass 12, the world markers in pass 16 and the badge in pass 20, its
    // count in the UI's batches of pass 20 (ADR-145).
    if (crowd.localTeam.has_value())
    {
      _view.Preview().AppendBatches(crowd.camera, crowd.cameraFront, m_presentationClock.RealSeconds(), TeamColor(*crowd.localTeam),
                                    inputs.quadBatches);
    }
    inputs.markers = _view.Markers().Quads();
    inputs.cursorBadge = _view.Badge();
    if (const std::optional<BadgeLabel>& count = _view.BadgeCountLabel(); m_gameFace != nullptr && count.has_value())
    {
      AppendBadgeCount(*count, *m_gameFace, *m_desc.presentation, _view.ViewSize().height, inputs.uiCursor);
    }
    if (displayTick.has_value())
    {
      // Passes 10a, 10b and 15, and the camera shake as an offset of the frame's view alone (VFX-232).
      EffectDraws effects;
      m_effects.Draw(InterpolateEffects(_session.Replica(), fraction), m_terrain->field, effectsView, effects);
      if (m_sceneMeshes != nullptr && m_sceneMeshes->meshes.size() == static_cast<std::size_t>(SceneMesh::Count))
      {
        inputs.sceneMeshes = m_sceneMeshes;
        inputs.meshBatches.insert(inputs.meshBatches.end(), std::make_move_iterator(effects.meshes.begin()),
                                  std::make_move_iterator(effects.meshes.end()));
      }
      inputs.quadBatches.insert(inputs.quadBatches.end(), std::make_move_iterator(effects.quads.begin()),
                                std::make_move_iterator(effects.quads.end()));
      inputs.triangleBatches.insert(inputs.triangleBatches.end(), std::make_move_iterator(effects.triangles.begin()),
                                    std::make_move_iterator(effects.triangles.end()));
      inputs.camera.yawRadians += effects.shakeYawRadians;
      inputs.camera.rollRadians += effects.shakeRollRadians;
    }
  }
  // ADR-145: the UI's face and images, which its batches read.
  inputs.uiFace = m_gameFace;
  inputs.uiImages = m_uiImages;
#if ENGINE_DEVELOPMENT
  if (m_desc.uiTest && m_gameFace != nullptr)
  {
    AppendUiTestPage(*m_gameFace, *m_desc.presentation, *m_strings, *m_locale, _view.ViewSize(), inputs.ui);
  }
  // DBG-095, DBG-096, ADR-120: while the overlay is shown, the frame-rate line, whose objects are counted while a world
  // exists, and the frame page.
  if (m_developmentFace != nullptr && m_overlayShown)
  {
    const FramePageInputs figures = FramePageOf(_session, _framesPerSecond);
    inputs.overlayFont = m_developmentFace;
    inputs.overlay.push_back(FrameRateLine(*m_desc.presentation, _framesPerSecond, figures.objects));
    FramePage page = BuildFramePage(figures, *m_desc.presentation, *m_desc.tuning, static_cast<float>(m_developmentFace->cellHeightPixels));
    inputs.overlay.insert(inputs.overlay.end(), std::make_move_iterator(page.lines.begin()), std::make_move_iterator(page.lines.end()));
    inputs.overlayRects = std::move(page.rects);
  }
  // ADR-121: with the crowds drawn, the figure dots are a world layer of the overlay (DBG-099), shown with it.
  if (m_terrain != nullptr && m_overlayShown)
  {
    inputs.dots = FigureDots(InterpolateFigures(_session.Replica(), _session.InterpolationFraction()));
  }
#else
  static_cast<void>(_framesPerSecond);
#endif
  return inputs;
}

FramePageInputs WorldPresentation::FramePageOf(const ClientSession& _session, double _framesPerSecond) const
{
  FramePageInputs figures;
  figures.profile = &m_profile;
  figures.framesPerSecond = _framesPerSecond;
  if (m_terrain != nullptr && _session.ServerTick().has_value())
  {
    figures.objects = _session.Replica().Latest().size();
    figures.counts = CountReplica(_session.Replica());
  }
  figures.server = _session.ServerStatistics();
  return figures;
}

} // namespace Species
