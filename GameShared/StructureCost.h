#pragma once

#include "StructureKind.h"
#include "TuningMvp.h"

#include <cstdint>

namespace Species
{

// MVP-034: a kind's cost in thousandths of a resource unit for a settlement that already has _dwellings dwellings, built
// or queued: a dwelling's grows by mvp.cost.dwelling_escalation for each, a storehouse costs mvp.cost.storehouse_ru and
// a turret mvp.cost.turret_ru. The server charges it and the HUD shows it (MVP-102), so both read this one function.
[[nodiscard]] std::int64_t StructureCostMilliRu(const TuningMvp& _tuning, StructureKind _kind, std::int64_t _dwellings) noexcept;

} // namespace Species
