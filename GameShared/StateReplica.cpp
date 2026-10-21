#include "pch.h"

#include "StateReplica.h"

#include <algorithm>
#include <iterator>

namespace Species
{

void StateReplica::Apply(const StateUpdate& _update)
{
  ++m_counts.updates;
  const bool late = m_latestTick && _update.tick < *m_latestTick;
  const bool newer = !m_latestTick || _update.tick > *m_latestTick;
  if (late)
  {
    ++m_counts.lateUpdates;
  }
  else if (newer)
  {
    m_previous = m_latest;
    m_previousTick = m_latestTick;
    m_latestTick = _update.tick;
  }
  const std::optional<std::uint64_t> baselineTick = _update.BaselineTick();
  for (const StateRecord& record : _update.records)
  {
    ++m_counts.records;
    if (record.kind == RecordKind::Removed)
    {
      // An id is never used again, so a removal holds at every tick, a late one too.
      m_latest.erase(record.id);
      m_previous.erase(record.id);
      m_history.erase(record.id);
      continue;
    }
    const ReplicatedState* baseline = nullptr;
    if (record.kind == RecordKind::Delta)
    {
      baseline = baselineTick ? Baseline(record.id, *baselineTick) : nullptr;
      if (baseline == nullptr)
      {
        ++m_counts.missingBaselines;
        continue;
      }
    }
    std::optional<ReplicatedState> state = ApplyRecord(record, baseline);
    if (!state)
    {
      ++m_counts.missingBaselines; // a baseline of another class
      continue;
    }
    if (!late)
    {
      if (record.kind == RecordKind::Whole)
      {
        m_previous.erase(record.id);
      }
      m_latest.insert_or_assign(record.id, *state);
    }
    Remember(record.id, _update.tick, *state);
  }
  if (newer)
  {
    Prune();
  }
}

void StateReplica::Clear()
{
  m_history.clear();
  m_latest.clear();
  m_previous.clear();
  m_latestTick.reset();
  m_previousTick.reset();
}

const ReplicatedState* StateReplica::Baseline(StableId _id, std::uint64_t _tick) const
{
  const auto found = m_history.find(_id);
  if (found == m_history.end())
  {
    return nullptr;
  }
  for (const Received& received : found->second)
  {
    if (received.tick == _tick)
    {
      return &received.state;
    }
  }
  return nullptr;
}

void StateReplica::Remember(StableId _id, std::uint64_t _tick, const ReplicatedState& _state)
{
  std::vector<Received>& received = m_history[_id];
  const auto at = std::ranges::lower_bound(received, _tick, {}, &Received::tick);
  if (at != received.end() && at->tick == _tick)
  {
    at->state = _state;
    return;
  }
  received.insert(at, Received{_tick, _state});
}

void StateReplica::Prune()
{
  if (!m_latestTick || *m_latestTick < STATE_HISTORY_TICKS)
  {
    return;
  }
  const std::uint64_t oldest = *m_latestTick - STATE_HISTORY_TICKS;
  for (auto entry = m_history.begin(); entry != m_history.end();)
  {
    std::erase_if(entry->second, [oldest](const Received& _received) { return _received.tick < oldest; });
    entry = entry->second.empty() ? m_history.erase(entry) : std::next(entry);
  }
}

} // namespace Species
