#pragma once

#include "PlatformThread.h"

#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Neuron
{

/// One serial first-in-first-out worker on its own named thread (PLT-085). A job's future completes when the job has
/// run. Stop cancels the jobs not yet started, lets the running one finish and joins: a cancelled job's future
/// reports std::future_errc::broken_promise from get(). A job posted after Stop is cancelled at once.
class JobQueue
{
public:
  explicit JobQueue(std::string_view _threadName);
  ~JobQueue();

  JobQueue(const JobQueue&) = delete;
  JobQueue& operator=(const JobQueue&) = delete;

  template <class Job> [[nodiscard]] auto Post(Job&& _job) -> std::future<std::invoke_result_t<Job>>;

  void Stop();

private:
  void Run(const std::stop_token& _stop);
  void Enqueue(std::move_only_function<void()> _job);

  std::mutex m_mutex;
  std::condition_variable_any m_wake;
  std::deque<std::move_only_function<void()>> m_jobs;
  bool m_stopped = false;
  PlatformThread m_thread;
};

template <class Job> auto JobQueue::Post(Job&& _job) -> std::future<std::invoke_result_t<Job>>
{
  std::packaged_task<std::invoke_result_t<Job>()> task(std::forward<Job>(_job));
  std::future<std::invoke_result_t<Job>> future = task.get_future();
  Enqueue([task = std::move(task)]() mutable { task(); });
  return future;
}

} // namespace Neuron
