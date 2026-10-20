#include "pch.h"

#include "Check.h"
#include "StableIdCounter.h"

namespace Species
{

StableId StableIdCounter::Issue()
{
  // 2^48 ids per owner: a region that issued a million a second would run for nine years.
  ENGINE_CHECK(m_next <= STABLE_ID_COUNTER_MAX, "owner {} has issued every stable id", m_owner);
  return MakeStableId(m_owner, m_next++);
}

} // namespace Species
