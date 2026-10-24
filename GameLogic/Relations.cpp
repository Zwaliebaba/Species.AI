#include "pch.h"

#include "Check.h"
#include "RegionSimulation.h"
#include "Relations.h"
#include "Statistics.h"
#include "Territory.h"

#include <algorithm>
#include <cmath>
#include <tuple>
#include <utility>
#include <vector>

namespace Species
{

namespace
{

// A share in integer parts per billion.
constexpr std::int64_t PARTS_PER_BILLION = 1'000'000'000;
constexpr double SECONDS_PER_HOUR = 3'600.0;

// A grievance's place in the region's order.
auto KeyOf(const Grievance& _grievance) noexcept
{
  return std::tuple{_grievance.holder, _grievance.against, _grievance.cause, _grievance.subject};
}

// The grievance of the key, or none.
Grievance* FindGrievance(std::vector<Grievance>& _grievances, std::uint8_t _holder, std::uint8_t _against, GrievanceCause _cause,
                         std::uint64_t _subject) noexcept
{
  const auto key = std::tuple{_holder, _against, _cause, _subject};
  const auto found = std::ranges::find(_grievances, key, [](const Grievance& _grievance) { return KeyOf(_grievance); });
  return found != _grievances.end() ? &*found : nullptr;
}

// The grievance of the key, made with no weight in the region's order where the region has none, a new one recorded in
// the holder's statistics (MVP-064, ADR-163).
Grievance& GrievanceOf(RegionSimulation& _region, std::uint8_t _holder, std::uint8_t _against, GrievanceCause _cause,
                       std::uint64_t _subject)
{
  std::vector<Grievance>& grievances = _region.Grievances();
  const Grievance key{_holder, _against, _cause, _subject};
  const auto place = std::ranges::lower_bound(grievances, KeyOf(key), {}, [](const Grievance& _grievance) { return KeyOf(_grievance); });
  if (place != grievances.end() && KeyOf(*place) == KeyOf(key))
  {
    return *place;
  }
  const auto made = static_cast<std::size_t>(place - grievances.begin());
  grievances.insert(place, key);
  RecordEvent(_region, _holder, StatisticKind::GrievanceAdded, static_cast<std::uint32_t>(_cause));
  RecordAbsenceGrievance(_region, {_region.NextTick(), _holder, _against, _cause}); // MVP-074 (ADR-186)
  return grievances[made];
}

// _share of a weight unit's millionths, as parts per billion.
std::int64_t PartsPerBillion(double _share) noexcept
{
  return std::llround(std::clamp(_share, 0.0, 1.0) * static_cast<double>(PARTS_PER_BILLION));
}

// _partsPerBillion of _weightMicro, to the nearest millionth, with no product that overflows.
std::int64_t ShareOf(std::int64_t _weightMicro, std::int64_t _partsPerBillion) noexcept
{
  return _weightMicro / PARTS_PER_BILLION * _partsPerBillion +
         (_weightMicro % PARTS_PER_BILLION * _partsPerBillion + PARTS_PER_BILLION / 2) / PARTS_PER_BILLION;
}

// MVP-052: the share of its weight a grievance loses in _seconds, at _percentPerHour of it an hour, _factor times faster;
// an hour at the rate leaves (1 − _percentPerHour / 100) of the weight.
std::int64_t FadeOver(double _percentPerHour, double _factor, double _seconds) noexcept
{
  const double keep = std::pow(1.0 - std::clamp(_percentPerHour / 100.0, 0.0, 1.0), _factor * _seconds / SECONDS_PER_HOUR);
  return PartsPerBillion(1.0 - keep);
}

} // namespace

std::int64_t AngerMicro(const RegionSimulation& _region, std::uint8_t _holder, std::uint8_t _against) noexcept
{
  std::int64_t anger = 0;
  for (const Grievance& grievance : _region.Grievances())
  {
    if (grievance.holder == _holder && grievance.against == _against)
    {
      anger += grievance.weightMicro;
    }
  }
  return anger;
}

bool InContact(const RegionSimulation& _region, std::uint8_t _first, std::uint8_t _second)
{
  for (std::size_t pair = 0; pair < VALLEY_PAIRS.size(); ++pair)
  {
    const auto& sites = VALLEY_PAIRS[pair];
    if ((sites[0] == _first && sites[1] == _second) || (sites[0] == _second && sites[1] == _first))
    {
      return _region.SettlementAt(_first) != nullptr && _region.SettlementAt(_second) != nullptr &&
             TerritoriesMeet(_region, _first, _second);
    }
  }
  return false;
}

void AddGrievance(RegionSimulation& _region, std::uint8_t _holder, std::uint8_t _against, GrievanceCause _cause, std::uint64_t _subject,
                  bool _unprovoked)
{
  ENGINE_ASSERT(_cause != GrievanceCause::Pressure && _holder != _against && _region.SettlementAt(_holder) != nullptr &&
                _region.SettlementAt(_against) != nullptr);
  const auto& tuning = _region.Tuning().mvp.grievance;
  double weight = _cause == GrievanceCause::Claim ? tuning.claimWeight : tuning.attackWeight;
  if (_cause == GrievanceCause::Attack && _unprovoked)
  {
    weight *= tuning.unprovokedFactor;
  }
  Grievance& grievance = GrievanceOf(_region, _holder, _against, _cause, _subject);
  grievance.unprovoked = grievance.unprovoked || _unprovoked;
  grievance.weightMicro += std::llround(weight * static_cast<double>(WEIGHT_MICRO));
}

void RemoveCause(RegionSimulation& _region, GrievanceCause _cause, std::uint64_t _subject)
{
  for (Grievance& grievance : _region.Grievances())
  {
    if (grievance.cause == _cause && grievance.subject == _subject)
    {
      grievance.causeRemoved = true;
    }
  }
}

void RelieveAfterBattle(RegionSimulation& _region, std::uint8_t _attacker, std::uint8_t _target, bool _attackerWon)
{
  const auto& tuning = _region.Tuning().mvp.grievance;
  const std::int64_t relief = PartsPerBillion((_attackerWon ? tuning.battleReliefWon : tuning.battleReliefLost) / 100.0);
  for (Grievance& grievance : _region.Grievances())
  {
    if (grievance.holder == _attacker && grievance.against == _target)
    {
      grievance.weightMicro -= ShareOf(grievance.weightMicro, relief);
    }
  }
}

void AdvanceRelations(RegionSimulation& _region, std::uint64_t _tick)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto ticksPerSecond = static_cast<std::uint64_t>(std::max<std::int64_t>(1, std::llround(1.0 / tuning.ref.sim.tickPeriod)));
  if (_tick % ticksPerSecond != 0 || _region.Terrain() == nullptr)
  {
    return;
  }
  const double seconds = static_cast<double>(ticksPerSecond) * tuning.ref.sim.tickPeriod;
  const auto& grievance = tuning.mvp.grievance;
  std::vector<Grievance>& grievances = _region.Grievances();
  // MVP-051: territory pressure, each way, while a pair of valley neighbours is in contact.
  const std::int64_t pressure = std::llround(grievance.pressurePerHour * static_cast<double>(WEIGHT_MICRO) * seconds / SECONDS_PER_HOUR);
  for (const auto& pair : VALLEY_PAIRS)
  {
    if (_region.SettlementAt(pair[0]) == nullptr || _region.SettlementAt(pair[1]) == nullptr)
    {
      continue;
    }
    const bool contact = InContact(_region, pair[0], pair[1]);
    for (const auto& [holder, against] : {std::pair{pair[0], pair[1]}, std::pair{pair[1], pair[0]}})
    {
      if (contact)
      {
        Grievance& held = GrievanceOf(_region, holder, against, GrievanceCause::Pressure, 0);
        held.causeRemoved = false;
        held.weightMicro += pressure;
      }
      else if (Grievance* held = FindGrievance(grievances, holder, against, GrievanceCause::Pressure, 0); held != nullptr)
      {
        held->causeRemoved = true;
      }
    }
  }
  // MVP-052 and PIL-122: the fade, faster once the cause is removed; such a grievance ends below half a unit.
  const std::int64_t fade = FadeOver(grievance.fadeSharePerHour, 1.0, seconds);
  const std::int64_t settledFade = FadeOver(grievance.fadeSharePerHour, tuning.pil.conflict.settledFadeFactor, seconds);
  for (Grievance& held : grievances)
  {
    held.weightMicro -= ShareOf(held.weightMicro, held.causeRemoved ? settledFade : fade);
  }
  std::erase_if(grievances, [](const Grievance& _held) { return _held.causeRemoved && _held.weightMicro < WEIGHT_MICRO / 2; });
}

void WriteGrievance(Neuron::BitWriter& _writer, const Grievance& _grievance)
{
  _writer.WriteBits(_grievance.holder, 8);
  _writer.WriteBits(_grievance.against, 8);
  _writer.WriteBounded(static_cast<std::uint64_t>(_grievance.cause), GRIEVANCE_CAUSE_MAX);
  _writer.WriteBits(_grievance.subject, 64);
  _writer.WriteBool(_grievance.unprovoked);
  _writer.WriteBool(_grievance.causeRemoved);
  _writer.WriteSigned(_grievance.weightMicro, 64);
}

bool ReadGrievance(Neuron::BitReader& _reader, Grievance& _grievance)
{
  _grievance.holder = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _grievance.against = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _grievance.cause = static_cast<GrievanceCause>(_reader.ReadBounded(GRIEVANCE_CAUSE_MAX));
  _grievance.subject = _reader.ReadBits(64);
  _grievance.unprovoked = _reader.ReadBool();
  _grievance.causeRemoved = _reader.ReadBool();
  _grievance.weightMicro = _reader.ReadSigned(64);
  return !_reader.Failed() && _grievance.holder != _grievance.against && _grievance.weightMicro >= 0 &&
         (_grievance.cause == GrievanceCause::Pressure) == (_grievance.subject == 0);
}

} // namespace Species
