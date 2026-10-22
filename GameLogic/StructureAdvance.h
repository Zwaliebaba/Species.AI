#pragma once

#include "Figure.h"
#include "Structure.h"
#include "TuningMvp.h"

#include <cstdint>

namespace Species
{

class RegionSimulation;

// R-SIM-218 step 4 for one structure: R-BLDG-031's base advance, which is its port evaluation (R-BLDG-021,
// R-BLDG-022): each port's occupant is the first Worker in actor-grid result order, of any team, that is Manning that
// very port of this very structure and stands strictly within ref.bldg.port.radius_wu of it horizontally; an occupied
// port's reservation counts are all 0 and an unoccupied port's fall by ref.bldg.port.reservation_decay_per_tick, never
// below 0. A built dwelling's or storehouse's ports have no occupant (PortsMannable). Then a queued construction site
// gains a Worker-tick for each occupied port, and completes when it has its RequiredWorkTicks: it is built, it leaves its
// settlement's queue, and it raises StructureCompleted (MVP-033). A built turret first runs its own advance
// (AdvanceTurret). True when the structure asks to be removed.
[[nodiscard]] bool AdvanceStructure(RegionSimulation& _region, Structure& _structure);

// R-BLDG-023: a Worker of _team approaching the port adds 1 to its team's reservation count, up to
// ref.bldg.port.reservation_cap.
void OperatePort(StructurePort& _port, std::uint8_t _team, const TuningMvp& _tuning);
// R-BLDG-023: the port has no occupant and _team's reservation count is below ref.bldg.port.reservation_limit.
[[nodiscard]] bool PortAvailable(const StructurePort& _port, std::uint8_t _team, const TuningMvp& _tuning) noexcept;

// The port that _worker walks to or operates, while its structure still holds its slot and has that port; or null.
[[nodiscard]] StructurePort* TargetPort(RegionSimulation& _region, const Figure& _worker);

// R-UNIT-090's last step for a chosen port: the Worker records the structure and the port, its waypoint becomes the
// port's position, and it becomes ToPort.
void SendToPort(Figure& _worker, const Structure& _structure, std::uint32_t _structureSlot, std::uint32_t _port);

} // namespace Species
