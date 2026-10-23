#include "pch.h"

#include "SoundAsset.h"
#include "TestSupport.h"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(SoundAssetTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Ast080AResidentSoundReadsBackAsWritten)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AST-080")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Ast080AResidentSoundReadsBackAsWritten)
  {
    // ADR-171: 16-bit PCM at 48 kHz, its channels interleaved, read back sample for sample.
    Neuron::SoundAsset stereo;
    stereo.channels = 2;
    stereo.samples = {0, 1, -1, 32'767, -32'767, 12'345};
    const std::vector<std::byte> bytes = Neuron::WriteSoundAsset(stereo);
    Assert::AreEqual(std::size_t{14 + 12}, bytes.size(), L"the header and the samples, nothing more");
    const auto read = Neuron::ReadSoundAsset(bytes);
    Assert::IsTrue(read.has_value(), L"read back");
    if (!read.has_value())
    {
      return;
    }
    Assert::IsTrue(read->channels == 2 && read->sampleRate == Neuron::SOUND_SAMPLE_RATE && read->Frames() == 3 &&
                     read->samples == stereo.samples,
                   L"as written");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen179AResidentSoundTheEngineCannotPlayDoesNotRead)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-179")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen179AResidentSoundTheEngineCannotPlayDoesNotRead)
  {
    Neuron::SoundAsset mono;
    mono.samples = {100, 200};
    const std::vector<std::byte> good = Neuron::WriteSoundAsset(mono);
    Assert::IsTrue(Neuron::ReadSoundAsset(good).has_value(), L"the sound itself reads");
    std::vector<std::byte> version = good;
    version[0] = std::byte{2};
    Assert::IsFalse(Neuron::ReadSoundAsset(version).has_value(), L"another version");
    Neuron::SoundAsset other = mono;
    other.sampleRate = 44'100;
    Assert::IsFalse(Neuron::ReadSoundAsset(Neuron::WriteSoundAsset(other)).has_value(), L"another rate");
    other = mono;
    other.channels = 3;
    other.samples = {1, 2, 3};
    Assert::IsFalse(Neuron::ReadSoundAsset(Neuron::WriteSoundAsset(other)).has_value(), L"three channels");
    other = mono;
    other.samples.clear();
    Assert::IsFalse(Neuron::ReadSoundAsset(Neuron::WriteSoundAsset(other)).has_value(), L"no frames");
    std::vector<std::byte> shortened = good;
    shortened.pop_back();
    Assert::IsFalse(Neuron::ReadSoundAsset(shortened).has_value(), L"a length its frame count does not give");
    Assert::IsFalse(Neuron::ReadSoundAsset(std::vector<std::byte>(5)).has_value(), L"a header cut short");
  }
};

} // namespace NeuronCoreTests
