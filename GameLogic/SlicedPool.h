#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "Check.h"

#include <cstdint>
#include <deque>
#include <vector>

namespace Species
{

/// A sliced pool of the simulation (`SlicedPool`, R-SIM-204): slots that never move (R-SIM-209), handed out
/// last-freed-first and grown by a step (R-SIM-207), and advanced a share per update slice (R-SIM-213 to R-SIM-216). The
/// values live in a deque, so an object created during another's advance never moves the one being advanced.
template <class T> class SlicedPool
{
public:
  explicit SlicedPool(std::uint32_t _growthStep, std::uint32_t _initialSlots = 0)
    : m_growthStep(_growthStep)
  {
    ENGINE_CHECK(_growthStep > 0);
    Grow(_initialSlots);
  }

  // R-SIM-207: the head of the free list, after growing by the step when no slot is free.
  [[nodiscard]] std::uint32_t Take(const T& _value)
  {
    if (m_free.empty())
    {
      Grow(m_growthStep);
    }
    const std::uint32_t slot = m_free.back();
    m_free.pop_back();
    m_values[slot] = _value;
    m_used[slot] = 1;
    ++m_count;
    return slot;
  }

  // R-SIM-207: a freed slot becomes the head of the free list.
  void Free(std::uint32_t _slot)
  {
    ENGINE_CHECK(Used(_slot));
    m_values[_slot] = T{};
    m_used[_slot] = 0;
    m_free.push_back(_slot);
    --m_count;
  }

  // R-SIM-215: an emptied pool keeps its capacity and records slot 0 as its last advanced slot.
  void Clear()
  {
    for (std::uint32_t slot = 0; slot < Capacity(); ++slot)
    {
      m_values[slot] = T{};
      m_used[slot] = 0;
    }
    m_free.clear();
    for (std::uint32_t slot = Capacity(); slot > 0; --slot)
    {
      m_free.push_back(slot - 1);
    }
    m_count = 0;
    m_lastAdvanced = 0;
  }

  [[nodiscard]] bool Used(std::uint32_t _slot) const noexcept
  {
    return _slot < Capacity() && m_used[_slot] != 0;
  }
  [[nodiscard]] T& At(std::uint32_t _slot)
  {
    ENGINE_CHECK(Used(_slot));
    return m_values[_slot];
  }
  [[nodiscard]] const T& At(std::uint32_t _slot) const
  {
    ENGINE_CHECK(Used(_slot));
    return m_values[_slot];
  }
  [[nodiscard]] std::uint32_t Capacity() const noexcept
  {
    return static_cast<std::uint32_t>(m_values.size());
  }
  [[nodiscard]] std::uint32_t Count() const noexcept
  {
    return m_count;
  }
  // R-SIM-514: the last slot the pool has advanced in the current tick.
  [[nodiscard]] std::uint32_t LastAdvanced() const noexcept
  {
    return m_lastAdvanced;
  }

  struct SliceRange
  {
    std::uint32_t first = 0;
    std::uint32_t last = 0;
    bool empty = true;
  };

  // R-SIM-213 and R-SIM-514 at this moment's capacity: slice 0 starts at slot 0 and every later slice at the slot after
  // the last one advanced (R-SIM-215); slices before the last cover ⌊capacity ÷ slices⌋ + 1 slots and the last slice
  // runs to the end.
  [[nodiscard]] SliceRange RangeOf(std::uint32_t _slice, std::uint32_t _slices) const noexcept
  {
    const std::uint32_t capacity = Capacity();
    const std::uint32_t first = _slice == 0 ? 0 : m_lastAdvanced + 1;
    if (capacity == 0 || first >= capacity)
    {
      return {};
    }
    std::uint32_t last = capacity - 1;
    if (_slice + 1 < _slices)
    {
      const std::uint32_t share = first + capacity / _slices;
      last = share < last ? share : last;
    }
    return {first, last, false};
  }

  // One slice's share (R-SIM-206): each used slot of the range in ascending order, slots that become used while it runs
  // included. _advance(slot) returns true to ask for the slot's removal, which frees it after the advance.
  template <class Fn> void AdvanceSlice(std::uint32_t _slice, std::uint32_t _slices, Fn&& _advance)
  {
    const SliceRange range = RangeOf(_slice, _slices);
    if (range.empty)
    {
      return;
    }
    for (std::uint32_t slot = range.first; slot <= range.last; ++slot)
    {
      if (Used(slot) && _advance(slot))
      {
        Free(slot);
      }
    }
    m_lastAdvanced = range.last;
  }

  // The pool canonically (ADR-081 item 7): its capacity, its last advanced slot, its free list from the bottom to the
  // head, and each slot's use, a used slot followed by its value as _write writes it.
  template <class Fn> void WriteState(Neuron::BitWriter& _writer, Fn&& _write) const
  {
    _writer.WriteBits(Capacity(), 32);
    _writer.WriteBits(m_lastAdvanced, 32);
    _writer.WriteBits(m_free.size(), 32);
    for (const std::uint32_t slot : m_free)
    {
      _writer.WriteBits(slot, 32);
    }
    for (std::uint32_t slot = 0; slot < Capacity(); ++slot)
    {
      _writer.WriteBool(m_used[slot] != 0);
      if (m_used[slot] != 0)
      {
        _write(_writer, m_values[slot]);
      }
    }
  }

  // The values WriteState wrote, into an empty pool of the same growth step; false when they do not read whole or do
  // not describe a pool (a free list that names a used slot, a slot twice, or a slot beyond the capacity).
  template <class Fn> [[nodiscard]] bool ReadState(Neuron::BitReader& _reader, std::uint32_t _maxCapacity, Fn&& _read)
  {
    const auto capacity = static_cast<std::uint32_t>(_reader.ReadBits(32));
    const auto lastAdvanced = static_cast<std::uint32_t>(_reader.ReadBits(32));
    const auto freeCount = static_cast<std::uint32_t>(_reader.ReadBits(32));
    if (_reader.Failed() || capacity > _maxCapacity || freeCount > capacity || (capacity > 0 && lastAdvanced >= capacity))
    {
      return false;
    }
    m_values.assign(capacity, T{});
    m_used.assign(capacity, 0);
    m_free.clear();
    std::vector<std::uint8_t> listed(capacity, 0);
    for (std::uint32_t index = 0; index < freeCount; ++index)
    {
      const auto slot = static_cast<std::uint32_t>(_reader.ReadBits(32));
      if (_reader.Failed() || slot >= capacity || listed[slot] != 0)
      {
        return false;
      }
      listed[slot] = 1;
      m_free.push_back(slot);
    }
    m_count = 0;
    for (std::uint32_t slot = 0; slot < capacity; ++slot)
    {
      const bool used = _reader.ReadBool();
      if (_reader.Failed() || used == (listed[slot] != 0))
      {
        return false;
      }
      if (used)
      {
        if (!_read(_reader, m_values[slot]))
        {
          return false;
        }
        m_used[slot] = 1;
        ++m_count;
      }
    }
    m_lastAdvanced = lastAdvanced;
    return !_reader.Failed();
  }

private:
  // R-SIM-207: new slots follow the highest slot and are handed out in ascending order.
  void Grow(std::uint32_t _slots)
  {
    const std::uint32_t first = Capacity();
    m_values.resize(static_cast<std::size_t>(first) + _slots);
    m_used.resize(static_cast<std::size_t>(first) + _slots, 0);
    for (std::uint32_t slot = first + _slots; slot > first; --slot)
    {
      m_free.push_back(slot - 1);
    }
  }

  std::uint32_t m_growthStep;
  std::deque<T> m_values;
  std::vector<std::uint8_t> m_used;
  std::vector<std::uint32_t> m_free; // the head of the free list is the back
  std::uint32_t m_count = 0;
  std::uint32_t m_lastAdvanced = 0; // R-SIM-215: slot 0 for a new or emptied pool
};

} // namespace Species
