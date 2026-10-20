#pragma once

#include "DrawPurpose.h"
#include "Figure.h"
#include "ObstacleGrid.h"
#include "TerrainField.h"
#include "TickDraws.h"
#include "TuningMvp.h"
#include "Vec3.h"

namespace Species
{

/// What the movement rules read of their region (R-SIM-360 to R-SIM-389, ADR-103): the tuning record, the region's
/// terrain and obstacle grid, the tick's draws and the region's square, which is the map of R-SIM-366 and R-SIM-368.
struct MovementContext
{
  const TuningMvp* tuning = nullptr;
  const Neuron::TerrainField* terrain = nullptr;
  const ObstacleGrid* obstacles = nullptr;
  TickDraws* draws = nullptr;
  double minX = 0.0;
  double minZ = 0.0;
  double maxX = 0.0;
  double maxZ = 0.0;

  // R-SIM-369: the bilinear terrain height at (_x, _z).
  [[nodiscard]] double Height(double _x, double _z) const noexcept
  {
    return terrain->Height(_x, _z);
  }
  // On the map: inside the region's square, its edges included.
  [[nodiscard]] bool OnMap(double _x, double _z) const noexcept
  {
    return _x >= minX && _x <= maxX && _z >= minZ && _z <= maxZ;
  }
};

// R-UNIT-042: 1 + (h_now − h_next) ÷ k, clamped to the kind's [f_min, f_max] of table 2.11-A.
[[nodiscard]] double SlopeFactor(FigureKind _kind, double _heightNow, double _heightNext, const TuningMvp& _tuning) noexcept;

// R-SIM-366: x and z clamped into the region's square.
void WorldClamp(Figure& _figure, const MovementContext& _context) noexcept;

// R-SIM-368: the spawn-position search around _requested for the figure _subject: the first of the attempts that lies
// on the map above sea level, or the requested point clamped inside the map edges by the edge margin.
[[nodiscard]] Neuron::Vec3 SpawnPosition(StableId _subject, const Neuron::Vec3& _requested, double _spread,
                                         const MovementContext& _context);

// R-SIM-379 and R-SIM-380 for a figure that is not a Worker: the candidate on the terrain while the figure is on the
// ground, its shore search, then the push out of each structure of its obstacle cell; the shore search draws from
// _shorePurpose. A Leader is tested as a sphere (R-UNIT-160). No Barrier Emitter exists in the MVP, so no kill applies.
[[nodiscard]] Neuron::Vec3 PushFromObstacles(const Figure& _figure, Neuron::Vec3 _candidate, DrawPurpose _shorePurpose,
                                             const MovementContext& _context);
// R-UNIT-112 to R-UNIT-115: the Worker's own push, with its shore search and its structure push; no wall and no
// Barrier Emitter exists in the MVP.
[[nodiscard]] Neuron::Vec3 WorkerPush(const Figure& _figure, Neuron::Vec3 _candidate, DrawPurpose _shorePurpose,
                                      const MovementContext& _context);
// The push of the figure's kind (R-UNIT-040).
[[nodiscard]] Neuron::Vec3 PushFor(const Figure& _figure, const Neuron::Vec3& _candidate, DrawPurpose _shorePurpose,
                                   const MovementContext& _context);

// R-SIM-370 to R-SIM-372: gravity and the integration of an airborne figure, the discarded push, and its landing,
// which starts its in-water count when it comes down at or below sea level.
void AirborneStep(Figure& _figure, const MovementContext& _context);

// R-SIM-376: the swimming step of a figure in water that is not a Worker (R-SIM-377); the drowning damage is the
// caller's, from the result: true when the step's drowning damage applies.
[[nodiscard]] bool SwimStep(Figure& _figure, const MovementContext& _context);

// R-UNIT-105 and R-UNIT-108: the Worker's movement step at its speed, the Idle factor when _idle; true on arrival.
[[nodiscard]] bool WorkerMoveToWaypoint(Figure& _figure, bool _idle, const MovementContext& _context);

// R-UNIT-156 to R-UNIT-158 and R-UNIT-160: the Leader walking to its waypoint, out of formation; true on arrival.
[[nodiscard]] bool LeaderMoveToWaypoint(Figure& _figure, const MovementContext& _context);

} // namespace Species
