#pragma once

#include "FxRng.h"
#include "Vec3.h"
#include "WorldScene.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace Neuron
{

// VFX-203, VFX-217: the batch a particle type is drawn in.
enum class ParticleBlend : std::uint8_t
{
  Glow, // the additive batch
  Smoke // the darkening batch
};

/// A particle type (VFX-203, VFX-207): its life in seconds of the scaled clock, its type size, its gravity (wu/s²,
/// positive falling) and friction (per second), its two colours (RGB of 1), its batch and its priority class (VFX-212). A
/// type that names a puff type makes particles of it as it flies, as explosion debris does (VFX-210). No type of M1 has
/// a terrain rule (ADR-123).
struct ParticleType
{
  double lifeSeconds = 10.0;
  double size = 1000.0;
  double gravity = 0.0;
  double friction = 0.0;
  std::array<float, 3> color1{0.5f, 0.5f, 0.5f};
  std::array<float, 3> color2{0.95f, 0.95f, 0.95f};
  ParticleBlend blend = ParticleBlend::Glow;
  std::uint8_t priorityClass = 0;
  std::optional<std::uint8_t> puffType;
};

/// The pool's values (VFX-208 to VFX-217), from the client's presentation record.
struct ParticleLook
{
  double sizeDivisor = 16.0;          // vfx.particle.size_divisor: half-extent = size ÷ 16
  double subStepSeconds = 1.0 / 60.0; // vfx.particle.sub_step
  std::uint32_t maxSubSteps = 30;     // vfx.particle.max_sub_steps
  double tickSeconds = 0.1;           // ref.sim.tick_period: VFX-210's T
  double puffRate = 5.0;              // vfx.particle.debris.puff_rate, per s of scaled time
  double puffVelocityDivisor = 5.0;
  double puffSizeDivisor = 1.5;
  std::size_t capacity = 65'536; // vfx.particle.pool_capacity
  std::size_t typeCap = 16'384;  // vfx.particle.type_cap
  std::array<double, 4> classFillLimits{1.0, 0.95, 0.9, 0.8};
  double lodFullDistance = 4000.0; // vfx.particle.lod.*, wu
  double lodCutoffDistance = 8000.0;
  double alpha = 90.0;     // vfx.particle.alpha, of 255
  double fadeStart = 0.75; // vfx.particle.fade_start, times the life
  double smokeShadeDivisor = 100.0;
};

/// One particle to emit (VFX-202): its type, position and velocity, and its size and colour when it asks for them.
struct ParticleEmission
{
  std::uint8_t type = 0;
  Vec3 position;
  Vec3 velocity;
  std::optional<double> size;
  std::optional<std::array<float, 3>> color;
};

/// The client's particle pool (VFX-205 to VFX-217; ADR-010's M1 form, ADR-123): its live particles on the CPU, advanced
/// in fixed sub-steps of the scaled presentation clock and drawn in pass 15 as two batches of camera-facing diamonds of
/// fx.particle_dot. No simulation code reads it. Frame thread only.
class ParticlePool
{
public:
  ParticlePool(std::vector<ParticleType> _types, const ParticleLook& _look, std::uint64_t _seed);

  // VFX-202, VFX-212: false when the caps drop the emission, which is not retried. _scaledNow is its birth time.
  bool Emit(const ParticleEmission& _emission, double _scaledNow);
  // VFX-213: how many of a burst of _count particles to emit _distance wu from the camera, the fraction kept by a draw of
  // _rng.
  [[nodiscard]] std::uint32_t BurstCount(double _count, double _distance, FxRng& _rng) const noexcept;
  // VFX-209, VFX-210: the particles integrated to _scaledNow in fixed sub-steps, at most maxSubSteps a call, the remainder
  // carried; a particle whose life has passed is removed.
  void Advance(double _scaledNow);
  // VFX-215 to VFX-217: the smoke batch, then the glow batch, of the texture _texture (fx.particle_dot) at _scaledNow.
  void AppendBatches(double _scaledNow, std::uint16_t _texture, std::vector<QuadBatch>& _batches) const;
  void Clear() noexcept;

  [[nodiscard]] std::size_t Live() const noexcept
  {
    return m_particles.size();
  }
  [[nodiscard]] const ParticleLook& Look() const noexcept
  {
    return m_look;
  }

private:
  struct Particle
  {
    Vec3 position;
    Vec3 velocity;
    double birth = 0.0; // seconds of the scaled clock
    double size = 0.0;
    std::array<float, 3> color{1.0f, 1.0f, 1.0f};
    std::uint8_t type = 0;
  };

  void Step(double _subStepSeconds, std::vector<ParticleEmission>& _puffs);

  std::vector<ParticleType> m_types;
  ParticleLook m_look;
  FxRng m_rng; // the pool's own draws: emission colours and debris puffs
  std::vector<Particle> m_particles;
  std::vector<std::size_t> m_typeCounts;
  std::optional<double> m_simulated; // the scaled time the particles are integrated to
};

} // namespace Neuron
