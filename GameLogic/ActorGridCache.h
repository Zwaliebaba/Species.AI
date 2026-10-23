#pragma once

#include "ActorGrid.h"
#include "BitReader.h"
#include "BitWriter.h"

#include <cstdint>
#include <vector>

namespace Species
{

/// The actor grid cache of a region (R-SIM-315 to R-SIM-319): the memo of recent enemy queries that the Worker's threat
/// search reads, kept with the reference's softness (DEC-094's stated behaviour). A hit returns a list made for another
/// point and range, unfiltered; the caller resolves and re-checks every entry (R-SIM-317).
class ActorGridCache
{
public:
  struct Desc
  {
    double reuseFraction = 0.5;     // ref.sim.grid_cache.reuse_fraction
    std::uint64_t maxAgeTicks = 10; // ref.sim.grid_cache.max_age
  };

  explicit ActorGridCache(const Desc& _desc)
    : m_desc(_desc)
  {
  }

  // R-SIM-316: the list of the entry of _team whose centre is nearest (_x, _z) among those strictly within their half
  // range of it, or, on a miss, _enemies() stored as a new entry made at _tick.
  template <class Query>
  [[nodiscard]] std::vector<ActorRef> CachedEnemies(double _x, double _z, double _range, std::uint8_t _team, std::uint64_t _tick,
                                                    Query&& _enemies)
  {
    const Entry* nearest = nullptr;
    double nearestSquared = 0.0;
    for (const Entry& entry : m_entries)
    {
      const double dx = entry.x - _x;
      const double dz = entry.z - _z;
      const double squared = dx * dx + dz * dz;
      if (entry.team == _team && squared < entry.halfRangeSquared && (nearest == nullptr || squared < nearestSquared))
      {
        nearest = &entry;
        nearestSquared = squared;
      }
    }
    if (nearest != nullptr)
    {
      return nearest->refs;
    }
    const double half = m_desc.reuseFraction * _range;
    Entry& added = m_entries.emplace_back(Entry{_x, _z, _range * _range, half * half, _team, _tick, _enemies()});
    return added.refs;
  }

  // R-SIM-318: every entry older than the maximum age at _tick goes; run in slice ref.sim.grid_cache.evict_slice.
  void Evict(std::uint64_t _tick);
  // ADR-161: every entry goes, when a battle starts or ends and the teams' enemies change with it.
  void Clear() noexcept
  {
    m_entries.clear();
  }

  [[nodiscard]] std::size_t EntryCount() const noexcept
  {
    return m_entries.size();
  }

  // Every entry in order, its centre and ranges as binary64 (ADR-081 item 7) and its list as stable id, slot and team.
  void WriteState(Neuron::BitWriter& _writer) const;
  // Into an empty cache; false when the values do not read whole or are not finite.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader);

private:
  struct Entry
  {
    double x = 0.0;
    double z = 0.0;
    double rangeSquared = 0.0;
    double halfRangeSquared = 0.0;
    std::uint8_t team = 0;
    std::uint64_t tick = 0;
    std::vector<ActorRef> refs;
  };

  Desc m_desc;
  std::vector<Entry> m_entries; // in the order they were made
};

} // namespace Species
