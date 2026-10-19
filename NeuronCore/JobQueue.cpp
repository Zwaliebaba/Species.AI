#include "pch.h"

#include "JobQueue.h"

namespace Neuron
{

JobQueue::JobQueue(std::string_view _threadName)
{
  m_thread = PlatformThread::Start(_threadName, [this](const std::stop_token& _stop) { Run(_stop); });
}

JobQueue::~JobQueue()
{
  Stop();
}

void JobQueue::Stop()
{
  std::deque<std::move_only_function<void()>> canceled;
  {
    const std::lock_guard lock(m_mutex);
    m_stopped = true;
    canceled.swap(m_jobs);
  }
  // Destroying a job that never ran breaks its promise, which is how its future reports the cancellation, at once
  // and while the running job still runs.
  canceled.clear();
  m_thread.RequestStop();
  m_thread.Join();
}

void JobQueue::Enqueue(std::move_only_function<void()> _job)
{
  {
    const std::lock_guard lock(m_mutex);
    if (m_stopped)
    {
      return;
    }
    m_jobs.push_back(std::move(_job));
  }
  m_wake.notify_one();
}

void JobQueue::Run(const std::stop_token& _stop)
{
  while (true)
  {
    std::move_only_function<void()> job;
    {
      std::unique_lock lock(m_mutex);
      if (!m_wake.wait(lock, _stop, [this] { return !m_jobs.empty(); }))
      {
        return;
      }
      job = std::move(m_jobs.front());
      m_jobs.pop_front();
    }
    job();
  }
}

} // namespace Neuron
