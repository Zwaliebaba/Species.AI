#include "pch.h"

#include "InMemoryGpuResourceFactory.h"
#include "PlatformThread.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

Neuron::GpuTextureDesc Texture(std::uint32_t _levels = 1)
{
  return {256, 256, _levels, DXGI_FORMAT_BC7_UNORM_SRGB, "test.texture"};
}

Neuron::InMemoryGpuResourceFactory::Desc Budget(std::uint64_t _bytes)
{
  Neuron::InMemoryGpuResourceFactory::Desc desc;
  desc.stagingBudgetBytes = _bytes;
  return desc;
}

} // namespace

TEST_CLASS(GpuResourceFactoryTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd009DescriptionsAreChecked)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-009")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd009DescriptionsAreChecked)
  {
    Assert::IsTrue(Neuron::IsValidDescription(Texture(9)), L"a full mip chain of 256 x 256 has nine levels");
    Assert::IsFalse(Neuron::IsValidDescription(Texture(10)), L"a tenth level does not exist");
    Assert::IsFalse(Neuron::IsValidDescription(Neuron::GpuTextureDesc{0, 256, 1, DXGI_FORMAT_R8_UNORM, {}}), L"no width");
    Assert::IsFalse(Neuron::IsValidDescription(Neuron::GpuTextureDesc{256, 256, 1, DXGI_FORMAT_UNKNOWN, {}}), L"no format");
    Assert::IsTrue(Neuron::IsValidDescription(Neuron::GpuBufferDesc{1024, 16, false, {}}), L"64 structured elements");
    Assert::IsFalse(Neuron::IsValidDescription(Neuron::GpuBufferDesc{1000, 16, false, {}}), L"a partial element");
    Assert::IsFalse(Neuron::IsValidDescription(Neuron::GpuBufferDesc{}), L"no size");
    Neuron::InMemoryGpuResourceFactory factory(Budget(1024));
    const auto refused = factory.CreateTexture(Texture(10));
    Assert::IsTrue(!refused.has_value() && refused.error() == Neuron::GpuResourceError::InvalidDescription, L"refused");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd009UploadsFromSeveralThreadsReturnIncreasingFenceValues)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-009")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd009UploadsFromSeveralThreadsReturnIncreasingFenceValues)
  {
    // RND T11: eight threads upload while the frame thread submits; each thread's fence values never decrease, and a
    // value is complete only once its frame has completed.
    Neuron::InMemoryGpuResourceFactory factory(Budget(std::uint64_t{64} * 1024 * 1024));
    const std::array<std::byte, 64> bytes{};
    const std::array subresource{Neuron::GpuSubresourceData{bytes, 0}};
    constexpr std::size_t THREADS = 8;
    constexpr std::size_t UPLOADS = 200;
    std::vector<std::vector<std::uint64_t>> fences(THREADS);
    std::mutex failuresMutex;
    std::size_t failures = 0;
    {
      std::vector<Neuron::PlatformThread> threads;
      threads.reserve(THREADS);
      for (std::size_t index = 0; index < THREADS; ++index)
      {
        threads.push_back(Neuron::PlatformThread::Start(
          "test.uploader",
          [&, index](const std::stop_token&)
          {
            for (std::size_t upload = 0; upload < UPLOADS; ++upload)
            {
              const auto buffer = factory.CreateBuffer({64, 0, false, "test.buffer"});
              const auto fence = buffer.has_value() ? factory.ScheduleUpload(*buffer, subresource)
                                                    : std::expected<std::uint64_t, Neuron::GpuResourceError>(std::unexpect, buffer.error());
              if (!fence.has_value())
              {
                const std::lock_guard lock(failuresMutex);
                ++failures;
                continue;
              }
              fences[index].push_back(*fence);
            }
          }));
      }
      for (std::size_t frame = 0; frame < 50; ++frame)
      {
        factory.BeginFrame();
      }
    }
    factory.BeginFrame();
    ExpectEqual(0, static_cast<std::int64_t>(failures), L"no upload refused");
    std::uint64_t highest = 0;
    for (const std::vector<std::uint64_t>& values : fences)
    {
      ExpectEqual(static_cast<std::int64_t>(UPLOADS), static_cast<std::int64_t>(values.size()), L"every upload has a fence value");
      for (std::size_t index = 1; index < values.size(); ++index)
      {
        Assert::IsTrue(values[index - 1] <= values[index], L"a thread's fence values never decrease");
      }
      highest = std::max(highest, values.back());
    }
    Assert::IsFalse(factory.IsUploadComplete(highest), L"nothing completes before the GPU");
    factory.CompleteFramesThrough(highest);
    Assert::IsTrue(factory.IsUploadComplete(highest), L"complete with its frame");
    ExpectEqual(0, static_cast<std::int64_t>(factory.StagingBytesInFlight()), L"the staging memory is free again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd009FullStagingRefusesAtOnce)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-009")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd009FullStagingRefusesAtOnce)
  {
    Neuron::InMemoryGpuResourceFactory factory(Budget(100));
    const auto buffer = factory.CreateBuffer({64, 0, false, "test.buffer"});
    const std::array<std::byte, 64> bytes{};
    const std::array subresource{Neuron::GpuSubresourceData{bytes, 0}};
    Assert::IsTrue(factory.ScheduleUpload(*buffer, subresource).has_value(), L"64 of 100 bytes");
    const auto refused = factory.ScheduleUpload(*buffer, subresource);
    Assert::IsTrue(!refused.has_value() && refused.error() == Neuron::GpuResourceError::StagingFull, L"128 of 100 bytes is refused");
    factory.BeginFrame();
    factory.CompleteFramesThrough(1);
    Assert::IsTrue(factory.ScheduleUpload(*buffer, subresource).has_value(), L"repeated on a later frame");
    Assert::IsTrue(factory.Calls().size() == 4, L"every call is recorded");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd009ReleaseWaitsForTheFramesInFlight)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-009")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd009ReleaseWaitsForTheFramesInFlight)
  {
    Neuron::InMemoryGpuResourceFactory factory(Budget(1024));
    factory.BeginFrame();
    factory.BeginFrame(); // frame 2 records while frame 1 may still run
    const Neuron::GpuResourceId first = *factory.CreateTexture(Texture());
    factory.Release(first);
    ExpectEqual(Neuron::INVALID_HEAP_INDEX, factory.HeapIndex(first), L"a released resource is gone for its owner");
    const Neuron::GpuResourceId second = *factory.CreateTexture(Texture());
    Assert::IsTrue(second.slot != first.slot, L"the slot is not reused while a frame in flight may use it");
    factory.CompleteFramesThrough(1);
    Assert::IsTrue(factory.CreateTexture(Texture())->slot != first.slot, L"nor once only the earlier frame completed");
    factory.CompleteFramesThrough(2);
    const Neuron::GpuResourceId reused = *factory.CreateTexture(Texture());
    Assert::IsTrue(reused.slot == first.slot && reused.generation == first.generation + 1, L"reused, with a new generation");
    const std::array<std::byte, 16> bytes{};
    const std::array subresource{Neuron::GpuSubresourceData{bytes, 64}};
    const auto stale = factory.ScheduleUpload(first, subresource);
    Assert::IsTrue(!stale.has_value() && stale.error() == Neuron::GpuResourceError::UnknownResource, L"the old id names nothing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd035RebuildKeepsIdsAndHeapIndices)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-035")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd035RebuildKeepsIdsAndHeapIndices)
  {
    Neuron::InMemoryGpuResourceFactory factory(Budget(1024));
    const Neuron::GpuResourceId texture = *factory.CreateTexture(Texture());
    const Neuron::GpuResourceId buffer = *factory.CreateBuffer({64, 0, false, "test.buffer"});
    const std::array<std::byte, 64> bytes{};
    const std::array subresource{Neuron::GpuSubresourceData{bytes, 0}};
    const std::uint64_t fence = *factory.ScheduleUpload(buffer, subresource);
    factory.BeginFrame();
    factory.CompleteFramesThrough(fence);
    Assert::IsTrue(factory.HoldsData(buffer), L"the upload landed");
    const std::uint32_t textureIndex = factory.HeapIndex(texture);
    const std::uint32_t bufferIndex = factory.HeapIndex(buffer);

    factory.BeginRebuild();
    const auto created = factory.CreateTexture(Texture());
    Assert::IsTrue(!created.has_value() && created.error() == Neuron::GpuResourceError::DeviceRebuilding, L"creation is refused");
    const auto upload = factory.ScheduleUpload(buffer, subresource);
    Assert::IsTrue(!upload.has_value() && upload.error() == Neuron::GpuResourceError::DeviceRebuilding, L"an upload is refused");
    factory.EndRebuild();

    ExpectEqual(textureIndex, factory.HeapIndex(texture), L"the texture keeps its heap index");
    ExpectEqual(bufferIndex, factory.HeapIndex(buffer), L"the buffer keeps its heap index");
    Assert::IsFalse(factory.HoldsData(buffer), L"a rebuilt resource holds no data until its owner fills it again");
    Assert::IsTrue(factory.ScheduleUpload(buffer, subresource).has_value(), L"the owner fills it again after DeviceRebuilt");
  }
};

} // namespace NeuronClientTests
