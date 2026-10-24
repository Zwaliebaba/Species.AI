#pragma once

#include "Combat.h"
#include "Figure.h"
#include "Settlement.h"
#include "Vec3.h"

#include <array>
#include <cstdint>

namespace Species
{

class RegionSimulation;

// The first state version whose regions hold the gatherers' home ports (O112, ADR-195).
inline constexpr std::uint32_t STATE_VERSION_WITH_HOME_PORTS = 23;

// The settlement whose team is _team, or null for a team that is no settlement's, as the battle sandbox's are.
[[nodiscard]] Settlement* SettlementOfTeam(RegionSimulation& _region, std::uint8_t _team);

/// MVP-036: how many of a settlement's living Workers that follow no Leader hold each task, in WorkerTask's order from
/// Gather, and how many such Workers there are, all but one Worker that is searching.
struct TaskCounts
{
  std::array<std::uint32_t, 3> tasks{};
  std::uint32_t workers = 0;
};

[[nodiscard]] TaskCounts CountTasks(const RegionSimulation& _region, const Settlement& _settlement, StableId _searching);

// MVP-031: the Worker, which carries a load, walks to the nearest drop-off point of the settlement of _team; with none it
// stands idle. From M3 task T6 (ADR-161) a won raid's survivors carry the stores home this way.
void StartCarrying(RegionSimulation& _region, Figure& _worker, std::uint8_t _team);

// MVP-031, MVP-033, MVP-036 and MVP-037 (ADR-135): the settlement's part of an idle Worker's task search, after its
// orders and its Leader search. Each task that has work keeps its share, and the shares of those with none go to the
// others in proportion to theirs:
// - gathering has work while a port of a resource site in the territory has room for another gatherer: as many share a
//   port as there are turns at it in one gatherer's round to the nearest drop-off point and back (O112, ADR-195). A
//   gatherer goes back to its home port while that port has room for it; another takes the port with the fewest
//   gatherers, the nearest on a tie (O84);
// - construction while the first queued site has a free port;
// - guard duty always, at the nearest free port of a built, active turret of the settlement in its territory, or else
//   within mvp.guard.gather_radius of the Wellspring.
// The Worker takes the task furthest below its share of the Workers, in WorkerTask's order on a tie, and walks to its
// port along a route, or, a guard with no port, towards a point near the Wellspring. The gatherer's port becomes its
// home port, and a Worker that takes another task or none has none. With no task its idle reason is set. True when it
// took a task.
[[nodiscard]] bool SettlementTaskSearch(Figure& _worker, const FigureContext& _context);

// MVP-031: a gatherer manning a resource site's port counts mvp.gather.port_ticks, then picks up mvp.gather.load_ru and
// carries it to the nearest drop-off point of its settlement; the port stays its home port, to which it goes back (ADR-195).
void GatherAtPort(Figure& _worker, const FigureContext& _context);

// MVP-031: a carrier walks along its route to its drop-off point, the Wellspring's door or an active storehouse's
// entrance, and deposits its load; a full store refuses it, and the carrier waits there with the idle reason StoreFull.
void CarryingStep(Figure& _worker, const FigureContext& _context);

} // namespace Species
