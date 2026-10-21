#include "pch.h"

#include "ParticlePool.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string_view>
#include <utility>

namespace Neuron
{

namespace
{

// VFX-217: the two batches' catalogue entries.
constexpr std::string_view SMOKE_ENTRY = "particle_darkening";
constexpr std::string_view GLOW_ENTRY = "particle_additive";

} // namespace

ParticlePool::ParticlePool(std::vector<ParticleType> _types, const ParticleLook& _look, std::uint64_t _seed)
  : m_types(std::move(_types)),
    m_look(_look),
    m_rng(_seed),
    m_typeCounts(m_types.size(), 0)
{
}

bool ParticlePool::Emit(const ParticleEmission& _emission, double _scaledNow)
{
  if (_emission.type >= m_types.size())
  {
    return false;
  }
  const ParticleType& type = m_types[_emission.type];
  // VFX-212: a particle of class c only while the live count is below the capacity × the class's fill limit, and its
  // type below its cap.
  const double limit = m_look.classFillLimits[std::min<std::size_t>(type.priorityClass, m_look.classFillLimits.size() - 1)];
  if (static_cast<double>(m_particles.size()) >= static_cast<double>(m_look.capacity) * limit ||
      m_typeCounts[_emission.type] >= m_look.typeCap)
  {
    return false;
  }
  Particle particle;
  particle.position = _emission.position;
  particle.velocity = _emission.velocity;
  particle.birth = _scaledNow;
  particle.size = _emission.size.value_or(type.size);
  particle.type = _emission.type;
  if (_emission.color.has_value())
  {
    particle.color = *_emission.color;
  }
  else
  {
    // VFX-202: colour 1 × a + colour 2 × (1 − a), a drawn once at emission.
    const auto a = static_cast<float>(m_rng.NextUnit(1.0));
    for (std::size_t c = 0; c < 3; ++c)
    {
      particle.color[c] = type.color1[c] * a + type.color2[c] * (1.0f - a);
    }
  }
  m_particles.push_back(particle);
  ++m_typeCounts[_emission.type];
  return true;
}

std::uint32_t ParticlePool::BurstCount(double _count, double _distance, FxRng& _rng) const noexcept
{
  const double span = m_look.lodCutoffDistance - m_look.lodFullDistance;
  const double scale =
    span > 0.0 ? std::clamp((m_look.lodCutoffDistance - _distance) / span, 0.0, 1.0) : (_distance < m_look.lodCutoffDistance ? 1.0 : 0.0);
  const double scaled = std::max(0.0, _count) * scale;
  const double whole = std::floor(scaled);
  // The fractional part kept by a presentation draw.
  const bool extra = scaled - whole > 0.0 && _rng.NextUnit(1.0) < scaled - whole;
  return static_cast<std::uint32_t>(whole) + (extra ? 1u : 0u);
}

void ParticlePool::Step(double _subStepSeconds, std::vector<ParticleEmission>& _puffs)
{
  const double h = _subStepSeconds;
  const double t = m_look.tickSeconds;
  const double now = *m_simulated;
  for (std::size_t n = 0; n < m_particles.size();)
  {
    Particle& particle = m_particles[n];
    const ParticleType& type = m_types[particle.type];
    if (now >= particle.birth + type.lifeSeconds)
    {
      --m_typeCounts[particle.type];
      particle = m_particles.back();
      m_particles.pop_back();
      continue;
    }
    // VFX-210: the reference's per-tick friction factor, per sub-step, and gravity scaled by the size.
    particle.position = particle.position + particle.velocity * h;
    particle.velocity = particle.velocity * std::pow(std::max(0.0, 1.0 - t * type.friction), h / t);
    if (type.gravity != 0.0)
    {
      particle.velocity.y -= h * type.gravity * (particle.size / type.size);
    }
    if (type.puffType.has_value() && m_rng.NextUnit(1.0) < m_look.puffRate * h)
    {
      _puffs.push_back({*type.puffType, particle.position, particle.velocity * (1.0 / m_look.puffVelocityDivisor),
                        particle.size / m_look.puffSizeDivisor, std::nullopt});
    }
    ++n;
  }
}

void ParticlePool::Advance(double _scaledNow)
{
  if (!m_simulated.has_value())
  {
    m_simulated = _scaledNow;
    return;
  }
  const double h = m_look.subStepSeconds;
  std::vector<ParticleEmission> puffs;
  for (std::uint32_t step = 0; step < m_look.maxSubSteps && *m_simulated + h <= _scaledNow; ++step)
  {
    *m_simulated += h;
    Step(h, puffs);
    // VFX-211: a puff joins the pool as any emission does, at the sub-step's time.
    for (const ParticleEmission& puff : puffs)
    {
      static_cast<void>(Emit(puff, *m_simulated));
    }
    puffs.clear();
  }
  // A frame longer than the sub-steps allow drops the excess rather than carrying it (VFX-209).
  m_simulated = std::max(*m_simulated, _scaledNow - h);
}

void ParticlePool::AppendBatches(double _scaledNow, std::uint16_t _texture, std::vector<QuadBatch>& _batches) const
{
  QuadBatch smoke{ScenePass::Particles, SMOKE_ENTRY, _texture, 0.0f, 0.0f, {}};
  QuadBatch glow{ScenePass::Particles, GLOW_ENTRY, _texture, 0.0f, 0.0f, {}};
  const double remainder = m_simulated.has_value() ? std::max(0.0, _scaledNow - *m_simulated) : 0.0;
  for (const Particle& particle : m_particles)
  {
    const ParticleType& type = m_types[particle.type];
    const double age = _scaledNow - particle.birth;
    if (age >= type.lifeSeconds)
    {
      continue;
    }
    // VFX-215: the alpha of 255 until the fade start, then a linear fade, truncated and floored at 0.
    double alpha = m_look.alpha;
    const double fadeFrom = m_look.fadeStart * type.lifeSeconds;
    if (age >= fadeFrom)
    {
      const double fadeLength = (1.0 - m_look.fadeStart) * type.lifeSeconds;
      alpha = fadeLength > 0.0 ? std::max(0.0, std::trunc(m_look.alpha - m_look.alpha * (age - fadeFrom) / fadeLength)) : 0.0;
    }
    if (alpha <= 0.0)
    {
      continue;
    }
    // VFX-216: a diamond of half-extent size ÷ the divisor, the camera-facing square turned a quarter of a right angle.
    const double halfExtent = particle.size / m_look.sizeDivisor;
    SceneQuad quad;
    quad.position = particle.position + particle.velocity * remainder;
    quad.halfWidth = static_cast<float>(halfExtent / std::numbers::sqrt2);
    quad.halfHeight = quad.halfWidth;
    quad.rotationRadians = static_cast<float>(std::numbers::pi / 4.0);
    QuadBatch& batch = type.blend == ParticleBlend::Smoke ? smoke : glow;
    if (type.blend == ParticleBlend::Smoke)
    {
      // VFX-217: the shade RGB × alpha ÷ the divisor, with alpha 0, so the destination is darkened by it.
      const auto shade = static_cast<float>(alpha / m_look.smokeShadeDivisor);
      quad.color = {particle.color[0] * shade, particle.color[1] * shade, particle.color[2] * shade, 0.0f};
    }
    else
    {
      quad.color = {particle.color[0], particle.color[1], particle.color[2], static_cast<float>(alpha / 255.0)};
    }
    batch.cullRadius = std::max(batch.cullRadius, static_cast<float>(halfExtent));
    batch.quads.push_back(quad);
  }
  for (QuadBatch* batch : {&smoke, &glow})
  {
    if (!batch->quads.empty())
    {
      _batches.push_back(std::move(*batch));
    }
  }
}

void ParticlePool::Clear() noexcept
{
  m_particles.clear();
  std::ranges::fill(m_typeCounts, std::size_t{0});
  m_simulated.reset();
}

} // namespace Neuron
