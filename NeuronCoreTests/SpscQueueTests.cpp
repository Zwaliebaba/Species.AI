#include "pch.h"

#include "PlatformThread.h"
#include "SpscQueue.h"

#include <cstdint>
#include <optional>
#include <stop_token>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(SpscQueueTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Aen021AFullQueueRefusesTheNewestValue)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-020")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-021")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen021AFullQueueRefusesTheNewestValue)
  {
    Neuron::SpscQueue<int> queue(3);
    Assert::IsTrue(queue.Capacity() == 3, L"it holds its capacity");
    Assert::IsFalse(queue.Pop().has_value(), L"a new queue is empty");
    for (int round = 0; round < 4; ++round) // the slots wrap around
    {
      Assert::IsTrue(queue.Push(round * 10 + 1) && queue.Push(round * 10 + 2) && queue.Push(round * 10 + 3), L"three fit");
      Assert::IsFalse(queue.Push(99), L"the fourth is refused");
      Assert::IsTrue(queue.Pop() == round * 10 + 1 && queue.Pop() == round * 10 + 2, L"first in, first out");
      Assert::IsTrue(queue.Push(round * 10 + 4), L"a popped slot is free again");
      Assert::IsTrue(queue.Pop() == round * 10 + 3 && queue.Pop() == round * 10 + 4, L"in order");
      Assert::IsFalse(queue.Pop().has_value(), L"and empty");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr026ValuesCrossBetweenTwoThreadsInOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-301")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr026ValuesCrossBetweenTwoThreadsInOrder)
  {
    constexpr std::uint32_t COUNT = 200'000;
    Neuron::SpscQueue<std::uint32_t> queue(64);
    const auto produce = [&queue](const std::stop_token& /*_stop*/)
    {
      for (std::uint32_t value = 0; value < COUNT;)
      {
        if (queue.Push(value))
        {
          ++value;
        }
      }
    };
    Neuron::PlatformThread producer = Neuron::PlatformThread::Start("producer", produce);
    std::uint32_t expected = 0;
    bool ordered = true;
    while (expected < COUNT)
    {
      if (const std::optional<std::uint32_t> value = queue.Pop())
      {
        ordered = ordered && *value == expected;
        ++expected;
      }
    }
    producer.Join();
    Assert::IsTrue(ordered, L"every value arrives once, in the order pushed");
  }
};

} // namespace NeuronCoreTests
