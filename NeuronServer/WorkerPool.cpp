#include "pch.h"

#include "WorkerPool.h"

#include <format>

namespace Neuron
{

WorkerPool::WorkerPool(std::size_t _workers)
{
  m_threads.reserve(_workers);
  for (std::size_t i = 0; i < _workers; ++i)
  {
    m_threads.push_back(PlatformThread::Start(std::format("server.worker.{}", i), [this](const std::stop_token& _stop) { Work(_stop); }));
  }
}

WorkerPool::~WorkerPool()
{
  for (PlatformThread& thread : m_threads)
  {
    thread.RequestStop();
  }
  m_wake.notify_all();
  for (PlatformThread& thread : m_threads)
  {
    thread.Join();
  }
}

void WorkerPool::Run(std::size_t _count, const std::function<void(std::size_t)>& _job)
{
  if (_count == 0)
  {
    return;
  }
  {
    const std::scoped_lock lock(m_mutex);
    m_job = &_job;
    m_count = _count;
    m_next = 0;
    m_unfinished = _count;
    m_failure = nullptr;
    ++m_generation;
  }
  m_wake.notify_all();
  RunJobs();
  std::exception_ptr failure;
  {
    std::unique_lock lock(m_mutex);
    m_done.wait(lock, [this] { return m_unfinished == 0; });
    m_job = nullptr;
    failure = m_failure;
  }
  if (failure)
  {
    std::rethrow_exception(failure);
  }
}

void WorkerPool::Work(const std::stop_token& _stop)
{
  std::uint64_t seen = 0;
  while (true)
  {
    {
      std::unique_lock lock(m_mutex);
      if (!m_wake.wait(lock, _stop, [this, seen] { return m_generation != seen; }))
      {
        return; // stop was requested
      }
      seen = m_generation;
    }
    RunJobs();
  }
}

void WorkerPool::RunJobs()
{
  while (true)
  {
    std::size_t index = 0;
    const std::function<void(std::size_t)>* job = nullptr;
    {
      const std::scoped_lock lock(m_mutex);
      if (m_job == nullptr || m_next == m_count)
      {
        return;
      }
      index = m_next++;
      job = m_job;
    }
    try
    {
      (*job)(index);
    }
    catch (...)
    {
      const std::scoped_lock lock(m_mutex);
      if (!m_failure)
      {
        m_failure = std::current_exception();
      }
    }
    bool last = false;
    {
      const std::scoped_lock lock(m_mutex);
      last = --m_unfinished == 0;
    }
    if (last)
    {
      m_done.notify_all();
    }
  }
}

} // namespace Neuron
