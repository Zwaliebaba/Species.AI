#include "pch.h"

#include "FrameView.h"
#include "FxRng.h"
#include "PresentationClock.h"
#include "SceneQuads.h"
#include "TestSupport.h"

#include <bit>
#include <chrono>
#include <cmath>
#include <numbers>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

constexpr Neuron::Vec3 FAR_CAMERA{33'000.0, 800.0, 34'500.0};

} // namespace

TEST_CLASS(SceneQuadsTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Wld246AQuadIsMadeRelativeToTheCameraInBinary64)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-246")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-262")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld246AQuadIsMadeRelativeToTheCameraInBinary64)
  {
    Neuron::SceneQuad quad;
    quad.position = {33'000.125, 801.5, 34'499.875};
    quad.endPoint = {33'010.0, 800.0, 34'500.0};
    quad.halfWidth = 3.0f;
    quad.halfHeight = 4.0f;
    quad.blendStart = 300.0f;
    quad.blendLength = 1'000.0f;
    quad.rotationRadians = 0.5f;
    quad.blendLeft = true;
    quad.orientation = Neuron::QuadOrientation::WorldOriented;
    quad.anchor = Neuron::QuadAnchor::BottomCenter;
    const Neuron::QuadInstance instance = Neuron::MakeQuadInstance(quad, FAR_CAMERA);
    // The difference is exact in binary64 and representable in binary32, so nothing of the eighth is lost.
    Assert::IsTrue(instance.position.x == 0.125f && instance.position.y == 1.5f && instance.position.z == -0.125f, L"relative");
    Assert::IsTrue(instance.position.w == 3.0f && instance.endPoint.w == 4.0f, L"the half-sizes");
    Assert::IsTrue(instance.endPoint.x == 10.0f, L"the end point, relative too");
    Assert::IsTrue(instance.axisRight.w == 300.0f && instance.axisUp.w == 1'000.0f, L"the billboard blend");
    Assert::IsTrue(instance.shape.x == Neuron::QUAD_WORLD_ORIENTED && instance.shape.y == Neuron::QUAD_ANCHOR_BOTTOM &&
                     std::bit_cast<float>(instance.shape.z) == 0.5f && instance.shape.w == Neuron::QUAD_BLEND_LEFT,
                   L"the mode, the anchor, the rotation and the flag");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd236TheViewTestIsConservative)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-236")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd236TheViewTestIsConservative)
  {
    Neuron::CameraPose pose;
    pose.position = FAR_CAMERA;
    pose.verticalFovRadians = std::numbers::pi / 3.0;
    const Neuron::FrameView frame = Neuron::MakeFrameView(pose, 16.0 / 9.0, {});
    const std::array<Neuron::Float4, 6> planes = Neuron::FrustumPlanes(frame);
    Neuron::SceneQuad quad;
    quad.position = FAR_CAMERA + Neuron::Vec3{0.0, 0.0, -200.0}; // ahead: a yaw of 0 looks along −z
    Assert::IsTrue(Neuron::QuadMayBeVisible(planes, quad, FAR_CAMERA, 5.0f), L"ahead");
    quad.position = FAR_CAMERA + Neuron::Vec3{0.0, 0.0, 200.0};
    Assert::IsFalse(Neuron::QuadMayBeVisible(planes, quad, FAR_CAMERA, 5.0f), L"behind");
    // A segment behind the camera whose end reaches ahead is kept.
    quad.orientation = Neuron::QuadOrientation::Segment;
    quad.endPoint = FAR_CAMERA + Neuron::Vec3{0.0, 0.0, -300.0};
    Assert::IsTrue(Neuron::QuadMayBeVisible(planes, quad, FAR_CAMERA, 1.0f), L"a segment by its whole length");
  }
};

TEST_CLASS(FxRngTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(RSim607ThePresentationDrawsKeepTheirRanges)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-607")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-011")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim607ThePresentationDrawsKeepTheirRanges)
  {
    Neuron::FxRng a(Neuron::FxSeed(120, 7, 2));
    Neuron::FxRng b(Neuron::FxSeed(120, 7, 2));
    for (int n = 0; n < 1'000; ++n)
    {
      const double unit = a.NextUnit(4.0);
      const double centered = a.NextSigned(4.0);
      Assert::IsTrue(unit >= 0.0 && unit <= 4.0 && centered >= -2.0 && centered <= 2.0, L"in range");
      Assert::IsTrue(unit == b.NextUnit(4.0) && centered == b.NextSigned(4.0), L"the same seed draws the same values");
    }
    Assert::IsTrue(Neuron::FxSeed(120, 7, 2) != Neuron::FxSeed(120, 7, 3) && Neuron::FxSeed(120, 7, 2) != Neuron::FxSeed(121, 7, 2),
                   L"each part of the identity changes the seed");
  }
};

TEST_CLASS(PresentationClockTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Vfx016TheStepIsClampedAndHeldWorldsStop)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"VFX-016")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Vfx016TheStepIsClampedAndHeldWorldsStop)
  {
    using std::chrono::milliseconds;
    Neuron::PresentationClock clock(0.25);
    clock.Advance(milliseconds(1'000), false, 1.0);
    Assert::IsTrue(clock.RealSeconds() == 0.0 && clock.UnscaledSeconds() == 0.0, L"the first frame starts every reading");
    clock.Advance(milliseconds(1'100), false, 2.0);
    Assert::IsTrue(std::abs(clock.UnscaledSeconds() - 0.1) < 1e-12 && std::abs(clock.ScaledSeconds() - 0.2) < 1e-12, L"scaled by speed");
    clock.Advance(milliseconds(3'100), false, 1.0);
    Assert::IsTrue(std::abs(clock.UnscaledStepSeconds() - 0.25) < 1e-12, L"a stall's step is clamped");
    clock.Advance(milliseconds(3'200), true, 1.0);
    Assert::IsTrue(clock.UnscaledStepSeconds() == 0.0 && std::abs(clock.RealSeconds() - 2.2) < 1e-12, L"held: only the real reading moves");
  }
};

} // namespace NeuronClientTests
