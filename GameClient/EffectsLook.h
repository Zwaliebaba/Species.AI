#pragma once

#include "BattleVerbs.h"
#include "ClientSession.h"
#include "FxRng.h"
#include "ParticlePool.h"
#include "ReplicaTiming.h"
#include "StateReplica.h"
#include "TerrainField.h"
#include "TuningMvp.h"
#include "TuningPresentation.h"
#include "WorldScene.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Species
{

// ADR-123: the particle types M1 emits, by their index in the pool: the rows of VFX-207's table 4.2-A that its effects use.
enum class ParticleKind : std::uint8_t
{
  ExplosionCore,
  ExplosionDebris,
  MuzzleFlare, // vfx.particle.muzzle_flash.*
  ControlFlash,
  MissileTrail,
  WorkerFire,
  Spark, // vfx.particle.spark.* (ADR-141): a turret round's or a laser bolt's hit on a structure
  Count
};

// VFX-207: the types of ParticleKind from the record, with table 4.2-A's batches and classes.
[[nodiscard]] std::vector<Neuron::ParticleType> ParticleTypesOf(const TuningPresentation& _presentation);
[[nodiscard]] Neuron::ParticleLook ParticleLookOf(const TuningPresentation& _presentation, const TuningMvp& _tuning) noexcept;

/// What the effects read of the frame (ADR-123).
struct EffectsView
{
  Neuron::Vec3 camera;
  Neuron::Vec3 cameraFront{0.0, 0.0, -1.0}; // a unit vector
  double verticalFovRadians = 1.0;
  double displayTick = 0.0;   // the replica's display time in ticks: one tick behind the latest, plus the fraction
  double fraction = 0.0;      // the interpolation fraction between the replica's two latest states
  double scaledSeconds = 0.0; // the presentation clock's readings (VFX-016)
  double unscaledSeconds = 0.0;
  double unscaledStep = 0.0;
  double realSeconds = 0.0;
};

/// What a frame of the effects draws: pass 10a's grenades, flashes, rings and full-screen flash, pass 10b's laser bolts
/// and pass 15's particles, and the camera shake's view offset (VFX-232), which the camera never stores.
struct EffectDraws
{
  std::vector<Neuron::MeshBatch> meshes;
  std::vector<Neuron::QuadBatch> quads;
  std::vector<Neuron::TriangleBatch> triangles;
  double shakeYawRadians = 0.0;
  double shakeRollRadians = 0.0;
};

/// The weapons' effects, the screen effects and the particles of the client (M1 task T16c, ADR-123; R-CRATE-405,
/// R-CRATE-414, R-CRATE-445, R-CRATE-473, R-CRATE-475, RND-331 to RND-340, VFX-207 to VFX-235). Laser bolts and muzzle
/// flashes are rebuilt from the presentation events, each played as the display time reaches its tick; grenades and
/// shockwave rings are drawn from the replica, a ring's blast from a presentation copy that outlives it; the per-tick
/// emitters run once for each new tick of the replica. From M2 task T12 (ADR-141), the turret's rounds are drawn from the
/// replica with their tracers, and each shot's muzzle flash and ejected casing, and the sparks and dust of a hit, come from
/// its events. Nothing here reaches the simulation (DEC-007, DEC-013). Frame thread only.
class EffectsLook
{
public:
  // _presentation and _tuning outlive the look.
  EffectsLook(const TuningPresentation& _presentation, const TuningMvp& _tuning);

  // Once per frame, before the crowds: _events, taken from the session, join those waiting, and each is played once the
  // display time reaches its tick; each new tick of _replica runs the per-tick emitters on _figures, the frame's figures;
  // the particles advance to the frame's scaled time.
  void Update(std::span<const ReceivedEvent> _events, const StateReplica& _replica, std::span<const InterpolatedFigure> _figures,
              const EffectsView& _view);
  // O75, ADR-123: each Worker that fired within vfx.worker.recoil_time is drawn pushed back from its shot.
  void ApplyRecoil(std::vector<InterpolatedFigure>& _figures, const EffectsView& _view);
  // R-CTRL-370, VFX-220 row 1 (ADR-124): while the selection circle is held, its control flashes at each fixed effect step;
  // ADR-127: and while a released circle fades.
  void EmitCircle(const std::optional<SelectionCircle>& _circle, const Neuron::TerrainField& _field, const EffectsView& _view);
  // The frame's draws, after Update; _effects are the replica's effect objects at the frame's fraction.
  void Draw(std::span<const InterpolatedEffect> _effects, const Neuron::TerrainField& _field, const EffectsView& _view,
            EffectDraws& _draws);
  // A new world: every effect, particle and waiting event is dropped.
  void Clear();
  // ADR-141: the scene meshes, whose turret barrel set places each shot's casing; until they are given, a shot ejects none.
  void UseMeshes(std::shared_ptr<const Neuron::SceneMeshes> _meshes) noexcept;

  [[nodiscard]] std::size_t LiveBolts() const noexcept
  {
    return m_bolts.size();
  }
  [[nodiscard]] std::size_t LiveFlashes() const noexcept
  {
    return m_flashes.size();
  }
  [[nodiscard]] std::size_t LiveRings() const noexcept
  {
    return m_rings.size();
  }
  [[nodiscard]] std::size_t LiveCasings() const noexcept
  {
    return m_casings.size();
  }
  [[nodiscard]] const Neuron::ParticlePool& Particles() const noexcept
  {
    return m_particles;
  }
  [[nodiscard]] double Shake() const noexcept
  {
    return m_shake;
  }

private:
  struct Leg // a bolt's straight flight: at origin at the display tick originTick, moving at velocity
  {
    Neuron::Vec3 origin;
    Neuron::Vec3 velocity;
    double originTick = 0.0;
  };
  struct Bolt
  {
    StableId id = 0;
    std::uint8_t team = 0;
    double firedTick = 0.0;
    std::vector<Leg> legs;         // the first flight, and the flight after its ricochet
    std::optional<double> endTick; // the display tick at which it meets a structure (R-CRATE-441 step 6), when it does
  };
  struct RoundShot // a turret round's velocity, wu/s, and the display tick its shot's event played at
  {
    Neuron::Vec3 velocity;
    double firedTick = 0.0;
  };
  struct Casing // a shell casing (R-CRATE-455, R-CRATE-456), presentation alone
  {
    Neuron::Vec3 position;
    Neuron::Vec3 velocity; // wu/s
    Neuron::Vec3 front;    // its mesh's +z
    Neuron::Vec3 up;       // its mesh's +y
    Neuron::Vec3 spinAxis; // a unit vector
    double force = 1.0;
    double seconds = 0.0; // the scaled time it has moved to
  };
  struct Flash // a muzzle flash (R-CRATE-475)
  {
    Neuron::Vec3 origin;
    Neuron::Vec3 front;
    double startTick = 0.0;
    double size = 0.0;
    double life = 0.0; // tenths of a second
  };
  struct Ring // a shockwave ring's presentation copy (RND-333, RND-334)
  {
    StableId id = 0;
    Neuron::Vec3 position;
    double bornTick = 0.0;
    double sizeSeconds = 0.0;
  };
  struct Recoil
  {
    std::uint8_t team = 0;
    Neuron::Vec3 foot;      // the muzzle less the muzzle height
    Neuron::Vec3 direction; // the shot's horizontal direction
    double firedSeconds = 0.0;
    std::optional<StableId> shooter; // matched once
  };
  struct Waiting
  {
    ReceivedEvent received;
    std::uint32_t sequence = 0; // its order within its tick, for its generator's seed (VFX-011)
  };

  void Play(const Waiting& _waiting, const EffectsView& _view);
  void LaunchCasing(const EffectEvent& _event, Neuron::FxRng& _rng, const EffectsView& _view);
  void AdvanceCasings(const Neuron::TerrainField& _field, const EffectsView& _view);
  void RunTickEmitters(std::uint64_t _tick, const StateReplica& _replica, std::span<const InterpolatedFigure> _figures,
                       const EffectsView& _view);
  void EmitBurst(Neuron::FxRng& _rng, double _count, const Neuron::Vec3& _at, const EffectsView& _view, ParticleKind _kind,
                 const Neuron::Vec3& _velocity, std::optional<double> _size, std::optional<std::array<float, 3>> _color);
  void Emit(ParticleKind _kind, const Neuron::Vec3& _at, const Neuron::Vec3& _velocity, std::optional<double> _size,
            std::optional<std::array<float, 3>> _color, const EffectsView& _view);
  void DrawBolts(const Neuron::TerrainField& _field, const EffectsView& _view, EffectDraws& _draws, double& _shakeRequest);
  void DrawFlashes(const EffectsView& _view, EffectDraws& _draws);
  void DrawGrenades(std::span<const InterpolatedEffect> _effects, const EffectsView& _view, EffectDraws& _draws);
  void DrawRounds(std::span<const InterpolatedEffect> _effects, const EffectsView& _view, EffectDraws& _draws);
  void DrawCasings(EffectDraws& _draws) const;
  void DrawRings(const EffectsView& _view, EffectDraws& _draws, double& _flashAlpha, double& _shakeRequest);
  void DrawMarkers(std::span<const InterpolatedEffect> _effects, const EffectsView& _view, EffectDraws& _draws) const;
  void DrawTrailMarks(std::span<const InterpolatedEffect> _effects, const Neuron::TerrainField& _field, const EffectsView& _view,
                      EffectDraws& _draws);
  void DrawScreen(double _flashAlpha, double _shakeRequest, const EffectsView& _view, EffectDraws& _draws);

  const TuningPresentation* m_presentation;
  const TuningMvp* m_tuning;
  Neuron::ParticlePool m_particles;
  std::vector<Waiting> m_waiting;
  std::optional<std::uint64_t> m_sequenceTick; // the tick whose events are being numbered
  std::uint32_t m_sequence = 0;
  std::vector<Bolt> m_bolts;
  std::vector<Flash> m_flashes;
  std::vector<Ring> m_rings;
  std::vector<Recoil> m_recoils;
  std::vector<Casing> m_casings;
  std::uint64_t m_casingSerial = 0;       // R-CRATE-456's serial, which turns each casing's spin axis
  std::map<StableId, RoundShot> m_rounds; // each turret round's shot, from its event (R-CRATE-450)
  std::shared_ptr<const Neuron::SceneMeshes> m_meshes;
  std::map<StableId, bool> m_controlled;  // each Worker's crowd control at the last tick seen (VFX-219's ControlGained)
  std::map<StableId, double> m_trailBorn; // each order trail mark's display tick when first seen, from which its age runs
  std::optional<std::uint64_t> m_lastTick;
  double m_circleCarry = 0.0; // the unscaled time not yet stepped by the circle's emitter
  // The screen effects (VFX-232 to VFX-235).
  Neuron::FxRng m_screenRng;
  double m_shake = 0.0;
  double m_shakeYaw = 0.0;
  double m_shakeRoll = 0.0;
  double m_shakeCarry = 0.0;
  std::uint64_t m_shakeSteps = 0;
  std::vector<double> m_flashOnsets; // the real times of the flash onsets shown within the window, oldest first
  bool m_flashAbove = false;         // the last frame's flash reached the onset threshold
  bool m_flashSuppressed = false;    // its onset was refused, so it is not drawn until it falls below the threshold
};

} // namespace Species
