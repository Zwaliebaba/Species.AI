#include "pch.h"

#include "FrameView.h"
#include "Picking.h"
#include "TestSupport.h"

#include <cmath>
#include <numbers>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// A camera over the MVP's region, 33,000 wu from the origin, looking a little down and to the side.
Neuron::CameraPose RegionCamera()
{
  Neuron::CameraPose pose;
  pose.position = {33'000.0, 800.0, 34'500.0};
  pose.yawRadians = 0.6;
  pose.pitchRadians = -0.45;
  pose.verticalFovRadians = std::numbers::pi / 3.0;
  return pose;
}

constexpr Neuron::SizePx SIZE{1'920, 1'080};

} // namespace

TEST_CLASS(PickingTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Cam090TheCenterRayRunsAlongTheViewFromTheNearPlane)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"CAM-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-218")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Cam090TheCenterRayRunsAlongTheViewFromTheNearPlane)
  {
    const Neuron::CameraPose pose = RegionCamera();
    const Neuron::FrameView frame = Neuron::MakeFrameView(pose, 16.0 / 9.0, {});
    const Neuron::Ray ray = Neuron::PickRayAt(960.0, 540.0, frame, SIZE, 0.0);
    const Neuron::Vec3 front = Neuron::ViewDirection(pose);
    Assert::AreEqual(1.0, Neuron::Dot(ray.direction, front), 1e-6, L"along the view");
    Assert::AreEqual(5.0, Neuron::Dot(ray.origin - pose.position, front), 1e-3, L"from the near plane, 5 wu ahead");
    Assert::AreEqual(0.0, Neuron::Length(ray.origin - (pose.position + front * 5.0)), 1e-3, L"in binary64, far from the origin");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Cam092APickedPointProjectsBackToItsPixel)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"CAM-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"CAM-092")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-216")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Cam092APickedPointProjectsBackToItsPixel)
  {
    const Neuron::FrameView frame = Neuron::MakeFrameView(RegionCamera(), 16.0 / 9.0, {});
    for (const auto& [x, y] : {std::pair{0.0, 0.0}, std::pair{1'919.0, 1'079.0}, std::pair{300.5, 820.25}})
    {
      // DEC-956: the reference's ray runs one pixel below the pointer's row.
      const Neuron::Ray ray = Neuron::PickRayAt(x, y, frame, SIZE, 1.0);
      const auto screen = Neuron::ProjectToScreen(ray.origin + ray.direction * 2'000.0, frame, SIZE);
      Assert::IsTrue(screen.has_value(), L"in front of the camera");
      Assert::AreEqual(x, screen.value_or(Neuron::ScreenPoint{}).x, 1e-2, L"the same column");
      Assert::AreEqual(y + 1.0, screen.value_or(Neuron::ScreenPoint{}).y, 1e-2, L"one row below");
    }
    Assert::IsFalse(
      Neuron::ProjectToScreen(RegionCamera().position - Neuron::ViewDirection(RegionCamera()) * 10.0, frame, SIZE).has_value(),
      L"a point behind the camera has no screen position");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit192ARayMeetsASphereOnlyAhead)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-192")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit192ARayMeetsASphereOnlyAhead)
  {
    const Neuron::Ray ray{{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
    Assert::AreEqual(92.0, Neuron::RaySphere(ray, {100.0, 0.0, 0.0}, 8.0).value_or(-1.0), 1e-9, L"its near side");
    Assert::IsFalse(Neuron::RaySphere(ray, {100.0, 9.0, 0.0}, 8.0).has_value(), L"a miss");
    Assert::IsFalse(Neuron::RaySphere(ray, {-100.0, 0.0, 0.0}, 8.0).has_value(), L"behind the start");
    Assert::AreEqual(8.0, Neuron::RaySphere(ray, {0.0, 0.0, 0.0}, 8.0).value_or(-1.0), 1e-9, L"from inside, its far side");
  }
};

} // namespace NeuronClientTests
