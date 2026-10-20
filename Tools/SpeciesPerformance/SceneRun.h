#pragma once

#include "MarkerTimings.h"
#include "WorldSimulation.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Performance
{

/// What one measured scene ran and what its steps cost (ADR-110): its ticks, the wall time of the world's step in each
/// tick on one thread, its figures, its AIs and sides, and the markers' times over the run.
struct SceneRun
{
  std::string name;
  std::uint64_t ticks = 0;
  std::vector<std::int64_t> stepNanoseconds; // one per tick
  std::uint32_t firstFigures = 0;
  std::uint32_t lastFigures = 0;
  double meanFigures = 0.0; // the figures the step advanced, after each tick, averaged
  std::size_t ais = 0;
  std::size_t sides = 0; // the teams that hold figures at the start
  std::vector<Neuron::MarkerTiming> markers;
};

// The figures every region's teams hold, corpses included: what the step advances.
[[nodiscard]] std::uint32_t FiguresOf(const Species::WorldSimulation& _world);

// _world stepped for _ticks ticks from its next tick on the calling thread, as one reference core runs it (ADR-080
// item 1), each step timed and marker timing on. _beforeTick, when given, runs before each tick outside the timing: the
// scene's recorded commands.
[[nodiscard]] SceneRun MeasureWorld(std::string _name, Species::WorldSimulation& _world, std::uint64_t _ticks,
                                    const std::function<void(Species::WorldSimulation&, std::uint64_t)>& _beforeTick = {});

} // namespace Performance
