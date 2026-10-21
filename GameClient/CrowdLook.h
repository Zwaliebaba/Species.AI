#pragma once

#include "LooseContent.h"
#include "ReplicaTiming.h"
#include "TerrainField.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "WorldScene.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

// ADR-121: the scene's textures, by their index in SceneTextures::files, in the order LoadSceneTextures reads them.
enum class SceneTexture : std::uint16_t
{
  WorkerFront, // sprite.worker.front
  WorkerSide,  // sprite.worker.side
  WorkerHurt01,
  WorkerHurt02,
  WorkerHurt03,
  WorkerHurt04, // sprite.worker.hurt variant 04, the fixed hurt pose
  GlowRound,    // fx.glow_round
  BannerIdle,   // marker.leader_banner and marker.leader_banner_tint, by R-UNIT-191's variant (ADR-122)
  BannerFollow,
  BannerFormation,
  BannerTintIdle,
  BannerTintFollow,
  BannerTintFormation,
  MuzzleFlash, // fx.muzzle_flash, and the effects of ADR-123
  Starburst,   // fx.starburst
  BeamShort,   // fx.beam_short
  ParticleDot, // fx.particle_dot
  Count
};

// ADR-122: the scene's meshes, by their index in SceneMeshes::meshes.
enum class SceneMesh : std::uint16_t
{
  LeaderFigure, // mesh.unit.leader_figure
  ThrownWeapon, // mesh.projectile.thrown_weapon (ADR-123)
  Count
};

// The scene's textures from the content: none when one is missing or damaged, which is logged.
[[nodiscard]] std::shared_ptr<const Neuron::SceneTextures> LoadSceneTextures(const Neuron::LooseContent& _content);
// The scene's cooked meshes from the content: none when one is missing or damaged, which is logged.
[[nodiscard]] std::shared_ptr<const Neuron::SceneMeshes> LoadSceneMeshes(const Neuron::LooseContent& _content);

/// What the crowd's look reads of the frame (ADR-121).
struct CrowdView
{
  Neuron::Vec3 camera;
  Neuron::Vec3 cameraFront{0.0, 0.0, -1.0}; // a unit vector
  double scaledSeconds = 0.0;               // the presentation clock's readings (VFX-009)
  double unscaledSeconds = 0.0;
  std::optional<std::uint8_t> localTeam; // the team the client commands
};

// RND-314: the high-detail factor of a Worker _distance wu from the camera, at entity detail 1, the option's default.
[[nodiscard]] double WorkerDetail(const TuningPresentation& _presentation, double _distance) noexcept;
// R-UNIT-142, RND-314: a Worker card's half-size, with the variation of its unit id and the distance scale.
[[nodiscard]] double WorkerCardHalfSize(const TuningPresentation& _presentation, StableId _id, double _distance) noexcept;

// VFX-117, VFX-118, RND-314 to RND-318, R-UNIT-142 to R-UNIT-147 and O75: the Worker layers of pass 7d, in their order:
// the ground shadows, the bodies, the side strips, the corpse fragments and the control glows, a batch per layer and
// card texture. _field gives the terrain's height and normal.
void AppendWorkerBatches(std::span<const InterpolatedFigure> _figures, const Neuron::TerrainField& _field, const CrowdView& _view,
                         const TuningPresentation& _presentation, const TuningMvp& _tuning, std::vector<Neuron::QuadBatch>& _batches);

// R-UNIT-330, R-UNIT-331, VFX-225, O77: the Essence orbs of pass 11, an inner and an outer quad of fx.glow_round each.
void AppendOrbBatches(std::span<const InterpolatedOrb> _orbs, const TuningPresentation& _presentation,
                      std::vector<Neuron::QuadBatch>& _batches);

} // namespace Species
