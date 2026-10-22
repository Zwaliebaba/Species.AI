#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "StableId.h"
#include "Structure.h"
#include "TuningMvp.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Species
{

// A settlement's fidelity (PIL-057): always full in the MVP, which defers the abstract model and fidelity switching
// (MVP-096). A state value that is never renumbered.
enum class Fidelity : std::uint8_t
{
  Full = 0
};

inline constexpr std::uint64_t FIDELITY_MAX = 0;

// An AI settlement's temperament (MVP-057); the steward's settlement has none. A state value that is never renumbered.
enum class Temperament : std::uint8_t
{
  None = 0,
  Bold = 1,
  Territorial = 2,
  Wary = 3
};

inline constexpr std::uint64_t TEMPERAMENT_MAX = 3;

/// A modifier of a Cladan (PIL-007, O7, O11): the place that a later stage fills, empty in the MVP (MVP-099). It names
/// the stat it changes by that stat's tuning key, and the factor it scales it by.
struct CladanModifier
{
  std::string key;
  double factor = 1.0;
};

/// A settlement's Cladan (MVP-013): its colour, one of the four of a world, its temperament and its modifiers.
struct Cladan
{
  std::uint8_t color = 0; // the colour of the settlement's site, so that the four Cladans of a world differ (ADR-130)
  Temperament temperament = Temperament::None;
  std::vector<CladanModifier> modifiers;
};

/// A settlement, owned by the job of its region (MVP-096, ADR-130): its site, the team slot of its Workers, which is its
/// site, whether a steward runs it or it is an AI settlement (PIL-007), its Cladan, its fidelity, its Wellspring, its
/// stores in integer thousandths of a resource unit (MVP-030), its territory's radius, which never shrinks (MVP-026),
/// its construction sites in the order they are built (MVP-032, ADR-132), its Wellspring's growth timer (MVP-024,
/// ADR-134) and its priorities (MVP-036, ADR-135).
struct Settlement
{
  std::uint8_t site = 0;
  std::uint8_t team = 0;
  bool stewarded = false;
  Cladan cladan;
  Fidelity fidelity = Fidelity::Full;
  StableId wellspringId = 0;
  std::uint32_t wellspringSlot = 0;
  std::int64_t storesMilliRu = 0;
  double territoryRadius = 0.0; // wu
  std::vector<StructureRef> queue;
  std::int32_t growthTicks = 0; // MVP-024: until the Wellspring's next orb, counting only while it may grow (ADR-134)
  // MVP-036 (ADR-135): its shares of gathering, construction and guard duty in percent, in WorkerTask's order from
  // Gather, summing to 100.
  std::array<std::uint8_t, 3> priorities{};
  // MVP-035 (ADR-135): the upkeep accrued and not yet due, in thousandths of a resource unit × ticks per hour, and the
  // ticks to its next step of inactive structures, one way or the other.
  std::int64_t upkeepCarry = 0;
  std::int32_t inactiveTicks = 0;
};

// MVP-030: a tuning amount of resource units in thousandths, rounded once to the nearest.
[[nodiscard]] std::int64_t MilliRuOf(double _resourceUnits) noexcept;
// MVP-021, MVP-034: the stores' capacity, the Wellspring's and each active storehouse's.
[[nodiscard]] std::int64_t StoreCapacityMilliRu(const TuningMvp& _tuning, std::uint32_t _activeStorehouses) noexcept;
// MVP-031: the stores take the whole of _amount when it fits under _capacity and are unchanged otherwise, as a full
// store refuses a load.
[[nodiscard]] bool Deposit(Settlement& _settlement, std::int64_t _amountMilliRu, std::int64_t _capacityMilliRu) noexcept;
// MVP-032: the stores pay the whole of _amount when they hold it and are unchanged otherwise.
[[nodiscard]] bool Withdraw(Settlement& _settlement, std::int64_t _amountMilliRu) noexcept;
// MVP-022: mvp.settlement.population_base and mvp.dwelling.population for each active dwelling, never above
// mvp.settlement.population_max.
[[nodiscard]] std::uint32_t PopulationLimit(const TuningMvp& _tuning, std::uint32_t _activeDwellings) noexcept;

// A settlement canonically, as its region's state holds it (ADR-081 item 7), and the values Write wrote; Read is false
// when they do not read whole.
void WriteSettlement(Neuron::BitWriter& _writer, const Settlement& _settlement);
[[nodiscard]] bool ReadSettlement(Neuron::BitReader& _reader, Settlement& _settlement);

} // namespace Species
