#pragma once

#include "PlatformThread.h"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <mutex>
#include <vector>

namespace Neuron
{

/// The server's workers (ADR-083 item 6). The tick thread hands them the jobs of one phase, such as one region step
/// per region, takes part in them itself and returns when every job has run. The jobs of a phase are independent, so
/// the result is that of the serial order at any number of workers (R-SIM-732). With no workers, the tick thread runs
/// every job itself.
class WorkerPool
{
public:
  // Starts _workers threads, named "server.worker.<n>".
  explicit WorkerPool(std::size_t _workers);
  ~WorkerPool();

  WorkerPool(const WorkerPool&) = delete;
  WorkerPool& operator=(const WorkerPool&) = delete;

  // Runs _job(i) for every i below _count and returns when all have run. When a job throws, the others still run,
  // and the first exception is thrown here. Not reentrant: one phase at a time, from one thread.
  void Run(std::size_t _count, const std::function<void(std::size_t)>& _job);

  [[nodiscard]] std::size_t Workers() const noexcept
  {
    return m_threads.size();
  }

private:
  void Work(const std::stop_token& _stop);
  void RunJobs();

  std::mutex m_mutex;
  std::condition_variable_any m_wake;
  std::condition_variable m_done;
  const std::function<void(std::size_t)>* m_job = nullptr;
  std::size_t m_count = 0;
  std::size_t m_next = 0;
  std::size_t m_unfinished = 0;
  std::uint64_t m_generation = 0;
  std::exception_ptr m_failure;
  std::vector<PlatformThread> m_threads;
};

} // namespace Neuron
