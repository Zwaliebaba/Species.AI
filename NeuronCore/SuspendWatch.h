#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

namespace Neuron
{

/// The computer's suspends as the operating system reports them (MVP-071, PIL-131): a count that rises at each suspend,
/// which a world server reads to tell a sleep from a stall of its own (ADR-187). It registers for the system's suspend
/// and resume notifications when it is made and unregisters when it ends; a registration the system refuses leaves the
/// count at 0, and Watching says so. The count is read on any thread.
class SuspendWatch
{
public:
  SuspendWatch();
  ~SuspendWatch();

  SuspendWatch(const SuspendWatch&) = delete;
  SuspendWatch& operator=(const SuspendWatch&) = delete;

  [[nodiscard]] std::uint64_t Suspensions() const noexcept
  {
    return m_suspensions.load(std::memory_order_acquire);
  }
  [[nodiscard]] bool Watching() const noexcept
  {
    return m_registration != nullptr;
  }

  // The system's notification, from its own thread: a suspend counts once.
  void Notify(std::uint32_t _event) noexcept;

private:
  struct Subscription; // the system's subscription parameters, kept while it is registered

  std::atomic<std::uint64_t> m_suspensions{0};
  std::unique_ptr<Subscription> m_subscription;
  void* m_registration = nullptr;
};

} // namespace Neuron
