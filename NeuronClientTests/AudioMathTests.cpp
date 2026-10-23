#include "pch.h"

#include "AudioMath.h"
#include "Geometry.h"

#include <cmath>
#include <numbers>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

bool Near(double _expected, double _actual, double _tolerance = 1e-6)
{
  return std::abs(_expected - _actual) <= _tolerance;
}

} // namespace

TEST_CLASS(AudioMathTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Aen214ALevelInDecibelsIsAnAmplitudeAboveTheFloor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-214")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen214ALevelInDecibelsIsAnAmplitudeAboveTheFloor)
  {
    Assert::IsTrue(Near(1.0, Neuron::DecibelsToAmplitude(0.0, -100.0)), L"0 dB is unity");
    Assert::IsTrue(Near(0.5, Neuron::DecibelsToAmplitude(20.0 * std::log10(0.5), -100.0)), L"-6.02 dB is a half");
    Assert::IsTrue(Neuron::DecibelsToAmplitude(-100.0, -100.0) == 0.0, L"the floor is silence");
    Assert::IsTrue(Neuron::DecibelsToAmplitude(-140.0, -100.0) == 0.0, L"below the floor is silence");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen201TheDistanceGainIsTheMinimumDistanceOverTheDistance)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-201")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-202")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen201TheDistanceGainIsTheMinimumDistanceOverTheDistance)
  {
    Assert::IsTrue(Near(1.0, Neuron::DistanceGain(50.0, 100.0, 1e9, 0.1)), L"1 within the minimum distance");
    Assert::IsTrue(Near(0.5, Neuron::DistanceGain(200.0, 100.0, 1e9, 0.1)), L"a half at twice it");
    Assert::IsTrue(Near(100.0 / 950.0 * 0.5, Neuron::DistanceGain(950.0, 100.0, 1'000.0, 0.1)), L"faded over the last tenth");
    Assert::IsTrue(Neuron::DistanceGain(1'000.0, 100.0, 1'000.0, 0.1) == 0.0, L"none at the maximum distance");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen204TheStereoPanIsConstantPowerAcrossTheListenersRight)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-204")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-209")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen204TheStereoPanIsConstantPowerAcrossTheListenersRight)
  {
    const Neuron::Vec3 listener{10.0, 5.0, 10.0};
    const Neuron::Vec3 front{0.0, 0.0, 1.0};
    const Neuron::Vec3 up{0.0, 1.0, 0.0};
    const Neuron::Vec3 right = Neuron::Cross(front, up);
    const auto onRight = Neuron::StereoPan(listener + right * 50.0, listener, front, up);
    Assert::IsTrue(Near(0.0, onRight[0]) && Near(1.0, onRight[1]), L"an emitter on the right is in the right channel");
    const auto onLeft = Neuron::StereoPan(listener - right * 50.0, listener, front, up);
    Assert::IsTrue(Near(1.0, onLeft[0]) && Near(0.0, onLeft[1]), L"an emitter on the left is in the left channel");
    const auto ahead = Neuron::StereoPan(listener + front * 50.0, listener, front, up);
    const double center = std::numbers::sqrt2 * 0.5;
    Assert::IsTrue(Near(center, ahead[0]) && Near(center, ahead[1]), L"an emitter ahead is centered");
    const auto at = Neuron::StereoPan(listener, listener, front, up);
    Assert::IsTrue(Near(center, at[0]) && Near(center, at[1]), L"an emitter at the listener is centered");
    const auto halfway = Neuron::StereoPan(listener + (front + right) * 50.0, listener, front, up);
    Assert::IsTrue(Near(1.0, halfway[0] * halfway[0] + halfway[1] * halfway[1]) && halfway[1] > halfway[0], L"the power is constant");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen215AVolumeStepLowersItsStageByTheStepDecibels)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-215")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen215AVolumeStepLowersItsStageByTheStepDecibels)
  {
    Assert::IsTrue(Near(0.0, Neuron::VolumeStepDb(255, 255, 0.32, -100.0)), L"the top step is 0 dB");
    Assert::IsTrue(Near(-0.32, Neuron::VolumeStepDb(254, 255, 0.32, -100.0)), L"each step below it is 0.32 dB");
    Assert::IsTrue(Near(-81.6, Neuron::VolumeStepDb(0, 255, 0.32, -100.0)), L"step 0 is 81.6 dB down");
    Assert::IsTrue(Near(-100.0, Neuron::VolumeStepDb(0, 255, 1.0, -100.0)), L"never below the floor");
    Assert::IsTrue(Near(0.0, Neuron::VolumeStepDb(300, 255, 0.32, -100.0)), L"a step above the top is the top");
  }
};

} // namespace NeuronClientTests
