#pragma once

#include "StableId.h"

#include <cstdint>

namespace Species
{

/// An owner's creation counter (ADR-083 item 5): the next stable id it issues. The counter is owner state, saved and
/// hashed with the owner, so no id is issued twice in a world's life.
class StableIdCounter
{
public:
  explicit StableIdCounter(OwnerId _owner, std::uint64_t _next = 1) noexcept
    : m_owner(_owner),
      m_next(_next)
  {
  }

  [[nodiscard]] StableId Issue();

  [[nodiscard]] OwnerId Owner() const noexcept
  {
    return m_owner;
  }
  [[nodiscard]] std::uint64_t Next() const noexcept
  {
    return m_next;
  }

private:
  OwnerId m_owner;
  std::uint64_t m_next;
};

} // namespace Species
