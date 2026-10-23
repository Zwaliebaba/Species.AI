#pragma once

#include <atomic>
#include <cstddef>
#include <optional>
#include <vector>

namespace Neuron
{

/// A bounded, lock-free queue between one producing thread and one consuming thread (ADR-026, AEN-020): its slots are
/// allocated once, a full queue refuses the newest value (AEN-021), and neither side ever waits or locks. Values are
/// copied in and out, so T is a plain value.
template <class T> class SpscQueue
{
public:
  explicit SpscQueue(std::size_t _capacity)
    : m_slots(_capacity + 1)
  {
  }

  SpscQueue(const SpscQueue&) = delete;
  SpscQueue& operator=(const SpscQueue&) = delete;

  // The producer's: false when the queue is full, and the value is dropped.
  bool Push(const T& _value) noexcept
  {
    const std::size_t tail = m_tail.load(std::memory_order_relaxed);
    const std::size_t next = Next(tail);
    if (next == m_head.load(std::memory_order_acquire))
    {
      return false;
    }
    m_slots[tail] = _value;
    m_tail.store(next, std::memory_order_release);
    return true;
  }

  // The consumer's: the oldest value, or none when the queue is empty.
  [[nodiscard]] std::optional<T> Pop() noexcept
  {
    const std::size_t head = m_head.load(std::memory_order_relaxed);
    if (head == m_tail.load(std::memory_order_acquire))
    {
      return std::nullopt;
    }
    T value = m_slots[head];
    m_head.store(Next(head), std::memory_order_release);
    return value;
  }

  [[nodiscard]] std::size_t Capacity() const noexcept
  {
    return m_slots.size() - 1;
  }

private:
  [[nodiscard]] std::size_t Next(std::size_t _slot) const noexcept
  {
    return _slot + 1 == m_slots.size() ? 0 : _slot + 1;
  }

  std::vector<T> m_slots; // one more than the capacity, so that a full queue differs from an empty one
  std::atomic<std::size_t> m_head{0};
  std::atomic<std::size_t> m_tail{0};
};

} // namespace Neuron
