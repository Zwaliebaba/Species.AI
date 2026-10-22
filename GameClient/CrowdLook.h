#pragma once

#include "LooseContent.h"
#include "ReplicaTiming.h"
#include "TerrainField.h"
#include "WorldScene.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

struct TuningMvp;
struct TuningPresentation;

// ADR-121: the scene's textures, by their index in SceneTextures::files, in the order LoadSceneTextures reads them.
enum class SceneTexture : std::uint16_t
{
  WorkerFront, // sprite.worker.front
  WorkerSide,  // sprite.worker.side
  WorkerHurt01,
  WorkerHurt02,
  WorkerHurt03,
  WorkerHurt04,    // sprite.worker.hurt variant 04, the fixed hurt pose
  WorkerCarryLoad, // sprite.worker.carry_load (ADR-140), a stand-in until the owner's art (O88)
  GlowRound,       // fx.glow_round
  BannerIdle,      // marker.leader_banner and marker.leader_banner_tint, by R-UNIT-191's variant (ADR-122)
  BannerFollow,
  BannerFormation,
  BannerTintIdle,
  BannerTintFollow,
  BannerTintFormation,
  MuzzleFlash, // fx.muzzle_flash, and the effects of ADR-123
  Starburst,   // fx.starburst
  BeamShort,   // fx.beam_short
  ParticleDot, // fx.particle_dot
  OrderTrail,  // marker.order_trail, and the masks of ADR-125
  RouteArrow,  // marker.route_arrow
  FlagPointer, // marker.flag_pointer
  EdgeCompass, // marker.edge_compass
  EdgeCompassEmpty,
  EdgeCompassShadow,
  TeamDisc, // marker.team_disc
  TeamDiscShadow,
  IconLeader,  // icon.power.leader
  IconWorkers, // icon.power.workers
  GlowCloudy,  // fx.glow_cloudy (ADR-140): the orbs drifting home and the Wellspring's glow
  Count
};

// ADR-122: the scene's meshes, by their index in SceneMeshes::meshes.
enum class SceneMesh : std::uint16_t
{
  LeaderFigure, // mesh.unit.leader_figure
  ThrownWeapon, // mesh.projectile.thrown_weapon (ADR-123)
  // M2 task T12 (ADR-140): the structures' meshes of the mesh kit (ADR-128), in StructureKind's order, then the turret's
  // parts, its round and casing, and the port pad.
  Wellspring,          // mesh.structure.wellspring
  ResourceSite,        // mesh.structure.resource_site, member 1
  ResourceSiteCentral, // mesh.structure.resource_site, member 2
  Dwelling,            // mesh.structure.dwelling
  Storehouse,          // mesh.structure.storehouse
  TurretMount,         // mesh.structure.turret_mount: sockets only
  TurretHead,          // mesh.structure.turret_head
  TurretBarrelGun,     // mesh.structure.turret_barrel, member 1
  TurretRound,         // mesh.projectile.turret_round
  ShellCasing,         // mesh.projectile.shell_casing
  PortPad,             // mesh.structure.structure_default
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

// RND-216 (ADR-125): the direction from _from to _to on the screen, x to the camera's right and y up, from the points'
// projections when both lie ahead of the camera and from their difference on the view plane otherwise; (0, 0) when they
// coincide on the screen.
[[nodiscard]] std::array<double, 2> ScreenDirection(const Neuron::Vec3& _from, const Neuron::Vec3& _to, const Neuron::Vec3& _camera,
                                                    const Neuron::Vec3& _cameraFront) noexcept;

// RND-314: the high-detail factor of a Worker _distance wu from the camera, at entity detail 1, the option's default.
[[nodiscard]] double WorkerDetail(const TuningPresentation& _presentation, double _distance) noexcept;
// R-UNIT-142, RND-314: a Worker card's half-size, with the variation of its unit id and the distance scale.
[[nodiscard]] double WorkerCardHalfSize(const TuningPresentation& _presentation, StableId _id, double _distance) noexcept;

// VFX-117, VFX-118, RND-314 to RND-318, R-UNIT-142 to R-UNIT-147 and O75: the Worker layers of pass 7d, in their order:
// the ground shadows, the bodies, the side strips, the corpse fragments and the control glows, a batch per layer and
// card texture. _field gives the terrain's height and normal.
void AppendWorkerBatches(std::span<const InterpolatedFigure> _figures, const Neuron::TerrainField& _field, const CrowdView& _view,
                         const TuningPresentation& _presentation, const TuningMvp& _tuning, std::vector<Neuron::QuadBatch>& _batches);
// ADR-127: a disc of marker.team_disc on the ground under each of the local team's living Workers under control, in
// vfx.worker.control_glow_color at mvp.selection.disc_alpha, of mvp.selection.disc_size_factor × its card's half-size,
// in pass 7d before the Worker layers and with no depth test, so that the cards drawn after cover it and no other card
// hides it.
void AppendSelectionDiscs(std::span<const InterpolatedFigure> _figures, const Neuron::TerrainField& _field, const CrowdView& _view,
                          const TuningPresentation& _presentation, std::vector<Neuron::QuadBatch>& _batches);

// R-UNIT-330, R-UNIT-331, VFX-225, O77: the Essence orbs of pass 11, an inner and an outer quad of fx.glow_round each;
// ADR-140: and an orb drifting home or waiting at the Wellspring a quad of fx.glow_cloudy about them.
void AppendOrbBatches(std::span<const InterpolatedOrb> _orbs, const TuningPresentation& _presentation,
                      std::vector<Neuron::QuadBatch>& _batches);

} // namespace Species
