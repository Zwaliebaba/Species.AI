#pragma once

#include "RegionTerrain.h"
#include "SipHash.h"
#include "StructureRecords.h"
#include "TuningMvp.h"
#include "WorldSimulation.h"

#include <array>
#include <cstdint>
#include <memory>

namespace Species
{

// The valley at war's two battles (M3 plan T9, ADR-164): a raid on the steward's settlement by its first valley
// neighbour, and a raid between the two other settlements.
struct ValleyRaid
{
  std::uint8_t attacker = 0;
  std::uint8_t defender = 0;
};

[[nodiscard]] std::array<ValleyRaid, 2> ValleyRaidsOf(std::uint8_t _stewardSite) noexcept;

/// The valley at war (M3 plan T9, ADR-164): the measurement scene of the region at the MVP's bound, four settlements at
/// their population with two battles in progress (MVP-015, MVP-022, ADR-080 item 10). The steward's settlement at
/// mvp.sandbox.steward_site and three AI settlements, the steward's run by a scripted steward with the decisions of a
/// Territorial settlement; the two raids of ValleyRaidsOf, each of two Leaders with _expedition Workers between them,
/// declared at once; then each settlement filled at home to _population (0: mvp.settlement.population_max), its
/// expedition among it, and stepped until both battles are in progress, or for _settleTicks at most. Its run never ends,
/// so it can be measured for as long as wanted.
struct ValleyAtWarDesc
{
  Neuron::SipHashKey key;
  std::shared_ptr<const RegionTerrain> terrain;
  const TuningMvp* tuning = nullptr;
  const StructureRecords* structures = nullptr;
  std::uint32_t population = 0;
  std::uint32_t expedition = 200;
  std::uint64_t settleTicks = 6'000;
};

[[nodiscard]] std::unique_ptr<WorldSimulation> StartValleyAtWar(const ValleyAtWarDesc& _desc);

} // namespace Species
