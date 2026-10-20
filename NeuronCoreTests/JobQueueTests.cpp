#include "pch.h"

#include "JobQueue.h"
#include "PlatformThread.h"
#include "TestSupport.h"

#include <atomic>
#include <future>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

bool IsCanceled(std::future<int>& _future)
{
  try
  {
    _future.get();
  }
  catch (const std::future_error& error)
  {
    return error.code() == std::future_errc::broken_promise;
  }
  return false;
}

} // namespace

TEST_CLASS(JobQueueTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt085JobsRunInOrderAndTheirFuturesComplete)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-085")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt085JobsRunInOrderAndTheirFuturesComplete)
  {
    // PLT T27, first half.
    Neuron::JobQueue queue("test.jobs");
    std::vector<int> order;
    std::vector<std::future<int>> futures;
    futures.reserve(100);
    for (int index = 0; index < 100; ++index)
    {
      futures.push_back(queue.Post(
        [&order, index]
        {
          order.push_back(index);
          return index * 2;
        }));
    }
    for (int index = 0; index < 100; ++index)
    {
      ExpectEqual(static_cast<std::int64_t>(index) * 2, futures[static_cast<std::size_t>(index)].get(), L"result");
    }
    ExpectEqual(100, static_cast<std::int64_t>(order.size()), L"jobs run");
    for (int index = 0; index < 100; ++index)
    {
      ExpectEqual(index, order[static_cast<std::size_t>(index)], L"order");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt085StopCancelsTheJobsNotStarted)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-085")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt085StopCancelsTheJobsNotStarted)
  {
    // PLT T27, second half: the running job finishes, the waiting ones report cancellation, and Stop joins.
    Neuron::JobQueue queue("test.jobs");
    std::atomic<bool> running{false};
    std::atomic<bool> release{false};
    std::future<int> blocking = queue.Post(
      [&]
      {
        running.store(true, std::memory_order_release);
        release.wait(false, std::memory_order_acquire);
        return 1;
      });
    std::future<int> waiting = queue.Post([] { return 2; });
    std::future<int> alsoWaiting = queue.Post([] { return 3; });
    while (!running.load(std::memory_order_acquire))
    {
      std::this_thread::yield();
    }
    Neuron::PlatformThread stopper = Neuron::PlatformThread::Start("test.stopper", [&](const std::stop_token&) { queue.Stop(); });
    waiting.wait();
    Assert::IsTrue(IsCanceled(waiting), L"a waiting job reports cancellation while the running one still runs");
    release.store(true, std::memory_order_release);
    release.notify_all();
    stopper.Join();
    ExpectEqual(1, blocking.get(), L"the running job finished");
    Assert::IsTrue(IsCanceled(alsoWaiting), L"every waiting job is cancelled");
    std::future<int> late = queue.Post([] { return 4; });
    Assert::IsTrue(IsCanceled(late), L"a job posted after Stop is cancelled at once");
  }
};

} // namespace NeuronCoreTests
