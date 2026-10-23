#include "pch.h"

#include "RegionSimulation.h"
#include "Relations.h"
#include "Renown.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Species
{

namespace
{

// A region's local sites are 0 to 3 (WLD-040).
constexpr std::uint64_t LOCAL_SITE_MAX = 3;
constexpr std::int64_t MILLI_RU_PER_RU = 1'000;

std::int64_t Points(double _perUnit, std::int64_t _units) noexcept
{
  return static_cast<std::int64_t>(std::llround(_perUnit * static_cast<double>(_units)));
}

} // namespace

RenownPoints RenownOf(const TuningMvp& _tuning, const SettlementRenown& _renown) noexcept
{
  const auto& renown = _tuning.mvp.renown;
  RenownPoints points;
  points.defense = renown.defenseHeld * std::int64_t{_renown.defensesHeld};
  points.claims = renown.claimWon * std::int64_t{_renown.claimsWon};
  points.easing = Points(renown.grievanceEased, _renown.easedMicro / (RENOWN_EASED_WEIGHT * WEIGHT_MICRO));
  points.structures = Points(renown.structurePer100ru, _renown.builtMilliRu / (RENOWN_STRUCTURE_RU * MILLI_RU_PER_RU));
  return points;
}

std::vector<std::uint8_t> Standing(const RegionSimulation& _region)
{
  std::vector<std::pair<std::int64_t, std::uint8_t>> ranked;
  ranked.reserve(_region.Renown().size());
  for (const SettlementRenown& renown : _region.Renown())
  {
    ranked.emplace_back(RenownOf(_region.Tuning(), renown).Total(), renown.site);
  }
  std::ranges::sort(ranked,
                    [](const auto& _a, const auto& _b) { return _a.first != _b.first ? _a.first > _b.first : _a.second < _b.second; });
  std::vector<std::uint8_t> standing;
  standing.reserve(ranked.size());
  for (const auto& [points, site] : ranked)
  {
    standing.push_back(site);
  }
  return standing;
}

SettlementRenown* RenownAt(RegionSimulation& _region, std::uint8_t _site) noexcept
{
  const auto found = std::ranges::find(_region.Renown(), _site, &SettlementRenown::site);
  return found != _region.Renown().end() ? &*found : nullptr;
}

const SettlementRenown* RenownAt(const RegionSimulation& _region, std::uint8_t _site) noexcept
{
  const auto found = std::ranges::find(_region.Renown(), _site, &SettlementRenown::site);
  return found != _region.Renown().end() ? &*found : nullptr;
}

void WriteSettlementRenown(Neuron::BitWriter& _writer, const SettlementRenown& _renown)
{
  _writer.WriteBounded(_renown.site, LOCAL_SITE_MAX);
  _writer.WriteBits(_renown.defensesHeld, 32);
  _writer.WriteBits(_renown.claimsWon, 32);
  _writer.WriteSigned(_renown.easedMicro, 64);
  _writer.WriteSigned(_renown.builtMilliRu, 64);
}

bool ReadSettlementRenown(Neuron::BitReader& _reader, SettlementRenown& _renown)
{
  _renown.site = static_cast<std::uint8_t>(_reader.ReadBounded(LOCAL_SITE_MAX));
  _renown.defensesHeld = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _renown.claimsWon = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _renown.easedMicro = _reader.ReadSigned(64);
  _renown.builtMilliRu = _reader.ReadSigned(64);
  return !_reader.Failed() && _renown.easedMicro >= 0 && _renown.builtMilliRu >= 0;
}

} // namespace Species
