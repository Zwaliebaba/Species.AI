#pragma once

#include "BitReader.h"
#include "BitWriter.h"

#include <cstdint>
#include <vector>

namespace Species
{

class RegionSimulation;
struct TuningMvp;

// The first state version whose regions hold their settlements' renown and whose statistics hold the battles, claims,
// grievances and halfway lines (M3 task T8, ADR-163).
inline constexpr std::uint32_t STATE_VERSION_WITH_RENOWN = 15;

// MVP-061: the units the renown keys are stated in: renown for each 20 weight of grievance eased, and for each 100 ru of
// structures completed.
inline constexpr std::int64_t RENOWN_EASED_WEIGHT = 20;
inline constexpr std::int64_t RENOWN_STRUCTURE_RU = 100;

/// MVP-061 (ADR-163): what the settlement at `site` has done that earns renown, in the units its sources count: the
/// battles it won as a defender, the claim battles it won as a claimant, the weight of grievance it eased by tribute or by
/// a withdrawn claim in millionths, and the cost of the structures it completed in thousandths of a resource unit. The
/// renown itself is derived from them by the tuning (RenownOf). Region state from version 15.
struct SettlementRenown
{
  std::uint8_t site = 0;
  std::uint32_t defensesHeld = 0;
  std::uint32_t claimsWon = 0;
  std::int64_t easedMicro = 0;
  std::int64_t builtMilliRu = 0;

  bool operator==(const SettlementRenown&) const = default;
};

/// MVP-061 and MVP-062: a settlement's renown by source, and in all.
struct RenownPoints
{
  std::int64_t defense = 0;
  std::int64_t claims = 0;
  std::int64_t easing = 0;
  std::int64_t structures = 0;

  [[nodiscard]] std::int64_t Total() const noexcept
  {
    return defense + claims + easing + structures;
  }
};

[[nodiscard]] RenownPoints RenownOf(const TuningMvp& _tuning, const SettlementRenown& _renown) noexcept;

// MVP-062: the region's settlements by site in the order of their renown, the most first and the lower site on a tie.
[[nodiscard]] std::vector<std::uint8_t> Standing(const RegionSimulation& _region);

// The renown of the settlement at _site, or null for no settlement.
[[nodiscard]] SettlementRenown* RenownAt(RegionSimulation& _region, std::uint8_t _site) noexcept;
[[nodiscard]] const SettlementRenown* RenownAt(const RegionSimulation& _region, std::uint8_t _site) noexcept;

void WriteSettlementRenown(Neuron::BitWriter& _writer, const SettlementRenown& _renown);
[[nodiscard]] bool ReadSettlementRenown(Neuron::BitReader& _reader, SettlementRenown& _renown);

} // namespace Species
