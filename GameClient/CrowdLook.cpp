#include "pch.h"

#include "AssetError.h"
#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "FigureKind.h"
#include "Log.h"
#include "SettlementView.h"
#include "StableId.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <string_view>
#include <utility>

namespace Species
{

namespace
{

// The content keys of the scene's textures, in SceneTexture's order (AST-014; Design/implementation/mvp_assets.md).
constexpr std::array<std::string_view, static_cast<std::size_t>(SceneTexture::Count)> SCENE_TEXTURE_KEYS{
  "textures/sprite/workerfront",
  "textures/sprite/workerside",
  "textures/sprite/workerhurt01",
  "textures/sprite/workerhurt02",
  "textures/sprite/workerhurt03",
  "textures/sprite/workerhurt04",
  "textures/sprite/workercarryload",
  "textures/fx/glowround",
  "textures/marker/leaderbanneridle",
  "textures/marker/leaderbannerfollow",
  "textures/marker/leaderbannerformation",
  "textures/marker/leaderbannertintidle",
  "textures/marker/leaderbannertintfollow",
  "textures/marker/leaderbannertintformation",
  "textures/fx/muzzleflash",
  "textures/fx/starburst",
  "textures/fx/beamshort",
  "textures/fx/particledot",
  "textures/marker/ordertrail",
  "textures/marker/routearrow",
  "textures/marker/flagpointer",
  "textures/marker/edgecompass",
  "textures/marker/edgecompassempty",
  "textures/marker/edgecompassshadow",
  "textures/marker/teamdisc",
  "textures/marker/teamdiscshadow",
  "textures/icon/power/leader",
  "textures/icon/power/workers",
  "textures/fx/glowcloudy",
};
constexpr std::array<std::string_view, static_cast<std::size_t>(SceneMesh::Count)> SCENE_MESH_KEYS{
  "meshes/leaderfigure", "meshes/thrownweapon", "meshes/wellspring",      "meshes/resourcesite", "meshes/resourcesitecentral",
  "meshes/dwelling",     "meshes/storehouse",   "meshes/turretmount",     "meshes/turrethead",   "meshes/turretbarrelgun",
  "meshes/turretround",  "meshes/shellcasing",  "meshes/structuredefault"};

constexpr double FULL_ALPHA = 255.0;
// ADR-121: R-UNIT-142's (slot index × unit id) mod 10 reads the unit id's counter times this prime, since a figure's
// pool slot is not replicated (ADR-111).
constexpr std::uint64_t VARIATION_PRIME = 7919;
constexpr std::uint64_t VARIATION_STEPS = 10;
constexpr std::size_t HURT_AIR_VARIANTS = 3; // sprite.worker.hurt variants 01 to 03 (R-UNIT-145)
constexpr Neuron::Vec3 WORLD_UP{0.0, 1.0, 0.0};
constexpr Neuron::Vec3 WORLD_Z{0.0, 0.0, 1.0};
constexpr double SCREEN_DEPTH_MIN = 1e-3; // wu: a point nearer the camera's plane than this has no screen position

// The Worker layers of VFX-118, in their order, each with its catalogue entry.
enum class WorkerLayer : std::uint8_t
{
  Shadow,
  Body,
  Load, // ADR-140: sprite.worker.carry_load over the body of a Worker that carries a load (MVP-082)
  Side,
  Fragments,
  Glow,
  Count
};

constexpr std::array<std::string_view, static_cast<std::size_t>(WorkerLayer::Count)> WORKER_LAYER_ENTRIES{
  "worker_ground_shadow", "worker_card", "worker_card", "worker_card", "worker_card", "worker_glow"};
// ADR-140: the load's card stands this far nearer the camera than the body's, so the depth test keeps it over it.
constexpr double LOAD_TOWARDS_CAMERA = 0.05; // wu
// ADR-127: the selection discs' entry, the masked additive quad with no depth test of pass 10a's trail marks.
constexpr std::string_view SELECTION_DISC_ENTRY = "effect_additive_nodepth";

Neuron::Vec3 Cross(const Neuron::Vec3& _a, const Neuron::Vec3& _b) noexcept
{
  return {_a.y * _b.z - _a.z * _b.y, _a.z * _b.x - _a.x * _b.z, _a.x * _b.y - _a.y * _b.x};
}

// Rodrigues' rotation of _v about the unit axis _axis by _radians.
Neuron::Vec3 Rotated(const Neuron::Vec3& _v, const Neuron::Vec3& _axis, double _radians) noexcept
{
  const double c = std::cos(_radians);
  const double s = std::sin(_radians);
  return _v * c + Cross(_axis, _v) * s + _axis * (Neuron::Dot(_axis, _v) * (1.0 - c));
}

std::array<float, 4> Scaled(const std::array<float, 4>& _color, double _rgb, double _alpha) noexcept
{
  return {static_cast<float>(_color[0] * _rgb), static_cast<float>(_color[1] * _rgb), static_cast<float>(_color[2] * _rgb),
          static_cast<float>(std::clamp(_alpha, 0.0, 1.0))};
}

std::uint64_t VariationOf(StableId _id) noexcept
{
  return (CounterOf(_id) * VARIATION_PRIME) % VARIATION_STEPS;
}

// RND-316, R-UNIT-145: the card's texture. The hurt pose 04 for a Burning Worker; for the share of airborne corpses that
// the unit id chooses, variants 01 to 03; the front card otherwise.
SceneTexture CardTextureOf(const InterpolatedFigure& _figure, const TuningPresentation& _presentation) noexcept
{
  if (!_figure.state.dead && _figure.state.task == static_cast<std::uint8_t>(WorkerState::Burning))
  {
    return SceneTexture::WorkerHurt04;
  }
  const double share = _presentation.ref.unit.worker.airCorpseHurtShare;
  if (_figure.state.dead && !_figure.state.onGround &&
      static_cast<double>(VariationOf(_figure.id)) < share * static_cast<double>(VARIATION_STEPS))
  {
    return static_cast<SceneTexture>(static_cast<std::size_t>(SceneTexture::WorkerHurt01) + CounterOf(_figure.id) % HURT_AIR_VARIANTS);
  }
  return SceneTexture::WorkerFront;
}

/// The batches of the Worker layers, one per layer and texture, made in layer order so the draws keep it.
class WorkerBatches
{
public:
  WorkerBatches(std::vector<Neuron::QuadBatch>& _batches, double _cutout)
    : m_batches(_batches),
      m_first(_batches.size())
  {
    for (std::size_t layer = 0; layer < WORKER_LAYER_ENTRIES.size(); ++layer)
    {
      for (std::size_t texture = 0; texture < static_cast<std::size_t>(SceneTexture::GlowRound); ++texture)
      {
        Neuron::QuadBatch batch;
        batch.pass = Neuron::ScenePass::Workers;
        batch.entry = WORKER_LAYER_ENTRIES[layer];
        batch.texture = static_cast<std::uint16_t>(texture);
        batch.cutout = layer == static_cast<std::size_t>(WorkerLayer::Glow) ? 0.0f : static_cast<float>(_cutout);
        m_batches.push_back(std::move(batch));
      }
    }
  }

  WorkerBatches(const WorkerBatches&) = delete;
  WorkerBatches& operator=(const WorkerBatches&) = delete;

  std::vector<Neuron::SceneQuad>& Of(WorkerLayer _layer, SceneTexture _texture)
  {
    const std::size_t textures = static_cast<std::size_t>(SceneTexture::GlowRound);
    return m_batches[m_first + static_cast<std::size_t>(_layer) * textures + static_cast<std::size_t>(_texture)].quads;
  }

  // The view test's radius for every batch, and the batches with no quad removed.
  void Finish(float _cullRadius)
  {
    for (std::size_t n = m_first; n < m_batches.size(); ++n)
    {
      m_batches[n].cullRadius = _cullRadius;
    }
    const auto empty = std::remove_if(m_batches.begin() + static_cast<std::ptrdiff_t>(m_first), m_batches.end(),
                                      [](const Neuron::QuadBatch& _batch) { return _batch.quads.empty(); });
    m_batches.erase(empty, m_batches.end());
  }

private:
  std::vector<Neuron::QuadBatch>& m_batches;
  std::size_t m_first;
};

} // namespace

std::shared_ptr<const Neuron::SceneTextures> LoadSceneTextures(const Neuron::LooseContent& _content)
{
  auto textures = std::make_shared<Neuron::SceneTextures>();
  for (const std::string_view key : SCENE_TEXTURE_KEYS)
  {
    auto bytes = _content.Read(key);
    if (!bytes)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "a scene texture: {}", Neuron::FormatAssetError(bytes.error()));
      return nullptr;
    }
    textures->files.push_back(std::move(*bytes));
  }
  return textures;
}

std::shared_ptr<const Neuron::SceneMeshes> LoadSceneMeshes(const Neuron::LooseContent& _content)
{
  auto meshes = std::make_shared<Neuron::SceneMeshes>();
  for (const std::string_view key : SCENE_MESH_KEYS)
  {
    const auto bytes = _content.Read(key);
    if (!bytes)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "a scene mesh: {}", Neuron::FormatAssetError(bytes.error()));
      return nullptr;
    }
    auto mesh = Neuron::ReadMeshAsset(*bytes);
    if (!mesh)
    {
      Neuron::Log(Neuron::LogCategory::Content, Neuron::LogLevel::Error, "the scene mesh {} is damaged: {}", key, mesh.error());
      return nullptr;
    }
    meshes->meshes.push_back(std::make_shared<const Neuron::MeshAsset>(std::move(*mesh)));
  }
  return meshes;
}

std::array<double, 2> ScreenDirection(const Neuron::Vec3& _from, const Neuron::Vec3& _to, const Neuron::Vec3& _camera,
                                      const Neuron::Vec3& _cameraFront) noexcept
{
  // The camera's right and up, level with the ground as the frame's view is when it does not roll (ADR-119).
  const Neuron::Vec3 right = Neuron::NormalizedOr(Cross(_cameraFront, WORLD_UP), {1.0, 0.0, 0.0});
  const Neuron::Vec3 up = Cross(right, _cameraFront);
  const Neuron::Vec3 from = _from - _camera;
  const Neuron::Vec3 to = _to - _camera;
  const double fromDepth = Neuron::Dot(from, _cameraFront);
  const double toDepth = Neuron::Dot(to, _cameraFront);
  std::array<double, 2> direction{Neuron::Dot(to - from, right), Neuron::Dot(to - from, up)};
  if (fromDepth > SCREEN_DEPTH_MIN && toDepth > SCREEN_DEPTH_MIN)
  {
    direction = {Neuron::Dot(to, right) / toDepth - Neuron::Dot(from, right) / fromDepth,
                 Neuron::Dot(to, up) / toDepth - Neuron::Dot(from, up) / fromDepth};
  }
  const double length = std::hypot(direction[0], direction[1]);
  return length > 0.0 ? std::array<double, 2>{direction[0] / length, direction[1] / length} : std::array<double, 2>{0.0, 0.0};
}

double WorkerDetail(const TuningPresentation& _presentation, double _distance) noexcept
{
  const double limit = _presentation.rnd.crowd.workerDetailDistance.first; // rnd.option.entity_detail.default is 1
  return std::clamp(1.0 - (_distance * _distance) / (limit * limit), 0.0, 1.0);
}

double WorkerCardHalfSize(const TuningPresentation& _presentation, StableId _id, double _distance) noexcept
{
  const auto& worker = _presentation.ref.unit.worker;
  double half = worker.cardHalfWidth * (1.0 + worker.cardVariation * static_cast<double>(VariationOf(_id)));
  if (_distance > worker.cardScaleStart)
  {
    half *= 1.0 + WORKER_DISTANCE_SCALE_FACTOR * (_distance - worker.cardScaleStart) / worker.cardScaleDivisor;
  }
  return half;
}

void AppendWorkerBatches(std::span<const InterpolatedFigure> _figures, const Neuron::TerrainField& _field, const CrowdView& _view,
                         const TuningPresentation& _presentation, const TuningMvp& _tuning, std::vector<Neuron::QuadBatch>& _batches)
{
  const auto& worker = _presentation.ref.unit.worker;
  const auto& look = _presentation.vfx.worker;
  WorkerBatches batches(_batches, _presentation.rnd.pso.workerCutout);
  const double billboardDistance = worker.cardFaceStart + worker.cardScaleDivisor; // the card faces the camera beyond it
  const Neuron::Vec3 cameraRight = Neuron::NormalizedOr(Cross(_view.cameraFront, WORLD_UP), {1.0, 0.0, 0.0});
  const double tickSeconds = _tuning.ref.sim.tickPeriod;
  const double maxHealth = std::max(static_cast<double>(_tuning.ref.unit.worker.health), 1.0);
  double largestHalf = 0.0;
  for (const InterpolatedFigure& figure : _figures)
  {
    const ReplicatedFigure& state = figure.state;
    if (state.kind != FigureKind::Worker)
    {
      continue;
    }
    const double distance = Neuron::Length(figure.position - _view.camera);
    const double detail = WorkerDetail(_presentation, distance);
    const double half = WorkerCardHalfSize(_presentation, figure.id, distance);
    largestHalf = std::max(largestHalf, half);
    const bool airborne = !state.onGround;
    const bool drowning = state.task == static_cast<std::uint8_t>(WorkerState::Drowning);
    const Neuron::Vec3 front{std::sin(figure.yawRadians), 0.0, std::cos(figure.yawRadians)};
    const std::uint64_t idStep = CounterOf(figure.id) % VARIATION_STEPS;

    // VFX-117: the foot point and the up axis, the terrain's normal near the camera.
    Neuron::Vec3 foot = figure.position;
    Neuron::Vec3 up = WORLD_UP;
    if (detail > 0.0 && distance < billboardDistance)
    {
      if (!airborne)
      {
        foot.y = _field.Height(foot.x, foot.z);
        up = _field.Normal(foot.x, foot.z);
      }
      else if (!drowning)
      {
        foot.y += look.airborneLift;
      }
    }
    if (airborne && !drowning)
    {
      const double sign = idStep % 2 == 0 ? 1.0 : -1.0;
      up = Rotated(up, front, sign * _view.scaledSeconds * static_cast<double>(idStep) * worker.tumbleRate);
    }
    // ADR-121: RND-314's right = front × up is written for the reference's left-handed world; in Species' right-handed
    // world (ADR-016) it is up × front, which meets the camera right with the sign of VFX-117 when the card faces it.
    const Neuron::Vec3 right = Neuron::NormalizedOr(Cross(up, front), cameraRight);
    const bool blendLeft = !(Neuron::Dot(front, _view.cameraFront) < 0.0);
    // O75, ADR-121: a walking Worker bobs, by its speed over the latest tick.
    if (!state.dead && !airborne && tickSeconds > 0.0)
    {
      const double speed = std::hypot(figure.tickMotion.x, figure.tickMotion.z) / tickSeconds;
      const double gait = std::min(speed / look.walkBobFullSpeed, 1.0);
      foot =
        foot + up * (look.walkBobHeight * gait * std::abs(std::sin(look.walkBobRate * _view.scaledSeconds + static_cast<double>(idStep))));
    }

    const SceneTexture texture = CardTextureOf(figure, _presentation);
    const std::array<float, 4> team = TeamColor(state.team);
    Neuron::SceneQuad card;
    card.position = foot;
    card.axisRight = right;
    card.axisUp = up;
    card.halfWidth = static_cast<float>(half);
    card.halfHeight = static_cast<float>(half);
    card.blendStart = static_cast<float>(worker.cardFaceStart);
    card.blendLength = static_cast<float>(worker.cardScaleDivisor);
    card.blendLeft = blendLeft;
    card.orientation = Neuron::QuadOrientation::WorldOriented;
    card.anchor = Neuron::QuadAnchor::BottomCenter;

    // RND-241, VFX-118: the ground shadow, the card drawn flat in black, spanning its right axis and world +z.
    if (detail > 0.0 && !state.dead && distance <= worker.shadowRange)
    {
      Neuron::SceneQuad shadow = card;
      shadow.position = {figure.position.x, _field.Height(figure.position.x, figure.position.z) + look.shadowLift, figure.position.z};
      shadow.axisRight = Neuron::NormalizedOr({right.x, 0.0, right.z}, {1.0, 0.0, 0.0});
      shadow.axisUp = WORLD_Z;
      shadow.blendLength = 0.0f;
      shadow.color = {0.0f, 0.0f, 0.0f, static_cast<float>(std::min(worker.shadowAlpha * detail / FULL_ALPHA, 1.0))};
      batches.Of(WorkerLayer::Shadow, texture).push_back(shadow);
    }
    // VFX-118's body, alive or airborne, in the colour of RND-317.
    const bool body = !state.dead || airborne;
    if (body)
    {
      Neuron::SceneQuad drawn = card;
      drawn.color = state.dead ? Scaled(team, worker.corpseColor, worker.corpseAlphaPerHealth * figure.health / FULL_ALPHA)
                               : Scaled(team, worker.colorBase + worker.colorHealthWeight * figure.health / maxHealth, 1.0);
      batches.Of(WorkerLayer::Body, texture).push_back(drawn);
      // MVP-082 (ADR-140): a carried load on the card, scaled and turned with it, in mvp.worker.load_color.
      if (state.loaded && !state.dead)
      {
        const auto& load = _presentation.mvp.worker.loadColor;
        Neuron::SceneQuad carried = card;
        carried.position = foot + Neuron::NormalizedOr(_view.camera - foot, WORLD_UP) * LOAD_TOWARDS_CAMERA;
        carried.color = {static_cast<float>(load.r), static_cast<float>(load.g), static_cast<float>(load.b), 1.0f};
        batches.Of(WorkerLayer::Load, SceneTexture::WorkerCarryLoad).push_back(carried);
      }
      // VFX-118: the side strip of a card seen nearly edge-on, fading in with the sine of the view angle.
      const Neuron::Vec3 towardsCamera = Neuron::NormalizedOr(_view.camera - figure.position, WORLD_UP);
      const double edge = Neuron::Length(Cross(front, towardsCamera));
      if (edge > worker.sideViewThreshold)
      {
        Neuron::SceneQuad side = card;
        side.axisRight = cameraRight;
        side.halfWidth = static_cast<float>(look.sideHalfWidth * half / worker.cardHalfWidth);
        side.blendLength = 0.0f;
        side.color = Scaled(team, 1.0, look.sideAlphaGain * (edge - worker.sideViewThreshold));
        batches.Of(WorkerLayer::Side, SceneTexture::WorkerSide).push_back(side);
      }
    }
    // VFX-118: a corpse's fragments, half-size black cards drifting apart and rising as its fade runs down.
    if (state.dead)
    {
      const double spread = std::max(look.fragmentSpread - figure.health / look.fragmentSpreadDivisor, 0.0);
      const auto count = static_cast<std::size_t>(std::max<std::int64_t>(worker.corpseFragments, 0));
      for (std::size_t n = 0; n < count; ++n)
      {
        const double angle = 2.0 * std::numbers::pi * static_cast<double>(n) / static_cast<double>(count);
        Neuron::SceneQuad fragment = card;
        fragment.position = foot + Neuron::Vec3{std::cos(angle) * spread, spread, std::sin(angle) * spread};
        fragment.halfWidth = static_cast<float>(half * 0.5);
        fragment.halfHeight = static_cast<float>(half * 0.5);
        fragment.color = {0.0f, 0.0f, 0.0f, static_cast<float>(std::clamp(figure.health * look.fragmentAlpha / FULL_ALPHA, 0.0, 1.0))};
        batches.Of(WorkerLayer::Fragments, texture).push_back(fragment);
      }
    }
    // R-UNIT-147, VFX-118: the local team's controlled or selected Worker glows, ten stacked cards pulsing.
    if (!state.dead && _view.localTeam && state.team == *_view.localTeam && (state.underControl || state.selected))
    {
      const double pulse = std::max(std::abs(std::sin(look.glowPulseFast * _view.unscaledSeconds)), worker.controlGlowMin);
      Neuron::SceneQuad glow = card;
      glow.color = {static_cast<float>(look.controlGlowColor.r), static_cast<float>(look.controlGlowColor.g),
                    static_cast<float>(look.controlGlowColor.b), static_cast<float>(pulse)};
      std::vector<Neuron::SceneQuad>& glows = batches.Of(WorkerLayer::Glow, texture);
      for (std::int64_t layer = 0; layer < look.controlGlowLayers; ++layer)
      {
        glows.push_back(glow);
      }
    }
  }
  // The view test's sphere holds a card's whole height and its fragments' drift.
  batches.Finish(static_cast<float>(2.0 * largestHalf + look.fragmentSpread * 2.0));
}

void AppendSelectionDiscs(std::span<const InterpolatedFigure> _figures, const Neuron::TerrainField& _field, const CrowdView& _view,
                          const TuningPresentation& _presentation, std::vector<Neuron::QuadBatch>& _batches)
{
  if (!_view.localTeam.has_value())
  {
    return;
  }
  const auto& selection = _presentation.mvp.selection;
  const auto& glow = _presentation.vfx.worker.controlGlowColor;
  Neuron::QuadBatch batch;
  batch.pass = Neuron::ScenePass::Workers;
  batch.entry = SELECTION_DISC_ENTRY;
  batch.texture = static_cast<std::uint16_t>(SceneTexture::TeamDisc);
  for (const InterpolatedFigure& figure : _figures)
  {
    const ReplicatedFigure& state = figure.state;
    if (state.kind != FigureKind::Worker || state.dead || !state.underControl || state.team != *_view.localTeam)
    {
      continue;
    }
    // On the terrain under the Worker, turned into its slope; marker.team_disc is round, so its turn about the normal
    // does not show.
    const Neuron::Vec3 at{figure.position.x, _field.Height(figure.position.x, figure.position.z), figure.position.z};
    const Neuron::Vec3 normal = Neuron::NormalizedOr(_field.Normal(at.x, at.z), WORLD_UP);
    const Neuron::Vec3 right = Neuron::NormalizedOr(Cross(normal, WORLD_Z), {1.0, 0.0, 0.0});
    const double half = selection.discSizeFactor * WorkerCardHalfSize(_presentation, figure.id, Neuron::Length(at - _view.camera));
    Neuron::SceneQuad disc;
    disc.position = at;
    disc.halfWidth = static_cast<float>(half);
    disc.halfHeight = disc.halfWidth;
    disc.color = {static_cast<float>(glow.r), static_cast<float>(glow.g), static_cast<float>(glow.b),
                  static_cast<float>(selection.discAlpha)};
    disc.endColor = disc.color;
    disc.orientation = Neuron::QuadOrientation::WorldOriented;
    disc.axisRight = right;
    disc.axisUp = Cross(right, normal);
    batch.cullRadius = std::max(batch.cullRadius, static_cast<float>(half * std::numbers::sqrt2));
    batch.quads.push_back(disc);
  }
  if (!batch.quads.empty())
  {
    _batches.push_back(std::move(batch));
  }
}

void AppendOrbBatches(std::span<const InterpolatedOrb> _orbs, const TuningPresentation& _presentation,
                      std::vector<Neuron::QuadBatch>& _batches)
{
  const auto& orb = _presentation.ref.unit.essenceOrb;
  Neuron::QuadBatch batch;
  batch.pass = Neuron::ScenePass::Orbs;
  batch.entry = "orb_additive";
  batch.texture = static_cast<std::uint16_t>(SceneTexture::GlowRound);
  for (const InterpolatedOrb& drawn : _orbs)
  {
    // R-UNIT-331: growing in Birth, shrinking and fading in Ascent.
    double size = 1.0;
    double fade = 1.0;
    if (drawn.state.state == OrbState::Birth)
    {
      size = std::max(orb.birthScaleOffset - static_cast<double>(drawn.state.timer) / orb.birthScaleTicks, 0.0);
    }
    else if (drawn.state.state == OrbState::Ascent)
    {
      size = std::max(static_cast<double>(drawn.state.timer) / orb.ascentFadeTicks, 0.0);
      fade = size;
    }
    const std::array<float, 4> team = TeamColor(drawn.state.team);
    for (const auto& [half, alpha] : {std::pair{orb.innerSize, orb.innerAlpha}, std::pair{orb.outerSize, orb.outerAlpha}})
    {
      Neuron::SceneQuad quad;
      quad.position = drawn.position;
      quad.halfWidth = static_cast<float>(half * size);
      quad.halfHeight = quad.halfWidth;
      quad.color = Scaled(team, 1.0, alpha / FULL_ALPHA * fade);
      batch.quads.push_back(quad);
    }
  }
  if (!batch.quads.empty())
  {
    batch.cullRadius = static_cast<float>(orb.outerSize * std::max(orb.birthScaleOffset, 1.0));
    _batches.push_back(std::move(batch));
  }
  // ADR-140 (O77): an orb drifting home or waiting at its Wellspring is wrapped in a cloudy glow, which the others lack.
  const auto& cloudy = _presentation.mvp.orb;
  Neuron::QuadBatch clouds;
  clouds.pass = Neuron::ScenePass::Orbs;
  clouds.entry = "orb_additive";
  clouds.texture = static_cast<std::uint16_t>(SceneTexture::GlowCloudy);
  clouds.cullRadius = static_cast<float>(cloudy.cloudySize);
  for (const InterpolatedOrb& drawn : _orbs)
  {
    if (drawn.state.state == OrbState::Drifting || drawn.state.state == OrbState::Waiting)
    {
      Neuron::SceneQuad quad;
      quad.position = drawn.position;
      quad.halfWidth = static_cast<float>(cloudy.cloudySize);
      quad.halfHeight = quad.halfWidth;
      quad.color = Scaled(TeamColor(drawn.state.team), 1.0, cloudy.cloudyAlpha);
      clouds.quads.push_back(quad);
    }
  }
  if (!clouds.quads.empty())
  {
    _batches.push_back(std::move(clouds));
  }
}

} // namespace Species
