#include "pch.h"

#include "DevelopmentOverlay.h"
#include "FigureKind.h"
#include "FxRng.h"
#include "LeaderLook.h"
#include "StableId.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace Species
{

namespace
{

constexpr Neuron::Vec3 WORLD_UP{0.0, 1.0, 0.0};
constexpr std::int64_t MAX_CLOTH_STEPS = 30; // a frame's cloth steps at most; a clamped frame of 0.25 s holds 15 (VFX-016)
constexpr double BANNER_PHASE_PER_ID = 10.0; // VFX-142: t + unit id × 10
constexpr float SQRT_TWO = 1.41421356f;      // a turned square's half-diagonal over its half-side

Neuron::Vec3 Cross(const Neuron::Vec3& _a, const Neuron::Vec3& _b) noexcept
{
  return {_a.y * _b.z - _a.z * _b.y, _a.z * _b.x - _a.x * _b.z, _a.x * _b.y - _a.y * _b.x};
}

Neuron::Vec3 Rotated(const Neuron::Vec3& _v, const Neuron::Vec3& _axis, double _radians) noexcept
{
  const double c = std::cos(_radians);
  const double s = std::sin(_radians);
  return _v * c + Cross(_axis, _v) * s + _axis * (Neuron::Dot(_axis, _v) * (1.0 - c));
}

// R-UNIT-191: the banner sits at the figure's flag socket; a mesh without one carries it at the top of its root.
Neuron::Vec3 FlagSocketOf(const Neuron::MeshAsset& _mesh) noexcept
{
  for (const Neuron::MeshSocket& socket : _mesh.sockets)
  {
    if (socket.kind == Neuron::SocketKind::Flag)
    {
      return {socket.transform[12], socket.transform[13], socket.transform[14]};
    }
  }
  return _mesh.nodes.empty() ? Neuron::Vec3{} : Neuron::Vec3{0.0, _mesh.nodes.front().boundsMax[1], 0.0};
}

// R-UNIT-191: the banner's variant, idle for None, PrepareWaypoint and Waypoint, follow for Follow, formation in formation
// mode; its tint mask is the matching one.
SceneTexture BannerTextureOf(const ReplicatedFigure& _figure) noexcept
{
  if (_figure.formation)
  {
    return SceneTexture::BannerFormation;
  }
  return _figure.order == static_cast<std::uint8_t>(LeaderOrder::Follow) ? SceneTexture::BannerFollow : SceneTexture::BannerIdle;
}

SceneTexture TintMaskOf(SceneTexture _banner) noexcept
{
  return static_cast<SceneTexture>(static_cast<std::uint16_t>(_banner) + static_cast<std::uint16_t>(SceneTexture::BannerTintIdle) -
                                   static_cast<std::uint16_t>(SceneTexture::BannerIdle));
}

} // namespace

void LeaderLook::Draw(std::span<const InterpolatedFigure> _figures, const Neuron::MeshAsset& _figure, const CrowdView& _view,
                      double _unscaledStep, const TuningPresentation& _presentation, const TuningMvp& _tuning, LeaderDraws& _draws)
{
  const auto& worker = _presentation.ref.unit.worker;
  const auto& leader = _presentation.ref.unit.leader;
  const auto& flag = _presentation.ref.bldg.clothFlag;
  const auto& cloth = _presentation.vfx.cloth;
  const auto& flicker = _presentation.ref.unit.flicker;
  const auto& flickerLook = _presentation.vfx.flicker;
  const auto resolution = static_cast<std::size_t>(std::max<std::int64_t>(flag.resolution, 2));
  const double columns = static_cast<double>(resolution - 1);
  const double stepSeconds = 1.0 / _presentation.vfx.emit.stepRate;
  // VFX-018: the cloth's 60 Hz steps that this frame's unscaled step holds, the remainder carried.
  m_stepCarry += std::max(_unscaledStep, 0.0);
  const auto due = static_cast<std::int64_t>(std::floor(m_stepCarry / stepSeconds));
  m_stepCarry -= static_cast<double>(due) * stepSeconds;
  const std::int64_t steps = std::min(due, MAX_CLOTH_STEPS);
  const Neuron::Vec3 cameraRight = Neuron::NormalizedOr(Cross(_view.cameraFront, WORLD_UP), {1.0, 0.0, 0.0});
  const Neuron::Vec3 socketModel = FlagSocketOf(_figure);
  const double maxHealth = std::max(static_cast<double>(_tuning.ref.unit.leader.health), 1.0);

  Neuron::MeshBatch bodies{Neuron::ScenePass::Actors, "actor_mesh_lit", static_cast<std::uint16_t>(SceneMesh::LeaderFigure), 0.0f, {}};
  Neuron::MeshBatch flickers = bodies;
  flickers.entry = "actor_flicker_additive";
  Neuron::QuadBatch poles;
  poles.pass = Neuron::ScenePass::Actors;
  poles.entry = "cloth_pole";
  poles.texture = static_cast<std::uint16_t>(SceneTexture::GlowRound); // the pole reads no texture (VFX-141)
  Neuron::QuadBatch pointers;
  pointers.pass = Neuron::ScenePass::Pointers;
  pointers.entry = "cloth_pointer";
  pointers.texture = static_cast<std::uint16_t>(SceneTexture::FlagPointer);
  std::array<Neuron::TriangleBatch, 3> banners{};
  for (std::size_t n = 0; n < banners.size(); ++n)
  {
    const auto texture = static_cast<SceneTexture>(static_cast<std::size_t>(SceneTexture::BannerIdle) + n);
    banners[n] = {
      Neuron::ScenePass::Actors, "cloth_cells", static_cast<std::uint16_t>(texture), static_cast<std::uint16_t>(TintMaskOf(texture)), {}};
  }
  for (auto& [id, kept] : m_leaders)
  {
    kept.drawn = false;
  }

  double largestScale = 1.0;
  double largestBanner = 0.0;
  for (const InterpolatedFigure& figure : _figures)
  {
    const ReplicatedFigure& state = figure.state;
    if (state.kind != FigureKind::Leader)
    {
      continue;
    }
    Leader& kept = m_leaders[figure.id];
    kept.drawn = true;
    const double distance = Neuron::Length(figure.position - _view.camera);
    const std::uint64_t counter = CounterOf(figure.id);
    // R-UNIT-190: scaled with the camera's distance and turned towards the camera, as a Worker card is (R-UNIT-142).
    const double scale =
      distance > worker.cardScaleStart ? 1.0 + _view.workerScaleFactor * (distance - worker.cardScaleStart) / worker.cardScaleDivisor : 1.0;
    largestScale = std::max(largestScale, scale);
    const Neuron::Vec3 heading{std::sin(figure.yawRadians), 0.0, std::cos(figure.yawRadians)};
    const Neuron::Vec3 toCamera =
      Neuron::NormalizedOr({_view.camera.x - figure.position.x, 0.0, _view.camera.z - figure.position.z}, heading);
    const double face = std::clamp((distance - worker.cardFaceStart) / worker.cardScaleDivisor, 0.0, 1.0);
    const Neuron::Vec3 front = Neuron::NormalizedOr(heading * (1.0 - face) + toCamera * face, heading);
    Neuron::SceneMeshInstance body;
    body.position = figure.position;
    body.axisZ = front * scale;
    body.axisY = WORLD_UP * scale;
    body.axisX = Cross(WORLD_UP, front) * scale;
    const std::array<float, 4> team = TeamColor(state.team);
    // R-UNIT-044, RND-161, RND-271: the whole-model tint, the team colour × the tint factor.
    const auto factor = static_cast<float>(_presentation.rnd.tint.factor);
    body.tint = {team[0] * factor, team[1] * factor, team[2] * factor};
    body.tintWeight = 1.0f;

    // VFX-126, VFX-127: the damage flicker, chosen once a flicker step; full health never flickers.
    const double t = _view.scaledSeconds;
    const double wave = std::sin(t + flicker.phasePerId * static_cast<double>(counter));
    const auto step = static_cast<std::int64_t>(std::floor(t / flickerLook.step));
    if (step != kept.flickerStep)
    {
      kept.flickerStep = step;
      Neuron::FxRng rng(Neuron::FxSeed(static_cast<std::uint64_t>(std::max<std::int64_t>(step, 0)), figure.id, 0));
      kept.flickering = rng.NextUnit(flicker.chanceScale) * (1.0 - std::abs(wave) * flicker.sineWeight) > figure.health / maxHealth;
      const double pick = rng.NextUnit(1.0);
      const auto& shares = flickerLook.axisShares;
      kept.flickerAxis = static_cast<std::uint8_t>(pick < shares.front ? 0 : (pick < shares.front + shares.up ? 1 : 2));
    }
    if (kept.flickering && !state.dead)
    {
      Neuron::SceneMeshInstance flickering = body;
      if (kept.flickerAxis == 0)
      {
        flickering.axisZ = body.axisZ * (1.0 - wave * flickerLook.gain);
      }
      else if (kept.flickerAxis == 1)
      {
        flickering.axisY = body.axisY * (1.0 - wave * flickerLook.upGain);
      }
      else
      {
        flickering.axisX = body.axisX * (1.0 - wave * flickerLook.gain);
      }
      flickers.instances.push_back(flickering);
    }
    else
    {
      bodies.instances.push_back(body);
    }
    if (state.dead)
    {
      kept.cloth.clear(); // O77: the corpse sinks, with no shatter and no banner
      continue;
    }

    // R-UNIT-191, VFX-142: the banner at the flag socket, grown with the distance, streaming behind the Leader and turned
    // towards the camera, flipped where the turn would show it mirrored.
    const Neuron::Vec3 socket = figure.position + body.axisX * socketModel.x + body.axisY * socketModel.y + body.axisZ * socketModel.z;
    const double size =
      leader.bannerSize * (distance > worker.cardScaleStart
                             ? 1.0 + _view.workerScaleFactor * (distance - worker.cardScaleStart) / leader.bannerGrowthDivisor
                             : 1.0);
    largestBanner = std::max(largestBanner, size);
    const Neuron::Vec3 streaming = heading * -1.0;
    const Neuron::Vec3 side = cameraRight * (Neuron::Dot(streaming, cameraRight) >= 0.0 ? 1.0 : -1.0);
    const double turn = std::min(distance / leader.bannerFaceDivisor, 1.0);
    const Neuron::Vec3 bannerFront = Neuron::NormalizedOr(streaming * (1.0 - turn) + side * turn, streaming);
    Neuron::Vec3 up = WORLD_UP;
    if (state.order != static_cast<std::uint8_t>(LeaderOrder::None))
    {
      const double phase = _view.unscaledSeconds + static_cast<double>(counter) * BANNER_PHASE_PER_ID;
      up = Rotated(up, bannerFront, cloth.bannerWobble * std::sin(cloth.bannerWobbleRate * phase));
    }
    const bool highlighted = state.selected;
    const bool damped = size > flag.dampedSizeWu || highlighted;
    const double wobble = highlighted ? flag.highlightWobble : 1.0;
    const auto at = [resolution](std::size_t _x, std::size_t _z) { return _x * resolution + _z; };
    // VFX-139: each vertex relaxes towards its target, column by column from the pole, its links at most half the size.
    const auto relax = [&](double _t, bool _whole)
    {
      for (std::size_t x = 0; x < resolution; ++x)
      {
        const double column = static_cast<double>(x);
        const double k = _whole || x == 0 ? 1.0
                         : damped         ? 1.0 - std::pow(1.0 - flag.dampedFactor, _presentation.vfx.emit.stepRate * stepSeconds)
                                  : stepSeconds * flag.relaxRate * static_cast<double>(resolution - x) / static_cast<double>(resolution);
        for (std::size_t z = 0; z < resolution; ++z)
        {
          Neuron::Vec3 target;
          if (x == 0)
          {
            target = socket + up * (size * 0.5 + size * static_cast<double>(z) / columns);
          }
          else
          {
            const double reach = wobble * column / static_cast<double>(resolution);
            target = kept.cloth[at(x - 1, z)] + bannerFront * (size / columns) +
                     Neuron::Vec3{reach * std::cos(_t + cloth.wobblePhases.first * column),
                                  cloth.liftGain * reach * std::sin(_t + cloth.wobblePhases.second * column),
                                  reach * std::cos(_t + cloth.wobblePhases.third * column)};
          }
          Neuron::Vec3& vertex = kept.cloth[at(x, z)];
          vertex = vertex + (target - vertex) * k;
          if (x > 0)
          {
            const Neuron::Vec3 link = vertex - kept.cloth[at(x - 1, z)];
            const double length = Neuron::Length(link);
            if (length > size * 0.5)
            {
              vertex = kept.cloth[at(x - 1, z)] + link * (size * 0.5 / length);
            }
          }
        }
      }
    };
    if (kept.cloth.size() != resolution * resolution)
    {
      kept.cloth.assign(resolution * resolution, socket);
      relax(_view.unscaledSeconds, true);
    }
    for (std::int64_t n = 0; n < steps; ++n)
    {
      relax(_view.unscaledSeconds - static_cast<double>(steps - 1 - n) * stepSeconds, false);
    }

    // VFX-141: the white pole, then the cloth's cells, its team area in the team colour × 0.5, whole while selected.
    Neuron::SceneQuad pole;
    pole.position = socket;
    pole.orientation = Neuron::QuadOrientation::UprightCard;
    pole.anchor = Neuron::QuadAnchor::BottomCenter;
    pole.halfWidth = static_cast<float>(flag.poleWidthWu * 0.5);
    pole.halfHeight = static_cast<float>(size * flag.poleHeightFactor * 0.5);
    poles.quads.push_back(pole);
    const float tintScale = highlighted ? 1.0f : static_cast<float>(cloth.teamTint);
    const std::array<float, 4> tint{team[0] * tintScale, team[1] * tintScale, team[2] * tintScale, 1.0f}; // a Leader always has a team
    const bool flip = Neuron::Dot(bannerFront, cameraRight) < 0.0;
    std::vector<Neuron::SceneVertex>& cells =
      banners[static_cast<std::size_t>(BannerTextureOf(state)) - static_cast<std::size_t>(SceneTexture::BannerIdle)].vertices;
    const auto corner = [&](std::size_t _x, std::size_t _z)
    {
      const auto u = static_cast<float>(static_cast<double>(_x) / columns);
      const auto v = static_cast<float>(1.0 - static_cast<double>(_z) / columns);
      return Neuron::SceneVertex{kept.cloth[at(_x, _z)], {flip ? 1.0f - u : u, v}, tint};
    };
    for (std::size_t x = 0; x + 1 < resolution; ++x)
    {
      for (std::size_t z = 0; z + 1 < resolution; ++z)
      {
        for (const auto& [cx, cz] :
             {std::pair{x, z}, std::pair{x + 1, z}, std::pair{x + 1, z + 1}, std::pair{x, z}, std::pair{x + 1, z + 1}, std::pair{x, z + 1}})
        {
          cells.push_back(corner(cx, cz));
        }
      }
    }
    // VFX-141, R-UNIT-191 (ADR-125): a Waypoint banner carries the pointer over its cloth, turned so that its arrow, which
    // points down in its art, points towards the screen position of its order route's first waypoint (O78).
    if (state.order == static_cast<std::uint8_t>(LeaderOrder::Waypoint) && state.target.has_value())
    {
      Neuron::Vec3 center{};
      for (const Neuron::Vec3& vertex : kept.cloth)
      {
        center = center + vertex;
      }
      center = center * (1.0 / static_cast<double>(kept.cloth.size()));
      const std::array<double, 2> towards = ScreenDirection(center, PositionOf(*state.target), _view.camera, _view.cameraFront);
      Neuron::SceneQuad pointer;
      pointer.position = center;
      pointer.halfWidth = static_cast<float>(size * cloth.pointerSizeFactor);
      pointer.halfHeight = pointer.halfWidth;
      pointer.rotationRadians = static_cast<float>(std::atan2(towards[0], -towards[1]));
      pointers.quads.push_back(pointer);
      pointers.cullRadius = std::max(pointers.cullRadius, pointer.halfWidth * SQRT_TWO);
    }
  }
  std::erase_if(m_leaders, [](const auto& _entry) { return !_entry.second.drawn; });

  const auto radius = static_cast<float>(_tuning.mvp.unit.leaderMeshRadius * largestScale);
  for (Neuron::MeshBatch* batch : {&bodies, &flickers})
  {
    if (!batch->instances.empty())
    {
      batch->cullRadius = radius;
      _draws.meshes.push_back(std::move(*batch));
    }
  }
  if (!poles.quads.empty())
  {
    poles.cullRadius = static_cast<float>(largestBanner * flag.poleHeightFactor);
    _draws.quads.push_back(std::move(poles));
  }
  for (Neuron::TriangleBatch& batch : banners)
  {
    if (!batch.vertices.empty())
    {
      _draws.triangles.push_back(std::move(batch));
    }
  }
  if (!pointers.quads.empty())
  {
    _draws.quads.push_back(std::move(pointers));
  }
}

} // namespace Species
