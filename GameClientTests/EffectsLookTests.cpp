#include "pch.h"

#include "CrowdLook.h"
#include "DepthBias.h"
#include "DevelopmentOverlay.h"
#include "EffectKind.h"
#include "EffectsLook.h"
#include "RegionTerrain.h"
#include "StableId.h"
#include "StateUpdate.h"
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <string_view>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

const Species::RegionTerrain& SeedOneTerrain()
{
  static const Species::RegionTerrain REGION = []
  {
    auto generated = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(generated.has_value(), L"seed 1 gives a region that passes");
    return generated ? std::move(*generated) : Species::RegionTerrain{};
  }();
  return REGION;
}

// The steward's site on the valley's floor.
Neuron::Vec3 Ground()
{
  const Species::RegionTerrain& terrain = SeedOneTerrain();
  Neuron::Vec3 site = terrain.layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.stewardSite)];
  site.y = terrain.field.Height(site.x, site.z);
  return site;
}

Neuron::Vec3 Air()
{
  return Ground() + Neuron::Vec3{0.0, 30.0, 0.0};
}

Species::StableId IdOf(std::uint64_t _counter)
{
  return Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter);
}

// A view of _target from 200 wu along +z and 50 wu up, at the display tick _tick; the clocks run at the tick's rate.
Species::EffectsView ViewAt(const Neuron::Vec3& _target, double _tick)
{
  Species::EffectsView view;
  view.camera = _target + Neuron::Vec3{0.0, 50.0, 200.0};
  view.cameraFront = Neuron::Normalized(_target - view.camera);
  view.verticalFovRadians = 1.0;
  view.displayTick = _tick;
  view.scaledSeconds = 0.1 * _tick;
  view.unscaledSeconds = 0.1 * _tick;
  view.realSeconds = 0.1 * _tick;
  view.unscaledStep = 1.0 / 60.0;
  return view;
}

Species::ReceivedEvent EventOf(Species::EffectEventKind _kind, std::uint64_t _tick, std::uint64_t _subject, const Neuron::Vec3& _at,
                               const Neuron::Vec3& _vector, double _value = 0.0)
{
  return {_tick, {_kind, 0, IdOf(_subject), _at, _vector, _value}};
}

std::size_t QuadsOf(const std::vector<Neuron::QuadBatch>& _batches, std::string_view _entry, Species::SceneTexture _texture)
{
  std::size_t count = 0;
  for (const Neuron::QuadBatch& batch : _batches)
  {
    if (batch.entry == _entry && batch.texture == static_cast<std::uint16_t>(_texture))
    {
      count += batch.quads.size();
    }
  }
  return count;
}

const Neuron::QuadBatch* BatchOf(const std::vector<Neuron::QuadBatch>& _batches, std::string_view _entry)
{
  const auto found = std::ranges::find(_batches, _entry, &Neuron::QuadBatch::entry);
  return found != _batches.end() ? &*found : nullptr;
}

const Neuron::TriangleBatch* ScreenFlashOf(const Species::EffectDraws& _draws)
{
  const auto found = std::ranges::find(_draws.triangles, true, &Neuron::TriangleBatch::screenSpace);
  return found != _draws.triangles.end() ? &*found : nullptr;
}

Species::InterpolatedEffect EffectAt(std::uint64_t _counter, Species::EffectKind _kind, const Neuron::Vec3& _at, std::uint16_t _timer,
                                     std::uint16_t _size = 0)
{
  Species::InterpolatedEffect effect;
  effect.id = IdOf(_counter);
  effect.state.kind = _kind;
  effect.state.timer = _timer;
  effect.state.size = _size;
  effect.position = _at;
  return effect;
}

Species::InterpolatedFigure WorkerAt(std::uint64_t _counter, const Neuron::Vec3& _at)
{
  Species::InterpolatedFigure figure;
  figure.id = IdOf(_counter);
  figure.state.kind = Species::FigureKind::Worker;
  figure.state.onGround = true;
  figure.state.health = 50;
  figure.health = 50.0;
  figure.position = _at;
  return figure;
}

} // namespace

TEST_CLASS(EffectsLookTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd219TheLaserBiasIsTheRecordsFactor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-219")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-220")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd219TheLaserBiasIsTheRecordsFactor)
  {
    const auto& bias = CommittedPresentation().rnd.depthBias;
    ExpectEqual(Neuron::DEPTH_BIAS_LASER, Neuron::DepthBiasOf(bias.laser, bias.unitScale), L"the compiled-in bias_laser");
    ExpectEqual(3'355'443, Neuron::DEPTH_BIAS_LASER, L"round(0.2 × 2²⁴)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx207TheParticleTypesAreTheRecords)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-207")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx207TheParticleTypesAreTheRecords)
  {
    const std::vector<Neuron::ParticleType> types = Species::ParticleTypesOf(CommittedPresentation());
    Assert::IsTrue(types.size() == static_cast<std::size_t>(Species::ParticleKind::Count), L"one type per kind");
    const Neuron::ParticleType& core = types[static_cast<std::size_t>(Species::ParticleKind::ExplosionCore)];
    Assert::IsTrue(core.lifeSeconds == 2.0 && core.size == 150.0 && core.gravity == 10.0 && core.friction == 0.2, L"the explosion core");
    Assert::IsTrue(std::abs(core.color1[0] - 200.0f / 255.0f) < 1e-6f && std::abs(core.color2[1] - 120.0f / 255.0f) < 1e-6f,
                   L"its colours");
    const Neuron::ParticleType& debris = types[static_cast<std::size_t>(Species::ParticleKind::ExplosionDebris)];
    Assert::IsTrue(debris.blend == Neuron::ParticleBlend::Smoke &&
                     debris.puffType == static_cast<std::uint8_t>(Species::ParticleKind::MissileTrail),
                   L"debris is smoke and puffs missile trails");
    Assert::IsTrue(types[static_cast<std::size_t>(Species::ParticleKind::ControlFlash)].gravity == -2.0 &&
                     types[static_cast<std::size_t>(Species::ParticleKind::ControlFlash)].priorityClass == 0,
                   L"the control flash rises and is command feedback");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCrate445ABoltIsRebuiltFromItsEventAndEndsWithItsLife)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-440")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-445")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-338")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-339")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCrate445ABoltIsRebuiltFromItsEventAndEndsWithItsLife)
  {
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    const Species::StateReplica replica;
    const std::vector<Species::ReceivedEvent> fired{EventOf(Species::EffectEventKind::LaserFired, 10, 1, Air(), {150.0, 0.0, 0.0})};
    look.Update(fired, replica, {}, ViewAt(Air(), 8.5));
    Assert::IsTrue(look.LiveBolts() == 0, L"an event of tick 10 waits for the display time 9");
    look.Update({}, replica, {}, ViewAt(Air(), 9.0));
    Assert::IsTrue(look.LiveBolts() == 1 && look.LiveFlashes() == 1, L"the bolt and its muzzle flash");
    Species::EffectDraws draws;
    look.Draw({}, SeedOneTerrain().field, ViewAt(Air(), 9.5), draws);
    const Neuron::QuadBatch* bolts = BatchOf(draws.quads, "laser_bolt");
    Assert::IsTrue(bolts != nullptr && bolts->pass == Neuron::ScenePass::Lasers && bolts->quads.size() == 5, L"five layers in pass 10b");
    const Neuron::SceneQuad& segment = bolts->quads.front();
    Assert::IsTrue(segment.orientation == Neuron::QuadOrientation::Segment && segment.halfWidth == 0.8f, L"a segment of half-width 0.8");
    Assert::IsTrue(std::abs(segment.position.x - (Air().x + 7.5)) < 1e-9 && std::abs(segment.endPoint.x - (Air().x + 17.5)) < 1e-9,
                   L"half a tick at 150 wu/s, then 10 wu long");
    Assert::IsTrue(bolts->texture == static_cast<std::uint16_t>(Species::SceneTexture::BeamShort), L"fx.beam_short");
    draws = {};
    look.Draw({}, SeedOneTerrain().field, ViewAt(Air(), 15.0), draws);
    Assert::IsTrue(look.LiveBolts() == 0, L"gone after its six advances");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCrate441ARicochetStartsTheSecondFlightAndTheSecondContactEndsIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-441")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCrate441ARicochetStartsTheSecondFlightAndTheSecondContactEndsIt)
  {
    const Neuron::Vec3 contact = Ground();
    for (const double second : {150.0, -150.0})
    {
      Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
      const Species::StateReplica replica;
      // Straight down from 30 wu: the contact is two ticks into the first flight.
      const std::vector<Species::ReceivedEvent> events{
        EventOf(Species::EffectEventKind::LaserFired, 10, 1, Air(), {0.0, -150.0, 0.0}),
        EventOf(Species::EffectEventKind::LaserRicochet, 11, 1, contact, {0.0, second, 0.0})};
      look.Update(events, replica, {}, ViewAt(Air(), 10.0));
      Species::EffectDraws draws;
      look.Draw({}, SeedOneTerrain().field, ViewAt(Air(), 10.0), draws);
      const Neuron::QuadBatch* bolts = BatchOf(draws.quads, "laser_bolt");
      Assert::IsTrue(bolts != nullptr && std::abs(bolts->quads.front().position.y - (Air().y - 15.0)) < 1e-9, L"still on its first flight");
      draws = {};
      look.Draw({}, SeedOneTerrain().field, ViewAt(Air(), 11.5), draws);
      if (second > 0.0)
      {
        bolts = BatchOf(draws.quads, "laser_bolt");
        Assert::IsTrue(bolts != nullptr && std::abs(bolts->quads.front().position.y - (contact.y + 7.5)) < 1e-9,
                       L"half a tick up from the contact");
      }
      else
      {
        Assert::IsTrue(look.LiveBolts() == 0, L"into the ground again: removed");
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCrate475AMuzzleFlashMovesAndFades)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-405")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-475")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-222")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCrate475AMuzzleFlashMovesAndFades)
  {
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    const Species::StateReplica replica;
    look.Update(std::vector{EventOf(Species::EffectEventKind::LaserFired, 10, 1, Air(), {150.0, 0.0, 0.0})}, replica, {},
                ViewAt(Air(), 9.0));
    Species::EffectDraws draws;
    look.Draw({}, SeedOneTerrain().field, ViewAt(Air(), 9.0), draws);
    // Size 25 × 0.6 s = 15, below the depth-test size: five nested quads with no depth test, each × 0.8.
    const Neuron::QuadBatch* flash = BatchOf(draws.quads, "effect_additive_nodepth");
    Assert::IsTrue(flash != nullptr && flash->quads.size() == 5 && flash->pass == Neuron::ScenePass::Effects, L"five quads in pass 10a");
    Assert::IsTrue(std::abs(flash->quads[0].halfWidth - 15.0f) < 1e-4f && std::abs(flash->quads[1].halfWidth - 12.0f) < 1e-4f,
                   L"the half-sizes");
    Assert::IsTrue(flash->quads[0].color[3] == 1.0f, L"life 1.5 × 0.75, clamped to 1");
    draws = {};
    look.Draw({}, SeedOneTerrain().field, ViewAt(Air(), 10.0), draws);
    flash = BatchOf(draws.quads, "effect_additive_nodepth");
    Assert::IsTrue(flash != nullptr && std::abs(flash->quads[0].color[3] - 0.375f) < 1e-6f, L"0.1 s later its life is 0.5");
    Assert::IsTrue(std::abs(flash->quads[0].position.x - (Air().x + 1.0)) < 1e-9, L"it moved 1 wu along its front");
    draws = {};
    look.Draw({}, SeedOneTerrain().field, ViewAt(Air(), 10.5), draws);
    Assert::IsTrue(look.LiveFlashes() == 0, L"removed at life 0");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx219AnExplosionEmitsItsCoresAndItsDebris)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-406")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-219")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx219AnExplosionEmitsItsCoresAndItsDebris)
  {
    // VFX-219's test T9: range 12.5 and damage 50 emit 6 to 12 cores and 6 debris particles.
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    const Species::StateReplica replica;
    look.Update(std::vector{EventOf(Species::EffectEventKind::Explosion, 10, 0, Ground(), {50.0, 0.0, 0.0}, 12.5)}, replica, {},
                ViewAt(Ground(), 9.0));
    Species::EffectDraws draws;
    look.Draw({}, SeedOneTerrain().field, ViewAt(Ground(), 9.0), draws);
    const Neuron::QuadBatch* smoke = BatchOf(draws.quads, "particle_darkening");
    const Neuron::QuadBatch* glow = BatchOf(draws.quads, "particle_additive");
    Assert::IsTrue(smoke != nullptr && smoke->quads.size() == 6, L"six debris particles, in the smoke batch");
    Assert::IsTrue(glow != nullptr && glow->quads.size() >= 6 && glow->quads.size() <= 12, L"six to twelve cores");
    Assert::IsTrue(glow->texture == static_cast<std::uint16_t>(Species::SceneTexture::ParticleDot), L"fx.particle_dot");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd333ARingsDiscGrowsAndItsBlastOutlivesIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-333")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-334")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-473")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-234")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd333ARingsDiscGrowsAndItsBlastOutlivesIt)
  {
    // RND-333's test T12: a ring of size 2.5 s, its rim at 35 wu and alpha 0.6 when created.
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    const std::vector<Species::InterpolatedEffect> ring{EffectAt(1, Species::EffectKind::ShockwaveRing, Ground(), 0, 160)};
    Species::EffectDraws draws;
    look.Draw(ring, SeedOneTerrain().field, ViewAt(Ground(), 9.0), draws);
    Assert::IsTrue(!draws.triangles.empty() && !draws.triangles[0].screenSpace && !draws.triangles[0].textured, L"the disc's fan");
    const Neuron::TriangleBatch& disc = draws.triangles[0];
    Assert::IsTrue(disc.entry == "effect_gradient_nodepth" && disc.vertices.size() == std::size_t{3} * 32, L"32 triangles");
    const Neuron::SceneVertex& center = disc.vertices[0];
    const Neuron::SceneVertex& rim = disc.vertices[1];
    Assert::IsTrue(std::abs(center.position.y - (Ground().y + 5.0)) < 1e-9 && center.color[3] == 0.0f, L"the centre, 5 wu up, alpha 0");
    Assert::IsTrue(std::abs(Neuron::Length(rim.position - center.position) - 35.0) < 1e-9 && std::abs(rim.color[3] - 0.6f) < 1e-6f,
                   L"the rim at 35 wu and alpha 0.6");
    Assert::IsTrue(QuadsOf(draws.quads, "effect_additive", Species::SceneTexture::Starburst) == 5 &&
                     QuadsOf(draws.quads, "effect_additive", Species::SceneTexture::GlowRound) == 5,
                   L"the blast's two stacks");
    const Neuron::TriangleBatch* flash = ScreenFlashOf(draws);
    const double distance = Neuron::Length(Neuron::Vec3{0.0, 50.0, 200.0});
    Assert::IsTrue(flash != nullptr && std::abs(flash->vertices[0].color[3] - static_cast<float>(1.0 - distance / 500.0)) < 1e-6f,
                   L"the full-screen flash, faded by the distance");
    Assert::IsTrue(std::abs(look.Shake() - (1.0 - distance / 500.0)) < 1e-9, L"and the shake of its alpha");
    // 2.4 s later the ring is gone from the replica; its copy draws the disc at 35 + 40 × 2.4 wu, and no blast.
    draws = {};
    look.Draw({}, SeedOneTerrain().field, ViewAt(Ground(), 33.0), draws);
    Assert::IsTrue(!draws.triangles.empty() &&
                     std::abs(Neuron::Length(draws.triangles[0].vertices[1].position - draws.triangles[0].vertices[0].position) - 131.0) <
                       1e-9,
                   L"the grown disc");
    Assert::IsTrue(QuadsOf(draws.quads, "effect_additive", Species::SceneTexture::Starburst) == 0, L"the blast lasted its second");
    draws = {};
    look.Draw({}, SeedOneTerrain().field, ViewAt(Ground(), 34.0), draws);
    Assert::IsTrue(look.LiveRings() == 0 && draws.triangles.empty(), L"removed at the end of its life");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx235AtMostThreeFlashOnsetsAreShownInAWindow)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-235")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx235AtMostThreeFlashOnsetsAreShownInAWindow)
  {
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    std::vector<bool> shown;
    for (std::uint64_t ring = 0; ring < 4; ++ring)
    {
      // A ring every 0.2 s of real time, each flash over 0.15 s later.
      const double born = 9.0 + 2.0 * static_cast<double>(ring);
      Species::EffectDraws draws;
      look.Draw(std::vector{EffectAt(ring + 1, Species::EffectKind::ShockwaveRing, Ground(), 0, 40)}, SeedOneTerrain().field,
                ViewAt(Ground(), born), draws);
      shown.push_back(ScreenFlashOf(draws) != nullptr);
      draws = {};
      look.Draw({}, SeedOneTerrain().field, ViewAt(Ground(), born + 1.5), draws);
      Assert::IsTrue(ScreenFlashOf(draws) == nullptr, L"each flash is over");
    }
    Assert::IsTrue(shown == std::vector<bool>{true, true, true, false}, L"the fourth onset within the second is not drawn");
    Species::EffectDraws later;
    look.Draw(std::vector{EffectAt(9, Species::EffectKind::ShockwaveRing, Ground(), 0, 40)}, SeedOneTerrain().field, ViewAt(Ground(), 30.0),
              later);
    Assert::IsTrue(ScreenFlashOf(later) != nullptr, L"a second later, onsets are shown again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx232ANearBoltShakesTheViewAndTheShakeDecays)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-232")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-340")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx232ANearBoltShakesTheViewAndTheShakeDecays)
  {
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    const Species::StateReplica replica;
    Species::EffectsView view = ViewAt(Air(), 9.0);
    view.camera = Air() + Neuron::Vec3{0.0, 0.0, 5.0};
    look.Update(std::vector{EventOf(Species::EffectEventKind::LaserFired, 10, 1, Air(), {150.0, 0.0, 0.0})}, replica, {}, view);
    Species::EffectDraws draws;
    look.Draw({}, SeedOneTerrain().field, view, draws);
    Assert::IsTrue(look.Shake() == 0.5, L"a bolt 5 wu from the camera");
    Assert::IsTrue(std::abs(draws.shakeYawRadians) <= 0.0125 && std::abs(draws.shakeRollRadians) <= 0.075, L"within the shake's draws");
    view.camera = Ground() + Neuron::Vec3{0.0, 50.0, 2'000.0};
    view.unscaledStep = 0.25;
    for (int frame = 0; frame < 2; ++frame)
    {
      draws = {};
      look.Draw({}, SeedOneTerrain().field, view, draws);
    }
    Assert::IsTrue(look.Shake() == 0.0 && draws.shakeYawRadians == 0.0 && draws.shakeRollRadians == 0.0, L"decayed at 1 a second");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCrate414AGrenadeTumblesAndBlinksInTheLastFifthOfEachSecond)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-410")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-414")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCrate414AGrenadeTumblesAndBlinksInTheLastFifthOfEachSecond)
  {
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    Species::EffectsView view = ViewAt(Air(), 9.0);
    view.fraction = 0.5;
    for (const auto& [timer, blinks] : {std::pair{std::uint16_t{21}, true}, std::pair{std::uint16_t{25}, false}})
    {
      Species::EffectDraws draws;
      look.Draw(std::vector{EffectAt(1, Species::EffectKind::Grenade, Air(), timer)}, SeedOneTerrain().field, view, draws);
      Assert::IsTrue(draws.meshes.size() == 1 && draws.meshes[0].entry == "effect_mesh_lit" && draws.meshes[0].instances.size() == 1 &&
                       draws.meshes[0].mesh == static_cast<std::uint16_t>(Species::SceneMesh::ThrownWeapon),
                     L"mesh.projectile.thrown_weapon in pass 10a");
      const Neuron::SceneMeshInstance& grenade = draws.meshes[0].instances[0];
      Assert::IsTrue(std::abs(Neuron::Length(grenade.axisY) - 1.0) < 1e-9 && std::abs(Neuron::Dot(grenade.axisY, grenade.axisZ)) < 1e-9,
                     L"tumbled, its axes still orthonormal");
      // Age 0.85 s with 21 ticks of its fuse left, and 0.45 s with 25.
      ExpectEqual(blinks ? 2 : 0, static_cast<std::int64_t>(QuadsOf(draws.quads, "effect_additive", Species::SceneTexture::Starburst)),
                  L"the blink's two starbursts");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(O75AShooterRecoilsFromItsShot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-118")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(O75AShooterRecoilsFromItsShot)
  {
    Species::EffectsLook look(CommittedPresentation(), CommittedTuning());
    const Species::StateReplica replica;
    const Neuron::Vec3 muzzle = Ground() + Neuron::Vec3{0.0, CommittedTuning().ref.unit.attack.muzzleHeight, 0.0};
    Species::EffectsView view = ViewAt(Ground(), 9.0);
    look.Update(std::vector{EventOf(Species::EffectEventKind::LaserFired, 10, 1, muzzle, {150.0, 0.0, 0.0})}, replica, {}, view);
    std::vector<Species::InterpolatedFigure> figures{WorkerAt(2, Ground() + Neuron::Vec3{0.5, 0.0, 0.0})};
    look.ApplyRecoil(figures, view);
    const double distance = CommittedPresentation().vfx.worker.recoilDistance;
    Assert::IsTrue(std::abs(figures[0].position.x - (Ground().x + 0.5 - distance)) < 1e-9, L"pushed back from the shot");
    figures = {WorkerAt(2, Ground() + Neuron::Vec3{0.5, 0.0, 0.0})};
    view.scaledSeconds += CommittedPresentation().vfx.worker.recoilTime;
    look.ApplyRecoil(figures, view);
    Assert::IsTrue(figures[0].position.x == Ground().x + 0.5, L"over once the recoil time has passed");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx219ABurningWorkerAndAControlChangeEmitPerTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-219")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx219ABurningWorkerAndAControlChangeEmitPerTick)
  {
    Species::EffectsLook burning(CommittedPresentation(), CommittedTuning());
    Species::EffectsLook control(CommittedPresentation(), CommittedTuning());
    Species::StateReplica replica;
    Species::InterpolatedFigure fire = WorkerAt(1, Ground());
    fire.state.task = static_cast<std::uint8_t>(Species::WorkerState::Burning);
    Species::InterpolatedFigure controlled = WorkerAt(2, Ground());
    for (std::uint64_t tick = 1; tick <= 10; ++tick)
    {
      replica.Apply({tick, 0, {}});
      controlled.state.underControl = tick >= 2;
      const Species::EffectsView view = ViewAt(Ground(), static_cast<double>(tick) - 1.0);
      burning.Update({}, replica, std::vector{fire}, view);
      control.Update({}, replica, std::vector{controlled}, view);
      if (tick == 2)
      {
        Assert::IsTrue(control.Particles().Live() == 1, L"one control flash as it is taken into control");
      }
    }
    Assert::IsTrue(burning.Particles().Live() > 0, L"a burning Worker gives off fire and smoke");
    Assert::IsTrue(control.Particles().Live() == 1, L"and none while its control is unchanged");
  }
};

} // namespace GameClientTests
