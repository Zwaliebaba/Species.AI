#include "pch.h"

#include "ParticlePool.h"
#include "TestSupport.h"

#include <cmath>
#include <numbers>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// A glow type of class 0 and a smoke type of class 1, both of life 1 s and type size 16, with no gravity or friction.
std::vector<Neuron::ParticleType> TwoTypes()
{
  Neuron::ParticleType glow;
  glow.lifeSeconds = 1.0;
  glow.size = 16.0;
  glow.color1 = {1.0f, 0.5f, 0.0f};
  glow.color2 = {1.0f, 0.5f, 0.0f};
  Neuron::ParticleType smoke = glow;
  smoke.blend = Neuron::ParticleBlend::Smoke;
  smoke.priorityClass = 1;
  return {glow, smoke};
}

const Neuron::QuadBatch* BatchOf(const std::vector<Neuron::QuadBatch>& _batches, std::string_view _entry)
{
  for (const Neuron::QuadBatch& batch : _batches)
  {
    if (batch.entry == _entry)
    {
      return &batch;
    }
  }
  return nullptr;
}

} // namespace

TEST_CLASS(ParticlePoolTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx212TheClassFillLimitsAndTheTypeCapDropEmissions)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-212")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx212TheClassFillLimitsAndTheTypeCapDropEmissions)
  {
    Neuron::ParticleLook look;
    look.capacity = 10;
    look.typeCap = 8;
    look.classFillLimits = {1.0, 0.5, 0.5, 0.5};
    Neuron::ParticlePool pool(TwoTypes(), look, 1);
    std::size_t smoke = 0;
    for (int n = 0; n < 10; ++n)
    {
      smoke += pool.Emit({1, {}, {}, std::nullopt, std::nullopt}, 0.0) ? 1u : 0u;
    }
    Assert::IsTrue(smoke == 5, L"class 1 stops at half the capacity");
    std::size_t glow = 0;
    for (int n = 0; n < 10; ++n)
    {
      glow += pool.Emit({0, {}, {}, std::nullopt, std::nullopt}, 0.0) ? 1u : 0u;
    }
    Assert::IsTrue(glow == 5 && pool.Live() == 10, L"class 0 fills the pool");
    Assert::IsFalse(pool.Emit({7, {}, {}, std::nullopt, std::nullopt}, 0.0), L"an unknown type is dropped");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx213ABurstThinsWithItsDistance)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-213")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx213ABurstThinsWithItsDistance)
  {
    const Neuron::ParticlePool pool(TwoTypes(), Neuron::ParticleLook{}, 1);
    Neuron::FxRng rng(5);
    Assert::IsTrue(pool.BurstCount(10.0, 3'000.0, rng) == 10, L"in full within the full distance");
    Assert::IsTrue(pool.BurstCount(10.0, 6'000.0, rng) == 5, L"half way to the cut-off, half");
    Assert::IsTrue(pool.BurstCount(10.0, 9'000.0, rng) == 0, L"nothing beyond the cut-off");
    const std::uint32_t kept = pool.BurstCount(1.0, 6'000.0, rng);
    Assert::IsTrue(kept <= 1, L"a fraction is kept by a draw");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx210SubStepsIntegrateGravityScaledBySize)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-209")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-210")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx210SubStepsIntegrateGravityScaledBySize)
  {
    std::vector<Neuron::ParticleType> types = TwoTypes();
    types[0].lifeSeconds = 10.0;
    types[0].gravity = 10.0;
    Neuron::ParticlePool pool(types, Neuron::ParticleLook{}, 1);
    pool.Advance(0.0);
    Assert::IsTrue(pool.Emit({0, {0.0, 100.0, 0.0}, {6.0, 0.0, 0.0}, 32.0, std::nullopt}, 0.0), L"emitted at twice the type size");
    for (int frame = 1; frame <= 10; ++frame)
    {
      pool.Advance(0.1 * frame);
    }
    std::vector<Neuron::QuadBatch> batches;
    pool.AppendBatches(1.0, 0, batches);
    const Neuron::QuadBatch* glow = BatchOf(batches, "particle_additive");
    Assert::IsTrue(glow != nullptr && glow->quads.size() == 1, L"one glow particle");
    // 60 sub-steps of 1/60 s: x moves 6 wu; y falls by g × 2 × h² × n(n − 1) ÷ 2, since each step moves before it falls.
    const Neuron::Vec3 at = glow->quads[0].position;
    const double h = 1.0 / 60.0;
    const double fall = 10.0 * 2.0 * h * h * 60.0 * 59.0 / 2.0;
    Assert::IsTrue(std::abs(at.x - 6.0) < 1e-6, L"the horizontal flight");
    Assert::IsTrue(std::abs(at.y - (100.0 - fall)) < 1e-6, L"the fall, doubled by the size");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx215TheAlphaFadesOverTheLastQuarter)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-215")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-216")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-217")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx215TheAlphaFadesOverTheLastQuarter)
  {
    Neuron::ParticlePool pool(TwoTypes(), Neuron::ParticleLook{}, 1);
    pool.Advance(0.0);
    Assert::IsTrue(pool.Emit({0, {}, {}, std::nullopt, std::nullopt}, 0.0));
    Assert::IsTrue(pool.Emit({1, {}, {}, std::nullopt, std::nullopt}, 0.0));
    std::vector<Neuron::QuadBatch> batches;
    pool.AppendBatches(0.5, 7, batches);
    Assert::IsTrue(batches.size() == 2 && batches[0].entry == "particle_darkening" && batches[1].entry == "particle_additive",
                   L"the smoke batch first");
    Assert::IsTrue(batches[0].texture == 7 && batches[0].pass == Neuron::ScenePass::Particles, L"fx.particle_dot in pass 15");
    const Neuron::SceneQuad& smoke = batches[0].quads[0];
    Assert::IsTrue(std::abs(smoke.color[0] - 0.9f) < 1e-6f && smoke.color[3] == 0.0f, L"the shade RGB × 90 ÷ 100, alpha 0");
    const Neuron::SceneQuad& glow = batches[1].quads[0];
    Assert::IsTrue(std::abs(glow.color[3] - 90.0f / 255.0f) < 1e-6f, L"alpha 90 before the fade");
    // A diamond of half-extent size ÷ 16 = 1: the square turned by π ÷ 4, its half-side 1 ÷ √2.
    Assert::IsTrue(std::abs(glow.halfWidth - 1.0f / std::numbers::sqrt2_v<float>) < 1e-6f &&
                     std::abs(glow.rotationRadians - std::numbers::pi_v<float> / 4.0f) < 1e-6f,
                   L"the diamond");
    batches.clear();
    pool.AppendBatches(0.875, 7, batches);
    Assert::IsTrue(std::abs(batches[1].quads[0].color[3] - 45.0f / 255.0f) < 1e-6f, L"half way through the fade, 45");
    batches.clear();
    for (int frame = 1; frame <= 11; ++frame)
    {
      pool.Advance(0.1 * frame);
    }
    pool.AppendBatches(1.1, 7, batches);
    Assert::IsTrue(batches.empty() && pool.Live() == 0, L"removed at the end of its life");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx210DebrisPuffsIntoItsPuffType)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-210")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-211")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx210DebrisPuffsIntoItsPuffType)
  {
    std::vector<Neuron::ParticleType> types = TwoTypes();
    types[0].puffType = std::uint8_t{1};
    Neuron::ParticleLook look;
    look.puffRate = 120.0; // twice a sub-step: every sub-step puffs
    Neuron::ParticlePool pool(types, look, 1);
    pool.Advance(0.0);
    Assert::IsTrue(pool.Emit({0, {}, {10.0, 0.0, 0.0}, 30.0, std::nullopt}, 0.0));
    pool.Advance(1.0 / 60.0);
    std::vector<Neuron::QuadBatch> batches;
    pool.AppendBatches(1.0 / 60.0, 0, batches);
    const Neuron::QuadBatch* smoke = BatchOf(batches, "particle_darkening");
    Assert::IsTrue(pool.Live() == 2 && smoke != nullptr && smoke->quads.size() == 1, L"one puff of the smoke type");
    Assert::IsTrue(std::abs(smoke->quads[0].halfWidth - static_cast<float>(20.0 / 16.0 / std::numbers::sqrt2)) < 1e-5f,
                   L"the size ÷ the puff divisor");
  }
};

} // namespace NeuronClientTests
